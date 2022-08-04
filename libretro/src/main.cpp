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

#include <retro/opt/interface.hpp>
#include <retro/opt/utility.hpp>

#include <retro/analysis/workspace.hpp>
#include <retro/analysis/method.hpp>



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

static ref<ir::routine> analysis_test(analysis::domain* dom, const std::string& name, u64 rva) {
	// Demo.
	//
	auto method = analysis::lift_async(dom, rva);
	auto rtn = method->wait_for_irp(analysis::IRP_BUILT);

	// Print statistics.
	//
	fmt::printf(RC_WHITE " ----- Routine '%s' -------\n", name.data());
	fmt::printf(RC_GRAY " # Successfully lifted " RC_VIOLET "%llu" RC_GRAY " instructions into " RC_GREEN "%llu" RC_RED " (avg: ~%llu/ins) #\n" RC_RESET, method->stats.minsn_disasm,
		 method->stats.insn_lifted, method->stats.insn_lifted / method->stats.minsn_disasm);
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
	auto img = ldr::load_from_file(path).value();

	// Create the workspace.
	//
	auto ws		 = analysis::workspace::create();
	auto machine = arch::instance::lookup(img->arch_hash);
	auto loader	 = ldr::instance::lookup(img->ldr_hash);
	RC_ASSERT(loader && machine);
	fmt::println("-> loader:  ", loader->get_name());
	fmt::println("-> machine: ", machine->get_name());

	// Call the demo code.
	//
	auto dom = ws->add_image(std::move(img));
	analysis_test(dom, fmt::str("sub_%llx", va), va - dom->img->base_address);
}
static void analysis_test_from_source(std::string src) {
	// Determine flags.
	//
	std::string flags = "-O1";
	if (auto it = src.find("// clang: "); it != std::string::npos) {
		std::string_view new_flags{src.begin() + sizeof("// clang: ") - 1, src.end()};
		auto p = new_flags.find_first_of("\r\n");
		if (p != std::string::npos) {
			new_flags = new_flags.substr(0, p);
		}
		flags.assign(new_flags);
	}

	// Compile the source code.
	//
	auto bin = compile(src, flags.data());
	auto img = ldr::load_from_memory(bin).value();

	// Create the workspace.
	//
	auto ws		 = analysis::workspace::create();
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
		if (sym.name.starts_with("_"))
			continue;
		auto r = analysis_test(dom, sym.name, sym.rva);
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
		std::string test_file = "S:\\Projects\\Retro\\tests\\cc32.c";
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