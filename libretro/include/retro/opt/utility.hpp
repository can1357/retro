#pragma once
#include <retro/common.hpp>
#include <retro/ir/basic_block.hpp>
#include <retro/ir/insn.hpp>
#include <retro/ir/routine.hpp>
#include <retro/ir/value.hpp>

// Utility functions.
//
namespace retro::ir::opt::util {
	// Checks IR values for equality.
	//
	struct identity_check_record {
		identity_check_record* prev = nullptr;
		const value*			  a;
		const value*			  b;
	};
	inline bool is_identical(const operand& a, const operand& b, identity_check_record* prev = nullptr);
	inline bool is_identical(const value* a, const value* b, identity_check_record* prev = nullptr) {
		// If same value, assume equal.
		// If different types, assume non-equal.
		//
		if (a == b)
			return true;
		if (a->get_type() != b->get_type())
			return false;

		// If in the equality check stack already, assume equal. (?)
		//
		for (auto it = prev; it; it = it->prev)
			if (it->a == a && it->b == b)
				return true;

		// If both are instructions:
		//
		auto ai = a->get_if<insn>();
		auto bi = b->get_if<insn>();
		if (ai && bi && ai->op == bi->op && ai->template_types == bi->template_types) {
			// If there are side effects or instruction is not pure, assume false.
			//
			auto& desc = ai->desc();
			if (desc.side_effect || !desc.is_pure)
				return false;

			// If non-constant instruction:
			//
			if (desc.is_const) {
				// TODO: Trace until common dominator to ensure no side effects.
			}

			// If opcode is the same and operand size matches:
			//
			if (ai->op == bi->op && ai->operand_count == bi->operand_count) {
				// Make a new record to fix infinite-loops with PHIs.
				//
				identity_check_record rec{prev, a, b};

				// If all values are equal, pass the check.
				//
				for (size_t i = 0; i != ai->operand_count; i++)
					if (!is_identical(ai->opr(i), bi->opr(i), &rec))
						return false;
				return true;
			}
		}
		return false;
	}
	inline bool is_identical(const operand& a, const operand& b, identity_check_record* prev) {
		if (a.is_const()) {
			return b.is_const() && a.get_const().equals(b.get_const());
		} else {
			return !b.is_const() && is_identical(a.get_value(), b.get_value(), prev);
		}
	}

	// Simple local DCE pass.
	//
	static size_t local_dce(basic_block* bb) {
		return bb->rerase_if([](insn* i) { return !i->uses() && !i->desc().side_effect; });
	}

	// Runs validation and fast local dead code elimination after a pass is done.
	//
	static size_t complete(basic_block* bb, size_t n) {
		// If this pass changed the block:
		//
		if (n) {
#if RC_DEBUG
			bb->validate().raise();
#endif
			n += local_dce(bb);
		}
		return n;
	}
	static size_t complete(routine* rtn, size_t n) {
		// If this pass changed the routine:
		//
		if (n) {
#if RC_DEBUG
			rtn->validate().raise();
#endif
			for (auto& bb : rtn->blocks)
				n += local_dce(bb);
		}
		return n;
	}
};