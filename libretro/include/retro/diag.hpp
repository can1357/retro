#pragma once
#include <array>
#include <utility>
#include <tuple>
#include <string_view>
#include <initializer_list>
#include <retro/common.hpp>
#include <retro/format.hpp>
#include <variant>

// This file defines structured diagnosis formatters.
// - Format string is a string literal that contains '%'s where a parameter is to be expected.
// '% expected, got %' will define the following functions in the class instance:
//    - static std::string format<T1, T2>(const T1& a, const T2& b)
//    - static void raise<T1, T2> [[noreturn]] (const T1& a, const T2& b);
//    - std::string operator()(...) const -> format(...).
//
namespace retro::diag {
	// For each parameter we use a different color.
	//
	namespace detail {
		inline constexpr std::string_view styles[] = {
			 RC_TEAL,
			 RC_SEA_GREEN,
			 RC_PURPLE,
			 RC_BLUE,
			 RC_GREEN,
			 RC_VIOLET,
			 RC_NAVY_BLUE,
			 RC_CYAN,
		};
	};
	static constexpr std::string_view get_parameter_style(size_t n) { return detail::styles[n % std::size(detail::styles)]; }

	// Diagnosis kinds:
	// - Ok       = OK.
	// - Warn     = Warning, usually immediately printed so not checked against.
	// - Error    = Hard error.
	//
	enum diag_kind : u8 {
		ok,
		warn,
		error,
		max,
	};

	// Defines a lazy diagnosis type.
	//
	namespace detail {
		template<typename F = void>
		struct lazy_storage;

		template<>
		struct lazy_storage<void> {
			lazy_storage<void>* next	 = nullptr;
			diag_kind			  kind	 = ok;
			virtual std::string print() = 0;
			virtual ~lazy_storage() = default;
		};

		template<typename F>
		struct lazy_storage : lazy_storage<void> {
			F fn;
			template<typename T>
			constexpr lazy_storage(diag_kind kind, T&& arg) : fn(std::forward<T>(arg)) {
				this->kind = kind;
			}
			std::string print() override {
				std::string result = fn();
				if (next)
					result += next->print();
				return result;
			}
		};

		template<typename F>
		inline lazy_storage<void>* make_lazy(diag_kind kind, F&& fn) {
			using Fn = std::decay_t<F>;
			if constexpr (std::is_same_v<Fn, const char*> || std::is_same_v<Fn, std::string_view>) {
				switch (kind) {
					case warn:
						return detail::make_lazy(kind, [s = fn]() { return fmt::concat(RC_YELLOW "warn: " RC_RESET, s); });
					case error:
						return detail::make_lazy(kind, [s = fn]() { return fmt::concat(RC_CRIMSON "error: " RC_RESET, s); });
					default:
						return nullptr;
				}
			} else {
				RC_ASSERT(kind != ok);
				return new detail::lazy_storage<Fn>(kind, std::forward<F>(fn));
			}
		}

		inline constexpr size_t arg_count(const char* n) {
			size_t result = 0;
			while (*n) {
				result += *n == '%';
				n++;
			}
			return result;
		}
	};
	struct RC_TRIVIAL_ABI lazy {
		detail::lazy_storage<>* ptr = nullptr;

		// Default construction.
		//
		constexpr lazy() = default;
		constexpr lazy(std::nullopt_t){};

		// Construction by kind and formatter.
		//
		template<typename F>
		lazy(diag_kind kind, F&& fn) : ptr(detail::make_lazy(kind, std::forward<F>(fn))) {}
		lazy(diag_kind kind) { upgrade(kind); }

		// No copy, move by swap.
		//
		lazy(const lazy&)				  = delete;
		lazy& operator=(const lazy&) = delete;
		constexpr lazy(lazy&& o) noexcept : ptr(std::exchange(o.ptr, nullptr)) {}
		constexpr lazy& operator=(lazy&& o) noexcept {
			std::swap(ptr, o.ptr);
			return *this;
		}

		// Combination.
		//
		void combine(lazy&& b) {
			auto optr = std::exchange(b.ptr, nullptr);
			if (!optr)
				return;
			if (!ptr) {
				ptr = optr;
				return;
			}

			if (ptr->kind < optr->kind)
				std::swap(ptr, optr);

			for (auto it = ptr;; it = it->next) {
				if (!it->next) {
					it->next = optr;
					break;
				}
			}
		}
		lazy operator+(lazy b) && {
			combine(std::move(b));
			return std::move(*this);
		}
		lazy& operator+=(lazy o) {
			combine(std::move(o));
			return *this;
		}

		// String conversion.
		//
		std::string to_string() const {
			if (ptr)
				return ptr->print();
			else
				return "";
		}
		void print() const {
			if (ptr) {
				puts(ptr->print().c_str());
			}
		}

		// Gets error kind.
		//
		diag_kind kind() const {
			return ptr ? ptr->kind : ok;
		}

		// Upgrades error kind.
		//
		void upgrade(diag_kind k) {
			if (ptr) {
				if (ptr->kind < k)
					ptr->kind = k;
			} else {
				switch (k) {
					case error:
						ptr = detail::make_lazy(k, "unknown error.");
						break;
					default:
						break;
				}
			}
		}

		// Equality comparison with kind.
		//
		constexpr bool operator==(diag_kind k) const noexcept {
			if (k == ok)
				return ptr == nullptr;
			else
				return ptr && ptr->kind == k;
		}

		// Raises the error.
		//
		void raise() {
			if (ptr) [[unlikely]] {
				print();
				if (ptr->kind > warn) {
					fmt::abort_no_msg();
				}
			}
		}

		// Conversion to bool to check state.
		//
		constexpr bool		 has_error() const { return ptr && ptr->kind >= error; }
		constexpr bool		 okay() const { return ptr == nullptr; }
		constexpr explicit operator bool() const { return ptr != nullptr; }

		// Delete on destruction.
		//
		constexpr void reset() {
			while (ptr) {
				delete std::exchange(ptr, ptr->next);
			}
		}
		constexpr ~lazy() { reset(); }
	};

	// Expected type.
	//
	template<typename T>
	struct expected {
		std::variant<lazy, T> var;

		// Default to error, construction by values.
		//
		constexpr expected() : var(lazy(diag_kind::error)) {}
		constexpr expected(lazy msg) : var(std::move(msg)) {}
		constexpr expected(T val) : var(std::move(val)) {}

		// Default move.
		//
		constexpr expected(expected&&) noexcept = default;
		constexpr expected& operator=(expected&&) noexcept = default;

		// Conversion to bool to check state.
		//
		constexpr bool		 has_value() const { return var.index() == 1; }
		constexpr explicit operator bool() const { return has_value(); }

		// Gets the value or error.
		//
		constexpr const lazy& error() const & {
			RC_ASSERT(!has_value());
			return std::get<0>(var);
		}
		constexpr lazy& error() & {
			RC_ASSERT(!has_value());
			return std::get<0>(var);
		}
		constexpr lazy error() && {
			RC_ASSERT(!has_value());
			return std::get<0>(std::move(var));
		}
		constexpr T& value() & {
			if (!has_value()) [[unlikely]] {
				std::get<0>(var).raise();
				fmt::abort_no_msg();
			}
			return std::get<1>(var);
		}
		constexpr const T& value() const& {
			if (!has_value()) [[unlikely]] {
				std::get<0>(var).raise();
				fmt::abort_no_msg();
			}
			return std::get<1>(var);
		}
		constexpr T value() && {
			if (!has_value()) [[unlikely]] {
				std::get<0>(var).raise();
				fmt::abort_no_msg();
			}
			return std::get<1>(std::move(var));
		}
	};
	
	// Define the formatter.
	//
	namespace detail {
		template<size_t N>
		struct splitter {
			std::array<std::string_view, N + 1> data = {};
			constexpr splitter(std::string_view format) {
				for (size_t i = 0; i != N; i++) {
					auto pos = format.find('%');
					data[i]	= format.substr(0, pos);
					format.remove_prefix(pos + 1);
				}
				data[N] = format;
			}

			template<typename Tup, size_t... Idx>
				requires(sizeof...(Idx) == N)
			std::string mix(const Tup& tuple, std::index_sequence<Idx...>) const {
				return fmt::concat(std::forward_as_tuple(std::get<Idx>(data), get_parameter_style(Idx), std::get<Idx>(tuple), RC_RESET)..., data.back());
			}
		};
	};
	template<diag_kind Kind, string_literal F, size_t N>
	struct formatter {
		static constexpr detail::splitter<N> pieces = {F.to_string()};

		template<typename... Tx>
		static std::string format(const Tx&... args) {
			return pieces.mix(std::tuple<const Tx&...>(args...), std::make_index_sequence<N>{});
		}
		
		template<typename... Tx>
		RC_COLD static void print(const Tx&... args) {
			fmt::println(format<Tx...>(args...));
		}

		template<typename... Tx>
		RC_COLD static void raise(const Tx&... args) {
			print<Tx...>(args...);
			if constexpr (Kind > warn) {
				fmt::abort_no_msg();
			}
		}

		template<typename... Tx>
		lazy operator()(Tx&&... args) const {
			return lazy(Kind, [args = std::tuple<std::decay_t<Tx>...>{std::forward<Tx>(args)...}]() { return std::apply([](const auto&... ax) { return format(ax...); }, args); });
		}
	};
};
#define RC_DEF_ERR(name, ...)                                                                                                                              \
	namespace err {                                                                                                                                         \
		static constexpr retro::diag::formatter<retro::diag::error, RC_CRIMSON "error: " RC_RESET __VA_ARGS__ ".", retro::diag::detail::arg_count(__VA_ARGS__)> name; \
	}
#define RC_DEF_WARN(name, ...)                                                                                                                           \
	namespace warn {                                                                                                                                      \
		static constexpr retro::diag::formatter<retro::diag::warn, RC_YELLOW "warn:  " RC_RESET __VA_ARGS__ ".", retro::diag::detail::arg_count(__VA_ARGS__)> name; \
	}