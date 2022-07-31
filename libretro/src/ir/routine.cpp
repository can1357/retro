#include <retro/ir/routine.hpp>
#include <retro/ir/basic_block.hpp>

namespace retro::ir {
	// Creates or removes a block.
	//
	basic_block* routine::add_block() {
		dirty_cfg();
		auto* blk = blocks.emplace_back(make_rc<basic_block>()).get();
		blk->rtn	 = this;
		blk->name = next_blk_name++;
		return blk;
	}
	auto routine::del_block(basic_block* b) {
		dirty_cfg();
		RC_ASSERT(b->predecessors.empty());
		RC_ASSERT(b->successors.empty());
		for (auto it = blocks.begin();; ++it) {
			RC_ASSERT(it != blocks.end());
			if (it->get() == b) {
				return blocks.erase(it);
			}
		}
		RC_UNREACHABLE();
	}

	// Topologically sorts the basic block list.
	//
	void routine::topological_sort() {
		u32 tmp = narrow_cast<u32>(blocks.size());

		basic_block* ep = get_entry();
		graph::dfs(ep, [&](basic_block* b) { b->name = --tmp; });
		RC_ASSERT(ep->name == 0);
		range::sort(blocks, [](auto& a, auto& b) { return a->name < b->name; });
	}

	// Simple renaming by order.
	//
	void routine::rename_insns() {
		next_ins_name = 0;
		for (auto& bb : blocks) {
			for (auto i : *bb) {
				i->name = next_ins_name++;
			}
		}
	}
	void routine::rename_blocks() {
		next_blk_name = 0;
		for (auto& bb : blocks) {
			bb->name = next_blk_name++;
		}
	}

	// String conversion.
	//
	std::string routine::to_string(fmt_style s) const {
		std::string result;
		if (ip != NO_LABEL) {
			result = fmt::str(RC_ORANGE "sub_%llx " RC_RESET " [%p]", ip, this);
		} else {
			result = fmt::str(RC_ORANGE "sub_# " RC_RESET " [%p]", this);
		}

		if (s == fmt_style::concise) {
			return result;
		} else {
			for (auto&& blk : *this) {
				result += "\n" + fmt::shift(blk->to_string());
			}
			result += RC_RESET;
			return result;
		}
	}

	// Clear all block references on destruction to prevent an error being raised.
	//
	routine::~routine() {
		for (auto& bb : blocks) {
			bb->replace_all_uses_with(std::nullopt);
		}
	}
};