#include <retro/ir/value.hpp>
#include <retro/ir/insn.hpp>

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
