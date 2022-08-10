#include <retro/arch/x86.hpp>
#include <retro/arch/x86/callconv.hpp>
#include <retro/arch/x86/regs.hxx>
#include <retro/core/callbacks.hpp>
#include <retro/core/image.hpp>
#include <retro/core/workspace.hpp>
#include <retro/ir/z3x.hpp>
#include <retro/ir/basic_block.hpp>
#include <retro/ir/insn.hpp>

using namespace retro;
using namespace retro::core;
using namespace retro::arch;

RC_INSTALL_CB(on_cc_analysis, msabi_x64, ir::routine* rtn, irp_phi_info& info) {
	auto method = rtn->method;
	if (method->arch.get_hash() != x86_64 || method->img->abi_name != "ms")
		return false;

	z3x::variable_set vs	 = {};
	auto&					ctx = z3x::get_context();

	// Write some assumptions.
	//
	rtn->entry_point->insert(rtn->entry_point->begin(), ir::make_write_reg(x86::reg::flag_df, false));
	// TODO: X87 stack == empty on i386 version.

	// There's only one valid calling convention and SP is always callee adjusted, so apply it over all XCALL and XRETs.
	//
	auto* cc = &x86::cc_msabi_x86_64;
	info.cc = cc;
	for (auto& bb : rtn->blocks) {
		bb->erase_if([&](ir::insn* i) {
			// If XCALL:
			//
			if (i->op == ir::opcode::xcall) {
				// Read all registers.
				//
				auto args	  = bb->insert(i, ir::make_context_begin(bb->insert(i, ir::make_read_reg(ir::type::pointer, arch::x86::reg::rsp)).get()));
				auto push_reg = [&](arch::mreg a) {
					if (a) {
						auto val = bb->insert(i, ir::make_read_reg(enum_reflect(a.get_kind()).type, a));
						args		= bb->insert(i, ir::make_insert_context(args.get(), a, val.get()));
					}
				};
				range::for_each(cc->argument_gpr, push_reg);
				range::for_each(cc->argument_fp, push_reg);
				push_reg(cc->fp_varg_counter);

				// Create the call.
				//
				auto res		 = bb->insert(i, ir::make_call(i->opr(0), args.get()));
				auto pop_reg = [&](arch::mreg a) {
					if (a) {
						auto& desc = enum_reflect(a.get_kind());
						auto	val  = bb->insert(i, ir::make_extract_context(desc.type, res.get(), a));
						i->arch->explode_write_reg(bb->insert(i, ir::make_write_reg(a, val.get())));
					}
				};

				// Write back the result.
				//
				range::for_each(cc->retval_gpr, pop_reg);
				range::for_each(cc->retval_fp, pop_reg);

				// Trash arguments that cannot be retvals.
				//
				for (auto* aset : {&cc->argument_gpr, &cc->argument_fp}) {
					auto* oset = aset == &cc->argument_gpr ? &cc->retval_gpr : &cc->retval_fp;
					for (auto& arg : *aset) {
						if (!range::find(*oset, arg)) {
							auto a  = bitcast<arch::mreg>(arg);
							auto ty = enum_reflect(a.get_kind()).type;
							auto ud = bb->insert(i, ir::make_undef(ty));
							i->arch->explode_write_reg(bb->insert(i, ir::make_write_reg(a, ud.get())));
						}
					}
				}

				// Erase the XCALL.
				//
				return true;
			}
			// If XRET:
			//
			else if (i->op == ir::opcode::xret) {
				auto sp	  = bb->insert(i, ir::make_read_reg(ir::type::pointer, arch::x86::reg::rsp));
				auto spex  = z3x::to_expr(vs, ctx, sp);
				auto retex = z3x::to_expr(vs, ctx, i->opr(0));

				i64 delta = 8;
				if (z3x::ok(spex) && z3x::ok(retex)) {
					if (auto delta_expr = z3x::value_of(spex - retex, true)) {
						delta = delta_expr.get_i64();
					}
				}

				auto args = bb->insert(i, ir::make_context_begin(i->opr(0)));

				// Read the results.
				//
				auto push_reg = [&](arch::mreg a) {
					if (a) {
						auto val = bb->insert(i, ir::make_read_reg(enum_reflect(a.get_kind()).type, a));
						args		= bb->insert(i, ir::make_insert_context(args.get(), a, val.get()));
					}
				};
				range::for_each(cc->retval_gpr, push_reg);
				range::for_each(cc->retval_fp, push_reg);

				// Create the ret.
				//
				bb->insert(i, ir::make_ret(args.get(), delta));
				return true;
			}
			// If stack_reset:
			//
			else if (i->op == ir::opcode::stack_reset) {
				return true;
			}
			return false;
		});
	}
	return true;
}