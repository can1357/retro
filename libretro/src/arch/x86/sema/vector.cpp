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
			write_reg(sema_context(), ins.op[0].r, ir::constant(i8x16{0}));
			return diag::ok;
		}
	}

	auto lhs = read(sema_context(), 0, ir::type::i8x16);
	auto rhs = read(sema_context(), 1, ir::type::i8x16);
	auto res = bb->push_binop(ir::op::bit_xor, lhs, rhs);
	write(sema_context(), 0, res);
	return diag::ok;
}
DECL_SEMA(XORPD) { return lift_XORPS(sema_context()); }
DECL_SEMA(PXOR) { return lift_XORPS(sema_context()); }

DECL_SEMA(ANDPS) {
	auto lhs = read(sema_context(), 0, ir::type::i8x16);
	auto rhs = read(sema_context(), 1, ir::type::i8x16);
	auto res = bb->push_binop(ir::op::bit_and, lhs, rhs);
	write(sema_context(), 0, res);
	return diag::ok;
}
DECL_SEMA(ANDPD) { return lift_ANDPS(sema_context()); }
DECL_SEMA(PAND) { return lift_ANDPS(sema_context()); }

DECL_SEMA(ORPS) {
	auto lhs = read(sema_context(), 0, ir::type::i8x16);
	auto rhs = read(sema_context(), 1, ir::type::i8x16);
	auto res = bb->push_binop(ir::op::bit_or, lhs, rhs);
	write(sema_context(), 0, res);
	return diag::ok;
}
DECL_SEMA(ORPD) { return lift_ORPS(sema_context()); }
DECL_SEMA(POR) { return lift_ORPS(sema_context()); }

// Vector moves.
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

/*
movq -> 14 0.066238%
movd -> 5 0.023656%
movss -> 9 0.042581%
divss -> 1 0.004731%
cvtdq2pd -> 2 0.009463%
cvtdq2ps -> 2 0.009463%
cvtps2pd -> 2 0.009463%
cvtsi2ss -> 6 0.028388%
cvtsi2sd -> 4 0.018925%
addsd -> 1 0.004731%
addss -> 1 0.004731%
psrldq -> 6 0.028388%
*/