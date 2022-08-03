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
	auto	rsp	  = reg_sp(mach);
	auto	pty	  = mach->ptr_type();
	auto	prev_sp = read_reg(sema_context(), rsp, pty);

	// Retptr = [SP]
	auto* retptr  = bb->push_load_mem(ir::type::pointer, ir::segment::NO_SEGMENT, bb->push_bitcast(ir::type::pointer, prev_sp));
	
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
DECL_SEMA(RDTSC) {
	auto res = bb->push_intrinsic(ir::intrinsic::readcyclecounter);
	res = bb->push_extract(ir::type::i64, res, 0);
	write_reg(sema_context(), reg::eax, bb->push_cast(ir::type::i32, res));
	write_reg(sema_context(), reg::edx, bb->push_cast(ir::type::i32, bb->push_binop(ir::op::bit_shr, res, (i64)32)));
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
	auto res = bb->push_volatile_intrinsic(ir::intrinsic::ia32_cpuid, read_reg(sema_context(), reg::eax), read_reg(sema_context(), reg::ecx));
	write_reg(sema_context(), reg::eax, bb->push_extract(ir::type::i32, res, 0));
	write_reg(sema_context(), reg::ebx, bb->push_extract(ir::type::i32, res, 1));
	write_reg(sema_context(), reg::ecx, bb->push_extract(ir::type::i32, res, 2));
	write_reg(sema_context(), reg::edx, bb->push_extract(ir::type::i32, res, 3));
	return diag::ok;
}
DECL_SEMA(XGETBV) {
	auto res = bb->push_volatile_intrinsic(ir::intrinsic::ia32_xgetbv, read_reg(sema_context(), reg::ecx));
	res		= bb->push_extract(ir::type::i64, res, 0);
	write_reg(sema_context(), reg::eax, bb->push_cast(ir::type::i32, res));
	write_reg(sema_context(), reg::edx, bb->push_cast(ir::type::i32, bb->push_binop(ir::op::bit_shr, res, (i64) 32)));
	return diag::ok;
}
DECL_SEMA(XSETBV) {
	auto a  = bb->push_cast(ir::type::i64, read_reg(sema_context(), reg::eax) a);
	auto d  = bb->push_cast(ir::type::i64, read_reg(sema_context(), reg::eax));
	d		  = bb->push_binop(ir::op::bit_shl, d, (i64) 32);
	auto da = bb->push_binop(ir::op::bit_or, d, a);
	bb->push_volatile_intrinsic(ir::intrinsic::ia32_xsetbv, read_reg(sema_context(), reg::ecx), da);
	return diag::ok;
}
// TODO: INT / INTO


/*
pushfq
popfq

xchg -> 4 0.018925%
movsd -> 85 0.402157%
cmpxchg -> 1 0.004731%

cqo -> 8 0.037850%
sbb -> 5 0.023656%
adc -> 3 0.014194%

scasd -> 1 0.004731%
stosw -> 3 0.014194%
lodsb -> 1 0.004731%
stosd -> 1 0.004731%
*/