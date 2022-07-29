#pragma once
#include <optional>
#include <retro/arch/mreg.hpp>

namespace retro::arch {
	// Memory operand details.
	//
	struct mem {
		u16  width = 0;	// Access size in bits.
		u16  segv  = 0;	// Segment by value.
		mreg segr  = {};	// Segment by register.
		mreg base  = {};	// Base register.
		mreg index = {};	// Index register.
		i32  scale = 0;	// Must be non-zero if index is given.
		i64  disp  = 0;	// Displacement.

		// String conversion.
		//
		std::string to_string(instance* a = nullptr, u64 ip = 0) const;
	};

	// Immediate operand details.
	//
	struct imm {
		u16  width		  = 0;		// Effective size in bits.
		bool is_signed	  = false;	// Set if signed.
		bool is_relative = false;	// Set if relative (from ip prior to execution regardless of arch).

		union {
			u64 u = 0;
			i64 s;
		};

		// String conversion.
		//
		std::string to_string(u64 ip = 0) const;
	};

	// Common operand type.
	//
	enum class mop_type : u8 { none, reg, mem, imm };
	struct mop {
		mop_type type;
		union {
			mreg r;
			mem  m;
			imm  i;
		};

		// Default copy / construct.
		//
		constexpr mop() : type(mop_type::none), i{} {}
		constexpr mop(std::nullopt_t) : mop() {}
		constexpr mop(const mop&)				 = default;
		constexpr mop& operator=(const mop&) = default;

		// Construction by type.
		//
		constexpr mop(mreg v) : type(mop_type::reg), r(v) {}
		constexpr mop(mem v) : type(mop_type::mem), m(v) {}
		constexpr mop(imm v) : type(mop_type::imm), i(v) {}

		// String conversion.
		//
		std::string to_string(instance* a = nullptr, u64 ip = 0) const {
			switch (type) {
				case mop_type::reg:
					return r.to_string(a);
				case mop_type::mem:
					return m.to_string(a, ip);
				case mop_type::imm:
					return i.to_string(ip);
				default:
					return {};
			}
		}
	};

	// Instruction type.
	//
	static constexpr size_t max_mop_count = 8;
	struct minsn {
		// Identifiers.
		//
		u32 arch		  = 0;  // Architecture id.
		u32 mnemonic  = 0;  // Identifier of the mnemonic.
		u64 modifiers = 0;  // Modifier set.

		// Instruction details.
		//
		u16 effective_width	 = 0;	 // Effective width if used.
		u16 length : 8			 = 0;	 // Length of the instruction.
		u16 operand_count : 4 = 0;	 // Number of operands present.
		u16 is_privileged : 1 = 0;	 // Set if privileged instruction.

		// Operand list.
		//
		mop op[max_mop_count] = {};

		// Gets the name of the instruction.
		//
		std::string_view name() const;

		// Formats the instruction.
		//
		std::string to_string(u64 ip = 0) const;

		// Wrapper around operand list.
		//
		constexpr std::span<mop>		 operands() { return {op, operand_count}; }
		constexpr std::span<const mop> operands() const { return {op, operand_count}; }
	};
};