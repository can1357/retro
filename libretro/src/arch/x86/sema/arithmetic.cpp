#include <retro/arch/x86/sema.hpp>
#include <retro/common.hpp>

using namespace retro;
using namespace retro::arch::x86;

// Arithmetic.
//
DECL_SEMA(LEA) {
	// Pattern: [lea reg, [reg]] <=> [nop]
	if (ins.op[0].type == arch::mop_type::reg && !ins.op[1].m.index && ins.op[1].m.disp) {
		if (ins.op[0].r == ins.op[1].m.base) {
			return diag::ok;
		}
	}
	auto [ptr, seg] = agen(sema_context(), ins.op[1].m, false);
	write_reg(sema_context(), ins.op[0].r, std::move(ptr));
	return diag::ok;
}
DECL_SEMA(ADD) {
	auto ty	= ir::int_type(ins.effective_width);
	auto rhs = read(sema_context(), 1, ty);

	ir::insn*	result;
	ir::variant lhs;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto [ptr, seg] = agen(sema_context(), ins.op[0].m, true);
		lhs				 = bb->push_atomic_binop(ir::op::add, seg, std::move(ptr), rhs);
		result			 = bb->push_binop(ir::op::add, lhs, rhs);
	} else {
		lhs	 = read(sema_context(), 0, ty);
		result = bb->push_binop(ir::op::add, lhs, rhs);
		write(sema_context(), 0, result);
	}

	set_af(bb, lhs, rhs, result);
	set_sf(bb, result);
	set_zf(bb, result);
	set_pf(bb, result);

	bb->push_write_reg(reg::flag_of, bb->push_test_overflow(ir::op::add, lhs, rhs));
	bb->push_write_reg(reg::flag_cf, bb->push_test_overflow_u(ir::op::add, lhs, rhs));
	return diag::ok;
}
DECL_SEMA(XADD) {
	auto ty	= ir::int_type(ins.effective_width);
	auto rhs = read(sema_context(), 1, ty);

	auto [ptr, seg] = agen(sema_context(), ins.op[0].m, true);
	auto lhs			 = bb->push_atomic_binop(ir::op::add, seg, std::move(ptr), rhs);
	auto result		 = bb->push_binop(ir::op::add, lhs, rhs);
	write(sema_context(), 1, lhs);

	set_af(bb, lhs, rhs, result);
	set_sf(bb, result);
	set_zf(bb, result);
	set_pf(bb, result);
	bb->push_write_reg(reg::flag_of, bb->push_test_overflow(ir::op::add, lhs, rhs));
	bb->push_write_reg(reg::flag_cf, bb->push_test_overflow_u(ir::op::add, lhs, rhs));
	return diag::ok;
}
DECL_SEMA(SUB) {
	auto ty	= ir::int_type(ins.effective_width);
	auto rhs = read(sema_context(), 1, ty);

	ir::insn*	result;
	ir::variant lhs;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto [ptr, seg] = agen(sema_context(), ins.op[0].m, true);
		lhs				 = bb->push_atomic_binop(ir::op::sub, seg, std::move(ptr), rhs);
		result			 = bb->push_binop(ir::op::sub, lhs, rhs);
	} else {
		lhs	 = read(sema_context(), 0, ty);
		result = bb->push_binop(ir::op::sub, lhs, rhs);
		write(sema_context(), 0, result);
	}

	set_af(bb, lhs, rhs, result);
	set_sf(bb, result);
	set_zf(bb, result);
	set_pf(bb, result);
	bb->push_write_reg(reg::flag_of, bb->push_test_overflow(ir::op::sub, lhs, rhs));
	bb->push_write_reg(reg::flag_cf, bb->push_test_overflow_u(ir::op::sub, lhs, rhs));
	return diag::ok;
}
DECL_SEMA(INC) {
	auto ty	= ir::int_type(ins.effective_width);
	auto rhs = ir::constant(ty, 1);

	ir::insn*	result;
	ir::variant lhs;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto [ptr, seg] = agen(sema_context(), ins.op[0].m, true);
		lhs				 = bb->push_atomic_binop(ir::op::add, seg, std::move(ptr), rhs);
		result			 = bb->push_binop(ir::op::add, lhs, rhs);
	} else {
		lhs	 = read(sema_context(), 0, ty);
		result = bb->push_binop(ir::op::add, lhs, rhs);
		write(sema_context(), 0, result);
	}

	set_af(bb, lhs, rhs, result);
	set_sf(bb, result);
	set_zf(bb, result);
	set_pf(bb, result);
	bb->push_write_reg(reg::flag_of, bb->push_test_overflow(ir::op::add, lhs, rhs));
	return diag::ok;
}
DECL_SEMA(DEC) {
	auto ty	= ir::int_type(ins.effective_width);
	auto rhs = ir::constant(ty, 1);

	ir::insn*	result;
	ir::variant lhs;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto [ptr, seg] = agen(sema_context(), ins.op[0].m, true);
		lhs				 = bb->push_atomic_binop(ir::op::sub, seg, std::move(ptr), rhs);
		result			 = bb->push_binop(ir::op::sub, lhs, rhs);
	} else {
		lhs	 = read(sema_context(), 0, ty);
		result = bb->push_binop(ir::op::sub, lhs, rhs);
		write(sema_context(), 0, result);
	}

	set_af(bb, lhs, rhs, result);
	set_sf(bb, result);
	set_zf(bb, result);
	set_pf(bb, result);
	bb->push_write_reg(reg::flag_of, bb->push_test_overflow(ir::op::sub, lhs, rhs));
	return diag::ok;
}
DECL_SEMA(NEG) {
	auto ty = ir::int_type(ins.effective_width);

	ir::insn*	result;
	ir::variant lhs;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto [ptr, seg] = agen(sema_context(), ins.op[0].m, true);
		lhs				 = bb->push_atomic_unop(ty, ir::op::neg, seg, std::move(ptr));
		result			 = bb->push_unop(ir::op::neg, lhs);
	} else {
		lhs	 = read(sema_context(), 0, ty);
		result = bb->push_unop(ir::op::neg, lhs);
		write(sema_context(), 0, result);
	}

	set_af(bb, lhs, result);
	set_sf(bb, result);
	set_zf(bb, result);
	set_pf(bb, result);
	bb->push_write_reg(reg::flag_cf, bb->push_cmp(ir::op::ne, lhs, ir::constant(ty, 0)));
	bb->push_write_reg(reg::flag_of, bb->push_test_overflow(ir::op::sub, ir::constant(ty, 0), lhs));
	return diag::ok;
}
DECL_SEMA(CMP) {
	auto ty		= ir::int_type(ins.effective_width);
	auto lhs		= read(sema_context(), 0, ty);
	auto rhs		= read(sema_context(), 1, ty);
	auto result = bb->push_binop(ir::op::sub, lhs, rhs);
	set_af(bb, lhs, rhs, result);
	set_sf(bb, result);
	set_zf(bb, result);
	set_pf(bb, result);
	bb->push_write_reg(reg::flag_of, bb->push_test_overflow(ir::op::sub, lhs, rhs));
	bb->push_write_reg(reg::flag_cf, bb->push_test_overflow_u(ir::op::sub, lhs, rhs));
	return diag::ok;
}