#pragma once
#include <retro/common.hpp>
#include <retro/ir/basic_block.hpp>
#include <retro/ir/routine.hpp>

namespace retro::ir::opt {
	// IRP_INIT optimizers.
	//
	namespace init {
		// Local register move propagation.
		//
		size_t reg_move_prop(basic_block* bb);

		// Converts register read/write into PHIs.
		//
		size_t reg_to_phi(routine* rtn);
	};

	// Local constant folding.
	//
	size_t const_fold(basic_block* bb);

	// Local identical value folding.
	//
	size_t id_fold(basic_block* bb);

	// Local instruction combination.
	//
	size_t ins_combine(basic_block* bb);

	// Conversion of load_mem with constant address.
	//
	size_t const_load(basic_block* bb);
};