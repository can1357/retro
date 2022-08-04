#pragma once
#include <atomic>
#include <retro/arch/callconv.hpp>
#include <retro/arch/interface.hpp>
#include <retro/common.hpp>
#include <retro/ir/routine.hpp>
#include <retro/rc.hpp>
#include <retro/task.hpp>

namespace retro::analysis {
	struct domain;

	// Method statistics.
	//
	struct method_stats {
		// Phase 0 statistics.
		//
		u64 minsn_disasm = 0;  // Machine instructions diassembled.
		u64 insn_lifted  = 0;  // IR instructions created to represent the disassembled instructions.
		u64 block_count  = 0;  // Blocks parsed.
	};

	// IR Phases.
	//
	enum ir_phase : u8 {
		// Initial IR representation.
		//
		IRP_BUILT,

		// Pseudo indices.
		//
		IRP_MAX,
	};

	// A method describes a collection of IR routines in different states and
	// an AST belonging to a single assembly function in the domain.
	//
	struct method {
		// Owning domain.
		//
		weak<domain> dom = {};

		// RVA of the method in the image.
		//
		u64 rva = 0;

		// Architecture and the calling convention.
		//
		arch::handle					 arch = {};
		const arch::call_conv_desc* cc	= nullptr;

		// Statistics.
		//
		method_stats stats = {};

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

		// Lifts a basic block into the IRP_BUILT IR from the given RVA.
		//
		unique_task<ir::basic_block*> build_block(u64 rva);
	};

	// Lifts a new method into the domain at the given RVA, if it does not already exist.
	// - If there is an existing entry with arch/cc matching, returns it, otherwise clears it and lifts from scratch.
	//
	ref<method> lift(domain* dom, u64 rva, arch::handle arch = {}, const arch::call_conv_desc* cc = nullptr);
}