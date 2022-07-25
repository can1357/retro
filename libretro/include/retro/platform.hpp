#pragma once
#include <cstdint>
#include <thread>
#include <random>
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
};