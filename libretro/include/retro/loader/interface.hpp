#pragma once
#include <retro/common.hpp>
#include <retro/diag.hpp>
#include <retro/rc.hpp>
#include <retro/interface_manager.hpp>

namespace retro::doc {
	struct image;
};

namespace retro::ldr {
	struct instance : interface_manager<instance> {
		// Gets the associated extension list.
		//
		virtual std::span<const std::string_view> get_extensions() = 0;

		// Returns true if the given binary blob's magic matches this format.
		//
		virtual bool validate(std::span<const u8> data) = 0;

		// Loads the binary blob into an image.
		//
		virtual diag::expected<ref<doc::image>> load(std::span<const u8> data) = 0;
	};
};