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
	auto* retptr = bb->push_load_mem(ir::type::pointer, bb->push_bitcast(ir::type::pointer, prev_sp));

	// SP = SP + Imm + sizeof Ptr.
	i64 sp_delta = mach->ptr_width / 8;
	if (ins.operand_count)
		sp_delta += ins.op[0].i.s;
	auto new_sp = bb->push_binop(ir::op::add, prev_sp, ir::constant(pty, sp_delta));
	write_reg(sema_context(), rsp, new_sp);

	// XRET(retptr)
	bb->push_xret(retptr);
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
	auto rbpv = bb->push_load_mem(ty, bb->push_bitcast(ir::type::pointer, rsp));
	auto rspv = bb->push_binop(ir::op::add, std::move(rsp), ir::constant(ty, n));

	write_reg(sema_context(), spr, rspv);
	write_reg(sema_context(), bpr, rbpv);
	return diag::ok;
}

// TODO: IRETQ, IRETD, SYSRET...
DECL_SEMA(NOP) { return diag::ok; }
DECL_SEMA(UD2) {
	bb->push_trap("ud2");
	return diag::ok;
}
DECL_SEMA(INT3) {
	bb->push_trap("int3");
	return diag::ok;
}
DECL_SEMA(INT1) {
	bb->push_trap("int1");
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
DECL_SEMA(SWAPGS) {
	bb->push_sideeffect_intrinsic(ir::intrinsic::ia32_swapgs);
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

// TODO: push/popf(_/d/q)
//       pushad/popad