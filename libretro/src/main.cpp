#include <retro/common.hpp>
#include <retro/core/image.hpp>
#include <retro/core/workspace.hpp>
#include <retro/ir/basic_block.hpp>
#include <retro/ir/routine.hpp>
#include <retro/ir/insn.hpp>
#include <retro/llvm/clang.hpp>
#include <retro/bind/js.hpp>

#include <retro/core/callbacks.hpp>
#include <Zydis/Zydis.h>

namespace retro::bind {

	// Machine instructions.
	//
	template<>
	struct type_descriptor<arch::imm> : user_class<arch::imm> {
		inline static constexpr const char* name = "MImm";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_property("width", [](arch::imm* i) { return i->width; });
			proto.add_property("isSigned", [](arch::imm* i) { return i->is_signed; });
			proto.add_property("isRelative", [](arch::imm* i) { return i->is_relative; });
			proto.add_property("s", [](arch::imm* i) { return i->s; });
			proto.add_property("u", [](arch::imm* i) { return i->u; });

			proto.add_method("getSigned", [](arch::imm* i, std::optional<u64> ip) { return i->get_signed(ip.value_or(0)); });
			proto.add_method("getUnsigned", [](arch::imm* i, std::optional<u64> ip) { return i->get_unsigned(ip.value_or(0)); });
			
			proto.add_method("toString", [](arch::imm* m, std::optional<arch::handle>, std::optional<u64> ip) { return m->to_string(ip.value_or(0)); });
		}
	};
	template<>
	struct type_descriptor<arch::mreg> : user_class<arch::mreg> {
		inline static constexpr const char* name = "MReg";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_property("id", [](arch::mreg* m) { return m->id; });
			proto.add_property("kind", [](arch::mreg* m) { return m->get_kind(); });
			proto.add_property("uid", [](arch::mreg* m) { return m->uid(); });
			proto.add_method("equals", [](arch::mreg* m, arch::mreg* o) { return *m == *o; });

			proto.add_method("getName", [](arch::mreg* m, std::optional<arch::handle> h) { return m->name(h.value_or(std::nullopt).get()); });
			proto.add_method("toString", [](arch::mreg* m, std::optional<arch::handle> h, std::optional<u64>) { return m->to_string(h.value_or(std::nullopt).get()); });
		}
	};
	template<>
	struct type_descriptor<arch::mem> : user_class<arch::mem> {
		inline static constexpr const char* name = "MMem";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_property("width", [](arch::mem* m) { return m->width; });
			proto.add_property("segVal", [](arch::mem* m) { return m->segv; });
			proto.add_property("scale", [](arch::mem* m) { return m->scale; });
			proto.add_property("disp", [](arch::mem* m) { return m->disp; });
			proto.add_property("seg", [](arch::mem* m) -> std::optional<arch::mreg> {
				if (m->segr)
					return m->segr;
				return std::nullopt;
			});
			proto.add_property("index", [](arch::mem* m) -> std::optional<arch::mreg> {
				if (m->index)
					return m->index;
				return std::nullopt;
			});
			proto.add_property("base", [](arch::mem* m) -> std::optional<arch::mreg> {
				if (m->base)
					return m->base;
				return std::nullopt;
			});
			proto.add_method("toString", [](arch::mem* m, std::optional<arch::handle> h, std::optional<u64> ip) { return m->to_string(h.value_or(std::nullopt).get(), ip.value_or(0)); });
		}
	};
	template<>
	struct type_descriptor<arch::minsn> : user_class<arch::minsn> {
		inline static constexpr const char* name = "MInsn";

		template<typename Proto>
		static void write(Proto& proto) {
			using engine = typename Proto::engine_type;

			proto.add_property("name", [](arch::minsn* i) { return i->name(); });
			proto.add_property("arch", [](arch::minsn* i) { return arch::handle(i->arch); });
			proto.add_property("mnemonic", [](arch::minsn* i) { return i->mnemonic; });
			proto.add_property("modifiers", [](arch::minsn* i) { return i->modifiers; });
			proto.add_property("effectiveWidth", [](arch::minsn* i) { return i->effective_width; });
			proto.add_property("length", [](arch::minsn* i) { return i->length; });
			proto.add_property("operandCount", [](arch::minsn* i) { return i->operand_count; });
			proto.add_property("isSupervisor", [](arch::minsn* i) { return i->is_supervisor; });

			proto.add_method("getOperand", [](const engine& e, arch::minsn* ins, u32 i) {
				using value = typename engine::value_type;
				if (ins->operand_count > i) {
					if (ins->op[i].type == arch::mop_type::reg)
						return value::make(e, ins->op[i].r);
					else if (ins->op[i].type == arch::mop_type::mem)
						return value::make(e, ins->op[i].m);
					else if (ins->op[i].type == arch::mop_type::imm)
						return value::make(e, ins->op[i].i);
				}
				return value::make(e, std::nullopt);
			});
			proto.add_method("toString", [](arch::minsn* i, std::optional<u64> ip) {
				return i->to_string(ip.value_or(0));
			});
		}
	};

	// IR types.
	//
	template<>
	struct type_descriptor<ir::insn> : user_class<ir::insn> {
		inline static constexpr const char* name = "Insn";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_method(
				 "toString", [](ir::insn* r, std::optional<bool> full) { return r->to_string(full.value_or(false) ? ir::fmt_style::full : ir::fmt_style::concise); });
			proto.add_property("image", [](ir::insn* r) { return r->get_image(); });
			proto.add_property("workspace", [](ir::insn* r) { return r->get_workspace(); });
			proto.add_property("routine", [](ir::insn* r) { return r->get_routine(); });
			//			proto.add_property("method", [](ir::insn* r) { return r->get_method(); });
			proto.add_property("block", [](ir::insn* r) { return r->bb; });

			proto.add_method("validate", [](ir::insn* r) { r->validate().raise(); });
			proto.add_property("arch", [](ir::insn* r) { return r->arch; });
			proto.add_property("ip", [](ir::insn* r) { return r->ip; });
			proto.add_property("name", [](ir::insn* r) { return r->name; });
			proto.add_property("isOprhan", [](ir::insn* r) { return r->is_orphan(); });
			proto.add_property("opcode", [](ir::insn* r) { return r->op; });
			proto.add_property("operandCount", [](ir::insn* i) { return i->operand_count; });
			proto.add_property("templates", [](ir::insn* i) { return i->template_types; });
		}
	};
	template<>
	struct type_descriptor<ir::basic_block> : user_class<ir::basic_block> {
		inline static constexpr const char* name = "BasicBlock";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_method("toString", [](ir::basic_block* r, std::optional<bool> full) { return r->to_string(full.value_or(false) ? ir::fmt_style::full : ir::fmt_style::concise); });
			proto.add_property("image", [](ir::basic_block* r) { return r->get_image(); });
			proto.add_property("workspace", [](ir::basic_block* r) { return r->get_workspace(); });
			proto.add_property("routine", [](ir::basic_block* r) { return r->rtn; });
			//			proto.add_property("method", [](ir::basic_block* r) { return r->get_method(); });

			proto.add_method("validate", [](ir::basic_block* r) { r->validate().raise(); });
			proto.add_property("successors", [](ir::basic_block* r) { return r->successors; });
			proto.add_property("predecessors", [](ir::basic_block* r) { return r->predecessors; });
			proto.add_property("arch", [](ir::basic_block* r) { return r->arch; });
			proto.add_property("ip", [](ir::basic_block* r) { return r->ip; });
			proto.add_property("endIp", [](ir::basic_block* r) { return r->end_ip; });
			proto.add_property("name", [](ir::basic_block* r) { return r->name; });

			// TODO: Phi stuff.
			proto.add_property("terminator", [](ir::basic_block* r) { return r->terminator(); });
			proto.add_property("phis", [](ir::basic_block* r) { return r->phis(); });

			proto.make_iterable([](ir::basic_block * r) { return r->insns(); });
		}
	};
	template<>
	struct type_descriptor<ir::routine> : user_class<ir::routine> {
		inline static constexpr const char* name = "Routine";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_method("toString", [](ir::routine* r, std::optional<bool> full) { return r->to_string(full.value_or(false) ? ir::fmt_style::full : ir::fmt_style::concise); });
			proto.add_property("image", [](ir::routine* r) { return r->get_image(); });
			proto.add_property("workspace", [](ir::routine* r) { return r->get_workspace(); });
			//proto.add_property("method", [](ir::routine* r) { return r->method.get(); });
			proto.add_method("validate", [](ir::routine* r) { r->validate().raise(); });
			proto.add_method("renameBlocks", [](ir::routine* r) { r->rename_blocks(); });
			proto.add_method("renameInsns", [](ir::routine* r) { r->rename_insns(); });
			proto.add_method("topologicalSort", [](ir::routine* r) { r->topological_sort(); });

			// TODO: method

			proto.add_property("ip", [](ir::routine* r) { return r->ip; });
			proto.add_property("entryPoint", [](ir::routine* r) { return r->entry_point.get(); });

			proto.make_iterable([](ir::routine * r) -> auto& { return r->blocks; });
		}
	};

	// Arch interface.
	//
	template<>
	struct type_descriptor<arch::instance> : user_class<arch::instance> {
		inline static constexpr const char* name = "Arch";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_property("ptrType", [](arch::handle i) {
				return i->ptr_type();
			});
			proto.add_property("effectivePtrWidth", [](arch::handle i) {
				return i->get_effective_pointer_width();
			});
			proto.add_property("ptrWidth", [](arch::handle i) {
				return i->get_pointer_width();
			});
			proto.add_property("isLittleEndian", [](arch::handle i) {
				return i->get_byte_order() == std::endian::little;
			});
			proto.add_property("isBigEndian", [](arch::handle i) {
				return i->get_byte_order() == std::endian::big;
			});
			proto.add_property("stackRegister", [](arch::handle i) {
				return i->get_stack_register();
			});

			proto.add_method("formatInsnModifiers", [](arch::handle i, arch::minsn* m) {
				return i->format_minsn_modifiers(*m);
			});
			proto.add_method("nameMnemonic", [](arch::handle i, u32 m) {
				return i->name_mnemonic(m);
			});
			proto.add_method("nameRegister", [](arch::handle i, arch::mreg* r) {
				return i->name_register(*r);
			});
			proto.add_method("lift", [](arch::handle i, ir::basic_block* bb, const arch::minsn& m, u64 ip) {
				i->lift(bb, m, ip).raise();
			});
			proto.add_method("disasm", [](arch::handle i, std::span<const u8> data) {
				auto m = std::make_unique<arch::minsn>();
				if (!i->disasm(data, m.get())) {
					m.reset();
				}
				return m;
			});

			/*
				TODO:
				virtual const call_conv_desc* get_cc_desc(call_conv cc) = 0;
				virtual mreg_info get_register_info(mreg r) { return {r}; }
				virtual void		for_each_subreg(mreg r, function_view<void(mreg)> f) {}
				virtual ir::insn* explode_write_reg(ir::insn* i) { return i; }
			*/
		}
	};

	template<>
	struct type_descriptor<core::image> : user_class<core::image> {
		inline static constexpr const char* name = "Image";
		
		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_property("name", [](core::image* i) { return i->name; });
			proto.add_property("kind", [](core::image* i) { return i->kind; });
			proto.add_property("baseAddress", [](core::image* i) { return i->base_address; });
			proto.add_property("ldr", [](core::image* i) { return i->ldr; });
			proto.add_property("arch", [](core::image* i) { return i->arch; });
			proto.add_property("abiName", [](core::image* i) { return i->abi_name; });
			proto.add_property("envName", [](core::image* i) { return i->env_name; });
			proto.add_property("isEnvSupervisor", [](core::image* i) { return i->env_supervisor; });
			proto.add_property("entryPoints", [](core::image* i) { return i->entry_points; });
			
			proto.add_method("slice", [](core::image* i, u64 rva, u64 len) {
				auto result = i->slice(rva);
				if (result.size() >= len)
					result = result.subspan(0, len);
				return result;
			});

			proto.add_async_method("lift", [](core::image* img, u64 rva) {
				auto m	= core::lift(img, rva, &img->ws->user_analysis_scheduler);
				return weak<ir::routine>{m->wait_for_irp(core::IRP_INIT)};
			});


			// TODO:
			// std::vector<section> sections = {};
			// std::vector<reloc>	relocs	= {};
			// std::vector<symbol>	symbols	= {};
			//TODO: proto.add_property("defaultCc", [](core::image* i) { return i->default_cc; });
		}
	};

	template<>
	struct type_descriptor<ldr::instance> : user_class<ldr::instance> {
		inline static constexpr const char* name = "Loader";

		
		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_method("match", [](ldr::handle l, std::vector<u8> data) { return l->match(data); });
			proto.add_property("extensions", [](ldr::handle l) {
				auto e = l->get_extensions();
				return std::vector<std::string_view>{e.begin(), e.end()};
			});
		}
	};

	template<>
	struct type_descriptor<core::workspace> : user_class<core::workspace> {
		inline static constexpr const char* name = "Workspace";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_static_method("create", []() {
				return core::workspace::create();
			});
			proto.add_property("numImages", [](core::workspace* ws) {
				std::shared_lock _g{ws->image_list_mtx};
				return (u32)ws->image_list.size();
			});
			proto.add_async_method("loadImage", [](core::workspace* ws, std::string path, std::optional<ldr::handle> ldr) {
				return ws->load_image(path, ldr.value_or(std::nullopt)).value();
			});
			proto.add_async_method("loadImageInMemory", [](core::workspace* ws, std::vector<u8> data, std::optional<ldr::handle> ldr) {
				return ws->load_image_in_memory(data, ldr.value_or(std::nullopt)).value();
			});
		}
	};
};


using namespace retro;
using Engine = bind::js::engine; // template<typename Engine>
static void export_api(const Engine& eng, const Engine::object_type& mod) {
	using object = typename Engine::object_type;
	using function = typename Engine::function_type;

	/*
	TODO:
	 Try to alocate less :(
	 Insn operands
	 Optimizers
	 Insn::uses, Insn::replace_all_uses
	 Section symbol reloc
	 Creating/Erasing instructions
	 Callbacks, interface registration
	 Z3x
	 Clang
	*/

	eng.export_type<ir::insn>(mod);
	eng.export_type<ir::basic_block>(mod);
	eng.export_type<ir::routine>(mod);
	eng.export_type<arch::imm>(mod);
	eng.export_type<arch::mreg>(mod);
	eng.export_type<arch::mem>(mod);
	eng.export_type<arch::minsn>(mod);
	eng.export_type<arch::instance>(mod);
	eng.export_type<ldr::instance>(mod);
	eng.export_type<core::image>(mod);
	eng.export_type<core::workspace>(mod);

	auto clang = object::make(eng, 3);
	clang.set("locate", function::make(eng, "clang.locate", [](std::optional<std::string> at) {
		std::optional<std::string_view> result;
		if (auto res = llvm::locate_install(at.value_or(std::string{})))
			result.emplace(*res);
		return result;
	}));
	clang.set("compile", function::make_async(eng, "clang.compile", [](std::string source, std::optional<std::string> arguments) {
		std::string err;
		if (!arguments)
			arguments.emplace();
		auto res = llvm::compile(source, *arguments, &err);
		if (!err.empty())
			throw std::runtime_error(std::move(err));
		return res;
	}));
	clang.set("format", function::make_async(eng, "clang.format", [](std::string source, std::optional<std::string> style) {
		std::string err;
		if (!style)
			style.emplace();
		auto res = llvm::format(source, *style, &err);
		if (!err.empty())
			throw std::runtime_error(std::move(err));
		return res;
	}));
	clang.freeze();
	mod.set("Clang", clang);
	mod.freeze();
}



NAPI_MODULE_INIT() {
	try {
		core::on_minsn_lift.insert([](arch::handle arch, ir::basic_block* bb, arch::minsn& i, u64 va) {
			if (i.mnemonic == ZYDIS_MNEMONIC_VMREAD) {
				auto str		= (const char*) bb->get_image()->slice(i.op[0].m.disp + va + i.length - bb->get_image()->base_address).data();
				auto result = bb->push_annotation(ir::int_type(i.op[1].get_width()), std::string_view{str});
				auto write	= bb->push_write_reg(i.op[1].r, result);
				arch->explode_write_reg(write);
				return true;
			} else if (i.mnemonic == ZYDIS_MNEMONIC_VMWRITE) {
				auto str	 = (const char*) bb->get_image()->slice(i.op[1].m.disp + va + i.length - bb->get_image()->base_address).data();
				auto read = bb->push_read_reg(ir::int_type(i.op[0].get_width()), i.op[0].r);
				bb->push_annotation(ir::type::none, std::string_view{str}, read);
				return true;
			}
			return false;
		});

		bind::js::engine ctx{env};
		bind::js::object mod{env, exports};
		bind::js::typedecl::init(env);

		export_api(ctx, mod);


		return exports;
	} catch (std::exception ex) {
		napi_throw_error(env, nullptr, ex.what());
		return {};
	}
}

#if RC_WINDOWS
	#include <Windows.h>
	#include <DbgHelp.h>
	#pragma comment(lib, "Dbghelp.lib")

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
	SetErrorMode(0);
	SetUnhandledExceptionFilter([](PEXCEPTION_POINTERS ep) -> LONG {
		auto file = CreateFileA("crashdump.dmp", GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

		MINIDUMP_EXCEPTION_INFORMATION ex_info;
		ex_info.ThreadId			  = GetCurrentThreadId();
		ex_info.ExceptionPointers = ep;
		ex_info.ClientPointers	  = FALSE;

		BOOL state =
			 MiniDumpWriteDump((HANDLE) -1, GetCurrentProcessId(), file, MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory), &ex_info, nullptr, nullptr);
		if (state) {
			printf("Exception 0x%08x @ %p, %s", ep->ExceptionRecord->ExceptionCode, ep->ExceptionRecord->ExceptionAddress, state ? "Written minidump." : "Failed to write minidump.");
		}
		return EXCEPTION_EXECUTE_HANDLER;
	});

	return TRUE;
}
#endif