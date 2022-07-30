#include <retro/ir/types.hpp>

// TODO: Constant cast.

namespace retro::ir {
	// Catch-all fail case.
	//
#define DECL_FAIL(OP) static constant RC_CONCAT(apply_, OP)(...) { return {}; }
	RC_VISIT_IR_OP(DECL_FAIL)


	template<typename T>
	concept Integer = std::is_integral_v<T>;
	template<typename T>
	concept Float = std::is_floating_point_v<T>;
	template<typename T>
	concept Arithmetic = std::is_arithmetic_v<T>;

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
	RC_INLINE static auto apply_rem(Arithmetic auto lhs, Arithmetic auto rhs) -> decltype(rhs) { return lhs % rhs; }
	RC_INLINE static auto apply_max(Arithmetic auto lhs, Arithmetic auto rhs) -> decltype(rhs) { return std::max(lhs, rhs); }
	RC_INLINE static auto apply_min(Arithmetic auto lhs, Arithmetic auto rhs) -> decltype(rhs) { return std::min(lhs, rhs); }

	// Bitwise unary.
	//
	RC_INLINE static constexpr auto apply_bit_not(Integer auto rhs, ...) -> decltype(rhs) { return ~rhs; }
	RC_INLINE static constexpr auto apply_bit_lsb(Integer auto rhs, ...) -> decltype(rhs) { return (decltype(rhs)) (std::countl_zero(to_unsigned(rhs))); }
	RC_INLINE static constexpr auto apply_bit_msb(Integer auto rhs, ...) -> decltype(rhs) { return (decltype(rhs)) (sizeof(rhs) * 8 - std::countr_zero(to_unsigned(rhs))); }
	RC_INLINE static constexpr auto apply_bit_popcnt(Integer auto rhs, ...) -> decltype(rhs) { return (decltype(rhs)) (std::popcount(to_unsigned(rhs))); }
	RC_INLINE static constexpr auto apply_bit_byteswap(Integer auto rhs, ...) -> decltype(rhs) { return (decltype(rhs)) (std::byteswap(to_unsigned(rhs))); }

	// Bitwise binary.
	//
	RC_INLINE static auto apply_bit_or(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return lhs | rhs; }
	RC_INLINE static auto apply_bit_and(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return lhs & rhs; }
	RC_INLINE static auto apply_bit_xor(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return lhs ^ rhs; }
	RC_INLINE static auto apply_bit_shl(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return lhs << rhs; }
	RC_INLINE static auto apply_bit_shr(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return lhs >> rhs; }
	RC_INLINE static auto apply_bit_sar(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return to_unsigned(to_signed(lhs) >> rhs); }
	RC_INLINE static auto apply_bit_rol(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return std::rotl(lhs, (int) rhs); }
	RC_INLINE static auto apply_bit_ror(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return std::rotr(lhs, (int) rhs); }

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
	RC_INLINE static auto apply_umax(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return to_signed(std::max(to_unsigned(lhs), to_signed(rhs))); }
	RC_INLINE static auto apply_umin(Integer auto lhs, Integer auto rhs) -> decltype(rhs) { return to_signed(std::min(to_unsigned(lhs), to_signed(rhs))); }


	//
	// TODO: f80, i128, u128, vector types.
	//

	// Operator visitor.
	//
	template<type Ty, typename A>
	RC_INLINE static constant apply_visitor(op o, const A& lhs, const A& rhs) {
#define OPERATOR_VISITOR(OP) \
	case op::OP:              \
		return RC_CONCAT(apply_, OP)(Ty, lhs, rhs);

		switch (o) {
			RC_VISIT_IR_OP(OPERATOR_VISITOR)
			default:
				assume_unreachable();
		}
	}

	// Application of an operator over two constants, returns "none" on failure.
	//
	constant constant::apply(op o, const constant& rhs) const {
		RC_ASSERT(rhs.get_type() == get_type());

		// Visit the type and apply.
		//
#define APPLY_IF(A, B)                                             \
	case type::A: {                                                 \
		if constexpr (BuiltinType<B> && type::A != type::str) {      \
			return apply_visitor<type::A>(o, get<B>(), rhs.get<B>()); \
		} else {                                                     \
			assume_unreachable();                                     \
		}                                                            \
	}
		switch (get_type()) {
			RC_VISIT_IR_TYPE(APPLY_IF)
			default: assume_unreachable();
		}
	}
};