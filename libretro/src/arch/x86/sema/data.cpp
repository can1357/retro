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
			return diag::ok;
		}
	}

	auto ty = ir::int_type(ins.effective_width);
	write(sema_context(), 0, read(sema_context(), 1, ty));
	return diag::ok;
}

// Stack.
//
DECL_SEMA(PUSH) {
	auto rsp		 = reg_sp(mach);
	auto pty		 = mach->ptr_type();
	auto ty		 = ir::int_type(ins.effective_width);
	i32  dif		 = ins.effective_width == 2 ? 2 : mach->ptr_width / 8;
	auto prev_sp = read_reg(sema_context(), rsp, pty);

	// Update SP.
	auto value	= read(sema_context(), 0, ty);
	auto new_sp = bb->push_binop(ir::op::sub, prev_sp, ir::constant(pty, dif));
	write_reg(sema_context(), rsp, new_sp);

	// Write the value.
	bb->push_store_mem(ir::NO_SEGMENT, bb->push_cast(ir::type::pointer, new_sp), std::move(value));
	return diag::ok;
}
DECL_SEMA(POP) {
	auto rsp		 = reg_sp(mach);
	auto pty		 = mach->ptr_type();
	auto ty		 = ir::int_type(ins.effective_width);
	i32  dif		 = ins.effective_width == 2 ? 2 : mach->ptr_width / 8;
	auto prev_sp = read_reg(sema_context(), rsp, pty);

	// Read the value.
	auto value = bb->push_load_mem(ty, ir::NO_SEGMENT, bb->push_cast(ir::type::pointer, prev_sp));

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

// Vector moves.
//
DECL_SEMA(MOVUPS) {
	constexpr auto ty = ir::type::f32x4;
	write(sema_context(), 0, read(sema_context(), 1, ty));
	return diag::ok;
}
DECL_SEMA(MOVAPS) {
	constexpr auto ty = ir::type::f32x4;
	write(sema_context(), 0, read(sema_context(), 1, ty));
	return diag::ok;
}
DECL_SEMA(MOVUPD) {
	constexpr auto ty = ir::type::f64x2;
	write(sema_context(), 0, read(sema_context(), 1, ty));
	return diag::ok;
}
DECL_SEMA(MOVAPD) {
	constexpr auto ty = ir::type::f64x2;
	write(sema_context(), 0, read(sema_context(), 1, ty));
	return diag::ok;
}
DECL_SEMA(MOVDQU) {
	constexpr auto ty = ir::type::i32x4;
	write(sema_context(), 0, read(sema_context(), 1, ty));
	return diag::ok;
}
DECL_SEMA(MOVDQA) {
	constexpr auto ty = ir::type::i32x4;
	write(sema_context(), 0, read(sema_context(), 1, ty));
	return diag::ok;
}