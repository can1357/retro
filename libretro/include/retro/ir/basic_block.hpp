#pragma once
#include <retro/dyn.hpp>
#include <retro/list.hpp>
#include <retro/format.hpp>
#include <retro/ir/types.hpp>
#include <retro/ir/value.hpp>
#include <retro/ir/insn.hpp>
#include <retro/graph/naive.hpp>

namespace retro::ir {
	// Basic block type.
	//
	struct routine;
	struct basic_block final : dyn<basic_block, value>, list::head<insn> {
		// Owning routine.
		//
		routine* rtn = nullptr;

		// Name and IP ranges.
		//
		u32 name = 0;
		u64 ip	  = NO_LABEL;
		u64 end_ip = NO_LABEL;

		// Archtecture.
		//
		arch::handle arch = {};

		// Successor and predecesor list.
		//
		std::vector<weak<basic_block>> successors	  = {};
		std::vector<weak<basic_block>> predecessors = {};

		// Container observers.
		//
		list::iterator<insn> end_phi() const {
			return range::find_if(*this, [](insn* i) { return i->op != opcode::phi; });
		}
		auto phis() const { return range::subrange(begin(), end_phi()); }
		auto insns() const { return range::subrange(begin(), end()); }
		insn* terminator() const {
			if (auto r = back()) {
				if (auto& desc = r->desc(); desc.bb_terminator || desc.terminator)
					return r;
			}
			return nullptr;
		}

		// Insertion.
		//
		list::iterator<insn> insert(list::iterator<insn> position, ref<insn> v);
		list::iterator<insn> insert_after(list::iterator<insn> position, ref<insn> v) { return insert(++position, v); }
		list::iterator<insn> push_front(ref<insn> v) { return insert(begin(), std::move(v)); }
		list::iterator<insn> push_back(ref<insn> v) { return insert(end(), std::move(v)); }

		// Erasing.
		//
		list::iterator<insn> erase(list::iterator<insn> it) {
			auto next = it->next;
			it->erase();
			return next;
		}
		list::iterator<insn> erase(ref<insn> it) {
			auto r = erase(list::iterator<insn>(it.get()));
			it.reset();
			return r;
		}
		template<typename F>
		size_t erase_if(F&& f) {
			size_t n = 0;
			for (auto it = begin(); it != end();) {
				if (f(it.get())) {
					n++;
					it = erase(it);
				} else {
					++it;
				}
			}
			return n;
		}
		template<typename F>
		size_t rerase_if(F&& f) {
			size_t n = 0;
			if (!empty()) {
				auto it = std::prev(end());
				while (it != end()) {
					if (f(it)) {
						n++;
						std::exchange(it, it->prev)->erase();
					} else {
						--it;
					}
				}
			}
			return n;
		}

		// Adds or removes a jump from this basic-block to another.
		//
		void add_jump(basic_block* to);
		void del_jump(basic_block* to, bool fix_phi = true);

		// Splits the basic block at the specified instruction boundary.
		//
		basic_block* split(list::iterator<insn> boundary);

		// Validation.
		//
		diag::lazy validate() const;

		// Nested access wrappers.
		//
		ref<core::method>		get_method() const;
		ref<core::image>		get_image() const;
		ref<core::workspace> get_workspace() const;

		// String conversion and type getter.
		//
		std::string to_string(fmt_style s = {}) const override;
		type get_type() const override { return type::label; }

		// Deref and oprhan all instructions on destruction.
		//
		~basic_block();

		// Create the auto-generated constructors.
		//
#define ADD_CTOR(a, oprhan, bb) bb
		RC_VISIT_IR_OPCODE(ADD_CTOR)
#undef ADD_CTOR
	};
};
