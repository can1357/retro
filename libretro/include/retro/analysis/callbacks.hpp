#pragma once
#include <retro/callback.hpp>
#include <retro/ir/insn.hpp>
#include <retro/analysis/method.hpp>

// Analysis callbacks.
//
namespace retro::analysis {
	// Handles resolution of an XJMP statement with non-constant target, for instance in the case of jump tables.
	//
	inline handler_list<method*, ir::insn*> indirect_xjmp_resolver = {};

	// Notified when an IRP is complete.
	//
	inline notification_list<ir::routine*, ir_phase> irp_complete_hook = {};
};