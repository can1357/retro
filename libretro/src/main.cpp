#include <retro/common.hpp>
#include <retro/platform.hpp>
#include <retro/format.hpp>
#include <retro/diag.hpp>
#include <retro/arch/interface.hpp>
#include <retro/ldr/interface.hpp>
#include <retro/ir/insn.hpp>

using namespace retro;


#include <retro/ir/routine.hpp>
#include <retro/arch/x86/regs.hxx>
#include <retro/robin_hood.hpp>

#include <retro/ir/z3x.hpp>

#include <retro/opt/interface.hpp>
#include <retro/opt/utility.hpp>

#include <retro/analysis/image.hpp>
#include <retro/analysis/workspace.hpp>
#include <retro/analysis/method.hpp>
#include <retro/analysis/callbacks.hpp>

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
	auto tmp_dir  = std::filesystem::temp_directory_path();
	auto in		  = tmp_dir / "retrotmp.c";
	auto out		  = tmp_dir / "retrotmp.exe";

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


static void whole_program_analysis_test(analysis::image* img) {
	static auto analyse_rva_if_code = [](analysis::image* img, u64 rva) {
		if (auto scn = img->find_section(rva); scn && scn->execute) {
			if (!img->lookup_method(rva)) {
				//fmt::printf("queued analysis of sub_%llx\n", rva + img->base_address);
				analysis::lift_async(img, rva);
			}
		}
	};

	analysis::on_irp_complete.insert([](ir::routine* r, analysis::ir_phase ph) {
		if (ph == analysis::IRP_INIT) {
			auto m  = r->method.get();
			auto va = m->rva + m->img->base_address;
			//fmt::printf("sub_%llx: " RC_GRAY " # Successfully lifted " RC_VIOLET "%llu" RC_GRAY " instructions into " RC_GREEN "%llu" RC_RED " (avg: ~%llu/ins)" RC_RESET " #\n", va,
			//	 m->init_info.stats_minsn_disasm, m->init_info.stats_insn_lifted, m->init_info.stats_insn_lifted / (m->init_info.stats_minsn_disasm ? m->init_info.stats_minsn_disasm : 1));

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

static ref<ir::routine> analysis_test(analysis::image* img, const std::string& name, u64 rva) {
	analysis::on_irp_complete.insert([](ir::routine* r, analysis::ir_phase ph) {
		if (ph == analysis::IRP_INIT) {
			auto m  = r->method.get();
			auto va = m->rva + m->img->base_address;
			fmt::printf("sub_%llx: " RC_GRAY " # Successfully lifted " RC_VIOLET "%llu" RC_GRAY " instructions into " RC_GREEN "%llu" RC_RED " (avg: ~%llu/ins)" RC_RESET " #\n",
			 va, 	 m->init_info.stats_minsn_disasm, m->init_info.stats_insn_lifted, m->init_info.stats_insn_lifted / (m->init_info.stats_minsn_disasm ? m->init_info.stats_minsn_disasm : 1));
		}
	});

	// Demo.
	//
	auto method = analysis::lift_async(img, rva);
	auto rtn = method->wait_for_irp(analysis::IRP_INIT);

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
	return rtn;
}

// Demo wrappers.
//
static void analysis_test_from_image_va(std::filesystem::path path, u64 va) {
	// Load the binary.
	//
	auto ws = analysis::workspace::create();
	auto img = ws->load_image(path).value();
	fmt::println("-> loader:  ", img->ldr.get_name());
	fmt::println("-> machine: ", img->arch.get_name());

	// Call the demo code.
	//
	//whole_program_analysis_test(img);
	analysis_test(img, fmt::str("sub_%llx", va), va - img->base_address);
}
static void analysis_test_from_source(std::string src) {
	// Determine flags.
	//
	std::string flags = "-O1";
	if (auto it = src.find("// clang: "); it != std::string::npos) {
		std::string_view new_flags{src.begin() + it + sizeof("// clang: ") - 1, src.end()};
		auto p = new_flags.find_first_of("\r\n");
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
	auto ws	= analysis::workspace::create();
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
			/*analysis::apply_cc_info(r);
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
		std::string test_file = "S:\\Projects\\Retro\\tests\\stackanalysis.c";
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