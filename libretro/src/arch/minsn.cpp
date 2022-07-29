#include <retro/arch/mreg.hpp>
#include <retro/arch/minsn.hpp>
#include <retro/arch/interface.hpp>
#include <retro/format.hpp>

namespace retro::arch {
	// Forwarded naming.
	//
	std::string_view minsn::name() const {
		if (auto a = arch::instance::resolve(arch)) {
			return a->name_mnemonic(mnemonic);
		} else {
			return "?";
		}
	}
	std::string_view mreg::name(instance* a) const {
		if (a) {
			return a->name_register(*this);
		} else {
			return {};
		}
	}
	std::string mreg::to_string(instance* a) const {
		std::string result{name(a)};
		if (result.empty()) {
			result = fmt::str("r%u.%x", kind, id);
		}
		return result;
	}

	// Simple formatters.
	//
	std::string imm::to_string(u64 address) const {
		if (address && is_relative) {
			return fmt::str("%llx", address + s);
		}
		if (is_signed || is_relative) {
			return fmt::to_str(s);
		} else {
			return fmt::to_str(u);
		}
	}
	std::string mem::to_string(instance* a, u64 address) const {
		std::string result;

		// TODO: Should probably ask the arch :)
		//
		switch (width) {
			case 8:
				result = "byte ptr ";
				break;
			case 16:
				result = "word ptr ";
				break;
			case 32:
				result = "dword ptr ";
				break;
			case 64:
				result = "qword ptr ";
				break;
			case 48:
				result = "fword ptr ";
				break;
			case 80:
				result = "tbyte ptr ";
				break;
			case 128:
				result = "xmmword ptr ";
				break;
			case 256:
				result = "ymmword ptr ";
				break;
			case 512:
				result = "zmmword ptr ";
				break;
			case 1024:
				result = "tmmword ptr ";
				break;
			default:
				break;
		}

		// seg:[
		if (segv) {
			result += fmt::str("%x:[", (u32) segv);
		} else if (segr) {
			result += segr.to_string(a) + ":[";
		} else {
			result = "[";
		}

		// [ptr]
		if (!base && !index) {
			return result + fmt::str("%llx]", disp);
		}

		// [ip+disp] | [ip+index*scale+disp]
		if (base.get_kind() == reg_kind::instruction && address) {
			u64 total_disp = disp + address;
			if (index) {
				result += index.to_string(a);
				if (scale != 1)
					result += fmt::str("*%d", scale);
				result += "+";
			}
			return result + fmt::str("0x%llx]", total_disp);
		}

		// [base
		if (!index) {
			result += base.to_string(a);
		}
		// [index*scale
		else if (!base) {
			result += index.to_string(a);
			if (scale != 1)
				result += fmt::str("*%d", scale);
		}
		// [base+index*scale
		else {
			result += base.to_string(a);
			result += "+";
			result += index.to_string(a);
			if (scale != 1)
				result += fmt::str("*%d", scale);
		}

		// +disp]
		if (disp) {
			result += fmt::str("+0x%llx", disp);
		}
		result += "]";
		return result;
	}
	std::string minsn::to_string(u64 address) const {
		auto a = arch::instance::resolve(arch);
		if (!a) {
			return {};
		}

		std::string result{name()};
		if (modifiers) {
			result.insert(0, a->format_minsn_modifiers(*this));
		}
		if (operand_count) {
			fmt::ljust(result, 12);

			for (auto& op : operands()) {
				result += op.to_string(a, address);
				result += ", ";
			}
			result.erase(result.end() - 2, result.end());
		}
		return result;
	}
};