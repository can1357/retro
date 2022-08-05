#pragma once
#include <retro/format.hpp>
#include <retro/ir/insn.hpp>
#include <retro/ir/types.hpp>
#include <retro/ir/value.hpp>
#include <retro/ir/basic_block.hpp>
#include <retro/graph/search.hpp>
#include <vector>

namespace retro::core { struct method; };

namespace retro::ir {
	// Routine type.
	//
	struct routine : range::view_base {
		using container		= std::vector<ref<basic_block>>;
		using iterator			= typename container::iterator;
		using const_iterator = typename container::const_iterator;

		// Owning method.
		//
		weak<core::method> method = {};

		// Entry point ip if relevant.
		//
		u64 ip = NO_LABEL;

		// Counters.
		//
		mutable u32 next_ins_name = 0;  // Next instruction name.
		mutable u32 next_blk_name = 0;  // Next basic-block name.

		// Last control flow modification time.
		//
		mutable u64 last_cfg_modify_timer = 0;

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
		basic_block* add_block();
		auto del_block(basic_block* b);

		// Topologically sorts the basic block list.
		//
		void topological_sort();

		// Simple renaming by order.
		//
		void rename_insns();
		void rename_blocks();

		// Marks the cfg dirty.
		//
		void dirty_cfg() const { last_cfg_modify_timer = graph::monotonic_counter(); }

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
		std::string to_string(fmt_style s = {}) const;

		// Nested access wrappers.
		//
		ref<core::image>		 get_image() const;
		ref<core::workspace> get_workspace() const;

		// Clones the routine.
		//
		ref<routine> clone() const;

		// Clear all block references on destruction to prevent an error being raised.
		//
		~routine();
	};
};
