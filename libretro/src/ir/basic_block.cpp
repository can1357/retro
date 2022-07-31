#include <retro/ir/basic_block.hpp>
#include <retro/ir/routine.hpp>

namespace retro::ir {
	// Insertion.
	//
	list::iterator<insn> basic_block::insert(list::iterator<insn> position, ref<insn> v) {
		RC_ASSERT(v->is_orphan());
		// Guessed IPs.
		if (v->ip == NO_LABEL && position->prev != end().get()) {
			v->ip = position->prev->ip;
		}
		v->block = this;
		v->name	= (rtn ? rtn->next_ins_name : orphan_next_ins_name)++;
		list::link_before(position.get(), v.get());
		return {v.release()};
	}

	// Adds or removes a jump from this basic-block to another.
	//
	void basic_block::add_jump(basic_block* to) {
		successors.emplace_back(to);
		to->predecessors.emplace_back(this);
	}
	void basic_block::del_jump(basic_block* to, bool fix_phi) {
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

	// String conversion and type getter.
	//
	std::string basic_block::to_string(fmt_style s) const {
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

	// Deref and oprhan all instructions on destruction.
	//
	basic_block::~basic_block() {
		for (auto it = begin(); it != end();) {
			auto next = std::next(it);
			it->replace_all_uses_with(std::nullopt);
			it->erase();
			it = next;
		}
	}
};