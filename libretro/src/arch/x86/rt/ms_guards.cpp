#include <retro/core/callbacks.hpp>
#include <retro/core/image.hpp>
#include <retro/core/workspace.hpp>
#include <retro/ir/basic_block.hpp>
#include <retro/ir/insn.hpp>
#include <retro/arch/x86/regs.hxx>
#include <retro/arch/x86.hpp>

using namespace retro;
using namespace retro::core;
using namespace retro::arch;

static constexpr u8 alloca_probe_x86_64[] = {
	 0x48, 0x83, 0xEC, 0x10,										  //  sub     rsp, 10h
	 0x4C, 0x89, 0x14, 0x24,										  //  mov     [rsp+10h+var_10], r10
	 0x4C, 0x89, 0x5C, 0x24, 0x08,								  //  mov     [rsp+10h+var_8], r11
	 0x4D, 0x33, 0xDB,												  //  xor     r11, r11
	 0x4C, 0x8D, 0x54, 0x24, 0x18,								  //  lea     r10, [rsp+10h+arg_0]
	 0x4C, 0x2B, 0xD0,												  //  sub     r10, rax
	 0x4D, 0x0F, 0x42, 0xD3,										  //  cmovb   r10, r11
	 0x65, 0x4C, 0x8B, 0x1C, 0x25, 0x10, 0x00, 0x00, 0x00,  //  mov     r11, gs:10h
	 0x4D, 0x3B, 0xD3,												  //  cmp     r10, r11
	 0x73, 0x16,														  //  jnb     short cs20
	 0x66, 0x41, 0x81, 0xE2, 0x00, 0xF0,						  //  and     r10w, 0F000h
	 0x4D, 0x8D, 0x9B, 0x00, 0xF0, 0xFF, 0xFF,				  //  lea     r11, [r11-1000h]
	 0x41, 0xC6, 0x03, 0x00,										  //  mov     byte ptr [r11], 0
	 0x4D, 0x3B, 0xD3,												  //  cmp     r10, r11
	 0x75, 0xF0,														  //  jnz     short cs10
	 0x4C, 0x8B, 0x14, 0x24,										  //  mov     r10, [rsp+10h+var_10]
	 0x4C, 0x8B, 0x5C, 0x24, 0x08,								  //  mov     r11, [rsp+10h+var_8]
	 0x48, 0x83, 0xC4, 0x10,										  //  add     rsp, 10h
	 0xC3																	  //  retn
};

static constexpr u8 alloca_probe_i386[] = {
	 0x51,								  //  push    ecx
	 0x8D, 0x4C, 0x24, 0x04,		  //  lea     ecx, [esp+4]
	 0x2B, 0xC8,						  //  sub     ecx, eax
	 0x1B, 0xC0,						  //  sbb     eax, eax
	 0xF7, 0xD0,						  //  not     eax
	 0x23, 0xC8,						  //  and     ecx, eax
	 0x8B, 0xC4,						  //  mov     eax, esp
	 0x25, 0x00, 0xF0, 0xFF, 0xFF,  //  and     eax, 0FFFFF000h
	 0x3B, 0xC8,						  //  cmp     ecx, eax
	 0x72, 0x0A,						  //  jb      short loc_4010C2
	 0x8B, 0xC1,						  //  mov     eax, ecx
	 0x59,								  //  pop     ecx
	 0x94,								  //  xchg    eax, esp
	 0x8B, 0x00,						  //  mov     eax, [eax]
	 0x89, 0x04, 0x24,				  //  mov     [esp+0], eax
	 0xC3,								  //  retn
	 0x2D, 0x00, 0x10, 0x00, 0x00,  //  sub     eax, 1000h
	 0x85, 0x00,						  //  test    [eax], eax
	 0xEB, 0xE9							  //  jmp     short loc_4010B4
};

// Removes retpoline artifacts.
//
RC_INSTALL_CB(on_irp_init_xcall, retpoline_remove, ir::insn* i) {
	auto method = i->get_method();
	if (method->arch.get_hash() != x86_64 || method->img->abi_name != "ms")
		return false;

	// If address is constant:
	//
	if (i->opr(0).is_const()) {
		static constexpr u8 sample[] = {0x48, 0x83, 0xC4, 0x08}; // add rsp, 8

		// If data matches:
		//
		auto va	 = (u64) i->opr(0).get_const().get<ir::pointer>();
		auto data = method->img->slice(va - method->img->base_address);
		if (data.size() > sizeof(sample) && !memcmp(data.data(), sample, sizeof(sample))) {
			// Replace with a XJMP, change the destination to skip the stack pointer change.
			//
			i->op = ir::opcode::xjmp;
			i->opr(0) = va + sizeof(sample);
			return true;
		}
	}
	return false;
}

// Removes __guard_dispatch_icall_fptr artifacts.
//
RC_INSTALL_CB(on_irp_init_xcall, cfg_remove, ir::insn* i) {
	auto method = i->get_method();
	if (!method->arch->is<x86arch>() || method->img->abi_name != "ms")
		return false;

	// Pick register.
	//
	x86::reg reg;
	switch (method->arch.get_hash()) {
		case x86_64:
			reg = x86::reg::rax;
			break;
		case i386:
			reg = x86::reg::eax;
			break;
		default:
			RC_UNREACHABLE();
	}

	// Skip if target is not a load_mem instruction.
	//
	auto& target = i->opr(0);
	if (target.is_const())
		return false;
	auto* ins = target.get_value()->get_if<ir::insn>();
	if (!ins || ins->op != ir::opcode::load_mem)
		return false;

	// Skip if address is not constant.
	//
	auto& adr = ins->opr(0);
	if (!adr.is_const())
		return false;

	// Lookup the symbol, if it matches __guard_dispatch_icall_fptr:
	//
	auto img = i->get_image();
	auto rva = adr.get_const().get_u64() - img->base_address;
	auto sym = find_rva_set_eq(img->symbols, rva);
	if (sym && sym->name == "__guard_dispatch_icall_fptr") {
		// Replace target operand with a read register.
		//
		auto regv = i->bb->insert(i, ir::make_read_reg(ir::type::pointer, reg));
		target	 = regv.get();
	}
	return false;
}

// Removes __alloca_probe artifacts.
//
RC_INSTALL_CB(on_irp_init_xcall, stack_probe_remove, ir::insn* i) {
	auto method = i->get_method();
	if (!method->arch->is<x86arch>() || method->img->abi_name != "ms")
		return false;

	// Get the relavant sample.
	//
	std::span<const u8> sample;
	switch (method->arch.get_hash()) {
		case x86_64:
			sample = alloca_probe_x86_64;
			break;
		case i386:
			sample = alloca_probe_i386;
			break;
		default:
			RC_UNREACHABLE();
	}

	// If address is constant:
	//
	if (i->opr(0).is_const()) {
		// If data matches, remove the call.
		//
		auto va	 = (u64) i->opr(0).get_const().get<ir::pointer>();
		auto data = method->img->slice(va - method->img->base_address);
		if (data.size() > sample.size() && !memcmp(data.data(), sample.data(), sample.size())) {
			i->op = ir::opcode::nop;
			return true;
		}
	}
	return false;
}