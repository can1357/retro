#include <retro/core/image.hpp>
#include <retro/core/method.hpp>
#include <retro/core/workspace.hpp>
#include <retro/opt/interface.hpp>
#include <retro/opt/utility.hpp>

namespace retro::ir::opt {
	// Conversion of load_mem with constant address.
	//
	size_t const_load(basic_block* bb) {
		size_t n = 0;

		// For each load_mem instruction:
		//
		for (auto* ins : bb->insns()) {
			if (ins->op == opcode::load_mem) {
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
				u64  rva = adr.get_const().get_u64() + ins->opr(1).get_const().get_i64() - img->base_address;
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

					if (value) {
						// Do not propagate the value if symbol is marked with read_only_ignore.
						//
						bool no_const = false;
						for (size_t i = 0; i != value.size(); i++) {
							if (auto s = core::find_rva_set_eq(img->symbols, rva + i); s && s->read_only_ignore) {
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
