#pragma once
#include <retro/common.hpp>
#include <retro/hashers.hpp>

// Compile-Time type identifiers.
//
namespace retro::ctti {
	using id = u32;

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
		static constexpr id get() {
			id tmp = fnv1a_32_hasher::offset;
			for (char c : name()) {
				tmp = fnv1a_32_hash(u8(c), tmp);
			}
			return tmp;
		}
		static constexpr id value = get();
	};

	// Wrapper for type_id::value.
	//
	template<typename T>
	static constexpr id of = type_id<T>::value;
};