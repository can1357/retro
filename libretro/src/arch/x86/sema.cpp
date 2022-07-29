#include <retro/common.hpp>
#include <retro/arch/x86/sema.hpp>

using namespace retro;
using namespace retro::arch::x86;

// TODO: Special handling:
//  xor  eax,  eax
//  pxor xmm0, xmm0
//  lock or [rsp], 0
//  mov eax, eax
//  lea eax, [eax]
//
//

/*
	static diag::lazy lift_add(ir::basic_block* bb, const zydis::decoded_ins& dec, u64 ip) {
		auto ty = ir::int_type(dec.ins.operand_width);

		auto rhs = read(sema_context(), 1, ty);

		ir::insn* result;
		ir::variant lhs;
		if (dec.ins.attributes & ZYDIS_ATTRIB_HAS_LOCK) {
			auto [ptr, seg] = agen(sema_context(), 0, true);

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
		bb->push_write_reg(reg::flag_of, bb->push_poison(ir::type::i1, "Overflow flag NYI"));	// TODO
		bb->push_write_reg(reg::flag_cf, bb->push_poison(ir::type::i1, "Carry flag NYI"));		// TODO
		return diag::ok;
	}
*/


// Declare semantics.
//
DECL_SEMA(NOP) {
	return diag::ok;
}
DECL_SEMA(MOV) {
	auto ty = ir::int_type(ins.effective_width);
	write(sema_context(), 0, read(sema_context(), 1, ty));
	return diag::ok;
}
DECL_SEMA(LEA) {
	auto [ptr, seg] = agen(sema_context(), ins.op[1].m, false);
	write_reg(sema_context(), ins.op[0].r, std::move(ptr));
	return diag::ok;
}

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
mov -> 7439 35.195873%
jz -> 2855 13.507759%
lea -> 1473 6.969152%
cmp -> 1405 6.647426%
call -> 1170 5.535579%
add -> 949 4.489970%
movzx -> 834 3.945874%
test -> 750 3.548448%
sub -> 579 2.739402%
pop -> 571 2.701552%
push -> 489 2.313588%
xor -> 332 1.570780%
ret -> 299 1.414648%
and -> 251 1.187547%
movups -> 231 1.092922%
nop -> 221 1.045609%
inc -> 154 0.728615%
int3 -> 112 0.529902%
shr -> 111 0.525170%
dec -> 94 0.444739%
movsd -> 85 0.402157%
or -> 71 0.335920%
cmovb -> 67 0.316995%
movsxd -> 57 0.269682%
xadd -> 46 0.217638%
shl -> 43 0.203444%
setz -> 42 0.198713%
movaps -> 34 0.160863%
bts -> 28 0.132475%
xorps -> 24 0.113550%
sar -> 21 0.099357%
cmovnb -> 21 0.099357%
cmovnz -> 20 0.094625%
imul -> 19 0.089894%
cmovz -> 19 0.089894%
bt -> 15 0.070969%
movq -> 14 0.066238%
movsx -> 13 0.061506%
cdqe -> 13 0.061506%
not -> 13 0.061506%
rol -> 13 0.061506%
movdqa -> 11 0.052044%
neg -> 10 0.047313%
mul -> 10 0.047313%
clc -> 10 0.047313%
movss -> 9 0.042581%
cqo -> 8 0.037850%
cvtsi2ss -> 6 0.028388%
psrldq -> 6 0.028388%
sbb -> 5 0.023656%
movdqu -> 5 0.023656%
movd -> 5 0.023656%
xchg -> 4 0.018925%
cvtsi2sd -> 4 0.018925%
in -> 4 0.018925%
cmovs -> 4 0.018925%
cpuid -> 3 0.014194%
stosw -> 3 0.014194%
adc -> 3 0.014194%
cvtdq2pd -> 2 0.009463%
cvtdq2ps -> 2 0.009463%
cvtps2pd -> 2 0.009463%
leave -> 2 0.009463%
int -> 2 0.009463%
cmovbe -> 2 0.009463%
loopne -> 1 0.004731%
lodsb -> 1 0.004731%
addsd -> 1 0.004731%
cmovns -> 1 0.004731%
cmovnbe -> 1 0.004731%
addss -> 1 0.004731%
popfq -> 1 0.004731%
scasd -> 1 0.004731%
ror -> 1 0.004731%
xgetbv -> 1 0.004731%
divss -> 1 0.004731%
cmpxchg -> 1 0.004731%
div -> 1 0.004731%
cmovl -> 1 0.004731%
outsb -> 1 0.004731%
stosd -> 1 0.004731%
out -> 1 0.004731%
*/
