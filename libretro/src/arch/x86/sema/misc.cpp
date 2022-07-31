#include <retro/arch/x86/sema.hpp>
#include <retro/common.hpp>

using namespace retro;
using namespace retro::arch::x86;

// TODO: Special handling:
//  pxor xmm0, xmm0
//  lock or [rsp], 0
//
//

DECL_SEMA(CALL) {
	bb->push_xcall(read(sema_context(), 0, ir::type::pointer));
	return diag::ok;
}
DECL_SEMA(JMP) {
	bb->push_xjmp(read(sema_context(), 0, ir::type::pointer));
	return diag::ok;
}
DECL_SEMA(RET) {
	// RET imm16:
	if (ins.operand_count) {
		// SP = SP + Imm.
		auto rsp		 = reg_sp(mach);
		auto pty		 = mach->ptr_type();
		auto prev_sp = read_reg(sema_context(), rsp, pty);
		auto new_sp	 = bb->push_binop(ir::op::add, prev_sp, ir::constant(pty, ins.op[0].i.s));
		write_reg(sema_context(), rsp, new_sp);
	}
	bb->push_xret();
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
// TODO: INT / INTO


/*
pushfq
popfq

xchg -> 4 0.018925%
movsd -> 85 0.402157%
xorps -> 24 0.113550%
cmpxchg -> 1 0.004731%

cqo -> 8 0.037850%
mul -> 10 0.047313%
imul -> 19 0.089894%
div -> 1 0.004731%
sbb -> 5 0.023656%
adc -> 3 0.014194%

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

scasd -> 1 0.004731%
stosw -> 3 0.014194%
lodsb -> 1 0.004731%
stosd -> 1 0.004731%

xgetbv -> 1 0.004731%
cpuid -> 3 0.014194%
*/