#pragma once
#include <retro/common.hpp>
#include <retro/dyn.hpp>
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

	// RVA-Sorted set helper.
	//
	template<typename Tv, typename T>
	inline void insert_into_rva_set(Tv& set, T&& value) {
		auto it = range::lower_bound(set, value, [](auto& a, auto& b) { return a.rva < b.rva; });
		set.insert(it, std::forward<T>(value));
	}

	// Image type.
	//
	struct image : dyn<image> {
		// Base address.
		//
		u64 base_address = 0;

		// Kind of image.
		//
		image_kind kind = image_kind::unknown;

		// Identified loader and architecture.
		//
		interface::hash ldr_hash  = 0;
		interface::hash arch_hash = 0;
		// TODO: env_hash / abi_hash

		// Image name if known.
		//
		std::string image_name = {};

		// Raw data as mapped to memory.
		//
		std::vector<u8> raw_data = {};

		// Descriptor tables.
		//
		std::vector<section> sections = {};	 // Sorted! Use insert_into_rva_set.
		std::vector<reloc>	relocs	= {};	 // Sorted! Use insert_into_rva_set.
		std::vector<symbol>	symbols	= {};	 // Sorted! Use insert_into_rva_set.

		// RVA of the entry points.
		//
		std::vector<u64> entry_points = {};
	};
};