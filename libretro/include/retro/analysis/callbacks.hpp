#pragma once
#include <retro/callback.hpp>
#include <retro/ir/insn.hpp>
#include <retro/analysis/method.hpp>

// Analysis callbacks.
//
namespace retro::analysis {
	// Handles resolution of an XJMP instruction with non-constant target, for instance in the case of jump tables.
	//
	inline handler_list<method*, ir::insn*> indirect_xjmp_resolver = {};

	// Notifications invoked after the lifting of an epilogue/prologue block.
	//
	inline notification_list<ir::basic_block*> on_irp_init_prologue = {};
	inline notification_list<ir::basic_block*> on_irp_init_epilogue = {};

	// Notifications invoked on creation of an XCALL instruction.
	//
	inline notification_list<ir::insn*> on_irp_init_xcall = {};

	// Notified when an IRP is complete.
	//
	inline notification_list<ir::routine*, ir_phase> on_irp_complete = {};
};