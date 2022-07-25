#include <array>
#include <retro/format.hpp>
#include <retro/platform.hpp>

#if RC_WINDOWS
	#define NOMINMAX
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
	#pragma comment(lib, "ntdll.lib")

extern "C" {
__declspec(dllimport) int32_t
	 __stdcall NtAllocateVirtualMemory(HANDLE ProcessHandle, PVOID* BaseAddress, ULONG_PTR ZeroBits, PSIZE_T RegionSize, ULONG AllocationType, ULONG Protect);
__declspec(dllimport) int32_t __stdcall NtProtectVirtualMemory(HANDLE ProcessHandle, PVOID* BaseAddress, PSIZE_T RegionSize, ULONG NewProtect, PULONG OldProtect);
__declspec(dllimport) int32_t __stdcall NtFreeVirtualMemory(HANDLE ProcessHandle, PVOID* BaseAddress, PSIZE_T RegionSize, ULONG FreeType);
};

#else
	#include <sys/mman.h>
#endif

#include <random>
namespace retro::platform {
#if RC_WINDOWS
	static constexpr ULONG protection_map[] = {PAGE_READONLY, PAGE_READWRITE, PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE};

	[[nodiscard]] void* mem_alloc(size_t page_count, u8 prot) {
		void*	 base = nullptr;
		SIZE_T size = RC_FROM_PFN(page_count);
		NtAllocateVirtualMemory(HANDLE(-1), &base, 0, &size, MEM_COMMIT | MEM_RESERVE, protection_map[prot]);
		return base;
	}
	[[nodiscard]] bool mem_protect(void* pointer, size_t page_count, u8 prot) {
		ULONG	 old_prot	 = 0;
		SIZE_T region_size = RC_FROM_PFN(page_count);
		return NtProtectVirtualMemory(HANDLE(-1), &pointer, &region_size, protection_map[prot], &old_prot) >= 0;
	}
	[[nodiscard]] bool mem_free(void* pointer, size_t page_count) {
		RC_UNUSED(page_count);
		SIZE_T region_size = 0;
		return NtFreeVirtualMemory(HANDLE(-1), &pointer, &region_size, MEM_RELEASE) >= 0;
	}

	void setup_ansi_escapes() {
	#if !RC_NO_ANSI
		auto	console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD mode				= 0;
		GetConsoleMode(console_handle, &mode);
		mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(console_handle, mode);
	#endif
		SetConsoleOutputCP(CP_UTF8);
	}
#else
	static constexpr int protection_map[] = {PROT_READ, PROT_READ | PROT_WRITE, PROT_READ | PROT_EXEC, PROT_READ | PROT_WRITE | PROT_EXEC};

	[[nodiscard]] void* mem_alloc(size_t page_count, u8 prot) { return mmap(0, RC_FROM_PFN(page_count), protection_map[prot], MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); }
	[[nodiscard]] bool  mem_protect(void* pointer, size_t page_count, u8 prot) { return mprotect(pointer, RC_FROM_PFN(page_count), protection_map[prot]) >= 0; }
	[[nodiscard]] bool  mem_free(void* pointer, size_t page_count) { return munmap(pointer, RC_FROM_PFN(page_count)) >= 0; }

	void setup_ansi_escapes() {}
#endif
};
