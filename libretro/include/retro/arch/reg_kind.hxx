#pragma once
#include <retro/common.hpp>
#include <retro/ir/builtin_types.hxx>

// clang-format off
namespace retro::arch {
	enum class reg_kind : u8 /*:5*/ {
		none        = 0,
		gpr8        = 1,
		gpr16       = 2,
		gpr32       = 3,
		gpr64       = 4,
		gpr128      = 5,
		fp32        = 6,
		fp64        = 7,
		fp80        = 8,
		simd64      = 9,
		simd128     = 10,
		simd256     = 11,
		simd512     = 12,
		instruction = 13,
		flag        = 14,
		segment     = 15,
		control     = 16,
		misc        = 17,
		// PSEUDO
		last        = 17,
		bit_width   = 5,
	};
	#define RC_VISIT_ARCH_REG_KIND(_) _(gpr8) _(gpr16) _(gpr32) _(gpr64) _(gpr128) _(fp32) _(fp64) _(fp80) _(simd64) _(simd128) _(simd256) _(simd512) _(instruction) _(flag) _(segment) _(control) _(misc)
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//                                                      Descriptors                                                      //
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	struct reg_kind_desc {
		std::string_view name           = {};
		ir::type         type           = {};
		u16              width : 10     = 0;
		u8               is_pointer : 1 = 0;
	
		using value_type = reg_kind;
		static constexpr std::span<const reg_kind_desc> all();
		RC_INLINE constexpr const reg_kind id() const { return reg_kind(this - all().data()); }
	};
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//                                                         Tables                                                         //
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	inline constexpr reg_kind_desc reg_kinds[] = {
		{"none"},
		{"gpr8",ir::type::i8,8,0},
		{"gpr16",ir::type::i16,16,0},
		{"gpr32",ir::type::i32,32,0},
		{"gpr64",ir::type::i64,64,0},
		{"gpr128",ir::type::i128,128,0},
		{"fp32",ir::type::f32,32,0},
		{"fp64",ir::type::f64,64,0},
		{"fp80",ir::type::f80,80,0},
		{"simd64",ir::type::i32x2,64,0},
		{"simd128",ir::type::i32x4,128,0},
		{"simd256",ir::type::i32x8,256,0},
		{"simd512",ir::type::i32x16,512,0},
		{"instruction",ir::type::pointer,0,1},
		{"flag",ir::type::i1,1,0},
		{"segment",ir::type::i16,16,0},
		{"control",ir::type::none,0,0},
		{"misc",ir::type::none,0,0},
	};
	RC_INLINE constexpr std::span<const reg_kind_desc> reg_kind_desc::all() { return reg_kinds; }
};
namespace retro { template<> struct descriptor<retro::arch::reg_kind> { using type = retro::arch::reg_kind_desc; }; };
RC_DEFINE_STD_VISITOR_FOR(retro::arch::reg_kind, RC_VISIT_ARCH_REG_KIND)
// clang-format on