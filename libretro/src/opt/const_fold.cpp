#include <retro/opt/interface.hpp>
#include <retro/opt/utility.hpp>
#include <retro/analysis/method.hpp>
#include <retro/analysis/image.hpp>
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
			} else if (ins->op == opcode::bitcast) {
				auto	into = ins->template_types[1];
				auto& val  = ins->opr(0);
				if (val.is_const()) {
					auto cv = val.get_const().bitcast(into);
					RC_ASSERT(!cv.is<void>());
					n += ins->replace_all_uses_with(cv);
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
				auto img = bb->get_image();
				if (!img)
					continue;

				// If RVA maps to a constant section:
				//
				u64 rva = adr.get_const().get_u64() - img->base_address;
				auto scn = img->find_section(rva);
				if (scn && !scn->write) {
					auto data = img->slice(rva);

					ir::constant value;
					if (ins->template_types[0] == ir::type::pointer) {
						auto arch = ins->arch;
						if (!arch)
							arch = img->arch;
						if (arch) {
							u8 ptrbytes = arch->get_pointer_width() / 8;
							if (data.size() >= ptrbytes) {
								RC_ASSERT(ptrbytes <= 8);
								u64 pval = 0;
								memcpy(&pval, data.data(), ptrbytes);
								value = {ir::type::pointer, pval};
							}
						}
					} else {
						value = {ins->template_types[0], data};
					}

					if (!value.is<void>()) {
						// Do not propagate the value if symbol is marked with read_only_ignore.
						//
						bool no_const = false;
						for (size_t i = 0; i != value.size(); i++) {
							if (auto s = analysis::find_rva_set_eq(img->symbols, rva + i); s && s->read_only_ignore) {
								no_const = true;
								break;
							}
						}
						if (!no_const)
							n += ins->replace_all_uses_with(std::move(value));
					}
				}
			}
		}
		return util::complete(bb, n);
	}
};
