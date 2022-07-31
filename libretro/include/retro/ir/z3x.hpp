#pragma once
#include <z3++.h>
#include <retro/ir/ops.hxx>
#include <retro/ir/types.hpp>
#include <retro/ir/value.hpp>
#include <retro/ir/basic_block.hpp>
#include <retro/ir/insn.hpp>

// Z3 with IR semantics and helpers.
//
namespace retro::z3x {
	using expr	  = z3::expr;
	using sort	  = z3::sort;
	using context = z3::context;

	// Null checks.
	//
	RC_INLINE static bool ok(const expr& expr) { return bool(expr); }
	RC_INLINE static bool ok(const sort& sort) { return bool(sort); }
	RC_INLINE static bool is_null(const expr& expr) { return !bool(expr); }
	RC_INLINE static bool is_null(const sort& sort) { return !bool(sort); }

	// Given an expression and a sort, casts between them.
	//
	expr						 cast(const expr& val, const sort& into, bool sx = false);
	RC_INLINE static expr cast_sx(const expr& val, const sort& into) { return cast(val, into, true); }

	// Expression depth calculation.
	//
	size_t						expr_depth(Z3_context c, Z3_ast a);
	RC_INLINE static size_t expr_depth(const expr& of) { return expr_depth(of.ctx(), of); }

	// Symbol based variables.
	//
	std::optional<i32> get_variable(const expr& expr);
	RC_INLINE static expr make_variable(context& c, const sort& s, i32 id) { return c.constant(c.int_symbol(id), s); }
	RC_INLINE static bool is_variable(const expr& expr) { return ok(expr) && get_variable(expr).has_value(); }

	// Converts from an IR type to a Z3 sort and vice versa.
	//
	ir::type						  type_of(const sort& s);
	sort							  make_sort(context& c, ir::type ty);
	RC_INLINE static ir::type type_of(const expr& expr) { return expr ? type_of(expr.get_sort()) : ir::type::none; }

	// Converts from an IR constant to Z3 numeral and vice versa.
	//
	ir::constant value_of(const expr& expr);
	expr			 make_value(context& c, const ir::constant& v);
	expr			 make_value(context& c, const ir::constant& v, u8 lane);
	bool			 is_value(const expr& expr);

	// Applies an ir::op given the operands as Z3 expressions.
	//
	expr apply(ir::op o, expr rhs);
	expr apply(ir::op o, expr lhs, expr rhs);

	// Variable set mapping each integer symbol into an uninterpeted instruction.
	//
	struct variable_set {
		// Value list for symbol naming.
		//
		std::vector<std::pair<weak<ir::value>, expr>> vars;

		// Write list for deduplication.
		//
		std::vector<std::pair<weak<ir::insn>, expr>> write_list;

		// Wrapper using get_variable.
		//
		weak<ir::value> get(const expr& expr) const {
			if (auto i = get_variable(expr)) {
				if (*i < vars.size())
					return vars[*i].first;
			}
			return nullptr;
		}
		weak<ir::value> operator[](const expr& expr) const { return get(expr); }
	};

	// Translates ir::insn/ir::operand instances into a Z3 expression trees.
	//
	expr to_expr(variable_set& vs, context& c, ir::value* i, size_t max_depth = SIZE_MAX);
	expr to_expr(variable_set& vs, context& c, const ir::operand& o, size_t max_depth = SIZE_MAX);

	// Translates a Z3 expression tree into a series of IR instructions at the end of the block.
	//
	ir::variant to_insn(variable_set& vs, const expr& expr, ir::basic_block* bb);
};