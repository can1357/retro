#include <retro/arch/x86/sema.hpp>
#include <retro/common.hpp>

using namespace retro;
using namespace retro::arch::x86;

// Declare all forms of JCC, SETCC and CMOVCC.
//
template<auto Cc>
static diag::lazy make_jcc(SemaContext) {
	bb->push_xjs(Cc(sema_context()), (ir::pointer) ins.op[0].i.get_unsigned(ip), (ir::pointer)(ip + ins.length));
	return diag::ok;
}
template<auto Cc>
static diag::lazy make_jccn(SemaContext) {
	bb->push_xjs(Cc(sema_context()), (ir::pointer)(ip + ins.length), (ir::pointer) ins.op[0].i.get_unsigned(ip));
	return diag::ok;
}
template<auto Cc>
static diag::lazy make_setcc(SemaContext) {
	auto result = bb->push_cast(ir::type::i8, Cc(sema_context()));
	write(sema_context(), 0, result);
	return diag::ok;
}
template<auto Cc>
static diag::lazy make_setccn(SemaContext) {
	auto result = bb->push_cast(ir::type::i8, Cc(sema_context()));
	write(sema_context(), 0, bb->push_binop(ir::op::bit_xor, result, ir::constant((i8) 1)));
	return diag::ok;
}
template<auto Cc>
static diag::lazy make_cmovcc(SemaContext) {
	auto ty		= ir::int_type(ins.effective_width);
	auto lhs		= read(sema_context(), 0, ty);
	auto rhs		= read(sema_context(), 1, ty);
	auto result = bb->push_select(Cc(sema_context()), rhs, lhs);
	write(sema_context(), 0, result);
	return diag::ok;
}
template<auto Cc>
static diag::lazy make_cmovccn(SemaContext) {
	auto ty		= ir::int_type(ins.effective_width);
	auto lhs		= read(sema_context(), 0, ty);
	auto rhs		= read(sema_context(), 1, ty);
	auto result = bb->push_select(Cc(sema_context()), lhs, rhs);
	write(sema_context(), 0, result);
	return diag::ok;
}

#define DECLARE_CONDITIONALS(CC, _)                                                             \
	DECL_SEMA(RC_CONCAT(J, CC)) { return make_jcc<RC_CONCAT(test_, CC)>(sema_context()); }       \
	DECL_SEMA(RC_CONCAT(JN, CC)) { return make_jccn<RC_CONCAT(test_, CC)>(sema_context()); }     \
	DECL_SEMA(RC_CONCAT(SET, CC)) { return make_setcc<RC_CONCAT(test_, CC)>(sema_context()); }   \
	DECL_SEMA(RC_CONCAT(SETN, CC)) { return make_setccn<RC_CONCAT(test_, CC)>(sema_context()); } \
	DECL_SEMA(RC_CONCAT(CMOV, CC)) { return make_cmovcc<RC_CONCAT(test_, CC)>(sema_context()); } \
	DECL_SEMA(RC_CONCAT(CMOVN, CC)) { return make_cmovccn<RC_CONCAT(test_, CC)>(sema_context()); }
VISIT_CONDITIONS(DECLARE_CONDITIONALS)


template<reg R>
static ir::insn* test_zero(SemaContext) {
	auto r = read_reg(sema_context(), R);
	return bb->push_cmp(ir::op::eq, r, ir::constant(r->get_type(), 0));
}
DECL_SEMA(JRCXZ) { return make_jcc<test_zero<reg::rcx>>(sema_context()); }
DECL_SEMA(JECXZ) { return make_jcc<test_zero<reg::ecx>>(sema_context()); }
DECL_SEMA(JCXZ)  { return make_jcc<test_zero<reg::cx>>(sema_context()); }

static ir::insn* dec_ntest(SemaContext) {
	reg		cx = reg::rcx;
	ir::type ty = ir::type::i64;
	if (mach->is_32())
		cx = reg::ecx, ty = ir::type::i32;
	else if (mach->is_16())
		cx = reg::cx, ty = ir::type::i16;

	// Count <- Count - 1
	auto cxv = read_reg(sema_context(), cx, ty);
	// CC = Count != 0
	auto ccv = bb->push_cmp(ir::op::ne, cxv, ir::constant(ty, 0));
	write_reg(sema_context(), cx, bb->push_binop(ir::op::sub, std::move(cxv), ir::constant(ty, 1)));
	return ccv;
}
DECL_SEMA(LOOP) {
	auto cc = dec_ntest(sema_context());
	// (Count!=0) -> Rel
	bb->push_xjs(cc, (ir::pointer) ins.op[0].i.get_unsigned(ip), (ir::pointer)(ip + ins.length));
	return diag::ok;
}
DECL_SEMA(LOOPE) {
	auto cc = dec_ntest(sema_context());
	auto zf = bb->push_read_reg(ir::type::i1, reg::flag_zf);
	cc		  = bb->push_binop(ir::op::bit_and, cc, zf);
	// (Count!=0 && ZF) -> Rel
	bb->push_xjs(cc, (ir::pointer) ins.op[0].i.get_unsigned(ip), (ir::pointer)(ip + ins.length));
	return diag::ok;
}
DECL_SEMA(LOOPNE) {
	auto cc = dec_ntest(sema_context());
	auto zf = bb->push_read_reg(ir::type::i1, reg::flag_zf);
	cc		  = bb->push_binop(ir::op::bit_and, cc, bb->push_unop(ir::op::bit_not, zf));
	// (Count!=0 && !ZF) -> Rel
	bb->push_xjs(cc, (ir::pointer) ins.op[0].i.get_unsigned(ip), (ir::pointer)(ip + ins.length));
	return diag::ok;
}