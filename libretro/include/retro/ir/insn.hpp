#pragma once
#include <retro/ir/opcodes.hxx>
#include <retro/ir/value.hpp>
#include <retro/diag.hpp>

namespace retro::ir {
	RC_DEF_ERR(insn_type_mismatch, "expected operand #% to be of type '%', got '%' instead: %")

	// Instruction type.
	//
	struct insn final : dyn<insn, value> {
		// TODO: linked list

		// Opcode.
		//
		opcode op = opcode::none;

		// Template types.
		//
		type template_types[2] = {};

		// Value name.
		// TODO:
		u32 name = 0;

		// Variable length operand array.
		//
		const u32 operand_count = 0;

		// Allocated with operand count.
		//
		inline insn(u32 n) : operand_count(n) {}
		inline static ref<insn> allocate(size_t operand_count) {
			u32  oc = narrow_cast<u32>(operand_count);
			auto r  = make_overalloc_rc<insn>(sizeof(operand) * oc, oc);
			for (auto& op : r->operands())
				std::construct_at(&op, r.get());
			return r;
		}

		// Gets the operands.
		//
		std::span<operand>		 operands() { return {(operand*) (this + 1), operand_count}; }
		std::span<const operand> operands() const { return {(operand*) (this + 1), operand_count}; }

		// Given a use from this instruction, gets the operand index.
		//
		size_t index_of(const operand* operand) const { return operand - operands().data(); }

		// Changes an operands value.
		//
		void set_operand(size_t idx) {}
		template<typename T, typename... Tx>
		void set_operand(size_t idx, T&& new_value, Tx&&... rest) {
			auto& op = operands()[idx];
			op.reset(std::forward<T>(new_value));
			if constexpr (sizeof...(Tx) != 0) {
				set_operand<Tx...>(idx + 1, std::forward<Tx>(rest)...);
			}
		}

		// Forward string conversion and type getter.
		//
		std::string to_string(fmt_style s = {}) const override {
			if (s == fmt_style::concise) {
				return fmt::str(RC_YELLOW "%%%u" RC_RESET, name);
			} else {
				auto& info = enum_reflect(op);

				std::string result = {};
				if (get_type() != type::none) {
					result = fmt::str(RC_YELLOW "%%%u" RC_RESET " = ", name);
				}

				result += RC_RED;
				result += info.name;
				for (size_t i = 0; i != info.template_count; i++) {
					result += ".";
					result += enum_name(template_types[i]);
				}
				result += ' ';

				for (auto& op : operands()) {
					result += op.to_string(fmt_style::concise);
					result += RC_RESET ", ";
				}
				if (operand_count) {
					result.erase(result.end() - 2, result.end());
				}
				return result;
			}
		}
		type get_type() const override {
			auto& info = enum_reflect(op);
			if (info.templates[0] == 0) {
				return info.types[0];
			} else {
				return template_types[info.templates[0] - 1];
			}
		}

		// Basic validation.
		//
		diag::lazy validate() const {
			// Validate the operand types.
			//
			auto& info = enum_reflect(op);
			for (size_t i = 1; i < info.templates.size(); i++) {
				type treal = operands()[i - 1].get_type();
				type texpc;
				if (info.templates[i] != 0) {
					texpc = template_types[info.templates[i] - 1];
				} else {
					texpc = info.types[i];
					if (texpc == type::pack) {
						continue;
					}
				}
				if (treal != texpc) {
					return err::insn_type_mismatch(i - 1, texpc, treal, to_string());
				}
			}
			return diag::ok;
		}

		// Destroy all operands on destruction.
		//
		~insn() { range::destroy(operands()); }
	};

	// Create the auto-generated constructors.
	//
#define ADD_CTOR(a, ...) __VA_ARGS__
	RC_VISIT_OPCODE(ADD_CTOR)
#undef ADD_CTOR
};
