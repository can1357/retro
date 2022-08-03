#include <retro/arch/x86/sema.hpp>
#include <retro/common.hpp>

using namespace retro;
using namespace retro::arch::x86;

// TODO: Special handling:
//  lock or [rsp], 0

template<auto Operation>
static diag::lazy logical(SemaContext) {
	auto ty	= ir::int_type(ins.effective_width);
	auto rhs = read(sema_context(), 1, ty);

	ir::insn*	result;
	ir::variant lhs;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto [ptr, seg] = agen(sema_context(), ins.op[0].m, true);
		lhs				 = bb->push_atomic_binop(Operation, seg, std::move(ptr), rhs);
		result			 = bb->push_binop(Operation, lhs, rhs);
	} else {
		lhs	 = read(sema_context(), 0, ty);
		result = bb->push_binop(Operation, lhs, rhs);
		write(sema_context(), 0, result);
	}

	set_flags_logical(bb, result);
	return diag::ok;
}
DECL_SEMA(OR) { return logical<ir::op::bit_or>(sema_context()); }
DECL_SEMA(AND) { return logical<ir::op::bit_and>(sema_context()); }
DECL_SEMA(XOR) {
	// Pattern: [xor reg, reg] <=> [mov reg, 0] + flags update
	if (ins.op[0].type == arch::mop_type::reg && ins.op[1].type == arch::mop_type::reg) {
		if (ins.op[0].r == ins.op[1].r) {
			bb->push_write_reg(reg::flag_sf, false);
			bb->push_write_reg(reg::flag_zf, true);
			bb->push_write_reg(reg::flag_pf, true);
			bb->push_write_reg(reg::flag_of, false);
			bb->push_write_reg(reg::flag_cf, false);
			bb->push_write_reg(reg::flag_af, false);
			write_reg(sema_context(), ins.op[0].r, ir::constant(ir::int_type(ins.effective_width), 0));
			return diag::ok;
		}
	}
	return logical<ir::op::bit_xor>(sema_context());
}
DECL_SEMA(NOT) {
	auto ty = ir::int_type(ins.effective_width);

	ir::insn*	result;
	ir::variant lhs;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto [ptr, seg] = agen(sema_context(), ins.op[0].m, true);
		lhs				 = bb->push_atomic_unop(ty, ir::op::bit_not, seg, std::move(ptr));
		result			 = bb->push_unop(ir::op::bit_not, lhs);
	} else {
		lhs	 = read(sema_context(), 0, ty);
		result = bb->push_unop(ir::op::bit_not, lhs);
		write(sema_context(), 0, result);
	}
	return diag::ok;
}

template<auto Operation>
static diag::lazy shift(SemaContext) {
	auto ty		= ir::int_type(ins.effective_width);
	auto rhs		= bb->push_cast(ty, read(sema_context(), 1, ir::type::i8));
	auto lhs		= read(sema_context(), 0, ty);
	auto result = bb->push_binop(Operation, lhs, rhs);
	write(sema_context(), 0, result);

	/*
	The CF flag contains the value of the last bit shifted out of the destination operand; it is undefined for SHL and SHR instructions where the count is greater than or equal to
	the size (in bits) of the destination operand.
	The OF flag is affected only for 1-bit shifts (see “Description” above); otherwise, it is undefined.
	*/
	/*
	The SF, ZF, and PF flags are
	set according to the result. If the count is 0, the flags are not affected. For a non-zero count, the AF flag is undefined.
	*/
	bb->push_write_reg(reg::flag_cf, bb->push_poison(ir::type::i1, "Shift - Carry flag NYI"));	  // TODO
	bb->push_write_reg(reg::flag_of, bb->push_poison(ir::type::i1, "Shift - Overflow flag NYI"));  // TODO
	bb->push_write_reg(reg::flag_sf, bb->push_poison(ir::type::i1, "Shift - Sign flag NYI"));		  // TODO
	bb->push_write_reg(reg::flag_zf, bb->push_poison(ir::type::i1, "Shift - Zero flag NYI"));		  // TODO
	bb->push_write_reg(reg::flag_pf, bb->push_poison(ir::type::i1, "Shift - Parity flag NYI"));	  // TODO
	return diag::ok;
}
template<auto Operation>
static diag::lazy rot(SemaContext) {
	auto ty		= ir::int_type(ins.effective_width);
	auto rhs		= bb->push_cast(ty, read(sema_context(), 1, ir::type::i8));
	auto lhs		= read(sema_context(), 0, ty);
	auto result = bb->push_binop(Operation, lhs, rhs);
	write(sema_context(), 0, result);

	/*
	If the masked count is 0, the flags are not affected. If the masked count is 1, then the OF flag is affected, otherwise (masked count is greater than 1) the OF flag is
	undefined. The CF flag is affected when the masked count is nonzero. The SF, ZF, AF, and PF flags are always unaffected.
	*/
	bb->push_write_reg(reg::flag_cf, bb->push_poison(ir::type::i1, "Rotate - Carry flag NYI"));		// TODO
	bb->push_write_reg(reg::flag_of, bb->push_poison(ir::type::i1, "Rotate - Overflow flag NYI"));	// TODO
	return diag::ok;
}
DECL_SEMA(SHR) { return shift<ir::op::bit_shr>(sema_context()); }
DECL_SEMA(SHL) { return shift<ir::op::bit_shl>(sema_context()); }
DECL_SEMA(SAR) { return shift<ir::op::bit_sar>(sema_context()); }
DECL_SEMA(ROR) { return shift<ir::op::bit_ror>(sema_context()); }
DECL_SEMA(ROL) { return shift<ir::op::bit_rol>(sema_context()); }
// TODO: RCL, RCR, SHRD, SHLD.
//       SHLX, SHRX, SARX, RORX.

DECL_SEMA(TEST) {
	auto ty		= ir::int_type(ins.effective_width);
	auto lhs		= read(sema_context(), 0, ty);
	auto rhs		= read(sema_context(), 1, ty);
	auto result = bb->push_binop(ir::op::bit_and, lhs, rhs);
	set_flags_logical(bb, result);
	return diag::ok;
}

// Bit operations.
//
template<auto Operation>
static diag::lazy do_bt_rmw(SemaContext) {
	auto ty	= ir::int_type(ins.effective_width);

	// count = rhs & mask
	auto count = bb->push_binop(ir::op::bit_and, read(sema_context(), 1, ty), ir::constant(ty, ins.effective_width - 1));
	// mask =  1 << count
	auto rhs = bb->push_binop(ir::op::bit_shl, ir::constant(ty, 1), count);

	ir::insn*	result;
	ir::variant lhs;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto [ptr, seg] = agen(sema_context(), ins.op[0].m, true);
		lhs				 = bb->push_atomic_binop(Operation, seg, std::move(ptr), rhs);
		result			 = bb->push_binop(Operation, lhs, rhs);
	} else {
		lhs	 = read(sema_context(), 0, ty);
		result = bb->push_binop(Operation, lhs, rhs);
		write(sema_context(), 0, result);
	}

	// The CF flag contains the value of the selected bit before it is set. The ZF flag is unaffected. The OF, SF, AF, and PF flags are undefined.
	// Runtime behaviour: Rest are unchanged.
	// tmp = lhs >> count
	auto tmp = bb->push_binop(ir::op::bit_shr, lhs, count);
	// cf =  tmp & 1
	auto cf = bb->push_cast(ir::type::i1, tmp);
	bb->push_write_reg(reg::flag_cf, cf);
	return diag::ok;
}

DECL_SEMA(BTS) {
	auto ty = ir::int_type(ins.effective_width);

	// count = rhs & mask
	auto count = bb->push_binop(ir::op::bit_and, read(sema_context(), 1, ty), ir::constant(ty, ins.effective_width - 1));
	// mask =  1 << count
	auto rhs = bb->push_binop(ir::op::bit_shl, ir::constant(ty, 1), count);

	// lhs := lhs | mask
	ir::insn*	result;
	ir::variant lhs;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto [ptr, seg] = agen(sema_context(), ins.op[0].m, true);
		lhs				 = bb->push_atomic_binop(ir::op::bit_or, seg, std::move(ptr), rhs);
		result			 = bb->push_binop(ir::op::bit_or, lhs, rhs);
	} else {
		lhs	 = read(sema_context(), 0, ty);
		result = bb->push_binop(ir::op::bit_or, lhs, rhs);
		write(sema_context(), 0, result);
	}

	// The CF flag contains the value of the selected bit before it is set. The ZF flag is unaffected. The OF, SF, AF, and PF flags are undefined.
	// Runtime behaviour: Rest are unchanged.
	// tmp = lhs >> count
	auto tmp = bb->push_binop(ir::op::bit_shr, lhs, count);
	// cf =  tmp & 1
	auto cf = bb->push_cast(ir::type::i1, tmp);
	bb->push_write_reg(reg::flag_cf, cf);
	return diag::ok;
}
DECL_SEMA(BTR) {
	auto ty = ir::int_type(ins.effective_width);

	// count = rhs & mask
	auto count = bb->push_binop(ir::op::bit_and, read(sema_context(), 1, ty), ir::constant(ty, ins.effective_width - 1));
	// mask =  1 << count
	auto rhs = bb->push_binop(ir::op::bit_shl, ir::constant(ty, 1), count);
	// mask =  ~mask

	// lhs := lhs & mask
	ir::insn*	result;
	ir::variant lhs;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto [ptr, seg] = agen(sema_context(), ins.op[0].m, true);
		lhs				 = bb->push_atomic_binop(ir::op::bit_and, seg, std::move(ptr), rhs);
		result			 = bb->push_binop(ir::op::bit_and, lhs, rhs);
	} else {
		lhs	 = read(sema_context(), 0, ty);
		result = bb->push_binop(ir::op::bit_and, lhs, rhs);
		write(sema_context(), 0, result);
	}

	// The CF flag contains the value of the selected bit before it is cleared. The ZF flag is unaffected. The OF, SF, AF, and PF flags are undefined.
	// Runtime behaviour: Rest are unchanged.
	// tmp = lhs >> count
	auto tmp = bb->push_binop(ir::op::bit_shr, lhs, count);
	// cf =  tmp & 1
	auto cf = bb->push_cast(ir::type::i1, tmp);
	bb->push_write_reg(reg::flag_cf, cf);
	return diag::ok;
}
DECL_SEMA(BTC) {
	auto ty = ir::int_type(ins.effective_width);

	// count = rhs & mask
	auto count = bb->push_binop(ir::op::bit_and, read(sema_context(), 1, ty), ir::constant(ty, ins.effective_width - 1));
	// mask =  1 << count
	auto rhs = bb->push_binop(ir::op::bit_shl, ir::constant(ty, 1), count);

	// lhs := lhs ^ mask
	ir::insn*	result;
	ir::variant lhs;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto [ptr, seg] = agen(sema_context(), ins.op[0].m, true);
		lhs				 = bb->push_atomic_binop(ir::op::bit_xor, seg, std::move(ptr), rhs);
		result			 = bb->push_binop(ir::op::bit_xor, lhs, rhs);
	} else {
		lhs	 = read(sema_context(), 0, ty);
		result = bb->push_binop(ir::op::bit_xor, lhs, rhs);
		write(sema_context(), 0, result);
	}

	// The CF flag contains the value of the selected bit before it is complemented. The ZF flag is unaffected. The OF, SF, AF, and PF flags are undefined.
	// Runtime behaviour: Rest are unchanged.
	// tmp = lhs >> count
	auto tmp = bb->push_binop(ir::op::bit_shr, lhs, count);
	// cf =  tmp & 1
	auto cf = bb->push_cast(ir::type::i1, tmp);
	bb->push_write_reg(reg::flag_cf, cf);
	return diag::ok;
}
DECL_SEMA(BT) {
	auto ty = ir::int_type(ins.effective_width);

	// count = rhs & mask
	auto count = bb->push_binop(ir::op::bit_and, read(sema_context(), 1, ty), ir::constant(ty, ins.effective_width - 1));

	auto lhs = read(sema_context(), 0, ty);

	// The CF flag contains the value of the selected bit. The ZF flag is unaffected. The OF, SF, AF, and PF flags are undefined.
	// Runtime behaviour: Rest are unchanged.
	// tmp = lhs >> count
	auto tmp = bb->push_binop(ir::op::bit_shr, lhs, count);
	// cf =  tmp & 1
	auto cf = bb->push_cast(ir::type::i1, tmp);
	bb->push_write_reg(reg::flag_cf, cf);
	return diag::ok;
}
DECL_SEMA(BSWAP) {
	auto ty = ir::int_type(ins.effective_width);
	if (ins.effective_width == 64 || ins.effective_width == 32) {
		auto val = read(sema_context(), 0, ty);
		auto tmp = bb->push_unop(ir::op::bit_byteswap, val);
		write(sema_context(), 0, tmp);
	} else {
		// "Undefined behavior", actually writes 0 during runtime.
		write(sema_context(), 0, ir::constant(ty, 0));
	}
	return diag::ok;
}