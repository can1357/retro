#pragma once
#define Z3_THROW(...) retro::fmt::abort((__VA_ARGS__).msg())
#include <retro/format.hpp>
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

	// Global context per thread.
	//
	inline static context& get_context() {
		static thread_local context ctx = {};
		return ctx;
	}

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
	u32						expr_depth(Z3_context c, Z3_ast a);
	RC_INLINE static u32 expr_depth(const expr& of) { return expr_depth(of.ctx(), of); }

	// Symbol based variables.
	//
	std::optional<i32> get_variable(const expr& expr);
	RC_INLINE static expr make_variable(context& c, const sort& s, i32 id) { return c.constant(c.int_symbol(id), s); }
	RC_INLINE static bool is_variable(const expr& expr) { return ok(expr) && get_variable(expr).has_value(); }

	// Converts from an IR type to a Z3 sort and vice versa.
	//
	ir::type						  type_of(const sort& s);
	sort							  make_sort(context& c, ir::type ty, ir::insn* i = nullptr);
	RC_INLINE static ir::type type_of(const expr& expr) { return expr ? type_of(expr.get_sort()) : ir::type::none; }

	// Converts from an IR constant to Z3 numeral and vice versa.
	//
	ir::constant value_of(const expr& expr, bool coerce = false);
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

		// Inserts a new variable or fetches it from cache.
		//
		expr emplace(context& c, weak<ir::value> v, expr* value = nullptr) {
			// Return null expression if sort is invalid.
			//
			auto ty = make_sort(c, v->get_type(), v->get_if<ir::insn>());
			if (!ok(ty)) {
				return c;
			}

			// If index is cached and valid, return.
			//
			auto i = v->get_if<ir::insn>();
			if (i) {
				u32 idx = (u32) i->tmp_mapping;
				if (vars.size() > idx && vars[idx].first == i) {
					return vars[idx].second;
				}
			}

			// Try finding the value in the list.
			//
			u32 idx = 0;
			for (; idx != vars.size(); idx++) {
				// If found, update cache index and return.
				//
				if (vars[idx].first == v) {
					if (i)
						i->tmp_mapping = idx;
					return vars[idx].second;
				}
			}

			// Create a new entry, update cache index and return.
			//
			if (i)
				i->tmp_mapping = idx;
			return vars.emplace_back(v, value ? *value : c.constant(c.int_symbol(idx), ty)).second;
		}

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

	// Translates a Z3 expression tree into a series of IR instructions before the specified position of the block.
	//
	ir::variant from_expr(variable_set& vs, const expr& expr, ir::basic_block* bb, list::iterator<ir::insn>& it);
	RC_INLINE static ir::variant from_expr(variable_set& vs, const expr& expr, ir::basic_block* bb) {
		auto it = bb->end();
		return from_expr(vs, expr, bb, it);
	}
};