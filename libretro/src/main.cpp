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

#include <retro/opt/interface.hpp>
#include <retro/opt/utility.hpp>


namespace retro::analysis {

	struct workspace;

	// Domain statistics.
	//
	struct domain_stats {
		// Machine instructions diassembled.
		//
		std::atomic<u64> minsn_disasm = 0;

		// IR instructions created to represent the disassembled instructions.
		//
		std::atomic<u64> insn_lifted = 0;

		// Blocks parsed.
		//
		std::atomic<u64> block_count = 0;
	};

	// Lifter parameters.
	//
	struct lifter_params {
		arch::handle					 arch = {};
	};

	// Domain represents the analysis state associated with a single image.
	//
	struct domain {
		// Owning workspace.
		//
		weak<workspace> parent = {};

		// Image details.
		//
		ref<ldr::image> img			  = {};
		domain_stats	 stats		  = {};

		// Loader provided defaults.
		//
		arch::handle					 arch = {};
		const arch::call_conv_desc* default_cc	  = nullptr;

		// Lifts a routine given the RVA.
		//
		ref<ir::routine> lift_routine(u64 rva, const lifter_params& p);
		ref<ir::routine> lift_routine(u64 rva) { return lift_routine(rva, {.arch = arch}); }

		// Given a xcall instruction or the routine determines the calling convention. 
		//
		const arch::call_conv_desc* get_routine_cc(ir::routine* rtn);
		const arch::call_conv_desc* get_routine_cc(ir::insn* xcall);
	};

	// Workspace holds the document state and may represent multiple images.
	//
	struct workspace {
		// Workspace list.
		//
		rw_lock						 domain_list_lock = {};
		std::vector<ref<domain>> domain_list;

		// Creates a new workspace.
		//
		static ref<workspace> create() { return make_rc<workspace>(); }

		// Creates a domain for the image.
		//
		ref<domain> add_image(ref<ldr::image> img) {
			std::unique_lock _g {domain_list_lock};

			auto dom				= domain_list.emplace_back(make_rc<domain>());
			dom->parent			= this;
			dom->img				= img;
			dom->arch			= arch::instance::lookup(img->arch_hash);
			if (dom->arch) {
				dom->default_cc = dom->arch->get_cc_desc(img->default_call_conv);
			}
			return dom;
		}
	};

	// Given a routine determines the calling convention.
	//
	const arch::call_conv_desc* domain::get_routine_cc(ir::routine* rtn) {
		// TODO: logic.
		RC_UNUSED(rtn);
		return default_cc;
	}
	const arch::call_conv_desc* domain::get_routine_cc(ir::insn* xcall) {
		// TODO: logic.
		RC_UNUSED(xcall);
		return default_cc;
	}

	static ir::basic_block* lift_block(domain* dom, ir::routine* rtn, u64 va, const lifter_params& p) {
		// Invalid jump if out of image boundaries.
		//
		std::span<const u8> data = dom->img->slice(va - dom->img->base_address);
		if (data.empty()) {
			return nullptr;
		}

		// First check the basic block list for an already lifted range.
		//
		for (ir::basic_block* bbs : rtn->blocks) {
			// Skip if not within the range.
			//
			if (va < bbs->ip || bbs->end_ip <= va)
				continue;

			// If exact match, no need to lift anything.
			//
			if (bbs->ip == va)
				return bbs;

			// Find the label.
			//
			for (auto* ins : bbs->insns()) {
				if (ins->ip != va) {
					continue;
				}

				// Split the block, add a jump from the previous block to this one.
				//
				auto new_block = bbs->split(ins);
				if (!new_block)
					return bbs;
				bbs->push_jmp(new_block);
				bbs->add_jump(new_block);

				// Return it.
				//
				return new_block;
			}

			// Misaligned jump, sneaky! Lift as a new block.
			//
			fmt::println("Misaligned jump?");
		}

		// Add a new block.
		//
		dom->stats.block_count++;
		auto* bb	  = rtn->add_block();
		bb->ip	  = va;
		bb->end_ip = va;
		bb->arch	  = p.arch;
		while (!data.empty()) {
			// Diassemble the instruction, push trap on failure and break.
			//
			arch::minsn ins = {};
			if (!p.arch->disasm(data, &ins)) {
				bb->push_trap("undefined opcode")->ip = va;
				break;
			}
			dom->stats.minsn_disasm++;

			// Update the block range.
			//
			bb->end_ip = va + ins.length;

			// Lift the instruction, push trap on failure and break.
			//
			if (auto err = p.arch->lift(bb, ins, va)) {
				bb->push_trap("lifter error: " + err.to_string())->ip = va;
				// TODO: Log
				fmt::println(err.to_string());
				break;
			}
			if (!bb->empty()) {
				auto it = std::prev(bb->end());
				while (it != bb->begin() && it->prev->ip == va) {
					--it;
				}
				dom->stats.insn_lifted += std::distance(it, bb->end());
			}

			// If last instruction is a terminator break out.
			//
			if (bb->terminator() != nullptr)
				break;

			// Skip the bytes and increment IP.
			//
			data = data.subspan(ins.length);
			va += ins.length;
		}

		// Try to continue traversal.
		//
		z3x::variable_set vs;

		auto coerce_const = [&](ir::operand& op) {
			if (op.is_const())
				return true;
			if (auto expr = z3x::to_expr(vs, z3x::get_context(), op)) {
				if (auto v = z3x::value_of(expr); !v.is<void>()) {
					v.type_id = (u64) op.get_type();	 // TODO: Replace with a proper cast.
					op			 = std::move(v);
					return true;
				}
			}
			return false;
		};

		auto* term = bb->terminator();
		switch (term->op) {
			case ir::opcode::xjs: {
				// If condition cannot be coerced to a constant:
				//
				if (auto cc = coerce_const(term->opr(0)); !cc) {
					ir::basic_block* bbs[2] = {nullptr, nullptr};
					for (size_t i = 0; i != 2; i++) {
						// Try coercing destination into a constant.
						//
						if (coerce_const(term->opr(i + 1))) {
							bbs[i] = lift_block(dom, rtn, (u64) term->opr(i + 1).const_val.get<ir::pointer>(), p);
						}
					}

					// If we managed to lift both blocks succesfully:
					//
					if (bbs[0] && bbs[1]) {
						bb = term->block;

						// Replace with js.
						//
						ir::variant cc{term->opr(0)};
						bb->push_js(std::move(cc), bbs[0], bbs[1]);
						term->erase();
						bb->add_jump(bbs[0]);
						bb->add_jump(bbs[1]);
					}
					break;
				} else {
					// Swap with a xjmp.
					//
					ir::variant target{term->opr(term->opr(0).const_val.get<bool>() ? 1 : 2)};
					auto nterm = bb->push_xjmp(std::move(target));
					std::exchange(term, nterm)->erase();
				}
				// Fallthrough to xjmp handler.
				//
				[[fallthrough]];
			}
			case ir::opcode::xjmp: {
				// Try coercing destination into a constant.
				//
				if (coerce_const(term->opr(0))) {
					// Lift the target block.
					//
					if (auto target = lift_block(dom, rtn, (u64) term->opr(0).const_val.get<ir::pointer>(), p)) {
						bb = term->block;

						// Replace with jmp if successful.
						//
						bb->push_jmp(target);
						term->erase();
						bb->add_jump(target);
					}
				} else {
					fmt::println("dynamic jump, TODO.");
				}
				break;
			}
			default:
				break;
		}
		return bb;
	}
	ref<ir::routine> domain::lift_routine(u64 rva, const lifter_params& p) {
		if (!p.arch)
			return nullptr;
		
		// Create the routine.
		//
		ref<ir::routine> rtn = make_rc<ir::routine>();
		rtn->dom					= this;
		rtn->ip					= rva + img->base_address;

		// Recursively lift starting from the entry point.
		//
		if (!lift_block(this, rtn, rva + img->base_address, p)) {
			return nullptr;
		}
		return rtn;
	}
};


namespace retro::analysis {
	// Converts xcall/xret into call/ret where possible.
	//
	static size_t apply_cc_info(ir::routine* rtn) {
		// Get domain for CC analysis.
		//
		auto* dom = rtn->dom.get();
		if (!dom)
			return 0;

		// For each instruction.
		//
		size_t n = 0;
		for (auto& bb : rtn->blocks) {
			n += bb->erase_if([&](ir::insn* i) {
				// If XCALL:
				//
				if (i->op == ir::opcode::xcall) {
					// Determine the calling convention.
					//
					auto* cc = dom->get_routine_cc(i);
					if (!cc)
						return false;

					// First read each argument:
					//
					auto args	  = bb->insert(i, ir::make_undef(ir::type::context));
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
					auto res = bb->insert(i, ir::make_call(i->opr(0), args.get()));

					// Read each result.
					//
					auto pop_reg = [&](arch::mreg a) {
						if (a) {
							auto& desc = enum_reflect(a.get_kind());
							auto	val  = bb->insert(i, ir::make_extract_context(desc.type, res.get(), a));
							i->arch->explode_write_reg(bb->insert(i, ir::make_write_reg(a, val.get())));
						}
					};
					range::for_each(cc->retval_gpr, pop_reg);
					range::for_each(cc->retval_fp, pop_reg);
					// stack if sp_caller_adjusted?
					// eflags?

					return true;
				}
				// If XRET:
				//
				else if (i->op == ir::opcode::xret) {
					// Determine the calling convention.
					//
					auto* cc = dom->get_routine_cc(rtn);
					if (!cc)
						return false;
				
					// Read each result:
					//
					auto args	  = bb->insert(i, ir::make_undef(ir::type::context));
					auto push_reg = [&](arch::mreg a) {
						if (a) {
							auto val = bb->insert(i, ir::make_read_reg(enum_reflect(a.get_kind()).type, a));
							args		= bb->insert(i, ir::make_insert_context(args.get(), a, val.get()));
						}
					};
					range::for_each(cc->retval_gpr, push_reg);
					range::for_each(cc->retval_fp, push_reg);

					// TODO: assert retptr == [rsp] from entry point??

					// Create the ret.
					//
					bb->insert(i, ir::make_ret(args.get()));
					return true;
				}
				return false;
			});
		}
		return ir::opt::util::complete(rtn, n);
	}

	//
	//
	static void auto_cc(ir::routine* rtn) {

		// Split blocks at xcall boundaries.
		//
		//graph::bfs(rtn->get_entry(), [](ir::basic_block* bb) {
		//	auto it = range::find_if(bb->insns(), [&](ir::insn* ins) { return ins->op == ir::opcode::xcall; });
		//	if (it != bb->end() && std::next(it).get() != bb->back()) {
		//		auto nb = bb->split(std::next(it));
		//		RC_ASSERT(nb);
		//		bb->push_jmp(nb);
		//		bb->add_jump(nb);
		//	}
		//});
		//
		//fmt::println(rtn->to_string());
		//fmt::println(rtn->to_string());
	}
};








static const char code_prefix[] =
#if RC_WINDOWS
	 "#define EXPORT __attribute__((noinline)) __declspec(dllexport)\n"
#else
	 "#define EXPORT __attribute__((noinline)) __attribute__((visibility(\"default\")))\n"
#endif
	 R"(
__attribute__((noinline)) static void sinkptr(void* _) { asm volatile(""); }
__attribute__((noinline)) static void sinkull(unsigned long long _) { asm volatile(""); }
__attribute__((noinline)) static void sinkll(long long _) { asm volatile(""); }
__attribute__((noinline)) static void sinku(unsigned int _) { asm volatile(""); }
__attribute__((noinline)) static void sinki(int _) { asm volatile(""); }
__attribute__((noinline)) static void sinkf(float _) { asm volatile(""); }
__attribute__((noinline)) static void sinkl(double _) { asm volatile(""); }
int main() {}
)";


static std::vector<u8> compile(std::string code, const char* args) {
	code.insert(0, code_prefix);

	// Create the temporary paths.
	//
	auto tmp_dir  = std::filesystem::temp_directory_path();
	auto in		  = tmp_dir / "retrotmp.c";
	auto out		  = tmp_dir / "retrotmp.exe";

	std::error_code ec;
	std::filesystem::remove(in, ec);
	std::filesystem::remove(out, ec);

	// Write the file.
	//
	platform::write_file(in, {(const u8*) code.data(), code.size()});

	// Create and execute the command.
	//
	auto output = platform::exec(fmt::str("%%LLVM_PATH%%/bin/clang \"%s\" -fms-extensions -o \"%s\" %s", in.string().c_str(), out.string().c_str(), args));

	// Read the file.
	//
	bool ok;
	auto result = platform::read_file(out, ok);
	if (result.empty()) {
		fmt::abort("failed to compile the code:\n%s\n", output.c_str());
	}
	return result;
}



static constexpr u8 alloca_probe_code[] = {
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


#include <retro/robin_hood.hpp>


struct stack_analysis {
	// Difference in stack pointer after a call to this function.
	//
	i32 stack_delta = 0;

	// Frame register if any used and difference from initial SP.
	//
	arch::mreg frame_reg			= {};
	i32		  frame_reg_delta = 0;

	// Min/Max of non-indexed accesses.
	//
	i32 min_access = 0;
	i32 max_access = 0;

	// Layout of registers saved on the stack frame.
	//
	flat_umap<i32, arch::mreg> save_area_layout = {};
};

struct function_analysis {
	// Analysis information.
	//
	std::optional<stack_analysis> stack_analysis = {};

	// Flags.
	//
	u32 is_noreturn : 1 = false;
	u32 is_const : 1	  = false;
};

static void phase0(ref<ir::routine> rtn) {
	auto& dom = rtn->dom;

	// Replace call to crt!__alloca_probe with nop.
	//
	for (auto& bb : rtn->blocks) {
		for (auto&& i : bb->insns()) {
			if (i->op == ir::opcode::xcall) {
				if (i->opr(0).is_const()) {
					auto va	 = (u64) i->opr(0).get_const().get<ir::pointer>();
					auto data = dom->img->slice(va - dom->img->base_address);
					if (data.size() > sizeof(alloca_probe_code)) {
						if (!memcmp(data.data(), alloca_probe_code, sizeof(alloca_probe_code))) {
							i->erase();
							break;
						}
					}
				}
			}
		}
	}


	// TODO: Call $0 => push jmp

	// Apply simple optimizations.
	//
	size_t n = 0;
	for (auto& bb : rtn->blocks) {
		n += ir::opt::p0::reg_move_prop(bb);
		n += ir::opt::const_fold(bb);
		n += ir::opt::id_fold(bb);
		n += ir::opt::ins_combine(bb);
		n += ir::opt::const_fold(bb);
		n += ir::opt::id_fold(bb);
		// TODO: Cfg optimization
	}

	// Sort topologically and rename all instructions.
	//
	rtn->topological_sort();
	rtn->rename_insns();
	printf(RC_GRAY " # Optimized " RC_RED "%llu " RC_GRAY "instructions.\n" RC_RESET, n);

	// Map of all registers with offset from SP.
	//
	constexpr i32		  no_offset			 = INT32_MAX;
	auto					  i_sp_offset_list = std::make_unique<i32[]>(rtn->next_ins_name + 1);
	flat_umap<u32, i32> r_sp_offset_map	 = {}; 

	// Analyze entry point.
	//
	auto sp = dom->arch->get_stack_register();

	for (auto& bb : rtn->blocks) {
		printf("--- block $%x --- \n", bb->name);

		r_sp_offset_map.clear();
		r_sp_offset_map[sp.uid()] = 0;

		r_sp_offset_map[arch::mreg(arch::x86::reg::rbp).uid()] = -24;

		for (auto* i : bb->insns()) {
			i_sp_offset_list[i->name] = no_offset;

			// Read reg => imap[i] := rmap[i]
			//
			if (i->op == ir::opcode::read_reg) {
				auto r = i->opr(0).get_const().get<arch::mreg>();
				if (auto it = r_sp_offset_map.find(r.uid()); it != r_sp_offset_map.end()) {
					i_sp_offset_list[i->name] = it->second;
				} else {
					i_sp_offset_list[i->name] = no_offset;
				}
				continue;
			}

			// Bit cast.
			//
			if (i->op == ir::opcode::bitcast) {
				if (i->template_types[0] == ir::type::pointer || i->template_types[1] == ir::type::pointer) {
						auto& v = i->opr(0);
						if (!v.is_const()) {
							if (auto* in = v.get_value()->get_if<ir::insn>()) {
								i_sp_offset_list[i->name] = i_sp_offset_list[in->name];
								continue;
							}
						}
				}
			}

			// Write reg => rmap[r] := imap[i]
			//
			if (i->op == ir::opcode::write_reg) {
				auto	r = i->opr(0).get_const().get<arch::mreg>();
				auto& v = i->opr(1);
				if (!v.is_const()) {
					if (auto* in = v.get_value()->get_if<ir::insn>()) {
						if (i32 o = i_sp_offset_list[in->name]; o != no_offset) {
							r_sp_offset_map[r.uid()] = o;
						} else {

							if (in->op == ir::opcode::load_mem) {
								auto& ptr = in->opr(0);
								if (!ptr.is_const() && ptr.get_value()->is<ir::insn>()) {
									if (i32 o = i_sp_offset_list[ptr.get_value()->get<ir::insn>().name]; o != no_offset) {
										fmt::println(r.to_string(dom->arch), " = {$sp + ", o, "}");
									}
								}
							} 

							r_sp_offset_map.erase(r.uid());
						}
						continue;
					}
				}
			}

			// Write to memory.
			//
			if (i->op == ir::opcode::store_mem) {
				auto& ptr = i->opr(0);
				auto& val = i->opr(1);

				if (!ptr.is_const() && ptr.get_value()->is<ir::insn>()) {
					if (i32 o = i_sp_offset_list[ptr.get_value()->get<ir::insn>().name]; o != no_offset) {
						fmt::println("storing to {$sp + ", o, "} = ", val);
					}
				}
				// noseg ptr
			}

			// XCALL.
			//
			if (i->op == ir::opcode::xcall) {
				auto cc = dom->get_routine_cc(i);
				for (auto it = r_sp_offset_map.begin(); it != r_sp_offset_map.end();) {
					if (range::find(cc->retval_gpr, retro::bit_cast<arch::mreg>(it->first)) != cc->retval_gpr.end()) {
						it = r_sp_offset_map.erase(it);
					} else {
						++it;
					}
				}
				// TODO:
			}

			// Add/Sub.
			//
			if (i->op == ir::opcode::binop) {
				auto op = i->opr(0).get_const().get<ir::op>();
				if (op == ir::op::add || op == ir::op::sub) {
					auto* o1 = &i->opr(1);
					auto* o2 = &i->opr(2);

					if (op == ir::op::add && o1->is_const())
						std::swap(o1, o2);

					if (!o1->is_const() && o2->is_const()) {
						if (auto* in = o1->get_value()->get_if<ir::insn>()) {
							if (i32 o = i_sp_offset_list[in->name]; o != no_offset) {
								i64 delta = o2->const_val.get<i64>();	// TODO: Wrong for ptr = 32
								if (op == ir::op::add) {
									o += delta;
								} else {
									o -= delta;
								}
								i_sp_offset_list[i->name] = o;
								continue;
							}
						}
					}
				}
			}
		}

		for (auto& [reg, offset] : r_sp_offset_map) {
			fmt::println("'", retro::bit_cast<arch::mreg>(reg).to_string(dom->arch), "' => $sp +", offset);
		}
	}
}


static ref<ir::routine> analysis_test(analysis::domain* dom, const std::string& name, u64 rva) {
	// Demo.
	//
	auto rtn = dom->lift_routine(rva);

	// Print statistics.
	//
	printf(RC_WHITE " ----- Routine '%s' -------\n", name.data());
	printf(RC_GRAY " # Successfully lifted " RC_VIOLET "%llu" RC_GRAY " instructions into " RC_GREEN "%llu" RC_RED " (avg: ~%llu/ins) #\n" RC_RESET, dom->stats.minsn_disasm.load(),
		 dom->stats.insn_lifted.load(), dom->stats.insn_lifted.load() / dom->stats.minsn_disasm.load());

	range::sort(rtn->blocks, [](auto& a, auto& b) { return a->ip < b->ip; });
	for (auto& bb : rtn->blocks) {
		std::string result = fmt::str(RC_CYAN "$%x:" RC_RESET, bb->name);
		result += fmt::str(RC_GRAY " [%llx => %llx]" RC_RESET, bb->ip, bb->end_ip);
		fmt::println(result);
		fmt::println("\t", bb->terminator()->to_string());
	}

	phase0(rtn);
	return rtn;
}

// Demo wrappers.
//
static void analysis_test_from_image_va(std::filesystem::path path, u64 va) {
	// Load the binary.
	//
	auto img = ldr::load_from_file(path).value();

	// Create the workspace.
	//
	auto ws		 = analysis::workspace::create();
	auto machine = arch::instance::lookup(img->arch_hash);
	auto loader	 = ldr::instance::lookup(img->ldr_hash);
	RC_ASSERT(loader && machine);
	fmt::println("-> loader:  ", loader->get_name());
	fmt::println("-> machine: ", machine->get_name());

	// Call the demo code.
	//
	auto dom = ws->add_image(std::move(img));
	analysis_test(dom, fmt::str("sub_%llx", va), va - dom->img->base_address);
}
static void analysis_test_from_source(std::string src) {
	// Compile the source code.
	//
	auto bin = compile(src, "-O1");
	auto img = ldr::load_from_memory(bin).value();

	// Create the workspace.
	//
	auto ws		 = analysis::workspace::create();
	auto machine = arch::instance::lookup(img->arch_hash);
	auto loader	 = ldr::instance::lookup(img->ldr_hash);
	RC_ASSERT(loader && machine);
	fmt::println("-> loader:  ", loader->get_name());
	fmt::println("-> machine: ", machine->get_name());

	// Add entry point symbol if none present, create the domain.
	//
	if (img->symbols.empty()) {
		img->symbols.push_back({img->base_address, "entry point"});
	}
	auto dom = ws->add_image(std::move(img));
	for (auto& sym : dom->img->symbols) {
		if (sym.name.starts_with("_"))
			continue;
		auto r = analysis_test(dom, sym.name, sym.rva);
		fmt::println(r->to_string());
	}
}


#include <retro/utf.hpp>
int main(int argv, const char** args) {
	platform::setup_ansi_escapes();

	// Large function test:
	//
	if (true) {
		analysis_test_from_image_va("S:\\Dumps\\ntoskrnl_2004.exe", 0x140A1AEE4);
	}
	// Small C file test:
	//
	else {
		std::string test_file = "S:\\Projects\\Retro\\tests\\simple.c";
		if (argv > 1) {
			test_file = args[1];
		}
		bool ok;
		auto code = platform::read_file(test_file, ok);
		if (!ok) {
			fmt::abort("failed to read the file.");
		}
		analysis_test_from_source(utf::convert<char>(code));
	}
	return 0;
}