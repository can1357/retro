#include <retro/arch/x86/sema.hpp>
#include <retro/common.hpp>

using namespace retro;
using namespace retro::arch::x86;

// Simple data transfer.
//
DECL_SEMA(MOV) {
	// Pattern: [mov reg, reg] <=> [nop]
	if (ins.op[0].type == arch::mop_type::reg && ins.op[1].type == arch::mop_type::reg) {
		if (ins.op[0].r == ins.op[1].r) {
			// Exception: mov eax, eax in long mode
			if (ins.op[0].r.get_kind() != arch::reg_kind::gpr32 || !mach->is_64()) {
				bb->push_nop();
				return diag::ok;
			}
		}
	}

	// Write segment reg.
	//
	if (ins.op[0].type == arch::mop_type::reg && ins.op[0].r.get_kind() == arch::reg_kind::segment) {
		auto ty		= ir::int_type(ins.op[1].get_width());
		auto val = read(sema_context(), 1, ty);
		if (ty != ir::type::i16) {
			val = bb->push_cast(ir::type::i16, val);
		}
		auto intrin = (ir::intrinsic)(ins.op[0].r.id - (u32) reg::es + (u32) ir::intrinsic::ia32_setes);
		bb->push_sideeffect_intrinsic(intrin, val);
		return diag::ok;
	}
	// Read segment reg.
	//
	else if (ins.op[1].type == arch::mop_type::reg && ins.op[1].r.get_kind() == arch::reg_kind::segment) {
		auto ty = ir::int_type(ins.op[0].get_width());
		auto intrin = (ir::intrinsic)(ins.op[1].r.id - (u32) reg::es + (u32) ir::intrinsic::ia32_getes);
		auto res = bb->push_extract(ir::type::i16, bb->push_intrinsic(intrin), 0);
		if (ty != ir::type::i16) {
			res = bb->push_cast(ty, res);
		}
		write(sema_context(), 0, res);
		return diag::ok;
	}
	// TODO: Debug/Control.

	auto ty = ir::int_type(ins.effective_width);
	write(sema_context(), 0, read(sema_context(), 1, ty));
	return diag::ok;
}

// Memset*.
//
static diag::lazy make_stos(SemaContext) {
	arch::mreg dst, cnt;
	if (mach->is_64())
		dst = reg::rdi, cnt = reg::rcx;
	else if (mach->is_32())
		dst = reg::edi, cnt = reg::ecx;
	else
		dst = reg::di, cnt = reg::cx;
	auto dstv = read_reg(sema_context(), dst);

	i32			  delta;
	arch::mreg	  data;
	ir::intrinsic intr;
	if (ins.mnemonic == ZYDIS_MNEMONIC_STOSB)
		data = reg::al, delta = 1, intr = ir::intrinsic::memset;
	else if (ins.mnemonic == ZYDIS_MNEMONIC_STOSW)
		data = reg::ax, delta = 2, intr = ir::intrinsic::memset16;
	else if (ins.mnemonic == ZYDIS_MNEMONIC_STOSD)
		data = reg::eax, delta = 4, intr = ir::intrinsic::memset32;
	else
		data = reg::rax, delta = 8, intr = ir::intrinsic::memset64;
	auto datav = read_reg(sema_context(), data);

	auto df = bb->push_read_reg(ir::type::i1, reg::flag_df);
	if (ins.modifiers & (ZYDIS_ATTRIB_HAS_REP | ZYDIS_ATTRIB_HAS_REPE | ZYDIS_ATTRIB_HAS_REPNE)) {
		// ByteCnt = Cnt * N
		auto cntv  = read_reg(sema_context(), cnt);
		auto bcntv = bb->push_binop(ir::op::mul, cntv, ir::constant(cntv->get_type(), delta));

		// Tmp0 = DF ? ByteCnt : 0
		auto tmp0 = bb->push_select(df, bcntv, ir::constant(bcntv->get_type(), 0));
		// Tmp1 = DI - Tmp0
		auto tmp1 = bb->push_binop(ir::op::sub, dstv, tmp0);

		// Intrin(Ptr=Tmp1, Data, Cnt)
		auto call_ptr = bb->push_bitcast(ir::type::pointer, tmp1);
		auto call_arg = delta < 4 ? bb->push_cast(ir::type::i32, datav) : datav;
		auto call_cnt = mach->is_64() ? cntv : bb->push_cast(ir::type::i64, cntv);
		bb->push_sideeffect_intrinsic(intr, call_ptr, call_arg, call_cnt);

		// Tmp2 = DI + ByteCnt
		auto tmp2 = bb->push_binop(ir::op::add, dstv, bcntv);
		// DI =   DF ? Tmp1 : tmp2
		write_reg(sema_context(), dst, bb->push_select(df, tmp1, tmp2));
	} else {
		// DEST <- *
		bb->push_store_mem(bb->push_bitcast(ir::type::pointer, dstv), datav);
		// DI += DF ? -delta : +delta
		auto dv = bb->push_select(df, ir::constant(dstv->get_type(), -delta), ir::constant(dstv->get_type(), +delta));
		write_reg(sema_context(), dst, bb->push_binop(ir::op::add, dstv, dv));
	}
	return diag::ok;
}
DECL_SEMA(STOSB) { return make_stos(sema_context()); }
DECL_SEMA(STOSW) { return make_stos(sema_context()); }
DECL_SEMA(STOSD) { return make_stos(sema_context()); }
DECL_SEMA(STOSQ) { return make_stos(sema_context()); }

// Memcpy*.
//
static diag::lazy make_movs(SemaContext) {
	arch::mreg src, dst, cnt;
	if (mach->is_64())
		src = reg::rsi, dst = reg::rdi, cnt = reg::rcx;
	else if (mach->is_32())
		src = reg::esi, dst = reg::edi, cnt = reg::ecx;
	else
		src = reg::si, dst = reg::di, cnt = reg::cx;
	auto srcv = read_reg(sema_context(), src);
	auto dstv = read_reg(sema_context(), dst);

	i32 delta;
	if (ins.mnemonic == ZYDIS_MNEMONIC_MOVSB)
		delta = 1;
	else if (ins.mnemonic == ZYDIS_MNEMONIC_MOVSW)
		delta = 2;
	else if (ins.mnemonic == ZYDIS_MNEMONIC_MOVSD)
		delta = 4;
	else
		delta = 8;

	auto df = bb->push_read_reg(ir::type::i1, reg::flag_df);
	if (ins.modifiers & (ZYDIS_ATTRIB_HAS_REP | ZYDIS_ATTRIB_HAS_REPE | ZYDIS_ATTRIB_HAS_REPNE)) {
		// ByteCnt = Cnt * N
		auto cntv  = read_reg(sema_context(), cnt);
		auto bcntv = bb->push_binop(ir::op::mul, cntv, ir::constant(cntv->get_type(), delta));

		// Tmp0 = DF ? ByteCnt : 0
		auto tmp0 = bb->push_select(df, bcntv, ir::constant(bcntv->get_type(), 0));
		// Tmp1(d/s) = DI/SI - Tmp0
		auto tmp1d = bb->push_binop(ir::op::sub, dstv, tmp0);
		auto tmp1s = bb->push_binop(ir::op::sub, srcv, tmp0);

		// Intrin(Tmp1d, Tmp1s, Bytecnt)
		auto call_dst = bb->push_bitcast(ir::type::pointer, tmp1d);
		auto call_src = bb->push_bitcast(ir::type::pointer, tmp1s);
		auto call_cnt = mach->is_64() ? bcntv : bb->push_cast(ir::type::i64, bcntv);
		bb->push_sideeffect_intrinsic(ir::intrinsic::memcpy, call_dst, call_src, call_cnt);

		// Tmp2(d/s) = DI/SI + ByteCnt
		auto tmp2d = bb->push_binop(ir::op::add, dstv, bcntv);
		auto tmp2s = bb->push_binop(ir::op::add, srcv, bcntv);
		// DI/SI =   DF ? Tmp1(d/s) : tmp2(d/s)
		write_reg(sema_context(), dst, bb->push_select(df, tmp1d, tmp2d));
		write_reg(sema_context(), src, bb->push_select(df, tmp1s, tmp2s));
	} else {
		// DV = DF ? -delta : +delta
		auto dv = bb->push_select(df, ir::constant(dstv->get_type(), -delta), ir::constant(dstv->get_type(), +delta));
		// memcpy(DEST, SRC, Delta)
		bb->push_sideeffect_intrinsic(ir::intrinsic::memcpy, bb->push_bitcast(ir::type::pointer, dstv), bb->push_bitcast(ir::type::pointer, srcv), (i64) delta);
		// DI += DV
		write_reg(sema_context(), dst, bb->push_binop(ir::op::add, dstv, dv));
	}
	return diag::ok;
}
DECL_SEMA(MOVSB) { return make_movs(sema_context()); }
DECL_SEMA(MOVSW) { return make_movs(sema_context()); }
DECL_SEMA(MOVSD) { return make_movs(sema_context()); }
DECL_SEMA(MOVSQ) { return make_movs(sema_context()); }

// Atomic data transfer.
//
DECL_SEMA(CMPXCHG16B) {
	constexpr auto ty				  = ir::type::i128;
	auto				desired		  = read_pair(sema_context(), reg::rcx, reg::rbx);
	auto				comperand_val = read_pair(sema_context(), reg::rdx, reg::rax);
	ir::variant		prev;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto adr	  = agen(sema_context(), ins.op[0].m);
		auto previ = bb->push_atomic_cmpxchg(std::move(adr), comperand_val, desired);
		bb->push_write_reg(reg::flag_zf, bb->push_cmp(ir::op::eq, previ, comperand_val));
		prev = previ;
	} else {
		// Read dst, check if equal to comperand.
		prev			= read(sema_context(), 0, ty);
		auto was_eq = bb->push_cmp(ir::op::eq, prev, comperand_val);
		// [dst] := eq ? desired : prev
		write(sema_context(), 0, bb->push_select(was_eq, desired, prev));
		// zf :=    was eq.
		bb->push_write_reg(reg::flag_zf, was_eq);
	}
	// comperand := prev
	write_pair(sema_context(), reg::rdx, reg::rax, std::move(prev));
	return diag::ok;
}
DECL_SEMA(CMPXCHG8B) {
	constexpr auto ty				  = ir::type::i64;
	auto				desired		  = read_pair(sema_context(), reg::ecx, reg::ebx);
	auto				comperand_val = read_pair(sema_context(), reg::edx, reg::eax);
	ir::variant		prev;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto adr	  = agen(sema_context(), ins.op[0].m);
		auto previ = bb->push_atomic_cmpxchg(std::move(adr), comperand_val, desired);
		bb->push_write_reg(reg::flag_zf, bb->push_cmp(ir::op::eq, previ, comperand_val));
		prev = previ;
	} else {
		// Read dst, check if equal to comperand.
		prev			= read(sema_context(), 0, ty);
		auto was_eq = bb->push_cmp(ir::op::eq, prev, comperand_val);
		// [dst] := eq ? desired : prev
		write(sema_context(), 0, bb->push_select(was_eq, desired, prev));
		// zf :=    was eq.
		bb->push_write_reg(reg::flag_zf, was_eq);
	}
	// comperand := prev
	write_pair(sema_context(), reg::edx, reg::eax, std::move(prev));
	return diag::ok;
}
DECL_SEMA(CMPXCHG) {
	arch::mreg comperand;
	switch (ins.effective_width) {
		case 8:
			comperand = reg::al;
			break;
		case 16:
			comperand = reg::ax;
			break;
		case 32:
			comperand = reg::eax;
			break;
		case 64:
			comperand = reg::rax;
			break;
		default:
			RC_UNREACHABLE();
	}

	auto ty				 = ir::int_type(ins.effective_width);
	auto desired		 = read_reg(sema_context(), ins.op[1].r, ty);
	auto comperand_val = read_reg(sema_context(), comperand, ty);

	ir::variant prev;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto adr	  = agen(sema_context(), ins.op[0].m);
		auto previ = bb->push_atomic_cmpxchg(std::move(adr), comperand_val, desired);
		bb->push_write_reg(reg::flag_zf, bb->push_cmp(ir::op::eq, previ, comperand_val));
		prev = previ;
	} else {
		// Read dst, check if equal to comperand.
		prev			= read(sema_context(), 0, ty);
		auto was_eq = bb->push_cmp(ir::op::eq, prev, comperand_val);
		// [dst] := eq ? desired : prev
		write(sema_context(), 0, bb->push_select(was_eq, desired, prev));
		// zf :=    was eq.
		bb->push_write_reg(reg::flag_zf, was_eq);
	}
	// comperand := prev
	write_reg(sema_context(), comperand, std::move(prev));
	return diag::ok;
}
DECL_SEMA(XCHG) {
	auto ty = ir::int_type(ins.effective_width);

	// mem, reg swap.
	//
	if (ins.op[0].type == arch::mop_type::mem || ins.op[1].type == arch::mop_type::mem) {
		auto& reg = ins.op[ins.op[0].type == arch::mop_type::mem ? 1 : 0].r;
		auto& mem = ins.op[ins.op[0].type == arch::mop_type::mem ? 0 : 1].m;

		auto adr	 = agen(sema_context(), mem);
		auto prev = bb->push_atomic_xchg(std::move(adr), read_reg(sema_context(), reg, ty));
		write_reg(sema_context(), reg, prev);
	}
	// reg, reg swap.
	//
	else {
		// Pattern: [xchg reg, reg] <=> [nop]
		if (ins.op[0].r == ins.op[1].r) {
			bb->push_nop();
			return diag::ok;
		}

		auto o0 = read(sema_context(), 0, ty);
		auto o1 = read(sema_context(), 1, ty);
		write(sema_context(), 0, std::move(o1));
		write(sema_context(), 1, std::move(o0));
	}
	return diag::ok;
}

// Stack.
//
DECL_SEMA(PUSH) {
	auto rsp		 = reg_sp(mach);
	auto pty		 = mach->ptr_type();
	auto ty		 = ir::int_type(ins.effective_width);
	i32  dif		 = ins.effective_width == 16 ? 16 : mach->ptr_width / 8;
	auto prev_sp = read_reg(sema_context(), rsp, pty);

	// Update SP.
	auto value	= read(sema_context(), 0, ty);
	auto new_sp = bb->push_binop(ir::op::sub, prev_sp, ir::constant(pty, dif));
	write_reg(sema_context(), rsp, new_sp);

	// Write the value.
	bb->push_store_mem(bb->push_bitcast(ir::type::pointer, new_sp), std::move(value));
	return diag::ok;
}
DECL_SEMA(POP) {
	auto rsp		 = reg_sp(mach);
	auto pty		 = mach->ptr_type();
	auto ty		 = ir::int_type(ins.effective_width);
	i32  dif		 = ins.effective_width == 16 ? 16 : mach->ptr_width / 8;
	auto prev_sp = read_reg(sema_context(), rsp, pty);

	// Read the value.
	auto value = bb->push_load_mem(ty, bb->push_bitcast(ir::type::pointer, prev_sp));

	// Update SP.
	auto new_sp = bb->push_binop(ir::op::add, prev_sp, ir::constant(pty, dif));
	write_reg(sema_context(), rsp, new_sp);

	// Store the operand.
	write(sema_context(), 0, value);
	return diag::ok;
}

// Sign/Zero extension.
//
DECL_SEMA(MOVZX) {
	auto t0 = ir::int_type(ins.op[0].get_width());
	auto t1 = ir::int_type(ins.op[1].get_width());
	write(sema_context(), 0, bb->push_cast(t0, read(sema_context(), 1, t1)));
	return diag::ok;
}
DECL_SEMA(MOVSX) {
	auto t0 = ir::int_type(ins.op[0].get_width());
	auto t1 = ir::int_type(ins.op[1].get_width());
	write(sema_context(), 0, bb->push_cast_sx(t0, read(sema_context(), 1, t1)));
	return diag::ok;
}
DECL_SEMA(MOVSXD) {
	auto t0 = ir::int_type(ins.op[0].get_width());
	auto t1 = ir::int_type(ins.op[1].get_width());
	write(sema_context(), 0, bb->push_cast_sx(t0, read(sema_context(), 1, t1)));
	return diag::ok;
}
DECL_SEMA(CBW) {
	constexpr auto t0 = ir::type::i8;
	constexpr auto t1 = ir::type::i16;
	auto				a	= read_reg(sema_context(), reg::al, t0);
	a						= bb->push_cast_sx(t1, a);
	write_reg(sema_context(), reg::ax, a);
	return diag::ok;
}
DECL_SEMA(CWDE) {
	constexpr auto t0 = ir::type::i16;
	constexpr auto t1 = ir::type::i32;
	auto				a	= read_reg(sema_context(), reg::ax, t0);
	a						= bb->push_cast_sx(t1, a);
	write_reg(sema_context(), reg::eax, a);
	return diag::ok;
}
DECL_SEMA(CDQE) {
	constexpr auto t0 = ir::type::i32;
	constexpr auto t1 = ir::type::i64;
	auto				a	= read_reg(sema_context(), reg::eax, t0);
	a						= bb->push_cast_sx(t1, a);
	write_reg(sema_context(), reg::rax, a);
	return diag::ok;
}
DECL_SEMA(CWD) {
	constexpr auto t0 = ir::type::i16;
	constexpr auto t1 = ir::type::i32;
	auto				a	= read_reg(sema_context(), reg::ax, t0);
	a						= bb->push_cast_sx(t1, a);
	write_pair(sema_context(), reg::dx, reg::ax, a);
	return diag::ok;
}
DECL_SEMA(CDQ) {
	constexpr auto t0 = ir::type::i32;
	constexpr auto t1 = ir::type::i64;
	auto				a	= read_reg(sema_context(), reg::eax, t0);
	a						= bb->push_cast_sx(t1, a);
	write_pair(sema_context(), reg::edx, reg::eax, a);
	return diag::ok;
}
DECL_SEMA(CQO) {
	constexpr auto t0 = ir::type::i64;
	constexpr auto t1 = ir::type::i128;
	auto				a	= read_reg(sema_context(), reg::rax, t0);
	a						= bb->push_cast_sx(t1, a);
	write_pair(sema_context(), reg::rdx, reg::rax, a);
	return diag::ok;
}

// Flags.
//
DECL_SEMA(CLAC) {
	bb->push_write_reg(reg::flag_ac, false);
	return diag::ok;
}
DECL_SEMA(CLD) {
	bb->push_write_reg(reg::flag_df, false);
	return diag::ok;
}
DECL_SEMA(CLC) {
	bb->push_write_reg(reg::flag_cf, false);
	return diag::ok;
}
DECL_SEMA(CLI) {
	bb->push_write_reg(reg::flag_if, false);
	return diag::ok;
}
DECL_SEMA(STAC) {
	bb->push_write_reg(reg::flag_ac, true);
	return diag::ok;
}
DECL_SEMA(STD) {
	bb->push_write_reg(reg::flag_df, true);
	return diag::ok;
}
DECL_SEMA(STC) {
	bb->push_write_reg(reg::flag_cf, true);
	return diag::ok;
}
DECL_SEMA(STI) {
	bb->push_write_reg(reg::flag_if, true);
	return diag::ok;
}