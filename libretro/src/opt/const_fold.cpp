#include <retro/opt/interface.hpp>
#include <retro/opt/utility.hpp>
#include <retro/analysis/method.hpp>
#include <retro/analysis/workspace.hpp>

namespace retro::ir::opt {
	// Local constant folding.
	//
	size_t const_fold(basic_block* bb) {
		size_t n = 0;

		// For each instruction:
		//
		for (auto* ins : bb->insns()) {
			// If we can evaluate the instruction in a constant manner, do so.
			//
			if (ins->op == opcode::binop || ins->op == opcode::cmp) {
				auto& opc = ins->opr(0);
				auto& lhs = ins->opr(1);
				auto& rhs = ins->opr(2);
				if (lhs.is_const() && rhs.is_const()) {
					if (auto res = lhs.get_const().apply(opc.get_const().get<op>(), rhs.get_const()); !res.is<void>()) {
						n += 1 + ins->replace_all_uses_with(res);
					}
				}
			} else if (ins->op == opcode::unop) {
				auto& opc = ins->opr(0);
				auto& lhs = ins->opr(1);
				if (lhs.is_const()) {
					if (auto res = lhs.get_const().apply(opc.get_const().get<op>()); !res.is<void>()) {
						n += 1 + ins->replace_all_uses_with(res);
					}
				}
			} else if (ins->op == opcode::cast) {
				auto	into = ins->template_types[1];
				auto& val  = ins->opr(0);
				if (val.is_const()) {
					if (auto res = val.get_const().cast_zx(into); !res.is<void>()) {
						n += 1 + ins->replace_all_uses_with(res);
					}
				}
			} else if (ins->op == opcode::cast_sx) {
				auto	into = ins->template_types[1];
				auto& val  = ins->opr(0);
				if (val.is_const()) {
					if (auto res = val.get_const().cast_sx(into); !res.is<void>()) {
						n += 1 + ins->replace_all_uses_with(res);
					}
				}
			} else if (ins->op == opcode::select) {
				auto& val = ins->opr(0);
				if (val.is_const()) {
					n += 1 + ins->replace_all_uses_with(ins->opr(val.const_val.get<bool>() ? 1 : 2));
				}
			} else if (ins->op == opcode::load_mem) {
				// Skip if not constant.
				//
				auto& adr = ins->opr(0);
				if (!adr.is_const()) {
					continue;
				}

				// Skip if no associated image.
				//
				auto method = bb->rtn->method.lock();
				if (!method)
					continue;
				auto domain = method->dom.lock();
				if (!domain)
					continue;

				// If RVA maps to a constant section:
				//
				u64 rva = adr.get_const().get_u64() - domain->img->base_address;
				auto scn = domain->img->find_section(rva);
				if (scn && !scn->write) {
					// If we succeded in loading a constant:
					//
					auto			 data = domain->img->slice(rva);
					ir::constant value{ins->template_types[0], data};
					if (!value.is<void>()) {
						// Replace all uses.
						//
						n += ins->replace_all_uses_with(std::move(value));
					}
				}
			}
		}
		return util::complete(bb, n);
	}
};
