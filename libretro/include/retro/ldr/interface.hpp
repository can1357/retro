#pragma once
#include <retro/common.hpp>
#include <retro/diag.hpp>
#include <retro/rc.hpp>
#include <retro/interface.hpp>

namespace retro::ldr {
	// Image loader instance.
	//
	struct image;
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
};