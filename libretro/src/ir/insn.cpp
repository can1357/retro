#include <retro/ir/insn.hpp>

namespace retro::ir {
	RC_DEF_ERR(insn_operand_type_mismatch, "expected operand #% to be of type '%', got '%' instead: %")
	RC_DEF_ERR(insn_constexpr_mismatch, "expected operand #% to be constexpr got '%' instead: %")

	// Erases an operand.
	//
	void insn::erase_operand(size_t i) {
		// Reset the operand, move rest of it.
		// - Note: this is very unsafe!
		//
		operands()[i].reset();
		if (i != (operand_count - 1)) {
			memmove(&operands()[i], &operands()[i + 1], (operand_count - i - 1) * sizeof(operand));

			// Fix list entries.
			//
			for (; i != (operand_count - 1); ++i) {
				auto& op = operands()[i];
				if (!op.is_const()) {
					auto* p = op.prev;
					auto* n = op.next;
					p->next = &op;
					n->prev = &op;
				}
			}
		}
		--operand_count;
	}

	// String conversion and type getter.
	//
	std::string insn::to_string(fmt_style s) const {
		if (s == fmt_style::concise) {
			return fmt::str(RC_YELLOW "%%%x" RC_RESET, name);
		} else {
			auto& info = enum_reflect(op);

			std::string result = {};
			if (get_type() != type::none) {
				result = fmt::str(RC_YELLOW "%%%x" RC_RESET " = ", name);
			}

			if (info.side_effect)
				result += RC_RED;
			else
				result += RC_TEAL;

			result += info.name;
			for (size_t i = 0; i != info.template_count; i++) {
				result += ".";
				result += enum_name(template_types[i]);
			}
			result += " " RC_RESET;

			for (auto& op : operands()) {
				if (op.is_const()) {
					result += RC_GREEN;

					// Handle specially formatted types.
					//
					auto& cv = op.get_const();
					if (cv.is<arch::mreg>()) {
						if (arch) {
							result += arch->name_register(cv.get<arch::mreg>());
							result += RC_RESET ", ";
							continue;
						}
					}
					if (cv.is<ir::segment>()) {
						if (auto s = cv.get<ir::segment>()) {
							result += fmt::str("seg(%x)", u32(s));
							result += RC_RESET ", ";
						} else {
							result += RC_NAVY_BLUE "global";
							result += RC_RESET ", ";
						}
						continue;
					}
				}
				result += op.to_string(fmt_style::concise);
				result += RC_RESET ", ";
			}
			if (operand_count) {
				result.erase(result.end() - 2, result.end());
			}
			return result;
		}
	}

	// Basic validation.
	//
	diag::lazy insn::validate() const {
		// Validate the operand types.
		//
		auto& info = enum_reflect(op);
		for (size_t i = 1; i < info.templates.size(); i++) {
			type texpc;
			if (info.templates[i] != 0) {
				texpc = template_types[info.templates[i] - 1];
			} else {
				texpc = info.types[i];
				if (texpc == type::pack) {
					continue;
				}
			}
			type treal = operands()[i - 1].get_type();
			if (treal != texpc) {
				return err::insn_operand_type_mismatch(i - 1, texpc, treal, to_string());
			}
		}

		// Validate constexpr requirements.
		//
		for (u8 cxpr : info.constexprs) {
			if (!operands()[cxpr - 1].is_const()) {
				return err::insn_constexpr_mismatch(cxpr - 1, operands()[cxpr - 1].to_string(fmt_style::concise), to_string());
			}
		}
		return diag::ok;
	}
};