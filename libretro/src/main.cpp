#include <retro/common.hpp>
#include <retro/core/workspace.hpp>


#include <retro/bind/js.hpp>


using namespace retro;


#include <retro/ctti.hpp>

struct MyBla : dyn<MyBla> {
	int x = 0;

	int keks[4] = {1, 2, 3, 4};

	auto begin() const { return std::begin(keks); }
	auto end() const { return std::end(keks); }

	//MyBla() { printf("new MyBla @ %p!\n", this); }
	//~MyBla() { printf("MyBla @ %p no more!\n", this); }

	virtual u32 get_api_id() const { return retro::bind::user_class<MyBla>::api_id; }
};

struct MySuperBla : dyn<MySuperBla, MyBla> {
	int q = 0;
	//MySuperBla() { printf("new MySuperBla @ %p!\n", this); }
	//~MySuperBla() { printf("MySuperBla @ %p no more!\n", this); }

	virtual u32 get_api_id() const { return retro::bind::user_class<MySuperBla>::api_id; }
};

namespace retro::bind {
	/*
	template<>
	struct type_descriptor<core::workspace> : user_class {
		inline static constexpr const char* name = "Workspace";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_property("numImages", [](core::workspace* ws) {
				std::lock_guard _g{ws->image_list_mtx};
				return ws->image_list.size();
			});
		}
	};*/

	template<>
	struct type_descriptor<MyBla> : user_class<MyBla> {
		inline static constexpr const char* name = "MyBla";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_method("add", [](MyBla* bla, int y) { return bla->x += y; });
			proto.add_method("sub", [](MyBla* bla, int y) { return bla->x -= y; });
			proto.add_static_method("getCount", [](MyBla* bla) { return bla->x; });
			proto.add_property(
				 "count", [](MyBla* bla) { return bla->x; }, [](MyBla* bla, int z) { bla->x = z; });
			proto.add_property("roCount", [](MyBla* bla) { return bla->x; });

			proto.add_async_method("asyncStuff", [](ref<MyBla> bla, int x) {
				std::this_thread::sleep_for(x * 1s);
				return x * 6;
			});
			proto.make_iterable([](MyBla* bla) {
				return std::vector<std::string>{"aa", "bb", "cc"};
				//return range::subrange(bla->begin(), bla->end());
			});
		}
	};

	template<>
	struct type_descriptor<MySuperBla> : user_class<MySuperBla> {
		inline static constexpr const char* name = "MySuperBla";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_property("xcount", [](MySuperBla* bla) { return bla->q; }, [](MySuperBla* bla, int z) { bla->q = z; });
			proto.template set_super<MyBla>();
		}
	};
};

NAPI_MODULE_INIT() {
	try {
		bind::js::engine ctx{env};
		bind::js::object mod{env, exports};
		bind::js::typedecl::init(env);

		mod.set("abcd", bind::js::create_function<false, false>(env, "abcd", [](int x, bind::js::function f) {
			return f.invoke(x).template as<int>() * x;
		}));
		mod.set("bla", 512.0);


		ctx.export_type<MyBla>(mod);
		ctx.export_type<MySuperBla>(mod);


		mod.set("blaInstance", make_rc<MyBla>());

		mod.freeze();

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