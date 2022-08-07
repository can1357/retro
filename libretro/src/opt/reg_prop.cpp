#include <retro/opt/interface.hpp>
#include <retro/opt/utility.hpp>
#include <retro/robin_hood.hpp>

namespace retro::ir::opt::init {
	// Local register move propagation.
	//
	size_t reg_move_prop(basic_block* bb) {
		size_t n = 0;

		// For each instruction:
		//
		flat_umap<u32, std::pair<ref<insn>, bool>> producer_map = {};
		for (auto* ins : bb->insns()) {
			// If producing a value:
			if (ins->op == opcode::write_reg) {
				auto r = ins->opr(0).get_const().get<arch::mreg>();

				// If there's another producer, remove it from the stream, declare ourselves as the last one.
				auto& [lp, lpw] = producer_map[r.uid()];
				if (lp && lpw) {
					n++;
					lp->erase();
				}
				lpw = true;
				lp	 = ins;
			}
			// If requires a value:
			else if (ins->op == opcode::read_reg) {
				auto r = ins->opr(0).get_const().get<arch::mreg>();

				// If there's a cached producer of this value:
				auto& [lp, lpw] = producer_map[r.uid()];
				if (lp) {
					variant lpv;
					if (lpw) {
						lpv = variant{lp->opr(1)};
					} else {
						lpv = weak<value>{lp};
					}

					// If the type matches:
					if (auto ty = lpv.get_type(); ty == ins->template_types[0]) {
						// Replace uses with the assigned value.
						n += 1 + ins->replace_all_uses_with(lpv);
						ins->op = opcode::nop;
					}
					// Otherwise:
					else {
						// Insert a bitcast right before the read.
						auto bc = bb->insert(ins, make_bitcast(ins->template_types[0], lpv));
						// Replace uses with the result value.
						n += 1 + ins->replace_all_uses_with(bc.get());
					}
				}
				// Otherwise, declare ourselves as the producer to deduplicate.
				else {
					lp	 = ins;
					lpw = false;
				}
			}
			// Clear current state if we hit an instruction with unknown register use.
			//
			else if (ins->desc().unk_reg_use) {
				producer_map.clear();
			}
		}
		return util::complete(bb, n);
	}
};
