#if RC_UNIX
	#ifndef _GNU_SOURCE
		#define _GNU_SOURCE
	#endif
	#include <sched.h>
   #include <pthread.h>
#endif

#include <array>
#include <filesystem>
#include <fstream>
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
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <unistd.h>
#endif

#include <random>
namespace retro::platform {
	// Simple functions, not specialized by any platform for now.
	//
	bool write_file(const std::filesystem::path& path, std::span<const u8> data) {
		std::ofstream file(path, std::ios::binary);
		if (!file.good())
			return false;

		// Write the data and return.
		//
		file.write((char*) data.data(), data.size());
		return true;
	}
	bool read_file(const std::filesystem::path& path, std::span<const u8> data) {
		std::ifstream file(path, std::ios::binary);
		if (!file.good())
			return false;

		// Determine file size.
		//
		file.seekg(0, std::ios_base::end);
		size_t file_size = file.tellg();
		file.seekg(0, std::ios_base::beg);

		// Fail if requesting more bytes than there is.
		//
		if (data.size() > file_size) {
			return false;
		}

		file.read((char*) data.data(), data.size());
		return true;
	}
	std::vector<u8> read_file(const std::filesystem::path& path, bool& ok) {
		ok = false;
		std::ifstream file(path, std::ios::binary);
		if (!file.good())
			return {};

		// Determine file size.
		//
		file.seekg(0, std::ios_base::end);
		size_t file_size = file.tellg();
		file.seekg(0, std::ios_base::beg);

		// Read and return.
		//
		std::vector<u8> buffer(file_size);
		file.read((char*) buffer.data(), buffer.size());
		ok = true;
		return buffer;
	}

	// Windows compatible exec implementation.
	//
#if RC_WINDOWS
	#define popen	_popen
	#define pclose _pclose
#endif
	std::string exec(std::string cmd) {
		std::array<char, 128> buffer;
		std::string				 result;
		FILE*						 f = popen(cmd.c_str(), "r");
		while (fgets(buffer.data(), (int) buffer.size(), f) != nullptr) {
			result += buffer.data();
		}
		pclose(f);
		return result;
	}

	// Fetches an environment variable, returns empty string on failure.
	//
	std::string env(const char* name) {
#if RC_WINDOWS
		size_t cnt;
		char*	 buffer = nullptr;
		if (!_dupenv_s(&buffer, &cnt, name) && buffer) {
			std::string res = {buffer, cnt - 1};
			free(buffer);
			return res;
		}
#else
		if (auto* s = ::getenv(name))
			return s;
#endif
		return {};
	}

	// API linux and osx implement differently.
	//
#if RC_WINDOWS
	void set_affinity(u64 mask) { SetThreadAffinityMask(HANDLE(-2), mask); }
#elif RC_UNIX
	void set_affinity(u64 mask) {
		cpu_set_t set;
		CPU_ZERO(&set);
		for (int i = 0; i != 64; i++) {
			if (mask & (1ull << i)) {
				CPU_SET(i, &set);
			}
		}
		sched_setaffinity(0, sizeof(cpu_set_t), &set);
	}
#elif RC_OSX
	// TODO:
	void set_affinity(u64 mask) { RC_UNUSED(mask); }
#else
	void set_affinity(u64 mask) { RC_UNUSED(mask); }
#endif


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

	file_mapping map_file(const std::filesystem::path& path, size_t length_req) {
		file_mapping result			 = {};
		auto&			 file_handle	 = (void*&) result.handle;
		auto&			 mapping_handle = (void*&) result.reserved;

		// Determine file path if not specified.
		//
		if (length_req == 0) {
			std::error_code ec;
			result.length = std::filesystem::file_size(path, ec);
			if (ec)
				return {};
		} else {
			result.length = length_req;
		}

		// Create the file.
		//
		file_handle = CreateFileW(path.native().c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr);
		if (!file_handle || file_handle == INVALID_HANDLE_VALUE)
			return {};

		// Map the file if its not empty:
		//
		if (result.length != 0) {
			mapping_handle = CreateFileMappingFromApp(file_handle, nullptr, PAGE_READONLY, 0, nullptr);
			if (!mapping_handle || mapping_handle == INVALID_HANDLE_VALUE) {
				return {};
			}
			result.base_address = MapViewOfFileFromApp(mapping_handle, SECTION_MAP_READ, 0, result.length);
			if (!result.base_address) {
				return {};
			}
		}
		return result;
	}
	void file_mapping::reset() {
		if (handle != -1) {
			auto& file_handle		= (void*&) handle;
			auto& mapping_handle = (void*&) reserved;
			if (mapping_handle && mapping_handle != INVALID_HANDLE_VALUE) {
				if (base_address) {
					UnmapViewOfFile(std::exchange(base_address, nullptr));
					length = 0;
				}
				CloseHandle(std::exchange(mapping_handle, nullptr));
			}
			CloseHandle(std::exchange(file_handle, nullptr));
		}
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

	file_mapping map_file(const std::filesystem::path& path, size_t length_req) {
		file_mapping result = {};
		auto&			 fd	  = (int&) result.handle;

		// Determine file path if not specified.
		//
		if (length_req == 0) {
			std::error_code ec;
			result.length = std::filesystem::file_size(path, ec);
			if (ec)
				return {};
		} else {
			result.length = length_req;
		}

		// Create the file.
		//
		fd = open(path.string().c_str(), 0 /*O_RDONLY*/);
		if (fd == -1)
			return {};

		// Map the file if its not empty:
		//
		if (result.length != 0) {
			result.base_address = mmap(nullptr, result.length, 1 /*PROT_READ*/, 1 /*MAP_SHARED*/, fd, 0);
			if (!result.base_address) {
				return {};
			}
		}
		return result;
	}
	void file_mapping::reset() {
		auto& fd = (int&) handle;
		if (fd != -1) {
			if (base_address) {
				munmap((void*) std::exchange(base_address, nullptr), std::exchange(length, 0));
			}
			close(std::exchange(fd, -1));
		}
	}
#endif
};
