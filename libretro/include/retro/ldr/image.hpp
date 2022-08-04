#pragma once
#include <retro/common.hpp>
#include <retro/dyn.hpp>
#include <retro/arch/callconv.hpp>
#include <string>
#include <vector>
#include <variant>

namespace retro::ldr {
	// Image kind.
	//
	enum class image_kind : u8 {
		unknown = 0,
		executable,			// Executable.
		dynamic_library,	// Dynamic library.
		static_library,	// Static library.
		object_file,		// Single unit of compilation.
		memory_dump,		// Process memory dump, meaning multiple images are present.
	};

	// Relocation entries.
	//
	enum class reloc_kind : u8 {
		none		= 0,
		absolute = 1,
		// RVA, etc, should be handled by the loader, only inserted as none to hint the reference.
	};
	struct reloc {
		// RVA of the relocated address.
		//
		u64 rva = 0;

		// RVA of the target address or the symbol name.
		//
		std::variant<u64, std::string> target;

		// Kind of relocation.
		//
		reloc_kind kind = reloc_kind::none;
	};

	// Section entries.
	//
	struct section {
		// RVA of the section start/send.
		//
		u64 rva		= 0;
		u64 rva_end = 0;

		// Section name.
		//
		std::string name;

		// Memory details.
		//
		u8 write : 1	= false;
		u8 execute : 1 = false;
	};

	// Symbol entries.
	//
	struct symbol {
		// RVA of the symbol.
		//
		u64 rva		= 0;

		// Symbol name.
		//
		std::string name;
	};

	// RVA-Sorted set helpers.
	//
	template<typename Tv, typename T>
	inline void insert_into_rva_set(std::vector<Tv>& set, T&& value) {
		auto it = range::lower_bound(set, value, [](auto& a, auto& b) { return a.rva < b.rva; });
		set.insert(it, std::forward<T>(value));
	}
	template<typename Tv>
	inline const Tv* find_rva_set_le(const std::vector<Tv>& set, u64 rva) {
		auto it = std::upper_bound(set.begin(), set.end(), rva, [](auto& a, auto& b) { return a < b.rva; });
		if (it != set.begin()) {
			--it;
			if (it->rva <= rva)
				return std::to_address(it);
		}
		return nullptr;
	}
	template<typename Tv>
	inline const Tv* find_rva_set_eq(const std::vector<Tv>& set, u64 rva) {
		auto it = std::lower_bound(set.begin(), set.end(), rva, [](auto& a, auto& b) { return a < b.rva; });
		if (it != set.end() && it->rva == rva) {
			return std::to_address(it);
		}
		return nullptr;
	}

	// Image type.
	//
	struct image final {
		// Base address.
		//
		u64 base_address = 0;

		// Raw data as mapped to memory.
		//
		std::vector<u8> raw_data = {};

		// Image name if known.
		//
		std::string image_name = {};

		// Kind of image.
		//
		image_kind kind = image_kind::unknown;

		// Identified loader and architecture.
		//
		interface::hash ldr_hash  = 0;
		interface::hash arch_hash = 0;

		// ABI and environment details.
		//
		bool				 env_privileged = false;
		arch::call_conv default_call_conv = arch::call_conv::unknown;

		// Descriptor tables.
		// - Sorted by RVA, use insert_into_rva_set for insertion.
		//
		std::vector<section> sections = {};
		std::vector<reloc>	relocs	= {};
		std::vector<symbol>	symbols	= {};

		// RVA of the entry points.
		// - Unordered, first entry should be main() or equivalent.
		//
		std::vector<u64> entry_points = {};

		// Finds a section entry given an RVA.
		//
		const section* find_section(u64 rva) const {
			auto scn = find_rva_set_le(sections, rva);
			if (scn && scn->rva_end <= rva)
				scn = nullptr;
			return scn;
		}

		// Gets a slice given an RVA.
		// - Allowed to be out of boundaries, will return empty span.
		//
		std::span<const u8> slice(u64 rva) const {
			if (rva > raw_data.size()) {
				return {};
			}
			return std::span{raw_data}.subspan(rva);
		}
	};
};
