#include <retro/arch/x86/sema.hpp>
#include <retro/common.hpp>

using namespace retro;
using namespace retro::arch::x86;

// TODO: AVX variants.

// Vector logical.
//
DECL_SEMA(XORPS) {
	// Pattern: [xorps reg, reg] <=> [mov reg, {0....}]
	if (ins.op[0].type == arch::mop_type::reg && ins.op[1].type == arch::mop_type::reg) {
		if (ins.op[0].r == ins.op[1].r) {
			write_reg(sema_context(), ins.op[0].r, ir::constant(i32x4{0}));
			return diag::ok;
		}
	}

	auto lhs = read(sema_context(), 0, ir::type::i32x4);
	auto rhs = read(sema_context(), 1, ir::type::i32x4);
	auto res = bb->push_binop(ir::op::bit_xor, lhs, rhs);
	write(sema_context(), 0, res);
	return diag::ok;
}
DECL_SEMA(XORPD) { return lift_XORPS(sema_context()); }
DECL_SEMA(PXOR) { return lift_XORPS(sema_context()); }

DECL_SEMA(ANDPS) {
	auto lhs = read(sema_context(), 0, ir::type::i32x4);
	auto rhs = read(sema_context(), 1, ir::type::i32x4);
	auto res = bb->push_binop(ir::op::bit_and, lhs, rhs);
	write(sema_context(), 0, res);
	return diag::ok;
}
DECL_SEMA(ANDPD) { return lift_ANDPS(sema_context()); }
DECL_SEMA(PAND) { return lift_ANDPS(sema_context()); }

DECL_SEMA(ANDNPS) {
	auto lhs = read(sema_context(), 0, ir::type::i32x4);
	auto rhs = read(sema_context(), 1, ir::type::i32x4);
	auto res = bb->push_binop(ir::op::bit_and, bb->push_unop(ir::op::bit_not, lhs), rhs);
	write(sema_context(), 0, res);
	return diag::ok;
}
DECL_SEMA(ANDNPD) { return lift_ANDNPS(sema_context()); }
DECL_SEMA(PANDN) { return lift_ANDNPS(sema_context()); }

DECL_SEMA(ORPS) {
	auto lhs = read(sema_context(), 0, ir::type::i32x4);
	auto rhs = read(sema_context(), 1, ir::type::i32x4);
	auto res = bb->push_binop(ir::op::bit_or, lhs, rhs);
	write(sema_context(), 0, res);
	return diag::ok;
}
DECL_SEMA(ORPD) { return lift_ORPS(sema_context()); }
DECL_SEMA(POR) { return lift_ORPS(sema_context()); }

// Vector-width moves.
//
DECL_SEMA(MOVUPS) {
	constexpr auto ty = ir::type::f32x4;
	write(sema_context(), 0, read(sema_context(), 1, ty));
	return diag::ok;
}
DECL_SEMA(MOVAPS) {
	constexpr auto ty = ir::type::f32x4;
	write(sema_context(), 0, read(sema_context(), 1, ty));
	return diag::ok;
}
DECL_SEMA(MOVUPD) {
	constexpr auto ty = ir::type::f64x2;
	write(sema_context(), 0, read(sema_context(), 1, ty));
	return diag::ok;
}
DECL_SEMA(MOVAPD) {
	constexpr auto ty = ir::type::f64x2;
	write(sema_context(), 0, read(sema_context(), 1, ty));
	return diag::ok;
}
DECL_SEMA(MOVDQU) {
	constexpr auto ty = ir::type::i32x4;
	write(sema_context(), 0, read(sema_context(), 1, ty));
	return diag::ok;
}
DECL_SEMA(MOVDQA) {
	constexpr auto ty = ir::type::i32x4;
	write(sema_context(), 0, read(sema_context(), 1, ty));
	return diag::ok;
}

// Vector-scalar moves.
//
template<ir::type TyVec, ir::type TyPiece>
static diag::lazy scalar_move(SemaContext) {
	if (ins.op[0].type == arch::mop_type::reg && ins.op[1].type == arch::mop_type::reg) {
		// Legacy SSE version when the source and destination operands are both XMM registers
		// DST[31..0] <- SRC[31:0]
		auto dst = read(sema_context(), 0, TyVec);
		auto src = read(sema_context(), 1, TyVec);

		auto val = bb->push_extract(TyPiece, std::move(src), 0);
		dst		= bb->push_insert(std::move(dst), 0, val);
		write(sema_context(), 0, std::move(dst));
	} else if (ins.op[0].type == arch::mop_type::reg) {
		// Legacy SSE version when the source operand is memory and the destination is an XMM register
		//  DST[31..0] <- SRC[31:0]
		//  DST[128..32] <- 0
		auto src = read(sema_context(), 1, TyPiece);
		auto dst = bb->push_insert(ir::constant(ir::type_t<TyVec>{0}), 0, std::move(src));
		write(sema_context(), 0, std::move(dst));
	} else {
		// when the source operand is an XMM register and the destination is memory.
		// DEST[31:0] <- SRC[31:0]
		auto src = read(sema_context(), 1, TyVec);
		auto dst = bb->push_extract(TyPiece, std::move(src), 0);
		write(sema_context(), 0, std::move(dst));
	}
	return diag::ok;
}

// Forward MOVSD handler.
namespace retro::arch::x86 {
	diag::lazy make_movs(SemaContext);
};

DECL_SEMA(MOVSD) {
	if (ins.operand_count == 0) {
		return make_movs(sema_context());
	} else {
		return scalar_move<ir::type::f64x2, ir::type::f64>(sema_context());
	}
}
DECL_SEMA(MOVSS) { return scalar_move<ir::type::f32x4, ir::type::f32>(sema_context()); }
DECL_SEMA(MOVD) {
	auto& r = ins.op[0].type == arch::mop_type::reg ? ins.op[0].r : ins.op[1].r;
	if (r.get_kind() == arch::reg_kind::simd64) {
		return scalar_move<ir::type::i32x2, ir::type::i32>(sema_context());
	} else {
		return scalar_move<ir::type::i32x4, ir::type::i32>(sema_context());
	}
}
DECL_SEMA(MOVQ) {
	auto& r = ins.op[0].type == arch::mop_type::reg ? ins.op[0].r : ins.op[1].r;
	if (r.get_kind() == arch::reg_kind::simd64) {
		write(sema_context(), 0, read(sema_context(), 1, ir::type::i64));
		return diag::ok;
	} else {
		return scalar_move<ir::type::i64x2, ir::type::i64>(sema_context());
	}
}

// Vector test.
//
DECL_SEMA(PTEST) {
	constexpr auto ty	 = ir::type::i32x4;

	i32x4 vzero{0};
	auto	src	= read(sema_context(), 0, ty);
	auto	dst	= read(sema_context(), 1, ty);
	auto	vand	= bb->push_binop(ir::op::bit_and, src, dst);
	auto	vandn = bb->push_binop(ir::op::bit_and, src, bb->push_unop(ir::op::bit_not, dst));

	bb->push_write_reg(reg::flag_zf, bb->push_cmp(ir::op::eq, vand, vzero));
	bb->push_write_reg(reg::flag_cf, bb->push_cmp(ir::op::eq, vandn, vzero));
	bb->push_write_reg(reg::flag_af, false);
	bb->push_write_reg(reg::flag_of, false);
	bb->push_write_reg(reg::flag_pf, false);
	bb->push_write_reg(reg::flag_sf, false);
	return diag::ok;
}

// Vector arithmetic.
//
template<ir::op Op, ir::type TyVec, ir::type TyPiece>
static diag::lazy scalar_op(SemaContext) {
	constexpr auto Kind = enum_reflect(Op).kind;

	ir::variant src;
	if (ins.op[1].type == arch::mop_type::reg) {
		auto srcv = read(sema_context(), 1, TyVec);
		src		 = bb->push_extract(TyPiece, std::move(srcv), 0);
	} else {
		src = read(sema_context(), 1, TyPiece);
	}

	// 128-bit Legacy SSE version
	if constexpr (ir::op_kind::unary_signed <= Kind && Kind <= ir::op_kind::unary_float) {
		// DST[31..] <- OP SRC[31..]
		auto dst	  = read(sema_context(), 0, TyVec);
		write(sema_context(), 0, bb->push_insert(std::move(dst), 0, bb->push_unop(Op, std::move(src))));
		return diag::ok;
	} else {
		// DST[31..] <- DST[31..] OP SRC[31..]
		auto dst	  = read(sema_context(), 0, TyVec);
		auto piece = bb->push_extract(TyPiece, dst, 0);
		piece		  = bb->push_binop(Op, piece, std::move(src));
		write(sema_context(), 0, bb->push_insert(std::move(dst), 0, piece));
		return diag::ok;
	}
}
template<ir::op Op, ir::type TyVec>
static diag::lazy vector_op(SemaContext) {
	auto src = read(sema_context(), 1, TyVec);

	constexpr auto Kind = enum_reflect(Op).kind;
	if constexpr (ir::op_kind::unary_signed <= Kind && Kind <= ir::op_kind::unary_float) {
		write(sema_context(), 0, bb->push_unop(Op, std::move(src)));
		return diag::ok;
	} else {
		auto dst = read(sema_context(), 0, TyVec);
		write(sema_context(), 0, bb->push_binop(Op, std::move(dst), std::move(src)));
		return diag::ok;
	}
}

DECL_SEMA(MULSS) { return scalar_op<ir::op::mul, ir::type::f32x4, ir::type::f32>(sema_context()); }
DECL_SEMA(MULPS) { return vector_op<ir::op::mul, ir::type::f32x4>(sema_context()); }
DECL_SEMA(MULSD) { return scalar_op<ir::op::mul, ir::type::f64x2, ir::type::f64>(sema_context()); }
DECL_SEMA(MULPD) { return vector_op<ir::op::mul, ir::type::f64x2>(sema_context()); }

DECL_SEMA(DIVSS) { return scalar_op<ir::op::div, ir::type::f32x4, ir::type::f32>(sema_context()); }
DECL_SEMA(DIVPS) { return vector_op<ir::op::div, ir::type::f32x4>(sema_context()); }
DECL_SEMA(DIVSD) { return scalar_op<ir::op::div, ir::type::f64x2, ir::type::f64>(sema_context()); }
DECL_SEMA(DIVPD) { return vector_op<ir::op::div, ir::type::f64x2>(sema_context()); }

DECL_SEMA(ADDSS) { return scalar_op<ir::op::add, ir::type::f32x4, ir::type::f32>(sema_context()); }
DECL_SEMA(ADDPS) { return vector_op<ir::op::add, ir::type::f32x4>(sema_context()); }
DECL_SEMA(ADDSD) { return scalar_op<ir::op::add, ir::type::f64x2, ir::type::f64>(sema_context()); }
DECL_SEMA(ADDPD) { return vector_op<ir::op::add, ir::type::f64x2>(sema_context()); }

DECL_SEMA(SUBSS) { return scalar_op<ir::op::sub, ir::type::f32x4, ir::type::f32>(sema_context()); }
DECL_SEMA(SUBPS) { return vector_op<ir::op::sub, ir::type::f32x4>(sema_context()); }
DECL_SEMA(SUBSD) { return scalar_op<ir::op::sub, ir::type::f64x2, ir::type::f64>(sema_context()); }
DECL_SEMA(SUBPD) { return vector_op<ir::op::sub, ir::type::f64x2>(sema_context()); }

DECL_SEMA(MINSS) { return scalar_op<ir::op::min, ir::type::f32x4, ir::type::f32>(sema_context()); }
DECL_SEMA(MINPS) { return vector_op<ir::op::min, ir::type::f32x4>(sema_context()); }
DECL_SEMA(MINSD) { return scalar_op<ir::op::min, ir::type::f64x2, ir::type::f64>(sema_context()); }
DECL_SEMA(MINPD) { return vector_op<ir::op::min, ir::type::f64x2>(sema_context()); }

DECL_SEMA(MAXSS) { return scalar_op<ir::op::max, ir::type::f32x4, ir::type::f32>(sema_context()); }
DECL_SEMA(MAXPS) { return vector_op<ir::op::max, ir::type::f32x4>(sema_context()); }
DECL_SEMA(MAXSD) { return scalar_op<ir::op::max, ir::type::f64x2, ir::type::f64>(sema_context()); }
DECL_SEMA(MAXPD) { return vector_op<ir::op::max, ir::type::f64x2>(sema_context()); }

DECL_SEMA(SQRTSS) { return scalar_op<ir::op::sqrt, ir::type::f32x4, ir::type::f32>(sema_context()); }
DECL_SEMA(SQRTPS) { return vector_op<ir::op::sqrt, ir::type::f32x4>(sema_context()); }
DECL_SEMA(SQRTSD) { return scalar_op<ir::op::sqrt, ir::type::f64x2, ir::type::f64>(sema_context()); }
DECL_SEMA(SQRTPD) { return vector_op<ir::op::sqrt, ir::type::f64x2>(sema_context()); }

DECL_SEMA(PADDB) { return vector_op<ir::op::add, ir::type::i8x16>(sema_context()); }
DECL_SEMA(PADDW) { return vector_op<ir::op::add, ir::type::i16x8>(sema_context()); }
DECL_SEMA(PADDD) { return vector_op<ir::op::add, ir::type::i32x4>(sema_context()); }
DECL_SEMA(PADDQ) { return vector_op<ir::op::add, ir::type::i64x2>(sema_context()); }

DECL_SEMA(PSUBB) { return vector_op<ir::op::sub, ir::type::i8x16>(sema_context()); }
DECL_SEMA(PSUBW) { return vector_op<ir::op::sub, ir::type::i16x8>(sema_context()); }
DECL_SEMA(PSUBD) { return vector_op<ir::op::sub, ir::type::i32x4>(sema_context()); }
DECL_SEMA(PSUBQ) { return vector_op<ir::op::sub, ir::type::i64x2>(sema_context()); }

// Scalar compare.
//
template<ir::type TyVec, ir::type TyPiece>
static diag::lazy scalar_compare(SemaContext) {
	ir::variant src;
	if (ins.op[1].type == arch::mop_type::reg) {
		auto srcv = read(sema_context(), 1, TyVec);
		src		 = bb->push_extract(TyPiece, std::move(srcv), 0);
	} else {
		src = read(sema_context(), 1, TyPiece);
	}

	auto dst	  = read(sema_context(), 0, TyVec);
	auto piece = bb->push_extract(TyPiece, dst, 0);

	// PF = Unordered
	bb->push_write_reg(reg::flag_pf, bb->push_poison(ir::type::i1, "Unordered comparison NYI"));	 // TODO
	// ZF = EQ || Unordered
	bb->push_write_reg(reg::flag_zf, bb->push_cmp(ir::op::eq, piece, src));
	// CF = LT || Unordered
	bb->push_write_reg(reg::flag_cf, bb->push_cmp(ir::op::lt, piece, src));
	// OF AF SF = 0
	bb->push_write_reg(reg::flag_af, false);
	bb->push_write_reg(reg::flag_of, false);
	bb->push_write_reg(reg::flag_sf, false);
	return diag::ok;
}
DECL_SEMA(COMISD) { return scalar_compare<ir::type::f32x4, ir::type::f32>(sema_context()); }
DECL_SEMA(COMISS) { return scalar_compare<ir::type::f64x2, ir::type::f64>(sema_context()); }

// TODO:
// RCP
// RSQRT
// CVT... DQ/PI/PS/SD/SI/SS/TPD/TPS/TSD/TSS
// ROUND
// SHUFPD
// BROADCAST
// P...