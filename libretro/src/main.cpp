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

#if 0
namespace retro::core {
	// Converts xcall/xret into call/ret where possible.
	//
	static size_t apply_cc_info(ir::routine* rtn) {
		// Get image for CC analysis.
		//
		auto* img = rtn->img.get();
		if (!img)
			return 0;

		// For each instruction.
		//
		size_t n = 0;
		for (auto& bb : rtn->blocks) {
			n += bb->erase_if([&](ir::insn* i) {
				// If XCALL:
				//
				if (i->op == ir::opcode::xcall) {
					// Determine the calling convention.
					//
					auto* cc = dom->get_routine_cc(i);
					if (!cc)
						return false;

					// First read each argument:
					//
					auto args	  = bb->insert(i, ir::make_undef(ir::type::context));
					auto push_reg = [&](arch::mreg a) {
						if (a) {
							auto val = bb->insert(i, ir::make_read_reg(enum_reflect(a.get_kind()).type, a));
							args		= bb->insert(i, ir::make_insert_context(args.get(), a, val.get()));
						}
					};
					range::for_each(cc->argument_gpr, push_reg);
					range::for_each(cc->argument_fp, push_reg);
					push_reg(cc->fp_varg_counter);

					// Create the call.
					//
					auto res = bb->insert(i, ir::make_call(i->opr(0), args.get()));

					// Read each result.
					//
					auto pop_reg = [&](arch::mreg a) {
						if (a) {
							auto& desc = enum_reflect(a.get_kind());
							auto	val  = bb->insert(i, ir::make_extract_context(desc.type, res.get(), a));
							i->arch->explode_write_reg(bb->insert(i, ir::make_write_reg(a, val.get())));
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
				else if (i->op == ir::opcode::xret) {
					// Determine the calling convention.
					//
					auto* cc = dom->get_routine_cc(rtn);
					if (!cc)
						return false;
				
					// Read each result:
					//
					auto args	  = bb->insert(i, ir::make_undef(ir::type::context));
					auto push_reg = [&](arch::mreg a) {
						if (a) {
							auto val = bb->insert(i, ir::make_read_reg(enum_reflect(a.get_kind()).type, a));
							args		= bb->insert(i, ir::make_insert_context(args.get(), a, val.get()));
						}
					};
					range::for_each(cc->retval_gpr, push_reg);
					range::for_each(cc->retval_fp, push_reg);

					// TODO: assert retptr == [rsp] from entry point??

					// Create the ret.
					//
					bb->insert(i, ir::make_ret(args.get()));
					return true;
				}
				return false;
			});
		}
		return ir::opt::util::complete(rtn, n);
	}

	//
	//
	static void auto_cc(ir::routine* rtn) {

		// Split blocks at xcall boundaries.
		//
		//graph::bfs(rtn->get_entry(), [](ir::basic_block* bb) {
		//	auto it = range::find_if(bb->insns(), [&](ir::insn* ins) { return ins->op == ir::opcode::xcall; });
		//	if (it != bb->end() && std::next(it).get() != bb->back()) {
		//		auto nb = bb->split(std::next(it));
		//		RC_ASSERT(nb);
		//		bb->push_jmp(nb);
		//		bb->add_jump(nb);
		//	}
		//});
		//
		//fmt::println(rtn->to_string());
		//fmt::println(rtn->to_string());
	}
};
#endif

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

static void phase0(ref<ir::routine> rtn) {
	auto dom = rtn->method->img.get();

	// TODO: Detect tail call.
	// TODO: Call $0 => push jmp.
	// TODO: Call x -> x: add rsp, 8 -> push jmp for retpoline

	// Convert XCALL and XRETs.
	//
	auto	 sp = dom->arch->get_stack_register();
	size_t n	 = 0;
	for (auto& bb : rtn->blocks) {
		n += bb->erase_if([&](ir::insn* i) {
			// If XCALL:
			//
			if (i->op == ir::opcode::xcall) {
				// If user did not specify a calling convention:
				// (TODO: How)
				//
				{
					flat_uset<u32> read_set	 = {};
					flat_uset<u32> write_set = {};

					auto args	  = bb->insert(i, ir::make_undef(ir::type::context));
					auto push_reg = [&](arch::mreg a) {
						if (a && read_set.emplace(a.uid()).second) {
							auto val = bb->insert(i, ir::make_read_reg(enum_reflect(a.get_kind()).type, a));
							args		= bb->insert(i, ir::make_insert_context(args.get(), a, val.get()));
						}
					};

					// For each valid calling convention:
					//
					for (u32 i = 1; i <= 0xFF; i++) {
						auto* cc = dom->arch->get_cc_desc((arch::call_conv) i);
						if (!cc)
							break;

						// Read all registers.
						//
						range::for_each(cc->argument_gpr, push_reg);
						range::for_each(cc->argument_fp, push_reg);
						push_reg(cc->fp_varg_counter);
						//push_reg(sp);
					}

					// Create the call.
					//
					auto res		 = bb->insert(i, ir::make_call(i->opr(0), args.get()));
					auto pop_reg = [&](arch::mreg a) {
						if (a && write_set.emplace(a.uid()).second) {
							auto& desc = enum_reflect(a.get_kind());
							auto	val  = bb->insert(i, ir::make_extract_context(desc.type, res.get(), a));
							i->arch->explode_write_reg(bb->insert(i, ir::make_write_reg(a, val.get())));
						}
					};

					// For each valid calling convention:
					//
					for (u32 i = 1; i <= 0xFF; i++) {
						auto* cc = dom->arch->get_cc_desc((arch::call_conv) i);
						if (!cc)
							break;

						// Write back the result.
						//
						range::for_each(cc->retval_gpr, pop_reg);
						range::for_each(cc->retval_fp, pop_reg);
						if (!cc->sp_caller_adjusted) {
							pop_reg(sp);
						}
					}

					// Trash arguments that cannot be retvals.
					//
					for (auto& arg : read_set) {
						if (arg != sp.uid() && !write_set.contains(arg)) {
							auto a  = retro::bit_cast<arch::mreg>(arg);
							auto ty = enum_reflect(a.get_kind()).type;
							auto ud = bb->insert(i, ir::make_undef(ty));
							i->arch->explode_write_reg(bb->insert(i, ir::make_write_reg(a, ud.get())));
						}
					}
				}

				// Erase the XCALL.
				//
				return true;
			}
			// If XRET:
			//
			else if (i->op == ir::opcode::xret) {
				auto args = bb->insert(i, ir::make_undef(ir::type::context));

				// If user did not specify a calling convention:
				// (TODO: How)
				//
				{
					flat_uset<u32> write_set = {};

					auto push_reg = [&](arch::mreg a) {
						if (a && write_set.emplace(a.uid()).second) {
							auto val = bb->insert(i, ir::make_read_reg(enum_reflect(a.get_kind()).type, a));
							args		= bb->insert(i, ir::make_insert_context(args.get(), a, val.get()));
						}
					};

					// For each valid calling convention:
					//
					for (u32 i = 1; i <= 0xFF; i++) {
						auto* cc = dom->arch->get_cc_desc((arch::call_conv) i);
						if (!cc)
							break;

						// Read the results and stack pointer.
						//
						range::for_each(cc->retval_gpr, push_reg);
						range::for_each(cc->retval_fp, push_reg);
						//push_reg(sp);
					}
				}

				// Create the ret.
				//
				bb->insert(i, ir::make_ret(i->opr(0), args.get()));
				return true;
			}
			return false;
		});
	}

	// Apply simple optimizations.
	//
	for (auto& bb : rtn->blocks) {
		n += ir::opt::p0::reg_move_prop(bb);
		n += ir::opt::const_fold(bb);
		n += ir::opt::id_fold(bb);
		n += ir::opt::ins_combine(bb);
		n += ir::opt::const_fold(bb);
		n += ir::opt::id_fold(bb);
		// TODO: Cfg optimization
	}

	// DCE, run another pass of local optimizations.
	//
	n = ir::opt::util::complete(rtn, n);
	for (auto& bb : rtn->blocks) {
		n += ir::opt::const_fold(bb);
		n += ir::opt::id_fold(bb);
		n += ir::opt::ins_combine(bb);
		n += ir::opt::const_fold(bb);
		n += ir::opt::id_fold(bb);
	}

	// Convert register use to PHIs, repeat.
	//
	n += ir::opt::p0::reg_to_phi(rtn);
	for (auto& bb : rtn->blocks) {
		n += ir::opt::const_fold(bb);
		n += ir::opt::id_fold(bb);
		n += ir::opt::ins_combine(bb);
		n += ir::opt::const_fold(bb);
		n += ir::opt::id_fold(bb);
		n += ir::opt::util::local_dce(bb);
	}
	// Sort topologically and rename all instructions.
	//
	rtn->topological_sort();
	rtn->rename_insns();

#if 0
	// Map of all registers with offset from SP.
	//
	constexpr i32		  no_offset			 = INT32_MAX;
	auto					  i_sp_offset_list = std::make_unique<i32[]>(rtn->next_ins_name + 1);
	flat_umap<u32, i32> r_sp_offset_map	 = {}; 

	// Analyze entry point.
	//
	auto sp = dom->arch->get_stack_register();
	for (auto& bb : rtn->blocks) {
		fmt::printf("--- block $%x --- \n", bb->name);

		r_sp_offset_map.clear();
		r_sp_offset_map[sp.uid()] = 0;

		r_sp_offset_map[arch::mreg(arch::x86::reg::rbp).uid()] = -24;

		for (auto* i : bb->insns()) {
			i_sp_offset_list[i->name] = no_offset;

			// Read reg => imap[i] := rmap[i]
			//
			if (i->op == ir::opcode::read_reg) {
				auto r = i->opr(0).get_const().get<arch::mreg>();
				if (auto it = r_sp_offset_map.find(r.uid()); it != r_sp_offset_map.end()) {
					i_sp_offset_list[i->name] = it->second;
				} else {
					i_sp_offset_list[i->name] = no_offset;
				}
				continue;
			}

			// Bit cast.
			//
			if (i->op == ir::opcode::bitcast) {
				if (i->template_types[0] == ir::type::pointer || i->template_types[1] == ir::type::pointer) {
						auto& v = i->opr(0);
						if (!v.is_const()) {
							if (auto* in = v.get_value()->get_if<ir::insn>()) {
								i_sp_offset_list[i->name] = i_sp_offset_list[in->name];
								continue;
							}
						}
				}
			}

			// Write reg => rmap[r] := imap[i]
			//
			if (i->op == ir::opcode::write_reg) {
				auto	r = i->opr(0).get_const().get<arch::mreg>();
				auto& v = i->opr(1);
				if (!v.is_const()) {
					if (auto* in = v.get_value()->get_if<ir::insn>()) {
						if (i32 o = i_sp_offset_list[in->name]; o != no_offset) {
							r_sp_offset_map[r.uid()] = o;
						} else {

							if (in->op == ir::opcode::load_mem) {
								auto& ptr = in->opr(0);
								if (!ptr.is_const() && ptr.get_value()->is<ir::insn>()) {
									if (i32 o = i_sp_offset_list[ptr.get_value()->get<ir::insn>().name]; o != no_offset) {
										fmt::println(r.to_string(dom->arch), " = {$sp + ", o, "}");
									}
								}
							} 

							r_sp_offset_map.erase(r.uid());
						}
						continue;
					}
				}
			}

			// Write to memory.
			//
			if (i->op == ir::opcode::store_mem) {
				auto& ptr = i->opr(0);
				auto& val = i->opr(1);

				if (!ptr.is_const() && ptr.get_value()->is<ir::insn>()) {
					if (i32 o = i_sp_offset_list[ptr.get_value()->get<ir::insn>().name]; o != no_offset) {
						fmt::println("storing to {$sp + ", o, "} = ", val);
					}
				}
				// noseg ptr
			}

			// XCALL.
			//
			if (i->op == ir::opcode::xcall) {
				auto cc = dom->default_cc;	 // TODO: dom->get_routine_cc(i);
				for (auto it = r_sp_offset_map.begin(); it != r_sp_offset_map.end();) {
					if (range::find(cc->retval_gpr, retro::bit_cast<arch::mreg>(it->first)) != cc->retval_gpr.end()) {
						it = r_sp_offset_map.erase(it);
					} else {
						++it;
					}
				}
				// TODO:
			}

			// Add/Sub.
			//
			if (i->op == ir::opcode::binop) {
				auto op = i->opr(0).get_const().get<ir::op>();
				if (op == ir::op::add || op == ir::op::sub) {
					auto* o1 = &i->opr(1);
					auto* o2 = &i->opr(2);

					if (op == ir::op::add && o1->is_const())
						std::swap(o1, o2);

					if (!o1->is_const() && o2->is_const()) {
						if (auto* in = o1->get_value()->get_if<ir::insn>()) {
							if (i32 o = i_sp_offset_list[in->name]; o != no_offset) {
								i64 delta = o2->const_val.get_i64();
								if (op == ir::op::add) {
									o += delta;
								} else {
									o -= delta;
								}
								i_sp_offset_list[i->name] = o;
								continue;
							}
						}
					}
				}
			}
		}

		for (auto& [reg, offset] : r_sp_offset_map) {
			fmt::println("'", retro::bit_cast<arch::mreg>(reg).to_string(dom->arch), "' => $sp +", offset);
		}
	}
#endif
}

#include <retro/core/callbacks.hpp>

static void whole_program_analysis_test(core::image* img) {
	static auto analyse_rva_if_code = [](core::image* img, u64 rva) {
		if (auto scn = img->find_section(rva); scn && scn->execute) {
			if (!img->lookup_method(rva)) {
				// fmt::printf("queued analysis of sub_%llx\n", rva + img->base_address);
				core::lift_async(img, rva);
			}
		}
	};

	core::on_irp_complete.insert([](ir::routine* r, core::ir_phase ph) {
		if (ph == core::IRP_INIT) {
			auto m  = r->method.get();
			auto va = m->rva + m->img->base_address;
			// fmt::printf("sub_%llx: " RC_GRAY " # Successfully lifted " RC_VIOLET "%llu" RC_GRAY " instructions into " RC_GREEN "%llu" RC_RED " (avg: ~%llu/ins)" RC_RESET " #\n",
			// va, 	 m->init_info.stats_minsn_disasm, m->init_info.stats_insn_lifted, m->init_info.stats_insn_lifted / (m->init_info.stats_minsn_disasm ? m->init_info.stats_minsn_disasm
			//: 1));

			for (auto& bb : r->blocks) {
				for (auto&& ins : bb->insns()) {
					if (ins->op == ir::opcode::xcall) {
						if (ins->opr(0).is_const()) {
							analyse_rva_if_code(r->method->img.get(), ins->opr(0).get_const().get_u64() - r->method->img->base_address);
						}
					}
				}
			}
		}
	});

	for (auto& sym : img->symbols) {
		analyse_rva_if_code(img, sym.rva);
	}
	for (auto& reloc : img->relocs) {
		if (std::holds_alternative<u64>(reloc.target)) {
			analyse_rva_if_code(img, std::get<u64>(reloc.target));
		}
	}

	std::this_thread::sleep_for(std::chrono::seconds(100));
}

static ref<ir::routine> analysis_test(core::image* img, const std::string& name, u64 rva) {
	core::on_irp_complete.insert([](ir::routine* r, core::ir_phase ph) {
		if (ph == core::IRP_INIT) {
			auto m  = r->method.get();
			auto va = m->rva + m->img->base_address;
			fmt::printf("sub_%llx: " RC_GRAY " # Successfully lifted " RC_VIOLET "%llu" RC_GRAY " instructions into " RC_GREEN "%llu" RC_RED " (avg: ~%llu/ins)" RC_RESET " #\n", va,
				 m->init_info.stats_minsn_disasm, m->init_info.stats_insn_lifted,
				 m->init_info.stats_insn_lifted / (m->init_info.stats_minsn_disasm ? m->init_info.stats_minsn_disasm : 1));
		}
	});

	// Demo.
	//
	auto method = core::lift_async(img, rva);
	auto rtn		= method->wait_for_irp(core::IRP_INIT);

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

	auto clone = rtn->clone();
	fmt::println(clone->to_string());

	phase0(rtn);
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
	// whole_program_analysis_test(img);
	analysis_test(img, fmt::str("sub_%llx", va), va - img->base_address);
}
static void analysis_test_from_source(std::string src) {
	// Determine flags.
	//
	std::string flags = "-O1";
	if (auto it = src.find("// clang: "); it != std::string::npos) {
		std::string_view new_flags{src.begin() + it + sizeof("// clang: ") - 1, src.end()};
		auto				  p = new_flags.find_first_of("\r\n");
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
	// whole_program_analysis_test(img);

	for (auto& sym : img->symbols) {
		if (sym.name.starts_with("_"))
			continue;
		auto r = analysis_test(img, sym.name, sym.rva);
		{
			size_t n = 0;
			for (auto& bb : r->blocks) {
				n += ir::opt::p0::reg_move_prop(bb);
				n += ir::opt::const_fold(bb);
				n += ir::opt::id_fold(bb);
				n += ir::opt::ins_combine(bb);
				n += ir::opt::const_fold(bb);
				n += ir::opt::id_fold(bb);
				// TODO: Cfg optimization
			}
			/*core::apply_cc_info(r);
			for (auto& bb : r->blocks) {
				n += ir::opt::p0::reg_move_prop(bb);
				n += ir::opt::const_fold(bb);
				n += ir::opt::id_fold(bb);
				n += ir::opt::ins_combine(bb);
				n += ir::opt::const_fold(bb);
				n += ir::opt::id_fold(bb);
			}
			n += ir::opt::p0::reg_to_phi(r);
			for (auto& bb : r->blocks) {
				n += ir::opt::const_fold(bb);
				n += ir::opt::id_fold(bb);
				n += ir::opt::ins_combine(bb);
				n += ir::opt::const_fold(bb);
				n += ir::opt::id_fold(bb);
			}*/
		}
		fmt::println(r->to_string());
	}
}

#include <retro/utf.hpp>
int main(int argv, const char** args) {
	platform::setup_ansi_escapes();

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