#pragma once
#include <retro/common.hpp>
#include <retro/arch/interface.hpp>
#include <Zycore/LibC.h>
#include <Zydis/Zydis.h>

namespace retro::arch {
	// Native disassembly.
	//
	struct x86insn {
		ZydisDecodedInstruction ins;
		ZydisDecodedOperand		ops[ZYDIS_MAX_OPERAND_COUNT_VISIBLE];

		// Operand accessor.
		//
		std::span<const ZydisDecodedOperand> operands() const { return {ops, ins.operand_count_visible}; }

		// Formatting.
		//
		std::string to_string(u64 ip = ZYDIS_RUNTIME_ADDRESS_NONE) const;
	};

	// Define the instance.
	//
	struct x86arch final : dyn<x86arch, instance> {
		ZydisDecoder	  decoder;
		ZydisMachineMode machine_mode;
		ZydisStackWidth  stack_width;
		u32				  ptr_width;
		u32				  ptr_width_eff;

		// Construction.
		//
		x86arch(ZydisMachineMode mode);

		// Helpers.
		//
		bool is_64() { return machine_mode == ZYDIS_MACHINE_MODE_LONG_64; }
		bool is_32() { return machine_mode == ZYDIS_MACHINE_MODE_LONG_COMPAT_32 || machine_mode == ZYDIS_MACHINE_MODE_LEGACY_32; }
		bool is_16() { return machine_mode == ZYDIS_MACHINE_MODE_LONG_COMPAT_16 || machine_mode == ZYDIS_MACHINE_MODE_LEGACY_16 || machine_mode == ZYDIS_MACHINE_MODE_REAL_16; }

		// ABI information.
		//
		const call_conv_desc* get_cc_desc(call_conv cc);

		// Register information.
		//
		bool test_reg_alias(mreg a, mreg b);
		bool is_subreg(mreg a);

		// Lifting and disassembly.
		//
		bool		  disasm(std::span<const u8> data, x86insn* out);
		bool		  disasm(std::span<const u8> data, minsn* out);
		diag::lazy lift(ir::basic_block* bb, const minsn& ins, u64 ip);

		// Formatting.
		//
		std::string_view name_register(mreg r);
		std::string_view name_mnemonic(u32 id);
		std::string		  format_minsn_modifiers(const minsn& i);

		// Architectural details.
		//
		std::endian get_byte_order() { return std::endian::little; }
		u32			get_pointer_width() { return ptr_width; }
		u32			get_effective_pointer_width() { return ptr_width_eff; }
	};
};