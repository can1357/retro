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
			if (ai && bi && ai->op == bi->op) {
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
					n += 1 + lp->replace_all_uses_with(ins);
					lp->erase();
				}
				lpw = true;
				lp = ins;
			}
			// If requires a value:
			else if (ins->op == opcode::read_reg) {
				auto r = ins->opr(0).get_const().get<arch::mreg>();

				// If there's a cached producer of this value:
				auto& [lp, lpw] = producer_map[r.uid()];
				if (lp) {
					ir::variant lpv;
					if (lpw) {
						lpv = ir::variant{lp->opr(1)};
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
					lp = ins;
					lpw = false;
				}
			}
			// If trashes registers:
			else if (ins->desc().trashes_regs) {
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
			if (ins->op == ir::opcode::binop || ins->op == ir::opcode::unop || ins->op == ir::opcode::cmp) {
				for (auto& match : directives::replace_list) {
					pattern::match_context ctx{};
					if (match(ins, ctx)) {
						n++;
						break;
					}
				}
			}
		}

		// Run DCE if there were any changes, return the change count.
		return n ? n + dce(bb) : n;
	}
};

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


	struct domain {
		// Owning workspace.
		//
		weak<workspace> parent = {};

		// Image details.
		//
		ref<ldr::image> img			  = {};
		domain_stats	 stats		  = {};
		arch::handle	 default_arch = {};




		ref<ir::routine> lift_routine(u64 va, arch::handle machine_override = {});
	};
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
			dom->default_arch = arch::instance::lookup(img->arch_hash);
			dom->img				= std::move(img);
			return dom;
		}
	};




	static ir::basic_block* lift_block(domain* dom, ir::routine* rtn, u64 va, arch::handle machine) {
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
				bbs->push_jmp(new_block);
				bbs->add_jump(new_block);

				// Return it.
				//
				return bbs;
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
		while (!data.empty()) {
			// Diassemble the instruction, push trap on failure and break.
			//
			arch::minsn ins = {};
			if (!machine->disasm(data, &ins)) {
				bb->push_trap("undefined opcode")->ip = va;
				break;
			}
			dom->stats.minsn_disasm++;

			// Update the block range.
			//
			bb->end_ip = va + ins.length;

			// Lift the instruction, push trap on failure and break.
			//
			if (auto err = machine->lift(bb, ins, va)) {
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
			if (bb->terminator())
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
							bbs[i] = lift_block(dom, rtn, (u64) term->opr(i + 1).const_val.get<ir::pointer>(), machine);
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
					if (auto target = lift_block(dom, rtn, (u64) term->opr(0).const_val.get<ir::pointer>(), machine)) {
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

	ref<ir::routine> domain::lift_routine(u64 va, arch::handle machine_override) {
		// If no machine override specified, use default architecture.
		//
		if (!machine_override) {
			machine_override = default_arch;
		}

		// If we do not have a valid machine in the end, fail.
		//
		if (!machine_override) {
			// TODO: log
			return nullptr;
		}

		// Create the routine.
		//
		ref<ir::routine> rtn = make_rc<ir::routine>();
		rtn->dom					= this;
		rtn->ip					= va;

		// Recursively lift starting from the entry point.
		//
		if (!lift_block(this, rtn, va, machine_override)) {
			return nullptr;
		}
		return rtn;
	}
};


int main(int argv, const char** args) {
	platform::setup_ansi_escapes();

	//auto rtn = make_rc<ir::routine>();
	//auto bb	= rtn->add_block();
	//
	//auto a = bb->push_read_reg(ir::type::i64, arch::x86::reg::rax);
	//a = bb->push_binop(ir::op::add, a, i64(1));
	//a = bb->push_binop(ir::op::add, a, i64(1));
	//bb->push_write_reg(arch::x86::reg::rax, a);
	//
	//fmt::println(bb->to_string());
	//
	//size_t n = 0;
	//for (auto& bb : rtn->blocks) {
	//	n += ir::opt::reg_move_prop(bb);
	//	n += ir::opt::const_fold(bb);
	//	n += ir::opt::id_fold(bb);
	//	n += ir::opt::ins_combine(bb);
	//	n += ir::opt::const_fold(bb);
	//	n += ir::opt::id_fold(bb);
	//}
	//fmt::println(bb->to_string());


	// Create the workspace.
	//
	auto ws = analysis::workspace::create();
	
	// Load the image.
	//
	auto img		 = ldr::load_from_file("../tests/libretro.exe").value();
	auto machine = arch::instance::lookup(img->arch_hash);
	auto loader	 = ldr::instance::lookup(img->ldr_hash);
	RC_ASSERT(loader && machine);
	fmt::println("-> loader:  ", loader->get_name());
	fmt::println("-> machine: ", machine->get_name());

	// Create a domain for it.
	//
	auto dom = ws->add_image(std::move(img));
	
	// Demo.
	//
	auto rtn = dom->lift_routine(0x1400072C0);

	// Print statistics.
	//
	printf(RC_GRAY " # Successfully lifted " RC_VIOLET "%llu" RC_GRAY " instructions into " RC_GREEN "%llu" RC_RED " (avg: ~%llu/ins) #\n" RC_RESET, dom->stats.minsn_disasm.load(),
		 dom->stats.insn_lifted.load(), dom->stats.insn_lifted.load() / dom->stats.minsn_disasm.load());

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
	printf("Optimized %llu instructions.\n", n);



	fmt::println(rtn->to_string());
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
	//// SF != OF
	//{
	//	auto a = c.bv_const("a", 64);
	//	auto b = c.bv_const("b", 64);
	//
	//
	//	/*
	//	CY = ¬ unu ∧ (t2 < 0).
	//	OV = ¬ (no ∨ unu) ∨ ¬ snu
	//	where
	//	*/
	//
	//	auto sl = a < 0;
	//	auto sr	= b < 0;
	//	auto sf	= (a - b) < 0;
	//	auto of	= (sl != sr) & (sl != sf);
	//	auto nof = (sl == sr) | (sl == sf);
	//
	//	fmt::println((nof == sf).simplify());
	//
	//
	//	z3::solver s(c);
	//	s.add((nof == sf) == (a < b));
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