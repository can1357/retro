#include <retro/opt/interface.hpp>
#include <retro/opt/utility.hpp>
#include <retro/robin_hood.hpp>

namespace retro::ir::opt::init {
	// The following algorithm is a modified version adapted from the paper:
	// - Simple and Efficient Construction of Static Single Assignment Form (2013) Braun, M., et al.
	//
	struct block_cache {
		flat_umap<u32, weak<insn>> reg_cache;
		static auto& get(basic_block* b) { return ((block_cache*)b->tmp_mapping)->reg_cache; }
	};

	static insn* strip_launders(insn* i) {
		while (i->op == opcode::bitcast && i->template_types[0] == i->template_types[1]) {
			if (i->opr(0).is_value()) {
				if (auto i2 = i->opr(0).get_value()->get_if<ir::insn>(); i2 && i2 != i) {
					i = i2;
				} else {
					break;
				}
			} else {
				break;
			}
		}
		return i;
	}
	static bool compare_nolaunder(insn* a, insn* b) {
		if (a == b)
			return true;
		else
			return a->op == opcode::bitcast && b->op == opcode::bitcast && a->opr(0) == b->opr(0);
	}

	static insn* read_variable_local(arch::mreg r, basic_block* b, insn* before, bool* fail) {
		auto& c = block_cache::get(b);

		if (before == b->end().get()) {
			auto it = c.find(r.uid());
			if (it != c.end()) {
				if (!it->second.expired()) {
					return it->second.get();
				}
			}
		}
		auto ret_w_cache = [&](insn* val) RC_INLINE -> insn* {
			if (before == b->end().get()) {
				c[r.uid()] = val;
			}
			return val;
		};

		for (insn* ins : b->rslice(before)) {
			// Write reg:
			//
			if (ins->op == opcode::write_reg && ins->opr(0).get_const().get<arch::mreg>() == r) {
				auto& val = ins->opr(1);
				if (val.is_value() && val.get_value()->is<ir::insn>()) {
					return ret_w_cache((ir::insn*) val.get_value());
				}
				// Very dumb cast to launder the value.
				return ret_w_cache(b->insert(ins, ir::make_bitcast(val.get_type(), val)));
			}
			// Possible implicit write:
			//
			else if (ins->desc().unk_reg_use) {
				if (fail) {
					*fail = true;
					return {};
				}
				auto ty = enum_reflect(r.get_kind()).type;  // TODO: Might be unknown type.
				return ret_w_cache(b->insert_after(ins, make_read_reg(ty, r)).get());
			}
		}
		return ret_w_cache(nullptr);
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
	static insn* read_variable_recursive(arch::mreg r, basic_block* b, insn* until);
	static insn* read_variable(arch::mreg r, basic_block* b, insn* until) {
		bool fail = false;
		if (auto i = read_variable_local(r, b, until, &fail)) {
			return i;
		} else if (!fail) {
			return read_variable_recursive(r, b, until);
		} else {
			return nullptr;
		}
	}
	static void try_remove_trivial_phi(insn* phi) {
		ref<insn> same = nullptr;
		for (auto& opv : phi->operands()) {
			RC_ASSERT(opv.is_value() && opv.get_value()->is<ir::insn>());
			insn* op = (insn*) opv.get_value();
			op			= strip_launders(op);

			if (compare_nolaunder(op, phi)) {
				continue;
			}
			if (same) {
				if (!compare_nolaunder(op, same))
					return;
			} else {
				same = op;
			}
		}
		// The phi is unreachable or in the start block.
		//
		if (!same)
			same = phi->bb->insert_after(phi->bb->end_phi(), ir::make_poison(phi->get_type(), "unreachable or entry point phi")).get();

		std::vector<ref<insn>> phi_users;
		for (auto u : phi->uses()) {
			if (auto& i = u->user->get<insn>(); i.op == opcode::phi && &i != phi) {
				phi_users.emplace_back(&i);
			}
		}
		phi->replace_all_uses_with(same);
		phi->erase();

		for (auto& p : phi_users) {
			if (!p->is_orphan())
				try_remove_trivial_phi(p);
		}
	}

	static insn* read_variable_recursive(arch::mreg r, basic_block* b, insn* until) {
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
			auto phi = b->insert(b->begin(), insn::allocate(opcode::phi, {ty}, b->predecessors.size())).get();

			// Create a temporary cache entry to break cycles.
			//
			auto& cache = block_cache::get(b);
			auto	cast		= ir::make_bitcast(ty, phi);
			if (until == b->end().get())
				cache[r.uid()] = cast.get();

			// For each predecessor, append a PHI node.
			//
			for (size_t n = 0; n != b->predecessors.size(); n++) {
				auto bb		= b->predecessors[n].get();
				phi->opr(n) = read_variable(r, bb, bb->end());
			}

			// Remove trivial PHIs.
			//
			try_remove_trivial_phi(phi);

			// Delete the temporary, update the cache.
			//
			RC_ASSERT(cast->opr(0).is_value() && cast->opr(0).get_value()->template is<ir::insn>());
			auto replace_by = (ir::insn*) cast->opr(0).get_value();
			cast->replace_all_uses_with(replace_by);
			if (until == b->end().get())
				cache[r.uid()] = replace_by;
			return replace_by;
		}
	}

	// Converts register read/write into PHIs.
	//
	size_t reg_to_phi(routine* rtn) {
		rtn->topological_sort();

		// If entry point has predecessors, we have to insert a pure entry point.
		//
		if (!rtn->get_entry()->predecessors.empty()) {
			ref entry = rtn->get_entry();
			auto* blk = rtn->blocks.emplace(rtn->blocks.begin(), make_rc<basic_block>())->get();
			blk->rtn	 = rtn;
			blk->name = 0;
			for (size_t i = 1; i != rtn->blocks.size(); i++) {
				rtn->blocks[i]->name++;
			}
			blk->add_jump(entry);
			blk->push_jmp(entry.get());
		}

		// Create block caches.
		//
		for (auto& bb : rtn->blocks) {
			bb->tmp_mapping = (u64) new block_cache();
		}

		// Generate PHIs to replace read_reg in every block.
		//
		size_t n = 0;
		for (auto& bb : rtn->blocks) {
			n += bb->erase_if([&](insn* ins) {
				if (ins->op == opcode::read_reg) {
					auto r = ins->opr(0).const_val.get<arch::mreg>();
					auto v = read_variable(r, bb.get(), ins);
					if (v) {
						if (v->get_type() != ins->get_type()) {
							v = bb->insert(ins, make_bitcast(ins->get_type(), v)).get();
						}
						ins->replace_all_uses_with(v);
						return true;
					}
				}
				return false;
			});
		}

		// Delete the block caches, remove the stupid bitcasts we inserted.
		//
		for (auto& bb : rtn->blocks) {
			bb->rerase_if([](insn* i) {
				if (i->op == opcode::bitcast && i->template_types[0] == i->template_types[1]) {
					i->replace_all_uses_with(i->opr(0));
					return true;
				}
				return false;
			});
			delete (block_cache*) std::exchange(bb->tmp_mapping, 0);
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
