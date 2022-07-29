#pragma once
#include <retro/common.hpp>
#include <retro/arch/reg_kind.hxx>

namespace retro::arch {
	struct instance;

	// Register type.
	//
	struct RC_TRIVIAL_ABI mreg {
		u32 id : 24	 = 0;	 // Register id. (Note: may not be not unique, combine with kind).
		u32 kind : 8 = 0;	 // Register kind.
		static_assert(u32(reg_kind::bit_width) <= 8, "Invalid layout.");

		// Default construction and copy.
		//
		constexpr mreg()									 = default;
		constexpr mreg(mreg&&) noexcept				 = default;
		constexpr mreg& operator=(mreg&&) noexcept = default;
		constexpr mreg(const mreg&)					 = default;
		constexpr mreg& operator=(const mreg&)		 = default;

		// Full value construction.
		//
		constexpr mreg(u32 id, reg_kind kind) : id(id), kind(u32(kind)) {}

		// Automatic resolution from enum types.
		//
		template<typename T>
			requires std::is_scoped_enum_v<T>
		constexpr mreg(T id) : id((u32) id), kind((u32) enum_reflect(id).kind) {}

		// Gets the kind in the enum type.
		//
		constexpr reg_kind get_kind() const { return reg_kind(kind); }

		// Gets the unique identifier.
		//
		constexpr u32 uid() const { return retro::bit_cast<u32>(*this); }

		// Validity checks.
		//
		constexpr bool		 is_valid() const { return uid() != 0; }
		constexpr explicit operator bool() const { return is_valid(); }

		// Comparison.
		//
		constexpr bool operator==(const mreg& o) const { return uid() == o.uid(); }
		constexpr bool operator!=(const mreg& o) const { return uid() != o.uid(); }
		constexpr bool operator<(const mreg& o) const { return uid() < o.uid(); }

		// String conversion.
		//
		std::string_view name(instance* i = nullptr) const;
		std::string		  to_string(instance* i = nullptr) const;
	};
};