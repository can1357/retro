#pragma once
#include <retro/ir/ops.hxx>
#include <retro/ir/insn.hpp>
#include <retro/ir/basic_block.hpp>

namespace retro::pattern {
	struct match_context {
		ir::variant symbols[4];
	};



	RC_INLINE static ir::insn* write_binop(ir::insn*& i, ir::op op, auto lhs, auto rhs) {
		auto r = ir::make_binop(op, std::move(lhs), std::move(rhs));
		auto it = i->block->insert_after(i, std::move(r));
		i = it.get();
		return i;
	}
	RC_INLINE static ir::insn* write_cmp(ir::insn*& i, ir::op op, auto lhs, auto rhs) {
		auto r  = ir::make_cmp(op, std::move(lhs), std::move(rhs));
		auto it = i->block->insert_after(i, std::move(r));
		i		  = it.get();
		return i;
	}
	RC_INLINE static ir::insn* write_unop(ir::insn*& i, ir::op op, auto rhs) {
		auto r  = ir::make_unop(op, std::move(rhs));
		auto it = i->block->insert_after(i, std::move(r));
		i		  = it.get();
		return i;
	}

	// Symbols.
	//
	RC_INLINE static bool match_symbol(char id, ir::value* i, match_context& ctx) {
		auto& sym = ctx.symbols[id];
		if (sym.is_null()) {
			sym = i;
			return true;
		} else {
			return !sym.is_const() && sym.get_value() == i;
		}
	}
	RC_INLINE static bool match_symbol(char id, ir::operand* o, match_context& ctx) {
		if (o->is_const()) {
			auto& sym = ctx.symbols[id];
			if (sym.is_null()) {
				sym = o->get_const();
				return true;
			} else {
				return sym.is_const() && sym.get_const().equals(o->get_const());
			}
		} else {
			return match_symbol(id, o->get_value(), ctx);
		}
		return true;
	}

	// Immediate symbols.
	//
	RC_INLINE static bool match_imm_symbol(char id, ir::insn* i, match_context& ctx) { return false; }
	RC_INLINE static bool match_imm_symbol(char id, ir::operand* o, match_context& ctx) {
		if (!o->is_const()) {
			return false;
		}

		auto& sym = ctx.symbols[id];
		if (sym.is_null()) {
			sym = o->get_const();
			return true;
		} else {
			return sym.is_const() && sym.get_const().equals(o->get_const());
		}
	}

	// Immediates.
	//
	RC_INLINE static bool match_imm(auto val, ir::insn* i, match_context& ctx) { return false; }
	RC_INLINE static bool match_imm(auto val, ir::operand* o, match_context& ctx) {
		if (!o->is_const())
			return false;
		return o->const_val.equals(ir::constant(o->const_val.get_type(), val));
	}

	// Operators.
	//
	RC_INLINE static bool match_binop(ir::op op, ir::operand** lhs, ir::operand** rhs, ir::insn* i, match_context& ctx) {
		if (i->op != ir::opcode::binop && i->op != ir::opcode::cmp)
			return false;
		if (i->opr(0).const_val.get<ir::op>() != op)
			return false;
		*lhs = &i->opr(1);
		*rhs = &i->opr(2);
		return true;
	}
	RC_INLINE static bool match_binop(ir::op op, ir::operand** lhs, ir::operand** rhs, ir::operand* o, match_context& ctx) {
		if (o->is_const())
			return false;
		auto* i = o->get_value()->get_if<ir::insn>();
		if (!i)
			return false;
		return match_binop(op, lhs, rhs, i, ctx);
	}
	RC_INLINE static bool match_unop(ir::op op, ir::operand** rhs, ir::insn* i, match_context& ctx) {
		if (i->op != ir::opcode::unop)
			return false;
		if (i->opr(0).const_val.get<ir::op>() != op)
			return false;
		*rhs = &i->opr(1);
		return true;
	}
	RC_INLINE static bool match_unop(ir::op op, ir::operand** rhs, ir::operand* o, match_context& ctx) {
		if (o->is_const())
			return false;
		auto* i = o->get_value()->get_if<ir::insn>();
		if (!i)
			return false;
		return match_unop(op, rhs, i, ctx);
	}
};

namespace retro::directives {
	using fn_match_t = bool(*)(ir::insn*i, pattern::match_context& ctx);

	// [[replace]]
	inline std::vector<fn_match_t> replace_list = {};
};

#define RC_DEF_DIR_REPLACE(SRC, DST) RC_INITIALIZER { replace_list.emplace_back(&SRC::match); }