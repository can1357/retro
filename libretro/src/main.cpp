#include <retro/common.hpp>
#include <retro/platform.hpp>
#include <retro/format.hpp>
#include <retro/diag.hpp>
#include <retro/arch/interface.hpp>
#include <retro/ldr/interface.hpp>
#include <retro/ldr/image.hpp>
#include <retro/ir/insn.hpp>

using namespace retro;


#include <retro/ir/routine.hpp>
#include <retro/arch/x86/regs.hxx>
#include <retro/robin_hood.hpp>

#include <retro/ir/z3x.hpp>

int main(int argv, const char** args) {
	platform::setup_ansi_escapes();

	u64					  ip		 = 0x100;
	constexpr u8		  test[]	 = {0x48, 0x39, 0xD8, 0x7C, 0x01, 0x90};
	std::span<const u8> data	 = test;
	auto					  machine = arch::instance::lookup("x86_64");

	// lift em all
	auto	rtn = make_rc<ir::routine>();
	auto* bb	 = rtn->add_block();
	for (size_t i = 0; !data.empty(); i++) {
		// Disasm the instruction and print it.
		//
		arch::minsn ins = {};
		if (!machine->disasm(data, &ins)) {
			bb->push_trap("undefined opcode");
			fmt::println("failed to disasm\n");
			break;
		}
		fmt::println(ip, ": ", ins.to_string(ip));
		if (auto err = machine->lift(bb, ins, ip)) {
			fmt::println("-> lifter failed with: ", err);
			break;
		}

		// Skip the bytes and increment IP.
		//
		data = data.subspan(ins.length);
		ip += ins.length;
	}
	
	// Local move propagation pass.
	//
	{
		robin_hood::unordered_flat_map<u32, ref<ir::insn>> producer = {};
		for (auto* ins : bb->insns()) {
			// If producing a value:
			if (ins->op == ir::opcode::write_reg) {
				auto r = ins->operands()[0].get_const().get<arch::mreg>();

				// If there's another producer, remove it from the stream, declare ourselves as the last one.
				auto& lp = producer[r.uid()];
				if (lp) {
					lp->replace_all_uses_with(ins);
					lp->erase();
				}
				lp = ins;
			} else if (ins->op == ir::opcode::read_reg) {
				auto r = ins->operands()[0].get_const().get<arch::mreg>();

				// If there's a cached producer of this value:
				auto& lp = producer[r.uid()];
				if (lp) {
					auto& lpv = lp->operands()[1];
					// If the type matches:
					if (auto ty = lpv.get_type(); ty == ins->template_types[0]) {
						// Replace uses with the assigned value.
						ins->replace_all_uses_with(lpv);
						ins->op = ir::opcode::nop;
					}
					// Otherwise:
					else {
						// Insert a bitcast right before the read.
						auto bc = bb->insert(ins, ir::make_bitcast(ins->template_types[0], lpv));
						// Replace uses with the result value.
						ins->replace_all_uses_with(bc.get());
					}
				}
			}
		}

	}
	bb->rerase_if([](ir::insn* i) { return !i->uses() && !i->desc().side_effect; });

	// Local constant propagation pass.
	//
	{
		for (auto* ins : bb->insns()) {
			if (ins->op == ir::opcode::binop || ins->op == ir::opcode::cmp) {
				auto& opc = ins->operands()[0];
				auto& lhs = ins->operands()[1];
				auto& rhs = ins->operands()[2];
				if (lhs.is_const() && rhs.is_const()) {
					if (auto res = lhs.get_const().apply(opc.get_const().get<ir::op>(), rhs.get_const()); !res.is<void>()) {
						ins->replace_all_uses_with(res);
					}
				}
			} else if (ins->op == ir::opcode::unop) {
				auto& opc = ins->operands()[0];
				auto& lhs = ins->operands()[1];
				if (lhs.is_const()) {
					if (auto res = lhs.get_const().apply(opc.get_const().get<ir::op>()); !res.is<void>()) {
						ins->replace_all_uses_with(res);
					}
				}
			} else if (ins->op == ir::opcode::cast) {
				auto	into = ins->template_types[1];
				auto& val  = ins->operands()[0];
				if (val.is_const()) {
					if (auto res = val.get_const().cast_zx(into); !res.is<void>()) {
						ins->replace_all_uses_with(res);
					}
				}
			} else if (ins->op == ir::opcode::cast_sx) {
				auto	into = ins->template_types[1];
				auto& val  = ins->operands()[0];
				if (val.is_const()) {
					if (auto res = val.get_const().cast_sx(into); !res.is<void>()) {
						ins->replace_all_uses_with(res);
					}
				}
			} else if (ins->op == ir::opcode::select) {
				auto	into = ins->template_types[1];
				auto& val  = ins->operands()[0];
				if (val.is_const()) {
					ins->replace_all_uses_with(ins->operands()[val.const_val.get<bool>() ? 1 : 2]);
				}
			}
		}
	}
	bb->rerase_if([](ir::insn* i) { return !i->uses() && !i->desc().side_effect; });

	// TODO: PHI Gen
	//
	fmt::println(bb->to_string());

	// ----------


	z3::context c;
	auto			termcc = bb->back()->operands()[0].get_value();
	fmt::println(termcc->to_string());

	bb->push_nop();
	list::iterator insert_here_plz = bb->push_nop();

	bb->push_nop();
	bb->push_nop();
	bb->push_nop();

	z3x::variable_set vs;
	if (auto expr = z3x::to_expr(vs, c, termcc)) {
		fmt::println(z3x::from_expr(vs, expr.simplify(), bb, insert_here_plz));
	}
	
	fmt::println(bb->to_string());

	#if 0
	// Print file details.
	//
	ref  img		 = ldr::load_from_file("../tests/libretro.exe").value();
	auto machine = arch::instance::lookup(img->arch_hash);
	auto loader	 = ldr::instance::lookup(img->ldr_hash);
	RC_ASSERT(loader && machine);
	fmt::println("-> loader:  ", loader->get_name());
	fmt::println("-> machine: ", machine->get_name());

	// Demo.
	//
	u64	ip		  = 0x1400072C0;
	auto	rtn	  = make_rc<ir::routine>();
	auto* bb		  = rtn->add_block();
	rtn->ip		  = ip;
	bb->ip		  = ip;

	size_t ins_read_count = 0;
	size_t ins_write_count = 0;
	std::span<const u8> data = img->slice(ip - img->base_address);
	for (size_t i = 0; !data.empty(); i++) {
		// Disasm the instruction and print it.
		//
		arch::minsn ins = {};
		if (!machine->disasm(data, &ins)) {
			bb->push_trap("undefined opcode");
			fmt::println("failed to disasm\n");
			break;
		}
		fmt::println(ip, ": ", ins.to_string(ip));
		ins_read_count++;

		// Try lifting it and print the result.
		//
		if (auto err = machine->lift(bb, ins, ip)) {
			fmt::println("-> lifter failed with: ", err);
			break;
		}
		if (!bb->empty()) {
			auto it = std::prev(bb->end());
			while (it != bb->begin() && it->prev->ip == ip) {
				--it;
			}
			while (it != bb->end()) {
				ins_write_count++;
				fmt::println("          -> ", it->to_string());
				++it;
			}
		}

		// Print statistics.
		//
		printf(RC_GRAY " # Successfully lifted " RC_VIOLET "%u" RC_GRAY " instructions into " RC_GREEN "%u" RC_RED " (avg: ~%u/ins) #\n" RC_RESET,
				ins_read_count, ins_write_count, ins_write_count / ins_read_count
		);

		// TODO: BB splitting and branch handling.
		//

		// Skip the bytes and increment IP.
		//
		data = data.subspan(ins.length);
		ip += ins.length;
	}
	return 0;
	#endif
}