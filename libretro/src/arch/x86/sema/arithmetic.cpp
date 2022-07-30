#include <retro/arch/x86/sema.hpp>
#include <retro/common.hpp>

using namespace retro;
using namespace retro::arch::x86;

// Addition and subtraction.
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

	ir::insn*	result;
	ir::variant lhs;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto [ptr, seg] = agen(sema_context(), ins.op[0].m, true);
		auto lhsi		 = bb->push_atomic_binop(ir::op::add, seg, std::move(ptr), rhs);
		result			 = bb->push_binop(ir::op::add, lhsi, rhs);
		write(sema_context(), 1, lhsi);
		lhs = lhsi;
	} else {
		lhs	 = read(sema_context(), 0, ty);
		result = bb->push_binop(ir::op::add, lhs, rhs);
		write(sema_context(), 0, result);
		write(sema_context(), 1, ir::variant{lhs});
	}

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

// Division and mulitplication.
//
static std::array<arch::mreg, 3> select_muldiv_regs(u16 w) {
	arch::mreg lhs	  = {};
	arch::mreg outlo = {};
	arch::mreg outhi = {};
	switch (w) {
		case 8: {
			// AX <| AL * OP1
			lhs	= reg::al;
			outlo = reg::ax;
			break;
		}
		case 16: {
			// DX:AX <| AX * OP1
			lhs	= reg::ax;
			outlo = reg::ax;
			outhi = reg::dx;
			break;
		}
		case 32: {
			// EDX:EAX <| EAX * OP1
			lhs	= reg::eax;
			outlo = reg::eax;
			outhi = reg::edx;
			break;
		}
		case 64: {
			// RDX:RAX <| RAX * OP1
			lhs	= reg::rax;
			outlo = reg::rax;
			outhi = reg::rdx;
			break;
		}
		default:
			RC_UNREACHABLE();
	}
	return {lhs, outlo, outhi};
}

DECL_SEMA(MUL) {
	auto ty	= ir::int_type(ins.effective_width);
	auto tyx = ir::int_type(ins.effective_width * 2);
	auto rhs = read(sema_context(), 0, ty);

	auto [lhs, outlo, outhi] = select_muldiv_regs(ins.effective_width);

	auto lhsx	  = bb->push_cast(tyx, read_reg(sema_context(), lhs, ty));
	auto rhsx	  = bb->push_cast(tyx, rhs);
	auto resultx  = bb->push_binop(ir::op::umul, lhsx, rhsx);
	auto resulthi = bb->push_cast(ty, bb->push_binop(ir::op::bit_shr, resultx, ir::constant(tyx, 1)));

	if (!outhi) {
		write_reg(sema_context(), outlo, resultx);
	} else {
		write_reg(sema_context(), outlo, bb->push_cast(ty, resultx));
		write_reg(sema_context(), outhi, resulthi);
	}

	// The OF and CF flags are set to 0 if the upper half of the result is 0; otherwise, they are set to 1. The SF, ZF, AF, and PF flags are undefined.
	auto of = bb->push_cmp(ir::op::ne, resulthi, ir::constant(ty, 0));
	bb->push_write_reg(reg::flag_of, of);
	bb->push_write_reg(reg::flag_cf, of);
	return diag::ok;
}
DECL_SEMA(MULX) {
	auto ty	= ir::int_type(ins.effective_width);
	auto tyx = ir::int_type(ins.effective_width * 2);

	auto		  rhs = read(sema_context(), 2, ty);
	arch::mreg lhs = ins.effective_width == 32 ? reg::edx : reg::rdx;

	auto lhsx	  = bb->push_cast(tyx, read_reg(sema_context(), lhs, ty));
	auto rhsx	  = bb->push_cast(tyx, rhs);
	auto resultx  = bb->push_binop(ir::op::umul, lhsx, rhsx);
	auto resulthi = bb->push_cast(ty, bb->push_binop(ir::op::bit_shr, resultx, ir::constant(tyx, 1)));

	write(sema_context(), 1, bb->push_cast(ty, resultx));
	write(sema_context(), 0, resulthi);
	return diag::ok;
}

static diag::lazy imul_1op(SemaContext) {
	auto ty	= ir::int_type(ins.effective_width);
	auto tyx = ir::int_type(ins.effective_width * 2);
	auto rhs = read(sema_context(), 0, ty);

	auto [lhs, outlo, outhi] = select_muldiv_regs(ins.effective_width);

	auto lhsx	  = bb->push_cast(tyx, read_reg(sema_context(), lhs, ty));
	auto rhsx	  = bb->push_cast(tyx, rhs);
	auto resultx  = bb->push_binop(ir::op::mul, lhsx, rhsx);
	auto resulthi = bb->push_cast(ty, bb->push_binop(ir::op::bit_shr, resultx, ir::constant(tyx, 1)));

	if (!outhi) {
		write_reg(sema_context(), outlo, resultx);
	} else {
		write_reg(sema_context(), outlo, bb->push_cast(ty, resultx));
		write_reg(sema_context(), outhi, resulthi);
	}
	
	// For the one operand form of the instruction, the CF and OF flags are set when significant bits are carried into the upper half of the result and cleared when the result fits exactly in the lower half of the result.
	// The OF and CF flags are set to 0 if the upper half of the result is 0; otherwise, they are set to 1. The SF, ZF, AF, and PF flags are undefined.
	auto of = bb->push_cmp(ir::op::ne, resultx, bb->push_sign_extend(tyx, bb->push_cast(ty, resultx)));
	bb->push_write_reg(reg::flag_of, of);
	bb->push_write_reg(reg::flag_cf, of);
	return diag::ok;
}
static diag::lazy imul_2_3op(SemaContext) {
	auto ty	= ir::int_type(ins.effective_width);
	auto tyx = ir::int_type(ins.effective_width * 2);

	ir::variant lhs, rhs;
	if (ins.operand_count == 2) {
		lhs = read(sema_context(), 0, ty);
		rhs = read(sema_context(), 1, ty);
	} else {
		lhs = read(sema_context(), 1, ty);
		rhs = read(sema_context(), 2, ty);
	}

	auto lhsx	  = bb->push_cast(tyx, lhs);
	auto rhsx	  = bb->push_cast(tyx, rhs);
	auto resultx  = bb->push_binop(ir::op::mul, lhsx, rhsx);
	write(sema_context(), 0, bb->push_cast(ty, resultx));

	// For the two- and three-operand forms of the instruction, the CF and OF flags are set when the result must be truncated to fit in the destination operand size and cleared when
	// the result fits exactly in the destination operand size. The SF, ZF, AF, and PF flags are undefined.
	//
	auto of = bb->push_cmp(ir::op::ne, resultx, bb->push_sign_extend(tyx, bb->push_cast(ty, resultx)));
	bb->push_write_reg(reg::flag_of, of);
	bb->push_write_reg(reg::flag_cf, of);
	return diag::ok;
}
DECL_SEMA(IMUL) {
	if (ins.operand_count == 1) {
		return imul_1op(sema_context());
	} else {
		return imul_2_3op(sema_context());
	}
}