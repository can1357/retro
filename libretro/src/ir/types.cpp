#include <retro/ir/types.hpp>

/*
TODO: Fix for f80 where it needs to be emulated
		Implement for i128, u128
		Implement vector comparison.
		Add overflow checks
*/

namespace retro::ir {
	// Catch-all fail case.
	//
#define DECL_FAIL(OP) static std::nullopt_t RC_CONCAT(apply_, OP)(...) { return std::nullopt; }
	RC_VISIT_IR_OP(DECL_FAIL)

	// Short names.
	//
	using i1 = bool;
	template<typename T, size_t N>
	using vec = std::array<T, N>;

	// Define the concepts.
	//
	template<typename T>
	concept Integer = std::is_integral_v<T> && !std::is_same_v<T, i1>;
	template<typename T>
	concept Float = std::is_floating_point_v<T>;
	template<typename T>
	concept Arithmetic = Integer<T> || Float<T>;
	template<typename T>
	concept Scalar = Integer<T> || Float<T> || std::is_same_v<T, i1>;

	// Sign reinterp. helper.
	//
	template<Integer I>
	RC_INLINE static std::make_unsigned_t<I> to_unsigned(I value) {
		return (std::make_unsigned_t<I>) value;
	}
	template<Integer I>
	RC_INLINE static std::make_signed_t<I> to_signed(I value) {
		return (std::make_signed_t<I>) value;
	}

	// Arithmetic unary.
	//
	RC_INLINE static auto apply_neg(Arithmetic auto rhs, ...) -> decltype(rhs) { return -rhs; }
	RC_INLINE static auto apply_abs(Arithmetic auto rhs, ...) -> decltype(rhs) { return rhs < 0 ? -rhs : rhs; }

	// Arithmetic binary.
	//
	RC_INLINE static auto apply_add(Arithmetic auto lhs, Arithmetic auto rhs) -> decltype(rhs) { return lhs + rhs; }
	RC_INLINE static auto apply_sub(Arithmetic auto lhs, Arithmetic auto rhs) -> decltype(rhs) { return lhs - rhs; }
	RC_INLINE static auto apply_mul(Arithmetic auto lhs, Arithmetic auto rhs) -> decltype(rhs) { return lhs * rhs; }
	RC_INLINE static auto apply_div(Arithmetic auto lhs, Arithmetic auto rhs) -> decltype(rhs) { return lhs / rhs; }
	RC_INLINE static auto apply_max(Arithmetic auto lhs, Arithmetic auto rhs) -> decltype(rhs) { return std::max(lhs, rhs); }
	RC_INLINE static auto apply_min(Arithmetic auto lhs, Arithmetic auto rhs) -> decltype(rhs) { return std::min(lhs, rhs); }
	RC_INLINE static auto apply_rem(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return lhs % rhs; }
	RC_INLINE static auto apply_rem(Float auto lhs, Float auto rhs) -> decltype(rhs) { return std::fmod(lhs, rhs); }

	// Bitwise unary.
	//
	RC_INLINE static auto apply_bit_not(Integer auto rhs, ...) -> decltype(rhs) { return ~rhs; }
	RC_INLINE static auto apply_bit_lsb(Integer auto rhs, ...) -> decltype(rhs) { return (decltype(rhs)) (std::countl_zero(to_unsigned(rhs))); }
	RC_INLINE static auto apply_bit_msb(Integer auto rhs, ...) -> decltype(rhs) { return (decltype(rhs)) (sizeof(rhs) * 8 - std::countr_zero(to_unsigned(rhs))); }
	RC_INLINE static auto apply_bit_popcnt(Integer auto rhs, ...) -> decltype(rhs) { return (decltype(rhs)) (std::popcount(to_unsigned(rhs))); }
	RC_INLINE static auto apply_bit_byteswap(Integer auto rhs, ...) -> decltype(rhs) { return (decltype(rhs)) (std::byteswap(to_unsigned(rhs))); }

	// Bitwise binary.
	//
	RC_INLINE static auto apply_bit_or(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return lhs | rhs; }
	RC_INLINE static auto apply_bit_and(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return lhs & rhs; }
	RC_INLINE static auto apply_bit_xor(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return lhs ^ rhs; }
	RC_INLINE static auto apply_bit_shl(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return lhs << rhs; }
	RC_INLINE static auto apply_bit_shr(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return lhs >> rhs; }
	RC_INLINE static auto apply_bit_sar(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return to_unsigned(to_signed(lhs) >> rhs); }
	RC_INLINE static auto apply_bit_rol(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return to_signed(std::rotl(to_unsigned(lhs), (int) rhs)); }
	RC_INLINE static auto apply_bit_ror(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return to_signed(std::rotr(to_unsigned(lhs), (int) rhs)); }

	// Floating point specials.
	//
	RC_INLINE static auto apply_ceil(Float auto rhs, ...) -> decltype(rhs) { return std::ceil(rhs); }
	RC_INLINE static auto apply_floor(Float auto rhs, ...) -> decltype(rhs) { return std::floor(rhs); }
	RC_INLINE static auto apply_trunc(Float auto rhs, ...) -> decltype(rhs) { return std::trunc(rhs); }
	RC_INLINE static auto apply_round(Float auto rhs, ...) -> decltype(rhs) { return std::round(rhs); }
	RC_INLINE static auto apply_sin(Float auto rhs, ...) -> decltype(rhs) { return std::sin(rhs); }
	RC_INLINE static auto apply_cos(Float auto rhs, ...) -> decltype(rhs) { return std::cos(rhs); }
	RC_INLINE static auto apply_tan(Float auto rhs, ...) -> decltype(rhs) { return std::tan(rhs); }
	RC_INLINE static auto apply_asin(Float auto rhs, ...) -> decltype(rhs) { return std::asin(rhs); }
	RC_INLINE static auto apply_acos(Float auto rhs, ...) -> decltype(rhs) { return std::acos(rhs); }
	RC_INLINE static auto apply_atan(Float auto rhs, ...) -> decltype(rhs) { return std::atan(rhs); }
	RC_INLINE static auto apply_sqrt(Float auto rhs, ...) -> decltype(rhs) { return std::sqrt(rhs); }
	RC_INLINE static auto apply_exp(Float auto rhs, ...) -> decltype(rhs) { return std::exp(rhs); }
	RC_INLINE static auto apply_log(Float auto rhs, ...) -> decltype(rhs) { return std::log(rhs); }
	RC_INLINE static auto apply_atan2(Float auto lhs, Float auto rhs) -> decltype(rhs) { return std::atan2(lhs, rhs); }
	RC_INLINE static auto apply_pow(Float auto lhs, Float auto rhs) -> decltype(rhs) { return std::pow(lhs, rhs); }

	// Comperators.
	//
	RC_INLINE static auto apply_ge(Arithmetic auto lhs, Arithmetic auto rhs) -> bool { return lhs >= rhs; }
	RC_INLINE static auto apply_gt(Arithmetic auto lhs, Arithmetic auto rhs) -> bool { return lhs > rhs; }
	RC_INLINE static auto apply_eq(Arithmetic auto lhs, Arithmetic auto rhs) -> bool { return lhs == rhs; }
	RC_INLINE static auto apply_ne(Arithmetic auto lhs, Arithmetic auto rhs) -> bool { return lhs != rhs; }
	RC_INLINE static auto apply_lt(Arithmetic auto lhs, Arithmetic auto rhs) -> bool { return lhs < rhs; }
	RC_INLINE static auto apply_le(Arithmetic auto lhs, Arithmetic auto rhs) -> bool { return lhs <= rhs; }

	// Unsigned specializations.
	//
	RC_INLINE static auto apply_uge(Integer auto lhs, Integer auto rhs) -> bool { return to_unsigned(lhs) >= to_unsigned(rhs); }
	RC_INLINE static auto apply_ugt(Integer auto lhs, Integer auto rhs) -> bool { return to_unsigned(lhs) > to_unsigned(rhs); }
	RC_INLINE static auto apply_ult(Integer auto lhs, Integer auto rhs) -> bool { return to_unsigned(lhs) < to_unsigned(rhs); }
	RC_INLINE static auto apply_ule(Integer auto lhs, Integer auto rhs) -> bool { return to_unsigned(lhs) <= to_unsigned(rhs); }
	RC_INLINE static auto apply_umul(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return to_signed(to_unsigned(lhs) * to_signed(rhs)); }
	RC_INLINE static auto apply_udiv(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return to_signed(to_unsigned(lhs) / to_signed(rhs)); }
	RC_INLINE static auto apply_urem(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return to_signed(to_unsigned(lhs) % to_signed(rhs)); }
	RC_INLINE static auto apply_umax(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return to_signed(std::max(to_unsigned(lhs), to_unsigned(rhs))); }
	RC_INLINE static auto apply_umin(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return to_signed(std::min(to_unsigned(lhs), to_unsigned(rhs))); }

	// Boolean implementations, always assumes unsigned.
	//
	RC_INLINE static auto apply_neg(i1 rhs, ...) -> decltype(rhs) { return i1(1); }
	RC_INLINE static auto apply_abs(i1 rhs, ...) -> decltype(rhs) { return i1(0); }
	RC_INLINE static auto apply_add(i1 lhs, i1 rhs) -> decltype(rhs) { return lhs != rhs; }
	RC_INLINE static auto apply_sub(i1 lhs, i1 rhs) -> decltype(rhs) { return lhs != rhs; }
	RC_INLINE static auto apply_mul(i1 lhs, i1 rhs) -> decltype(rhs) { return lhs && rhs; }
	RC_INLINE static auto apply_umul(i1 lhs, i1 rhs) -> decltype(rhs) { return lhs && rhs; }
	RC_INLINE static auto apply_div(i1 lhs, i1 rhs) -> decltype(rhs) { return lhs; /*since rhs == 1*/ }
	RC_INLINE static auto apply_udiv(i1 lhs, i1 rhs) -> decltype(rhs) { return lhs; /*since rhs == 1*/ }
	RC_INLINE static auto apply_max(i1 lhs, i1 rhs) -> decltype(rhs) { return lhs || rhs; }
	RC_INLINE static auto apply_umax(i1 lhs, i1 rhs) -> decltype(rhs) { return lhs || rhs; }
	RC_INLINE static auto apply_min(i1 lhs, i1 rhs) -> decltype(rhs) { return lhs && rhs; }
	RC_INLINE static auto apply_umin(i1 lhs, i1 rhs) -> decltype(rhs) { return lhs && rhs; }
	RC_INLINE static auto apply_rem(i1 lhs, i1 rhs) -> decltype(rhs) { return 0; }
	RC_INLINE static auto apply_urem(i1 lhs, i1 rhs) -> decltype(rhs) { return 0; }
	RC_INLINE static auto apply_bit_or(i1 lhs, i1 rhs) -> decltype(rhs) { return lhs || rhs; }
	RC_INLINE static auto apply_bit_and(i1 lhs, i1 rhs) -> decltype(rhs) { return lhs && rhs; }
	RC_INLINE static auto apply_bit_xor(i1 lhs, i1 rhs) -> decltype(rhs) { return lhs != rhs; }
	RC_INLINE static auto apply_bit_shl(i1 lhs, i1 rhs) -> decltype(rhs) { return rhs ? 0 : lhs; }
	RC_INLINE static auto apply_bit_shr(i1 lhs, i1 rhs) -> decltype(rhs) { return rhs ? 0 : lhs; }
	RC_INLINE static auto apply_bit_sar(i1 lhs, i1 rhs) -> decltype(rhs) { return lhs; }
	RC_INLINE static auto apply_bit_rol(i1 lhs, i1 rhs) -> decltype(rhs) { return lhs; }
	RC_INLINE static auto apply_bit_ror(i1 lhs, i1 rhs) -> decltype(rhs) { return lhs; }
	RC_INLINE static auto apply_bit_not(i1 rhs, ...) -> decltype(rhs) { return !rhs; }
	RC_INLINE static auto apply_bit_lsb(i1 rhs, ...) -> decltype(rhs) { return !rhs; }
	RC_INLINE static auto apply_bit_msb(i1 rhs, ...) -> decltype(rhs) { return !rhs; }
	RC_INLINE static auto apply_bit_popcnt(i1 rhs, ...) -> decltype(rhs) { return rhs; }
	RC_INLINE static auto apply_bit_byteswap(i1 rhs, ...) -> decltype(rhs) { return rhs; }
	RC_INLINE static auto apply_ge(i1 lhs, i1 rhs) -> bool { return lhs >= rhs; }
	RC_INLINE static auto apply_gt(i1 lhs, i1 rhs) -> bool { return lhs > rhs; }
	RC_INLINE static auto apply_eq(i1 lhs, i1 rhs) -> bool { return lhs == rhs; }
	RC_INLINE static auto apply_ne(i1 lhs, i1 rhs) -> bool { return lhs != rhs; }
	RC_INLINE static auto apply_lt(i1 lhs, i1 rhs) -> bool { return lhs < rhs; }
	RC_INLINE static auto apply_le(i1 lhs, i1 rhs) -> bool { return lhs <= rhs; }
	RC_INLINE static auto apply_uge(i1 lhs, i1 rhs) -> bool { return lhs >= rhs; }
	RC_INLINE static auto apply_ugt(i1 lhs, i1 rhs) -> bool { return lhs > rhs; }
	RC_INLINE static auto apply_ult(i1 lhs, i1 rhs) -> bool { return lhs < rhs; }
	RC_INLINE static auto apply_ule(i1 lhs, i1 rhs) -> bool { return lhs <= rhs; }

	// Operator visitor.
	//
	template<typename T>
	struct operator_visitor {
		RC_INLINE static std::nullopt_t apply(op o, const T& lhs, const T& rhs) { return std::nullopt; }
	};

	// Builtin types.
	//
	template<Scalar T>
	struct operator_visitor<T> {
		RC_INLINE static constant apply(op o, const T& lhs, const T& rhs) {
#define OPERATOR_VISITOR(OP)                                                                              \
	case op::OP:                                                                                           \
		if constexpr (op::OP == op::div || op::OP == op::rem || op::OP == op::udiv || op::OP == op::urem) { \
			if (rhs == 0 && Integer<T>) {                                                                    \
				return constant{};                                                                            \
			}                                                                                                \
		}                                                                                                   \
		return RC_CONCAT(apply_, OP)(lhs, rhs);

			switch (o) {
				RC_VISIT_IR_OP(OPERATOR_VISITOR)
				default:
					assume_unreachable();
			}
		}
	};

	// Vector types.
	//
	template<Scalar T, size_t N>
	struct operator_visitor<vec<T, N>> {
		RC_INLINE static constant apply(op o, const vec<T, N>& lhs, const vec<T, N>& rhs) {
#define VEC_OPERATOR_VISITOR(OP)                                                                                           \
	case op::OP: {                                                                                                          \
		/*If application result is boolean:*/                                                                                \
		using Result = decltype(RC_CONCAT(apply_, OP)(std::declval<T>(), std::declval<T>()));                                \
		if constexpr (std::is_same_v<Result, bool>) {                                                                        \
			/*Only EQ and NE are well defined.*/                                                                              \
			if (op::OP == op::eq) {                                                                                           \
				return memcmp(&lhs, &rhs, sizeof(T) * N) == 0;                                                                 \
			} else if (op::OP == op::ne) {                                                                                    \
				return memcmp(&lhs, &rhs, sizeof(T) * N) != 0;                                                                 \
			}                                                                                                                 \
			return std::nullopt;                                                                                              \
		} /*If implemented for the element type, apply in parallel:*/                                                        \
		else if constexpr (!std::is_same_v<Result, std::nullopt_t>) {                                                        \
			vec<T, N> result = {};                                                                                            \
			if constexpr (op::OP == op::div || op::OP == op::rem || op::OP == op::udiv || op::OP == op::urem && Integer<T>) { \
				for (size_t i = 0; i != N; i++) {                                                                              \
					if (rhs[i] == 0) {                                                                                          \
						return constant{};                                                                                       \
					}                                                                                                           \
				}                                                                                                              \
			}                                                                                                                 \
			for (size_t i = 0; i != N; i++) {                                                                                 \
				result[i] = RC_CONCAT(apply_, OP)(lhs[i], rhs[i]);                                                             \
			}                                                                                                                 \
			return result;                                                                                                    \
		} /*Otherwise invoke any vector handler if present.*/                                                                \
		else {                                                                                                               \
			return RC_CONCAT(apply_, OP)(lhs, rhs);                                                                           \
		}                                                                                                                    \
	}
			switch (o) {
				RC_VISIT_IR_OP(VEC_OPERATOR_VISITOR)
				default:
					assume_unreachable();
			}
		}
	};

	// Application of an operator over two constants, returns "none" on failure.
	//
	constant constant::apply(op o, const constant& rhs) const {
		RC_ASSERT(rhs.get_type() == get_type());

		// Visit the type and apply.
		//
#define APPLY_IF(A, B)                                                 \
	case type::A: {                                                     \
		if constexpr (BuiltinType<B> && type::A != type::str) {          \
			return operator_visitor<B>::apply(o, get<B>(), rhs.get<B>()); \
		} else {                                                         \
			assume_unreachable();                                         \
		}                                                                \
	}
		switch (get_type()) {
			RC_VISIT_IR_TYPE(APPLY_IF)
			default:
				assume_unreachable();
		}
	}


	// Cast visitor.
	//
	template<typename Src, typename Dst>
	struct cast_visitor {
		RC_INLINE static std::nullopt_t zx(const Src&) { return std::nullopt; }
		RC_INLINE static std::nullopt_t sx(const Src&) { return std::nullopt; }
	};

	// (int|fp) -> (int|fp).
	template<Arithmetic Src, Arithmetic Dst>
	struct cast_visitor<Src, Dst> {
		RC_INLINE static Dst zx(Src i) {
			if constexpr (Integer<Src>) {
				return (Dst) to_unsigned(i);
			} else {
				return (Dst) i;
			}
		}
		RC_INLINE static Dst sx(Src i) {
			return (Dst) i;
		}
	};

	// i1 -> (int|fp).
	template<Arithmetic Dst>
	struct cast_visitor<i1, Dst> {
		RC_INLINE static Dst zx(i1 i) { return Dst(i ? 1 : 0); }
		RC_INLINE static Dst sx(i1 i) { return Dst(i ? 1 : 0); }
	};

	// (int|fp) -> i1.
	template<Integer Src>
	struct cast_visitor<Src, i1> {
		RC_INLINE static i1 zx(Src i) { return (i & 1) != 0; }
		RC_INLINE static i1 sx(Src i) { return (i & 1) != 0; }
	};

	// Vector handler.
	//
	template<typename Src, typename Dst>
	concept ScalarConvertible = (!std::is_same_v<std::nullopt_t, decltype(cast_visitor<Src, Dst>::zx(std::declval<Src>()))>);
	template<typename Src, typename Dst, size_t N>
		requires ScalarConvertible<Src, Dst>
	struct cast_visitor<vec<Src, N>, vec<Dst, N>> {
		RC_INLINE static vec<Dst, N> zx(const vec<Src, N>& v) {
			vec<Dst, N> result = {};
			for (size_t i = 0; i != N; i++) {
				result[i] = cast_visitor<Src, Dst>::zx(v[i]);
			}
			return result;
		}
		RC_INLINE static vec<Dst, N> sx(const vec<Src, N>& v) {
			vec<Dst, N> result = {};
			for (size_t i = 0; i != N; i++) {
				result[i] = cast_visitor<Src, Dst>::sx(v[i]);
			}
			return result;
		}
	};

	template<typename F>
	RC_INLINE static decltype(auto) visit_valid(F&& fn, type a) {
		// Visit the type and apply.
		//
#define VISIT_IF(A, B)                                                 \
	case type::A: {                                                     \
		if constexpr (BuiltinType<B> && type::A != type::str) {          \
			return fn(type_tag<B>{});                                     \
		} else {                                                         \
			assume_unreachable();                                         \
		}                                                                \
	}
		switch (a) {
			RC_VISIT_IR_TYPE(VISIT_IF)
			default:
				assume_unreachable();
		}
	}


	// Cast helpers, returns "none" on failure.
	//
	constant constant::cast_zx(type into) const {
		return visit_valid([&]<typename Src> (type_tag<Src>) -> constant {
			return visit_valid([&] <typename Dst> (type_tag<Dst>) -> constant {
				return cast_visitor<Src, Dst>::zx(this->get<Src>());
			}, into);
		}, get_type());
	}
	constant constant::cast_sx(type into) const {
		return visit_valid([&]<typename Src> (type_tag<Src>) -> constant {
			return visit_valid([&] <typename Dst> (type_tag<Dst>) -> constant {
				return cast_visitor<Src, Dst>::sx(this->get<Src>());
			}, into);
		}, get_type());
	}

	// String conversion.
	//
	std::string constant::to_string() const {
		if (get_type() == type::str) {
			return fmt::concat(RC_ORANGE "'", get<std::string_view>(), "'" RC_RESET);
		} else if (get_type() == type::none) {
			return "void";
		} else {
			return visit_valid([&]<typename T>(type_tag<T>) { return std::string{fmt::to_str(get<T>())}; }, get_type());
		}
	}
};