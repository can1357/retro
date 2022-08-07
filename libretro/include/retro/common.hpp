#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <initializer_list>
#include <array>
#include <bit>
#include <chrono>
#include <string>
#include <span>
#include <string_view>
#include <numeric>
#include <algorithm>
#include <retro/ranges.hpp>

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
#define RC_VA_ARG1(A0, A1, ...) A1
#define RC_VA_EMPTY(...)		  RC_VA_ARG1(__VA_OPT__(, ) 0, 1, )
#define RC_VA_OPT_SUPPORT		  !RC_VA_EMPTY

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
#define RC_CONCAT_I(x, y)		  x##y
#define RC_IDENTITY(...)		  __VA_ARGS__
#define RC_STRINGIFY(x)			  RC_STRINGIFY_I(x)
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
	#pragma clang diagnostic ignored "-Wambiguous-reversed-operator" // Z3
#elif RC_GNU
	#pragma GCC diagnostic ignored "-Wunneeded-internal-declaration"
	#pragma GCC diagnostic ignored "-Wunused-function"
	#pragma GCC diagnostic ignored "-Wunused-const-variable"
	#pragma GCC diagnostic ignored "-Winvalid-offsetof"
	#pragma GCC diagnostic ignored "-Wswitch"
	#pragma GCC diagnostic ignored "-Wtrigraphs"
	#pragma GCC diagnostic ignored "-Wambiguous-reversed-operator" // Z3
#endif

namespace retro {
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

	// Extended floating-point.
	// - TODO: Implement emulation for ARM.
	using f80  = long double;

	// Extended integers.
	// - TODO: Implement operations.
	struct i128 {
		i64 low;
		i64 high;
	};
	struct u128 {
		u64 low;
		u64 high;
	};

	// SIMD types.
	//
	using f64x8  = std::array<f64, 8>;	 // Bit-width: 512
	using f64x4  = std::array<f64, 4>;	 // Bit-width: 256
	using f64x2  = std::array<f64, 2>;	 // Bit-width: 128
	
	using i64x8  = std::array<i64, 8>;	 // Bit-width: 512
	using i64x4  = std::array<i64, 4>;	 // Bit-width: 256
	using i64x2  = std::array<i64, 2>;	 // Bit-width: 128

	using f32x16 = std::array<f32, 16>;	 // Bit-width: 512
	using f32x8  = std::array<f32, 8>;	 // Bit-width: 256
	using f32x4  = std::array<f32, 4>;	 // Bit-width: 128
	using f32x2  = std::array<f32, 2>;	 // Bit-width: 64

	using i32x16 = std::array<i32, 16>;	 // Bit-width: 512
	using i32x8  = std::array<i32, 8>;	 // Bit-width: 256
	using i32x4  = std::array<i32, 4>;	 // Bit-width: 128
	using i32x2  = std::array<i32, 2>;	 // Bit-width: 64

	using i16x32 = std::array<i16, 32>;	 // Bit-width: 512
	using i16x16 = std::array<i16, 16>;	 // Bit-width: 256
	using i16x8  = std::array<i16, 8>;	 // Bit-width: 128
	using i16x4  = std::array<i16, 4>;	 // Bit-width: 64

	using i8x64  = std::array<i8,  64>;	 // Bit-width: 512
	using i8x32  = std::array<i8,  32>;	 // Bit-width: 256
	using i8x16  = std::array<i8,  16>;	 // Bit-width: 128
	using i8x8   = std::array<i8,  8>;	 // Bit-width: 64

	// Helper to pin types.
	//
	struct pinned {
		constexpr pinned()					= default;
		pinned(const pinned&)				= delete;
		pinned& operator=(const pinned&) = delete;
		pinned(pinned&&)						= delete;
		pinned& operator=(pinned&&)		= delete;
	};

	// Specialization trait check.
	//
	template<template<typename...> typename Tmp, typename>
	static constexpr bool is_specialization_v = false;
	template<template<typename...> typename Tmp, typename... Tx>
	static constexpr bool is_specialization_v<Tmp, Tmp<Tx...>> = true;
	template<template<typename, size_t> typename Tmp, typename>
	static constexpr bool is_tn_specialization_v = false;
	template<template<typename, size_t> typename Tmp, typename T, size_t N>
	static constexpr bool is_tn_specialization_v<Tmp, Tmp<T, N>> = true;

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

	// Tags.
	//
	template<typename T>
	struct type_tag {
		using type = T;
	};
	template<auto V>
	struct value_tag {
		using type						 = decltype(V);
		static constexpr type value = V;
	};

	// Compiler specifics.
	//
#if RC_GNU
	#define RC_PURE		  __attribute__((pure))
	#define RC_CONST		  __attribute__((const))
	#define RC_FLATTEN	  __attribute__((flatten))
	#define RC_COLD		  __attribute__((cold, noinline, disable_tail_calls))
#if RC_CLANG
	#define RC_TRIVIAL_ABI __attribute__((trivial_abi))
	#define RC_INLINE		  __attribute__((always_inline))
#else
	#define RC_TRIVIAL_ABI
	#define RC_INLINE		  
#endif
	#define RC_NOINLINE	  __attribute__((noinline))
	#define RC_USED    	  __attribute__((used))
	#define RC_ALIGN(x)	  __attribute__((aligned(x)))
	#define RC_DBGBREAK	  __builtin_trap
#elif RC_MSVC
	#define RC_PURE
	#define RC_CONST
	#define RC_FLATTEN
	#define RC_USED
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
	RC_INLINE inline static constexpr u16 bswapw(uint16_t value) noexcept {
#if __has_builtin(__builtin_bswap16)
		if (!std::is_constant_evaluated())
			return __builtin_bswap16(value);
#endif
		return (u16((u16(value) & 0xFF) << 8) | u16((u16(value) & 0xFF00) >> 8));
	}
	RC_INLINE inline static constexpr u32 bswapd(uint32_t value) noexcept {
#if __has_builtin(__builtin_bswap32)
		if (!std::is_constant_evaluated())
			return __builtin_bswap32(value);
#endif
		return (u32(bswapw(u16((u32(value) << 16) >> 16))) << 16) | (u32(bswapw(u16((u32(value) >> 16)))));
	}
	RC_INLINE inline static constexpr u64 bswapq(uint64_t value) noexcept {
#if __has_builtin(__builtin_bswap64)
		if (!std::is_constant_evaluated())
			return __builtin_bswap64(value);
#endif
		return (u64(bswapd(u32((u64(value) << 32) >> 32))) << 32) | (u64(bswapd(u32((u64(value) >> 32)))));
	}

	template<typename T>
	RC_INLINE inline static constexpr T bswap(T value) noexcept {
		if constexpr (sizeof(T) == 8)
			return bit_cast<T>(bswapq(bit_cast<u64>(value)));
		else if constexpr (sizeof(T) == 4)
			return bit_cast<T>(bswapd(bit_cast<u32>(value)));
		else if constexpr (sizeof(T) == 2)
			return bit_cast<T>(bswapw(bit_cast<u16>(value)));
		else if constexpr (sizeof(T) == 1)
			return value;
		else
			static_assert(sizeof(T) == -1, "unexpected integer size");
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
	template<typename Dst, typename Src>
	RC_INLINE inline static constexpr Dst narrow_cast(Src o) {
#if RC_DEBUG
		if (!narrow_check<Dst, Src>(o)) [[unlikely]] {
			breakpoint();
		}
#endif
		return Dst(o);
	}

	// Helper for coercing void returns into another type.
	//
	template<typename R, typename F, typename... Tx>
	RC_INLINE inline static constexpr decltype(auto) apply_novoid(F&& fn, Tx&&... args) {
		using T = decltype(fn(std::forward<Tx>(args)...));
		if constexpr (std::is_void_v<T>) {
			fn(std::forward<Tx>(args)...);
			return R{};
		} else {
			return fn(std::forward<Tx>(args)...);
		}
	}

	// Tagging for structured types described via the TOML.
	//
	template<typename T>
	struct descriptor {
		using type = void;
	};
	template<typename T>
	using reflect = typename descriptor<T>::type;
	template<typename T>
	concept StructuredType = (!std::is_void_v<reflect<T>>);
	template<typename T>
	concept StructuredEnum = StructuredType<T> && std::is_enum_v<T>;

	// Reflection.
	//
	template<StructuredEnum T>
	inline constexpr const reflect<T>& enum_reflect(T value) {
		return reflect<T>::all()[uptr(value)];
	}
	template<StructuredEnum T>
	inline constexpr std::string_view enum_name(T value) {
		return enum_reflect(value).name;
	}

	// Python generated std::visit for enums.
	//
#define _RC_DEFINE_STD_VISIT_CASE_FOR(V, ...) \
	case _Type::V:                             \
		return vis(retro::value_tag<_Type::V>{});
#define RC_DEFINE_STD_VISITOR_FOR(_TYPE, _MACRO_VISITOR)                   \
	namespace std {                                                                   \
		template<typename Visitor>                                                     \
		RC_INLINE inline constexpr decltype(auto) visit(Visitor&& vis, _TYPE opcode) { \
			using _Type = _TYPE;                                                        \
			switch (opcode) {                                                           \
				_MACRO_VISITOR(_RC_DEFINE_STD_VISIT_CASE_FOR)                            \
				_RC_DEFINE_STD_VISIT_CASE_FOR(none)                                      \
				default:                                                                 \
					retro::assume_unreachable();                                          \
			}                                                                           \
			retro::assume_unreachable();                                                \
		}                                                                              \
	};

	// Small array type for TOML generated lists.
	//
	template<typename T>
	struct small_array {
		T	data[24] = {};
		u8 length	= 0;
		constexpr small_array(std::initializer_list<T> init) : length((u8)init.size()) {
			range::copy(init, data);
		}
		constexpr const T* begin() const { return &data[0]; }
		constexpr const T* end() const { return &data[length]; }
		constexpr size_t size() const { return length; }
		constexpr const T& operator[](size_t n) const { return data[n]; }
	};

	// Initialization helpers.
	//
	template<typename T>
	inline T static_instance = {};

#if RC_GNU || RC_CLANG
	#define RC_INITIALIZER __attribute__((constructor)) static void RC_CONCAT(__initializer, __COUNTER__)()
#else
	namespace detail {
		struct init_hook {
			template<typename F>
			init_hook(F&& f) {
				f();
			}
		};
	};
	#define RC_INITIALIZER RC_USED static retro::detail::init_hook RC_CONCAT(__initializer, __COUNTER__) = []()
#endif

	// Include chrono.
	//
	namespace chrono = std::chrono;
	using namespace std::literals::chrono_literals;
	using duration	 = typename chrono::high_resolution_clock::duration;
	using timestamp = typename chrono::high_resolution_clock::time_point;
	RC_INLINE inline static timestamp now() { return chrono::high_resolution_clock::now(); }

	// Useful literals.
	//
	inline constexpr unsigned long long operator""_kb(unsigned long long n) { return (unsigned long long) (n * 1024ull); }
	inline constexpr unsigned long long operator""_mb(unsigned long long n) { return (unsigned long long) (n * 1024ull * 1024ull); }
	inline constexpr unsigned long long operator""_gb(unsigned long long n) { return (unsigned long long) (n * 1024ull * 1024ull * 1024ull); }
};

// String conversion.
//
namespace std {
	template<retro::StructuredEnum T>
	inline std::string to_string(T value) {
		return std::string{retro::enum_name<T>(value)};
	}
};