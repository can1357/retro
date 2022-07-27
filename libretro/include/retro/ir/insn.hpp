#pragma once
#include <retro/ir/opcodes.hxx>
#include <retro/ir/value.hpp>
#include <retro/ir/types.hpp>

namespace retro::ir {

	// Constant-as-value for non operand use cases.
	//
	struct constant_as_value final : dyn<constant_as_value, value> {
		// Data held.
		//
		constant data = {};

		// Forward string conversion and type getter.
		//
		std::string to_string(fmt_style s = {}) const override { return data.to_string(); }
		type			get_type() const override { return data.get_type(); }

		// Created by the constant value.
		//
		template<typename Ty>
		inline static shared<constant_as_value> make(Ty data) {
			auto result	 = make_rc<constant_as_value>();
			result->data = data;
			return result;
		}
	};

	// Define the operand type.
	//
	struct operand : pinned {
	  private:
		// Since use is aligned by 8, use.prev will also be aligned by 8.
		// - If misaligned (eg check bit 0 == 1), we can use it as a thombstone.
		//
		union {
			use		urec;
			constant cval;
		};

	  public:
		// Default constructor.
		//
		operand() : cval{} {}

		// Assignment.
		//
		void reset() {
			if (is_const()) {
				cval.reset();
			} else {
				std::destroy_at(&urec);
			}
		}
		void reset(constant value, const void* user = nullptr) {
			reset();
			std::construct_at(&cval, std::move(value));
			RC_ASSERT(is_const());
		}
		void reset(value* v, const void* user) {
			reset();
			if (v) {
				std::construct_at(&urec);
				urec.reset(v, user);
				RC_ASSERT(!is_const());
			}
		}
		template<typename T>
		void reset(const shared<T>& v, const void* user) {
			reset(v.get(), user);
		}

		// Observers.
		//
		bool is_const() const { return cval.__rsvd == 1; }
		bool is_value() const { return !is_const(); }

		const constant& get_const() const {
			RC_ASSERT(is_const());
			return cval;
		}
		value* get_value() const {
			RC_ASSERT(is_value());
			return urec.get();
		}

		std::string to_string(fmt_style s = {}) const { return is_const() ? get_const().to_string() : get_value()->to_string(s); }
		type			get_type() const { return is_const() ? get_const().get_type() : get_value()->get_type(); }

		// Reset on destruction.
		//
		~operand() { reset(); }
	};
	static_assert(sizeof(constant) <= sizeof(use), "ir::constant must be smaller or equally large as as ir::use.");

	// Instruction type.
	//
	struct insn final : dyn<insn, value> {
		// Opcode.
		//
		opcode op = opcode::none;

		// Template types.
		// TODO:
		type template_types[2] = {};

		// Value name.
		// TODO:
		u32 name = 0;

		// Variable length operand array.
		//
		const u32 operand_count = 0;
		operand	 operands_array[];

		// Allocated with operand count.
		//
		inline insn(u32 n) : operand_count(n) {}
		inline static shared<insn> allocate(size_t operand_count) {
			u32  oc = narrow_cast<u32>(operand_count);
			auto r  = make_overalloc_rc<insn>(sizeof(operand) * oc, oc);
			range::uninitialized_default_construct(r->operands());
			return r;
		}

		// Gets the operands.
		//
		std::span<operand>		 operands() { return {operands_array, operand_count}; }
		std::span<const operand> operands() const { return {operands_array, operand_count}; }

		// Given a use from this instruction, gets the operand index.
		//
		size_t index_of(const use* use) const { return ((operand*) use) - &operands_array[0]; }

		// Changes an operands value.
		//
		void set_operand(size_t idx) {}
		template<typename T, typename... Tx>
		void set_operand(size_t idx, T&& new_value, Tx&&... rest) {
			auto& op = operands_array[idx];
			op.reset(std::forward<T>(new_value), this);
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
		void validate() const {
			// TODO: Diag::
			//
			auto& info = enum_reflect(op);
			for (size_t i = 1; i < info.templates.size(); i++) {
				if (info.templates[i] != 0) {
					type treal = operands()[i - 1].get_type();
					type texpc = template_types[info.templates[i] - 1];
					if (treal != texpc) {
						fmt::println("Expected ", texpc, " got ", treal);
						fmt::println(to_string());
						fmt::abort_no_msg();
					}
				}
			}
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
