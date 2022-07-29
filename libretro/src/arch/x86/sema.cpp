#include <retro/common.hpp>
#include <retro/arch/x86/sema.hpp>

using namespace retro;
using namespace retro::arch::x86;

// TODO: Special handling:
//  pxor xmm0, xmm0
//  lock or [rsp], 0
//
//

// Declare semantics.
//
DECL_SEMA(NOP) {
	return diag::ok;
}
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
DECL_SEMA(PUSH) {
	auto rsp	 = reg_sp(mach);
	auto pty		 = mach->ptr_type();
	auto ty		 = ir::int_type(ins.effective_width);	// TODO: Test
	i32  dif		 = ins.effective_width == 2 ? 2 : mach->ptr_width / 8;
	auto prev_sp = read_reg(sema_context(), rsp, pty);

	// Update SP.
	auto value	 = read(sema_context(), 0, ty);
	auto new_sp	 = bb->push_binop(ir::op::sub, prev_sp, ir::constant(pty, dif));
	write_reg(sema_context(), rsp, new_sp);

	// Write the value.
	bb->push_store_mem(ir::NO_SEGMENT, bb->push_cast(ir::type::pointer, new_sp), std::move(value));
	return diag::ok;
}
DECL_SEMA(POP) {
	auto rsp	 = reg_sp(mach);
	auto pty		 = mach->ptr_type();
	auto ty		 = ir::int_type(ins.effective_width);	// TODO: Test
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

// Arithmetic.
//
DECL_SEMA(ADD) {
	auto ty	= ir::int_type(ins.effective_width);
	auto rhs = read(sema_context(), 1, ty);

	ir::insn*	result;
	ir::variant lhs;
	if (ins.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
		auto [ptr, seg] = agen(sema_context(), ins.op[0].m, true);
		lhs	 = bb->push_atomic_binop(ir::op::add, seg, std::move(ptr), rhs);
		result = bb->push_binop(ir::op::add, lhs, rhs);
	} else {
		lhs	 = read(sema_context(), 0, ty);
		result = bb->push_binop(ir::op::add, lhs, rhs);
		write(sema_context(), 0, result);
	}

	set_af(bb, lhs, rhs, result);
	set_sf(bb, result);
	set_zf(bb, result);
	set_pf(bb, result);
	bb->push_write_reg(reg::flag_of, bb->push_poison(ir::type::i1, "ADD - Overflow flag NYI"));	// TODO
	auto c0 = bb->push_cmp(ir::op::ult, result, lhs);
	auto c1 = bb->push_cmp(ir::op::ult, result, rhs);
	bb->push_write_reg(reg::flag_cf, bb->push_binop(ir::op::bit_or, c0, c1));
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
	bb->push_write_reg(reg::flag_of, bb->push_poison(ir::type::i1, "SUB - Overflow flag NYI"));	// TODO
	bb->push_write_reg(reg::flag_cf, bb->push_cmp(ir::op::ult, lhs, rhs));
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
	bb->push_write_reg(reg::flag_of, bb->push_poison(ir::type::i1, "INC - Overflow flag NYI"));	// TODO
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
	bb->push_write_reg(reg::flag_of, bb->push_poison(ir::type::i1, "SUB - Overflow flag NYI"));	// TODO
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
	bb->push_write_reg(reg::flag_of, bb->push_poison(ir::type::i1, "SUB/NEG - Overflow flag NYI"));	 // TODO
	return diag::ok;
}

// Logical.
//
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
	auto ty	= ir::int_type(ins.effective_width);

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

// Comparisons.
//
DECL_SEMA(CMP) {
	auto ty		= ir::int_type(ins.effective_width);
	auto lhs		= read(sema_context(), 0, ty);
	auto rhs		= read(sema_context(), 1, ty);
	auto result = bb->push_binop(ir::op::sub, lhs, rhs);
	set_af(bb, lhs, rhs, result);
	set_sf(bb, result);
	set_zf(bb, result);
	set_pf(bb, result);
	bb->push_write_reg(reg::flag_of, bb->push_poison(ir::type::i1, "SUB - Overflow flag NYI"));	// TODO
	bb->push_write_reg(reg::flag_cf, bb->push_cmp(ir::op::ult, lhs, rhs));
	return diag::ok;
}
DECL_SEMA(TEST) {
	auto ty		= ir::int_type(ins.effective_width);
	auto lhs		= read(sema_context(), 0, ty);
	auto rhs		= read(sema_context(), 1, ty);
	auto result = bb->push_binop(ir::op::bit_and, lhs, rhs);
	set_flags_logical(bb, result);
	return diag::ok;
}

// Conditionals.
//
#define VISIT_CONDITIONS(_) _(Z, NZ) _(S, NS) _(B, NB) _(BE, NBE) _(L, NL) _(LE, NLE) _(O, NO) _(P, NP)
// TODO: CXZ

// (ZF = 1)
static ir::insn* test_Z(SemaContext) { return bb->push_read_reg(ir::type::i1, reg::flag_zf); }
// (SF = 1)
static ir::insn* test_S(SemaContext) { return bb->push_read_reg(ir::type::i1, reg::flag_sf); }
// (CF=1)
static ir::insn* test_B(SemaContext) { return bb->push_read_reg(ir::type::i1, reg::flag_cf); }
// (PF=1)
static ir::insn* test_P(SemaContext) { return bb->push_read_reg(ir::type::i1, reg::flag_pf); }
// (OF=1)
static ir::insn* test_O(SemaContext) { return bb->push_read_reg(ir::type::i1, reg::flag_of); }
// (SF≠OF)
static ir::insn* test_L(SemaContext) { return bb->push_binop(ir::op::bit_xor, test_O(sema_context()), test_S(sema_context())); }
// (CF=1 or ZF=1)
static ir::insn* test_BE(SemaContext) { return bb->push_binop(ir::op::bit_or, test_B(sema_context()), test_Z(sema_context())); }
// (SF≠OF or ZF=1)
static ir::insn* test_LE(SemaContext) { return bb->push_binop(ir::op::bit_or, test_L(sema_context()), test_Z(sema_context())); }

template<auto Cc>
inline diag::lazy make_jcc(SemaContext) {
	bb->push_xjs(Cc(sema_context()), read(sema_context(), 0, ir::type::pointer), ir::constant(ir::type::pointer, ip + ins.length));
	return diag::ok;
}
template<auto Cc>
inline diag::lazy make_jccn(SemaContext) {
	bb->push_xjs(Cc(sema_context()), ir::constant(ir::type::pointer, ip + ins.length), read(sema_context(), 0, ir::type::pointer));
	return diag::ok;
}
template<auto Cc>
inline diag::lazy make_setcc(SemaContext) {
	auto result = bb->push_cast(ir::type::i8, Cc(sema_context()));
	write(sema_context(), 0, result);
	return diag::ok;
}
template<auto Cc>
inline diag::lazy make_setccn(SemaContext) {
	auto result = bb->push_cast(ir::type::i8, Cc(sema_context()));
	write(sema_context(), 0, bb->push_binop(ir::op::bit_xor, result, ir::constant((i8) 1)));
	return diag::ok;
}
template<auto Cc>
inline diag::lazy make_cmovcc(SemaContext) {
	auto ty		= ir::int_type(ins.effective_width);
	auto lhs		= read(sema_context(), 0, ty);
	auto rhs		= read(sema_context(), 1, ty);
	auto result = bb->push_select(test_Z(sema_context()), rhs, lhs);
	write(sema_context(), 0, result);
	return diag::ok;
}
template<auto Cc>
inline diag::lazy make_cmovccn(SemaContext) {
	auto ty		= ir::int_type(ins.effective_width);
	auto lhs		= read(sema_context(), 0, ty);
	auto rhs		= read(sema_context(), 1, ty);
	auto result = bb->push_select(test_Z(sema_context()), lhs, rhs);
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

// CALL / JMP / RET.
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
		auto rsp	 = reg_sp(mach);
		auto pty = mach->ptr_type();
		auto prev_sp = read_reg(sema_context(), rsp, pty);
		auto new_sp	 = bb->push_binop(ir::op::add, prev_sp, ir::constant(pty, ins.op[0].i.s));
		write_reg(sema_context(), rsp, new_sp);
	}
	bb->push_ret(std::nullopt);
	return diag::ok;
}

// INT1 / INT3 / UD2
//
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

#if 0
	static ir::insn* write_reg(ir::basic_block* bb, const zydis::decoded_ins& dec, ZydisRegister r, ir::variant&& value, bool no_zeroupper = false) {
		RC_ASSERT(r != ZYDIS_REGISTER_NONE);
		RC_ASSERT(r != ZYDIS_REGISTER_RIP && r != ZYDIS_REGISTER_EIP && r != ZYDIS_REGISTER_IP);

		reg ri = reg(r);
		if (!no_zeroupper) {
			auto& inf = enum_reflect(ri);
			if (inf.alias != reg::none) {
				bool gpr_ext = inf.kind == reg_kind::general && inf.width == 32 && dec.ins.machine_mode == ZYDIS_MACHINE_MODE_LONG_64;
				bool vex_ext = inf.kind == reg_kind::vector && (dec.ins.attributes & (ZYDIS_ATTRIB_HAS_EVEX | ZYDIS_ATTRIB_HAS_VEX));
				if (gpr_ext || vex_ext) {
					RC_ASSERT(inf.offset == 0);
					RC_ASSERT(value.get_type() == ir::int_type(inf.width));
					auto new_size = enum_reflect(inf.alias).width;
					return bb->push_write_reg(inf.alias, bb->push_cast(ir::int_type(new_size), std::move(value)));
				}
			}
		}
		return bb->push_write_reg(ri, std::move(value));
	}
#endif


	//{
//	u32				 total	 = 0;
//	std::unique_ptr ins_freq = std::make_unique<u32[]>(ZYDIS_MNEMONIC_MAX_VALUE);
//
//	auto sample_file = [&](const std::filesystem::path& path) {
//		if (auto buffer = read_file(path)) {
//			auto* img	= (win::image_x64_t*) buffer->data();
//			auto	exdir = img->get_directory(win::directory_entry_exception);
//			for (auto& f : win::exception_directory(img->rva_to_ptr(exdir->rva), exdir->size)) {
//				std::span data{img->rva_to_ptr<const u8>(f.rva_begin), f.rva_end - f.rva_begin};
//				while (auto i = zydis::decode(data)) {
//					if (i->ins.attributes & (ZYDIS_ATTRIB_HAS_VEX | ZYDIS_ATTRIB_HAS_EVEX | ZYDIS_ATTRIB_IS_PRIVILEGED))
//						continue;
//					if (i->ins.meta.category == ZYDIS_CATEGORY_X87_ALU || i->ins.meta.isa_ext == ZYDIS_ISA_EXT_X87)
//						continue;
//
//					if (ZYDIS_MNEMONIC_JB <= i->ins.mnemonic && i->ins.mnemonic <= ZYDIS_MNEMONIC_JZ)
//						i->ins.mnemonic = ZYDIS_MNEMONIC_JZ;
//					if (ZYDIS_MNEMONIC_SETB <= i->ins.mnemonic && i->ins.mnemonic <= ZYDIS_MNEMONIC_SETZ)
//						i->ins.mnemonic = ZYDIS_MNEMONIC_SETZ;
//
//					total++;
//					ins_freq[i->ins.mnemonic]++;
//				}
//			}
//		}
//	};
//	sample_file(args[0]);
//
//
//	std::vector<std::pair<ZydisMnemonic, u32>> ins_present;
//	for (auto i = 0; i != ZYDIS_MNEMONIC_MAX_VALUE; i++) {
//		if (u32 n = ins_freq[i]) {
//			ins_present.emplace_back(ZydisMnemonic(i), n);
//		}
//	}
//	range::sort(ins_present, [](auto& a, auto& b) { return a.second >= b.second; });
//
//	for (auto& [m, n] : ins_present) {
//		fmt::println(ZydisMnemonicGetString(m), " -> ", n, " ", float(100 * n) / float(total), "%");
//	}
//}
/*
popfq -> 1 0.004731%

xchg -> 4 0.018925%
movzx -> 834 3.945874%
movsx -> 13 0.061506%
movsxd -> 57 0.269682%
movsd -> 85 0.402157%
xorps -> 24 0.113550%
cmpxchg -> 1 0.004731%

xadd -> 46 0.217638%
shr -> 111 0.525170%
shl -> 43 0.203444%
sar -> 21 0.099357%
bts -> 28 0.132475%
bt -> 15 0.070969%
mul -> 10 0.047313%
imul -> 19 0.089894%
rol -> 13 0.061506%
ror -> 1 0.004731%
sbb -> 5 0.023656%
adc -> 3 0.014194%
div -> 1 0.004731%
cdqe -> 13 0.061506%
clc -> 10 0.047313%
cqo -> 8 0.037850%


movq -> 14 0.066238%
movd -> 5 0.023656%
movss -> 9 0.042581%
movups -> 231 1.092922%
movdqa -> 11 0.052044%
movaps -> 34 0.160863%
movdqu -> 5 0.023656%
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
outsb -> 1 0.004731%
in -> 4 0.018925%
out -> 1 0.004731%
*/
