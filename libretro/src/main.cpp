#include <retro/common.hpp>
#include <retro/platform.hpp>
#include <retro/format.hpp>
#include <retro/diag.hpp>
#include <retro/arch/interface.hpp>
#include <retro/ldr/interface.hpp>
#include <retro/ldr/image.hpp>
#include <retro/ir/insn.hpp>

using namespace retro;

RC_DEF_ERR(file_read_err, "failed to read file '%'")
RC_DEF_ERR(no_matching_loader, "failed to identify the image loader")


// Loads an image from memory.
//
static diag::expected<ref<ldr::image>> load_image(std::span<const u8> data) {
	auto loader = ldr::instance::find_if([&](auto& l) { return l->match(data); });
	if (!loader) {
		return err::no_matching_loader();
	}
	return loader->load(data);
}

// Loads an image from filesystem.
//
static diag::expected<ref<ldr::image>> load_image(const std::filesystem::path& path) {
	auto view = platform::map_file(path);
	if (!view) {
		return err::file_read_err(path);
	}
	return load_image(view);
}

#include <retro/ir/routine.hpp>
#include <retro/arch/x86/regs.hxx>

int main(int argv, const char** args) {
	platform::setup_ansi_escapes();
	
	// Map the file.
	//
	auto res = load_image("../tests/libretro.exe");
	if (!res) {
		res.error().raise();
	}

	// Print file details.
	//
	ref  img		 = std::move(res).value();
	auto machine = arch::instance::find(img->arch_hash);
	auto loader	 = ldr::instance::find(img->ldr_hash);
	RC_ASSERT(loader && machine);
	fmt::println("-> loader:  ", loader->get_name());
	fmt::println("-> machine: ", machine->get_name());

	// Demo.
	//
	u64	ip		  = 0x1400072C0;
	auto	rtn	  = make_rc<ir::routine>();
	auto* bb		  = rtn->add_block();
	rtn->ip		  = ip;
	bb->ip		  = ip;

	std::span<const u8> data = img->slice(ip - img->base_address);
	for (size_t i = 0; i < 64 && !data.empty(); i++) {
		// Disasm the instruction and print it.
		//
		arch::minsn ins = {};
		if (!machine->disasm(data, &ins)) {
			fmt::println("failed to disasm\n");
			break;
		}
		fmt::println(ip, ": ", ins.to_string(ip));

		// Try lifting it and print the result.
		//
		if (auto err = machine->lift(bb, ins, ip)) {
			fmt::println("-> lifter failed with: ", err);
			break;
		}
		if (!bb->empty()) {
			auto it = std::prev(bb->end());
			while (it != bb->begin() && it->prev->ip == ip) {
				--it;
			}
			while (it != bb->end()) {
				fmt::println("          -> ", it->to_string());
				++it;
			}
		}

		// TODO: BB splitting and branch handling.
		//

		// Skip the bytes and increment IP.
		//
		data = data.subspan(ins.length);
		ip += ins.length;
	}
	return 0;
}