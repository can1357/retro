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

int main(int argv, const char** args) {
	platform::setup_ansi_escapes();
	
	// Map the file.
	//
	auto res = load_image(args[0]);
	if (!res) {
		res.error().raise();
	}

	// Stuff!
	//
	ref img = std::move(res).value();
	fmt::println(" -> identified as: ", ldr::instance::find(img->ldr_hash)->get_name());
}