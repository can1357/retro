#include <retro/core/workspace.hpp>
#include <retro/core/image.hpp>
#include <retro/ldr/interface.hpp>
#include <retro/platform.hpp>

namespace retro::core {
	// Errors.
	//
	RC_DEF_ERR(file_read_err,      "failed to read file '%'")
	RC_DEF_ERR(no_matching_loader, "failed to identify the image loader")

	// Loads an image from filesystem/memory.
	//
	diag::expected<ref<image>> workspace::load_image(const std::filesystem::path& path, ldr::handle loader) {
		auto view = platform::map_file(path);
		if (!view) {
			return err::file_read_err(path);
		}
		auto res = load_image_in_memory(view, loader);
		if (res && res.value()->name.empty()) {
			res.value()->name = path.filename().string();
		}
		return res;
	}
	diag::expected<ref<image>> workspace::load_image_in_memory(std::span<const u8> data, ldr::handle loader) {
		if (!loader)
			loader = ldr::instance::find_if([&](auto& l) { return l->match(data); });
		if (!loader) {
			return err::no_matching_loader();
		}
		auto result = loader->load(data);
		if (result) {
			std::unique_lock _g{image_list_mtx};
			image_list.emplace_back(result.value())->ws = this;
		}
		return result;
	}
};