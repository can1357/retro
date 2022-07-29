#pragma once
#include <retro/arch/x86.hpp>
#include <retro/arch/x86/regs.hxx>

// Define conversion of Zydis types to Retro types.
//
namespace retro::arch::x86 {
	// Lookup maps.
	//
	namespace detail {
		inline constexpr std::array<reg, ZYDIS_REGISTER_MAX_VALUE> reg_from_zy = []() {
			std::array<reg, ZYDIS_REGISTER_MAX_VALUE> result{reg::none};
#define MAPIF(a, b)                        \
	if constexpr (b != ZYDIS_REGISTER_NONE) \
		result[u32(b)] = reg::a;
			RC_VISIT_ARCH_X86_REG(MAPIF)
#undef MAPIF
			return result;
		}();
	};

	// Conversion from ZydisRegister to x86::reg.
	//
	inline constexpr reg to_reg(ZydisRegister reg) { return detail::reg_from_zy[u32(reg)]; }

	// Conversion of ZydisDecodedOperand into mop.
	//
	inline constexpr mop to_mop(const ZydisDecodedOperand& op, u8 insn_len) {
		switch (op.type) {
			case ZYDIS_OPERAND_TYPE_REGISTER: {
				return mreg{to_reg(op.reg.value)};
			}
			case ZYDIS_OPERAND_TYPE_MEMORY: {
				return mem{
					 .width = op.size,
					 .segr  = to_reg(op.mem.segment),
					 .base  = to_reg(op.mem.base),
					 .index = to_reg(op.mem.index),
					 .scale = op.mem.scale,
					 .disp  = (op.mem.disp.has_displacement ? op.mem.disp.value : 0),
				};
			}
			case ZYDIS_OPERAND_TYPE_POINTER: {
				return mem{
					 .width = op.size,
					 .segv  = op.ptr.segment,
					 .disp  = op.ptr.offset,
				};
			}
			case ZYDIS_OPERAND_TYPE_IMMEDIATE: {
				return imm{
					 .width		  = op.size,
					 .is_signed	  = (bool) op.imm.is_signed,
					 .is_relative = (bool) op.imm.is_relative,
					 .u			  = op.imm.value.u + (op.imm.is_relative ? insn_len : 0),
				};
			}
			default:
				RC_UNREACHABLE();
		}
	}

};