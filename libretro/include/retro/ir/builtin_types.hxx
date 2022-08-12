#pragma once
#include <retro/common.hpp>
#include <string>

// clang-format off
namespace retro::ir {
	enum class type_kind : u8 /*:3*/ {
		none       = 0,
		memory     = 1,
		scalar_int = 2,
		scalar_fp  = 3,
		vector_int = 4,
		vector_fp  = 5,
		// PSEUDO
		last       = 5,
		bit_width  = 3,
	};
	#define RC_VISIT_IR_TYPE_KIND(_) _(memory) _(scalar_int) _(scalar_fp) _(vector_int) _(vector_fp)
	enum class type : u8 /*:6*/ {
		none      = 0,
		pack      = 1,
		context   = 2,
		reg       = 3,
		op        = 4,
		intrinsic = 5,
		label     = 6,
		str       = 7,
		i1        = 8,
		i8        = 9,
		i16       = 10,
		i32       = 11,
		i64       = 12,
		i128      = 13,
		f32       = 14,
		f64       = 15,
		f80       = 16,
		pointer   = 17,
		i32x2     = 18,
		i16x4     = 19,
		i8x8      = 20,
		f32x2     = 21,
		i64x2     = 22,
		i32x4     = 23,
		i16x8     = 24,
		i8x16     = 25,
		f64x2     = 26,
		f32x4     = 27,
		i64x4     = 28,
		i32x8     = 29,
		i16x16    = 30,
		i8x32     = 31,
		f64x4     = 32,
		f32x8     = 33,
		i64x8     = 34,
		i32x16    = 35,
		i16x32    = 36,
		i8x64     = 37,
		f64x8     = 38,
		f32x16    = 39,
		// PSEUDO
		last      = 39,
		bit_width = 6,
	};
	#define RC_VISIT_IR_TYPE(_) _(pack,retro::ir::value_pack_t) _(context,retro::ir::context_t) _(reg,retro::arch::mreg) _(op,retro::ir::op) _(intrinsic,retro::ir::intrinsic) _(label,basic_block*) _(str,std::string_view) _(i1,bool) _(i8,i8) _(i16,i16) _(i32,i32) _(i64,i64) _(i128,i128) _(f32,f32) _(f64,f64) _(f80,f80) _(pointer,retro::ir::pointer) _(i32x2,i32x2) _(i16x4,i16x4) _(i8x8,i8x8) _(f32x2,f32x2) _(i64x2,i64x2) _(i32x4,i32x4) _(i16x8,i16x8) _(i8x16,i8x16) _(f64x2,f64x2) _(f32x4,f32x4) _(i64x4,i64x4) _(i32x8,i32x8) _(i16x16,i16x16) _(i8x32,i8x32) _(f64x4,f64x4) _(f32x8,f32x8) _(i64x8,i64x8) _(i32x16,i32x16) _(i16x32,i16x32) _(i8x64,i8x64) _(f64x8,f64x8) _(f32x16,f32x16)
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//                                                      Descriptors                                                      //
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	struct type_kind_desc {
		std::string_view name = {};
	
		using value_type = type_kind;
		static constexpr std::span<const type_kind_desc> all();
		RC_INLINE constexpr const type_kind id() const { return type_kind(this - all().data()); }
	};
	struct type_desc {
		std::string_view name           = {};
		u16              bit_size : 10  = 0;
		u8               pseudo : 1     = 0;
		type_kind        kind : 3       = {};
		type             underlying : 6 = {};
		u8               lane_width : 7 = 0;
	
		using value_type = type;
		static constexpr std::span<const type_desc> all();
		RC_INLINE constexpr const type id() const { return type(this - all().data()); }
	};
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//                                                         Tables                                                         //
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	inline constexpr type_kind_desc type_kinds[] = {
		{"none"},
		{"memory"},
		{"scalar-int"},
		{"scalar-fp"},
		{"vector-int"},
		{"vector-fp"},
	};
	RC_INLINE constexpr std::span<const type_kind_desc> type_kind_desc::all() { return type_kinds; }
	inline constexpr type_desc types[] = {
		{"none"},
		{"pack",0,1,type_kind::none,type::none,0},
		{"context",0,1,type_kind::none,type::none,0},
		{"reg",0,0,type_kind::none,type::none,0},
		{"op",0,0,type_kind::none,type::none,0},
		{"intrinsic",0,0,type_kind::none,type::none,0},
		{"label",0,1,type_kind::none,type::none,0},
		{"str",0,0,type_kind::none,type::none,0},
		{"i1",1,0,type_kind::scalar_int,type::none,0},
		{"i8",8,0,type_kind::scalar_int,type::none,0},
		{"i16",16,0,type_kind::scalar_int,type::none,0},
		{"i32",32,0,type_kind::scalar_int,type::none,0},
		{"i64",64,0,type_kind::scalar_int,type::none,0},
		{"i128",128,0,type_kind::scalar_int,type::none,0},
		{"f32",32,0,type_kind::scalar_fp,type::none,0},
		{"f64",64,0,type_kind::scalar_fp,type::none,0},
		{"f80",80,0,type_kind::scalar_fp,type::none,0},
		{"pointer",64,0,type_kind::memory,type::none,0},
		{"i32x2",64,0,type_kind::vector_int,type::i32,2},
		{"i16x4",64,0,type_kind::vector_int,type::i16,4},
		{"i8x8",64,0,type_kind::vector_int,type::i8,8},
		{"f32x2",64,0,type_kind::vector_fp,type::f32,2},
		{"i64x2",128,0,type_kind::vector_int,type::i64,2},
		{"i32x4",128,0,type_kind::vector_int,type::i32,4},
		{"i16x8",128,0,type_kind::vector_int,type::i16,8},
		{"i8x16",128,0,type_kind::vector_int,type::i8,16},
		{"f64x2",128,0,type_kind::vector_fp,type::f64,2},
		{"f32x4",128,0,type_kind::vector_fp,type::f32,4},
		{"i64x4",256,0,type_kind::vector_int,type::i64,4},
		{"i32x8",256,0,type_kind::vector_int,type::i32,8},
		{"i16x16",256,0,type_kind::vector_int,type::i16,16},
		{"i8x32",256,0,type_kind::vector_int,type::i8,32},
		{"f64x4",256,0,type_kind::vector_fp,type::f64,4},
		{"f32x8",256,0,type_kind::vector_fp,type::f32,8},
		{"i64x8",512,0,type_kind::vector_int,type::i64,8},
		{"i32x16",512,0,type_kind::vector_int,type::i32,16},
		{"i16x32",512,0,type_kind::vector_int,type::i16,32},
		{"i8x64",512,0,type_kind::vector_int,type::i8,64},
		{"f64x8",512,0,type_kind::vector_fp,type::f64,8},
		{"f32x16",512,0,type_kind::vector_fp,type::f32,16},
	};
	RC_INLINE constexpr std::span<const type_desc> type_desc::all() { return types; }
};
namespace retro { template<> struct descriptor<retro::ir::type_kind> { using type = retro::ir::type_kind_desc; }; };
RC_DEFINE_STD_VISITOR_FOR(retro::ir::type_kind, RC_VISIT_IR_TYPE_KIND)
namespace retro { template<> struct descriptor<retro::ir::type> { using type = retro::ir::type_desc; }; };
RC_DEFINE_STD_VISITOR_FOR(retro::ir::type, RC_VISIT_IR_TYPE)
// clang-format on