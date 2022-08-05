#pragma once
#include <atomic>
#include <retro/arch/callconv.hpp>
#include <retro/arch/interface.hpp>
#include <retro/common.hpp>
#include <retro/ir/routine.hpp>
#include <retro/rc.hpp>
#include <retro/task.hpp>
#include <retro/robin_hood.hpp>

namespace retro::analysis {
	struct image;

	// IR Phases.
	//
	enum ir_phase : u8 {
		// Initial IR representation.
		//
		IRP_INIT,

		// Pseudo indices.
		//
		IRP_MAX,
	};

	// Analysis results and statistics for each IRP.
	//
	struct irp_init_analysis {
		// Statistics.
		//
		u64 stats_minsn_disasm = 0;  // Machine instructions diassembled.
		u64 stats_insn_lifted = 0;	 // IR instructions created to represent the disassembled instructions.
		u64 stats_block_count  = 0;  // Blocks parsed.

		// Difference in stack pointer after a call to this function.
		//
		i32 stack_delta = 0;

		// Frame register if any used and difference from initial SP.
		//
		arch::mreg frame_reg			= {};
		i32		  frame_reg_delta = 0;

		// Min/Max of non-indexed accesses.
		//
		i32 min_access = 0;
		i32 max_access = 0;

		// Layout of registers saved on the stack frame.
		//
		flat_umap<i32, arch::mreg> save_area_layout = {};

		// Flags.
		//
		u32 is_valid : 1	  = false;
		u32 is_noreturn : 1 = false;
		u32 is_const : 1	  = false;
	};

	// A method describes a collection of IR routines in different states and
	// an AST belonging to a single assembly function in the image.
	//
	struct method {
		// Owning image.
		//
		weak<image> img = {};

		// RVA of the method in the image.
		//
		u64 rva = 0;

		// Architecture and the calling convention.
		//
		arch::handle					 arch = {};
		const arch::call_conv_desc* cc	= nullptr;

		// Analysis information saved by each IRP.
		//
		irp_init_analysis init_info = {};

		// IR of the routine for each phase.
		//
		ref<ir::routine> routine[IRP_MAX] = {};

		// Set of flags containing details about each IR phase being complete.
		//
		std::atomic<u32> irp_mask = 0;

		// Observers.
		//
		bool irp_present(ir_phase p) const { return (irp_mask.load(std::memory_order::relaxed) & (1u << p)) != 0; }
		bool irp_busy(ir_phase p) const { return !irp_present(p) && routine[p] != nullptr; }
		bool irp_failed(ir_phase p) const { return irp_present(p) && routine[p] == nullptr; }
		bool irp_complete(ir_phase p) const { return irp_present(p) && routine[p] != nullptr; }
		ref<ir::routine> get_irp(ir_phase p) const {
			if (irp_complete(p)) {
				return routine[p];
			}
			return nullptr;
		}

		// Waits for an IR phase to be complete.
		//
		ref<ir::routine> wait_for_irp(ir_phase p) const {
			u32 mask = 1u << p;
			while (true) {
				// Break out if complete or not present.
				//
				auto expected = irp_mask.load(std::memory_order::relaxed);
				if (expected & mask) {
					return routine[p];
				}
				if (routine[p] == nullptr) {
					return nullptr;
				}

				// Wait for a change.
				//
				irp_mask.wait(expected, std::memory_order::relaxed);
			}
		}


		// Lifts a basic block into the IRP_INIT IR from the given RVA.
		//
		unique_task<ir::basic_block*> build_block(u64 rva);
	};

	// Lifts a new method into the image at the given RVA, if it does not already exist.
	// - If there is an existing entry with arch/cc matching, returns it, otherwise clears it and lifts from scratch.
	//
	ref<method> lift(image* img, u64 rva, arch::handle arch = {}, const arch::call_conv_desc* cc = nullptr);
	ref<method> lift_async(image* img, u64 rva, arch::handle arch = {}, const arch::call_conv_desc* cc = nullptr);
}