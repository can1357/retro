#include <retro/platform.hpp>
#include <retro/format.hpp>

using namespace retro;

#include <retro/common.hpp>
#include <retro/targets.hxx>


#include <retro/arch/x86/zydis.hpp>

#include <retro/ir/insn.hpp>
#include <retro/ir/procedure.hpp>

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

	// Integral/Pointer size helper.
	//
	static ir::type int_type(size_t n) {
		switch (n) {
			case 1:
				return ir::type::i1;
			case 8:
				return ir::type::i8;
			case 16:
				return ir::type::i16;
			case 32:
				return ir::type::i32;
			case 64:
				return ir::type::i64;
			case 128:
				return ir::type::i128;
			default:
				return ir::type::none;
		}
	}
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
		ir::type rt = int_type(enum_reflect(ri).width);
		RC_ASSERT(rt != ir::type::none);
		return bb->push_read_reg(rt, ri);
	}
	static ir::insn* write_reg(ir::basic_block* bb, ZydisRegister r, ir::variant&& value, bool zeroupper = true) {
		RC_ASSERT(r != ZYDIS_REGISTER_NONE);
		RC_ASSERT(r != ZYDIS_REGISTER_RIP && r != ZYDIS_REGISTER_EIP && r != ZYDIS_REGISTER_IP);

		reg	ri	 = reg(r);
		if (zeroupper) {
			auto& inf = enum_reflect(ri);
			if (inf.alias != reg::none) {
				RC_ASSERT(inf.offset == 0);
				RC_ASSERT(value.get_type() == int_type(inf.width));
				auto new_size = enum_reflect(inf.alias).width;
				return bb->push_write_reg(inf.alias, bb->push_cast(int_type(new_size), std::move(value)));
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
	static ir::insn* write(ir::basic_block* bb, const zydis::decoded_ins& dec, u64 ip, size_t idx, ir::variant&& value, bool zeroupper = true) {
		switch (dec.ops[idx].type) {
			case ZYDIS_OPERAND_TYPE_REGISTER: {
				return write_reg(bb, dec.ops[idx].reg.value, std::move(value), zeroupper);
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
		auto ty = int_type(dec.ins.operand_width);
		write(bb, dec, ip, 0, read(bb, dec, ip, 1, ty));
		return diag::ok;
	}
	static diag::lazy lift_lea(ir::basic_block* bb, const zydis::decoded_ins& dec, u64 ip) {
		auto [ptr, seg] = agen(bb, dec, ip, 1, false);
		write_reg(bb, dec.ops[0].reg.value, std::move(ptr));
		return diag::ok;
	}
	static diag::lazy lift_add(ir::basic_block* bb, const zydis::decoded_ins& dec, u64 ip) {
		auto ty = int_type(dec.ins.operand_width);

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
		// Switch based on the mnemonic:
		//
		auto prev = std::prev(bb->end(), bb->empty() ? 0 : 1).at;
		switch (dec.ins.mnemonic) {
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

/*
test rcx, rcx
jz   x
  lea rax, [rdx+rcx]
  ret
x:
  lea rax, [rdx+r8]
  ret
*/
constexpr const char lift_example[] = "\x48\x85\xC9\x74\x05\x48\x8D\x04\x0A\xC3\x4A\x8D\x04\x02\xC3";

#include <nt/image.hpp>

int main(int argv, const char** args) {
	platform::setup_ansi_escapes();

	auto	proc = make_rc<ir::procedure>();
	auto* bb	  = proc->add_block();

	/*auto i0 = bb->push_binop(ir::op::add, 2, 3);
	auto i1 = bb->push_binop(ir::op::add, 3, i0);
	auto i2 = bb->push_cast(ir::type::i16, i1);
	x86::sema::set_pf(bb, i2);
	x86::sema::set_zf(bb, i2);
	x86::sema::set_sf(bb, i2);

	fmt::println(proc->to_string());*/

	u8 test[] = {0x48, 0x8D, 0x04, 0x0A, 0x48, 0x8D, 0x0D, 0x01, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x1C, 0x88, 0x48, 0x01, 0x0B, 0xF0, 0x48, 0x01, 0x0B, 0xB9, 0x02, 0x00, 0x00, 0x00};
	std::span<const u8> data	= test;

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
	bb->erase_if([](ir::insn* i) { return !i->uses() && !i->desc().side_effects; });


	fmt::println(bb->to_string());
}