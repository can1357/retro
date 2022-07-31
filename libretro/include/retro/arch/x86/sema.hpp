#pragma once
#include <retro/arch/minsn.hpp>
#include <retro/arch/x86.hpp>
#include <retro/arch/x86/zy2rc.hpp>
#include <retro/ir/basic_block.hpp>
#include <retro/ir/insn.hpp>
#include <retro/ir/routine.hpp>
#include <retro/ir/basic_block.hpp>

// Private header for all units declaring semantics.
//
namespace retro::arch::x86 {
	// Shorthand for passing all lifter context.
	// - Macro instead of a struct to not be slown down horribly under MSABI.
	//
	#define sema_context() mach, bb, ins, ip
	#define SemaContext    arch::x86arch* mach, ir::basic_block * bb, const arch::minsn& ins, u64 ip

	// Lifter table.
	//
	using fn_lifter													  = diag::lazy (*)(SemaContext);
	inline fn_lifter lifter_table[ZYDIS_MNEMONIC_MAX_VALUE] = {nullptr};

// Lifter declaration.
//
#define DECL_SEMA(mnemonic)                                                             \
	static diag::lazy RC_CONCAT(lift_, mnemonic)(SemaContext);                           \
	RC_INITIALIZER {                                                                     \
		auto& entry = arch::x86::lifter_table[u32(RC_CONCAT(ZYDIS_MNEMONIC_, mnemonic))]; \
		RC_ASSERT(!entry);                                                                \
		entry = &RC_CONCAT(lift_, mnemonic);                                              \
	};                                                                                   \
	static diag::lazy RC_CONCAT(lift_, mnemonic)(SemaContext)

	// Common helpers for parity, sign, zero and auxiliary carry flags.
	//
	inline void set_pf(ir::basic_block* bb, ir::insn* result) {
		auto cast = bb->push_cast(ir::type::i8, result);
		auto pcnt = bb->push_unop(ir::op::bit_popcnt, cast);
		auto mod2 = bb->push_cast(ir::type::i1, pcnt);
		bb->push_write_reg(reg::flag_pf, mod2);
	}
	inline void set_sf(ir::basic_block* bb, ir::insn* result) {
		auto sgn = bb->push_cmp(ir::op::lt, result, ir::constant(result->get_type(), 0));
		bb->push_write_reg(reg::flag_sf, sgn);
	}
	inline void set_zf(ir::basic_block* bb, ir::insn* result) {
		auto is_zero = bb->push_cmp(ir::op::eq, result, ir::constant(result->get_type(), 0));
		bb->push_write_reg(reg::flag_zf, is_zero);
	}
	template<typename Rhs>
	inline void set_af(ir::basic_block* bb, Rhs&& rhs, ir::insn* result) {
		auto tmp		= bb->push_binop(ir::op::bit_xor, result, std::forward<Rhs>(rhs));
		tmp			= bb->push_binop(ir::op::bit_and, tmp, ir::constant(tmp->get_type(), 0x10));
		auto is_set = bb->push_cmp(ir::op::ne, tmp, ir::constant(tmp->get_type(), 0));
		bb->push_write_reg(reg::flag_af, is_set);
	}
	template<typename Lhs, typename Rhs>
	inline void set_af(ir::basic_block* bb, Lhs&& lhs, Rhs&& rhs, ir::insn* result) {
		auto tmp		= bb->push_binop(ir::op::bit_xor, std::forward<Lhs>(lhs), std::forward<Rhs>(rhs));
		tmp			= bb->push_binop(ir::op::bit_xor, tmp, result);
		tmp			= bb->push_binop(ir::op::bit_and, tmp, ir::constant(tmp->get_type(), 0x10));
		auto is_set = bb->push_cmp(ir::op::ne, tmp, ir::constant(tmp->get_type(), 0));
		bb->push_write_reg(reg::flag_af, is_set);
	}
	template<typename Lhs, typename Rhs, typename Carry>
	inline void set_af(ir::basic_block* bb, Lhs&& lhs, Rhs&& rhs, Carry&& carry, ir::insn* result) {
		auto tmp = bb->push_binop(ir::op::bit_xor, result, std::forward<Carry>(carry));
		return set_af(bb, std::forward<Lhs>(lhs), std::forward<Rhs>(rhs), tmp);
	}
	template<typename Lhs, typename Rhs>
	inline void set_cf_add(ir::basic_block* bb, Lhs&& lhs, Rhs&& rhs, ir::insn* res) {
		// res u< lhs || res u< rhs
		auto a = bb->push_cmp(ir::op::ult, res, std::forward<Lhs>(lhs));
		auto b = bb->push_cmp(ir::op::ult, res, std::forward<Rhs>(rhs));
		bb->push_write_reg(reg::flag_cf, bb->push_binop(ir::op::bit_or, a, b));
	}
	template<typename Lhs, typename Rhs>
	inline void set_cf_sub(ir::basic_block* bb, Lhs&& lhs, Rhs&& rhs) {
		// lhs u< rhs
		bb->push_write_reg(reg::flag_cf, bb->push_cmp(ir::op::ult, std::forward<Lhs>(lhs), std::forward<Rhs>(rhs)));
	}
	template<typename Lhs, typename Rhs>
	inline void set_of_add(ir::basic_block* bb, Lhs&& lhs, Rhs&& rhs, ir::insn* res) {
		auto t  = res->get_type();
		auto sl = bb->push_cmp(ir::op::lt, std::forward<Lhs>(lhs), ir::constant(t, 0));
		auto sr = bb->push_cmp(ir::op::lt, std::forward<Rhs>(rhs), ir::constant(t, 0));
		auto sx = bb->push_cmp(ir::op::lt, res, ir::constant(t, 0));
		// 0.. 0.. -> 1..
		// 1.. 1.. -> 0..

		auto a = bb->push_cmp(ir::op::eq, sl, sx);
		auto b = bb->push_cmp(ir::op::eq, sr, sx);
		bb->push_write_reg(reg::flag_cf, bb->push_binop(ir::op::bit_or, a, b));
	}
	template<typename Lhs, typename Rhs>
	inline void set_of_sub(ir::basic_block* bb, Lhs&& lhs, Rhs&& rhs, ir::insn* res) {
		auto t  = res->get_type();
		auto sl = bb->push_cmp(ir::op::lt, std::forward<Lhs>(lhs), ir::constant(t, 0));
		auto sr = bb->push_cmp(ir::op::lt, std::forward<Rhs>(rhs), ir::constant(t, 0));
		auto sx = bb->push_cmp(ir::op::lt, res, ir::constant(t, 0));
		// 0.. 1.. -> 1..
		// 1.. 0.. -> 0..
		auto a = bb->push_cmp(ir::op::eq, sl, sx);
		auto b = bb->push_cmp(ir::op::eq, sr, sx);
		bb->push_write_reg(reg::flag_cf, bb->push_binop(ir::op::bit_or, a, b));
	}

	// Sets logical flags.
	//
	static void set_flags_logical(ir::basic_block* bb, ir::insn* result) {
		set_sf(bb, result);
		set_zf(bb, result);
		set_pf(bb, result);
		// - The OF and CF flags are cleared; the SF, ZF, and PF flags are set according to the result. The state of the AF flag is undefined.
		bb->push_write_reg(reg::flag_of, false);
		bb->push_write_reg(reg::flag_cf, false);
		bb->push_write_reg(reg::flag_af, false); // Runtime behaviour.
	}

	// Conditionals.
	//
	#define VISIT_CONDITIONS(_) _(Z, NZ) _(S, NS) _(B, NB) _(BE, NBE) _(L, NL) _(LE, NLE) _(O, NO) _(P, NP)
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

	// Segment/Register mappings.
	//
	static ir::segment map_seg(mreg seg) {
		if (seg.id == (u32) reg::gs) {
			return ir::segment(1);
		} else if (seg.id == (u32) reg::fs) {
			return ir::segment(2);
		}
		return ir::NO_SEGMENT;
	}
	static mreg reg_ip(x86arch* mach) {
		if (mach->is_64()) {
			return reg::rip;
		} else if (mach->is_32()) {
			return reg::eip;
		} else {
			return reg::ip;
		}
	}
	static mreg reg_sp(x86arch* mach) {
		if (mach->is_64()) {
			return reg::rsp;
		} else if (mach->is_32()) {
			return reg::esp;
		} else {
			return reg::sp;
		}
	}

	// Register read/write helper.
	//
	inline ir::insn* read_reg(SemaContext, mreg _r, ir::type type = ir::type::none) {
		reg r = (reg) _r.id;
		RC_ASSERT(r != reg::none);
		RC_ASSERT(r != reg::rip && r != reg::eip && r != reg::ip);
		RC_ASSERT(r != reg::rflags && r != reg::eflags && r != reg::flags);

		// If no type specified, use the default type.
		//
		if (type == ir::type::none) {
			type = enum_reflect(enum_reflect(r).kind).type;

			// If no valid default type, throw.
			//
			if (type == ir::type::none) {
				auto name = enum_name(r);
				fmt::abort("cannot read register '%.*s' with no type specifier.", name.size(), name.data());
			}
			// Decay tag types if not specially requested.
			//
			else if (type == ir::type::pointer) {
				type = mach->ptr_type();
			} else if (type == ir::type::segment) {
				type = ir::type::i16;
			}
		} else {
			auto type2 = enum_reflect(enum_reflect(r).kind).type;
			RC_ASSERT(type2 == ir::type::none || enum_reflect(type2).bit_size == enum_reflect(type).bit_size);
		}

		// Push read_reg.
		return bb->push_read_reg(type, r);
	}
	inline ir::insn* write_reg(SemaContext, mreg _r, ir::variant&& value, bool implicit_zero = true) {
		reg	r	  = (reg) _r.id;
		auto* desc = &enum_reflect(r);
		RC_ASSERT(r != reg::none);
		RC_ASSERT(r != reg::rip && r != reg::eip && r != reg::ip);
		RC_ASSERT(r != reg::rflags && r != reg::eflags && r != reg::flags);

		// Long mode moves to 32-bit register zeroes the upper part.
		//
		if (implicit_zero && desc->kind == reg_kind::gpr32 && mach->is_64()) {
			// Cast to i64 and swap with parent.
			//
			value = bb->push_cast(ir::type::i64, std::move(value));
			r		= desc->super;
			desc	= &enum_reflect(r);
		}
		// TODO: VEX/EVEX for zeroing.
		else {
		}

		// Write sub registers.
		//
		if (reg_kind::gpr16 <= desc->kind && desc->kind <= reg_kind::gpr64) {
			// For each part effected:
			//
			for (reg sub : desc->subregs) {
				auto& sub_desc = enum_reflect(sub);
				auto	sub_ty	= ir::int_type(sub_desc.width);

				// Shift and truncate.
				//
				ir::insn* sub_val;
				if (sub_desc.offset) {
					sub_val = bb->push_binop(ir::op::bit_shr, value, ir::constant(value.get_type(), sub_desc.offset));
					sub_val = bb->push_cast(sub_ty, sub_val);
				} else {
					sub_val = bb->push_cast(sub_ty, value);
				}
				
				// Write.
				//
				bb->push_write_reg(sub, sub_val);
			}
		}

		// Finally write to the register and return.
		//
		return bb->push_write_reg(r, std::move(value));
	}

	// Address generation helper.
	//
	inline std::pair<ir::variant, ir::segment> agen(SemaContext, mem m, bool as_ptr = true) {
		auto pty = mach->ptr_type();

		// Handle RIP.
		//
		if (m.base == reg::rip) {
			m.base = reg::none;
			m.disp += (i64) ip;
		}

		// [base+...]
		ir::insn* result = nullptr;
		if (m.base) {
			result = read_reg(sema_context(), m.base, pty);
		}

		// [index*scale+...]
		if (m.index && m.scale != 0) {
			auto idx = read_reg(sema_context(), m.index, pty);
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
			return {ir::constant(as_ptr ? ir::type::pointer : pty, m.disp), map_seg(m.segr)};
		} else if (m.disp) {
			result = bb->push_binop(ir::op::add, result, ir::constant(pty, m.disp));
		}

		// Finally cast to pointer and return.
		//
		if (as_ptr) {
			result = bb->push_cast(ir::type::pointer, result);
		}
		return {result, map_seg(m.segr)};
	}

	// Memory read/write helper.
	//
	inline ir::insn* read_mem(SemaContext, mem m, ir::type ty) {
		auto [ptr, seg] = agen(sema_context(), m);
		return bb->push_load_mem(ty, seg, std::move(ptr));
	}
	inline ir::insn* write_mem(SemaContext, mem m, ir::variant&& value) {
		auto [ptr, seg] = agen(sema_context(), m);
		return bb->push_store_mem(seg, std::move(ptr), std::move(value));
	}

	// Common read/write helper.
	//
	static ir::variant read(SemaContext, size_t idx, ir::type ty) {
		switch (ins.op[idx].type) {
			case mop_type::reg: {
				auto reg = read_reg(sema_context(), ins.op[idx].r, ty);
				if (reg->get_type() != ty) {
					reg = bb->push_cast(ty, reg);
				}
				return reg;
			}
			case mop_type::mem: {
				return read_mem(sema_context(), ins.op[idx].m, ty);
			}
			case mop_type::imm: {
				auto i = ins.op[idx].i;
				if (i.is_relative) {
					i.u += ip;
				}
				if (i.is_signed) {
					return ir::constant(ty, i.s);
				} else {
					return ir::constant(ty, i.u);
				}
			}
			default:
				RC_UNREACHABLE();
		}
	}
	static ir::insn* write(SemaContext, size_t idx, ir::variant&& value) {
		switch (ins.op[idx].type) {
			case mop_type::reg: {
				return write_reg(sema_context(), ins.op[idx].r, std::move(value));
			}
			case mop_type::mem: {
				return write_mem(sema_context(), ins.op[idx].m, std::move(value));
			}
			case mop_type::imm: {
				fmt::abort("write invoked on immediate operand.");
			}
			default:
				RC_UNREACHABLE();
		}
	}
};