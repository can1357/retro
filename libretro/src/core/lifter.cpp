#include <retro/core/method.hpp>
#include <retro/core/workspace.hpp>
#include <retro/core/image.hpp>
#include <retro/core/callbacks.hpp>
#include <retro/ir/z3x.hpp>

namespace retro::core {
	// Helper for coercing an operand into a constant.
	//
	static ir::constant* coerce_const(z3x::variable_set& vs, ir::operand& op) {
		if (op.is_const())
			return &op.const_val;
		if (auto expr = z3x::to_expr(vs, z3x::get_context(), op)) {
			if (auto v = z3x::value_of(expr, true)) {
				v = v.bitcast(op.get_type());
				RC_ASSERT(!v.is<void>());
				op = std::move(v);
				return &op.const_val;
			}
		}
		return nullptr;
	}

	// Lifts a basic block into the IRP_INIT IR from the given RVA.
	//
	unique_task<ir::basic_block*> method::build_block(u64 rva) {
		ir::routine* rtn		 = routine[IRP_INIT].get();
		u64			 img_base = img->base_address;
		u64			 va		 = img_base + rva;

		
		// Invalid jump if out of image boundaries.
		//
		std::span<const u8> data = img->slice(rva);
		if (data.empty()) {
			co_return nullptr;
		}

		// First check the basic block list for an already lifted range.
		//
		for (ir::basic_block* bbs : rtn->blocks) {
			// Skip if not within the range.
			//
			if (va < bbs->ip || bbs->end_ip <= va)
				continue;

			// If exact match, no need to lift anything.
			//
			if (bbs->ip == va)
				co_return bbs;

			// Find the label.
			//
			auto non_nop = false;
			for (auto* ins : bbs->insns()) {
				if (ins->ip != va) {
					non_nop = non_nop || ins->op != ir::opcode::nop;
					continue;
				}

				// No need to split if the instructions we've skipped have no effect.
				//
				if (!non_nop) {
					co_return bbs;
				}

				// Split the block, add a jump from the previous block to this one.
				//
				auto new_block = bbs->split(ins);
				if (!new_block)
					co_return bbs;
				bbs->push_jmp(new_block);
				bbs->add_jump(new_block);

				// Return it.
				//
				co_return new_block;
			}

			// Misaligned jump, sneaky! Lift as a new block.
			//
			fmt::println("Misaligned jump?");
		}

		// Add a new block.
		//
		init_info.stats_block_count++;
		auto* bb	  = rtn->add_block();
		bb->ip	  = va;
		bb->end_ip = va;
		bb->arch	  = arch;

		// If entry point, add a stack_begin opcode.
		//
		if (bb == rtn->get_entry()) {
			auto frame = bb->push_bitcast(ir::int_type(arch->get_pointer_width()), bb->push_stack_begin());
			arch->explode_write_reg(bb->push_write_reg(arch->get_stack_register(), frame));
		}

		// Until we run out of instructions to decode:
		//
		while (!data.empty()) {
			// Diassemble the instruction, push trap on failure and break.
			//
			arch::minsn ins = {};
			if (!arch->disasm(data, &ins)) {
				bb->push_trap("undefined opcode")->ip = va;
				break;
			}
			init_info.stats_minsn_disasm++;

			// Update the block range.
			//
			bb->end_ip = va + ins.length;

			// Lift the instruction, push trap on failure and break.
			//
			if (auto err = arch->lift(bb, ins, va)) {
				bb->push_trap("lifter error: " + err.to_string())->ip = va;
				// TODO: Log
				fmt::println(err.to_string());
				break;
			}
			if (!bb->empty()) {
				auto it = std::prev(bb->end());
				while (it != bb->begin() && it->prev->ip == va) {
					--it;
				}
				init_info.stats_insn_lifted += std::distance(it, bb->end());
			}

			// If XCALL:
			//
			if (auto i = bb->back(); i && i->op == ir::opcode::xcall) {
				// Coerce first operand into a constant where possible.
				//
				z3x::variable_set vs;
				coerce_const(vs, i->opr(0));

				// Invoke the callbacks.
				//
				on_irp_init_xcall(i);
			}

			// If last instruction is a terminator break out.
			//
			if (bb->terminator() != nullptr)
				break;

			// Skip the bytes and increment IP.
			//
			data = data.subspan(ins.length);
			va += ins.length;
		}

		// Try to continue traversal.
		//
		z3x::variable_set vs;
		auto*					bb_ret = bb;
		auto*					term = bb->terminator();
		switch (term->op) {
			case ir::opcode::xjs: {
				// Try lifting both blocks.
				//
				ir::basic_block* bbs[2] = {nullptr, nullptr};
				for (size_t i = 0; i != 2; i++) {
					bbs[i] = co_await build_block(term->opr(i + 1).const_val.get_u64() - img_base);
					bb		 = term->bb;
				}

				// If we managed to lift both blocks succesfully:
				//
				if (bbs[0] && bbs[1]) {
					// Replace with js.
					//
					ir::variant cc{term->opr(0)};
					bb->push_js(std::move(cc), bbs[0], bbs[1]);
					term->erase();
					bb->add_jump(bbs[0]);
					bb->add_jump(bbs[1]);
				}
				break;
			}
			case ir::opcode::xjmp: {
				// Try coercing destination into a constant.
				//
				if (coerce_const(vs, term->opr(0))) {
					// Lift the target block.
					//
					if (auto target = co_await build_block(term->opr(0).const_val.get_u64() - img_base)) {
						bb = term->bb;

						// Replace with jmp if successful.
						//
						bb->push_jmp(target);
						term->erase();
						bb->add_jump(target);
					}
				}
				// Call indirect jump resolvers.
				//
				else {
					indirect_xjmp_resolver(this, term);
				}
				break;
			}
			default:
				break;
		}
		co_return bb_ret;
	}

	// Lifts a new method into the image at the given RVA, if it does not already exist.
	// - If there is an existing entry with arch/cc matching, returns it, otherwise clears it and lifts from scratch.
	//
	template<bool Async>
	RC_INLINE static ref<method> lift_impl(image* img, u64 rva, arch::handle arch, const arch::call_conv_desc* cc) {
		arch = arch ? arch : img->arch;
		if (!arch)
			return nullptr;
		cc = cc ? cc : arch->get_cc_desc(img->default_cc);
		if (!cc)
			return nullptr;

		ref<method> m;
		{
			std::unique_lock _g{img->method_map_lock};

			// Return if there is an exact match.
			//
			auto& mfound = img->method_map[rva];
			if (mfound && mfound->cc == cc && mfound->arch == arch) {
				return mfound;
			}

			// Replace it with our new entry.
			//
			m		  = make_rc<method>();
			m->rva  = rva;
			m->arch = arch;
			m->cc	  = cc;
			m->img  = img;
			mfound  = m;
		}

		// Recursively lift starting from the entry point.
		//
		auto rtn					 = make_rc<ir::routine>();
		m->routine[IRP_INIT] = rtn;
		rtn->method				 = m;
		rtn->ip					 = rva + img->base_address;

		auto work = [=]() {
			// If lifter fails, clear out the routine.
			//
			if (!m->build_block(rva).wait()) {
				m->routine[IRP_INIT] = nullptr;
			}
			// Otherwise, call the hooks.
			//
			else {
				on_irp_complete(m->routine[IRP_INIT], IRP_INIT);
			}

			// Mark IR phase as finished, return the method.
			//
			m->irp_mask.fetch_or(1u << IRP_INIT);
			m->irp_mask.notify_all();
		};

		if constexpr (Async) {
			later(std::move(work));
		} else {
			work();
		}
		return m;
	}

	ref<method> lift(image* img, u64 rva, arch::handle arch, const arch::call_conv_desc* cc) { return lift_impl<false>(img, rva, arch, cc); }
	ref<method> lift_async(image* img, u64 rva, arch::handle arch, const arch::call_conv_desc* cc) { return lift_impl<true>(img, rva, arch, cc); }
};
