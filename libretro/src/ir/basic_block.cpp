#include <retro/ir/basic_block.hpp>
#include <retro/ir/routine.hpp>
#include <retro/core/method.hpp>
#include <retro/core/image.hpp>

namespace retro::ir {
	RC_DEF_ERR(insn_ref_invalid, "instruction references value declared after itself: %")
	RC_DEF_ERR(insn_phi_order,   "phi instruction after non-phi instruction: %")
	RC_DEF_ERR(insn_after_term,  "instruction after terminator: %")

	// Insertion.
	//
	list::iterator<insn> basic_block::insert(list::iterator<insn> position, ref<insn> v) {
		RC_ASSERT(v->is_orphan());
		// Guessed IPs.
		if (position->prev != end().get()) {
			if (v->ip == NO_LABEL)
				v->ip = position->prev->ip;
		}
		if (!v->arch)
			v->arch = arch;

		v->bb = this;
		v->name	= rtn->next_ins_name++;
		list::link_before(position.get(), v.get());
		return {v.release()};
	}

	// Adds or removes a jump from this basic-block to another.
	//
	void basic_block::add_jump(basic_block* to) {
		rtn->dirty_cfg();
		successors.emplace_back(to);
		to->predecessors.emplace_back(this);
	}
	void basic_block::del_jump(basic_block* to, bool fix_phi) {
		rtn->dirty_cfg();
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

	// Splits the basic block at the specified instruction boundary.
	//
	basic_block* basic_block::split(list::iterator<insn> boundary) {
		// Filter out stupid calls.
		//
		if (boundary == begin()) {
			return nullptr;
		}
		auto* blk = rtn->add_block();
		blk->end_ip = this->end_ip;
		blk->arch	= this->arch;
		if (boundary == end()) {
			blk->ip = this->end_ip;
			return blk;
		}

		// Update the tracked IP ranges.
		//
		blk->ip		 = boundary->ip;
		this->end_ip = boundary->ip;

		// Manually split the list.
		//
		auto this_head	 = entry();
		auto this_front = front();
		auto this_back	 = boundary->prev;

		auto new_head	 = blk->entry();
		auto new_front	 = boundary.get();
		auto new_back	 = back();

		this_head->next = this_front;
		this_head->prev = this_back;
		this_front->prev = this_head;
		this_back->next  = this_head;

		new_head->next  = new_front;
		new_head->prev  = new_back;
		new_front->prev = new_head;
		new_back->next  = new_head;

		// Fix the parent pointers.
		//
		for (auto ins : blk->insns()) {
			ins->bb = blk;
		}

		// Fixup the successor / predecessor list.
		//
		for (auto& suc : successors) {
			for (auto& pred : suc->predecessors) {
				if (pred == this) {
					pred = blk;
					break;
				}
			}
		}
		successors.swap(blk->successors);
		return blk;
	}
	
	// Validation.
	//
	diag::lazy basic_block::validate() const {

		bool terminated = false;
		bool got_non_phi = false;
		for (auto&& ins : insns()) {
			// Order validation:
			//
			if (ins->op != opcode::phi) {
				got_non_phi = true;
			} else if (got_non_phi) {
				return err::insn_phi_order(ins->to_string());
			}
			if (terminated) {
				return err::insn_after_term(ins->to_string());
			} else if (ins->desc().terminator) {
				terminated = true;
			}

			// Instruction specific validation:
			//
			if (auto err = ins->validate()) {
				return err;
			}

			// Operand checks.
			//
			for (auto& op : ins->operands()) {
				if (op.is_value()) {
					if (auto* iref = op.get_value()->get_if<insn>()) {
						if (iref->bb == this) {
							if (ins->op != opcode::phi) {
								for (auto&& ins2 : slice(ins->next)) {
									if (ins2 == iref) {
										return err::insn_ref_invalid(ins->to_string());
									}
								}
							}
						} else {
							RC_ASSERT(!iref->is_orphan());
							// TODO: DOM check, specialize for PHI.
						}
					}
				}
			}
		}
		return diag::ok;
	}

	// Nested access wrappers.
	//
	ref<core::method> basic_block::get_method() const {
		return rtn->method.lock();
	}
	ref<core::image>	 basic_block::get_image() const {
		 auto r = get_method();
		 return r ? r->img.lock() : nullptr;
	}
	ref<core::workspace> basic_block::get_workspace() const {
		auto r = get_image();
		return r ? r->ws.lock() : nullptr;
	}

	// String conversion and type getter.
	//
	std::string basic_block::to_string(fmt_style s) const {
		if (s == fmt_style::concise) {
			return fmt::str(RC_CYAN "$%x" RC_RESET, name);
		} else {
			std::string result	  = fmt::str(RC_CYAN "$%x:" RC_RESET, name);
			if (ip != NO_LABEL && end_ip != NO_LABEL) {
				result += fmt::str(RC_GRAY " [%llx => %llx]" RC_RESET, ip, end_ip);
			} else if (end_ip != NO_LABEL) {
				result += fmt::str(RC_GRAY " [... => %llx]" RC_RESET, end_ip);
			} else if (ip != NO_LABEL) {
				result += fmt::str(RC_GRAY " [%llx => ...]" RC_RESET, ip);
			}

			if (predecessors.empty()) {
				result += " {" RC_GREEN "Entry" RC_RESET "}";
			} else {
				result += " {";
				for (auto& p : predecessors) {
					result += p->to_string(fmt_style::concise);
					result += ",";
				}
				result.pop_back();
				result += RC_RESET "}";
			}

			auto			last_label = NO_LABEL;
			for (insn* i : *this) {
				auto str = i->to_string();
				if (i->ip != NO_LABEL && i->ip != last_label) {
					last_label = i->ip;
					fmt::ljust(str, 64);
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