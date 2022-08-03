#pragma once
#include <retro/common.hpp>
#include <retro/arch/x86/regs.hxx>
#include <retro/arch/callconv.hpp>

// Calling convention descriptors.
//
namespace retro::arch::x86 {
	// x86_64:
	//
	inline constexpr call_conv_desc cc_msabi_x86_64 = {
		 .name							 = "msabi-x86_64",
		 .retval_gpr					 = {reg::rax, reg::rdx},
		 .retval_fp						 = {reg::xmm0},
		 .argument_gpr					 = {reg::rcx, reg::rdx, reg::r8, reg::r9},
		 .argument_fp					 = {reg::xmm0, reg::xmm1, reg::xmm2, reg::xmm3},
		 .fp_varg_counter				 = reg::none,
		 .combined_argument_counter = true,
		 .sp_caller_adjusted			 = true,
		 .home_size						 = 0x20,
		 .sp_alignment					 = 0x10,
	};
	inline constexpr call_conv_desc cc_sysv_x86_64 = {
		 .name							 = "sysv-x86_64",
		 .retval_gpr					 = {reg::rax, reg::rcx, reg::rdx},
		 .retval_fp						 = {reg::xmm0},
		 .argument_gpr					 = {reg::rdi, reg::rsi, reg::rdx, reg::rcx, reg::r8, reg::r9},
		 .argument_fp					 = {reg::xmm0, reg::xmm1, reg::xmm2, reg::xmm3, reg::xmm4, reg::xmm5, reg::xmm6, reg::xmm7},
		 .fp_varg_counter				 = reg::rax,
		 .combined_argument_counter = false,
		 .sp_caller_adjusted			 = true,
		 .home_size						 = 0x00,
		 .sp_alignment					 = 0x10,
	};

	// i386:
	//
	inline constexpr call_conv_desc cc_cdecl_i386 = {
		 .name							 = "cdecl-i386",
		 .attribute						 = "__cdecl",
		 .retval_gpr					 = {reg::eax, reg::edx},
		 .retval_fp						 = {reg::st0},
		 .argument_gpr					 = {},
		 .argument_fp					 = {},
		 .fp_varg_counter				 = reg::none,
		 .combined_argument_counter = false,
		 .sp_caller_adjusted			 = true,
		 .home_size						 = 0x00,
		 .sp_alignment					 = 0x00,
	};
	inline constexpr call_conv_desc cc_stdcall_i386 = {
		 .name							 = "stdcall-i386",
		 .attribute						 = "__stdcall",
		 .retval_gpr					 = {reg::eax, reg::edx},
		 .retval_fp						 = {reg::st0},
		 .argument_gpr					 = {},
		 .argument_fp					 = {},
		 .fp_varg_counter				 = reg::none,
		 .combined_argument_counter = false,
		 .sp_caller_adjusted			 = false,
		 .home_size						 = 0x00,
		 .sp_alignment					 = 0x00,
	};
	inline constexpr call_conv_desc cc_thiscall_i386 = {
		 .name							 = "thiscall-i386",
		 .attribute						 = "__thiscall",
		 .retval_gpr					 = {reg::eax, reg::edx},
		 .retval_fp						 = {reg::st0},
		 .argument_gpr					 = {reg::ecx},
		 .argument_fp					 = {},
		 .fp_varg_counter				 = reg::none,
		 .combined_argument_counter = false,
		 .sp_caller_adjusted			 = false,
		 .home_size						 = 0x00,
		 .sp_alignment					 = 0x00,
	};
	inline constexpr call_conv_desc cc_msfastcall_i386 = {
		 .name							 = "msfastcall-i386",
		 .attribute						 = "__fastcall",
		 .retval_gpr					 = {reg::eax, reg::edx},
		 .retval_fp						 = {reg::st0},
		 .argument_gpr					 = {reg::ecx, reg::edx},
		 .argument_fp					 = {},
		 .fp_varg_counter				 = reg::none,
		 .combined_argument_counter = false,
		 .sp_caller_adjusted			 = false,
		 .home_size						 = 0x00,
		 .sp_alignment					 = 0x00,
	};
};