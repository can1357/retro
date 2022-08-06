#pragma once
#include <retro/common.hpp>
#include <retro/diag.hpp>
#include <retro/rc.hpp>
#include <retro/interface.hpp>

namespace retro::core {
	struct image;
};

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
		virtual diag::expected<ref<core::image>> load(std::span<const u8> data) = 0;
	};
	using handle = instance::handle_t;
};