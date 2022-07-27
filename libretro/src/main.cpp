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

namespace retro::x86::sema {

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


int main(int argv, const char** args) {
	platform::setup_ansi_escapes();

	auto	proc = make_rc<ir::procedure>();
	auto* bb	  = proc->add_block();

	auto i0 = bb->push_back(ir::make_binop(ir::op::add, 2, 3));
	auto i1 = bb->push_back(ir::make_binop(ir::op::add, 3, i0));

	i1->erase_operand(1);
	i0->replace_all_uses_with(6);

	fmt::println(proc->to_string());

	/*std::span<const u8> data = {(u8*) lift_example, sizeof(lift_example) - 1};

	while (auto i = zydis::decode(data)) {
		for (auto& op : i->operands()) {
			if (op.type == ZYDIS_OPERAND_TYPE_REGISTER) {
				fmt::println("->", reg(op.reg.value));
			}
		}
		fmt::println(i->to_string());
	}*/ 
}