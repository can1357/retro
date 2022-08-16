#include <retro/ir/routine.hpp>
#include <retro/ir/basic_block.hpp>
#include <retro/core/method.hpp>
#include <retro/core/image.hpp>

namespace retro::ir {
	// Creates or removes a block.
	//
	basic_block* routine::add_block() {
		dirty_cfg();

		auto blk = make_rc<basic_block>();
		blk->rtn	 = this;
		blk->name = next_blk_name++;
		
		if (blocks.empty()) {
			entry_point = blk;
		}
		return blocks.emplace_back(std::move(blk)).get();
	}
	void routine::del_block(basic_block* b) {
		dirty_cfg();
		RC_ASSERT(b->predecessors.empty());
		RC_ASSERT(b->successors.empty());
		for (auto it = blocks.begin();; ++it) {
			RC_ASSERT(it != blocks.end());
			if (it->get() == b) {
				blocks.erase(it);
				return;
			}
		}
		RC_UNREACHABLE();
	}

	// Topologically sorts the basic block list.
	//
	void routine::topological_sort() {
		u32 tmp = narrow_cast<u32>(blocks.size());
		for (auto& bb : blocks) {
			if (bb->predecessors.empty() && bb.get() != entry_point) {
				bb->name = --tmp;
			}
		}

		graph::dfs(entry_point.get(), [&](basic_block* b) { b->name = --tmp; });
		RC_ASSERT(entry_point->name == 0);
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

	// Nested access wrappers.
	//
	ref<core::image>	 routine::get_image() const {
		 auto r = method.lock();
		 return r ? r->img.lock() : nullptr;
	}
	ref<core::workspace> routine::get_workspace() const {
		auto r = get_image();
		return r ? r->ws.lock() : nullptr;
	}

	// Clear all block references on destruction to prevent an error being raised.
	//
	routine::~routine() {
		for (auto& bb : blocks) {
			bb->replace_all_uses_with(std::nullopt);
		}
	}
};