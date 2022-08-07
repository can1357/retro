#include <retro/arch/interface.hpp>
#include <retro/common.hpp>
#include <retro/diag.hpp>
#include <retro/format.hpp>
#include <retro/ir/insn.hpp>
#include <retro/ldr/interface.hpp>
#include <retro/platform.hpp>

using namespace retro;

#include <retro/arch/x86/regs.hxx>
#include <retro/ir/routine.hpp>
#include <retro/robin_hood.hpp>

#include <retro/ir/z3x.hpp>

#include <retro/opt/interface.hpp>
#include <retro/opt/utility.hpp>

#include <retro/core/image.hpp>
#include <retro/core/method.hpp>
#include <retro/core/workspace.hpp>

static const char code_prefix[] =
#if RC_WINDOWS
	 "#define EXPORT __attribute__((noinline)) __declspec(dllexport)\n"
#else
	 "#define EXPORT __attribute__((noinline)) __attribute__((visibility(\"default\")))\n"
#endif
	 R"(
#define OUTLINE __attribute__((noinline))
__attribute__((noinline)) static void sinkptr(void* _) { asm volatile(""); }
__attribute__((noinline)) static void sinkull(unsigned long long _) { asm volatile(""); }
__attribute__((noinline)) static void sinkll(long long _) { asm volatile(""); }
__attribute__((noinline)) static void sinku(unsigned int _) { asm volatile(""); }
__attribute__((noinline)) static void sinki(int _) { asm volatile(""); }
__attribute__((noinline)) static void sinkf(float _) { asm volatile(""); }
__attribute__((noinline)) static void sinkl(double _) { asm volatile(""); }
int main() {}
)";

static std::vector<u8> compile(std::string code, const char* args) {
	code.insert(0, code_prefix);

	// Create the temporary paths.
	//
	auto tmp_dir = std::filesystem::temp_directory_path();
	auto in		 = tmp_dir / "retrotmp.c";
	auto out		 = tmp_dir / "retrotmp.exe";

	std::error_code ec;
	std::filesystem::remove(in, ec);
	std::filesystem::remove(out, ec);

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

#include <retro/robin_hood.hpp>



#include <execution>

#include <retro/arch/x86/regs.hxx>
#include <retro/arch/x86/callconv.hpp>
#include <retro/core/callbacks.hpp>


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
	auto* prologue = rtn->get_entry();
	
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
		if ((epilogue != rtn->get_entry() && epilogue->predecessors.empty()) || !epilogue->successors.empty())
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
			data.save_point->op = ir::opcode::nop;
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
	//rtn->get_entry()->erase_if([&](ir::insn* i) {
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
						if (i2->op == ir::opcode::read_reg && (i2->bb != i->bb && i2->bb == rtn->get_entry())) {
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
	auto beg = range::find_if(rtn->get_entry()->insns(), [](auto i) { return i->op == ir::opcode::stack_begin; });
	if (beg != rtn->get_entry()->end()) {
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

/*
static void phase0(ref<ir::routine> rtn) {
	fmt::println(rtn->to_string());

	// Print disassembly.
	// -- TODO: DEBUG
	range::sort(rtn->blocks, [](auto& a, auto& b) { return a->ip < b->ip; });
	for (auto& bb : rtn->blocks) {
		auto range = rtn->get_image()->slice(bb->ip - rtn->get_image()->base_address);
		range = range.subspan(0, bb->end_ip - bb->ip);

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

	// Print the routine.
	//
	apply_stack_analysis(rtn);

	fmt::println(rtn->to_string());
}*/

static umutex __out_mtx;

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
			auto va = m->rva + m->img->base_address;

			//__out_mtx.lock();
			//if (r) {
			//	fmt::printf("sub_%llx: " RC_GRAY " # Basic analysis " RC_GREEN "finished" RC_GRAY " #\n" RC_RESET, va);
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
	for (auto& sym : img->symbols) {
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
	range::sort(rtn->blocks, [](auto& a, auto& b) { return a->ip < b->ip; });
	for (auto& bb : rtn->blocks) {
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
	auto bin = compile(src, flags.data());

	// Load the binary.
	//
	auto ws	= core::workspace::create();
	auto img = ws->load_image_in_memory(bin).value();
	fmt::println("-> loader:  ", img->ldr.get_name());
	fmt::println("-> machine: ", img->arch.get_name());

	// Add entry point symbol if none present.
	//
	if (img->symbols.empty()) {
		img->symbols.push_back({img->base_address, "entry point"});
	}
	whole_program_analysis_test(img);

	for (auto& sym : img->symbols) {
		if (sym.name.starts_with("_"))
			continue;
		analysis_test(img, sym.name, sym.rva);
	}
}

#include <retro/utf.hpp>
int main(int argv, const char** args) {
	platform::setup_ansi_escapes();
	// TODO: Assumes its a UI app and we want responsive UX, should be handled by a launch flag or sthg.
	platform::g_affinity_mask = bit_mask(std::min<i32>((i32)std::thread::hardware_concurrency() / 2, 64));

	// Large function test:
	//
	if (true) {
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
