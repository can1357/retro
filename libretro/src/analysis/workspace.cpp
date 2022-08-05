#include <retro/analysis/workspace.hpp>
#include <retro/analysis/image.hpp>
#include <retro/ldr/interface.hpp>
#include <retro/platform.hpp>

namespace retro::analysis {
	// Errors.
	//
	RC_DEF_ERR(file_read_err,      "failed to read file '%'")
	RC_DEF_ERR(no_matching_loader, "failed to identify the image loader")

	// Loads an image from filesystem/memory.
	//
	diag::expected<ref<image>> workspace::load_image(const std::filesystem::path& path) {
		auto view = platform::map_file(path);
		if (!view) {
			return err::file_read_err(path);
		}
		auto res = load_image_in_memory(view);
		if (res && res.value()->name.empty()) {
			res.value()->name = path.filename().string();
		}
		return res;
	}
	diag::expected<ref<image>> workspace::load_image_in_memory( std::span<const u8> data ) {
		auto loader = ldr::instance::find_if([&](auto& l) { return l->match(data); });
		if (!loader) {
			return err::no_matching_loader();
		}
		auto result = loader->load(data);
		if (result) {
			std::unique_lock _g{image_list_lock};
			image_list.emplace_back(result.value())->ws = this;
		}
		return result;
	}
};