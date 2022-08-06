#pragma once
#include <retro/common.hpp>
#include <retro/arch/mreg.hpp>
#include <vector>

namespace retro::arch {
	// Call convention enumerator.
	//
	enum class call_conv : u8 {
		unknown = 0,  // Unknown.

		// i386:
		cdecl_i386		 = 1,
		stdcall_i386	 = 2,
		thiscall_i386	 = 3,
		msthiscall_i386 = 4,
		msfastcall_i386 = 5,

		// x86_64:
		msabi_x86_64	 = 1,
		sysv_x86_64		 = 2,
	};

	// Describes an architecture specific calling convention.
	// - Volatility information is not included as more and more compilers optimize calling conventions
	//   at link time based for hot/cold calls and access to a volatile register after a call would be
	//   treated the same as a conditionally nonvolatile one as it'd be upgraded based on call-site information.
	//
	struct call_conv_desc {
		// Name of the calling convention, ABI name, and attribute name if not default.
		//
		std::string_view name		= {};
		std::string_view abi_name	= {};
		std::string_view attribute = {};

		// Return value registers.
		//
		small_array<mreg> retval_gpr = {};
		small_array<mreg> retval_fp  = {};

		// Argument registers.
		//
		small_array<mreg> argument_gpr = {};
		small_array<mreg> argument_fp	 = {};

		// Valid frame pointers.
		//
		small_array<mreg> frame_gpr = {};

		// Register holding the count of FP registers used by the caller upon calling a vararg function, or none if not relevant.
		//
		mreg fp_varg_counter = {};

		// Set if argument types are taken into account while counting the arguments for assigning a register.
		// - Ex:.
		//       [false] SysV  x86_64 would assign argument_fp[0] to X
		//       [true]  MSABI x86_64 would assign argument_fp[1] to X
		//   void test(int, float);
		//
		bool combined_argument_counter = false;

		// Whether or not stack is adjusted by caller or callee on return.
		// - Forced true if vararg.
		//
		bool sp_caller_adjusted = false;

		// Size of home area and stack alignment.
		//
		u32 home_size	  = 0;
		u32 sp_alignment = 0;

		// TODO: ABI information, eg: pass by ref or breakdown or structures, etc.
		//
	};
}