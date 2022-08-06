#include <retro/arch/x86/sema.hpp>
#include <retro/common.hpp>

using namespace retro;
using namespace retro::arch::x86;

DECL_SEMA(CALL) {
	bb->push_xcall(read(sema_context(), 0, ir::type::pointer));
	return diag::ok;
}
DECL_SEMA(JMP) {
	bb->push_xjmp(read(sema_context(), 0, ir::type::pointer));
	return diag::ok;
}
DECL_SEMA(RET) {
	auto rsp		 = reg_sp(mach);
	auto pty		 = mach->ptr_type();
	auto prev_sp = read_reg(sema_context(), rsp, pty);

	// Retptr = [SP]
	auto* adrretptr = bb->push_bitcast(ir::type::pointer, prev_sp);

	// SP = SP + Imm + sizeof Ptr.
	i64 sp_delta = mach->ptr_width / 8;
	if (ins.operand_count)
		sp_delta += ins.op[0].i.s;
	auto new_sp = bb->push_binop(ir::op::add, prev_sp, ir::constant(pty, sp_delta));
	write_reg(sema_context(), rsp, new_sp);

	// XRET(adrretptr)
	bb->push_xret(adrretptr);
	return diag::ok;
}
DECL_SEMA(LEAVE) {
	ir::type ty;
	reg		bpr, spr;
	u32		n;

	if (mach->is_64()) {
		ty = ir::type::i64;
		bpr = reg::rbp;
		spr = reg::rsp;
		n	= 8;
	} else if (mach->is_32()) {
		ty	 = ir::type::i32;
		bpr = reg::ebp;
		spr = reg::esp;
		n	 = 4;
	} else {
		ty	 = ir::type::i16;
		bpr = reg::bp;
		spr = reg::sp;
		n	 = 2;
	}

	// RSP <- RBP;
	auto rsp = read_reg(sema_context(), bpr, ty);
	// RBP <- POP();
	auto rbpv = bb->push_load_mem(ty, bb->push_bitcast(ir::type::pointer, rsp), 0);
	auto rspv = bb->push_binop(ir::op::add, std::move(rsp), ir::constant(ty, n));

	write_reg(sema_context(), spr, rspv);
	write_reg(sema_context(), bpr, rbpv);
	return diag::ok;
}
DECL_SEMA(NOP) {
	bb->push_nop();
	return diag::ok;
}
DECL_SEMA(UD0) {
	bb->push_trap("ud0");
	return diag::ok;
}
DECL_SEMA(UD1) {
	bb->push_trap("ud1");
	return diag::ok;
}
DECL_SEMA(UD2) {
	bb->push_trap("ud2");
	return diag::ok;
}
DECL_SEMA(INT1) {
	bb->push_trap("int1");
	return diag::ok;
}
DECL_SEMA(INT3) {
	bb->push_trap("int3");
	return diag::ok;
}
DECL_SEMA(INT) {
	bb->push_trap(fmt::str("int%x", ins.op[0].i.get_unsigned()));
	return diag::ok;
}
DECL_SEMA(RDTSC) {
	auto res = bb->push_intrinsic(ir::intrinsic::readcyclecounter);
	res		= bb->push_extract(ir::type::i64, res, 0);
	write_pair(sema_context(), reg::edx, reg::eax, std::move(res));
	return diag::ok;
}
DECL_SEMA(RDTSCP) {
	lift_RDTSC(sema_context());
	auto res = bb->push_intrinsic(ir::intrinsic::ia32_readpid);
	res		= bb->push_extract(ir::type::i32, res, 0);
	write_reg(sema_context(), reg::ecx, res);
	return diag::ok;
}
DECL_SEMA(RDPID) {
	auto res = bb->push_intrinsic(ir::intrinsic::ia32_readpid);
	res		= bb->push_extract(ir::type::i32, res, 0);
	if (ins.op[0].r.get_kind() == arch::reg_kind::gpr64)
		res = bb->push_cast(ir::type::i64, res);
	write_reg(sema_context(), ins.op[0].r, res);
	return diag::ok;
}
DECL_SEMA(CPUID) {
	auto res = bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_cpuid, read_reg(sema_context(), reg::eax), read_reg(sema_context(), reg::ecx));
	write_reg(sema_context(), reg::eax, bb->push_extract(ir::type::i32, res, 0));
	write_reg(sema_context(), reg::ebx, bb->push_extract(ir::type::i32, res, 1));
	write_reg(sema_context(), reg::ecx, bb->push_extract(ir::type::i32, res, 2));
	write_reg(sema_context(), reg::edx, bb->push_extract(ir::type::i32, res, 3));
	return diag::ok;
}
DECL_SEMA(XGETBV) {
	auto res = bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_xgetbv, read_reg(sema_context(), reg::ecx));
	res		= bb->push_extract(ir::type::i64, res, 0);
	write_pair(sema_context(), reg::edx, reg::eax, std::move(res));
	return diag::ok;
}
DECL_SEMA(RDPMC) {
	auto res = bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_rdpmc, read_reg(sema_context(), reg::ecx));
	res		= bb->push_extract(ir::type::i64, res, 0);
	write_pair(sema_context(), reg::edx, reg::eax, std::move(res));
	return diag::ok;
}
DECL_SEMA(XSETBV) {
	auto val = read_pair(sema_context(), reg::edx, reg::eax);
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_xsetbv, read_reg(sema_context(), reg::ecx), std::move(val));
	return diag::ok;
}
DECL_SEMA(RDMSR) {
	auto res = bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_rdmsr, read_reg(sema_context(), reg::ecx));
	res		= bb->push_extract(ir::type::i64, res, 0);
	write_pair(sema_context(), reg::edx, reg::eax, std::move(res));
	return diag::ok;
}
DECL_SEMA(WRMSR) {
	auto val = read_pair(sema_context(), reg::edx, reg::eax);
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_wrmsr, read_reg(sema_context(), reg::ecx), std::move(val));
	return diag::ok;
}

DECL_SEMA(FXSAVE) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_fxsave, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(FXSAVE64) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_fxsave64, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(FXRSTOR) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_fxrstor, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(FXRSTOR64) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_fxrstor64, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}

DECL_SEMA(XRSTOR) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_xrstor, agen(sema_context(), ins.op[0].m, true), read_pair(sema_context(), reg::edx, reg::eax));
	return diag::ok;
}
DECL_SEMA(XRSTORS) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_xrstors, agen(sema_context(), ins.op[0].m, true), read_pair(sema_context(), reg::edx, reg::eax));
	return diag::ok;
}
DECL_SEMA(XSAVE) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_xsave, agen(sema_context(), ins.op[0].m, true), read_pair(sema_context(), reg::edx, reg::eax));
	return diag::ok;
}
DECL_SEMA(XSAVES) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_xsaves, agen(sema_context(), ins.op[0].m, true), read_pair(sema_context(), reg::edx, reg::eax));
	return diag::ok;
}
DECL_SEMA(XSAVEC) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_xsavec, agen(sema_context(), ins.op[0].m, true), read_pair(sema_context(), reg::edx, reg::eax));
	return diag::ok;
}
DECL_SEMA(XSAVEOPT) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_xsaveopt, agen(sema_context(), ins.op[0].m, true), read_pair(sema_context(), reg::edx, reg::eax));
	return diag::ok;
}
DECL_SEMA(XRSTOR64) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_xrstor64, agen(sema_context(), ins.op[0].m, true), read_pair(sema_context(), reg::edx, reg::eax));
	return diag::ok;
}
DECL_SEMA(XRSTORS64) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_xrstors64, agen(sema_context(), ins.op[0].m, true), read_pair(sema_context(), reg::edx, reg::eax));
	return diag::ok;
}
DECL_SEMA(XSAVE64) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_xsave64, agen(sema_context(), ins.op[0].m, true), read_pair(sema_context(), reg::edx, reg::eax));
	return diag::ok;
}
DECL_SEMA(XSAVES64) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_xsaves64, agen(sema_context(), ins.op[0].m, true), read_pair(sema_context(), reg::edx, reg::eax));
	return diag::ok;
}
DECL_SEMA(XSAVEC64) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_xsavec64, agen(sema_context(), ins.op[0].m, true), read_pair(sema_context(), reg::edx, reg::eax));
	return diag::ok;
}
DECL_SEMA(XSAVEOPT64) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_xsaveopt64, agen(sema_context(), ins.op[0].m, true), read_pair(sema_context(), reg::edx, reg::eax));
	return diag::ok;
}




DECL_SEMA(PREFETCH) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::prefetch, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(PREFETCHNTA) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_prefetchnta, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(PREFETCHT0) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_prefetcht0, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(PREFETCHT1) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_prefetcht1, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(PREFETCHT2) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_prefetcht2, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(PREFETCHW) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_prefetchw, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(PREFETCHWT1) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_prefetchwt1, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(CLWB) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_clwb, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(CLFLUSH) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_clflush, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(CLFLUSHOPT) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_clflushopt, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(CLDEMOTE) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_cldemote, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(INVLPG) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_invlpg, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(INVPCID) {
	auto ty	= ir::int_type(ins.op[0].get_width());
	auto val = read(sema_context(), 0, ty);
	if (ty != ir::type::i32) {
		val = bb->push_cast(ty, std::move(val));
	}
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_invpcid, std::move(val), agen(sema_context(), ins.op[1].m, true));
	return diag::ok;
}
DECL_SEMA(CLZERO) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_clzero);
	return diag::ok;
}
DECL_SEMA(MFENCE) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::mfence);
	return diag::ok;
}
DECL_SEMA(SFENCE) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::sfence);
	return diag::ok;
}
DECL_SEMA(LFENCE) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::lfence);
	return diag::ok;
}
DECL_SEMA(PAUSE) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_pause);
	return diag::ok;
}
DECL_SEMA(INVD) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_invd);
	return diag::ok;
}
DECL_SEMA(WBINVD) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_wbinvd);
	return diag::ok;
}
DECL_SEMA(SWAPGS) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_swapgs);
	return diag::ok;
}
DECL_SEMA(HLT) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_hlt);
	return diag::ok;
}
DECL_SEMA(RSM) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_rsm);
	return diag::ok;
}
DECL_SEMA(RDGSBASE) {
	u16  w		= ins.effective_width;
	auto intrin = w == 32 ? ir::intrinsic::ia32_rdgsbase32 : ir::intrinsic::ia32_rdgsbase64;
	auto ty		= w == 32 ? ir::type::i32 : ir::type::i64;
	write(sema_context(), 0, bb->push_extract(ty, bb->push_intrinsic(intrin), 0));
	return diag::ok;
}
DECL_SEMA(RDFSBASE) {
	u16  w		= ins.effective_width;
	auto intrin = w == 32 ? ir::intrinsic::ia32_rdfsbase32 : ir::intrinsic::ia32_rdfsbase64;
	auto ty		= w == 32 ? ir::type::i32 : ir::type::i64;
	write(sema_context(), 0, bb->push_extract(ty, bb->push_intrinsic(intrin), 0));
	return diag::ok;
}
DECL_SEMA(WRGSBASE) {
	u16 w = ins.effective_width;
	auto intrin = w == 32 ? ir::intrinsic::ia32_wrgsbase32 : ir::intrinsic::ia32_wrgsbase64;
	auto ty		= w == 32 ? ir::type::i32 : ir::type::i64;
	bb->push_sideeffect_intrinsic(intrin, read(sema_context(), 0, ty));
	return diag::ok;
}
DECL_SEMA(WRFSBASE) {
	u16  w		= ins.effective_width;
	auto intrin = w == 32 ? ir::intrinsic::ia32_wrfsbase32 : ir::intrinsic::ia32_wrfsbase64;
	auto ty		= w == 32 ? ir::type::i32 : ir::type::i64;
	bb->push_sideeffect_intrinsic(intrin, read(sema_context(), 0, ty));
	return diag::ok;
}
DECL_SEMA(STMXCSR) {
	write(sema_context(), 0, bb->push_extract(ir::type::i32, bb->push_intrinsic(ir::intrinsic::ia32_stmxcsr), 0));
	return diag::ok;
}
DECL_SEMA(VSTMXCSR) {
	write(sema_context(), 0, bb->push_extract(ir::type::i32, bb->push_intrinsic(ir::intrinsic::ia32_stmxcsr), 0));
	return diag::ok;
}
DECL_SEMA(LDMXCSR) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_ldmxcsr, read(sema_context(), 0, ir::type::i32));
	return diag::ok;
}
DECL_SEMA(VLDMXCSR) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_ldmxcsr, read(sema_context(), 0, ir::type::i32));
	return diag::ok;
}

DECL_SEMA(IN) {
	auto port = read(sema_context(), 1, ir::type::i16);

	switch (ins.op[0].get_width()) {
		case 8:
			write(sema_context(), 0, bb->push_extract(ir::type::i8, bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_inb, port), 0));
			break;
		case 16:
			write(sema_context(), 0, bb->push_extract(ir::type::i16, bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_inw, port), 0));
			break;
		case 32:
			write(sema_context(), 0, bb->push_extract(ir::type::i32, bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_ind, port), 0));
			break;
		default:
			RC_UNREACHABLE();
	}
	return diag::ok;
}
DECL_SEMA(OUT) {
	auto port = read(sema_context(), 0, ir::type::i16);
	switch (ins.op[1].get_width()) {
		case 8:
			bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_outb, port, read(sema_context(), 1, ir::type::i8));
			break;
		case 16:
			bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_outw, port, read(sema_context(), 1, ir::type::i16));
			break;
		case 32:
			bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_outd, port, read(sema_context(), 1, ir::type::i32));
			break;
		default:
			RC_UNREACHABLE();
	}
	return diag::ok;
}

DECL_SEMA(SGDT) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_sgdt, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(SIDT) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_sidt, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(LGDT) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_lgdt, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(LIDT) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_lidt, agen(sema_context(), ins.op[0].m, true));
	return diag::ok;
}
DECL_SEMA(SMSW) {
	auto res = bb->push_extract(ir::type::i16, bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_smsw), 0);
	auto ty	= ir::int_type(ins.op[0].get_width());
	if (ty != ir::type::i16) {
		res = bb->push_cast(ty, res);
	}
	write(sema_context(), 0, res);
	return diag::ok;
}
DECL_SEMA(SLDT) {
	auto res = bb->push_extract(ir::type::i16, bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_sldt), 0);
	auto ty	= ir::int_type(ins.op[0].get_width());
	if (ty != ir::type::i16) {
		res = bb->push_cast(ty, res);
	}
	write(sema_context(), 0, res);
	return diag::ok;
}
DECL_SEMA(STR) {
	auto res = bb->push_extract(ir::type::i16, bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_str), 0);
	auto ty	= ir::int_type(ins.op[0].get_width());
	if (ty != ir::type::i16) {
		res = bb->push_cast(ty, res);
	}
	write(sema_context(), 0, res);
	return diag::ok;
}
DECL_SEMA(LMSW) {
	auto ty	= ir::int_type(ins.op[0].get_width());
	auto val = read(sema_context(), 0, ty);
	if (ty != ir::type::i16) {
		val = bb->push_cast(ty, std::move(val));
	}
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_lmsw, std::move(val));
	return diag::ok;
}
DECL_SEMA(LLDT) {
	auto ty	= ir::int_type(ins.op[0].get_width());
	auto val = read(sema_context(), 0, ty);
	if (ty != ir::type::i16) {
		val = bb->push_cast(ty, std::move(val));
	}
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_lldt, std::move(val));
	return diag::ok;
}
DECL_SEMA(LTR) {
	auto ty	= ir::int_type(ins.op[0].get_width());
	auto val = read(sema_context(), 0, ty);
	if (ty != ir::type::i16) {
		val = bb->push_cast(ty, std::move(val));
	}
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_ltr, std::move(val));
	return diag::ok;
}

DECL_SEMA(PUSHAD) {
	constexpr reg reg_list[] = {reg::edi, reg::esi, reg::ebp, reg::esp, reg::ebx, reg::edx, reg::ecx, reg::eax};

	// Read stack pointer, allocate space.
	//
	constexpr auto rsp = reg::esp;
	auto sp	= read_reg(sema_context(), rsp, ir::type::i32);
	auto nsp = bb->push_binop(ir::op::sub, sp, i32(std::size(reg_list) * 4));
	write_reg(sema_context(), rsp, nsp);

	// Read and store all registers.
	//
	auto spp = bb->push_bitcast(ir::type::pointer, sp);
	for (size_t i = 0; i != std::size(reg_list); i++) {
		ir::insn* value = sp;
		if (reg_list[i] != rsp)
			value = read_reg(sema_context(), reg_list[i], ir::type::i32);
		bb->push_store_mem(spp, i64(i * 4) - i64(std::size(reg_list) * 4), value);
	}
	return diag::ok;
}
DECL_SEMA(POPAD) {
	constexpr reg reg_list[] = {reg::edi, reg::esi, reg::ebp, reg::esp, reg::ebx, reg::edx, reg::ecx, reg::eax};

	// Read stack pointer, calculate space after free.
	//
	constexpr auto rsp = reg::esp;
	auto sp	= read_reg(sema_context(), rsp, ir::type::i32);
	auto nsp = bb->push_binop(ir::op::add, sp, i32(std::size(reg_list) * 4));

	// Load and write all registers.
	//
	auto spp = bb->push_bitcast(ir::type::pointer, sp);
	for (size_t i = 0; i != std::size(reg_list); i++) {
		if (reg_list[i] != rsp) {
			write_reg(sema_context(), reg_list[i], bb->push_load_mem(ir::type::i32, spp, i64(i)*4));
		}
	}

	// Update stack pointer.
	//
	write_reg(sema_context(), rsp, nsp);
	return diag::ok;
}
DECL_SEMA(PUSHA) {
	constexpr reg reg_list[] = {reg::di, reg::si, reg::bp, reg::sp, reg::bx, reg::dx, reg::cx, reg::ax};

	// Read stack pointer, allocate space.
	//
	auto pty = mach->ptr_type();
	auto rsp = reg_sp(mach);
	auto sp	= read_reg(sema_context(), rsp, pty);
	auto nsp = bb->push_binop(ir::op::sub, sp, ir::constant(pty, i32(std::size(reg_list) * 2)));
	write_reg(sema_context(), rsp, nsp);

	// Read and store all registers.
	//
	auto spp = bb->push_bitcast(ir::type::pointer, sp);
	for (size_t i = 0; i != std::size(reg_list); i++) {
		ir::insn* value;
		if (reg_list[i] != reg::sp)
			value = read_reg(sema_context(), reg_list[i], ir::type::i16);
		else
			value = bb->push_cast(ir::type::i16, sp);
		bb->push_store_mem(spp, i64(i * 2) - i64(std::size(reg_list) * 2), value);
	}
	return diag::ok;
}
DECL_SEMA(POPA) {
	constexpr reg reg_list[] = {reg::di, reg::si, reg::bp, reg::sp, reg::bx, reg::dx, reg::cx, reg::ax};

	// Read stack pointer, calculate space after free.
	//
	auto pty = mach->ptr_type();
	auto rsp = reg_sp(mach);
	auto sp	= read_reg(sema_context(), rsp, pty);
	auto nsp = bb->push_binop(ir::op::add, sp, ir::constant(pty, i32(std::size(reg_list) * 2)));

	// Load and write all registers.
	//
	auto spp = bb->push_bitcast(ir::type::pointer, sp);
	for (size_t i = 0; i != std::size(reg_list); i++) {
		if (reg_list[i] != reg::sp) {
			write_reg(sema_context(), reg_list[i], bb->push_load_mem(ir::type::i32, spp, i64(i) * 2));
		}
	}

	// Update stack pointer.
	//
	write_reg(sema_context(), rsp, nsp);
	return diag::ok;
}

// TODO: push/popf(_/d/q)
//       lahf/sahf
//       rdrand/rdseed
//       lsl verr verw
//       iret iretq iretd
//       sysret sysexit
//       syscall sysenter
