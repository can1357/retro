#include <retro/analysis/method.hpp>
#include <retro/analysis/workspace.hpp>
#include <retro/analysis/callbacks.hpp>
#include <retro/ir/z3x.hpp>

namespace retro::analysis {
	// Lifts a basic block into the IRP_BUILT IR from the given RVA.
	//
	unique_task<ir::basic_block*> method::build_block(u64 rva) {
		ir::routine* rtn		 = routine[IRP_BUILT].get();
		u64			 img_base = dom->img->base_address;
		u64			 va		 = img_base + rva;

		// Invalid jump if out of image boundaries.
		//
		std::span<const u8> data = dom->img->slice(rva);
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
			for (auto* ins : bbs->insns()) {
				if (ins->ip != va) {
					continue;
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
		stats.block_count++;
		auto* bb	  = rtn->add_block();
		bb->ip	  = va;
		bb->end_ip = va;
		bb->arch	  = arch;
		while (!data.empty()) {
			// Diassemble the instruction, push trap on failure and break.
			//
			arch::minsn ins = {};
			if (!arch->disasm(data, &ins)) {
				bb->push_trap("undefined opcode")->ip = va;
				break;
			}
			stats.minsn_disasm++;

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
				stats.insn_lifted += std::distance(it, bb->end());
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

		auto coerce_const = [&](ir::operand& op) {
			if (op.is_const())
				return true;
			if (auto expr = z3x::to_expr(vs, z3x::get_context(), op)) {
				if (auto v = z3x::value_of(expr); !v.is<void>()) {
					v.type_id = (u64) op.get_type();	 // TODO: Replace with a proper cast.
					op			 = std::move(v);
					return true;
				}
			}
			return false;
		};

		auto* term = bb->terminator();
		switch (term->op) {
			case ir::opcode::xjs: {
				// If condition cannot be coerced to a constant:
				//
				if (auto cc = coerce_const(term->opr(0)); !cc) {
					ir::basic_block* bbs[2] = {nullptr, nullptr};
					for (size_t i = 0; i != 2; i++) {
						// Try coercing destination into a constant.
						//
						if (coerce_const(term->opr(i + 1))) {
							bbs[i] = co_await build_block(term->opr(i + 1).const_val.get_u64() - img_base);
						}
					}

					// If we managed to lift both blocks succesfully:
					//
					if (bbs[0] && bbs[1]) {
						bb = term->block;

						// Replace with js.
						//
						ir::variant cc{term->opr(0)};
						bb->push_js(std::move(cc), bbs[0], bbs[1]);
						term->erase();
						bb->add_jump(bbs[0]);
						bb->add_jump(bbs[1]);
						break;
					}
				} else {
					// Swap with a xjmp.
					//
					ir::variant target{term->opr(term->opr(0).const_val.get<bool>() ? 1 : 2)};
					auto			nterm = bb->push_xjmp(std::move(target));
					std::exchange(term, nterm)->erase();
				}
				// Fallthrough to xjmp handler.
				//
				[[fallthrough]];
			}
			case ir::opcode::xjmp: {
				// Try coercing destination into a constant.
				//
				if (coerce_const(term->opr(0))) {
					// Lift the target block.
					//
					if (auto target = co_await build_block(term->opr(0).const_val.get_u64() - img_base)) {
						bb = term->block;

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
		co_return bb;
	}

	// Lifts a new method into the domain at the given RVA, if it does not already exist.
	// - If there is an existing entry with arch/cc matching, returns it, otherwise clears it and lifts from scratch.
	//
	template<bool Async>
	RC_INLINE static ref<method> lift_impl(domain* dom, u64 rva, arch::handle arch, const arch::call_conv_desc* cc) {
		arch = arch ? arch : dom->arch;
		cc = cc ? cc : dom->default_cc;
		if (!arch || !cc)
			return nullptr;

		ref<method> m;
		{
			std::unique_lock _g{dom->method_map_lock};

			// Return if there is an exact match.
			//
			auto& mfound = dom->method_map[rva];
			if (mfound && mfound->cc == cc && mfound->arch == arch) {
				return mfound;
			}

			// Replace it with our new entry.
			//
			m		  = make_rc<method>();
			m->rva  = rva;
			m->arch = arch;
			m->cc	  = cc;
			m->dom  = dom;
			mfound  = m;
		}

		// Recursively lift starting from the entry point.
		//
		auto rtn					 = make_rc<ir::routine>();
		m->routine[IRP_BUILT] = rtn;
		rtn->method				 = m;
		rtn->ip					 = rva + dom->img->base_address;

		auto work = [=]() {
			// If lifter fails, clear out the routine.
			//
			if (!m->build_block(rva).wait()) {
				m->routine[IRP_BUILT] = nullptr;
			}
			// Otherwise, call the hooks.
			//
			else {
				irp_complete_hook(m->routine[IRP_BUILT], IRP_BUILT);
			}

			// Mark IR phase as finished, return the method.
			//
			m->irp_mask.fetch_or(1u << IRP_BUILT);
			m->irp_mask.notify_all();
		};

		if constexpr (Async) {
			later(std::move(work));
		} else {
			work();
		}
		return m;
	}

	ref<method> lift(domain* dom, u64 rva, arch::handle arch, const arch::call_conv_desc* cc) { return lift_impl<false>(dom, rva, arch, cc); }
	ref<method> lift_async(domain* dom, u64 rva, arch::handle arch, const arch::call_conv_desc* cc) { return lift_impl<true>(dom, rva, arch, cc); }
};
