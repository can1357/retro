#pragma once
#include <retro/common.hpp>
#include <retro/utf.hpp>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <filesystem>
#include <optional>
#include <tuple>
#include <memory>

#if !RC_NO_ANSI
	#define RC_ANSI_ESC(...)  "\x1B[" __VA_ARGS__ "m"
#else
	#define RC_ANSI_ESC(...) ""
#endif
#define RC_BLACK		RC_ANSI_ESC("0;2;30")	// Black -> White
#define RC_GRAY		RC_ANSI_ESC("0;1;30")	//
#define RC_DEFAULT	RC_ANSI_ESC("0;2;37")	//
#define RC_WHITE		RC_ANSI_ESC("0;1;37")	//
#define RC_PURPLE		RC_ANSI_ESC("0;1;35")	// Purple
#define RC_VIOLET		RC_ANSI_ESC("0;2;35")	//
#define RC_CYAN		RC_ANSI_ESC("0;1;36")	// Cyan
#define RC_TEAL		RC_ANSI_ESC("0;2;36")	//
#define RC_YELLOW		RC_ANSI_ESC("0;1;33")	// Yellow
#define RC_ORANGE		RC_ANSI_ESC("0;2;33")	//
#define RC_RED			RC_ANSI_ESC("0;1;31")	// Red
#define RC_CRIMSON	RC_ANSI_ESC("0;2;31")	//
#define RC_GREEN		RC_ANSI_ESC("0;1;32")	// Green
#define RC_SEA_GREEN RC_ANSI_ESC("0;2;32")	//
#define RC_BLUE		RC_ANSI_ESC("0;1;34")	// Blue
#define RC_NAVY_BLUE RC_ANSI_ESC("0;2;34")	//
#define RC_UNDERLINE RC_ANSI_ESC("000004")	//
#define RC_RESET		RC_ANSI_ESC("000000")	//

namespace retro::fmt {
	// We use fixed escape code lengths to make stripping code faster.
	//
	static constexpr size_t ansi_esc_len = sizeof(RC_RESET) - 1;

	// Calculates the length of string (in codepoints displayed) without the ANSI escapes we've introduced.
	//
	inline size_t display_length(std::string_view s) {
		size_t result = utf::length(s);
#if !RC_NO_ANSI
		while (true) {
			auto p = s.find("\x1B[");
			if (p == std::string::npos) {
				break;
			}
			result -= ansi_esc_len;
			s.remove_prefix(p + ansi_esc_len);
		}
#endif
		return result;
	}

	// Removes all ANSI escapes we've introduced.
	//
	inline std::string strip_ansi(std::string s) {
#if !RC_NO_ANSI
		size_t offset = 0;
		while (offset < s.size()) {
			auto p = s.find("\x1B[", offset);
			if (p == std::string::npos) {
				break;
			}
			s.erase(p, ansi_esc_len);
			offset = p;
		}
#endif
		return s;
	}

	// Format string and vararg to std::string.
	//
	inline std::string str(const char* fmt, ...) {
		static constexpr size_t small_capacity = 32;

		va_list a1;
		va_start(a1, fmt);

		va_list a2;
		va_copy(a2, a1);
		std::string buffer;
		buffer.resize(small_capacity);
		buffer.resize(vsnprintf(buffer.data(), small_capacity + 1, fmt, a2));
		va_end(a2);
		if (buffer.size() <= small_capacity) {
			va_end(a1);
			return buffer;
		} else {
			vsnprintf(buffer.data(), buffer.size() + 1, fmt, a1);
			va_end(a1);
			return buffer;
		}
	}

	// Forward declaration of concat.
	//
	template<typename... Tx>
	static std::string concat(const Tx&... args);

	// Conversion from any type to std::string_view or std::string.
	//
	namespace detail {
		template<typename T>
		concept CustomFormattable = requires(const T& v) { v.to_string(); };
		template<typename T>
		concept CustomFormattablePtr = requires(const T& v) { v->to_string(); };
		template<typename T>
		concept StlFormattable = requires(const T& v) { std::to_string(v); };
	};
	template<typename T>
	static auto to_str(const T& arg) {
		if constexpr (std::is_bounded_array_v<T> ) {
			using El = std::remove_extent_t<T>;
			if constexpr (std::is_same_v<std::remove_const_t<El>, char>) {
				return std::string_view{ arg, std::extent_v<T> - 1 };
			}
		}
		using Td = std::decay_t<T>;

		if constexpr (std::is_same_v<Td, char*> || std::is_same_v<Td, const char*> || std::is_same_v<Td, std::string_view> || std::is_same_v<Td, std::string>) {
			return std::string_view{arg};
		} else if constexpr (detail::CustomFormattable<Td>) {
			return arg.to_string();
		} else if constexpr (detail::CustomFormattablePtr<Td>) {
			return arg->to_string();
		} else if constexpr (std::is_same_v<Td, std::filesystem::path>) {
			return arg.string();
		} else if constexpr (detail::StlFormattable<Td>) {
			if constexpr (std::is_same_v<Td, char>) {
				if (isprint((int) arg)) {
					std::string res;
					res += '\'';
					res += (char) arg;
					res += '\'';
					return res;
				}
			}
			return std::to_string(arg);
		} else if constexpr (is_specialization_v<std::optional, Td>) {
			if (arg) {
				return to_str(*arg);
			} else {
				return (decltype(to_str(*arg))) "null";
			}
		} else if constexpr (is_specialization_v<std::pair, Td> || is_specialization_v<std::tuple, Td>) {
			return std::apply([]<typename... Tx>(Tx&&... args) { return fmt::concat<Tx...>(std::forward<Tx>(args)...); }, arg);
		} else if constexpr (std::is_pointer_v<T>) {
			return str("%p", arg);
		} else {
			static_assert(sizeof(T) == -1, "Type is not formattable.");
		}
	}
	
	// Variable count string concat, Tx should be all convertible to string_view.
	//
	namespace detail {
		template<typename T, typename... Tx>
		static void write(char* at, const T& first, const Tx&... rest) {
			memcpy(at, first.data(), first.size());
			if constexpr (sizeof...(Tx) != 0) {
				write<Tx...>(at + first.size(), rest...);
			}
		}
		template<typename... Tx>
		static std::string concat(const Tx&... args) {
			std::string buffer((args.size() + ...), '\x0');
			write(buffer.data(), args...);
			return buffer;
		}
	};
	template<typename... Tx>
	static std::string concat(const Tx&... args) {
		if constexpr (sizeof...(Tx) == 0) {
			return {};
		} else {
			return detail::concat(to_str(args)...);
		}
	}
	template<typename T, typename... Tx>
	static void print(const T& first, const Tx&... rest) {
		auto				  str = to_str(first);
		std::string_view view{str};
		fwrite(view.data(), 1, view.size(), stdout);
		if constexpr (sizeof...(Tx) != 0) {
			print<Tx...>(rest...);
		}
	}
	template<typename... Tx>
	static void println(const Tx&... rest) {
		print<Tx...>(rest...);
		putchar('\n');
	}

	// Asserts and errors.
	//
	RC_COLD inline void abort_no_msg [[noreturn]] () {
		puts(RC_RESET);
		fflush(stdout);
#if RC_DEBUG
		RC_DBGBREAK();
#else
		::abort();
#endif
	}
	RC_COLD inline void abort [[noreturn]] (const char* msg) {
		puts(msg);
		abort_no_msg();
	}
	template<typename T, typename... Tx>
	RC_COLD inline void abort [[noreturn]] (const char* fmt, const T& a1, const Tx&... rest) {
		printf(fmt, a1, rest...);
		abort_no_msg();
	}

#if RC_DEBUG
	#define RC_ASSERT(...)                                                                                                            \
		do                                                                                                                             \
			if (!(__VA_ARGS__)) [[unlikely]]                                                                                            \
				retro::fmt::abort("Assertion \"" RC_STRINGIFY(__VA_ARGS__) "\" failed. [" __FILE__ ":" RC_STRINGIFY(__LINE__) "]\n"); \
		while (0)
	#define RC_ASSERT_MSG(msg, ...)                                                   \
		do                                                                             \
			if (!(__VA_ARGS__)) [[unlikely]]                                            \
				retro::fmt::abort(msg "[" __FILE__ ":" RC_STRINGIFY(__LINE__) "]\n"); \
		while (0)
	#define RC_UNREACHABLE() retro::fmt::abort("Unreachable assumption failed at [" __FILE__ ":" RC_STRINGIFY(__LINE__) "]\n");
#else
	#define RC_ASSERT(...)			  retro::assume_that(bool(__VA_ARGS__))
	#define RC_ASSERT_MSG(msg, ...) retro::assume_that(bool(__VA_ARGS__))
	#define RC_UNREACHABLE()		  retro::assume_unreachable()
#endif
};