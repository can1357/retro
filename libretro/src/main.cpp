#include <retro/common.hpp>
#include <retro/platform.hpp>
#include <retro/format.hpp>
#include <retro/diag.hpp>
#include <retro/arch/interface.hpp>
#include <retro/ldr/interface.hpp>
#include <retro/ldr/image.hpp>
#include <retro/ir/insn.hpp>

using namespace retro;


#include <retro/ir/routine.hpp>
#include <retro/arch/x86/regs.hxx>
#include <retro/robin_hood.hpp>

#include <retro/ir/z3x.hpp>



#include <retro/lock.hpp>
#include <retro/directives/pattern.hpp>

namespace retro::analysis {

	struct workspace;

	// Domain statistics.
	//
	struct domain_stats {
		// Machine instructions diassembled.
		//
		std::atomic<u64> minsn_disasm = 0;

		// IR instructions created to represent the disassembled instructions.
		//
		std::atomic<u64> insn_lifted = 0;

		// Blocks parsed.
		//
		std::atomic<u64> block_count = 0;
	};

	// Lifter parameters.
	//
	struct lifter_params {
		arch::handle					 arch = {};
	};

	// Domain represents the analysis state associated with a single image.
	//
	struct domain {
		// Owning workspace.
		//
		weak<workspace> parent = {};

		// Image details.
		//
		ref<ldr::image> img			  = {};
		domain_stats	 stats		  = {};

		// Loader provided defaults.
		//
		arch::handle					 default_arch = {};
		const arch::call_conv_desc* default_cc	  = nullptr;

		// Lifts a routine given the RVA.
		//
		ref<ir::routine> lift_routine(u64 rva, const lifter_params& p);
		ref<ir::routine> lift_routine(u64 rva) { return lift_routine(rva, {.arch=default_arch}); }

		// Given a xcall instruction or the routine determines the calling convention. 
		//
		const arch::call_conv_desc* get_routine_cc(ir::routine* rtn);
		const arch::call_conv_desc* get_routine_cc(ir::insn* xcall);
	};

	// Workspace holds the document state and may represent multiple images.
	//
	struct workspace {
		// Workspace list.
		//
		rw_lock						 domain_list_lock = {};
		std::vector<ref<domain>> domain_list;

		// Creates a new workspace.
		//
		static ref<workspace> create() { return make_rc<workspace>(); }

		// Creates a domain for the image.
		//
		ref<domain> add_image(ref<ldr::image> img) {
			std::unique_lock _g {domain_list_lock};

			auto dom				= domain_list.emplace_back(make_rc<domain>());
			dom->parent			= this;
			dom->img				= img;
			dom->default_arch = arch::instance::lookup(img->arch_hash);
			if (dom->default_arch) {
				dom->default_cc = dom->default_arch->get_cc_desc(img->default_call_conv);
			}
			return dom;
		}
	};

	// Given a routine determines the calling convention.
	//
	const arch::call_conv_desc* domain::get_routine_cc(ir::routine* rtn) {
		// TODO: logic.
		RC_UNUSED(rtn);
		return default_cc;
	}
	const arch::call_conv_desc* domain::get_routine_cc(ir::insn* xcall) {
		// TODO: logic.
		RC_UNUSED(xcall);
		return default_cc;
	}



	static ir::basic_block* lift_block(domain* dom, ir::routine* rtn, u64 va, const lifter_params& p) {
		// Invalid jump if out of image boundaries.
		//
		std::span<const u8> data = dom->img->slice(va - dom->img->base_address);
		if (data.empty()) {
			return nullptr;
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
				return bbs;

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
					return bbs;
				bbs->push_jmp(new_block);
				bbs->add_jump(new_block);

				// Return it.
				//
				return new_block;
			}

			// Misaligned jump, sneaky! Lift as a new block.
			//
			fmt::println("Misaligned jump?");
		}

		// Add a new block.
		//
		dom->stats.block_count++;
		auto* bb	  = rtn->add_block();
		bb->ip	  = va;
		bb->end_ip = va;
		bb->arch	  = p.arch;
		while (!data.empty()) {
			// Diassemble the instruction, push trap on failure and break.
			//
			arch::minsn ins = {};
			if (!p.arch->disasm(data, &ins)) {
				bb->push_trap("undefined opcode")->ip = va;
				break;
			}
			dom->stats.minsn_disasm++;

			// Update the block range.
			//
			bb->end_ip = va + ins.length;

			// Lift the instruction, push trap on failure and break.
			//
			if (auto err = p.arch->lift(bb, ins, va)) {
				bb->push_trap("lifter error: " + err.to_string())->ip = va;
				break;
			}
			if (!bb->empty()) {
				auto it = std::prev(bb->end());
				while (it != bb->begin() && it->prev->ip == va) {
					--it;
				}
				dom->stats.insn_lifted += std::distance(it, bb->end());
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
		z3::context ctx;
		z3x::variable_set vs;

		auto coerce_const = [&](ir::operand& op) {
			if (op.is_const())
				return true;
			if (auto expr = z3x::to_expr(vs, ctx, op)) {
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
							bbs[i] = lift_block(dom, rtn, (u64) term->opr(i + 1).const_val.get<ir::pointer>(), p);
						}
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
				} else {
					// Swap with a xjmp.
					//
					ir::variant target{term->opr(term->opr(0).const_val.get<bool>() ? 1 : 2)};
					auto nterm = bb->push_xjmp(std::move(target));
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
					if (auto target = lift_block(dom, rtn, (u64) term->opr(0).const_val.get<ir::pointer>(), p)) {
						// Replace with jmp if successful.
						//
						bb->push_jmp(target);
						term->erase();
						bb->add_jump(target);
					}
				} else {
					fmt::println("dynamic jump, TODO.");
				}
				break;
			}
			default:
				break;
		}
		return bb;
	}
	ref<ir::routine> domain::lift_routine(u64 rva, const lifter_params& p) {
		if (!p.arch)
			return nullptr;
		
		// Create the routine.
		//
		ref<ir::routine> rtn = make_rc<ir::routine>();
		rtn->dom					= this;
		rtn->ip					= rva + img->base_address;

		// Recursively lift starting from the entry point.
		//
		if (!lift_block(this, rtn, rva + img->base_address, p)) {
			return nullptr;
		}
		return rtn;
	}
};


namespace retro::ir::opt {
	// Utility functions.
	//
	namespace util {
		// Checks IR values for equality.
		//
		struct identity_check_record {
			identity_check_record* prev = nullptr;
			const value*			  a;
			const value*			  b;
		};
		inline bool is_identical(const operand& a, const operand& b, identity_check_record* prev = nullptr);
		inline bool is_identical(const value* a, const value* b, identity_check_record* prev = nullptr) {
			// If same value, assume equal.
			// If different types, assume non-equal.
			//
			if (a == b)
				return true;
			if (a->get_type() != b->get_type())
				return false;

			// If in the equality check stack already, assume equal. (?)
			//
			for (auto it = prev; it; it = it->prev)
				if (it->a == a && it->b == b)
					return true;

			// If both are instructions:
			//
			auto ai = a->get_if<insn>();
			auto bi = b->get_if<insn>();
			if (ai && bi && ai->op == bi->op && ai->template_types == bi->template_types) {
				// If there are side effects or instruction is not pure, assume false.
				//
				auto& desc = ai->desc();
				if (desc.side_effect || !desc.is_pure)
					return false;

				// If non-constant instruction:
				//
				if (desc.is_const) {
					// TODO: Trace until common dominator to ensure no side effects.
				}

				// If opcode is the same and operand size matches:
				//
				if (ai->op == bi->op && ai->operand_count == bi->operand_count) {
					// Make a new record to fix infinite-loops with PHIs.
					//
					identity_check_record rec{prev, a, b};

					// If all values are equal, pass the check.
					//
					for (size_t i = 0; i != ai->operand_count; i++)
						if (!is_identical(ai->opr(i), bi->opr(i), &rec))
							return false;
					return true;
				}
			}
			return false;
		}
		inline bool is_identical(const operand& a, const operand& b, identity_check_record* prev) {
			if (a.is_const()) {
				return b.is_const() && a.get_const().equals(b.get_const());
			} else {
				return !b.is_const() && is_identical(a.get_value(), b.get_value(), prev);
			}
		}
	};

	// Local dead code elimination.
	//
	static size_t dce(basic_block* bb) {
#if RC_DEBUG
		bb->validate().raise();
#endif
		return bb->rerase_if([](insn* i) { return !i->uses() && !i->desc().side_effect; });
	}

	// Local register move propagation.
	//
	static size_t reg_move_prop(basic_block* bb) {
		size_t n = 0;

		// For each instruction:
		//
		robin_hood::unordered_flat_map<u32, std::pair<ref<insn>, bool>> producer_map = {};
		for (auto* ins : bb->insns()) {
			// If producing a value:
			if (ins->op == opcode::write_reg) {
				auto r = ins->opr(0).get_const().get<arch::mreg>();

				// If there's another producer, remove it from the stream, declare ourselves as the last one.
				auto& [lp, lpw] = producer_map[r.uid()];
				if (lp && lpw) {
					n++;
					lp->erase();
				}
				lpw = true;
				lp	 = ins;
			}
			// If requires a value:
			else if (ins->op == opcode::read_reg) {
				auto r = ins->opr(0).get_const().get<arch::mreg>();

				// If there's a cached producer of this value:
				auto& [lp, lpw] = producer_map[r.uid()];
				if (lp) {
					variant lpv;
					if (lpw) {
						lpv = variant{lp->opr(1)};
					} else {
						lpv = weak<value>{lp};
					}

					// If the type matches:
					if (auto ty = lpv.get_type(); ty == ins->template_types[0]) {
						// Replace uses with the assigned value.
						n += 1 + ins->replace_all_uses_with(lpv);
						ins->op = opcode::nop;
					}
					// Otherwise:
					else {
						// Insert a bitcast right before the read.
						auto bc = bb->insert(ins, make_bitcast(ins->template_types[0], lpv));
						// Replace uses with the result value.
						n += 1 + ins->replace_all_uses_with(bc.get());
					}
				}
				// Otherwise, declare ourselves as the producer to deduplicate.
				else {
					lp	 = ins;
					lpw = false;
				}
			}
			// Clear current state if we hit an instruction with unknown register use.
			//
			else if (ins->desc().unk_reg_use) {
				producer_map.clear();
			}
		}

		// Run DCE if there were any changes, return the change count.
		return n ? n + dce(bb) : n;
	}

	// Local constant folding.
	//
	static size_t const_fold(basic_block* bb) {
		size_t n = 0;

		// For each instruction:
		//
		for (auto* ins : bb->insns()) {
			// If we can evaluate the instruction in a constant manner, do so.
			//
			if (ins->op == opcode::binop || ins->op == opcode::cmp) {
				auto& opc = ins->opr(0);
				auto& lhs = ins->opr(1);
				auto& rhs = ins->opr(2);
				if (lhs.is_const() && rhs.is_const()) {
					if (auto res = lhs.get_const().apply(opc.get_const().get<op>(), rhs.get_const()); !res.is<void>()) {
						n += 1 + ins->replace_all_uses_with(res);
					}
				}
			} else if (ins->op == opcode::unop) {
				auto& opc = ins->opr(0);
				auto& lhs = ins->opr(1);
				if (lhs.is_const()) {
					if (auto res = lhs.get_const().apply(opc.get_const().get<op>()); !res.is<void>()) {
						n += 1 + ins->replace_all_uses_with(res);
					}
				}
			} else if (ins->op == opcode::cast) {
				auto	into = ins->template_types[1];
				auto& val  = ins->opr(0);
				if (val.is_const()) {
					if (auto res = val.get_const().cast_zx(into); !res.is<void>()) {
						n += 1 + ins->replace_all_uses_with(res);
					}
				}
			} else if (ins->op == opcode::cast_sx) {
				auto	into = ins->template_types[1];
				auto& val  = ins->opr(0);
				if (val.is_const()) {
					if (auto res = val.get_const().cast_sx(into); !res.is<void>()) {
						n += 1 + ins->replace_all_uses_with(res);
					}
				}
			} else if (ins->op == opcode::select) {
				auto	into = ins->template_types[1];
				auto& val  = ins->opr(0);
				if (val.is_const()) {
					n += 1 + ins->replace_all_uses_with(ins->opr(val.const_val.get<bool>() ? 1 : 2));
				}
			}
		}

		// Run DCE if there were any changes, return the change count.
		return n ? n + dce(bb) : n;
	}

	// Local identical value folding.
	//
	static size_t id_fold(basic_block* bb) {
		size_t n = 0;

		// For each instruction:
		//
		for (insn* ins : view::reverse(bb->insns())) {
			// For each instruction before it:
			//
			for (insn* ins2 : bb->rslice(ins)) {
				// If identical, replace.
				//
				if (util::is_identical(ins, ins2)) {
					n += 1 + ins->replace_all_uses_with(ins2);
					break;
				}

				// If instruction has side effects and the target is not const, fail.
				//
				if (ins2->desc().side_effect && !ins->desc().is_const) {
					break;
				}
			}
		}

		// Run DCE if there were any changes, return the change count.
		return n ? n + dce(bb) : n;
	}

	// Simple instruction combination rules.
	//
	static size_t ins_combine(basic_block* bb) {
		size_t n = 0;
		for (auto it = bb->begin(); it != bb->end();) {
			auto ins = it++;

			// Numeric rules:
			//
			if (ins->op == opcode::binop || ins->op == opcode::unop || ins->op == opcode::cmp) {
				for (auto& match : directives::replace_list) {
					pattern::match_context ctx{};
					if (match(ins, ctx)) {
						n++;
						break;
					}
				}
			}
			// Casts.
			//
			else if (ins->op == opcode::bitcast) {
				// If cast between same types, no op.
				//
				auto ty1 = ins->template_types[0];
				auto ty2 = ins->template_types[1];
				if (ty1 == ty2) {
					ins->replace_all_uses_with(ins->opr(0));
					ins->erase();
					n++;
					continue;
				}

				// If RHS is not an instruction, nothing else to match.
				//
				auto& rhsv = ins->opr(0);
				if (rhsv.is_const())
					continue;
				auto rhs = rhsv.get_value()->get_if<insn>();
				if (!rhs)
					continue;

				// If RHS is also a bitcast, propagate.
				//
				if (rhs->op == opcode::bitcast) {
					ins->template_types[0] = rhs->opr(0).get_type();
					ins->opr(0)				  = rhs->opr(0);
					n++;
				}
			}
			else if (ins->op == opcode::cast || ins->op == opcode::cast_sx) {
				// If cast between same types, no op.
				//
				auto ty1 = ins->template_types[0];
				auto ty2 = ins->template_types[1];
				if (ty1 == ty2) {
					n += 1 + ins->replace_all_uses_with(ins->opr(0));
					ins->erase();
					continue;
				}

				// If RHS is not an instruction, nothing else to match.
				//
				auto& rhsv = ins->opr(0);
				if (rhsv.is_const())
					continue;
				auto rhs = rhsv.get_value()->get_if<insn>();
				if (!rhs)
					continue;

				// If RHS is also a cast:
				//
				if (rhs->op == opcode::cast || rhs->op == opcode::cast_sx) {
					auto& val = rhs->opr(0);
					auto	ty0  = rhs->template_types[0];
					auto& ti0 = enum_reflect(ty0);
					auto& ti1 = enum_reflect(ty1);
					auto& ti2 = enum_reflect(ty2);

					// If cast between same type kinds:
					//
					if (ti0.kind == ti1.kind && ti1.kind == ti2.kind && ti0.lane_width == ti1.lane_width && ti1.lane_width == ti2.lane_width) {
						bool sx0_1 = rhs->op == opcode::cast_sx;
						bool sx1_2 = ins->op == opcode::cast_sx;

						// If no information lost during middle translation:
						//
						if (ti1.bit_size >= ti0.bit_size) {
							// [e.g. i32->i32]
							//
							if (ti2.bit_size == ti0.bit_size) {
								n += 1 + ins->replace_all_uses_with(val);
								ins->erase();
								continue;
							}
							// [e.g. i16->i32]
							//
							else if (ti2.bit_size < ti0.bit_size) {
								ins->template_types[0] = val.get_type();
								rhsv						  = val;
								continue;
							}
						}

						//fmt::println(rhs->to_string());
						//fmt::println(ins->to_string());
						//fmt::println(ty0, sx0_1 ? "->s" : "->", ty1, sx1_2 ? "->s" : "->", ty2);
					}
				} else {
					// TODO: convert unop/binop s
				}
			}
			// Nops.
			//
			else if (ins->op == opcode::select) {
				if (util::is_identical(ins->opr(1), ins->opr(2))) {
					ins->replace_all_uses_with(ins->opr(1));
				}
			}

			// TODO:
			// cast simplification
			// write_reg with op == read_reg
		}

		// Run DCE if there were any changes, return the change count.
		return n ? n + dce(bb) : n;
	}

	// Converts xcall/xret into call/ret where possible.
	//
	static size_t apply_cc_info(routine* rtn) {
		// Get domain for CC analysis.
		//
		auto* dom = rtn->dom.get();
		if (!dom)
			return 0;

		// For each instruction.
		//
		size_t n = 0;
		for (auto& bb : rtn->blocks) {
			n += bb->erase_if([&](insn* i){
				// If XCALL:
				//
				if (i->op == opcode::xcall) {
					// Determine the calling convention.
					//
					auto* cc = dom->get_routine_cc(i);
					if (!cc)
						return false;

					// First read each argument:
					//
					auto args = bb->insert(i, make_undef(type::context));
					auto push_reg = [&](arch::mreg a) {
						if (a) {
							auto val = bb->insert(i, make_read_reg(enum_reflect(a.get_kind()).type, a));
							args		= bb->insert(i, make_insert_context(args.get(), a, val.get()));
						}
					};
					range::for_each(cc->argument_gpr, push_reg);
					range::for_each(cc->argument_fp, push_reg);
					push_reg(cc->fp_varg_counter);

					// Create the call.
					//
					auto res = bb->insert(i, make_call(i->opr(0), args.get()));

					// Read each result.
					//
					auto pop_reg = [&](arch::mreg a) {
						if (a) {
							auto& desc = enum_reflect(a.get_kind());
							auto	val  = bb->insert(i, make_extract_context(desc.type, res.get(), a));

							bb->insert(i, make_write_reg(a, val.get()));

							auto ainfo = i->arch->get_register_info(a);
							i->arch->for_each_subreg(ainfo.full_reg, [&](arch::mreg sr) {
								if (sr != a) {
									auto sinfo	= i->arch->get_register_info(sr);
									auto sub_ty = int_type(sinfo.bit_width);

									// Shift and truncate.
									//
									insn* sub_val;
									if (sinfo.bit_offset) {
										sub_val = bb->insert(i, make_binop(op::bit_shr, val.get(), constant(val->get_type(), sinfo.bit_offset)));
										sub_val = bb->insert(i, make_cast(sub_ty, sub_val));
									} else {
										sub_val = bb->insert(i, make_cast(sub_ty, val.get()));
									}

									// Write.
									//
									bb->insert(i, make_write_reg(sr, sub_val));
								}
							});
						}
					};
					range::for_each(cc->retval_gpr, pop_reg);
					range::for_each(cc->retval_fp, pop_reg);
					// stack if sp_caller_adjusted?
					// eflags?

					return true;
				}
				// If XRET:
				//
				else if (i->op == opcode::xret) {
					// Determine the calling convention.
					//
					auto* cc = dom->get_routine_cc(rtn);
					if (!cc)
						return false;

					// Read each result:
					//
					auto args	  = bb->insert(i, make_undef(type::context));
					auto push_reg = [&](arch::mreg a) {
						if (a) {
							auto val = bb->insert(i, make_read_reg(enum_reflect(a.get_kind()).type, a));
							args		= bb->insert(i, make_insert_context(args.get(), a, val.get()));
						}
					};
					range::for_each(cc->retval_gpr, push_reg);
					range::for_each(cc->retval_fp, push_reg);

					// Create the ret.
					//
					bb->insert(i, make_ret(args.get()));
					return true;
				}
				return false;
			});
		}
		return n;
	}







	// The following algorithm is a modified version adapted from the paper:
	// Braun, M., Buchwald, S., Hack, S., Leißa, R., Mallon, C., Zwinkau, A. (2013). Simple and Efficient Construction of Static Single Assignment Form.
	// In: Jhala, R., De Bosschere, K. (eds) Compiler Construction. CC 2013. Lecture Notes in Computer Science, vol 7791. Springer, Berlin, Heidelberg.
	//
	static variant read_variable_local(arch::mreg r, basic_block* b, insn* before, bool* fail) {
		for (insn* ins : b->rslice(before)) {
			// Write reg:
			//
			if (ins->op == opcode::write_reg && ins->opr(0).get_const().get<arch::mreg>() == r) {
				return variant{ins->opr(1)};
			}
			// Possible implicit write:
			//
			else if (ins->desc().unk_reg_use) {
				if (fail) {
					*fail = true;
					return {};
				}
				auto ty = enum_reflect(r.get_kind()).type;  // TODO: Might be unknown type.
				return b->insert_after(ins, make_read_reg(ty, r)).get();
			}
		}
		return {};
	}
	static insn* reread_variable_local(arch::mreg r, basic_block* b, insn* until) {
		for (insn* ins : b->rslice(until)) {
			// Read reg:
			//
			if (ins->op == opcode::read_reg && ins->opr(0).get_const().get<arch::mreg>() == r) {
				return ins;
			}
		}
		return nullptr;
	}
	static variant read_variable_recursive(arch::mreg r, basic_block* b, insn* until);
	static variant read_variable(arch::mreg r, basic_block* b, insn* until) {
		bool fail = false;
		if (auto i = read_variable_local(r, b, until, &fail)) {
			return i;
		} else if (!fail) {
			return read_variable_recursive(r, b, until);
		} else {
			return {};
		}
	}
	static variant try_remove_trivial_phi(ref<insn> phi) {
		const operand* same = nullptr;
		for (auto& op : phi->operands()) {
			if (!op.is_const() && op.get_value() == phi) {
				continue;
			}
			if (same) {
				if (op != *same) {
					return phi.get();
				}
			} else {
				same = &op;
			}
		}
		RC_ASSERT(same);
		variant same_v{*same};

		std::vector<ref<insn>> phi_users;
		for (auto u : phi->uses()) {
			if (auto& i = u->user->get<insn>(); i.op == opcode::phi) {
				phi_users.emplace_back(&i);
			}
		}
		phi->replace_all_uses_with(same_v);
		phi->erase();

		for (auto& p : phi_users)
			if (!p->is_orphan())
				try_remove_trivial_phi(std::move(p));
		return same_v;
	}

	static variant read_variable_recursive(arch::mreg r, basic_block* b, insn* until) {
		auto ty = enum_reflect(r.get_kind()).type;  // TODO: Might be unknown type.

		// Actually load the value if it does not exist.
		//
		if (b->predecessors.empty()) {
			auto v = read_variable_local(r, b, until, nullptr);
			if (!v) {
				auto i = reread_variable_local(r, b, until);
				if (!i) {
					i = b->insert(b->begin(), make_read_reg(ty, r)).get();
				}
				v = i;
			}
			return v;
		}
		// No PHI needed.
		//
		else if (b->predecessors.size() == 1) {
			b = b->predecessors.front().get();
			return read_variable(r, b, b->end());
		}
		// Create a PHI recursively.
		//
		else {
			// Create an empty phi.
			//
			auto phi = insn::allocate(opcode::phi, {ty}, b->predecessors.size());  // TODO :(
			b->insert(b->begin(), phi);

			// Create a temporary store to break cycles.
			//
			auto tmp = b->insert(b->end_phi(), make_write_reg(r, phi));

			// For each predecessor, append a PHI node.
			//
			for (size_t n = 0; n != b->predecessors.size(); n++) {
				auto bb = b->predecessors[n].get();
				auto v  = read_variable(r, bb, bb->end());
				phi->opr(n) = v;
			}

			// Delete the temporary.
			//
			tmp->erase();
			return try_remove_trivial_phi(std::move(phi));
		}
	}

	// Lowers reads of registers into PHI nodes.
	//
	static size_t reg_to_phi(routine* rtn) {
		rtn->topological_sort();

		// Generate PHIs to replace read_reg in every block except the entry point.
		//
		size_t n = 0;
		for (auto& bb : view::reverse(rtn->blocks)) {
			n += bb->erase_if([&](insn* ins) {
				if (ins->op == opcode::read_reg) {
					auto r = ins->opr(0).const_val.get<arch::mreg>();
					auto v = read_variable(r, bb.get(), ins);
					if (!v.is_null()) {
						if (v.is_const() || v.get_value() != ins) {
							if (v.get_type() != ins->get_type()) {
								v = bb->insert(ins, make_bitcast(ins->get_type(), std::move(v))).get();
							}
							ins->replace_all_uses_with(std::move(v));
							return true;
						}
					}
				}
				return false;
			});
		}

		// Break out if theres instructions with unknown register use.
		//
		for (auto& bb : view::reverse(rtn->blocks)) {
			for (auto* ins : bb->insns()) {
				if (ins->desc().unk_reg_use)
					return false;
			}
		}

		// Otherwise, remove all write_regs.
		//
		for (auto& bb : view::reverse(rtn->blocks)) {
			n += bb->erase_if([](auto i) { return i->op == opcode::write_reg; });
			n += dce(bb);
		}
		return n;
	}
};


static std::vector<u8> compile(std::string code, const char* args) {
#if RC_WINDOWS
	code.insert(0, "#define EXPORT __declspec(dllexport)\nint main() {}\n");
#else
	code.insert(0, "#define EXPORT __attribute__((visibility(\"default\")))\nint main() {}\n")
#endif

	// Create the temporary paths.
	//
	auto tmp_dir  = std::filesystem::temp_directory_path();
	auto in		  = tmp_dir / "retrotmp.c";
	auto out		  = tmp_dir / "retrotmp.exe";

	// Write the file.
	//
	platform::write_file(in, {(const u8*) code.data(), code.size()});

	// Create and execute the command.
	//
	auto output = platform::exec(fmt::str("%%LLVM_PATH%%/bin/clang \"%s\" -fms-extensions -o \"%s\" %s", in.string().c_str(), out.string().c_str(), args));

	// Read the file.
	//
	bool ok;
	auto result = platform::read_file(out, ok);
	if (result.empty()) {
		fmt::abort("failed to compile the code:\n%s\n", output.c_str());
	}
	return result;
}

void do_stuff(ref<ldr::image> img) {
	// Create the workspace.
	//
	auto ws = analysis::workspace::create();

	// Load the image.
	//
	auto machine = arch::instance::lookup(img->arch_hash);
	auto loader	 = ldr::instance::lookup(img->ldr_hash);
	RC_ASSERT(loader && machine);
	fmt::println("-> loader:  ", loader->get_name());
	fmt::println("-> machine: ", machine->get_name());

	// Add entry point symbol if none present, create the domain.
	//
	if (img->symbols.empty()) {
		img->symbols.push_back({img->base_address, "entry point"});
	}
	auto dom = ws->add_image(std::move(img));

	for (auto& sym : dom->img->symbols) {
		// Demo.
		//
		auto rtn = dom->lift_routine(sym.rva);

		// Print statistics.
		//
		printf(RC_WHITE " ----- Routine '%s' -------\n", sym.name.data());
		printf(RC_GRAY " # Successfully lifted " RC_VIOLET "%llu" RC_GRAY " instructions into " RC_GREEN "%llu" RC_RED " (avg: ~%llu/ins) #\n" RC_RESET,
			 dom->stats.minsn_disasm.load(), dom->stats.insn_lifted.load(), dom->stats.insn_lifted.load() / dom->stats.minsn_disasm.load());

		// Run simple optimizations.
		//
		size_t n = 0;
		for (auto& bb : rtn->blocks) {
			n += ir::opt::reg_move_prop(bb);
			n += ir::opt::const_fold(bb);
			n += ir::opt::id_fold(bb);
			n += ir::opt::ins_combine(bb);
			n += ir::opt::const_fold(bb);
			n += ir::opt::id_fold(bb);
		}
		n += ir::opt::apply_cc_info(rtn);
		for (auto& bb : rtn->blocks) {
			n += ir::opt::reg_move_prop(bb);
			n += ir::opt::const_fold(bb);
			n += ir::opt::id_fold(bb);
			n += ir::opt::ins_combine(bb);
			n += ir::opt::const_fold(bb);
			n += ir::opt::id_fold(bb);
		}
		n += ir::opt::reg_to_phi(rtn);
		for (auto& bb : rtn->blocks) {
			n += ir::opt::const_fold(bb);
			n += ir::opt::id_fold(bb);
			n += ir::opt::ins_combine(bb);
			n += ir::opt::const_fold(bb);
			n += ir::opt::id_fold(bb);
		}
		rtn->rename_insns();
		printf(RC_GRAY " # Optimized " RC_RED "%llu " RC_GRAY "instructions.\n", n);
		fmt::println(rtn->to_string());
	}
}

#include <retro/utf.hpp>

int main(int argv, const char** args) {
	platform::setup_ansi_escapes();

	/*
			  $0: [140001010 => 140001016]
					 %0 = read_reg.i32 edx
					 %1 = read_reg.i32 ecx
					 %2 = read_reg.i64 rdx
					 %3 = read_reg.i8x16 xmm0
					 %4 = cast.i32.i64 %1                 ; 0000000140001010
					 %5 = cast.i64.i32 %4
					 %6 = cmp.i32 eq, %0, 0               ; 0000000140001012
					 js %6, $2, $1                        ; 0000000140001014
		  $1: [140001016 => 140001027]
					 %8 = phi.i32 %13, %0
					 %9 = phi.i32 %10, %5
					 %b = cast.i32.i64 %9                 ; 0000000140001020
					 %c = cast.i32.i64 %8
					 %d = binop.i64 mul, %b, %c
					 %e = cast.i64.i32 %d <--- missed opt
					 %f = cast.i32.i64 %e <--- missed opt
					 %10 = cast.i64.i32 %f <--- missed opt
					 %11 = binop.i32 sub, %8, 1           ; 0000000140001023
					 %12 = cast.i32.i64 %11
					 %13 = cast.i64.i32 %12
					 %14 = cmp.i32 eq, %11, 0
					 js %14, $2, $1                       ; 0000000140001025
		  $2: [140001027 => 140001028]
					 %17 = phi.i64 %12, %2
					 %18 = phi.i64 %f, %4
					 %19 = undef.context
					 %1a = insert_context.i64 %19, rax, %18
					 %1b = insert_context.i64 %1a, rdx, %17
					 %1c = insert_context.i8x16 %1b, xmm0, %16 <--- missed opt
					 ret %1c

	*/

	std::string test_file = "S:\\Projects\\Retro\\tests\\simple.c";
	if (argv > 1) {
		test_file = args[1];
	}

	bool ok;
	auto code = platform::read_file(test_file, ok);
	if (!ok) {
		fmt::abort("failed to read the file.");
	}

	auto bin = compile(utf::convert<char>(code), "-O1");
	do_stuff(ldr::load_from_memory(bin).value());
	return 0;



	//z3::context c;
	//auto			termcc = bb->back()->opr(0).get_value();
	//fmt::println(termcc->to_string());
	//
	//bb->push_nop();
	//list::iterator insert_here_plz = bb->push_nop();
	//
	//bb->push_nop();
	//bb->push_nop();
	//bb->push_nop();
	//
	//{
	//	auto a = c.bv_const("a", 64);
	//	auto b = c.bv_const("b", 64);
	//
	//	auto sl = a < 0;
	//	auto sr	= b < 0;
	//	auto sf	= (a - b) < 0;
	//	auto of	= (sl != sr) & (sl != sf);
	//
	//	z3::solver s(c);
	//	//s.add((of != sf) == (a < b));
	//	switch (s.check()) {
	//		case z3::unsat:
	//			printf("unsat\n");
	//			break;
	//		case z3::sat:
	//			printf("sat\n");
	//			break;
	//		case z3::unknown:
	//			printf("unknown\n");
	//			break;
	//	}
	//}
	//
	//
	//z3x::variable_set vs;
	//if (auto expr = z3x::to_expr(vs, c, termcc)) {
	//	fmt::println(expr);
	//	fmt::println("------------");
	//	expr = expr.simplify();
	//	fmt::println(expr);
	//
	//	//z3::tactic tac(c, "ctx-simplify");
	//	z3::tactic tac(c, "ctx-solver-simplify");
	//	z3::goal	  g(c);
	//	g.add(expr);
	//	auto res = tac(g);
	//	fmt::println(res[0].as_expr());
	//
	//	fmt::println(z3x::from_expr(vs, expr, bb, insert_here_plz));
	//}
	//
	//fmt::println(bb->to_string());
}