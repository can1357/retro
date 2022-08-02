#pragma once
#include <retro/common.hpp>
#include <retro/diag.hpp>
#include <retro/interface.hpp>
#include <retro/rc.hpp>
#include <retro/arch/callconv.hpp>
#include <retro/arch/mreg.hpp>
#include <retro/arch/minsn.hpp>
#include <retro/ir/types.hpp>
#include <retro/func.hpp>
#include <bit>

namespace retro::ir {
	struct basic_block;
	struct insn;
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

		// ABI information.
		//
		virtual const call_conv_desc* get_cc_desc(call_conv cc) = 0;

		// Register information.
		//
		virtual mreg		get_stack_register() = 0;
		virtual mreg_info get_register_info(mreg r) { return {r}; }
		virtual void		for_each_subreg(mreg r, function_view<void(mreg)> f) {}
		virtual ir::insn* explode_write_reg(ir::insn* i) { return i; }

		// Lifting and disassembly.
		//
		virtual bool		 disasm(std::span<const u8> data, minsn* out)		  = 0;
		virtual diag::lazy lift(ir::basic_block* bb, const minsn& ins, u64 ip) = 0;

		// Formatting.
		//
		virtual std::string_view name_register(mreg r)						 = 0;
		virtual std::string_view name_mnemonic(u32 id)						 = 0;
		virtual std::string		 format_minsn_modifiers(const minsn& i) = 0;

		// Architectural details.
		//
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