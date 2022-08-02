#include <retro/opt/interface.hpp>
#include <retro/opt/utility.hpp>

namespace retro::ir::opt {
	// Local identical value folding.
	//
	size_t id_fold(basic_block* bb) {
		size_t n = 0;

		// For each instruction:
		//
		for (insn* ins : view::reverse(bb->insns())) {
			// For each instruction before it:
			//
			for (insn* ins2 : bb->rslice(ins)) {
				// If identical, replace.
				//
				if (util::is_identical(ins, ins2)) {
					n += 1 + ins->replace_all_uses_with(ins2);
					break;
				}

				// If instruction has side effects and the target is not const, fail.
				//
				if (ins2->desc().side_effect && !ins->desc().is_const) {
					break;
				}
			}
		}
		return util::complete(bb, n);
	}
};
