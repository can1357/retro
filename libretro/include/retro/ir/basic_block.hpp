#pragma once
#include <retro/dyn.hpp>
#include <retro/list.hpp>
#include <retro/format.hpp>
#include <retro/ir/types.hpp>
#include <retro/ir/value.hpp>
#include <retro/ir/insn.hpp>

namespace retro::ir {
	// Basic block type.
	//
	struct procedure;
	namespace detail {
		static u32 next_name(const procedure*);
	};

	struct basic_block final : dyn<basic_block, value>, list::head<insn> {
		// Owning procedure.
		//
		procedure* proc = nullptr;

		// Name and Label IP.
		//
		u32 name = 0;
		u64 ip	= NO_LABEL;

		// Temporary for algorithms.
		//
		mutable u64 tmp_monotonic = 0;
		mutable u64 tmp_mapping	  = 0;

		// Successor and predecesor list.
		//
		std::vector<weak<basic_block>> successors	  = {};
		std::vector<weak<basic_block>> predecessors = {};

		// Container observers.
		//
		list::iterator<insn> end_phi() const {
			return range::find_if(begin(), end(), [](insn* i) { return i->op != opcode::phi; });
		}
		auto phis() const { return range::subrange(begin(), end_phi()); }
		auto insns() const { return range::subrange(begin(), end()); }

		// Insertion.
		//
		list::iterator<insn> insert(list::iterator<insn> position, ref<insn> v) {
			RC_ASSERT(v->is_orphan());
			v->block = this;
			v->name	= detail::next_name(proc);
			list::link_before(position.at, v.get());
			return {v.release()};
		}
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
				if (f(it.at)) {
					n++;
					it = erase(it);
				} else {
					++it;
				}
			}
			return n;
		}

		// Adds or removes a jump from this basic-block to another.
		//
		void add_jump(basic_block* to) {
			auto sit = range::find(successors, to);
			auto pit = range::find(to->predecessors, this);
			successors.emplace_back(to);
			to->predecessors.emplace_back(this);
		}
		void del_jump(basic_block* to, bool fix_phi = true) {
			auto sit = range::find(successors, to);
			auto pit = range::find(to->predecessors, this);
			if (fix_phi) {
				size_t n = pit - to->predecessors.begin();
				for (auto phi : to->phis())
					phi->erase_operand(n);
			}
			successors.erase(sit);
			to->predecessors.erase(pit);
		}

		// Validation.
		//
		diag::lazy validate() const {
			for (auto&& ins : insns()) {
				if (auto err = ins->validate()) {
					return err;
				}
			}
			return diag::ok;
		}

		// Declare string conversion and type getter.
		//
		std::string to_string(fmt_style s = {}) const override {
			if (s == fmt_style::concise) {
				return fmt::str(RC_CYAN "$%x" RC_RESET, name);
			} else {
				std::string result	  = fmt::str(RC_CYAN "$%x:" RC_RESET, name);
				auto			last_label = NO_LABEL;
				for (insn* i : *this) {
					auto str = i->to_string();
					if (i->ip != NO_LABEL && i->ip != last_label) {
						last_label = i->ip;
						fmt::ljust(str, 36);
						str += fmt::str(RC_PURPLE " ; %p", i->ip);
					}
					result += "\n\t" + str;
				}
				result += RC_RESET;
				return result;
			}
		}
		type get_type() const override { return type::label; }

		// Deref and oprhan all instructions on destruction.
		//
		~basic_block() {
			for (auto it = begin(); it != end();) {
				auto next = std::next(it);
				it->replace_all_uses_with(std::nullopt);
				it->erase();
				it = next;
			}
		}

		// Create the auto-generated constructors.
		//
#define ADD_CTOR(a, oprhan, bb) bb
		RC_VISIT_OPCODE(ADD_CTOR)
#undef ADD_CTOR
	};
};
