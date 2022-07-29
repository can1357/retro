#pragma once
#include <retro/common.hpp>
#include <retro/diag.hpp>
#include <retro/interface.hpp>
#include <retro/rc.hpp>
#include <retro/arch/mreg.hpp>
#include <retro/arch/minsn.hpp>
#include <bit>

// Common architecture interface.
//
namespace retro::arch {
	struct instance : interface::base<instance> {
		// TODO: Lifter
		// TODO: Context for emulation.

		// Disassembly and formatting.
		//
		virtual bool				 disasm(std::span<const u8> data, minsn* out)			 = 0;
		virtual std::string_view name_register(mreg r)											 = 0;
		virtual std::string_view name_mnemonic(u32 id)											 = 0;
		virtual std::string		 format_minsn_modifiers(const minsn& i)					 = 0;

		// Architectural details.
		//
		virtual bool		  is_physical()					  = 0;
		virtual std::endian get_byte_order()				  = 0;
		virtual u32			  get_pointer_width()			  = 0;
		virtual u32			  get_effective_pointer_width() = 0;
	};
	using handle = instance::handle;
};