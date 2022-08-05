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

	// Helper for builtin callbacks.
	//
#define RC_INSTALL_CB(list, name, ...)                                              \
	static typename decltype(list)::return_type RC_CONCAT(hook_, name)(__VA_ARGS__); \
	RC_INITIALIZER { list.insert(&RC_CONCAT(hook_, name)); };                        \
	static typename decltype(list)::return_type RC_CONCAT(hook_, name)(__VA_ARGS__)
};