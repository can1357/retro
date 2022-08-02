#include <retro/opt/interface.hpp>
#include <retro/opt/utility.hpp>
#include <retro/robin_hood.hpp>

namespace retro::ir::opt::p0 {

	// The following algorithm is a modified version adapted from the paper:
	// Braun, M., Buchwald, S., Hack, S., Leißa, R., Mallon, C., Zwinkau, A. (2013). Simple and Efficient Construction of Static Single Assignment Form.
	// In: Jhala, R., De Bosschere, K. (eds) Compiler Construction. CC 2013. Lecture Notes in Computer Science, vol 7791. Springer, Berlin, Heidelberg.
	//
	static variant read_variable_local(arch::mreg r, basic_block* b, insn* before, bool* fail) {
		for (insn* ins : b->rslice(before)) {
			// Write reg:
			//
			if (ins->op == opcode::write_reg && ins->opr(0).get_const().get<arch::mreg>() == r) {
				return variant{ins->opr(1)};
			}
			// Possible implicit write:
			//
			else if (ins->desc().unk_reg_use) {
				if (fail) {
					*fail = true;
					return {};
				}
				auto ty = enum_reflect(r.get_kind()).type;  // TODO: Might be unknown type.
				return b->insert_after(ins, make_read_reg(ty, r)).get();
			}
		}
		return {};
	}
	static insn* reread_variable_local(arch::mreg r, basic_block* b, insn* until) {
		for (insn* ins : b->rslice(until)) {
			// Read reg:
			//
			if (ins->op == opcode::read_reg && ins->opr(0).get_const().get<arch::mreg>() == r) {
				return ins;
			}
		}
		return nullptr;
	}
	static variant read_variable_recursive(arch::mreg r, basic_block* b, insn* until);
	static variant read_variable(arch::mreg r, basic_block* b, insn* until) {
		bool fail = false;
		if (auto i = read_variable_local(r, b, until, &fail)) {
			return i;
		} else if (!fail) {
			return read_variable_recursive(r, b, until);
		} else {
			return {};
		}
	}
	static variant try_remove_trivial_phi(ref<insn> phi) {
		const operand* same = nullptr;
		for (auto& op : phi->operands()) {
			if (!op.is_const() && op.get_value() == phi) {
				continue;
			}
			if (same) {
				if (op != *same) {
					return phi.get();
				}
			} else {
				same = &op;
			}
		}
		RC_ASSERT(same);
		variant same_v{*same};

		std::vector<ref<insn>> phi_users;
		for (auto u : phi->uses()) {
			if (auto& i = u->user->get<insn>(); i.op == opcode::phi) {
				phi_users.emplace_back(&i);
			}
		}
		phi->replace_all_uses_with(same_v);
		phi->erase();

		for (auto& p : phi_users)
			if (!p->is_orphan())
				try_remove_trivial_phi(std::move(p));
		return same_v;
	}

	static variant read_variable_recursive(arch::mreg r, basic_block* b, insn* until) {
		auto ty = enum_reflect(r.get_kind()).type;  // TODO: Might be unknown type.

		// Actually load the value if it does not exist.
		//
		if (b->predecessors.empty()) {
			auto v = read_variable_local(r, b, until, nullptr);
			if (!v) {
				auto i = reread_variable_local(r, b, until);
				if (!i) {
					i = b->insert(b->begin(), make_read_reg(ty, r)).get();
				}
				v = i;
			}
			return v;
		}
		// No PHI needed.
		//
		else if (b->predecessors.size() == 1) {
			b = b->predecessors.front().get();
			return read_variable(r, b, b->end());
		}
		// Create a PHI recursively.
		//
		else {
			// Create an empty phi.
			//
			auto phi = insn::allocate(opcode::phi, {ty}, b->predecessors.size());  // TODO :(
			b->insert(b->begin(), phi);

			// Create a temporary store to break cycles.
			//
			auto tmp = b->insert(b->end_phi(), make_write_reg(r, phi));

			// For each predecessor, append a PHI node.
			//
			for (size_t n = 0; n != b->predecessors.size(); n++) {
				auto bb		= b->predecessors[n].get();
				auto v		= read_variable(r, bb, bb->end());
				phi->opr(n) = v;
			}

			// Delete the temporary.
			//
			tmp->erase();
			return try_remove_trivial_phi(std::move(phi));
		}
	}

	// Converts register read/write into PHIs.
	//
	size_t reg_to_phi(routine* rtn) {
		rtn->topological_sort();

		// Generate PHIs to replace read_reg in every block except the entry point.
		//
		size_t n = 0;
		for (auto& bb : view::reverse(rtn->blocks)) {
			n += bb->erase_if([&](insn* ins) {
				if (ins->op == opcode::read_reg) {
					auto r = ins->opr(0).const_val.get<arch::mreg>();
					auto v = read_variable(r, bb.get(), ins);
					if (!v.is_null()) {
						if (v.is_const() || v.get_value() != ins) {
							if (v.get_type() != ins->get_type()) {
								v = bb->insert(ins, make_bitcast(ins->get_type(), std::move(v))).get();
							}
							ins->replace_all_uses_with(std::move(v));
							return true;
						}
					}
				}
				return false;
			});
		}

		// Break out if theres instructions with unknown register use.
		//
		for (auto& bb : view::reverse(rtn->blocks)) {
			for (auto* ins : bb->insns()) {
				if (ins->desc().unk_reg_use)
					return false;
			}
		}

		// Otherwise, remove all write_regs.
		//
		for (auto& bb : view::reverse(rtn->blocks)) {
			n += bb->erase_if([](auto i) { return i->op == opcode::write_reg; });
		}
		return util::complete(rtn, n);
	}
};
