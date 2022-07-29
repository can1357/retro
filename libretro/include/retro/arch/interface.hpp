#pragma once
#include <retro/common.hpp>
#include <retro/diag.hpp>
#include <retro/interface.hpp>
#include <retro/rc.hpp>
#include <retro/arch/mreg.hpp>
#include <retro/arch/minsn.hpp>
#include <retro/ir/types.hpp>
#include <bit>

namespace retro::ir {
	struct basic_block;
};

namespace retro::arch {
	// Hashcodes for each builtin instance defined.
	//
	enum {
		x86_32 = "x86_32"_ihash,
		x86_64 = "x86_64"_ihash,
	};

	// Common architecture interface.
	//
	struct instance : interface::base<instance> {
		// TODO: Context for emulation.

		// Lifting and disassembly.
		//
		virtual bool		 disasm(std::span<const u8> data, minsn* out)		  = 0;
		virtual diag::lazy lift(ir::basic_block* bb, const minsn& ins, u64 ip) = 0;

		// Formatting.
		//
		virtual std::string_view name_register(mreg r)											 = 0;
		virtual std::string_view name_mnemonic(u32 id)											 = 0;
		virtual std::string		 format_minsn_modifiers(const minsn& i)					 = 0;

		// Architectural details.
		//
		virtual bool		  is_physical()					  = 0;
		virtual std::endian get_byte_order()				  = 0;
		virtual u32			  get_pointer_width()			  = 0;
		virtual u32			  get_effective_pointer_width() = 0;

		// Pointer type helper.
		//
		ir::type ptr_type() {
			switch (get_pointer_width()) {
				default:
				case 64:
					return ir::type::i64;
				case 32:
					return ir::type::i32;
				case 16:
					return ir::type::i16;
			}
		}
	};
	using handle = instance::handle;
};