#include <retro/ir/value.hpp>
#include <retro/ir/insn.hpp>

#pragma once
#include <retro/common.hpp>
#include <retro/dyn.hpp>
#include <retro/format.hpp>
#include <retro/ir/types.hpp>
#include <retro/list.hpp>
#include <retro/rc.hpp>

// Automatic cross-reference tracking system, similar to LLVM's.
//
namespace retro::ir {
	// Ensure no references left on destruction.
	//
	value::~value() {
		for (auto it = use_list.begin(); it != use_list.end();) {
			if (auto* ins = it->user->get_if<ir::insn>()) {
				RC_ASSERTS("Destroying value with lingering uses.", ins->is_orphan());
			}
			auto next = std::next(it);
			list::unlink(it.get());
			it = next;
		}
	}
};
