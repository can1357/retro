#pragma once
#include <retro/common.hpp>
#include <retro/arch/mreg.hpp>

namespace retro::arch {
	// Call convention enumerator.
	//
	enum class call_conv : i8 {
		preserve_none = -3,	// None of the callee registers are preserved (except stack pointer).
		interrupt	  = -2,	// Same semantics as preserve all except the stack data on entry, which differs based on architecture.
		preserve_all  = -1,	// Preserves all registers.
		unknown		  = 0,	// Unknown.

		// Positive range (including values not defined here) are reserved for architecture specific conventions.
		//
		msabi_x86_64 = 1,
		sysv_x86_64	 = 2,
	};

	// Describes an architecture specific calling convention.
	//
	struct call_conv_desc {
		// Name of the calling convention.
		//
		std::string_view name = {};

		// Return value registers.
		//
		small_array<mreg> retval_gpr = {};
		small_array<mreg> retval_fp  = {};

		// Argument registers.
		//
		small_array<mreg> argument_gpr = {};
		small_array<mreg> argument_fp	 = {};

		// Non-volatile registers.
		//
		small_array<mreg> nonvolatile_gpr = {};
		small_array<mreg> nonvolatile_fp	 = {};

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
		//
		bool sp_caller_adjusted = false;

		// Size of home area and stack alignment.
		//
		u32 home_size	  = 0;
		u32 sp_alignment = 0;
	};
}