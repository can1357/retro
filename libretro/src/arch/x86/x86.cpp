#include <retro/arch/x86.hpp>
#include <retro/arch/x86/zy2rc.hpp>
#include <retro/arch/x86/sema.hpp>

namespace retro::arch {
	static_assert(ZYDIS_MAX_OPERAND_COUNT_VISIBLE <= arch::max_mop_count, "Update constants.");
	RC_DEF_ERR(unhandled_insn, "unhandled instruction: %");

	// Write the arch mode details on construction.
	//
	x86arch::x86arch(ZydisMachineMode mode) {
		machine_mode = mode;
		switch (machine_mode) {
			case ZYDIS_MACHINE_MODE_LONG_64:
				stack_width	  = ZYDIS_STACK_WIDTH_64;
				ptr_width	  = 64;
				ptr_width_eff = 48;
				break;
			case ZYDIS_MACHINE_MODE_LONG_COMPAT_32:
			case ZYDIS_MACHINE_MODE_LEGACY_32:
				stack_width	  = ZYDIS_STACK_WIDTH_32;
				ptr_width	  = 32;
				ptr_width_eff = 32;
				break;
			case ZYDIS_MACHINE_MODE_LONG_COMPAT_16:
			case ZYDIS_MACHINE_MODE_LEGACY_16:
			case ZYDIS_MACHINE_MODE_REAL_16:
				stack_width	  = ZYDIS_STACK_WIDTH_16;
				ptr_width	  = 16;
				ptr_width_eff = 16;
				break;
			default:
				RC_UNREACHABLE();
		}

		// Initialize the decoder.
		//
		ZydisDecoderInit(&decoder, machine_mode, stack_width);
	}

	// Lifting and disassembly.
	//
	bool x86arch::disasm(std::span<const u8> data, x86insn* out) {
		return ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, data.data(), data.size(), &out->ins, out->ops, (u8) std::size(out->ops), ZYDIS_DFLAG_VISIBLE_OPERANDS_ONLY));
	}
	bool x86arch::disasm(std::span<const u8> data, minsn* out) {
		x86insn nat;
		if (disasm(data, &nat)) {
			out->arch				= (u32) get_handle();
			out->mnemonic			= nat.ins.mnemonic;
			out->modifiers			= nat.ins.attributes;
			out->effective_width = nat.ins.operand_width;
			out->length				= nat.ins.length;
			out->operand_count	= nat.ins.operand_count_visible;
			out->is_privileged	= bool(nat.ins.attributes & ZYDIS_ATTRIB_IS_PRIVILEGED);
			for (size_t n = 0; n != nat.ins.operand_count_visible; n++) {
				out->op[n] = x86::to_mop(nat.ops[n], nat.ins.length);
			}
			return true;
		}
		return false;
	}
	diag::lazy x86arch::lift(ir::basic_block* bb, const minsn& ins, u64 ip) {
		// Resolve the lifter.
		//
		auto lifter = x86::lifter_table[ins.mnemonic];
		if (!lifter)
			return err::unhandled_insn(name_mnemonic(ins.mnemonic));

		// Reference the last instruction unrelated to the next one.
		//
		auto prev = std::prev(bb->end(), bb->empty() ? 0 : 1).at;

		// Invoke the lifter.
		//
		auto status = lifter(this, bb, ins, ip);

		// Mark the range with the ip/arch prior to execution, return the result.
		//
		for (auto* ins : view::reverse(bb->insns())) {
			if (ins == prev)
				break;
			ins->arch = get_handle();
			ins->ip	 = ip;
		}
		return status;
	}

	// Formatting.
	//
	std::string x86insn::to_string(u64 ip) const {
		char				buffer[128];
		ZydisFormatter formatter;
		ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);
		if (ZYAN_FAILED(ZydisFormatterFormatInstruction(&formatter, &ins, ops, ins.operand_count_visible, buffer, sizeof(buffer), ip))) {
			return {};
		} else {
			return std::string(&buffer[0]);
		}
	}
	std::string_view x86arch::name_register(mreg r) {
		return enum_name(x86::reg(r.id));
	}
	std::string_view x86arch::name_mnemonic(u32 i) {
		const char* p = ZydisMnemonicGetString(ZydisMnemonic(i));
		return p ? std::string_view{p} : std::string_view{};
	}
	std::string x86arch::format_minsn_modifiers(const minsn& i) {
		std::string result = {};
		if (i.modifiers & ZYDIS_ATTRIB_HAS_LOCK) {
			result += "lock ";
		}
		if (i.modifiers & ZYDIS_ATTRIB_HAS_REP) {
			result += "rep ";
		}
		if (i.modifiers & ZYDIS_ATTRIB_HAS_REPE) {
			result += "repe ";
		}
		if (i.modifiers & ZYDIS_ATTRIB_HAS_REPNE) {
			result += "repne ";
		}
		return result;
	}

	// Create the instances.
	//
	RC_ADD_INTERFACE("x86_32", x86arch, ZYDIS_MACHINE_MODE_LONG_COMPAT_32);
	RC_ADD_INTERFACE("x86_64", x86arch, ZYDIS_MACHINE_MODE_LONG_64);
};