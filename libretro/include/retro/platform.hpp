#pragma once
#include <cstdint>
#include <thread>
#include <random>
#include <filesystem>
#include <retro/common.hpp>
#include <retro/intrin.hpp>

// Platform-dependant functions.
//
namespace retro::platform {
	// Gets a pseudo thread identifier in a fast way.
	//
#if RC_IA32
	RC_INLINE RC_CONST static size_t thread_id() {
	#if RC_64 && (RC_WINDOWS || RC_OSX)
		return intrin::ia32_rdgsbase();
	#else
		return intrin::ia32_rdfsbase();
	#endif
	}
#elif RC_ARM
	RC_INLINE RC_CONST static size_t thread_id() {
		return intrin::arm_mrs_tid();
	}
#else
	inline thread_local char __id;
	RC_INLINE RC_CONST static size_t thread_id() {
		return (size_t) &__id;
	}
#endif

	// Secure random generator.
	//
	RC_INLINE static u64 srng() {
#if RC_IA32
		return intrin::ia32_rdrand();
#else
		std::random_device dev{};
		u64                res = 0;
		for (int i = 0; i < (8 / sizeof(decltype(dev()))); i++)
			res |= u64(dev()) << (i * 8);
		return res;
#endif
	}

	// Gloabl affinity mask used by the worker threads.
	//
	inline u64 g_affinity_mask = 0xFFFFFFFFFFFFFFFF;

	// Applies the given affinity mask to the current thread.
	//
	void set_affinity(u64 mask);

	// Invoked to ensure ANSI escapes work.
	//
	void setup_ansi_escapes();

	// Page management.
	//
	enum prot_flags : u8 {
		prot_write = 1 << 0,
		prot_exec  = 1 << 1,
	};
	[[nodiscard]] void* mem_alloc(size_t page_count, u8 prot);
	[[nodiscard]] bool  mem_protect(void* pointer, size_t page_count, u8 prot);
	[[nodiscard]] bool  mem_free(void* pointer, size_t page_count);

	// Basic read/write.
	//
	bool				 write_file(const std::filesystem::path& path, std::span<const u8> data);
	bool				 read_file(const std::filesystem::path& path, std::span<const u8> data);
	std::vector<u8> read_file(const std::filesystem::path& path, bool& ok);

	// Executes a command and returns the result.
	//
	std::string exec(std::string cmd, bool merge_stderr = true);

	// Fetches an environment variable, returns empty string on failure.
	//
	std::string env(const char* name);

	// File mapping.
	//
	struct file_mapping {
		using iterator = const u8*;

		const void* base_address = nullptr;
		size_t		length		 = 0;
		iptr			handle		 = -1;
		iptr			reserved		 = 0;

		// Default constructed, no copy.
		//
		file_mapping()											= default;
		file_mapping(const file_mapping&)				= delete;
		file_mapping& operator=(const file_mapping&) = delete;

		// Trivially relocatable.
		//
		file_mapping(file_mapping&& o) noexcept { swap(o); }
		file_mapping& operator=(file_mapping&& o) noexcept {
			swap(o);
			return *this;
		}
		void swap(file_mapping& o) {
			using as_bytes = std::array<u8, sizeof(file_mapping)>;
			std::swap(*(as_bytes*) this, (as_bytes&) o);
		}

		// Implement a container.
		//
		const u8* data() const { return (const u8*) base_address; }
		iterator	 begin() const { return (iterator) base_address; }
		iterator	 end() const { return begin() + size(); }
		size_t	 size() const { return length; }
		bool		 empty() const { return length == 0; }
		const u8& operator[](size_t n) const { return begin()[n]; }

		// Validity check.
		//
		bool ok() const { return handle != -1; }
		explicit operator bool() const { return ok(); }

		// Unmaps the file.
		//
		void reset();
	};
	file_mapping map_file(const std::filesystem::path& path, size_t length = 0);
};