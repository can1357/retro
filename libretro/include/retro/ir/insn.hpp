#pragma once
#include <retro/ir/opcodes.hxx>
#include <retro/ir/value.hpp>
#include <retro/diag.hpp>
#include <retro/arch/interface.hpp>

// Forwards.
namespace retro::analysis {
	struct image;
	struct method;
	struct workspace;
};
namespace retro::ir {
	struct basic_block;
	struct routine;
};

namespace retro::ir {
	// Fake IP value.
	//
	inline constexpr u64 NO_LABEL = ~0ull;

	// Instruction type.
	//
	struct basic_block;
	struct insn final : dyn<insn, value> {
		// Owning basic block and the linked list entry.
		//
		basic_block* bb	= nullptr;
		insn*			 prev = this;
		insn*			 next = this;

		// Value name.
		//
		u32 name = 0;

		// Architecture handle.
		//
		arch::handle arch = {};

		// Opcode and operand count.
		//
		u32	 operand_count = 0;
		opcode op				= opcode::none;

		// Instruction meta-parameters.
		//
		std::array<type, 2> template_types = {};

		// Source instruction.
		//
		u64 ip = NO_LABEL;

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
		inline static ref<insn> allocate(opcode o, std::array<type, 2> tmps, size_t operand_count) {
			auto res				  = allocate(operand_count);
			res->op				  = o;
			res->template_types = tmps;
			return res;
		}

		// Gets the opcode descriptor.
		//
		const opcode_desc& desc() const { return enum_reflect(op); }

		// Returns true if orphan instruction.
		//
		bool is_orphan() const {
			RC_ASSERT(list::is_detached(this) == (bb == nullptr));
			return list::is_detached(this);
		}

		// Erases the instruction from the containing block.
		//
		ref<insn> erase() {
			// Unlink from the linked list.
			//
			RC_ASSERT(!is_orphan());
			bb = nullptr;
			list::unlink(this);

			// Parent had a strong reference already, no need to increment anything, simply re-use it.
			//
			return ref<insn>::adopt(this);
		}

		// Given a use from this instruction, gets the operand index.
		//
		size_t index_of(const operand* operand) const { return operand - operands().data(); }

		// Operand getters.
		//
		std::span<operand>		 operands() { return {(operand*) (this + 1), operand_count}; }
		std::span<const operand> operands() const { return {(operand*) (this + 1), operand_count}; }
		operand&						 opr(size_t i) { return operands()[i]; }
		const operand&				 opr(size_t i) const { return operands()[i]; }

		// Changes an operands value.
		//
		void set_operands(size_t idx) {}
		template<typename T, typename... Tx>
		void set_operands(size_t idx, T&& new_value, Tx&&... rest) {
			operands()[idx] = std::forward<T>(new_value);
			if constexpr (sizeof...(Tx) != 0) {
				set_operands<Tx...>(idx + 1, std::forward<Tx>(rest)...);
			}
		}

		// Erases an operand.
		//
		void erase_operand(size_t i);

		// String conversion and type getter.
		//
		std::string to_string(fmt_style s = {}) const override;
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
		diag::lazy validate() const;

		// Nested access wrappers.
		//
		routine*						 get_routine() const;
		ref<analysis::method>	 get_method() const;
		ref<analysis::image>		 get_image() const;
		ref<analysis::workspace> get_workspace() const;

		// Destroy all operands on destruction.
		//
		~insn() { range::destroy(operands()); }
	};

	// Create the auto-generated constructors.
	//
#define ADD_CTOR(a, oprhan, bb) oprhan
	RC_VISIT_IR_OPCODE(ADD_CTOR)
#undef ADD_CTOR
};
