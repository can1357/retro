#include <retro/common.hpp>
#include <retro/diag.hpp>
#include <retro/format.hpp>
#include <retro/utf.hpp>
#include <retro/platform.hpp>
#include <retro/robin_hood.hpp>
#include <retro/arch/interface.hpp>
#include <retro/arch/x86/regs.hxx>
#include <retro/arch/x86/callconv.hpp>
#include <retro/ldr/interface.hpp>
#include <retro/ir/insn.hpp>
#include <retro/ir/routine.hpp>
#include <retro/ir/z3x.hpp>
#include <retro/opt/interface.hpp>
#include <retro/opt/utility.hpp>
#include <retro/core/image.hpp>
#include <retro/core/method.hpp>
#include <retro/core/workspace.hpp>
#include <retro/core/callbacks.hpp>
#include <retro/llvm/clang.hpp>
#include <Zydis/Zydis.h>
using namespace retro;

static const char code_prefix[] =
#if RC_WINDOWS
	 "#define EXPORT __attribute__((noinline)) __declspec(dllexport)\n"
#else
	 "#define EXPORT __attribute__((noinline)) __attribute__((visibility(\"default\")))\n"
#endif
	 R"(

#define va_list          __builtin_va_list
#define va_start(v,l)    __builtin_va_start(v,l)
#define va_end(v)        __builtin_va_end(v)
#define va_arg(v,l)      __builtin_va_arg(v,l)
#define va_copy(d,s)     __builtin_va_copy(d,s)

#define OUTLINE __attribute__((noinline))
__attribute__((noinline)) static void sinkptr(void* _) { asm volatile(""); }
__attribute__((noinline)) static void sinkull(unsigned long long _) { asm volatile(""); }
__attribute__((noinline)) static void sinkll(long long _) { asm volatile(""); }
__attribute__((noinline)) static void sinku(unsigned int _) { asm volatile(""); }
__attribute__((noinline)) static void sinki(int _) { asm volatile(""); }
__attribute__((noinline)) static void sinkf(float _) { asm volatile(""); }
__attribute__((noinline)) static void sinkl(double _) { asm volatile(""); }

__attribute__((always_inline)) int marker(const char* msg) {
	void* x;
	asm volatile("vmread %0, %1" : "+a"(x) : "m"(*msg));
	asm volatile("mov %0, %0" : "+a"(x));
	asm volatile("mov %0, %0" : "+a"(x));
	return (int)(long)x;
}
__attribute__((always_inline)) int short_marker(const char* msg) {
	void* x;
	asm volatile("vmread %0, %1" : "+a"(x) : "m"(*msg));
	return (int)(long)x;
}
__attribute__((always_inline)) void value_marker(const char* msg, long long v) {
	asm volatile("vmwrite %1, %0" :: "a"(v), "m"(*msg));
}
#define pointer_marker(msg, v)  { \
	asm volatile("" : "+m"(*v)); \
	asm volatile("vmwrite %1, %0" :: "a"(v), "m"(*msg)); \
}
int main() {}
)";


struct save_slot_analysis {
	arch::mreg reg					= {};
	size_t	  num_validations = 0;
	ir::insn*  save_point		= nullptr;
};

static neo::subtask<bool> apply_stack_analysis(ir::routine* rtn) {
	core::irp_phi_info&					  result			 = rtn->method->phi_info;
	flat_umap<i32, i64>					  frame_offsets = {};
	flat_umap<i64, save_slot_analysis> save_slots	 = {};

	// Get method information.
	//
	auto	mach		= rtn->method->arch;
	auto	spr		= mach->get_stack_register();
	auto	pty		= ir::int_type(mach->get_pointer_width());
	auto	def_cc	= mach->get_cc_desc(rtn->get_image()->default_cc);
	auto* prologue = rtn->entry_point.get();
	
	// Make an expression representing the initial stack pointer.
	//
	z3x::variable_set vs = {};
	z3::context&		ctx = z3x::get_context();
	z3::expr				sp_0_expr = {ctx};
	for (ir::insn* i : prologue->insns()) {
		if (i->op == ir::opcode::stack_begin) {
			sp_0_expr = vs.emplace(ctx, i);
			break;
		}
	}
	RC_ASSERT(z3x::ok(sp_0_expr));

	// Determine the save slots and possible frame register from the prologue.
	//
	for (ir::insn* i : prologue->insns()) {
		//co_await neo::checkpoint{}; // TODO: Fix

		// Memory writes:
		//
		if (i->op == ir::opcode::store_mem) {
			auto base	  = z3x::to_expr(vs, ctx, i->opr(0));
			if (!z3x::ok(base))
				break;
			auto sp_delta = base - sp_0_expr;
			auto cval	  = z3x::value_of(sp_delta, true);
			// Non-stack memory write, end of epilogue.
			if (!cval) {
				break;
			}
			auto spd = cval.get_i64() + i->opr(1).get_const().get_i64();
			if (i->opr(2).is_value()) {
				if (auto i2 = i->opr(2).get_value()->get_if<ir::insn>(); i2 && i2->op == ir::opcode::read_reg) {
					auto& si				 = save_slots[spd];
					si.reg				 = i2->opr(0).const_val.get<arch::mreg>();
					si.save_point		 = i;
					si.num_validations = 0;
				}
			}
		}
		// Memory reads:
		//
		else if (i->op == ir::opcode::load_mem) {
			auto base = z3x::to_expr(vs, ctx, i->opr(0));
			if (!z3x::ok(base))
				break;
			auto sp_delta = base - sp_0_expr;
			auto cval	  = z3x::value_of(sp_delta, true);
			// Non-stack memory read, end of epilogue.
			if (!cval) {
				break;
			}
		}
		// Register writes:
		//
		else if (i->op == ir::opcode::write_reg) {
			auto reg = i->opr(0).const_val.get<arch::mreg>();

			// If pointer size:
			//
			if (i->template_types[0] == pty || i->template_types[0] == ir::type::pointer) {
				// If valid frame register:
				//
				if (!def_cc || range::contains(def_cc->frame_gpr, reg)) {
					// If saved in the frame:
					//
					if (range::contains_if(save_slots, [reg](auto& pair) { return pair.second.reg == reg; })) {
						// If valid constant offset from stack:
						//
						auto base = z3x::to_expr(vs, ctx, i->opr(1));
						if (z3x::ok(base)) {
							auto sp_delta = base - sp_0_expr;
							if (auto cval = z3x::value_of(sp_delta, true)) {
								frame_offsets[reg.uid()] = cval.get_i64();
							}
						}
					}
				}
			}
		}
		// Instruction with side effect, end of epilogue.
		//
		else if (i->desc().side_effect) {
			break;
		}
	}

	// For each epilogue:
	//
	size_t num_epi = 0;
	for (auto& epilogue : rtn->blocks) {
		if ((epilogue != rtn->entry_point && epilogue->predecessors.empty()) || !epilogue->successors.empty())
			continue;

		// Skip if not terminated or does not end with xret.
		//
		auto term = epilogue->terminator();
		if (!term || term->op != ir::opcode::xret)
			continue;
		num_epi++;

		// Make an expression representing the final stack pointer.
		//
		auto sp_0_expr = z3x::to_expr(vs, ctx, term->opr(0));
		RC_ASSERT(z3x::ok(sp_0_expr));

		for (auto i : epilogue->rslice(term)) {
			//fmt::println(i->to_string());
			//co_await neo::checkpoint{}; // TODO: Fix

			// Register writes:
			//
			if (i->op == ir::opcode::write_reg) {
				auto reg = i->opr(0).const_val.get<arch::mreg>();

				if (i->opr(1).is_value()) {
					if (auto i2 = i->opr(1).get_value()->get_if<ir::insn>(); i2 && i2->op == ir::opcode::load_mem) {
						auto base	  = z3x::to_expr(vs, ctx, i2->opr(0));
						if (z3x::ok(base)) {
							auto sp_delta = base - sp_0_expr;
							if (auto cval = z3x::value_of(sp_delta, true)) {
								auto off = save_slots.find(cval.get_i64() + i2->opr(1).get_const().get_i64());
								if (off != save_slots.end() && off->second.reg == reg) {
									off->second.num_validations++;
								}
							}
						}
					}
				}

			}
			// Instruction with side effect, end of epilogue.
			//
			else if (i->desc().side_effect) {
				break;
			}
		}
	}

	// Write save slot information.
	//
	for (auto& [slot, data] : save_slots) {
		// Skip if not appropriately validated.
		//
		if (data.num_validations != num_epi) {
			//fmt::printf("---> [INVALID] %s = SP[%s] (%llu/%llu validations)\n", data.reg.to_string(mach).c_str(), fmt::to_str(slot).c_str(), data.num_validations, num_epi);

			if (auto it = frame_offsets.find(data.reg.uid()); it != frame_offsets.end()) {
				frame_offsets.erase(it);
			}

			// TODO: Probably allocating space, OK to NOP?:
			//data.save_point->op = ir::opcode::nop;
			continue;
		}

		// Save the information.
		//
		result.save_area_layout.emplace(slot, data.reg);
		// fmt::printf("---> %s = SP[%s] (%llu/%llu validations)\n", data.reg.to_string(mach).c_str(), fmt::to_str(slot).c_str(), data.num_validations, num_epi);

		// Nop-out the save instruction.
		//
		data.save_point->op = ir::opcode::nop;
	}

	// Validate the frame registers:
	//
	for (auto& bb : rtn->blocks) {
		if (!bb->successors.empty() && !bb->predecessors.empty()) {
			for (auto i : bb->insns()) {
				if (i->op == ir::opcode::write_reg) {
					auto reg = i->opr(0).const_val.get<arch::mreg>();
					if (auto it = frame_offsets.find(reg.uid()); it != frame_offsets.end()) {
						frame_offsets.erase(it);
					}
				}
			}
		}
	}
	if (auto it = frame_offsets.begin(); it != frame_offsets.end()) {
		result.frame_reg		  = bitcast<arch::mreg>(it->first);
		result.frame_reg_delta = it->second;
		//fmt::printf("Frame register is %s, offset: %lld\n", result.frame_reg.to_string(mach).c_str(), result.frame_reg_delta);
	} else {
		//fmt::printf("Routine does not use a frame register\n");
	}

	// Invoke calling convention detection.
	//
	if (!core::on_cc_analysis(rtn, result)) {
		co_return false;
	}

	// Apply all local optimizations.
	//
	for (auto& bb : rtn->blocks) {
		co_await neo::checkpoint{};
		ir::opt::init::reg_move_prop(bb);
		ir::opt::const_fold(bb);
		ir::opt::const_load(bb);
		ir::opt::id_fold(bb);
		ir::opt::ins_combine(bb);
		ir::opt::const_fold(bb);
		ir::opt::id_fold(bb);
		// TODO: Cfg optimization
	}

	// Convert register use to PHIs, apply local optimizations again.
	//
	ir::opt::init::reg_to_phi(rtn);
	//rtn->entry_point->erase_if([&](ir::insn* i) {
	//	if (i->op == ir::opcode::read_reg) {
	//		auto r	 = i->opr(0).get_const().get<arch::mreg>();
	//		auto info = mach->get_register_info(r);
	//		if (info.full_reg != r) {
	//			auto finfo = mach->get_register_info(info.full_reg);
	//			auto ty0 = enum_reflect(info.full_reg.get_kind()).type;
	//			auto ty1 = enum_reflect(r.get_kind()).type;
	//
	//			auto fr = i->bb->insert(i, ir::make_read_reg(ty0, info.full_reg));
	//			if (info.bit_offset) {
	//				// TODO: This will not work for AARCH where vector regs split into two.
	//				//
	//				RC_ASSERT(ty0 == ir::int_type(info.bit_width));
	//				fr = i->bb->insert(i, ir::make_binop(ir::op::bit_shr, fr.get(), ir::constant(ty0, info.bit_offset)));
	//			}
	//			fr = i->bb->insert(i, ir::make_cast(ty1, fr.get()));
	//			i->replace_all_uses_with(fr.get());
	//			return true;
	//		}
	//	}
	//	return false;
	//});
	
	for (auto& bb : rtn->blocks) {
		co_await neo::checkpoint{};
		ir::opt::init::reg_move_prop(bb);
		ir::opt::const_fold(bb);
		ir::opt::const_load(bb);
		ir::opt::id_fold(bb);
		ir::opt::ins_combine(bb);
		ir::opt::const_fold(bb);
		ir::opt::id_fold(bb);
	}

	// Erase insert_context's propagated from entry block.
	//
	for (auto& bb : rtn->blocks) {
		for (auto* i : bb->insns()) {
			if (i->op == ir::opcode::insert_context) {
				auto& v = i->opr(2);
				if (v.is_value()) {
					if (auto* i2 = v.get_value()->get_if<ir::insn>()) {
						if (i2->op == ir::opcode::read_reg && (i2->bb != i->bb && i2->bb == rtn->entry_point)) {
							i->replace_all_uses_with(i->opr(0));
						}
					}
				}
			}
		}
	}

	// Run DCE again.
	//
	size_t n= 0;
	while (true) {
		size_t m = n;
		for (auto& bb : rtn->blocks) {
			n += ir::opt::util::local_dce(bb);
		}
		if (m == n)
			break;
	}

	// Sort the blocks in topological order and rename all values.
	//
	rtn->topological_sort();
	rtn->rename_blocks();
	rtn->rename_insns();

	// Determine min-max stack usage.
	//
	auto beg = range::find_if(rtn->entry_point->insns(), [](auto i) { return i->op == ir::opcode::stack_begin; });
	if (beg != rtn->entry_point->end()) {
		for (auto* use : beg->uses()) {
			if (auto* i = use->user->get_if<ir::insn>()) {
				if (i->op == ir::opcode::load_mem || i->op == ir::opcode::store_mem) {
					if (i->opr(0).is_value() && i->opr(0).get_value() == beg) {
						i64 width = align_up(enum_reflect(i->template_types[0]).bit_size, 8) / 8;
						result.min_sp_used = std::min(result.min_sp_used, i->opr(1).get_const().get_i64() + width);
						result.max_sp_used = std::max(result.max_sp_used, i->opr(1).get_const().get_i64());
					}
				}
			}
		}
	}
	//fmt::printf("Max SP delta: %x\n", result.max_sp_used);
	//fmt::printf("Min SP delta: -%x\n", -result.min_sp_used);

	// Write the analysis results.
	//
	co_return true;
}


// Transition from IRP_INIT to IRP_PHI.
//
static neo::task irp_phi(ref<ir::routine> rtn) {
	auto m = rtn->method.lock();

	// Execute the analysis phase and notify the IRP completion.
	//
	if (!co_await apply_stack_analysis(rtn)) {
		fmt::printf("IRP_PHI failed for sub_%llx.\n", rtn->ip);
		rtn = nullptr;
		m->routine[core::IRP_PHI].reset();
	}
	m->irp_mask.fetch_or(1u << core::IRP_PHI);
	m->irp_mask.notify_all();
	core::on_irp_complete(m, core::IRP_PHI);
	co_return;
}

static bool irp_phi_queue(core::method* m, neo::scheduler* sched = nullptr) {
	// Last phase failed so fail this one as well.
	//
	auto rtn = m->get_irp(core::IRP_INIT);
	if (!rtn) {
		m->irp_mask.fetch_or(1u << core::IRP_PHI);
		m->irp_mask.notify_all();
		return false;
	}

	// Acquire the lock, if there already is a busy task, skip.
	//
	std::unique_lock wl{m->irp_write_lock};
	if (m->irp_busy(core::IRP_PHI) || m->irp_present(core::IRP_PHI))
		return true;

	// Clone the routine and set it.
	//
	auto clone						 = rtn->clone();
	m->routine[core::IRP_PHI]	 = clone;

	// Schedule the task and return the reference.
	//
	sched								 = sched ? sched : &m->img->ws->auto_analysis_scheduler;
	m->irp_tasks[core::IRP_PHI] = sched->insert(irp_phi(clone));
	return true;
}

static void analyse_calls(ir::routine* rt) {
	std::pair<arch::x86::reg, arch::x86::reg> ar[] = {
		 {arch::x86::reg::rcx, arch::x86::reg::xmm0},
		 {arch::x86::reg::rdx, arch::x86::reg::xmm1},
		 {arch::x86::reg::r8, arch::x86::reg::xmm2},
		 {arch::x86::reg::r9, arch::x86::reg::xmm3},
	};

	for (auto& bb : rt->blocks) {
		for (auto i : bb->insns()) {
			if (i->op != ir::opcode::call)
				continue;

			auto ctx		  = i->opr(1).get_value()->get_if<ir::insn>();
			auto find_reg = [&](arch::mreg r) -> ir::operand* {
				auto it = ctx;
				while (it && it->op == ir::opcode::insert_context) {
					if (it->opr(1).get_const().get<arch::mreg>() == r)
						return &it->opr(2);
					it = it->opr(0).get_value()->get_if<ir::insn>();
				}
				return nullptr;
			};

			auto&					c	= z3x::get_context();
			z3x::variable_set vs = {};

			auto find_mem = [&](i64 off) -> ir::insn* {
				auto it = ctx;
				while (it && it->op == ir::opcode::insert_context) {
					it = it->opr(0).get_value()->get_if<ir::insn>();
				}
				if (it && it->op == ir::opcode::context_begin) {
					auto sp = z3x::to_expr(vs, c, it->opr(0));
					if (z3x::ok(sp)) {
						for (auto ii : bb->rslice(i)) {
							if (ii->op == ir::opcode::store_mem) {
								auto sp2 = z3x::to_expr(vs, c, ii->opr(0));
								if (z3x::ok(sp2)) {
									if (auto v = z3x::value_of(sp2 - sp, true)) {
										if (v.get_i64() == (off - ii->opr(1).get_const().get_i64())) {
											return ii;
										}
									}
								}
							}
						}
					}
				}
				return nullptr;
			};

			fmt::println(i->to_string());

			auto score = [&](ir::operand* o, arch::mreg r) -> int {
				if (!o)
					return -100;
				if (o->is_value()) {
					if (auto i2 = o->get_value()->get_if<ir::insn>()) {
						if (i2->op == ir::opcode::read_reg) {
							if (i2->opr(0).get_const().get<arch::mreg>() == r)
								return 0;
						}
						if (i2->op == ir::opcode::extract_context) {
							if (i2->opr(1).get_const().get<arch::mreg>() == r)
								return 0;
						}
					}
				}
				return 1;
			};

			std::pair<ir::operand*, int> vr[4] = {};

			std::string type_list = {};

			int n = 0;
			for (auto& [g, f] : ar) {
				auto ng = enum_name(g);
				auto nf = enum_name(f);
				auto ag = find_reg(g);
				auto af = find_reg(f);

				int sg = score(ag, g);
				int sf = score(af, f);
				if (sg >= sf) {
					vr[n] = {ag, sg};
					fmt::printf("Arg #%d: %s (Score: %d)\n", n, ag ? ag->to_string().c_str() : "null", sg);
				} else {
					vr[n] = {af, sf};
					fmt::printf("Arg #%d: %s (Score: %d)\n", n, af ? af->to_string().c_str() : "null", sf);
				}
				if (vr[n].first) {
					type_list += enum_name(vr[n].first->get_type());
					type_list += ",";
				}
				fmt::printf("Arg #%d: [%s(Score:%d) | %s(Score:%d)]\n", n, ag ? ag->to_string().c_str() : "null", sg, af ? af->to_string().c_str() : "null", sf);
				++n;
			}
			while (n != 32) {
				auto wr = find_mem(n * 8);
				if (!wr)
					break;
				fmt::printf("Arg #%d: %s\n", ++n, wr->to_string().c_str());
				type_list += enum_name(wr->template_types[0]);
				type_list += ",";
			}
			type_list.pop_back();
			fmt::println("Guessed arguments: void(", type_list, ")");

			if (i->opr(0).is_const()) {
				auto img		= i->get_image();
				auto method = img->lookup_method(i->opr(0).get_const().get_u64() - img->base_address);
				// fmt::printf("Max arg count: %llu\n", std::max<size_t>((method->phi_info.max_sp_used - 1) / 8, 4));
			}
		}
	}
}



inline static umutex __out_mtx;

static void whole_program_analysis_test(core::image* img) {
	fmt::println("Starting whole program analysis...\n");

	static auto analyse_rva_if_code = [](core::image* img, u64 rva) {
		if (auto scn = img->find_section(rva); scn && scn->execute) {
			if (!img->lookup_method(rva)) {
				core::lift(img, rva);
			}
		}
	};

	static std::atomic<size_t> num_routines = 0;
	static std::atomic<size_t> num_insn = 0;
	static std::atomic<size_t> num_minsn= 0;

	core::on_irp_complete.insert([](core::method* m, core::ir_phase ph) {
		auto r = m->routine[ph];
		if (ph == core::IRP_INIT) {
			if (!r)
				return;
			auto va = m->rva + m->img->base_address;
			//__out_mtx.lock();
			//fmt::printf("sub_%llx: " RC_GRAY " # Successfully lifted " RC_VIOLET "%llu" RC_GRAY " instructions into " RC_GREEN "%llu" RC_RED " (avg: ~%llu/ins) #\n" RC_RESET, va,
			//	 m->init_info.stats_minsn_disasm, m->init_info.stats_insn_lifted,
			//	 m->init_info.stats_insn_lifted / (m->init_info.stats_minsn_disasm ? m->init_info.stats_minsn_disasm : 1));
			//__out_mtx.unlock();

			num_routines++;
			num_insn += m->init_info.stats_insn_lifted;
			num_minsn += m->init_info.stats_minsn_disasm;

			auto pty = r->method->arch->ptr_type();
			auto img = r->method->img.lock();
			auto img_base = img->base_address;
			auto img_limit	  = img_base + img->raw_data.size();
	
			flat_uset<u64> va_set;
			for (auto& bb : r->blocks) {
				for (auto&& ins : bb->insns()) {
					if (ins->op == ir::opcode::xjmp || ins->op == ir::opcode::xjs)
						continue;
					for (auto& op : ins->operands()) {
						if (op.is_const()) {
							if (op.get_type() == ir::type::pointer || op.get_type() == pty) {
								auto va = op.get_const().get_u64();
								if (img_base <= va && va < img_limit)
									va_set.emplace(va);
							}
						}
					}
				}
			}
			for (auto va : va_set)
				analyse_rva_if_code(img, va - img_base);

			// Queue IRP_PHI.
			//
			irp_phi_queue(m);
		} else if (ph == core::IRP_PHI) {
			//auto va = m->rva + m->img->base_address;
			//__out_mtx.lock();
			//if (r) {
			//	std::optional<i64> delta = 0;
			//	for (auto& x : r->blocks) {
			//		if (auto t = x->terminator(); t && t->op == ir::opcode::ret) {
			//			delta = t->opr(1).const_val.get_i64();
			//			break;
			//		}
			//	}
			//
			//	auto t = delta ? fmt::str(RC_GREEN "(Delta: %lld)", *delta) : RC_RED "(No valid return)";
			//	fmt::printf("sub_%llx: " RC_GRAY " # Basic analysis finished %s" RC_GRAY " #\n" RC_RESET, va, t.c_str());
			//
			//	if (!delta || *delta != 8) {
			//		fmt::println(r->to_string());
			//	}
			//
			//	/*
			//	for (auto& [offset, reg] : m->phi_info.save_area_layout) {
			//		fmt::printf("---> %s = SP[%s]\n", reg.to_string(m->arch).c_str(), fmt::to_str(offset).c_str());
			//	}
			//	if (m->phi_info.frame_reg) {
			//		fmt::printf("---> Frame register is %s, offset: %lld\n", m->phi_info.frame_reg.to_string(m->arch).c_str(), m->phi_info.frame_reg_delta);
			//	} else {
			//		fmt::printf("---> Procedure does not use a frame register\n");
			//	}
			//	fmt::printf("---> Max SP delta: %x\n", m->phi_info.max_sp_used);
			//	fmt::printf("---> Min SP delta: -%x\n", -m->phi_info.min_sp_used);*/
			//} else {
			//	fmt::printf("sub_%llx: " RC_GRAY " # Basic analysis " RC_RED "failed" RC_GRAY " #\n" RC_RESET, va);
			//}
			//__out_mtx.unlock();
		}
	});


	auto t0 = now();
	for (auto& rva : img->entry_points) {
		analyse_rva_if_code(img, rva);
	}
	for (auto& sym : img->symbols) {
		if (!sym.read_only_ignore)
			analyse_rva_if_code(img, sym.rva);
	}
	for (auto& reloc : img->relocs) {
		if (std::holds_alternative<u64>(reloc.target)) {
			analyse_rva_if_code(img, std::get<u64>(reloc.target));
		}
	}
	img->ws->auto_analysis_scheduler.wait_until_empty();
	auto t1 = now();

	fmt::printf("Whole program analysis complete!\n");
	fmt::printf(" - Created %llu routines\n", num_routines.load());
	fmt::printf(" - Lifted  %llu machine instructions\n", num_minsn.load());
	fmt::printf(" - Created %llu IR instructions\n", num_insn.load());
	fmt::printf(" - Took %.2f seconds\n", (t1-t0)/1.0s);
}

static std::string write_graph(ref<ir::routine> rtn) {
	std::string out = R"(digraph G {
	mclimit	= 1.5;
	rankdir	= TD;
	ordering = out;
	node[shape="box"]

)";

	for (auto& bb : rtn->blocks) {
		auto it = range::find_if(bb->insns(), [](auto i) { return i->op == ir::opcode::annotation; });
		std::string res;
		if (it != bb->end()) {
			for (auto i : bb->insns()) {
				if (i->op == ir::opcode::annotation) {
					res += i->opr(0).get_const().get<std::string_view>();
					res += "\\n";
				}
			}
			res.pop_back();
			res.pop_back();
		} else {
			res = fmt::strip_ansi(bb->to_string());
			while (true) {
				auto it = res.find('\n');
				if (it != std::string::npos) {
					res[it] = 'l';
					res.insert(res.begin() + it, '\\');
				} else {
					break;
				}
			}
			while (true) {
				auto it = res.find('"');
				if (it != std::string::npos) {
					res[it] = '\'';
				} else {
					break;
				}
			}
			while (true) {
				auto it = res.find('\t');
				if (it != std::string::npos) {
					res.erase(res.begin() + it);
				} else {
					break;
				}
			}
		}
		out += fmt::str("\tblk_%x[label = \"%s\"]\n", bb->name, res.c_str());
	}

	for (auto& bb : rtn->blocks) {
		auto t = bb->terminator();
		if (t->op == ir::opcode::js) {
			auto tc = t->opr(1).get_value()->get_if<ir::basic_block>()->name;
			auto fc = t->opr(2).get_value()->get_if<ir::basic_block>()->name;


			out += fmt::str("\tblk_%x->blk_%x[color=\"green\"];\n", bb->name, tc);
			out += fmt::str("\tblk_%x->blk_%x[color=\"red\"];\n", bb->name, fc);
		} else {
			for (auto& suc : bb->successors) {
				out += fmt::str("\tblk_%x->blk_%x;\n", bb->name, suc->name);
			}
		}
	}
	out += "\n}";
	return out;
}


static ref<ir::routine> analysis_test(core::image* img, const std::string& name, u64 rva) {
	// Demo.
	//
	auto m	= core::lift(img, rva, &img->ws->user_analysis_scheduler);
	auto rtn = m->wait_for_irp(core::IRP_INIT);
	
	auto va = m->rva + m->img->base_address;
	fmt::printf("sub_%llx: " RC_GRAY " # Successfully lifted " RC_VIOLET "%llu" RC_GRAY " instructions into " RC_GREEN "%llu" RC_RED " (avg: ~%llu/ins)" RC_RESET " #\n", va,
		 m->init_info.stats_minsn_disasm, m->init_info.stats_insn_lifted, m->init_info.stats_insn_lifted / (m->init_info.stats_minsn_disasm ? m->init_info.stats_minsn_disasm : 1));

	// Print statistics.
	//
	fmt::printf(RC_WHITE " ----- Routine '%s' -------\n", name.data());
	auto sorted_bb = rtn->blocks;
	range::sort(sorted_bb, [](auto& a, auto& b) { return a->ip < b->ip; });
	for (auto& bb : sorted_bb) {
		std::string result = fmt::str(RC_CYAN "$%x:" RC_RESET, bb->name);
		result += fmt::str(RC_GRAY " [%llx => %llx]" RC_RESET, bb->ip, bb->end_ip);
		fmt::println(result);
		fmt::println("\t", bb->terminator()->to_string());
	}

	if (auto pr = irp_phi_queue(m)) {
		auto nrtn = m->wait_for_irp(core::IRP_PHI);
		if (nrtn) {
			for (auto& [offset, reg] : m->phi_info.save_area_layout) {
				fmt::printf("---> %s = SP[%s]\n", reg.to_string(m->arch).c_str(), fmt::to_str(offset).c_str());
			}
			if (m->phi_info.frame_reg) {
				fmt::printf("Frame register is %s, offset: %lld\n", m->phi_info.frame_reg.to_string(m->arch).c_str(), m->phi_info.frame_reg_delta);
			} else {
				fmt::printf("Procedure does not use a frame register\n");
			}
			fmt::printf("Max SP delta: %x\n", m->phi_info.max_sp_used);
			fmt::printf("Min SP delta: -%x\n", -m->phi_info.min_sp_used);
			fmt::println(nrtn->to_string());
			//fmt::println(write_graph(nrtn));
			analyse_calls(nrtn);

			for (auto& bb : sorted_bb) {
				auto range = rtn->get_image()->slice(bb->ip - rtn->get_image()->base_address);
				range		  = range.subspan(0, bb->end_ip - bb->ip);

				u64 ip = bb->ip;
				while (!range.empty()) {
					arch::minsn insn;
					if (!rtn->get_image()->arch->disasm(range, &insn))
						break;
					fmt::println((void*) ip, ":", insn.to_string(ip));
					ip += insn.length;
					range = range.subspan(insn.length);
				}
			}
		}
	}


	return rtn;
}

// Demo wrappers.
//
static void analysis_test_from_image_va(std::filesystem::path path, u64 va) {
	// Load the binary.
	//
	auto ws	= core::workspace::create();
	auto img = ws->load_image(path).value();
	fmt::println("-> loader:  ", img->ldr.get_name());
	fmt::println("-> machine: ", img->arch.get_name());

	// Call the demo code.
	//
	whole_program_analysis_test(img);
	analysis_test(img, fmt::str("sub_%llx", va), va - img->base_address);
}
static void analysis_test_from_source(std::string src) {
	core::on_minsn_lift.insert([](arch::handle arch, ir::basic_block* bb, arch::minsn& i, u64 va) {
		if (i.mnemonic == ZYDIS_MNEMONIC_VMREAD) {
			auto str		= (const char*) bb->get_image()->slice(i.op[0].m.disp + va + i.length - bb->get_image()->base_address).data();
			auto result = bb->push_annotation(ir::int_type(i.op[1].get_width()), std::string_view{str});
			auto write	= bb->push_write_reg(i.op[1].r, result);
			arch->explode_write_reg(write);
			return true;
		} else if (i.mnemonic == ZYDIS_MNEMONIC_VMWRITE) {
			auto str		= (const char*) bb->get_image()->slice(i.op[1].m.disp + va + i.length - bb->get_image()->base_address).data();
			auto read	= bb->push_read_reg(ir::int_type(i.op[0].get_width()), i.op[0].r);
			bb->push_annotation(ir::type::none, std::string_view{str}, read);
			return true;
		}
		return false;
	});


	// Determine flags.
	//
	std::string flags = "-O1";
	if (auto it = src.find("// clang: "); it != std::string::npos) {
		auto new_flags = src.substr(it + sizeof("// clang: ") - 1);
		auto p			= new_flags.find_first_of("\r\n");
		if (p != std::string::npos) {
			new_flags = new_flags.substr(0, p);
		}
		flags.assign(new_flags);
	}

	// Compile the source code.
	//
	src.insert(0, code_prefix);
	std::string err;
	auto			bin = llvm::compile(src, flags, &err);
	if (!err.empty())
		fmt::abort(err.c_str());

	// Load the binary.
	//
	auto ws	= core::workspace::create();
	auto img = ws->load_image_in_memory(bin).value();
	fmt::println("-> loader:  ", img->ldr.get_name());
	fmt::println("-> machine: ", img->arch.get_name());

	// Add entry point symbol if none present.
	//
	whole_program_analysis_test(img);

	for (auto& sym : img->symbols) {
		if (sym.read_only_ignore || sym.name.starts_with("_"))
			continue;
		analysis_test(img, sym.name, sym.rva);
	}
}

int main(int argv, const char** args) {
	platform::setup_ansi_escapes();
	// TODO: Assumes its a UI app and we want responsive UX, should be handled by a launch flag or sthg.
	platform::g_affinity_mask = bit_mask(std::min<i32>((i32)std::thread::hardware_concurrency() / 2, 64));

	// Large function test:
	//
	if (false) {
		analysis_test_from_image_va("S:\\Dumps\\ntoskrnl_2004.exe", 0x140A1AEE4);
	}
	// Small C file test:
	//
	else {
		std::string test_file = "S:\\Projects\\Retro\\tests\\simple.c";
		if (argv > 1) {
			test_file = args[1];
		}
		bool ok;
		auto code = platform::read_file(test_file, ok);
		if (!ok) {
			fmt::abort("failed to read the file.");
		}
		analysis_test_from_source(utf::convert<char>(code));
	}
	return 0;
}
