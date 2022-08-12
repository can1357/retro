#pragma once
#include <retro/common.hpp>
#include <retro/arch/reg_kind.hxx>

// clang-format off
namespace retro::arch::x86 {
	enum class reg : u8 /*:8*/ {
		none       = 0,
		al         = 1,
		cl         = 2,
		dl         = 3,
		bl         = 4,
		ah         = 5,
		ch         = 6,
		dh         = 7,
		bh         = 8,
		spl        = 9,
		bpl        = 10,
		sil        = 11,
		dil        = 12,
		r8b        = 13,
		r9b        = 14,
		r10b       = 15,
		r11b       = 16,
		r12b       = 17,
		r13b       = 18,
		r14b       = 19,
		r15b       = 20,
		ax         = 21,
		cx         = 22,
		dx         = 23,
		bx         = 24,
		sp         = 25,
		bp         = 26,
		si         = 27,
		di         = 28,
		r8w        = 29,
		r9w        = 30,
		r10w       = 31,
		r11w       = 32,
		r12w       = 33,
		r13w       = 34,
		r14w       = 35,
		r15w       = 36,
		eax        = 37,
		ecx        = 38,
		edx        = 39,
		ebx        = 40,
		esp        = 41,
		ebp        = 42,
		esi        = 43,
		edi        = 44,
		r8d        = 45,
		r9d        = 46,
		r10d       = 47,
		r11d       = 48,
		r12d       = 49,
		r13d       = 50,
		r14d       = 51,
		r15d       = 52,
		rax        = 53,
		rcx        = 54,
		rdx        = 55,
		rbx        = 56,
		rsp        = 57,
		rbp        = 58,
		rsi        = 59,
		rdi        = 60,
		r8         = 61,
		r9         = 62,
		r10        = 63,
		r11        = 64,
		r12        = 65,
		r13        = 66,
		r14        = 67,
		r15        = 68,
		st0        = 69,
		st1        = 70,
		st2        = 71,
		st3        = 72,
		st4        = 73,
		st5        = 74,
		st6        = 75,
		st7        = 76,
		mm0        = 77,
		mm1        = 78,
		mm2        = 79,
		mm3        = 80,
		mm4        = 81,
		mm5        = 82,
		mm6        = 83,
		mm7        = 84,
		x87control = 85,
		x87status  = 86,
		x87tag     = 87,
		flags      = 88,
		eflags     = 89,
		rflags     = 90,
		ip         = 91,
		eip        = 92,
		rip        = 93,
		es         = 94,
		cs         = 95,
		ss         = 96,
		ds         = 97,
		fs         = 98,
		gs         = 99,
		cr0        = 100,
		cr2        = 101,
		cr3        = 102,
		cr4        = 103,
		cr8        = 104,
		dr0        = 105,
		dr1        = 106,
		dr2        = 107,
		dr3        = 108,
		dr4        = 109,
		dr5        = 110,
		dr6        = 111,
		dr7        = 112,
		flag_cf    = 113,
		flag_pf    = 114,
		flag_af    = 115,
		flag_zf    = 116,
		flag_sf    = 117,
		flag_tf    = 118,
		flag_if    = 119,
		flag_df    = 120,
		flag_of    = 121,
		flag_iopl  = 122,
		flag_nt    = 123,
		flag_rf    = 124,
		flag_vm    = 125,
		flag_ac    = 126,
		flag_vif   = 127,
		flag_vip   = 128,
		flag_id    = 129,
		xmm0s      = 130,
		xmm1s      = 131,
		xmm2s      = 132,
		xmm3s      = 133,
		xmm4s      = 134,
		xmm5s      = 135,
		xmm6s      = 136,
		xmm7s      = 137,
		xmm8s      = 138,
		xmm9s      = 139,
		xmm10s     = 140,
		xmm11s     = 141,
		xmm12s     = 142,
		xmm13s     = 143,
		xmm14s     = 144,
		xmm15s     = 145,
		xmm0d      = 146,
		xmm1d      = 147,
		xmm2d      = 148,
		xmm3d      = 149,
		xmm4d      = 150,
		xmm5d      = 151,
		xmm6d      = 152,
		xmm7d      = 153,
		xmm8d      = 154,
		xmm9d      = 155,
		xmm10d     = 156,
		xmm11d     = 157,
		xmm12d     = 158,
		xmm13d     = 159,
		xmm14d     = 160,
		xmm15d     = 161,
		xmm0       = 162,
		xmm1       = 163,
		xmm2       = 164,
		xmm3       = 165,
		xmm4       = 166,
		xmm5       = 167,
		xmm6       = 168,
		xmm7       = 169,
		xmm8       = 170,
		xmm9       = 171,
		xmm10      = 172,
		xmm11      = 173,
		xmm12      = 174,
		xmm13      = 175,
		xmm14      = 176,
		xmm15      = 177,
		ymm0       = 178,
		ymm1       = 179,
		ymm2       = 180,
		ymm3       = 181,
		ymm4       = 182,
		ymm5       = 183,
		ymm6       = 184,
		ymm7       = 185,
		ymm8       = 186,
		ymm9       = 187,
		ymm10      = 188,
		ymm11      = 189,
		ymm12      = 190,
		ymm13      = 191,
		ymm14      = 192,
		ymm15      = 193,
		// PSEUDO
		last       = 193,
		bit_width  = 8,
	};
	#define RC_VISIT_ARCH_X86_REG(_) _(al,ZYDIS_REGISTER_AL) _(cl,ZYDIS_REGISTER_CL) _(dl,ZYDIS_REGISTER_DL) _(bl,ZYDIS_REGISTER_BL) _(ah,ZYDIS_REGISTER_AH) _(ch,ZYDIS_REGISTER_CH) _(dh,ZYDIS_REGISTER_DH) _(bh,ZYDIS_REGISTER_BH) _(spl,ZYDIS_REGISTER_SPL) _(bpl,ZYDIS_REGISTER_BPL) _(sil,ZYDIS_REGISTER_SIL) _(dil,ZYDIS_REGISTER_DIL) _(r8b,ZYDIS_REGISTER_R8B) _(r9b,ZYDIS_REGISTER_R9B) _(r10b,ZYDIS_REGISTER_R10B) _(r11b,ZYDIS_REGISTER_R11B) _(r12b,ZYDIS_REGISTER_R12B) _(r13b,ZYDIS_REGISTER_R13B) _(r14b,ZYDIS_REGISTER_R14B) _(r15b,ZYDIS_REGISTER_R15B) _(ax,ZYDIS_REGISTER_AX) _(cx,ZYDIS_REGISTER_CX) _(dx,ZYDIS_REGISTER_DX) _(bx,ZYDIS_REGISTER_BX) _(sp,ZYDIS_REGISTER_SP) _(bp,ZYDIS_REGISTER_BP) _(si,ZYDIS_REGISTER_SI) _(di,ZYDIS_REGISTER_DI) _(r8w,ZYDIS_REGISTER_R8W) _(r9w,ZYDIS_REGISTER_R9W) _(r10w,ZYDIS_REGISTER_R10W) _(r11w,ZYDIS_REGISTER_R11W) _(r12w,ZYDIS_REGISTER_R12W) _(r13w,ZYDIS_REGISTER_R13W) _(r14w,ZYDIS_REGISTER_R14W) _(r15w,ZYDIS_REGISTER_R15W) _(eax,ZYDIS_REGISTER_EAX) _(ecx,ZYDIS_REGISTER_ECX) _(edx,ZYDIS_REGISTER_EDX) _(ebx,ZYDIS_REGISTER_EBX) _(esp,ZYDIS_REGISTER_ESP) _(ebp,ZYDIS_REGISTER_EBP) _(esi,ZYDIS_REGISTER_ESI) _(edi,ZYDIS_REGISTER_EDI) _(r8d,ZYDIS_REGISTER_R8D) _(r9d,ZYDIS_REGISTER_R9D) _(r10d,ZYDIS_REGISTER_R10D) _(r11d,ZYDIS_REGISTER_R11D) _(r12d,ZYDIS_REGISTER_R12D) _(r13d,ZYDIS_REGISTER_R13D) _(r14d,ZYDIS_REGISTER_R14D) _(r15d,ZYDIS_REGISTER_R15D) _(rax,ZYDIS_REGISTER_RAX) _(rcx,ZYDIS_REGISTER_RCX) _(rdx,ZYDIS_REGISTER_RDX) _(rbx,ZYDIS_REGISTER_RBX) _(rsp,ZYDIS_REGISTER_RSP) _(rbp,ZYDIS_REGISTER_RBP) _(rsi,ZYDIS_REGISTER_RSI) _(rdi,ZYDIS_REGISTER_RDI) _(r8,ZYDIS_REGISTER_R8) _(r9,ZYDIS_REGISTER_R9) _(r10,ZYDIS_REGISTER_R10) _(r11,ZYDIS_REGISTER_R11) _(r12,ZYDIS_REGISTER_R12) _(r13,ZYDIS_REGISTER_R13) _(r14,ZYDIS_REGISTER_R14) _(r15,ZYDIS_REGISTER_R15) _(st0,ZYDIS_REGISTER_ST0) _(st1,ZYDIS_REGISTER_ST1) _(st2,ZYDIS_REGISTER_ST2) _(st3,ZYDIS_REGISTER_ST3) _(st4,ZYDIS_REGISTER_ST4) _(st5,ZYDIS_REGISTER_ST5) _(st6,ZYDIS_REGISTER_ST6) _(st7,ZYDIS_REGISTER_ST7) _(mm0,ZYDIS_REGISTER_MM0) _(mm1,ZYDIS_REGISTER_MM1) _(mm2,ZYDIS_REGISTER_MM2) _(mm3,ZYDIS_REGISTER_MM3) _(mm4,ZYDIS_REGISTER_MM4) _(mm5,ZYDIS_REGISTER_MM5) _(mm6,ZYDIS_REGISTER_MM6) _(mm7,ZYDIS_REGISTER_MM7) _(x87control,ZYDIS_REGISTER_X87CONTROL) _(x87status,ZYDIS_REGISTER_X87STATUS) _(x87tag,ZYDIS_REGISTER_X87TAG) _(flags,ZYDIS_REGISTER_FLAGS) _(eflags,ZYDIS_REGISTER_EFLAGS) _(rflags,ZYDIS_REGISTER_RFLAGS) _(ip,ZYDIS_REGISTER_IP) _(eip,ZYDIS_REGISTER_EIP) _(rip,ZYDIS_REGISTER_RIP) _(es,ZYDIS_REGISTER_ES) _(cs,ZYDIS_REGISTER_CS) _(ss,ZYDIS_REGISTER_SS) _(ds,ZYDIS_REGISTER_DS) _(fs,ZYDIS_REGISTER_FS) _(gs,ZYDIS_REGISTER_GS) _(cr0,ZYDIS_REGISTER_CR0) _(cr2,ZYDIS_REGISTER_CR2) _(cr3,ZYDIS_REGISTER_CR3) _(cr4,ZYDIS_REGISTER_CR4) _(cr8,ZYDIS_REGISTER_CR8) _(dr0,ZYDIS_REGISTER_DR0) _(dr1,ZYDIS_REGISTER_DR1) _(dr2,ZYDIS_REGISTER_DR2) _(dr3,ZYDIS_REGISTER_DR3) _(dr4,ZYDIS_REGISTER_DR4) _(dr5,ZYDIS_REGISTER_DR5) _(dr6,ZYDIS_REGISTER_DR6) _(dr7,ZYDIS_REGISTER_DR7) _(flag_cf,ZYDIS_REGISTER_NONE) _(flag_pf,ZYDIS_REGISTER_NONE) _(flag_af,ZYDIS_REGISTER_NONE) _(flag_zf,ZYDIS_REGISTER_NONE) _(flag_sf,ZYDIS_REGISTER_NONE) _(flag_tf,ZYDIS_REGISTER_NONE) _(flag_if,ZYDIS_REGISTER_NONE) _(flag_df,ZYDIS_REGISTER_NONE) _(flag_of,ZYDIS_REGISTER_NONE) _(flag_iopl,ZYDIS_REGISTER_NONE) _(flag_nt,ZYDIS_REGISTER_NONE) _(flag_rf,ZYDIS_REGISTER_NONE) _(flag_vm,ZYDIS_REGISTER_NONE) _(flag_ac,ZYDIS_REGISTER_NONE) _(flag_vif,ZYDIS_REGISTER_NONE) _(flag_vip,ZYDIS_REGISTER_NONE) _(flag_id,ZYDIS_REGISTER_NONE) _(xmm0s,ZYDIS_REGISTER_NONE) _(xmm1s,ZYDIS_REGISTER_NONE) _(xmm2s,ZYDIS_REGISTER_NONE) _(xmm3s,ZYDIS_REGISTER_NONE) _(xmm4s,ZYDIS_REGISTER_NONE) _(xmm5s,ZYDIS_REGISTER_NONE) _(xmm6s,ZYDIS_REGISTER_NONE) _(xmm7s,ZYDIS_REGISTER_NONE) _(xmm8s,ZYDIS_REGISTER_NONE) _(xmm9s,ZYDIS_REGISTER_NONE) _(xmm10s,ZYDIS_REGISTER_NONE) _(xmm11s,ZYDIS_REGISTER_NONE) _(xmm12s,ZYDIS_REGISTER_NONE) _(xmm13s,ZYDIS_REGISTER_NONE) _(xmm14s,ZYDIS_REGISTER_NONE) _(xmm15s,ZYDIS_REGISTER_NONE) _(xmm0d,ZYDIS_REGISTER_NONE) _(xmm1d,ZYDIS_REGISTER_NONE) _(xmm2d,ZYDIS_REGISTER_NONE) _(xmm3d,ZYDIS_REGISTER_NONE) _(xmm4d,ZYDIS_REGISTER_NONE) _(xmm5d,ZYDIS_REGISTER_NONE) _(xmm6d,ZYDIS_REGISTER_NONE) _(xmm7d,ZYDIS_REGISTER_NONE) _(xmm8d,ZYDIS_REGISTER_NONE) _(xmm9d,ZYDIS_REGISTER_NONE) _(xmm10d,ZYDIS_REGISTER_NONE) _(xmm11d,ZYDIS_REGISTER_NONE) _(xmm12d,ZYDIS_REGISTER_NONE) _(xmm13d,ZYDIS_REGISTER_NONE) _(xmm14d,ZYDIS_REGISTER_NONE) _(xmm15d,ZYDIS_REGISTER_NONE) _(xmm0,ZYDIS_REGISTER_XMM0) _(xmm1,ZYDIS_REGISTER_XMM1) _(xmm2,ZYDIS_REGISTER_XMM2) _(xmm3,ZYDIS_REGISTER_XMM3) _(xmm4,ZYDIS_REGISTER_XMM4) _(xmm5,ZYDIS_REGISTER_XMM5) _(xmm6,ZYDIS_REGISTER_XMM6) _(xmm7,ZYDIS_REGISTER_XMM7) _(xmm8,ZYDIS_REGISTER_XMM8) _(xmm9,ZYDIS_REGISTER_XMM9) _(xmm10,ZYDIS_REGISTER_XMM10) _(xmm11,ZYDIS_REGISTER_XMM11) _(xmm12,ZYDIS_REGISTER_XMM12) _(xmm13,ZYDIS_REGISTER_XMM13) _(xmm14,ZYDIS_REGISTER_XMM14) _(xmm15,ZYDIS_REGISTER_XMM15) _(ymm0,ZYDIS_REGISTER_YMM0) _(ymm1,ZYDIS_REGISTER_YMM1) _(ymm2,ZYDIS_REGISTER_YMM2) _(ymm3,ZYDIS_REGISTER_YMM3) _(ymm4,ZYDIS_REGISTER_YMM4) _(ymm5,ZYDIS_REGISTER_YMM5) _(ymm6,ZYDIS_REGISTER_YMM6) _(ymm7,ZYDIS_REGISTER_YMM7) _(ymm8,ZYDIS_REGISTER_YMM8) _(ymm9,ZYDIS_REGISTER_YMM9) _(ymm10,ZYDIS_REGISTER_YMM10) _(ymm11,ZYDIS_REGISTER_YMM11) _(ymm12,ZYDIS_REGISTER_YMM12) _(ymm13,ZYDIS_REGISTER_YMM13) _(ymm14,ZYDIS_REGISTER_YMM14) _(ymm15,ZYDIS_REGISTER_YMM15)
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//                                                      Descriptors                                                      //
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	struct reg_desc {
		std::string_view name       = {};
		small_array<reg> parts;    
		reg_kind         kind       = {};
		u16              width : 9  = 0;
		u8               offset : 5 = 0;
		reg              super      = {};
	
		using value_type = reg;
		static constexpr std::span<const reg_desc> all();
		RC_INLINE constexpr const reg id() const { return reg(this - all().data()); }
	};
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//                                                         Tables                                                         //
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	inline constexpr reg_desc regs[] = {
		{"none"},
		{"al",{},reg_kind::gpr8,8,0,reg::rax},
		{"cl",{},reg_kind::gpr8,8,0,reg::rcx},
		{"dl",{},reg_kind::gpr8,8,0,reg::rdx},
		{"bl",{},reg_kind::gpr8,8,0,reg::rbx},
		{"ah",{},reg_kind::gpr8,8,8,reg::rax},
		{"ch",{},reg_kind::gpr8,8,8,reg::rcx},
		{"dh",{},reg_kind::gpr8,8,8,reg::rdx},
		{"bh",{},reg_kind::gpr8,8,8,reg::rbx},
		{"spl",{},reg_kind::gpr8,8,0,reg::rsp},
		{"bpl",{},reg_kind::gpr8,8,0,reg::rbp},
		{"sil",{},reg_kind::gpr8,8,0,reg::rsi},
		{"dil",{},reg_kind::gpr8,8,0,reg::rdi},
		{"r8b",{},reg_kind::gpr8,8,0,reg::r8},
		{"r9b",{},reg_kind::gpr8,8,0,reg::r9},
		{"r10b",{},reg_kind::gpr8,8,0,reg::r10},
		{"r11b",{},reg_kind::gpr8,8,0,reg::r11},
		{"r12b",{},reg_kind::gpr8,8,0,reg::r12},
		{"r13b",{},reg_kind::gpr8,8,0,reg::r13},
		{"r14b",{},reg_kind::gpr8,8,0,reg::r14},
		{"r15b",{},reg_kind::gpr8,8,0,reg::r15},
		{"ax",{},reg_kind::gpr16,16,0,reg::rax},
		{"cx",{},reg_kind::gpr16,16,0,reg::rcx},
		{"dx",{},reg_kind::gpr16,16,0,reg::rdx},
		{"bx",{},reg_kind::gpr16,16,0,reg::rbx},
		{"sp",{},reg_kind::gpr16,16,0,reg::rsp},
		{"bp",{},reg_kind::gpr16,16,0,reg::rbp},
		{"si",{},reg_kind::gpr16,16,0,reg::rsi},
		{"di",{},reg_kind::gpr16,16,0,reg::rdi},
		{"r8w",{},reg_kind::gpr16,16,0,reg::r8},
		{"r9w",{},reg_kind::gpr16,16,0,reg::r9},
		{"r10w",{},reg_kind::gpr16,16,0,reg::r10},
		{"r11w",{},reg_kind::gpr16,16,0,reg::r11},
		{"r12w",{},reg_kind::gpr16,16,0,reg::r12},
		{"r13w",{},reg_kind::gpr16,16,0,reg::r13},
		{"r14w",{},reg_kind::gpr16,16,0,reg::r14},
		{"r15w",{},reg_kind::gpr16,16,0,reg::r15},
		{"eax",{},reg_kind::gpr32,32,0,reg::rax},
		{"ecx",{},reg_kind::gpr32,32,0,reg::rcx},
		{"edx",{},reg_kind::gpr32,32,0,reg::rdx},
		{"ebx",{},reg_kind::gpr32,32,0,reg::rbx},
		{"esp",{},reg_kind::gpr32,32,0,reg::rsp},
		{"ebp",{},reg_kind::gpr32,32,0,reg::rbp},
		{"esi",{},reg_kind::gpr32,32,0,reg::rsi},
		{"edi",{},reg_kind::gpr32,32,0,reg::rdi},
		{"r8d",{},reg_kind::gpr32,32,0,reg::r8},
		{"r9d",{},reg_kind::gpr32,32,0,reg::r9},
		{"r10d",{},reg_kind::gpr32,32,0,reg::r10},
		{"r11d",{},reg_kind::gpr32,32,0,reg::r11},
		{"r12d",{},reg_kind::gpr32,32,0,reg::r12},
		{"r13d",{},reg_kind::gpr32,32,0,reg::r13},
		{"r14d",{},reg_kind::gpr32,32,0,reg::r14},
		{"r15d",{},reg_kind::gpr32,32,0,reg::r15},
		{"rax",{reg::al,reg::rax,reg::ah,reg::ax,reg::eax},reg_kind::gpr64,64,0,reg::none},
		{"rcx",{reg::cl,reg::rcx,reg::ch,reg::cx,reg::ecx},reg_kind::gpr64,64,0,reg::none},
		{"rdx",{reg::dl,reg::rdx,reg::dh,reg::dx,reg::edx},reg_kind::gpr64,64,0,reg::none},
		{"rbx",{reg::bl,reg::rbx,reg::bh,reg::bx,reg::ebx},reg_kind::gpr64,64,0,reg::none},
		{"rsp",{reg::spl,reg::rsp,reg::sp,reg::esp},reg_kind::gpr64,64,0,reg::none},
		{"rbp",{reg::bpl,reg::rbp,reg::bp,reg::ebp},reg_kind::gpr64,64,0,reg::none},
		{"rsi",{reg::sil,reg::rsi,reg::si,reg::esi},reg_kind::gpr64,64,0,reg::none},
		{"rdi",{reg::dil,reg::rdi,reg::di,reg::edi},reg_kind::gpr64,64,0,reg::none},
		{"r8",{reg::r8b,reg::r8,reg::r8w,reg::r8d},reg_kind::gpr64,64,0,reg::none},
		{"r9",{reg::r9b,reg::r9,reg::r9w,reg::r9d},reg_kind::gpr64,64,0,reg::none},
		{"r10",{reg::r10b,reg::r10,reg::r10w,reg::r10d},reg_kind::gpr64,64,0,reg::none},
		{"r11",{reg::r11b,reg::r11,reg::r11w,reg::r11d},reg_kind::gpr64,64,0,reg::none},
		{"r12",{reg::r12b,reg::r12,reg::r12w,reg::r12d},reg_kind::gpr64,64,0,reg::none},
		{"r13",{reg::r13b,reg::r13,reg::r13w,reg::r13d},reg_kind::gpr64,64,0,reg::none},
		{"r14",{reg::r14b,reg::r14,reg::r14w,reg::r14d},reg_kind::gpr64,64,0,reg::none},
		{"r15",{reg::r15b,reg::r15,reg::r15w,reg::r15d},reg_kind::gpr64,64,0,reg::none},
		{"st0",{reg::mm0,reg::st0},reg_kind::fp80,80,0,reg::none},
		{"st1",{reg::mm1,reg::st1},reg_kind::fp80,80,0,reg::none},
		{"st2",{reg::mm2,reg::st2},reg_kind::fp80,80,0,reg::none},
		{"st3",{reg::mm3,reg::st3},reg_kind::fp80,80,0,reg::none},
		{"st4",{reg::mm4,reg::st4},reg_kind::fp80,80,0,reg::none},
		{"st5",{reg::mm5,reg::st5},reg_kind::fp80,80,0,reg::none},
		{"st6",{reg::mm6,reg::st6},reg_kind::fp80,80,0,reg::none},
		{"st7",{reg::mm7,reg::st7},reg_kind::fp80,80,0,reg::none},
		{"mm0",{},reg_kind::simd64,64,0,reg::st0},
		{"mm1",{},reg_kind::simd64,64,0,reg::st1},
		{"mm2",{},reg_kind::simd64,64,0,reg::st2},
		{"mm3",{},reg_kind::simd64,64,0,reg::st3},
		{"mm4",{},reg_kind::simd64,64,0,reg::st4},
		{"mm5",{},reg_kind::simd64,64,0,reg::st5},
		{"mm6",{},reg_kind::simd64,64,0,reg::st6},
		{"mm7",{},reg_kind::simd64,64,0,reg::st7},
		{"x87control",{},reg_kind::control,16,0,reg::none},
		{"x87status",{},reg_kind::control,16,0,reg::none},
		{"x87tag",{},reg_kind::control,16,0,reg::none},
		{"flags",{},reg_kind::control,16,0,reg::rflags},
		{"eflags",{},reg_kind::control,32,0,reg::rflags},
		{"rflags",{reg::flags,reg::rflags,reg::eflags,reg::flag_cf,reg::flag_pf,reg::flag_af,reg::flag_zf,reg::flag_sf,reg::flag_tf,reg::flag_if,reg::flag_df,reg::flag_of,reg::flag_iopl,reg::flag_nt,reg::flag_rf,reg::flag_vm,reg::flag_ac,reg::flag_vif,reg::flag_vip,reg::flag_id},reg_kind::control,64,0,reg::none},
		{"ip",{},reg_kind::instruction,16,0,reg::rip},
		{"eip",{},reg_kind::instruction,32,0,reg::rip},
		{"rip",{reg::ip,reg::rip,reg::eip},reg_kind::instruction,64,0,reg::none},
		{"es",{},reg_kind::segment,16,0,reg::none},
		{"cs",{},reg_kind::segment,16,0,reg::none},
		{"ss",{},reg_kind::segment,16,0,reg::none},
		{"ds",{},reg_kind::segment,16,0,reg::none},
		{"fs",{},reg_kind::segment,16,0,reg::none},
		{"gs",{},reg_kind::segment,16,0,reg::none},
		{"cr0",{},reg_kind::control,32,0,reg::none},
		{"cr2",{},reg_kind::control,64,0,reg::none},
		{"cr3",{},reg_kind::control,64,0,reg::none},
		{"cr4",{},reg_kind::control,32,0,reg::none},
		{"cr8",{},reg_kind::control,8,0,reg::none},
		{"dr0",{},reg_kind::control,64,0,reg::none},
		{"dr1",{},reg_kind::control,64,0,reg::none},
		{"dr2",{},reg_kind::control,64,0,reg::none},
		{"dr3",{},reg_kind::control,64,0,reg::none},
		{"dr4",{},reg_kind::control,32,0,reg::none},
		{"dr5",{},reg_kind::control,32,0,reg::none},
		{"dr6",{},reg_kind::control,32,0,reg::none},
		{"dr7",{},reg_kind::control,32,0,reg::none},
		{"flag_cf",{},reg_kind::flag,1,0,reg::rflags},
		{"flag_pf",{},reg_kind::flag,1,2,reg::rflags},
		{"flag_af",{},reg_kind::flag,1,4,reg::rflags},
		{"flag_zf",{},reg_kind::flag,1,6,reg::rflags},
		{"flag_sf",{},reg_kind::flag,1,7,reg::rflags},
		{"flag_tf",{},reg_kind::flag,1,8,reg::rflags},
		{"flag_if",{},reg_kind::flag,1,9,reg::rflags},
		{"flag_df",{},reg_kind::flag,1,10,reg::rflags},
		{"flag_of",{},reg_kind::flag,1,11,reg::rflags},
		{"flag_iopl",{},reg_kind::control,2,12,reg::rflags},
		{"flag_nt",{},reg_kind::flag,1,14,reg::rflags},
		{"flag_rf",{},reg_kind::flag,1,16,reg::rflags},
		{"flag_vm",{},reg_kind::flag,1,17,reg::rflags},
		{"flag_ac",{},reg_kind::flag,1,18,reg::rflags},
		{"flag_vif",{},reg_kind::flag,1,19,reg::rflags},
		{"flag_vip",{},reg_kind::flag,1,20,reg::rflags},
		{"flag_id",{},reg_kind::flag,1,21,reg::rflags},
		{"xmm0s",{},reg_kind::fp32,32,0,reg::ymm0},
		{"xmm1s",{},reg_kind::fp32,32,0,reg::ymm1},
		{"xmm2s",{},reg_kind::fp32,32,0,reg::ymm2},
		{"xmm3s",{},reg_kind::fp32,32,0,reg::ymm3},
		{"xmm4s",{},reg_kind::fp32,32,0,reg::ymm4},
		{"xmm5s",{},reg_kind::fp32,32,0,reg::ymm5},
		{"xmm6s",{},reg_kind::fp32,32,0,reg::ymm6},
		{"xmm7s",{},reg_kind::fp32,32,0,reg::ymm7},
		{"xmm8s",{},reg_kind::fp32,32,0,reg::ymm8},
		{"xmm9s",{},reg_kind::fp32,32,0,reg::ymm9},
		{"xmm10s",{},reg_kind::fp32,32,0,reg::ymm10},
		{"xmm11s",{},reg_kind::fp32,32,0,reg::ymm11},
		{"xmm12s",{},reg_kind::fp32,32,0,reg::ymm12},
		{"xmm13s",{},reg_kind::fp32,32,0,reg::ymm13},
		{"xmm14s",{},reg_kind::fp32,32,0,reg::ymm14},
		{"xmm15s",{},reg_kind::fp32,32,0,reg::ymm15},
		{"xmm0d",{},reg_kind::fp64,64,0,reg::ymm0},
		{"xmm1d",{},reg_kind::fp64,64,0,reg::ymm1},
		{"xmm2d",{},reg_kind::fp64,64,0,reg::ymm2},
		{"xmm3d",{},reg_kind::fp64,64,0,reg::ymm3},
		{"xmm4d",{},reg_kind::fp64,64,0,reg::ymm4},
		{"xmm5d",{},reg_kind::fp64,64,0,reg::ymm5},
		{"xmm6d",{},reg_kind::fp64,64,0,reg::ymm6},
		{"xmm7d",{},reg_kind::fp64,64,0,reg::ymm7},
		{"xmm8d",{},reg_kind::fp64,64,0,reg::ymm8},
		{"xmm9d",{},reg_kind::fp64,64,0,reg::ymm9},
		{"xmm10d",{},reg_kind::fp64,64,0,reg::ymm10},
		{"xmm11d",{},reg_kind::fp64,64,0,reg::ymm11},
		{"xmm12d",{},reg_kind::fp64,64,0,reg::ymm12},
		{"xmm13d",{},reg_kind::fp64,64,0,reg::ymm13},
		{"xmm14d",{},reg_kind::fp64,64,0,reg::ymm14},
		{"xmm15d",{},reg_kind::fp64,64,0,reg::ymm15},
		{"xmm0",{},reg_kind::simd128,128,0,reg::ymm0},
		{"xmm1",{},reg_kind::simd128,128,0,reg::ymm1},
		{"xmm2",{},reg_kind::simd128,128,0,reg::ymm2},
		{"xmm3",{},reg_kind::simd128,128,0,reg::ymm3},
		{"xmm4",{},reg_kind::simd128,128,0,reg::ymm4},
		{"xmm5",{},reg_kind::simd128,128,0,reg::ymm5},
		{"xmm6",{},reg_kind::simd128,128,0,reg::ymm6},
		{"xmm7",{},reg_kind::simd128,128,0,reg::ymm7},
		{"xmm8",{},reg_kind::simd128,128,0,reg::ymm8},
		{"xmm9",{},reg_kind::simd128,128,0,reg::ymm9},
		{"xmm10",{},reg_kind::simd128,128,0,reg::ymm10},
		{"xmm11",{},reg_kind::simd128,128,0,reg::ymm11},
		{"xmm12",{},reg_kind::simd128,128,0,reg::ymm12},
		{"xmm13",{},reg_kind::simd128,128,0,reg::ymm13},
		{"xmm14",{},reg_kind::simd128,128,0,reg::ymm14},
		{"xmm15",{},reg_kind::simd128,128,0,reg::ymm15},
		{"ymm0",{reg::xmm0s,reg::ymm0,reg::xmm0d,reg::xmm0},reg_kind::simd256,256,0,reg::none},
		{"ymm1",{reg::xmm1s,reg::ymm1,reg::xmm1d,reg::xmm1},reg_kind::simd256,256,0,reg::none},
		{"ymm2",{reg::xmm2s,reg::ymm2,reg::xmm2d,reg::xmm2},reg_kind::simd256,256,0,reg::none},
		{"ymm3",{reg::xmm3s,reg::ymm3,reg::xmm3d,reg::xmm3},reg_kind::simd256,256,0,reg::none},
		{"ymm4",{reg::xmm4s,reg::ymm4,reg::xmm4d,reg::xmm4},reg_kind::simd256,256,0,reg::none},
		{"ymm5",{reg::xmm5s,reg::ymm5,reg::xmm5d,reg::xmm5},reg_kind::simd256,256,0,reg::none},
		{"ymm6",{reg::xmm6s,reg::ymm6,reg::xmm6d,reg::xmm6},reg_kind::simd256,256,0,reg::none},
		{"ymm7",{reg::xmm7s,reg::ymm7,reg::xmm7d,reg::xmm7},reg_kind::simd256,256,0,reg::none},
		{"ymm8",{reg::xmm8s,reg::ymm8,reg::xmm8d,reg::xmm8},reg_kind::simd256,256,0,reg::none},
		{"ymm9",{reg::xmm9s,reg::ymm9,reg::xmm9d,reg::xmm9},reg_kind::simd256,256,0,reg::none},
		{"ymm10",{reg::xmm10s,reg::ymm10,reg::xmm10d,reg::xmm10},reg_kind::simd256,256,0,reg::none},
		{"ymm11",{reg::xmm11s,reg::ymm11,reg::xmm11d,reg::xmm11},reg_kind::simd256,256,0,reg::none},
		{"ymm12",{reg::xmm12s,reg::ymm12,reg::xmm12d,reg::xmm12},reg_kind::simd256,256,0,reg::none},
		{"ymm13",{reg::xmm13s,reg::ymm13,reg::xmm13d,reg::xmm13},reg_kind::simd256,256,0,reg::none},
		{"ymm14",{reg::xmm14s,reg::ymm14,reg::xmm14d,reg::xmm14},reg_kind::simd256,256,0,reg::none},
		{"ymm15",{reg::xmm15s,reg::ymm15,reg::xmm15d,reg::xmm15},reg_kind::simd256,256,0,reg::none},
	};
	RC_INLINE constexpr std::span<const reg_desc> reg_desc::all() { return regs; }
};
namespace retro { template<> struct descriptor<retro::arch::x86::reg> { using type = retro::arch::x86::reg_desc; }; };
RC_DEFINE_STD_VISITOR_FOR(retro::arch::x86::reg, RC_VISIT_ARCH_X86_REG)
// clang-format on