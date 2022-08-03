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
	set_cf_add(bb, lhs, rhs, result);
	set_of_add(bb, std::move(lhs), std::move(rhs), result);
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
	set_cf_add(bb, lhs, rhs, result);
	set_of_add(bb, std::move(lhs), std::move(rhs), result);
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
	set_cf_sub(bb, lhs, rhs);
	set_of_sub(bb, std::move(lhs), std::move(rhs), result);
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
	set_of_add(bb, std::move(lhs), std::move(rhs), result);
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
	set_of_sub(bb, std::move(lhs), std::move(rhs), result);
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
	set_of_sub(bb, ir::constant(ty, 0), lhs, result);
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
	set_cf_sub(bb, lhs, rhs);
	set_of_sub(bb, std::move(lhs), std::move(rhs), result);
	return diag::ok;
}

// Division and mulitplication.
//
static std::array<arch::mreg, 3> select_mul_regs(u16 w) {
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

	auto [lhs, outlo, outhi] = select_mul_regs(ins.effective_width);

	auto lhsx	  = bb->push_cast(tyx, read_reg(sema_context(), lhs, ty));
	auto rhsx	  = bb->push_cast(tyx, rhs);
	auto resultx  = bb->push_binop(ir::op::mul, lhsx, rhsx);
	auto resulthi = bb->push_cast(ty, bb->push_binop(ir::op::bit_shr, resultx, ir::constant(tyx, ins.effective_width)));

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
	auto resultx  = bb->push_binop(ir::op::mul, lhsx, rhsx);
	auto resulthi = bb->push_cast(ty, bb->push_binop(ir::op::bit_shr, resultx, ir::constant(tyx, ins.effective_width)));

	write(sema_context(), 1, bb->push_cast(ty, resultx));
	write(sema_context(), 0, resulthi);
	return diag::ok;
}

static diag::lazy imul_1op(SemaContext) {
	auto ty	= ir::int_type(ins.effective_width);
	auto tyx = ir::int_type(ins.effective_width * 2);
	auto rhs = read(sema_context(), 0, ty);

	auto [lhs, outlo, outhi] = select_mul_regs(ins.effective_width);

	auto lhsx	  = bb->push_cast_sx(tyx, read_reg(sema_context(), lhs, ty));
	auto rhsx	  = bb->push_cast_sx(tyx, rhs);
	auto resultx  = bb->push_binop(ir::op::mul, lhsx, rhsx);
	auto resulthi = bb->push_cast(ty, bb->push_binop(ir::op::bit_shr, resultx, ir::constant(tyx, ins.effective_width)));

	if (!outhi) {
		write_reg(sema_context(), outlo, resultx);
	} else {
		write_reg(sema_context(), outlo, bb->push_cast(ty, resultx));
		write_reg(sema_context(), outhi, resulthi);
	}
	
	// For the one operand form of the instruction, the CF and OF flags are set when significant bits are carried into the upper half of the result and cleared when the result fits exactly in the lower half of the result.
	// The OF and CF flags are set to 0 if the upper half of the result is 0; otherwise, they are set to 1. The SF, ZF, AF, and PF flags are undefined.
	auto of = bb->push_cmp(ir::op::ne, resultx, bb->push_cast_sx(tyx, bb->push_cast(ty, resultx)));
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

	auto lhsx	 = bb->push_cast_sx(tyx, lhs);
	auto rhsx	 = bb->push_cast_sx(tyx, rhs);
	auto resultx = bb->push_binop(ir::op::mul, lhsx, rhsx);
	write(sema_context(), 0, bb->push_cast(ty, resultx));

	// For the two- and three-operand forms of the instruction, the CF and OF flags are set when the result must be truncated to fit in the destination operand size and cleared when
	// the result fits exactly in the destination operand size. The SF, ZF, AF, and PF flags are undefined.
	//
	auto of = bb->push_cmp(ir::op::ne, resultx, bb->push_cast_sx(tyx, bb->push_cast(ty, resultx)));
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


static std::array<arch::mreg, 4> select_div_regs(u16 w) {
	arch::mreg inlo = {};
	arch::mreg inhi = {};
	arch::mreg outq = {};
	arch::mreg outr = {};
	switch (w) {
		case 8: {
			// AL&AH <| AX /% OP1
			outq	= reg::al;
			outr	= reg::ah;
			inlo	= reg::ax;
			break;
		}
		case 16: {
			// AX&DX <| DX:AX /% OP1
			outq = reg::ax;
			outr = reg::dx;
			inlo = reg::ax;
			inhi = reg::dx;
			break;
		}
		case 32: {
			// EAX&EDX <| EDX:EAX /% OP1
			outq = reg::eax;
			outr = reg::edx;
			inlo = reg::eax;
			inhi = reg::edx;
			break;
		}
		case 64: {
			// RAX&RDX <| RDX:RAX /% OP1
			outq = reg::rax;
			outr = reg::rdx;
			inlo = reg::rax;
			inhi = reg::rdx;
			break;
		}
		default:
			RC_UNREACHABLE();
	}
	return {inlo, inhi, outq, outr};
}
DECL_SEMA(IDIV) {
	auto ty	= ir::int_type(ins.effective_width);
	auto tyx = ir::int_type(ins.effective_width * 2);

	auto [inlo, inhi, outq, outr] = select_div_regs(ins.effective_width);

	// Read RHS.
	//
	auto rhs	 = read(sema_context(), 0, ty);
	auto rhsx = bb->push_cast_sx(tyx, rhs);

	// Read LHS.
	//
	ir::insn* lhsx;
	if (inhi) {
		auto lhslo = bb->push_cast(tyx, read_reg(sema_context(), inlo, ty));
		auto lhshi = bb->push_cast(tyx, read_reg(sema_context(), inhi, ty));
		lhshi		  = bb->push_binop(ir::op::bit_shl, lhshi, ir::constant(tyx, ins.effective_width));
		lhsx		  = bb->push_binop(ir::op::bit_or, lhshi, lhslo);
	} else {
		lhsx = read_reg(sema_context(), inlo, tyx);
	}

	// Compute result and write.
	//
	auto rq = bb->push_binop(ir::op::div, lhsx, rhsx);
	auto rr = bb->push_binop(ir::op::rem, lhsx, rhsx);

	write_reg(sema_context(), outq, bb->push_cast(ty, rq));
	write_reg(sema_context(), outr, bb->push_cast(ty, rr));
	return diag::ok;
}
DECL_SEMA(DIV) {
	auto ty	= ir::int_type(ins.effective_width);
	auto tyx = ir::int_type(ins.effective_width * 2);

	auto [inlo, inhi, outq, outr] = select_div_regs(ins.effective_width);

	// Read RHS.
	//
	auto rhs	 = read(sema_context(), 0, ty);
	auto rhsx = bb->push_cast(tyx, rhs);

	// Read LHS.
	//
	ir::insn* lhsx;
	if (inhi) {
		auto lhslo = bb->push_cast(tyx, read_reg(sema_context(), inlo, ty));
		auto lhshi = bb->push_cast(tyx, read_reg(sema_context(), inhi, ty));
		lhshi		  = bb->push_binop(ir::op::bit_shl, lhshi, ir::constant(tyx, ins.effective_width));
		lhsx		  = bb->push_binop(ir::op::bit_or, lhshi, lhslo);
	} else {
		lhsx = read_reg(sema_context(), inlo, tyx);
	}

	// Compute result and write.
	//
	auto rq = bb->push_binop(ir::op::udiv, lhsx, rhsx);
	auto rr = bb->push_binop(ir::op::urem, lhsx, rhsx);

	write_reg(sema_context(), outq, bb->push_cast(ty, rq));
	write_reg(sema_context(), outr, bb->push_cast(ty, rr));
	return diag::ok;
}