#include <retro/platform.hpp>
#include <retro/format.hpp>

using namespace retro;

#include <retro/common.hpp>
#include <retro/targets.hxx>


#include <retro/arch/x86/zydis.hpp>

#include <retro/ir/insn.hpp>
#include <retro/ir/routine.hpp>

namespace retro::doc {
	// Image type.
	//
	struct image {
		// TODO: ??
		//
	};

	// Workspace type.
	//
	struct workspace {
		// TODO: ??
		//
	};
};

namespace retro::debug {
	static void print_insn_list() {
		std::string_view tmp_types[] = {"T", "Ty"};

		for (auto& ins : ir::opcode_desc::list()) {
			if (ins.id() == ir::opcode::none)
				continue;

			std::string_view ret_type = "!";
			if (!ins.terminator) {
				if (ins.templates.begin()[0] == 0) {
					ret_type = enum_name(ins.types[0]);
				} else {
					ret_type = tmp_types[ins.templates[0] - 1];
				}
			}

			fmt::print(RC_RED, ret_type, " " RC_YELLOW, ins.name);
			if (ins.template_count == 1) {
				fmt::print(RC_BLUE "<T>");
			} else if (ins.template_count == 2) {
				fmt::print(RC_BLUE "<T,Ty>");
			}
			fmt::print(" " RC_RESET);

			size_t num_args = ins.types.size() - 1;
			for (size_t i = 0; i != num_args; i++) {
				auto tmp	 = ins.templates[i + 1];
				auto type = ins.types[i + 1];
				auto name = ins.names[i + 1];

				std::string_view ty;
				if (tmp == 0) {
					ty = enum_name(type);
				} else {
					ty = tmp_types[tmp - 1];
				}

				if (range::count(ins.constexprs, i + 1)) {
					fmt::print(RC_GREEN "constexpr ");
				}
				fmt::print(RC_RED, ty, RC_RESET ":" RC_WHITE, name, RC_RESET " ");
			}
			fmt::print("\n");
		}
	}
};

#include <retro/arch/x86/zydis.hpp>
#include <retro/ir/basic_block.hpp>
#include <retro/ir/insn.hpp>
#include <retro/diag.hpp>

namespace retro::x86::sema {
	// Errors.
	//
	RC_DEF_ERR(unhandled_insn, "unhandled instruction: %");

	// Common helpers for parity, sign, zero and auxiliary carry flags.
	//
	static void set_pf(ir::basic_block* bb, ir::insn* result) {
		auto cast = bb->push_cast(ir::type::i8, result);
		auto pcnt = bb->push_unop(ir::op::bit_popcnt, cast);
		auto mod2 = bb->push_cast(ir::type::i1, pcnt);
		bb->push_write_reg(reg::flag_pf, mod2);
	}
	static void set_sf(ir::basic_block* bb, ir::insn* result) {
		auto sgn = bb->push_cmp(ir::op::lt, result, ir::constant(result->get_type(), 0));
		bb->push_write_reg(reg::flag_sf, sgn);
	}
	static void set_zf(ir::basic_block* bb, ir::insn* result) {
		auto is_zero = bb->push_cmp(ir::op::eq, result, ir::constant(result->get_type(), 0));
		bb->push_write_reg(reg::flag_zf, is_zero);
	}
	template<typename Lhs, typename Rhs>
	static void set_af(ir::basic_block* bb, Lhs&& lhs, Rhs&& rhs, ir::insn* result) {
		auto tmp		= bb->push_binop(ir::op::bit_xor, std::forward<Lhs>(lhs), std::forward<Rhs>(rhs));
		tmp			= bb->push_binop(ir::op::bit_xor, tmp, result);
		tmp			= bb->push_binop(ir::op::bit_and, tmp, ir::constant(tmp->get_type(), 0x10));
		auto is_set = bb->push_cmp(ir::op::ne, tmp, ir::constant(tmp->get_type(), 0));
		bb->push_write_reg(reg::flag_af, is_set);
	}
	template<typename Lhs, typename Rhs, typename Carry>
	static void set_af(ir::basic_block* bb, Lhs&& lhs, Rhs&& rhs, Carry&& carry, ir::insn* result) {
		auto tmp = bb->push_binop(ir::op::bit_xor, result, std::forward<Carry>(carry));
		return set_af(bb, std::forward<Lhs>(lhs), std::forward<Rhs>(rhs), tmp);
	}
	// TODO: set_cf
	// TODO: set_of

	// Pointer helpers.
	//
	static ir::type ptr_type(const zydis::decoded_ins& dec) {
		switch (dec.ins.machine_mode) {
			default:
			case ZYDIS_MACHINE_MODE_LONG_64:
				return ir::type::i64;
			case ZYDIS_MACHINE_MODE_LEGACY_32:
			case ZYDIS_MACHINE_MODE_LONG_COMPAT_32:
				return ir::type::i32;
			case ZYDIS_MACHINE_MODE_LONG_COMPAT_16:
			case ZYDIS_MACHINE_MODE_LEGACY_16:
			case ZYDIS_MACHINE_MODE_REAL_16:
				return ir::type::i16;
		}
	}
	static ir::segment map_seg(ZydisRegister seg) {
		if (seg == ZYDIS_REGISTER_GS) {
			return ir::segment(1);
		} else if (seg == ZYDIS_REGISTER_FS) {
			return ir::segment(2);
		}
		return ir::NO_SEGMENT;
	}


	// Register read/write helper.
	//
	static ir::insn* read_reg(ir::basic_block* bb, ZydisRegister r) {
		RC_ASSERT(r != ZYDIS_REGISTER_NONE);
		RC_ASSERT(r != ZYDIS_REGISTER_RIP && r != ZYDIS_REGISTER_EIP && r != ZYDIS_REGISTER_IP);

		reg		ri = reg(r);
		ir::type rt = ir::int_type(enum_reflect(ri).width);
		RC_ASSERT(rt != ir::type::none);
		return bb->push_read_reg(rt, ri);
	}
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

	// Address generation helper.
	//
	static std::pair<ir::variant, ir::segment> agen(ir::basic_block* bb, const zydis::decoded_ins& dec, u64 ip, size_t idx, bool as_ptr = true) {
		auto& op = dec.ops[idx];
		auto	pty = ptr_type(dec);

		if (op.type == ZYDIS_OPERAND_TYPE_POINTER) {
			return {ir::constant(as_ptr ? ir::type::pointer : pty, op.ptr.offset), ir::NO_SEGMENT};
		}
		RC_ASSERT(op.type == ZYDIS_OPERAND_TYPE_MEMORY);
		auto m = op.mem;

		// Handle RIP.
		//
		if (m.base == ZYDIS_REGISTER_RIP) {
			m.base = ZYDIS_REGISTER_NONE;
			if (m.disp.has_displacement) {
				m.disp.value += (i64) ip;
			} else {
				m.disp.value += (i64) ip;
			}
		}

		// [base+...]
		ir::insn* result = nullptr;
		if (m.base) {
			result = read_reg(bb, m.base);
		}

		// [index*scale+...]
		if (m.index && m.scale != 0) {
			auto idx = read_reg(bb, m.index);
			if (m.scale != 1)
				idx = bb->push_binop(ir::op::mul, idx, ir::constant(pty, m.scale));
			if (result) {
				result = bb->push_binop(ir::op::add, result, idx);
			} else {
				result = idx;
			}
		}

		// [...+disp]
		if (!result) {
			// gsbase/fsbase requires fs:|gs: instead of value, so this will always map to uniform memory under x86_64.
			//
			return {ir::constant(as_ptr ? ir::type::pointer : pty, m.disp.has_displacement ? m.disp.value : 0), map_seg(m.segment)};
		} else if (m.disp.has_displacement) {
			result = bb->push_binop(ir::op::add, result, ir::constant(pty, m.disp.value));
		}

		// Finally cast to pointer and return.
		//
		if (as_ptr) {
			result = bb->push_cast(ir::type::pointer, result);
		}

		return {result, map_seg(m.segment)};
	}

	// Memory read/write helper.
	//
	static ir::insn* read_mem(ir::basic_block* bb, const zydis::decoded_ins& dec, u64 ip, size_t idx, ir::type ty) {
		auto [ptr, seg] = agen(bb, dec, ip, idx);
		return bb->push_load_mem(ty, seg, std::move(ptr));
	}
	static ir::insn* write_mem(ir::basic_block* bb, const zydis::decoded_ins& dec, u64 ip, size_t idx, ir::variant&& value) {
		auto [ptr, seg] = agen(bb, dec, ip, idx);
		return bb->push_store_mem(seg, std::move(ptr), std::move(value));
	}

	// Common read/write helper.
	//
	static ir::variant read(ir::basic_block* bb, const zydis::decoded_ins& dec, u64 ip, size_t idx, ir::type ty) {
		switch (dec.ops[idx].type) {
			case ZYDIS_OPERAND_TYPE_REGISTER: {
				auto reg = read_reg(bb, dec.ops[idx].reg.value);
				if (reg->get_type() != ty) {
					reg = bb->push_cast(ty, reg);
				}
				return reg;
			}
			case ZYDIS_OPERAND_TYPE_POINTER:
			case ZYDIS_OPERAND_TYPE_MEMORY: {
				return read_mem(bb, dec, ip, idx, ty);
			}
			case ZYDIS_OPERAND_TYPE_IMMEDIATE: {
				if (dec.ops[idx].imm.is_signed) {
					return ir::constant(ty, dec.ops[idx].imm.value.s);
				} else {
					return ir::constant(ty, dec.ops[idx].imm.value.u);
				}
			}
			default:
				// TODO: Diag
				fmt::println("Idk how to handle read to oeprand type: ", dec.ops[idx].type);
				fmt::abort_no_msg();
				return nullptr;
		}
	}
	static ir::insn* write(ir::basic_block* bb, const zydis::decoded_ins& dec, u64 ip, size_t idx, ir::variant&& value) {
		switch (dec.ops[idx].type) {
			case ZYDIS_OPERAND_TYPE_REGISTER: {
				return write_reg(bb, dec, dec.ops[idx].reg.value, std::move(value));
			}
			case ZYDIS_OPERAND_TYPE_POINTER:
			case ZYDIS_OPERAND_TYPE_MEMORY: {
				return write_mem(bb, dec, ip, idx, std::move(value));
			}
			default:
				// TODO: Diag
				fmt::println("Idk how to handle write to oeprand type: ", dec.ops[idx].type);
				fmt::abort_no_msg();
				return nullptr;
		}
	}

	// Instruction handlers.
	//
	static diag::lazy lift_mov(ir::basic_block* bb, const zydis::decoded_ins& dec, u64 ip) {
		auto ty = ir::int_type(dec.ins.operand_width);
		write(bb, dec, ip, 0, read(bb, dec, ip, 1, ty));
		return diag::ok;
	}
	static diag::lazy lift_lea(ir::basic_block* bb, const zydis::decoded_ins& dec, u64 ip) {
		auto [ptr, seg] = agen(bb, dec, ip, 1, false);
		write_reg(bb, dec, dec.ops[0].reg.value, std::move(ptr));
		return diag::ok;
	}
	static diag::lazy lift_add(ir::basic_block* bb, const zydis::decoded_ins& dec, u64 ip) {
		auto ty = ir::int_type(dec.ins.operand_width);

		auto rhs = read(bb, dec, ip, 1, ty);

		ir::insn* result;
		ir::variant lhs;
		if (dec.ins.attributes & ZYDIS_ATTRIB_HAS_LOCK) {
			auto [ptr, seg] = agen(bb, dec, ip, 0, true);

			lhs	 = bb->push_atomic_binop(ir::op::add, seg, std::move(ptr), rhs);
			result = bb->push_binop(ir::op::add, lhs, rhs);
		} else {
			lhs	 = read(bb, dec, ip, 0, ty);
			result = bb->push_binop(ir::op::add, lhs, rhs);
			write(bb, dec, ip, 0, result);
		}

		set_af(bb, lhs, rhs, result);
		set_sf(bb, result);
		set_zf(bb, result);
		set_pf(bb, result);
		bb->push_write_reg(reg::flag_of, bb->push_poison(ir::type::i1, "Overflow flag NYI"));	// TODO
		bb->push_write_reg(reg::flag_cf, bb->push_poison(ir::type::i1, "Carry flag NYI"));		// TODO
		return diag::ok;
	}

	// Common lifter switch.
	//
	diag::lazy lift(ir::basic_block* bb, const zydis::decoded_ins& dec, u64 ip) {

		// TODO: Special handling:
		//  xor  eax,  eax
		//  pxor xmm0, xmm0
		//  lock or [rsp], 0
		//
		//
		//

		// Switch based on the mnemonic:
		//
		auto prev = std::prev(bb->end(), bb->empty() ? 0 : 1).at;
		switch (dec.ins.mnemonic) {
			case ZYDIS_MNEMONIC_NOP:
				return diag::ok;
			case ZYDIS_MNEMONIC_MOV:
				if (auto err = lift_mov(bb, dec, ip))
					return err;
				break;
			case ZYDIS_MNEMONIC_LEA:
				if (auto err = lift_lea(bb, dec, ip))
					return err;
				break;
			case ZYDIS_MNEMONIC_ADD:
				if (auto err = lift_add(bb, dec, ip))
					return err;
				break;
			default:
				return err::unhandled_insn(dec.to_string(ip - dec.ins.length));
		}

		// Mark the range with the ip prior to execution.
		//
		for (auto* ins : view::reverse(bb->insns())) {
			if (ins == prev)
				break;
			ins->ip = ip - dec.ins.length;
		}
		return diag::ok;
	}
};

#include <nt/image.hpp>

#include <filesystem>
#include <fstream>

// TODO: retro::fs
static std::optional<std::vector<u8>> read_file(const std::filesystem::path& path) {
	std::optional<std::vector<u8>> buffer;
	std::ifstream file(path, std::ios::binary);
	if (!file.good())
		return buffer;

	file.seekg(0, std::ios_base::end);
	buffer.emplace().resize(file.tellg());
	file.seekg(0, std::ios_base::beg);
	file.read((char*)buffer->data(), buffer->size());
	return buffer;
}

#include <retro/robin_hood.hpp>

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

int main(int argv, const char** args) {
	platform::setup_ansi_escapes();

	/*
	*
	* TODO TMW:
		figure out the arch bullshit
		basic! image loader
		properly design the lifter func, cfg exploration

		Conversion to actual SSA format:
	-> move propagate + phi gen where possible and no aliasing (pass #1)
	-> lower to move into super-reg, aka all write_regs are full size  (pass #2)
	-> run pass #1 again to finally get rid of all read_regs besides args / retval

		convert write_reg to stacksave and read_reg to stackrestore

		Include z3 for:
		 - constraint solving while cfg opt
		 - range resolution for jump table:
		https://stackoverflow.com/a/63374651
		 - simplification while in aggressive mode

		For all else:
		 - implement inst combine like thing for basic pattern matching: (x==-x -> x==0) and so on

	*/

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

	auto	rtn	 = make_rc<ir::routine>();
	rtn->start_ip = 0x140000000;
	auto* bb		 = rtn->add_block();
	u8 test[] = {0x90, 0x48, 0x8D, 0x04, 0x0A, 0x48, 0x8D, 0x0D, 0x01, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x1C, 0x88, 0x48, 0x01, 0x0B, 0xF0, 0x48, 0x01, 0x0B, 0xB9, 0x02, 0x00, 0x00, 0x00};


	// TODO: BB splitting

	/*
	add, sub, and, or, xor, shl, shr...
	bt
	cpuid, xgetbv
	jcc, setcc
	cmp, test
	movzx, movsxd
	push, pop
	call jmp ret
	*/

	std::span<const u8> data = test;
	while (true) {
		auto i = zydis::decode(data);
		if (!i) {
			break;
		}



		u64 rip = 0x140000000 + (data.data() - test);
		fmt::println((void*)(rip - i->ins.length), ": ", i->to_string(rip - i->ins.length));
		x86::sema::lift(bb, i.value(), rip).raise();
	}


	// DCE
	bb->erase_if([](ir::insn* i) { return !i->uses() && !i->desc().side_effect; });


	fmt::println(rtn->to_string());
}