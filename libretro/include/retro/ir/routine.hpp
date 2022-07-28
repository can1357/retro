#pragma once
#include <retro/format.hpp>
#include <retro/ir/insn.hpp>
#include <retro/ir/types.hpp>
#include <retro/ir/value.hpp>
#include <retro/ir/basic_block.hpp>
#include <retro/graph/search.hpp>
#include <vector>

namespace retro::ir {
	// Routine type.
	//
	struct routine : range::view_base {
		using container		= std::vector<ref<basic_block>>;
		using iterator			= typename container::iterator;
		using const_iterator = typename container::const_iterator;

		// Entry point ip if relevant.
		//
		u64 start_ip = NO_LABEL;

		// Counters.
		//
		mutable u32 next_ins_name = 0;  // Next instruction name.
		mutable u32 next_blk_name = 0;  // Next basic-block name.

		// List of basic-blocks.
		//
		container blocks = {};

		// Container observers.
		//
		iterator			begin() { return blocks.begin(); }
		iterator			end() { return blocks.end(); }
		const_iterator begin() const { return blocks.begin(); }
		const_iterator end() const { return blocks.end(); }
		size_t			size() const { return blocks.size(); }
		bool				empty() const { return blocks.empty(); }

		// Gets the entry point.
		//
		basic_block* get_entry() const { return blocks.empty() ? nullptr : blocks.front().get(); }

		// Creates or removes a block.
		//
		basic_block* add_block() {
			auto* blk = blocks.emplace_back(make_rc<basic_block>()).get();
			blk->rtn = this;
			blk->name = next_blk_name++;
			return blk;
		}
		auto del_block(basic_block* b) {
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
		void topological_sort() {
			u32 tmp = narrow_cast<u32>(blocks.size());

			basic_block* ep = get_entry();
			graph::dfs(ep, [&](basic_block* b) { b->name = --tmp; });
			RC_ASSERT(ep->name == 0);
			range::sort(blocks, [](auto& a, auto& b) { return a->name < b->name; });
		}

		// Simple renaming by order.
		//
		void rename_insns() {
			next_ins_name = 0;
			for (auto& bb : blocks) {
				for (auto i : *bb) {
					i->name = next_ins_name++;
				}
			}
		}
		void rename_blocks() {
			next_blk_name = 0;
			for (auto& bb : blocks) {
				bb->name = next_blk_name++;
			}
		}

		// Validation.
		//
		diag::lazy validate() const {
			for (auto&& blk : *this) {
				if (auto err = blk->validate()) {
					return err;
				}
			}
			return diag::ok;
		}

		// String conversion.
		//
		std::string to_string(fmt_style s = {}) const {
			std::string result;
			if (start_ip != NO_LABEL) {
				result = fmt::str(RC_ORANGE "sub_%llx " RC_RESET " [%p]", start_ip, this);
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
	};

	namespace detail {
		static u32 next_name(const routine* p) { return p->next_ins_name++; }
	};
};
