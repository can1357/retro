#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <initializer_list>
#include <array>
#include <bit>
#include <algorithm>
#include <string_view>
#include <numeric>
#include <ranges>

// Compiler details.
//
#ifndef _CONSTEVAL
	#if defined(__cpp_consteval)
		#define _CONSTEVAL consteval
	#else	 // ^^^ supports consteval / no consteval vvv
		#define _CONSTEVAL constexpr
	#endif
#endif
#ifndef __has_builtin
	#define __has_builtin(...) 0
#endif
#ifndef __has_attribute
	#define __has_attribute(...) 0
#endif
#ifndef __has_cpp_attribute
	#define __has_cpp_attribute(...) 0
#endif
#ifndef __has_feature
	#define __has_feature(...) 0
#endif
#ifndef __has_include
	#define __has_include(...) 0
#endif

// Detect architecture.
//
#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(__AMD_64) || defined(_M_AMD64) || defined(_M_IX86) || defined(__i386)
	#define RC_IA32		1
	#define RC_ARCH_NAME "IA32"
#elif defined(__aarch64__) || defined(__arm__) || defined(_M_ARM64)
	#define RC_ARM			1
	#define RC_ARCH_NAME "ARM"
#elif defined(__powerpc__) || defined(__powerpc64__)
	#define RC_PPC			1
	#define RC_ARCH_NAME "PowerPC"
#elif defined(__EMSCRIPTEN__)
	#define RC_WASM		1
	#define RC_ARCH_NAME "Emscripten"
#else
	#error "Unknown target architecture."
#endif

#if UINTPTR_MAX == 0xFFFFFFFF
	#define RC_32 1
	#define RC_64 0
#elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF
	#define RC_32 0
	#define RC_64 1
#else
	#error "Target architecture not supported."
#endif

// Detect architecture capabilities.
//
#if RC_IA32
	#if defined(__AVX2__)
		#define RC_BMI		1
		#define RC_AVX2	1
		#define RC_AVX		1
		#define RC_SSE42	1
		#define RC_CRC32C 1
	#elif defined(__AVX__)
		#define RC_AVX		1
		#define RC_SSE42	1
		#define RC_CRC32C 1
	#elif defined(__SSE4_2__)
		#define RC_SSE42	1
		#define RC_CRC32C 1
	#endif
#elif RC_ARM
	#if defined(__ARM_FEATURE_CRC32)
		#define RC_CRC32C 1
	#endif
#endif

// Define page-frame-number helpers.
// - ARM, IA32, POWERPC all have smallest page size of 4KB.
//
#define RC_PAGE_SHIFT  12
#define RC_FROM_PFN(x) (x << RC_PAGE_SHIFT)
#define RC_TO_PFN(x)	  (x >> RC_PAGE_SHIFT)

// Detect platform.
//
#if defined(_WIN64) || defined(_WIN32)
	#define RC_WINDOWS		 1
	#define RC_PLATFORM_NAME "Windows"
#elif defined(__APPLE__) || defined(__MACH__)
	#define RC_OSX				 1
	#define RC_PLATFORM_NAME "OS X"
#elif defined(__EMSCRIPTEN__)
	#define RC_EMSCRIPTEN	 1
	#define RC_PLATFORM_NAME "Emscripten"
#elif defined(__linux__) || defined(__unix__) || defined(__unix)
	#define RC_UNIX			 1
	#define RC_PLATFORM_NAME "Linux"
#else
	#error "Unknown target OS."
#endif

// Detect compiler.
//
#if defined(__clang__)
	#define RC_GNU				 1
	#define RC_CLANG			 1
	#define RC_MSVC			 0
	#define RC_COMPILER_NAME "Clang"
#elif defined(__GNUC__) || defined(__CYGWIN__)
	#define RC_GNU				 1
	#define RC_CLANG			 0
	#define RC_MSVC			 0
	#define RC_COMPILER_NAME "GCC"
#elif defined(_MSC_VER)
	#define RC_GNU				 0
	#define RC_CLANG			 0
	#define RC_MSVC			 1
	#define RC_COMPILER_NAME "MSVC"
#else
	#error "Unknown compiler."
#endif
#ifdef _MSC_VER
	#define RC_MS_EXTS 1
#endif

// Add CRT traits based on the platform.
//
#ifndef RC_MALLOC_ALIGN
	#if RC_WINDOWS && RC_64
		#define RC_MALLOC_ALIGN 0x10
	#else
		#define RC_MALLOC_ALIGN alignof(std::max_align_t)
	#endif
#endif
#ifndef RC_DEBUG
	#if defined(NDEBUG) && !defined(_DEBUG)
		#define RC_DEBUG 0
	#else
		#define RC_DEBUG 1
	#endif
#endif

// Common macros.
//
#define RC_STRINGIFY_I(x)		  #x
#define RC_STRCAT_I(x, y)		  x##y
#define RC_CONCAT_I(x, y)		  x#y
#define RC_IDENTITY(...)		  __VA_ARGS__
#define RC_STRINGIFY(x)			  RC_STRINGIFY_I(x)
#define RC_STRCAT(x, y)			  RC_STRCAT_I(x, y)
#define RC_CONCAT(x, y)			  RC_CONCAT_I(x, y)
#define RC_FIRST(x, ...)		  RC_IDENTITY(x)
#define RC_SECOND(_, x, ...)	  RC_IDENTITY(x)
#define RC_THIRD(_, __, x, ...) RC_IDENTITY(x)
#define RC_UNUSED(...)			  (void) (__VA_ARGS__)
#define RC_NOOP(...)

// Warning config.
//
#if RC_CLANG
	#pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
	#pragma clang diagnostic ignored "-Wunused-function"
	#pragma clang diagnostic ignored "-Wunused-const-variable"
	#pragma clang diagnostic ignored "-Winvalid-offsetof"
	#pragma clang diagnostic ignored "-Wswitch"
	#pragma clang diagnostic ignored "-Wtrigraphs"
#elif RC_GNU
	#pragma GCC diagnostic ignored "-Wunneeded-internal-declaration"
	#pragma GCC diagnostic ignored "-Wunused-function"
	#pragma GCC diagnostic ignored "-Wunused-const-variable"
	#pragma GCC diagnostic ignored "-Winvalid-offsetof"
	#pragma GCC diagnostic ignored "-Wswitch"
	#pragma GCC diagnostic ignored "-Wtrigraphs"
#endif

namespace retro {
	namespace view	 = std::views;
	namespace range = std::ranges;

	// Short names for builtin types.
	//
	using u8	  = uint8_t;
	using u16  = uint16_t;
	using u32  = uint32_t;
	using u64  = uint64_t;
	using i8	  = int8_t;
	using i16  = int16_t;
	using i32  = int32_t;
	using i64  = int64_t;
	using iptr = intptr_t;
	using uptr = uintptr_t;
	using f32  = float;
	using f64  = double;

	// Specialization trait check.
	//
	template<template<typename...> typename Tmp, typename>
	static constexpr bool is_specialization_v = false;
	template<template<typename...> typename Tmp, typename... Tx>
	static constexpr bool is_specialization_v<Tmp, Tmp<Tx...>> = true;

	// String literal.
	//
	template<size_t N>
	struct string_literal {
		char value[N]{};
		constexpr string_literal(const char (&str)[N]) { std::copy_n(str, N, value); }

		// Observers.
		//
		constexpr const char*		data() const { return &value[0]; }
		constexpr const char*		c_str() const { return data(); }
		constexpr std::string_view to_string() const { return {data(), size()}; }
		constexpr size_t				size() const { return N - 1; }
		constexpr size_t				length() const { return size(); }
		constexpr bool					empty() const { return size() == 0; }
		constexpr const char&		operator[](size_t n) const { return data()[n]; }
		constexpr auto					begin() const { return to_string().begin(); }
		constexpr auto					end() const { return to_string().end(); }

		// Decay to string view.
		//
		constexpr operator std::string_view() const { return to_string(); }
	};

	// Compiler specifics.
	//
#if RC_GNU
	#define RC_PURE		  __attribute__((pure))
	#define RC_CONST		  __attribute__((const))
	#define RC_FLATTEN	  __attribute__((flatten))
	#define RC_COLD		  __attribute__((cold, noinline, disable_tail_calls))
	#define RC_INLINE		  __attribute__((always_inline))
	#define RC_NOINLINE	  __attribute__((noinline))
	#define RC_ALIGN(x)	  __attribute__((aligned(x)))
	#define RC_TRIVIAL_ABI __attribute__((trivial_abi))
	#define RC_DBGBREAK	  __builtin_trap
#elif RC_MSVC
	#define RC_PURE
	#define RC_CONST
	#define RC_FLATTEN
	#define RC_INLINE	  [[msvc::forceinline]]
	#define RC_NOINLINE __declspec(noinline)
	#define RC_COLD	  RC_NOINLINE
	#define RC_ALIGN(x) __declspec(align(x))
	#define RC_TRIVIAL_ABI
	#define RC_DBGBREAK __debugbreak
#endif

	RC_INLINE inline static void breakpoint() {
#if __has_builtin(__builtin_debugtrap)
		__builtin_debugtrap();
#elif RC_MSVC
		__debugbreak();
#endif
	}
	RC_INLINE inline static constexpr void assume_that(bool condition) {
#if __has_builtin(__builtin_assume)
		__builtin_assume(condition);
#elif RC_MSVC
		__assume(condition);
#endif
	}
	RC_INLINE inline static void assume_unreachable [[noreturn]] () {
#if __has_builtin(__builtin_unreachable)
		__builtin_unreachable();
#else
		assume_that(false);
#endif
	}

	// Useful bit operations.
	//
	RC_INLINE inline static constexpr u64 bit_mask(i32 num_bits, i32 offset = 0) {
		u64 base = num_bits ? ~0ull : 0;
		return (base >> (64 - num_bits)) << offset;
	}
	template<typename T>
	RC_INLINE inline static constexpr T bswap(T value) {
#if RC_GNU || RC_CLANG
		if constexpr (sizeof(T) == 1) {
			return value;
		} else if constexpr (sizeof(T) == 2) {
	#if __has_builtin(__builtin_bswap16)
			if (!std::is_constant_evaluated())
				return __builtin_bswap16(u16(value));
	#endif
			return T(u16((u16(value) & 0xFF) << 8) | u16((u16(value) & 0xFF00) >> 8));
		} else if constexpr (sizeof(T) == 4) {
	#if __has_builtin(__builtin_bswap32)
			if (!std::is_constant_evaluated())
				return __builtin_bswap32(u32(value));
	#endif
			return T(u32(bswap(u16((u32(value) << 16) >> 16))) << 16) | (u32(bswap(u16((u32(value) >> 16)))));
		} else if constexpr (sizeof(T) == 8) {
	#if __has_builtin(__builtin_bswap64)
			if (!std::is_constant_evaluated())
				return __builtin_bswap64(value);
	#endif
			return T(u64(bswap(u32((u64(value) << 32) >> 32))) << 32) | (u64(bswap(u32((u64(value) >> 32)))));
		} else {
			static_assert(sizeof(T) == -1, "unexpected integer size");
		}
#else
		return std::byteswap<T>(value);
#endif
	}
	template<typename To, typename From>
	RC_INLINE inline static constexpr To bit_cast(const From& x) noexcept {
		return __builtin_bit_cast(To, x);
	}
	template<typename T>
	RC_INLINE inline static constexpr T align_up(T value, size_t boundary) {
#if __has_builtin(__builtin_align_up)
		if constexpr (std::is_pointer_v<T>) {
			return (T) __builtin_align_up(value, boundary);
		}
#endif
		size_t mask = boundary - 1;
		return T((uptr(value) + mask) & ~mask);
	}
	template<typename T>
	RC_INLINE inline static constexpr T align_down(T value, size_t boundary) {
#if __has_builtin(__builtin_align_down)
		if constexpr (std::is_pointer_v<T>) {
			return (T) __builtin_align_down(value, boundary);
		}
#endif
		size_t mask = boundary - 1;
		return T(uptr(value) & ~mask);
	}
	template<typename T>
	RC_INLINE inline static constexpr bool is_aligned(T value, size_t boundary) {
#if __has_builtin(__builtin_is_aligned)
		if constexpr (std::is_pointer_v<T>) {
			return __builtin_is_aligned(value, boundary);
		}
#endif
		size_t mask = boundary - 1;
		return (uptr(value) & mask) == 0;
	}

	// Overflow checks.
	//
	template<typename T>
	RC_INLINE inline static constexpr bool mul_check(T x, T y) {
		if constexpr (std::is_floating_point_v<T>) {
			return true;
		} else {
			// Based duck.
			return ((T) ((std::make_unsigned_t<T>) x * (std::make_unsigned_t<T>) y) / y) == x;
		}
	}
	template<typename Dst, typename Src>
	RC_INLINE inline static constexpr bool narrow_check(Src o) {
		if constexpr (std::is_floating_point_v<Dst>) {
			return true;
		}
		// Src signed == Dst signed:
		//
		else if constexpr (std::is_signed_v<Dst> == std::is_signed_v<Src>) {
			return ((Dst) o) == o;
		}
		// Src signed, Dst unsigned:
		//
		else if constexpr (std::is_signed_v<Src>) {
			return o >= 0 && narrow_check<Dst>((std::make_unsigned_t<Src>) o);
		}
		// Dst signed, Src unsigned:
		//
		else {
			return o <= ((u64) std::numeric_limits<Dst>::max());
		}
	}
};