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
			bb->push_nop();
			return diag::ok;
		}
	}

	auto ty = ir::int_type(ins.effective_width);
	write(sema_context(), 0, read(sema_context(), 1, ty));
	return diag::ok;
}

// Atomic data transfer.
//
DECL_SEMA(CMPXCHG16B) {
	constexpr auto ty				  = ir::type::i128;
	auto				desired		  = read_pair(sema_context(), reg::rcx, reg::rbx);
	auto				comperand_val = read_pair(sema_context(), reg::rdx, reg::rax);
	ir::variant		prev;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto [adr, seg] = agen(sema_context(), ins.op[0].m);
		auto previ		 = bb->push_atomic_cmpxchg(seg, std::move(adr), comperand_val, desired);
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
	ir::variant prev;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto [adr, seg] = agen(sema_context(), ins.op[0].m);
		auto previ		 = bb->push_atomic_cmpxchg(seg, std::move(adr), comperand_val, desired);
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

	auto ty = ir::int_type(ins.effective_width);
	auto desired		 = read_reg(sema_context(), ins.op[1].r, ty);
	auto comperand_val = read_reg(sema_context(), comperand, ty);

	ir::variant prev;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto [adr, seg] = agen(sema_context(), ins.op[0].m);
		auto previ = bb->push_atomic_cmpxchg(seg, std::move(adr), comperand_val, desired);
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

		auto [adr, seg] = agen(sema_context(), mem);
		auto prev = bb->push_atomic_xchg(seg, std::move(adr), read_reg(sema_context(), reg, ty));
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
	i32  dif		 = ins.effective_width == 2 ? 2 : mach->ptr_width / 8;
	auto prev_sp = read_reg(sema_context(), rsp, pty);

	// Update SP.
	auto value	= read(sema_context(), 0, ty);
	auto new_sp = bb->push_binop(ir::op::sub, prev_sp, ir::constant(pty, dif));
	write_reg(sema_context(), rsp, new_sp);

	// Write the value.
	bb->push_store_mem(ir::NO_SEGMENT, bb->push_bitcast(ir::type::pointer, new_sp), std::move(value));
	return diag::ok;
}
DECL_SEMA(POP) {
	auto rsp		 = reg_sp(mach);
	auto pty		 = mach->ptr_type();
	auto ty		 = ir::int_type(ins.effective_width);
	i32  dif		 = ins.effective_width == 2 ? 2 : mach->ptr_width / 8;
	auto prev_sp = read_reg(sema_context(), rsp, pty);

	// Read the value.
	auto value = bb->push_load_mem(ty, ir::NO_SEGMENT, bb->push_bitcast(ir::type::pointer, prev_sp));

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