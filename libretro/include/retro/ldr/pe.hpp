#pragma once
#include <retro/common.hpp>
#include <retro/ldr/interface.hpp>

namespace retro::ldr {
	// Hashcodes for each instance defined.
	//
	enum {
		coff_pe = "coff-pe"_ihash,
	};

	// Define the instance.
	//
	struct pe_loader final : instance {
		// Implement the interface.
		//
		std::span<const std::string_view> get_extensions();
		bool										 match(std::span<const u8> data);
		diag::expected<ref<image>>			 load(std::span<const u8> data);
	};
};