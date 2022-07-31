#include <retro/ir/z3x.hpp>

namespace retro::z3x {
	// Implement all operators with our semantics over bool.
	//
	RC_INLINE static expr abs(const expr& lhs) {
		if (lhs.is_bv()) {
			return ite(lhs < 0, -lhs, lhs);
		} else if (lhs.is_fpa()) {
			return {lhs.ctx(), Z3_mk_fpa_abs(lhs.ctx(), lhs)};
		} else {
			return lhs.ctx().bool_val(0);
		}
	}
	RC_INLINE static expr neg(const expr& lhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvneg(lhs.ctx(), lhs)};
		} else if (lhs.is_fpa()) {
			return {lhs.ctx(), Z3_mk_fpa_neg(lhs.ctx(), lhs)};
		} else {
			return lhs.ctx().bool_val(1);
		}
	}
	RC_INLINE static expr bit_not(const expr& lhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvnot(lhs.ctx(), lhs)};
		} else {
			return {lhs.ctx(), Z3_mk_not(lhs.ctx(), lhs)};
		}
	}
	RC_INLINE static expr rol(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_ext_rotate_left(lhs.ctx(), lhs, rhs)};
		} else {
			return lhs;	 // bool
		}
	}
	RC_INLINE static expr ror(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_ext_rotate_right(lhs.ctx(), lhs, rhs)};
		} else {
			return lhs;	 // bool
		}
	}
	RC_INLINE static expr rem(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvsrem(lhs.ctx(), lhs, rhs)};
		} else if (lhs.is_fpa()) {
			return {lhs.ctx(), Z3_mk_fpa_rem(lhs.ctx(), lhs, rhs)};
		} else {
			return lhs.ctx().bool_val(false);
		}
	}
	RC_INLINE static expr urem(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvurem(lhs.ctx(), lhs, rhs)};
		} else if (lhs.is_fpa()) {
			return {lhs.ctx(), Z3_mk_fpa_rem(lhs.ctx(), lhs, rhs)};
		} else {
			return lhs.ctx().bool_val(false);
		}
	}
	RC_INLINE static expr div(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvsdiv(lhs.ctx(), lhs, rhs)};
		} else if (lhs.is_fpa()) {
			return {lhs.ctx(), Z3_mk_fpa_div(lhs.ctx(), lhs.ctx().fpa_rounding_mode(), lhs, rhs)};
		} else {
			return lhs; /*since rhs == 1*/
		}
	}
	RC_INLINE static expr udiv(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvudiv(lhs.ctx(), lhs, rhs)};
		} else if (lhs.is_fpa()) {
			return {lhs.ctx(), Z3_mk_fpa_div(lhs.ctx(), lhs.ctx().fpa_rounding_mode(), lhs, rhs)};
		} else {
			return lhs; /*since rhs == 1*/
		}
	}
	RC_INLINE static expr bit_xor(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvxor(lhs.ctx(), lhs, rhs)};
		} else {
			return {lhs.ctx(), Z3_mk_xor(lhs.ctx(), lhs, rhs)};
		}
	}
	RC_INLINE static expr bit_or(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvor(lhs.ctx(), lhs, rhs)};
		} else {
			Z3_ast l[] = {lhs, rhs};
			return {lhs.ctx(), Z3_mk_or(lhs.ctx(), 2, l)};
		}
	}
	RC_INLINE static expr bit_and(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvor(lhs.ctx(), lhs, rhs)};
		} else {
			Z3_ast l[] = {lhs, rhs};
			return {lhs.ctx(), Z3_mk_and(lhs.ctx(), 2, l)};
		}
	}
	RC_INLINE static expr bit_shl(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvshl(lhs.ctx(), lhs, rhs)};
		} else {
			return bit_and(bit_not(rhs), lhs);
		}
	}
	RC_INLINE static expr bit_shr(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvlshr(lhs.ctx(), lhs, rhs)};
		} else {
			return bit_and(bit_not(rhs), lhs);
		}
	}
	RC_INLINE static expr bit_sar(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvashr(lhs.ctx(), lhs, rhs)};
		} else {
			return bit_and(bit_not(rhs), lhs);
		}
	}
	RC_INLINE static expr add(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvadd(lhs.ctx(), lhs, rhs)};
		} else {
			return bit_xor(lhs, rhs);
		}
	}
	RC_INLINE static expr sub(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvsub(lhs.ctx(), lhs, rhs)};
		} else {
			return bit_xor(lhs, rhs);
		}
	}
	RC_INLINE static expr mul(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvmul(lhs.ctx(), lhs, rhs)};
		} else {
			return bit_and(lhs, rhs);
		}
	}
	RC_INLINE static expr ge(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvsge(lhs.ctx(), lhs, rhs)};
		} else if (lhs.is_fpa()) {
			return {lhs.ctx(), Z3_mk_fpa_geq(lhs.ctx(), lhs, rhs)};
		} else {
			return bit_or(bit_not(rhs), lhs);
		}
	}
	RC_INLINE static expr uge(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvuge(lhs.ctx(), lhs, rhs)};
		} else {
			return bit_or(bit_not(rhs), lhs);
		}
	}
	RC_INLINE static expr gt(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvsgt(lhs.ctx(), lhs, rhs)};
		} else if (lhs.is_fpa()) {
			return {lhs.ctx(), Z3_mk_fpa_gt(lhs.ctx(), lhs, rhs)};
		} else {
			return bit_and(bit_not(rhs), lhs);
		}
	}
	RC_INLINE static expr ugt(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvugt(lhs.ctx(), lhs, rhs)};
		} else {
			return bit_and(bit_not(rhs), lhs);
		}
	}
	RC_INLINE static expr lt(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvslt(lhs.ctx(), lhs, rhs)};
		} else if (lhs.is_fpa()) {
			return {lhs.ctx(), Z3_mk_fpa_lt(lhs.ctx(), lhs, rhs)};
		} else {
			return bit_and(bit_not(lhs), rhs);
		}
	}
	RC_INLINE static expr ult(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvult(lhs.ctx(), lhs, rhs)};
		} else {
			return bit_and(bit_not(lhs), rhs);
		}
	}
	RC_INLINE static expr le(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvsle(lhs.ctx(), lhs, rhs)};
		} else if (lhs.is_fpa()) {
			return {lhs.ctx(), Z3_mk_fpa_leq(lhs.ctx(), lhs, rhs)};
		} else {
			return bit_or(bit_not(lhs), rhs);
		}
	}
	RC_INLINE static expr ule(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return {lhs.ctx(), Z3_mk_bvule(lhs.ctx(), lhs, rhs)};
		} else {
			return bit_or(bit_not(lhs), rhs);
		}
	}
	RC_INLINE static expr umax(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return ite(z3x::uge(lhs, rhs), lhs, rhs);
		} else if (lhs.is_fpa()) {
			return {lhs.ctx(), Z3_mk_fpa_max(lhs.ctx(), lhs, rhs)};
		} else {
			return lhs || rhs;
		}
	}
	RC_INLINE static expr max(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return ite(z3x::ge(lhs, rhs), lhs, rhs);
		} else if (lhs.is_fpa()) {
			return {lhs.ctx(), Z3_mk_fpa_max(lhs.ctx(), lhs, rhs)};
		} else {
			return lhs || rhs;
		}
	}
	RC_INLINE static expr umin(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return ite(z3x::uge(lhs, rhs), rhs, lhs);
		} else if (lhs.is_fpa()) {
			return {lhs.ctx(), Z3_mk_fpa_min(lhs.ctx(), lhs, rhs)};
		} else {
			return lhs && rhs;
		}
	}
	RC_INLINE static expr min(const expr& lhs, const expr& rhs) {
		if (lhs.is_bv()) {
			return ite(z3x::ge(lhs, rhs), rhs, lhs);
		} else if (lhs.is_fpa()) {
			return {lhs.ctx(), Z3_mk_fpa_min(lhs.ctx(), lhs, rhs)};
		} else {
			return lhs && rhs;
		}
	}
	RC_INLINE static expr sqrt(const expr& lhs) { return {lhs.ctx(), Z3_mk_fpa_sqrt(lhs.ctx(), lhs.ctx().fpa_rounding_mode(), lhs)}; }
	RC_INLINE static expr eq(const expr& lhs, const expr& rhs) { return lhs == rhs; }
	RC_INLINE static expr ne(const expr& lhs, const expr& rhs) { return lhs != rhs; }
	// TODO: lsb, msb, popcnt, byteswap, ceil, floor, trunc, exp, log, pow.

	// Given an expression and a sort, casts between them.
	//
	expr cast(const expr& val, const sort& into, bool sx) {
		// Float:
		if (val.is_fpa()) {
			// f -> i
			if (into.is_bv()) {
				if (sx)
					return z3::fpa_to_sbv(val, into.bv_size());
				else
					return z3::fpa_to_ubv(val, into.bv_size());
			}
			// f -> f
			else if (into.is_fpa()) {
				return z3::fpa_to_fpa(val, into);
			}
			// f -> b [illegal]
		}
		// Int:
		else if (val.is_bv()) {
			// i -> i
			if (into.is_bv()) {
				auto n = into.bv_size();
				auto m = val.get_sort().bv_size();
				if (n < m) {
					return val.extract(n - 1, 0);
				} else if (n == m) {
					return val;
				} else if (sx) {
					return z3::sext(val, m);
				} else {
					return z3::zext(val, m);
				}
			}
			// i -> f
			else if (into.is_fpa()) {
				if (sx)
					return z3::sbv_to_fpa(val, into);
				else
					return z3::ubv_to_fpa(val, into);
			}
			// i -> b
			else {
				return val.bit2bool(0);
			}
		}
		// Bool:
		else {
			// b -> i
			if (into.is_bv()) {
				return z3::ite(val, val.ctx().bv_val(1, into.bv_size()), val.ctx().bv_val(0, into.bv_size()));
			}
			// b -> b
			else if (into.is_bool()) {
				return val;
			}
			// b -> f [illegal]
		}
		return {val.ctx()};
	}

	// Expression depth calculation.
	//
	size_t expr_depth(Z3_context c, Z3_ast a) {
		auto	 k = Z3_get_ast_kind(c, a);
		size_t n = 0;
		if (k == Z3_APP_AST || k == Z3_NUMERAL_AST) {
			size_t i = Z3_get_app_num_args(c, (Z3_app) a);
			n += i;
			for (size_t j = 0; j != i; j++) {
				n += expr_depth(c, Z3_get_app_arg(c, (Z3_app) a, j));
			}
		}
		return n;
	}

	// Symbol based variables.
	//
	std::optional<i32> get_variable(const expr& expr) {
		if (expr.kind() == Z3_APP_AST) {
			auto decl = expr.decl();
			if (decl.decl_kind() == Z3_OP_UNINTERPRETED) {
				return decl.name().to_int();
			}
		}
		return std::nullopt;
	}

	// Converts from an IR type to a Z3 sort and vice versa.
	//
	ir::type type_of(const sort& sort) {
		// Floats.
		//
		if (sort.is_fpa()) {
			if (sort.fpa_ebits() == 11) {
				return ir::type::f64;
			} else {
				return ir::type::f32;
			}
			// TODO: fp80?
		}
		// Ints.
		//
		else if (sort.is_bv()) {
			return ir::int_type(sort.bv_size());
		}
		// Bool.
		//
		else if (sort.is_bool()) {
			return ir::type::i1;
		}
		return ir::type::none;
	}
	sort make_sort(context& c, ir::type ty) {
		switch (ty) {
			case ir::type::i1:
				return c.bool_sort();
			case ir::type::i8:
				return c.bv_sort(8);
			case ir::type::i16:
				return c.bv_sort(16);
			case ir::type::i32:
				return c.bv_sort(32);
			case ir::type::i64:
				return c.bv_sort(64);
			case ir::type::i128:
				return c.bv_sort(128);
			case ir::type::f32:
				return c.fpa_sort<32>();
			case ir::type::f64:
				return c.fpa_sort<64>();
			// TODO: fp80?
			default: return c;
		}
	}

	// Converts from an IR constant to Z3 numeral and vice versa.
	//
	ir::constant value_of(const expr& expr) {
		if (!is_value(expr)) {
			return std::nullopt;
		}
		switch (type_of(expr)) {
			case ir::type::i1:
				return (bool) expr.is_true();
			case ir::type::i8:
				return (i8) expr.as_int64();
			case ir::type::i16:
				return (i16) expr.as_int64();
			case ir::type::i32:
				return (i32) expr.as_int64();
			case ir::type::i64:
				return (i64) expr.as_int64();
			case ir::type::i128:
				// TODO:
				return std::nullopt;
			case ir::type::f32:
				return (f32) expr.as_double();
			case ir::type::f64:
				return (f64)expr.as_double();
			// TODO: fp80?
			default:
				return std::nullopt;
		}
	}
	expr make_value(context& c, const ir::constant& v) {
		switch (v.get_type()) {
			case ir::type::i1:
				return c.bool_val(v.get<bool>());
			case ir::type::i8:
				return c.bv_val(v.get<i8>(), 8);
			case ir::type::i16:
				return c.bv_val(v.get<i16>(), 16);
			case ir::type::i32:
				return c.bv_val(v.get<i32>(), 32);
			case ir::type::i64:
				return c.bv_val(v.get<i64>(), 64);
			// todo: i128
			case ir::type::f32:
				return c.fpa_val(v.get<f32>());
			case ir::type::f64:
				return c.fpa_val(v.get<f64>());
			default:
				return c;
		}
	}
	expr make_value(context& c, const ir::constant& v, u8 lane) {
		auto ty = v.get_type();
		auto& desc = enum_reflect(ty);
		if (desc.kind == ir::type_kind::vector_fp || desc.kind == ir::type_kind::vector_int) {
			if (lane < desc.lane_width) {
				switch (desc.underlying) {
					case ir::type::i8:
						return c.bv_val(((i8*) v.address())[lane], 8);
					case ir::type::i16:
						return c.bv_val(((i16*) v.address())[lane], 16);
					case ir::type::i32:
						return c.bv_val(((i32*) v.address())[lane], 32);
					case ir::type::i64:
						return c.bv_val(((i64*) v.address())[lane], 64);
					// todo: i128
					case ir::type::f32:
						return c.fpa_val(((f32*) v.address())[lane]);
					case ir::type::f64:
						return c.fpa_val(((f64*) v.address())[lane]);
					default:
						return c;
				}
			}
		} else if (lane == 0) {
			return make_value(c, v);
		}
		return c;
	}
	bool is_value(const expr& expr) {
		if (expr) {
			switch (expr.kind()) {
				case Z3_NUMERAL_AST:
					return true;
				case Z3_APP_AST: {
					auto k = expr.decl().decl_kind();
					return k == Z3_OP_TRUE || k == Z3_OP_FALSE;
				}
				default:
					break;
			}
		}
		return false;
	}

	// Applies an ir::op given the operands as Z3 expressions.
	//
	expr apply(ir::op o, expr rhs) {
		if (rhs) {
			switch (o) {
				// clang-format off
				case ir::op::neg: return z3x::neg(rhs);
				case ir::op::abs: return z3x::abs(rhs);
				case ir::op::bit_not: return z3x::bit_not(rhs);
				case ir::op::sqrt: return z3x::sqrt(rhs);
				default: break;
				// clang-format on
			}
		}
		return rhs.ctx();
	}
	expr apply(ir::op o, expr lhs, expr rhs) {
		if (ok(lhs) && ok(rhs)) {
			switch (o) {
				// clang-format off
				case ir::op::add: return z3x::add(lhs, rhs);
				case ir::op::sub: return z3x::sub(lhs, rhs);
				case ir::op::mul: return z3x::mul(lhs, rhs);
				case ir::op::div: return z3x::div(lhs, rhs);
				case ir::op::udiv: return z3x::udiv(lhs, rhs);
				case ir::op::rem: return z3x::rem(lhs, rhs);
				case ir::op::urem: return z3x::urem(lhs, rhs);
				case ir::op::bit_or: return z3x::bit_or(lhs, rhs);
				case ir::op::bit_and: return z3x::bit_and(lhs, rhs);
				case ir::op::bit_xor: return z3x::bit_xor(lhs, rhs);
				case ir::op::bit_shl: return z3x::bit_shl(lhs, rhs);
				case ir::op::bit_shr: return z3x::bit_shr(lhs, rhs);
				case ir::op::bit_sar: return z3x::bit_sar(lhs, rhs);
				case ir::op::bit_rol: return z3x::rol(lhs, rhs);
				case ir::op::bit_ror: return z3x::ror(lhs, rhs);
				case ir::op::max: return z3x::max(lhs, rhs);
				case ir::op::umax: return z3x::umax(lhs, rhs);
				case ir::op::min: return z3x::min(rhs, lhs);
				case ir::op::umin: return z3x::umax(rhs, lhs);
				case ir::op::gt: return z3x::gt(lhs, rhs);
				case ir::op::ugt: return z3x::ugt(lhs, rhs);
				case ir::op::le: return z3x::le(lhs, rhs);
				case ir::op::ule: return z3x::ule(lhs, rhs);
				case ir::op::ge: return z3x::ge(lhs, rhs);
				case ir::op::uge: return z3x::uge(lhs, rhs);
				case ir::op::lt: return z3x::lt(lhs, rhs);
				case ir::op::ult: return z3x::ult(lhs, rhs);
				case ir::op::eq: return z3x::eq(lhs, rhs);
				case ir::op::ne: return z3x::ne(lhs, rhs);
				default: break;
				// clang-format on
			}
		}
		return rhs.ctx();
	}

	// Translates ir::insn/ir::operand instances into a Z3 expression trees.
	//
	expr to_expr(variable_set& vs, context& c, ir::value* v, size_t max_depth) {
		// If of type instruction and we're not at max depth:
		//
		auto i = v->get_if<ir::insn>();
		if (i && max_depth != 0) {
			--max_depth;
			if (i->op == ir::opcode::cmp) {
				auto lhs = to_expr(vs, c, i->operands()[1], max_depth);
				auto rhs = to_expr(vs, c, i->operands()[2], max_depth);
				if (auto val = apply(i->operands()[0].get_const().get<ir::op>(), lhs, rhs)) {
					return val;
				}
			} else if (i->op == ir::opcode::binop) {
				auto lhs = to_expr(vs, c, i->operands()[1], max_depth);
				auto rhs = to_expr(vs, c, i->operands()[2], max_depth);
				if (auto val = apply(i->operands()[0].get_const().get<ir::op>(), lhs, rhs)) {
					return val;
				}
			} else if (i->op == ir::opcode::unop) {
				auto rhs = to_expr(vs, c, i->operands()[1], max_depth);
				if (auto val = apply(i->operands()[0].get_const().get<ir::op>(), rhs)) {
					return val;
				}
			} else if (i->op == ir::opcode::select) {
				auto cc = to_expr(vs, c, i->operands()[0], max_depth);
				auto lhs = to_expr(vs, c, i->operands()[1], max_depth);
				auto rhs = to_expr(vs, c, i->operands()[2], max_depth);
				if (ok(cc) && ok(lhs) && ok(rhs)) {
					return z3::ite(cc, lhs, rhs);
				}
			} else if (i->op == ir::opcode::cast) {
				auto into = make_sort(c, i->template_types[1]);
				auto val	 = to_expr(vs, c, i->operands()[0], max_depth);
				if (ok(into) && ok(val)) {
					val = cast(val, into);
					if (val) {
						return val;
					}
				}
			} else if (i->op == ir::opcode::cast_sx) {
				auto into = make_sort(c, i->template_types[1]);
				auto val	 = to_expr(vs, c, i->operands()[0], max_depth);
				if (ok(into) && ok(val)) {
					val = cast_sx(val, into);
					if (val) {
						return val;
					}
				}
			}
		}

		// Named symbol.
		//
		return vs.emplace(c, v);
	}
	expr to_expr(variable_set& vs, context& c, const ir::operand& o, size_t max_depth) {
		if (o.is_const()) {
			return make_value(c, o.const_val);
		} else if (auto i = o.get_value()->get_if<ir::insn>()) {
			return to_expr(vs, c, i, max_depth);
		} else {
			return c;
		}
	}



#define DEF_BINOP(Z3OP, RCOC, RCOP)                                                                        \
	case Z3OP: {                                                                                            \
		if (args.size() != 2) {                                                                              \
			fmt::abort("expected 2 arguments to %s (%x)", decl.name().str(), decl.decl_kind());               \
		}                                                                                                    \
		it = bb->insert(it, RC_CONCAT(ir::make_, RCOC)(RCOP, std::move(args[0]), std::move(args[1]))); \
		return ret_and_cache(it++);                                                                          \
	}
#define DEF_UNOP(Z3OP, RCOC, RCOP)                                                           \
	case Z3OP: {                                                                              \
		if (args.size() != 1) {                                                                \
			fmt::abort("expected 1 arguments to %s (%x)", decl.name().str(), decl.decl_kind()); \
		}                                                                                      \
		it = bb->insert(it, RC_CONCAT(ir::make_, RCOC)(RCOP, std::move(args[0])));       \
		return ret_and_cache(it++);                                                            \
	}																																															

	// Translates a Z3 expression tree into a series of IR instructions at the end of the block.
	//
	ir::variant from_expr(variable_set& vs, const expr& expr, ir::basic_block* bb, list::iterator<ir::insn>& it) {
		// Declare cache writer.
		//
		auto ret_and_cache = [&](ir::insn* i) {
			vs.write_list.emplace_back(i, expr);
			return i;
		};

		// If application:
		//
		auto expr_kind = expr.kind();
		if (expr_kind == Z3_APP_AST) {
			// Try fetching from cache.
			//
			for (auto& c : vs.write_list) {
				if (c.first && c.first->block == bb && z3::eq(c.second, expr)) {
					return ir::variant(c.first);
				}
			}

			// Translate all arguments.
			//
			std::vector<ir::variant> args(expr.num_args());
			for (size_t i = 0; i != args.size(); i++) {
				args[i] = from_expr(vs, expr.arg(i), bb, it);
				if (args[i].is_const() && args[i].const_val.is<void>())
					return {};
			}

			// Finally parse the applied operator.
			//
			auto decl = expr.decl();
			u32  decl_kind = decl.decl_kind();
			switch (decl_kind) {
				// Boolean constants.
				//
				case Z3_OP_TRUE:
					return ir::constant(true);
				case Z3_OP_FALSE:
					return ir::constant(false);

				// Variable.
				//
				case Z3_OP_UNINTERPRETED: {
					if (auto val = vs[expr]) {
						return ir::variant(std::move(val));
					} else {
						return std::nullopt;
					}
				}

				// Simply mapped operators.
				//
				// clang-format off
				DEF_BINOP(Z3_OP_EQ,      cmp, ir::op::eq);
				DEF_BINOP(Z3_OP_SLEQ,    cmp, ir::op::le);
				DEF_BINOP(Z3_OP_ULEQ,    cmp, ir::op::ule);
				DEF_BINOP(Z3_OP_SGEQ,    cmp, ir::op::ge);
				DEF_BINOP(Z3_OP_UGEQ,    cmp, ir::op::uge);
				DEF_BINOP(Z3_OP_SLT,     cmp, ir::op::lt);
				DEF_BINOP(Z3_OP_ULT,     cmp, ir::op::ult);
				DEF_BINOP(Z3_OP_SGT,     cmp, ir::op::gt);
				DEF_BINOP(Z3_OP_UGT,     cmp, ir::op::ugt);
				DEF_BINOP(Z3_OP_FPA_EQ,  cmp, ir::op::eq);
				DEF_BINOP(Z3_OP_FPA_LT,  cmp, ir::op::lt);
				DEF_BINOP(Z3_OP_FPA_GT,  cmp, ir::op::gt);
				DEF_BINOP(Z3_OP_FPA_LE,  cmp, ir::op::le);
				DEF_BINOP(Z3_OP_FPA_GE,  cmp, ir::op::ge);
				
				DEF_UNOP(Z3_OP_NOT,      unop,  ir::op::bit_not);
				DEF_UNOP(Z3_OP_BNOT,     unop,  ir::op::bit_not);
				DEF_UNOP(Z3_OP_FPA_NEG,  unop,  ir::op::neg);
				DEF_UNOP(Z3_OP_BNEG,     unop,  ir::op::neg);

				DEF_BINOP(Z3_OP_FPA_ADD, binop, ir::op::add);
				DEF_BINOP(Z3_OP_BADD,    binop, ir::op::add);
				DEF_BINOP(Z3_OP_FPA_SUB, binop, ir::op::sub);
				DEF_BINOP(Z3_OP_BSUB,    binop, ir::op::sub);

				DEF_BINOP(Z3_OP_FPA_MUL, binop, ir::op::mul);
				DEF_BINOP(Z3_OP_BMUL,    binop, ir::op::mul);
				DEF_BINOP(Z3_OP_FPA_DIV, binop, ir::op::div);
				DEF_BINOP(Z3_OP_BSDIV,   binop, ir::op::div);
				DEF_BINOP(Z3_OP_BUDIV,   binop, ir::op::udiv);
				
				DEF_BINOP(Z3_OP_FPA_REM, binop, ir::op::rem);
				DEF_BINOP(Z3_OP_BSREM,   binop, ir::op::rem);
				DEF_BINOP(Z3_OP_BSMOD,   binop, ir::op::rem);
				DEF_BINOP(Z3_OP_BUREM,   binop, ir::op::urem);

				DEF_BINOP(Z3_OP_FPA_ABS, binop, ir::op::abs);
				DEF_BINOP(Z3_OP_FPA_MIN, binop, ir::op::min);
				DEF_BINOP(Z3_OP_FPA_MAX, binop, ir::op::max);
				
				DEF_BINOP(Z3_OP_AND,  binop, ir::op::bit_and);
				DEF_BINOP(Z3_OP_BAND, binop, ir::op::bit_and);
				DEF_BINOP(Z3_OP_OR,   binop, ir::op::bit_or);
				DEF_BINOP(Z3_OP_BOR,  binop, ir::op::bit_or);
				DEF_BINOP(Z3_OP_XOR,  binop, ir::op::bit_xor);
				DEF_BINOP(Z3_OP_BXOR, binop, ir::op::bit_xor);

				DEF_BINOP(Z3_OP_BSHL,  binop, ir::op::bit_shl);
				DEF_BINOP(Z3_OP_BLSHR, binop, ir::op::bit_shr);
				DEF_BINOP(Z3_OP_BASHR, binop, ir::op::bit_sar);
				
				DEF_BINOP(Z3_OP_ROTATE_LEFT,      binop, ir::op::bit_rol);
				DEF_BINOP(Z3_OP_EXT_ROTATE_LEFT,  binop, ir::op::bit_rol);
				DEF_BINOP(Z3_OP_ROTATE_RIGHT,     binop, ir::op::bit_ror);
				DEF_BINOP(Z3_OP_EXT_ROTATE_RIGHT, binop, ir::op::bit_ror);
				DEF_BINOP(Z3_OP_FPA_SQRT,         binop, ir::op::sqrt);
				// clang-format on

				// Casts.
				//
				case Z3_OP_FPA_TO_SBV:
				case Z3_OP_FPA_TO_FP: {	 // (rounding mode, value)
					RC_ASSERT(args.size() == 2);
					auto ty = (ir::type) type_of(expr);
					it		  = bb->insert(it, ir::make_cast(ty, std::move(args[1])));
					return ret_and_cache(it++);
				}
				case Z3_OP_FPA_TO_UBV:
				case Z3_OP_FPA_TO_FP_UNSIGNED: {	 // (rounding mode, value)
					RC_ASSERT(args.size() == 2);
					auto ty = (ir::type) type_of(expr);
					it		  = bb->insert(it, ir::make_cast(ty, std::move(args[1])));
					return ret_and_cache(it++);
				}
				case Z3_OP_SIGN_EXT: {	// (value)
					RC_ASSERT(args.size() == 1);
					auto ty = (ir::type) type_of(expr);
					it		  = bb->insert(it, ir::make_cast_sx(ty, std::move(args[0])));
					return ret_and_cache(it++);
				}
				case Z3_OP_ZERO_EXT: {	// (value)
					RC_ASSERT(args.size() == 1);
					auto ty = (ir::type) type_of(expr);
					it		  = bb->insert(it, ir::make_cast(ty, std::move(args[0])));
					return ret_and_cache(it++);
				}
				// TODO: Wheres BV -> FP?

				// Boolean logic.
				//
				DEF_BINOP(Z3_OP_DISTINCT, cmp, ir::op::ne);	// TODO: Not really, vararg.
				case Z3_OP_ITE: {
					RC_ASSERT(args.size() == 3);
					it = bb->insert(it, ir::make_select(std::move(args[0]), std::move(args[1]), std::move(args[2])));
					return ret_and_cache(it++);
				}

				// Bit vectors.
				//
				case Z3_OP_EXTRACT: {
					RC_ASSERT(args.size() == 1);
					auto decl = expr.decl();
					auto hi = Z3_get_decl_int_parameter(decl.ctx(), decl, 0);
					auto lo = Z3_get_decl_int_parameter(decl.ctx(), decl, 1);

					// Shift first if needed.
					//
					if (lo != 0) {
						auto ty = args[0].get_type();
						it		  = bb->insert(it, ir::make_binop(ir::op::bit_shr, std::move(args[0]), ir::constant(ty, lo)));
						args[0] = it++.get();
					}

					// Round the final size up and cast.
					//
					auto bitlen = hi - lo + 1;
					if (bitlen <= 8) {
						it = bb->insert(it, ir::make_cast(ir::type::i8, std::move(args[0])));
						return ret_and_cache(it++);
					} else if (bitlen <= 16) {
						it = bb->insert(it, ir::make_cast(ir::type::i16, std::move(args[0])));
						return ret_and_cache(it++);
					} else if (bitlen <= 32) {
						it = bb->insert(it, ir::make_cast(ir::type::i32, std::move(args[0])));
						return ret_and_cache(it++);
					} else if (bitlen <= 64) {
						it = bb->insert(it, ir::make_cast(ir::type::i64, std::move(args[0])));
						return ret_and_cache(it++);
					} else if (bitlen <= 128) {
						it = bb->insert(it, ir::make_cast(ir::type::i128, std::move(args[0])));
						return ret_and_cache(it++);
					}
					fmt::abort("invalid extract %d:%d", lo, hi);
					break;
				}
				case Z3_OP_CONCAT: {
					RC_ASSERT(args.size() == 2);
					auto ty0 = args[0].get_type();
					auto ty1 = args[1].get_type();
					auto rty = std::max(ty0, ty1);

					// If types are mismatching:
					//
					if (ty0 != rty || ty1 != rty) {
						// Cast the other.
						//
						auto& bad = ty0 != rty ? args[0] : args[1];
						if (bad.is_const()) {
							bad = std::move(bad).get_const().cast_zx(rty);
						} else {
							it	 = bb->insert(it, ir::make_cast(rty, std::move(bad)));
							bad = it++.get();
						}
					}

					// Find the bit offset of the first operand.
					//
					u8 offset = expr.arg(1).get_sort().bv_size();

					// Shift the first operand.
					//
					if (args[0].is_const()) {
						args[0] = std::move(args[0]).get_const().apply(ir::op::bit_shl, ir::constant(rty, offset));
					} else {
						it		  = bb->insert(it, ir::make_binop(ir::op::bit_shl, std::move(args[0]), ir::constant(rty, offset)));
						args[0] = it++.get();
					}

					// Or both of it together and return.
					//
					it = bb->insert(it, ir::make_binop(ir::op::bit_or, std::move(args[0]), std::move(args[1])));
					return ret_and_cache(it++);
				}
				case Z3_OP_BNAND: {
					RC_ASSERT(args.size() == 2);
					it = bb->insert(it, ir::make_binop(ir::op::bit_and, std::move(args[0]), std::move(args[1])));
					it = bb->insert(++it, ir::make_unop(ir::op::bit_not, it.get()));
					return ret_and_cache(it++);
				}
				case Z3_OP_BNOR: {
					RC_ASSERT(args.size() == 2);
					it = bb->insert(it, ir::make_binop(ir::op::bit_or, std::move(args[0]), std::move(args[1])));
					it = bb->insert(++it, ir::make_unop(ir::op::bit_not, it.get()));
					return ret_and_cache(it++);
				}
				case Z3_OP_BXNOR: {
					RC_ASSERT(args.size() == 2);
					it = bb->insert(it, ir::make_binop(ir::op::bit_xor, std::move(args[0]), std::move(args[1])));
					it = bb->insert(++it, ir::make_unop(ir::op::bit_not, it.get()));
					return ret_and_cache(it++);
				}
				//case Z3_OP_REPEAT:
				//case Z3_OP_BREDOR:
				//case Z3_OP_BREDAND:
				//case Z3_OP_BCOMP:
				//case Z3_OP_BIT2BOOL:

				// Floating point.
				//
				//case Z3_OP_FPA_FMA:
				//case Z3_OP_FPA_ROUND_TO_INTEGRAL:
				//case Z3_OP_FPA_IS_NAN:
				//case Z3_OP_FPA_IS_INF:
				//case Z3_OP_FPA_IS_ZERO:
				//case Z3_OP_FPA_IS_NORMAL:
				//case Z3_OP_FPA_IS_SUBNORMAL:
				//case Z3_OP_FPA_IS_NEGATIVE:
				//case Z3_OP_FPA_IS_POSITIVE:

				// Unknown.
				//
				default: {
					printf("Don't know how to translate decl %x (%s)\n", decl.decl_kind(), decl.name().str().c_str());
					for (auto e : const_cast<z3::expr&>(expr)) {
						printf("-> %s\n", e.to_string().c_str());
					}
					fmt::abort_no_msg();
				}
			}
		} else if (expr_kind == Z3_SORT_AST) {
			auto ty = z3x::type_of(sort(expr.ctx(), expr));
			if (ty == ir::type::none) {
				return std::nullopt;
			} else {
				return ir::constant((u32)ty);
			}
		} else if (expr_kind == Z3_NUMERAL_AST) {
			if (expr.get_sort().sort_kind() == Z3_ROUNDING_MODE_SORT) {
				return ir::constant((i64) expr);
			}
			return value_of(expr);
		} else {
			printf("Don't know how to translate expr kind %u\n", expr_kind);
			printf("-> %s\n", expr.to_string().c_str());
			fmt::abort_no_msg();
		}
	}
};