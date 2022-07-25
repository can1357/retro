#pragma once
#include <retro/common.hpp>

#if RC_MSVC
	#include <intrin.h>
#elif RC_ARM
	#include <arm_acle.h> // TODO: Fix for MSVC/AArch64.
#endif

// Arch-dependant functions.
//
namespace retro::arch {
	// IA32 details.
	//
#if RC_IA32
	// RDGSBASE/RDFSBASE intrinsics.
	//
	RC_INLINE RC_CONST static size_t ia32_rdgsbase() {
	#if RC_64
		#if RC_MSVC
		return _readgsbase_u64();
		#else
		u64 value;
		asm("rdgsbase %0" : "=r"(value)::);
		return value;
		#endif
	#else
		#if RC_MSVC
		return _readgsbase_u32();
		#else
		u32 value;
		asm("rdgsbase %0" : "=r"(value)::);
		return value;
		#endif
	#endif
	}
	RC_INLINE RC_CONST static size_t ia32_rdfsbase() {
	#if RC_64
		#if RC_MSVC
		return _readfsbase_u64();
		#else
		u64 value;
		asm("rdfsbase %0" : "=r"(value)::);
		return value;
		#endif
	#else
		#if RC_MSVC
		return _readfsbase_u32();
		#else
		u32 value;
		asm("rdfsbase %0" : "=r"(value)::);
		return value;
		#endif
	#endif
	}

	// RDRAND/RDSEED intrinsics.
	//
	RC_INLINE static u64 ia32_rdrand() {
		u64 tmp;
	#if RC_CLANG || RC_GNU
		asm("0: rdrand %0; jae 0b;" : "=r"(tmp));
	#else
		while (!_rdrand64_step(&tmp));
	#endif
		return tmp;
	}
	RC_INLINE static u64 ia32_rdseed() {
		u64 tmp;
	#if RC_CLANG || RC_GNU
		asm("0: rdseed %0; jae 0b;" : "=r"(tmp));
	#else
		while (!_rdseed64_step(&tmp));
	#endif
		return tmp;
	}

	// PAUSE intrinsic.
	//
	RC_INLINE static void ia32_pause() {
	#if RC_CLANG || RC_GNU
		asm volatile("pause" ::: "memory");
	#else
		_mm_pause();
	#endif
	}

	// CRC32C intrinsics.
	//
	#if RC_CRC32C
	template<typename T>
	RC_INLINE static u32 ia32_crc32c(u32 chk, const T& value) {
		#if RC_MSVC
		if constexpr (sizeof(T) == 1) {
			return (u32) _mm_crc32_u8(chk, retro::bit_cast<u8>(value));
		} else if constexpr (sizeof(T) == 2) {
			return (u32) _mm_crc32_u16(chk, retro::bit_cast<u16>(value));
		} else if constexpr (sizeof(T) == 4) {
			return (u32) _mm_crc32_u32(chk, retro::bit_cast<u32>(value));
		} else {
			return (u32) _mm_crc32_u64(chk, retro::bit_cast<u64>(value));
		}
		#else
		if constexpr (sizeof(T) == 1) {
			return (u32) __builtin_ia32_crc32qi(chk, retro::bit_cast<u8>(value));
		} else if constexpr (sizeof(T) == 2) {
			return (u32) __builtin_ia32_crc32hi(chk, retro::bit_cast<u16>(value));
		} else if constexpr (sizeof(T) == 4) {
			return (u32) __builtin_ia32_crc32si(chk, retro::bit_cast<u32>(value));
		} else {
			return (u32) __builtin_ia32_crc32di(chk, retro::bit_cast<u64>(value));
		}
		#endif
	}
	#endif
#endif

	// ARM details.
	//
#if RC_ARM
	// YIELD intrinsic.
	//
	RC_INLINE static void arm_yield() {
		asm volatile("yield" ::: "memory");
	}

	// MRS for thread id.
	//
	RC_INLINE RC_CONST static size_t arm_mrs_tid() {
		size_t tid;
	#if RC_OSX
		asm("mrs %0, tpidrro_el0" : "=r"(tid));
	#else
		asm("mrs %0, tpidr_el0" : "=r"(tid));
	#endif
		return tid;
	}

	// CRC32C intrinsics.
	//
	#if RC_CRC32C
	template<typename T>
	RC_INLINE static u32 arm_crc32c(u32 chk, const T& value) {
		if constexpr (sizeof(T) == 1) {
			return (u32) __crc32cb(chk, retro::bit_cast<u8>(value));
		} else if constexpr (sizeof(T) == 2) {
			return (u32) __crc32ch(chk, retro::bit_cast<u16>(value));
		} else if constexpr (sizeof(T) == 4) {
			return (u32) __crc32cw(chk, retro::bit_cast<u32>(value));
		} else {
			return (u32) __crc32cd(chk, retro::bit_cast<u64>(value));
		}
	}
	#endif
#endif

	// Reads the cycle counter.
	//
	RC_INLINE static u64 cycle_counter() {
		u64 val = 0;
#if __has_builtin(__builtin_readcyclecounter)
		val =__builtin_readcyclecounter();
#elif RC_IA32 && RC_GNU
		u32 low, high;
		asm("rdtsc" : "=a"(low), "=d"(high));
		val = low | (u64(high) << 32);
#elif RC_IA32 && RC_MSVC
		val = __rdtsc();
#elif RC_ARM
		asm("mrs %0, cntvct_el0" : "=r"(val));
#elif RC_PPC
		asm("mfspr %%r3, 268" : "=r"(val));
#endif
		return val;
	}

	// Appends the given value to the CRC32C checksum being computed.
	//
	#if RC_CRC32C
	template<typename T>
		requires(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8)
	RC_INLINE static u32 crc32c(u32 chk, const T& value) {
		#if RC_IA32
		return ia32_crc32c(chk, value);
		#else
		return arm_crc32c(chk, value);
		#endif
	}
	#endif

	// Yields the processor to allow sibling threads to execute.
	//
	RC_INLINE static void yield() {
#if RC_IA32
		ia32_pause();
#elif RC_ARM
		arm_yield();
#endif
	}
};
