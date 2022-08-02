#pragma once
#include <retro/common.hpp>
#include <retro/arch/x86/regs.hxx>
#include <retro/arch/callconv.hpp>

// Calling convention descriptors.
//
namespace retro::arch::x86 {
	inline constexpr call_conv_desc cc_msabi_x86_64 = {
		 .name							 = "msabi-x86_64",
		 .retval_gpr					 = {reg::rax},//+TODO: rdx
		 .retval_fp						 = {reg::xmm0},
		 .argument_gpr					 = {reg::rcx, reg::rdx, reg::r8, reg::r9},
		 .argument_fp					 = {reg::xmm0, reg::xmm1, reg::xmm2, reg::xmm3},
		 .nonvolatile_gpr				 = {reg::rbp, reg::rbx, reg::rsi, reg::rdi, reg::r12, reg::r13, reg::r14, reg::r15},
		 .nonvolatile_fp				 = {reg::xmm6, reg::xmm7, reg::xmm8, reg::xmm9, reg::xmm10, reg::xmm11, reg::xmm12, reg::xmm13, reg::xmm14, reg::xmm15},
		 .fp_varg_counter				 = reg::none,
		 .combined_argument_counter = true,
		 .sp_caller_adjusted			 = true,
		 .home_size						 = 0x20,
		 .sp_alignment					 = 0x10,
	};
	inline constexpr call_conv_desc cc_sysv_x86_64 = {
		 .name							 = "sysv-x86_64",
		 .retval_gpr					 = {reg::rax},//+TODO: rcx,rdx
		 .retval_fp						 = {reg::xmm0},
		 .argument_gpr					 = {reg::rdi, reg::rsi, reg::rdx, reg::rcx, reg::r8, reg::r9},
		 .argument_fp					 = {reg::xmm0, reg::xmm1, reg::xmm2, reg::xmm3, reg::xmm4, reg::xmm5, reg::xmm6, reg::xmm7},
		 .nonvolatile_gpr				 = {reg::rbp, reg::rbx, reg::r12, reg::r13, reg::r14, reg::r15},
		 .nonvolatile_fp				 = {},
		 .fp_varg_counter				 = reg::rax,
		 .combined_argument_counter = false,
		 .sp_caller_adjusted			 = true,
		 .home_size						 = 0x20,
		 .sp_alignment					 = 0x10,
	};
};