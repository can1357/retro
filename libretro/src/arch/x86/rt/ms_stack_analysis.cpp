#include <retro/arch/x86.hpp>
#include <retro/arch/x86/regs.hxx>
#include <retro/core/callbacks.hpp>
#include <retro/core/image.hpp>
#include <retro/core/workspace.hpp>
#include <retro/ir/basic_block.hpp>
#include <retro/ir/insn.hpp>
#include <retro/opt/interface.hpp>

using namespace retro;
using namespace retro::core;
using namespace retro::arch;

/*RC_INSTALL_CB(on_irp_init_prologue, prologue_x86_64, ir::basic_block* bb) {
	auto method = bb->get_method();
	if (method->arch.get_hash() != x86_64 || method->img->abi_name != "ms")
		return;
	fmt::println("prologue_x86_64");

	ir::opt::p0::reg_move_prop(bb);
	ir::opt::const_fold(bb);
	ir::opt::id_fold(bb);
	ir::opt::ins_combine(bb);
	ir::opt::const_fold(bb);
	ir::opt::id_fold(bb);
	fmt::println(bb->to_string());
}
RC_INSTALL_CB(on_irp_init_epilogue, epilogue_x86_64, ir::basic_block* bb) {
	auto method = bb->get_method();
	if (method->arch.get_hash() != x86_64 || method->img->abi_name != "ms")
		return;
	fmt::println("epilogue_x86_64");

	
	ir::opt::p0::reg_move_prop(bb);
	ir::opt::const_fold(bb);
	ir::opt::id_fold(bb);
	ir::opt::ins_combine(bb);
	ir::opt::const_fold(bb);
	ir::opt::id_fold(bb);
	fmt::println(bb->to_string());
}*/