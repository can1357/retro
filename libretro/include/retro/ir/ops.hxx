#pragma once
#include <retro/common.hpp>
#include <retro/ir/builtin_types.hxx>

// clang-format off
namespace retro::ir {
	enum class op_kind : u8 /*:4*/ {
		none            = 0,
		unary_signed    = 1,
		unary_unsigned  = 2,
		unary_bitwise   = 3,
		unary_float     = 4,
		binary_signed   = 5,
		binary_unsigned = 6,
		binary_bitwise  = 7,
		binary_float    = 8,
		cmp_signed      = 9,
		cmp_unsigned    = 10,
		// PSEUDO
		last            = 10,
		bit_width       = 4,
	};
	#define RC_VISIT_IR_OP_KIND(_) _(unary_signed) _(unary_unsigned) _(unary_bitwise) _(unary_float) _(binary_signed) _(binary_unsigned) _(binary_bitwise) _(binary_float) _(cmp_signed) _(cmp_unsigned)
	enum class op : u8 /*:6*/ {
		none         = 0,
		neg          = 1,
		abs          = 2,
		bit_lsb      = 3,
		bit_msb      = 4,
		bit_popcnt   = 5,
		bit_byteswap = 6,
		bit_not      = 7,
		ceil         = 8,
		floor        = 9,
		trunc        = 10,
		round        = 11,
		sin          = 12,
		cos          = 13,
		tan          = 14,
		asin         = 15,
		acos         = 16,
		atan         = 17,
		sqrt         = 18,
		exp          = 19,
		log          = 20,
		atan2        = 21,
		pow          = 22,
		add          = 23,
		sub          = 24,
		mul          = 25,
		div          = 26,
		udiv         = 27,
		rem          = 28,
		urem         = 29,
		bit_or       = 30,
		bit_and      = 31,
		bit_xor      = 32,
		bit_shl      = 33,
		bit_shr      = 34,
		bit_sar      = 35,
		bit_rol      = 36,
		bit_ror      = 37,
		max          = 38,
		umax         = 39,
		min          = 40,
		umin         = 41,
		gt           = 42,
		ugt          = 43,
		le           = 44,
		ule          = 45,
		ge           = 46,
		uge          = 47,
		lt           = 48,
		ult          = 49,
		eq           = 50,
		ne           = 51,
		// PSEUDO
		last         = 51,
		bit_width    = 6,
	};
	#define RC_VISIT_IR_OP(_) _(neg) _(abs) _(bit_lsb) _(bit_msb) _(bit_popcnt) _(bit_byteswap) _(bit_not) _(ceil) _(floor) _(trunc) _(round) _(sin) _(cos) _(tan) _(asin) _(acos) _(atan) _(sqrt) _(exp) _(log) _(atan2) _(pow) _(add) _(sub) _(mul) _(div) _(udiv) _(rem) _(urem) _(bit_or) _(bit_and) _(bit_xor) _(bit_shl) _(bit_shr) _(bit_sar) _(bit_rol) _(bit_ror) _(max) _(umax) _(min) _(umin) _(gt) _(ugt) _(le) _(ule) _(ge) _(uge) _(lt) _(ult) _(eq) _(ne)
	enum class intrinsic : u8 /*:7*/ {
		none                = 0,
		alloc_stack         = 1,
		malloc              = 2,
		realloc             = 3,
		free                = 4,
		sfence              = 5,
		lfence              = 6,
		mfence              = 7,
		memcpy              = 8,
		memmove             = 9,
		memcmp              = 10,
		memset              = 11,
		memset16            = 12,
		memset32            = 13,
		memset64            = 14,
		memchr              = 15,
		strcpy              = 16,
		strcmp              = 17,
		strchr              = 18,
		strstr              = 19,
		loadnontemporal8    = 20,
		loadnontemporal16   = 21,
		loadnontemporal32   = 22,
		loadnontemporal64   = 23,
		loadnontemporal128  = 24,
		loadnontemporal256  = 25,
		loadnontemporal512  = 26,
		storenontemporal8   = 27,
		storenontemporal16  = 28,
		storenontemporal32  = 29,
		storenontemporal64  = 30,
		storenontemporal128 = 31,
		storenontemporal256 = 32,
		storenontemporal512 = 33,
		readcyclecounter    = 34,
		prefetch            = 35,
		ia32_rdgsbase32     = 36,
		ia32_rdfsbase32     = 37,
		ia32_rdgsbase64     = 38,
		ia32_rdfsbase64     = 39,
		ia32_wrgsbase32     = 40,
		ia32_wrfsbase32     = 41,
		ia32_wrgsbase64     = 42,
		ia32_wrfsbase64     = 43,
		ia32_swapgs         = 44,
		ia32_stmxcsr        = 45,
		ia32_ldmxcsr        = 46,
		ia32_pause          = 47,
		ia32_hlt            = 48,
		ia32_rsm            = 49,
		ia32_invd           = 50,
		ia32_wbinvd         = 51,
		ia32_readpid        = 52,
		ia32_cpuid          = 53,
		ia32_xgetbv         = 54,
		ia32_rdpmc          = 55,
		ia32_xsetbv         = 56,
		ia32_rdmsr          = 57,
		ia32_wrmsr          = 58,
		ia32_invlpg         = 59,
		ia32_invpcid        = 60,
		ia32_prefetcht0     = 61,
		ia32_prefetcht1     = 62,
		ia32_prefetcht2     = 63,
		ia32_prefetchnta    = 64,
		ia32_prefetchw      = 65,
		ia32_prefetchwt1    = 66,
		ia32_cldemote       = 67,
		ia32_clflush        = 68,
		ia32_clflushopt     = 69,
		ia32_clwb           = 70,
		ia32_clzero         = 71,
		ia32_outb           = 72,
		ia32_outw           = 73,
		ia32_outd           = 74,
		ia32_inb            = 75,
		ia32_inw            = 76,
		ia32_ind            = 77,
		ia32_fxrstor        = 78,
		ia32_fxrstor64      = 79,
		ia32_fxsave         = 80,
		ia32_fxsave64       = 81,
		ia32_xrstor         = 82,
		ia32_xrstor64       = 83,
		ia32_xrstors        = 84,
		ia32_xrstors64      = 85,
		ia32_xsave          = 86,
		ia32_xsavec         = 87,
		ia32_xsaveopt       = 88,
		ia32_xsaves         = 89,
		ia32_xsave64        = 90,
		ia32_xsavec64       = 91,
		ia32_xsaveopt64     = 92,
		ia32_xsaves64       = 93,
		ia32_sgdt           = 94,
		ia32_sidt           = 95,
		ia32_sldt           = 96,
		ia32_smsw           = 97,
		ia32_str            = 98,
		ia32_lgdt           = 99,
		ia32_lidt           = 100,
		ia32_lldt           = 101,
		ia32_ltr            = 102,
		ia32_lmsw           = 103,
		ia32_getes          = 104,
		ia32_getcs          = 105,
		ia32_getss          = 106,
		ia32_getds          = 107,
		ia32_getfs          = 108,
		ia32_getgs          = 109,
		ia32_setes          = 110,
		ia32_setcs          = 111,
		ia32_setss          = 112,
		ia32_setds          = 113,
		ia32_setfs          = 114,
		ia32_setgs          = 115,
		retaddr             = 116,
		addr_retaddr        = 117,
		// PSEUDO
		last                = 117,
		bit_width           = 7,
	};
	#define RC_VISIT_IR_INTRINSIC(_) _(alloc_stack) _(malloc) _(realloc) _(free) _(sfence) _(lfence) _(mfence) _(memcpy) _(memmove) _(memcmp) _(memset) _(memset16) _(memset32) _(memset64) _(memchr) _(strcpy) _(strcmp) _(strchr) _(strstr) _(loadnontemporal8) _(loadnontemporal16) _(loadnontemporal32) _(loadnontemporal64) _(loadnontemporal128) _(loadnontemporal256) _(loadnontemporal512) _(storenontemporal8) _(storenontemporal16) _(storenontemporal32) _(storenontemporal64) _(storenontemporal128) _(storenontemporal256) _(storenontemporal512) _(readcyclecounter) _(prefetch) _(ia32_rdgsbase32) _(ia32_rdfsbase32) _(ia32_rdgsbase64) _(ia32_rdfsbase64) _(ia32_wrgsbase32) _(ia32_wrfsbase32) _(ia32_wrgsbase64) _(ia32_wrfsbase64) _(ia32_swapgs) _(ia32_stmxcsr) _(ia32_ldmxcsr) _(ia32_pause) _(ia32_hlt) _(ia32_rsm) _(ia32_invd) _(ia32_wbinvd) _(ia32_readpid) _(ia32_cpuid) _(ia32_xgetbv) _(ia32_rdpmc) _(ia32_xsetbv) _(ia32_rdmsr) _(ia32_wrmsr) _(ia32_invlpg) _(ia32_invpcid) _(ia32_prefetcht0) _(ia32_prefetcht1) _(ia32_prefetcht2) _(ia32_prefetchnta) _(ia32_prefetchw) _(ia32_prefetchwt1) _(ia32_cldemote) _(ia32_clflush) _(ia32_clflushopt) _(ia32_clwb) _(ia32_clzero) _(ia32_outb) _(ia32_outw) _(ia32_outd) _(ia32_inb) _(ia32_inw) _(ia32_ind) _(ia32_fxrstor) _(ia32_fxrstor64) _(ia32_fxsave) _(ia32_fxsave64) _(ia32_xrstor) _(ia32_xrstor64) _(ia32_xrstors) _(ia32_xrstors64) _(ia32_xsave) _(ia32_xsavec) _(ia32_xsaveopt) _(ia32_xsaves) _(ia32_xsave64) _(ia32_xsavec64) _(ia32_xsaveopt64) _(ia32_xsaves64) _(ia32_sgdt) _(ia32_sidt) _(ia32_sldt) _(ia32_smsw) _(ia32_str) _(ia32_lgdt) _(ia32_lidt) _(ia32_lldt) _(ia32_ltr) _(ia32_lmsw) _(ia32_getes) _(ia32_getcs) _(ia32_getss) _(ia32_getds) _(ia32_getfs) _(ia32_getgs) _(ia32_setes) _(ia32_setcs) _(ia32_setss) _(ia32_setds) _(ia32_setfs) _(ia32_setgs) _(retaddr) _(addr_retaddr)
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//                                                      Descriptors                                                      //
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	struct op_kind_desc {
		std::string_view name = {};
	
		using value_type = op_kind;
		static constexpr std::span<const op_kind_desc> all();
		RC_INLINE constexpr const op_kind id() const { return op_kind(this - all().data()); }
	};
	struct op_desc {
		std::string_view name            = {};
		std::string_view symbol          = {};
		u8               commutative : 1 = 0;
		op_kind          kind : 4        = {};
		op               inverse : 6     = {};
		op               swap_sign : 6   = {};
	
		using value_type = op;
		static constexpr std::span<const op_desc> all();
		RC_INLINE constexpr const op id() const { return op(this - all().data()); }
	};
	struct intrinsic_desc {
		std::string_view  name   = {};
		std::string_view  symbol = {};
		small_array<type> args; 
		type              ret    = {};
	
		using value_type = intrinsic;
		static constexpr std::span<const intrinsic_desc> all();
		RC_INLINE constexpr const intrinsic id() const { return intrinsic(this - all().data()); }
	};
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//                                                         Tables                                                         //
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	inline constexpr op_kind_desc op_kinds[] = {
		{"none"},
		{"unary-signed"},
		{"unary-unsigned"},
		{"unary-bitwise"},
		{"unary-float"},
		{"binary-signed"},
		{"binary-unsigned"},
		{"binary-bitwise"},
		{"binary-float"},
		{"cmp-signed"},
		{"cmp-unsigned"},
	};
	RC_INLINE constexpr std::span<const op_kind_desc> op_kind_desc::all() { return op_kinds; }
	inline constexpr op_desc ops[] = {
		{"none"},
		{"neg","-",0,op_kind::unary_signed,op::neg,op::none},
		{"abs","abs",0,op_kind::unary_signed,op::none,op::none},
		{"bit_lsb","lsb",0,op_kind::unary_bitwise,op::none,op::none},
		{"bit_msb","msb",0,op_kind::unary_bitwise,op::none,op::none},
		{"bit_popcnt","popcnt",0,op_kind::unary_bitwise,op::none,op::none},
		{"bit_byteswap","bswap",0,op_kind::unary_bitwise,op::none,op::none},
		{"bit_not","~",0,op_kind::unary_bitwise,op::bit_not,op::none},
		{"ceil","ceil",0,op_kind::unary_float,op::none,op::none},
		{"floor","floor",0,op_kind::unary_float,op::none,op::none},
		{"trunc","trunc",0,op_kind::unary_float,op::none,op::none},
		{"round","round",0,op_kind::unary_float,op::none,op::none},
		{"sin","sin",0,op_kind::unary_float,op::none,op::none},
		{"cos","cos",0,op_kind::unary_float,op::none,op::none},
		{"tan","tan",0,op_kind::unary_float,op::none,op::none},
		{"asin","asin",0,op_kind::unary_float,op::none,op::none},
		{"acos","acos",0,op_kind::unary_float,op::none,op::none},
		{"atan","atan",0,op_kind::unary_float,op::none,op::none},
		{"sqrt","sqrt",0,op_kind::unary_float,op::none,op::none},
		{"exp","exp",0,op_kind::unary_float,op::log,op::none},
		{"log","log",0,op_kind::unary_float,op::exp,op::none},
		{"atan2","atan2",0,op_kind::binary_float,op::none,op::none},
		{"pow","pow",0,op_kind::binary_float,op::none,op::none},
		{"add","+",1,op_kind::binary_signed,op::none,op::none},
		{"sub","-",0,op_kind::binary_signed,op::none,op::none},
		{"mul","*",1,op_kind::binary_signed,op::none,op::none},
		{"div","/",0,op_kind::binary_signed,op::none,op::udiv},
		{"udiv","u/",0,op_kind::binary_unsigned,op::none,op::div},
		{"rem","%",0,op_kind::binary_signed,op::none,op::urem},
		{"urem","u%",0,op_kind::binary_unsigned,op::none,op::rem},
		{"bit_or","|",1,op_kind::binary_bitwise,op::none,op::none},
		{"bit_and","&",1,op_kind::binary_bitwise,op::none,op::none},
		{"bit_xor","^",1,op_kind::binary_bitwise,op::bit_xor,op::none},
		{"bit_shl","<<",0,op_kind::binary_bitwise,op::none,op::none},
		{"bit_shr",">>",0,op_kind::binary_bitwise,op::none,op::bit_sar},
		{"bit_sar","s>>",0,op_kind::binary_bitwise,op::none,op::bit_shr},
		{"bit_rol","rotl",0,op_kind::binary_bitwise,op::bit_ror,op::none},
		{"bit_ror","rotr",0,op_kind::binary_bitwise,op::bit_rol,op::none},
		{"max","max",1,op_kind::binary_signed,op::none,op::umax},
		{"umax","umax",1,op_kind::binary_unsigned,op::none,op::max},
		{"min","min",1,op_kind::binary_signed,op::none,op::umin},
		{"umin","umin",1,op_kind::binary_unsigned,op::none,op::min},
		{"gt",">",0,op_kind::cmp_signed,op::le,op::ugt},
		{"ugt","u>",0,op_kind::cmp_unsigned,op::ule,op::gt},
		{"le","<=",0,op_kind::cmp_signed,op::gt,op::ule},
		{"ule","u<=",0,op_kind::cmp_unsigned,op::ugt,op::le},
		{"ge",">=",0,op_kind::cmp_signed,op::lt,op::uge},
		{"uge","u>=",0,op_kind::cmp_unsigned,op::ult,op::ge},
		{"lt","<",0,op_kind::cmp_signed,op::ge,op::ult},
		{"ult","u<",0,op_kind::cmp_unsigned,op::uge,op::lt},
		{"eq","==",1,op_kind::cmp_signed,op::ne,op::none},
		{"ne","!=",1,op_kind::cmp_signed,op::eq,op::none},
	};
	RC_INLINE constexpr std::span<const op_desc> op_desc::all() { return ops; }
	inline constexpr intrinsic_desc intrinsics[] = {
		{"none"},
		{"alloc-stack","alloca",{type::i32},type::pointer},
		{"malloc",{},{type::i64},type::pointer},
		{"realloc",{},{type::pointer,type::i64},type::pointer},
		{"free",{},{type::pointer},type::none},
		{"sfence",{},{},type::none},
		{"lfence",{},{},type::none},
		{"mfence",{},{},type::none},
		{"memcpy",{},{type::pointer,type::pointer,type::i64},type::pointer},
		{"memmove",{},{type::pointer,type::pointer,type::i64},type::pointer},
		{"memcmp",{},{type::pointer,type::pointer,type::i64},type::i32},
		{"memset",{},{type::pointer,type::i32,type::i64},type::pointer},
		{"memset16",{},{type::pointer,type::i32,type::i64},type::pointer},
		{"memset32",{},{type::pointer,type::i32,type::i64},type::pointer},
		{"memset64",{},{type::pointer,type::i64,type::i64},type::pointer},
		{"memchr",{},{type::pointer,type::i32,type::i64},type::pointer},
		{"strcpy",{},{type::pointer,type::pointer},type::pointer},
		{"strcmp",{},{type::pointer,type::pointer},type::i32},
		{"strchr",{},{type::pointer,type::i32},type::pointer},
		{"strstr",{},{type::pointer,type::pointer},type::pointer},
		{"loadnontemporal8","__builtin_load_nontemporal8",{type::pointer},type::i8},
		{"loadnontemporal16","__builtin_load_nontemporal16",{type::pointer},type::i16},
		{"loadnontemporal32","__builtin_load_nontemporal32",{type::pointer},type::i32},
		{"loadnontemporal64","__builtin_load_nontemporal64",{type::pointer},type::i64},
		{"loadnontemporal128","__builtin_load_nontemporal128",{type::pointer},type::i32x4},
		{"loadnontemporal256","__builtin_load_nontemporal256",{type::pointer},type::i32x8},
		{"loadnontemporal512","__builtin_load_nontemporal512",{type::pointer},type::i32x16},
		{"storenontemporal8","__builtin_store_nontemporal8",{type::pointer,type::i8},type::none},
		{"storenontemporal16","__builtin_store_nontemporal16",{type::pointer,type::i16},type::none},
		{"storenontemporal32","__builtin_store_nontemporal32",{type::pointer,type::i32},type::none},
		{"storenontemporal64","__builtin_store_nontemporal64",{type::pointer,type::i64},type::none},
		{"storenontemporal128","__builtin_store_nontemporal128",{type::pointer,type::i32x4},type::none},
		{"storenontemporal256","__builtin_store_nontemporal256",{type::pointer,type::i32x8},type::none},
		{"storenontemporal512","__builtin_store_nontemporal512",{type::pointer,type::i32x16},type::none},
		{"readcyclecounter","__builtin_readcyclecounter",{},type::i64},
		{"prefetch","__builtin_prefetech",{type::pointer},type::none},
		{"ia32-rdgsbase32","_readgsbase_u32",{},type::i32},
		{"ia32-rdfsbase32","_readfsbase_u32",{},type::i32},
		{"ia32-rdgsbase64","_readgsbase_u64",{},type::i64},
		{"ia32-rdfsbase64","_readfsbase_u64",{},type::i64},
		{"ia32-wrgsbase32","_writegsbase_u32",{type::i32},type::none},
		{"ia32-wrfsbase32","_writefsbase_u32",{type::i32},type::none},
		{"ia32-wrgsbase64","_writegsbase_u64",{type::i64},type::none},
		{"ia32-wrfsbase64","_writefsbase_u64",{type::i64},type::none},
		{"ia32-swapgs","__swapgs",{},type::none},
		{"ia32-stmxcsr","_mm_getcsr",{},type::i32},
		{"ia32-ldmxcsr","_mm_setcsr",{type::i32},type::none},
		{"ia32-pause","_mm_pause",{},type::none},
		{"ia32-hlt","__hlt",{},type::none},
		{"ia32-rsm","__rsm",{},type::none},
		{"ia32-invd","__invd",{},type::none},
		{"ia32-wbinvd","__wbinvd",{},type::none},
		{"ia32-readpid","__builtin_ia32_readpid",{},type::i32},
		{"ia32-cpuid","_cpuid",{type::i32,type::i32},type::i32x4},
		{"ia32-xgetbv","_xgetbv",{type::i32},type::i64},
		{"ia32-rdpmc","_rdpmc",{type::i32},type::i64},
		{"ia32-xsetbv","_xgetbv",{type::i32,type::i64},type::none},
		{"ia32-rdmsr","__rdmsr",{type::i32},type::i64},
		{"ia32-wrmsr","__wrmsr",{type::i32,type::i64},type::none},
		{"ia32-invlpg","__invlpg",{type::pointer},type::none},
		{"ia32-invpcid","__invvpcid",{type::i32,type::pointer},type::none},
		{"ia32-prefetcht0","_m_prefetcht0",{type::pointer},type::none},
		{"ia32-prefetcht1","_m_prefetcht1",{type::pointer},type::none},
		{"ia32-prefetcht2","_m_prefetcht2",{type::pointer},type::none},
		{"ia32-prefetchnta","_m_prefetchnta",{type::pointer},type::none},
		{"ia32-prefetchw","_m_prefetchw",{type::pointer},type::none},
		{"ia32-prefetchwt1","_m_prefetchwt1",{type::pointer},type::none},
		{"ia32-cldemote","_cldemote",{type::pointer},type::none},
		{"ia32-clflush","_mm_clflush",{type::pointer},type::none},
		{"ia32-clflushopt","_mm_clflushopt",{type::pointer},type::none},
		{"ia32-clwb","_mm_clwb",{type::pointer},type::none},
		{"ia32-clzero","_mm_clzero",{type::pointer},type::none},
		{"ia32-outb","__outb",{type::i16,type::i8},type::none},
		{"ia32-outw","__outw",{type::i16,type::i16},type::none},
		{"ia32-outd","__outd",{type::i16,type::i32},type::none},
		{"ia32-inb","__inb",{type::i16},type::i8},
		{"ia32-inw","__inw",{type::i16},type::i16},
		{"ia32-ind","__inl",{type::i16},type::i32},
		{"ia32-fxrstor","_fxrstor",{type::pointer},type::none},
		{"ia32-fxrstor64","_fxrstor64",{type::pointer},type::none},
		{"ia32-fxsave","_fxsave",{type::pointer},type::none},
		{"ia32-fxsave64","_fxsave64",{type::pointer},type::none},
		{"ia32-xrstor","_xrstor",{type::pointer,type::i64},type::none},
		{"ia32-xrstor64","_xrstor64",{type::pointer,type::i64},type::none},
		{"ia32-xrstors","_xrstors",{type::pointer,type::i64},type::none},
		{"ia32-xrstors64","_xrstors64",{type::pointer,type::i64},type::none},
		{"ia32-xsave","_xsave",{type::pointer,type::i64},type::none},
		{"ia32-xsavec","_xsavec",{type::pointer,type::i64},type::none},
		{"ia32-xsaveopt","_xsaveopt",{type::pointer,type::i64},type::none},
		{"ia32-xsaves","_xsaves64",{type::pointer,type::i64},type::none},
		{"ia32-xsave64","_xsave64",{type::pointer,type::i64},type::none},
		{"ia32-xsavec64","_xsavec64",{type::pointer,type::i64},type::none},
		{"ia32-xsaveopt64","_xsaveopt64",{type::pointer,type::i64},type::none},
		{"ia32-xsaves64","_xsaves64",{type::pointer,type::i64},type::none},
		{"ia32-sgdt","__sgdt",{type::pointer},type::none},
		{"ia32-sidt","__sidt",{type::pointer},type::none},
		{"ia32-sldt","__sldt",{},type::i16},
		{"ia32-smsw","__smsw",{},type::i16},
		{"ia32-str","__str",{},type::i16},
		{"ia32-lgdt","__lgdt",{type::pointer},type::none},
		{"ia32-lidt","__lidt",{type::pointer},type::none},
		{"ia32-lldt","__lldt",{type::i16},type::none},
		{"ia32-ltr","__ltr",{type::i16},type::none},
		{"ia32-lmsw","__lmsw",{type::i16},type::none},
		{"ia32-getes","_getes",{},type::i16},
		{"ia32-getcs","_getcs",{},type::i16},
		{"ia32-getss","_getss",{},type::i16},
		{"ia32-getds","_getds",{},type::i16},
		{"ia32-getfs","_getfs",{},type::i16},
		{"ia32-getgs","_getgs",{},type::i16},
		{"ia32-setes","_setes",{type::i16},type::none},
		{"ia32-setcs","_setcs",{type::i16},type::none},
		{"ia32-setss","_setss",{type::i16},type::none},
		{"ia32-setds","_setds",{type::i16},type::none},
		{"ia32-setfs","_setfs",{type::i16},type::none},
		{"ia32-setgs","_setgs",{type::i16},type::none},
		{"retaddr","_ReturnAddress",{},type::pointer},
		{"addr-retaddr","_AddressOfReturnAddress",{},type::pointer},
	};
	RC_INLINE constexpr std::span<const intrinsic_desc> intrinsic_desc::all() { return intrinsics; }
};
namespace retro { template<> struct descriptor<retro::ir::op_kind> { using type = retro::ir::op_kind_desc; }; };
RC_DEFINE_STD_VISITOR_FOR(retro::ir::op_kind, RC_VISIT_IR_OP_KIND)
namespace retro { template<> struct descriptor<retro::ir::op> { using type = retro::ir::op_desc; }; };
RC_DEFINE_STD_VISITOR_FOR(retro::ir::op, RC_VISIT_IR_OP)
namespace retro { template<> struct descriptor<retro::ir::intrinsic> { using type = retro::ir::intrinsic_desc; }; };
RC_DEFINE_STD_VISITOR_FOR(retro::ir::intrinsic, RC_VISIT_IR_INTRINSIC)
// clang-format on