#pragma once
#include <retro/common.hpp>
#include <retro/diag.hpp>
#include <retro/rc.hpp>
#include <retro/interface.hpp>
#include <retro/ldr/image.hpp>

namespace retro::ldr {
	// Hashcodes for each builtin instance defined.
	//
	enum {
		coff_pe = "coff-pe"_ihash,
	};

	// Common loader interface.
	//
	struct instance : interface::base<instance> {
		// Gets the associated extension list.
		//
		virtual std::span<const std::string_view> get_extensions() = 0;

		// Returns true if the given binary blob's magic matches this format.
		//
		virtual bool match(std::span<const u8> data) = 0;

		// Loads the binary blob into an image.
		//
		virtual diag::expected<ref<image>> load(std::span<const u8> data) = 0;
	};
	using handle = instance::handle;

	// Errors.
	//
	RC_DEF_ERR(file_read_err, "failed to read file '%'")
	RC_DEF_ERR(no_matching_loader, "failed to identify the image loader")

	// Loads an image from memory.
	//
	static diag::expected<ref<ldr::image>> load_from_memory(std::span<const u8> data) {
		auto loader = ldr::instance::find_if([&](auto& l) { return l->match(data); });
		if (!loader) {
			return err::no_matching_loader();
		}
		return loader->load(data);
	}

	// Loads an image from filesystem.
	//
	static diag::expected<ref<ldr::image>> load_from_file(const std::filesystem::path& path) {
		auto view = platform::map_file(path);
		if (!view) {
			return err::file_read_err(path);
		}
		auto res = load_from_memory(view);
		if (res && res.value()->image_name.empty()) {
			res.value()->image_name = path.filename().string();
		}
		return res;
	}
};