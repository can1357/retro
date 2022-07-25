#pragma once
#include <retro/common.hpp>

// Compile-Time type identifiers.
//
namespace retro::ctti {
	// Type flags.
	//
	enum type_id_flags : u64 {
		flag_none       = 0,
		flag_const      = 1 << 0,
		flag_volatile   = 1 << 1,
		flag_lvalue_ref = 1 << 2,
		flag_rvalue_lef = 1 << 3,
		flag_pointer    = 1 << 4,
		flag_builtin    = 1 << 5,
		flag_modifiers  = (flag_builtin << 1) - 1
	};

	// Define the type hash.
	//
	template<typename T>
	struct type_id {
		// Note: Do not mix compilers between RT/CC or the final binary.
		// Exact value might differ and might cause hash values to be different than expected.
		//
		static constexpr auto name() {
			// -> clang: .asciz  "static auto retro::ctti::type_id<mystruct>::name() [T = mystruct]"
			// -> gcc:   .string "static constexpr auto retro::ctti::type_id<T>::name() [with T = mystruct]"
			// -> msvc:  DB      "retro::ctti::type_id<struct mystruct>::name"
#if RC_MSVC
			std::string_view sig = __FUNCTION__;
			sig.remove_suffix(sizeof(">::name")-1);
#else
			std::string_view sig = __PRETTY_FUNCTION__;
			sig.remove_suffix(1);
#endif

			#if RC_MSVC
			for (size_t i = sig.size() - 2;; i--) {
				if (sig.substr(i).starts_with("type_id<")) {
					sig = sig.substr(i + sizeof("type_id<") - 1);

					// Intentionally not -1'd to remove the space as well.
					if (sig.starts_with("enum"))
						sig.remove_prefix(sizeof("enum"));
					else if (sig.starts_with("struct"))
						sig.remove_prefix(sizeof("struct"));
					else if (sig.starts_with("class"))
						sig.remove_prefix(sizeof("class"));
					return sig;
				}
			}
			#else
			for (size_t i = sig.size() - 5;; i--) {
				if (sig.substr(i).starts_with("T = ")) {
					i += 4;
					return sig.substr(i, sig.size() - i);
				}
			}
			#endif
			assume_unreachable();
		}
		static constexpr auto get() {
			u64 tmp = 0xcbf29ce484222325;
			for (char c : name()) {
				tmp ^= (u8) c;
				tmp *= 0x00000100000001B3;
			}
			return tmp & ~flag_modifiers;
		}
		static constexpr u64 value = get();
	};

	// Implement specializations for modifiers and builtin types.
	//
	// clang-format off
	template<typename T> struct type_id<T*> { static constexpr u64 value = type_id<T>::value | flag_pointer; };
	template<typename T> struct type_id<T&> { static constexpr u64 value = type_id<T>::value | flag_lvalue_ref; };
	template<typename T> struct type_id<T&&> { static constexpr u64 value = type_id<T>::value | flag_rvalue_lef; };
	template<typename T> struct type_id<T[]> { static constexpr u64 value = type_id<T>::value | flag_pointer; };
	template<typename T> struct type_id<const T> { static constexpr u64 value = type_id<T>::value | flag_const; };
	template<typename T> struct type_id<volatile T> { static constexpr u64 value = type_id<T>::value | flag_volatile; };
	template<typename T, size_t N> struct type_id<T[N]> { static constexpr u64 value = type_id<T>::value | flag_pointer; };
	template<> struct type_id<void> { static constexpr u64 value = 0; };
	template<> struct type_id<bool> { static constexpr u64 value = flag_builtin + 0; };
	template<> struct type_id<i8> { static constexpr u64 value = flag_builtin + 1; };
	template<> struct type_id<i16> { static constexpr u64 value = flag_builtin + 2; };
	template<> struct type_id<i32> { static constexpr u64 value = flag_builtin + 3; };
	template<> struct type_id<i64> { static constexpr u64 value = flag_builtin + 4; };
	template<> struct type_id<wchar_t> { static constexpr u64 value = flag_builtin + 5; };
	template<> struct type_id<char8_t> { static constexpr u64 value = flag_builtin + 6; };
	template<> struct type_id<char16_t> { static constexpr u64 value = flag_builtin + 7; };
	template<> struct type_id<char32_t> { static constexpr u64 value = flag_builtin + 8; };
	template<> struct type_id<u8> { static constexpr u64 value = flag_builtin + 9; };
	template<> struct type_id<u16> { static constexpr u64 value = flag_builtin + 10; };
	template<> struct type_id<u32> { static constexpr u64 value = flag_builtin + 11; };
	template<> struct type_id<u64> { static constexpr u64 value = flag_builtin + 12; };
	template<> struct type_id<f32> { static constexpr u64 value = flag_builtin + 13; };
	template<> struct type_id<f64> { static constexpr u64 value = flag_builtin + 14; };
	// clang-format on

	// Wrapper for type_id::value.
	//
	template<typename T>
	static constexpr u64 of = type_id<T>::value;

	// Helper to test two type ids.
	//
	RC_INLINE static constexpr bool test(u64 a, u64 b, u64 ignored = flag_none) {
		a ^= b;
		return (a & ~ignored) == 0;
	}
	template<typename T>
	RC_INLINE static constexpr bool test(u64 b, u64 ignored = flag_none) {
		return test(of<T>, b, ignored);
	}
};