import { Type } from "./builtin_types";

// prettier-ignore
export enum OpKind {
	None           = 0,
	UnarySigned    = 1,
	UnaryUnsigned  = 2,
	UnaryBitwise   = 3,
	UnaryFloat     = 4,
	BinarySigned   = 5,
	BinaryUnsigned = 6,
	BinaryBitwise  = 7,
	BinaryFloat    = 8,
	CmpSigned      = 9,
	CmpUnsigned    = 10,
	// PSEUDO
	MAX            = 10,
	BIT_WIDTH      = 4,
}
// prettier-ignore
export enum Op {
	None        = 0,
	Neg         = 1,
	Abs         = 2,
	BitLsb      = 3,
	BitMsb      = 4,
	BitPopcnt   = 5,
	BitByteswap = 6,
	BitNot      = 7,
	Ceil        = 8,
	Floor       = 9,
	Trunc       = 10,
	Round       = 11,
	Sin         = 12,
	Cos         = 13,
	Tan         = 14,
	Asin        = 15,
	Acos        = 16,
	Atan        = 17,
	Sqrt        = 18,
	Exp         = 19,
	Log         = 20,
	Atan2       = 21,
	Pow         = 22,
	Add         = 23,
	Sub         = 24,
	Mul         = 25,
	Div         = 26,
	Udiv        = 27,
	Rem         = 28,
	Urem        = 29,
	BitOr       = 30,
	BitAnd      = 31,
	BitXor      = 32,
	BitShl      = 33,
	BitShr      = 34,
	BitSar      = 35,
	BitRol      = 36,
	BitRor      = 37,
	Max         = 38,
	Umax        = 39,
	Min         = 40,
	Umin        = 41,
	Gt          = 42,
	Ugt         = 43,
	Le          = 44,
	Ule         = 45,
	Ge          = 46,
	Uge         = 47,
	Lt          = 48,
	Ult         = 49,
	Eq          = 50,
	Ne          = 51,
	// PSEUDO
	MAX         = 51,
	BIT_WIDTH   = 6,
}
// prettier-ignore
export enum Intrinsic {
	None                = 0,
	AllocStack          = 1,
	Malloc              = 2,
	Realloc             = 3,
	Free                = 4,
	Sfence              = 5,
	Lfence              = 6,
	Mfence              = 7,
	Memcpy              = 8,
	Memmove             = 9,
	Memcmp              = 10,
	Memset              = 11,
	Memset16            = 12,
	Memset32            = 13,
	Memset64            = 14,
	Memchr              = 15,
	Strcpy              = 16,
	Strcmp              = 17,
	Strchr              = 18,
	Strstr              = 19,
	Loadnontemporal8    = 20,
	Loadnontemporal16   = 21,
	Loadnontemporal32   = 22,
	Loadnontemporal64   = 23,
	Loadnontemporal128  = 24,
	Loadnontemporal256  = 25,
	Loadnontemporal512  = 26,
	Storenontemporal8   = 27,
	Storenontemporal16  = 28,
	Storenontemporal32  = 29,
	Storenontemporal64  = 30,
	Storenontemporal128 = 31,
	Storenontemporal256 = 32,
	Storenontemporal512 = 33,
	Readcyclecounter    = 34,
	Prefetch            = 35,
	Ia32Rdgsbase32      = 36,
	Ia32Rdfsbase32      = 37,
	Ia32Rdgsbase64      = 38,
	Ia32Rdfsbase64      = 39,
	Ia32Wrgsbase32      = 40,
	Ia32Wrfsbase32      = 41,
	Ia32Wrgsbase64      = 42,
	Ia32Wrfsbase64      = 43,
	Ia32Swapgs          = 44,
	Ia32Stmxcsr         = 45,
	Ia32Ldmxcsr         = 46,
	Ia32Pause           = 47,
	Ia32Hlt             = 48,
	Ia32Rsm             = 49,
	Ia32Invd            = 50,
	Ia32Wbinvd          = 51,
	Ia32Readpid         = 52,
	Ia32Cpuid           = 53,
	Ia32Xgetbv          = 54,
	Ia32Rdpmc           = 55,
	Ia32Xsetbv          = 56,
	Ia32Rdmsr           = 57,
	Ia32Wrmsr           = 58,
	Ia32Invlpg          = 59,
	Ia32Invpcid         = 60,
	Ia32Prefetcht0      = 61,
	Ia32Prefetcht1      = 62,
	Ia32Prefetcht2      = 63,
	Ia32Prefetchnta     = 64,
	Ia32Prefetchw       = 65,
	Ia32Prefetchwt1     = 66,
	Ia32Cldemote        = 67,
	Ia32Clflush         = 68,
	Ia32Clflushopt      = 69,
	Ia32Clwb            = 70,
	Ia32Clzero          = 71,
	Ia32Outb            = 72,
	Ia32Outw            = 73,
	Ia32Outd            = 74,
	Ia32Inb             = 75,
	Ia32Inw             = 76,
	Ia32Ind             = 77,
	Ia32Fxrstor         = 78,
	Ia32Fxrstor64       = 79,
	Ia32Fxsave          = 80,
	Ia32Fxsave64        = 81,
	Ia32Xrstor          = 82,
	Ia32Xrstor64        = 83,
	Ia32Xrstors         = 84,
	Ia32Xrstors64       = 85,
	Ia32Xsave           = 86,
	Ia32Xsavec          = 87,
	Ia32Xsaveopt        = 88,
	Ia32Xsaves          = 89,
	Ia32Xsave64         = 90,
	Ia32Xsavec64        = 91,
	Ia32Xsaveopt64      = 92,
	Ia32Xsaves64        = 93,
	Ia32Sgdt            = 94,
	Ia32Sidt            = 95,
	Ia32Sldt            = 96,
	Ia32Smsw            = 97,
	Ia32Str             = 98,
	Ia32Lgdt            = 99,
	Ia32Lidt            = 100,
	Ia32Lldt            = 101,
	Ia32Ltr             = 102,
	Ia32Lmsw            = 103,
	Ia32Getes           = 104,
	Ia32Getcs           = 105,
	Ia32Getss           = 106,
	Ia32Getds           = 107,
	Ia32Getfs           = 108,
	Ia32Getgs           = 109,
	Ia32Setes           = 110,
	Ia32Setcs           = 111,
	Ia32Setss           = 112,
	Ia32Setds           = 113,
	Ia32Setfs           = 114,
	Ia32Setgs           = 115,
	Retaddr             = 116,
	AddrRetaddr         = 117,
	// PSEUDO
	MAX                 = 117,
	BIT_WIDTH           = 7,
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                      Descriptors                                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// prettier-ignore
export class OpKindDesc {
	name: string = "";
}
// prettier-ignore
export class OpDesc {
	name:        string  = "";
	symbol:      string  = "";
	commutative: boolean = false;
	kind:        OpKind  = OpKind.None;
	inverse:     Op      = Op.None;
	swapSign:    Op      = Op.None;
}
// prettier-ignore
export class IntrinsicDesc {
	name:   string = "";
	symbol: string = "";
	args:   Type[] = [];
	ret:    Type   = Type.None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                         Tables                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// prettier-ignore
const OpKind_DescTable: OpKindDesc[] = [
	new OpKindDesc(),
	{name:"unary-signed"},
	{name:"unary-unsigned"},
	{name:"unary-bitwise"},
	{name:"unary-float"},
	{name:"binary-signed"},
	{name:"binary-unsigned"},
	{name:"binary-bitwise"},
	{name:"binary-float"},
	{name:"cmp-signed"},
	{name:"cmp-unsigned"},
]
// prettier-ignore
export namespace OpKind {
    export function reflect(i:OpKind) : OpKindDesc { return OpKind_DescTable[i]; }
    export function toString(i:OpKind) : string { return OpKind_DescTable[i].name; }
}
// prettier-ignore
const Op_DescTable: OpDesc[] = [
	new OpDesc(),
	{name:"neg",symbol:"-",commutative:false,kind:OpKind.UnarySigned,inverse:Op.Neg,swapSign:Op.None},
	{name:"abs",symbol:"abs",commutative:false,kind:OpKind.UnarySigned,inverse:Op.None,swapSign:Op.None},
	{name:"bit_lsb",symbol:"lsb",commutative:false,kind:OpKind.UnaryBitwise,inverse:Op.None,swapSign:Op.None},
	{name:"bit_msb",symbol:"msb",commutative:false,kind:OpKind.UnaryBitwise,inverse:Op.None,swapSign:Op.None},
	{name:"bit_popcnt",symbol:"popcnt",commutative:false,kind:OpKind.UnaryBitwise,inverse:Op.None,swapSign:Op.None},
	{name:"bit_byteswap",symbol:"bswap",commutative:false,kind:OpKind.UnaryBitwise,inverse:Op.None,swapSign:Op.None},
	{name:"bit_not",symbol:"~",commutative:false,kind:OpKind.UnaryBitwise,inverse:Op.BitNot,swapSign:Op.None},
	{name:"ceil",symbol:"ceil",commutative:false,kind:OpKind.UnaryFloat,inverse:Op.None,swapSign:Op.None},
	{name:"floor",symbol:"floor",commutative:false,kind:OpKind.UnaryFloat,inverse:Op.None,swapSign:Op.None},
	{name:"trunc",symbol:"trunc",commutative:false,kind:OpKind.UnaryFloat,inverse:Op.None,swapSign:Op.None},
	{name:"round",symbol:"round",commutative:false,kind:OpKind.UnaryFloat,inverse:Op.None,swapSign:Op.None},
	{name:"sin",symbol:"sin",commutative:false,kind:OpKind.UnaryFloat,inverse:Op.None,swapSign:Op.None},
	{name:"cos",symbol:"cos",commutative:false,kind:OpKind.UnaryFloat,inverse:Op.None,swapSign:Op.None},
	{name:"tan",symbol:"tan",commutative:false,kind:OpKind.UnaryFloat,inverse:Op.None,swapSign:Op.None},
	{name:"asin",symbol:"asin",commutative:false,kind:OpKind.UnaryFloat,inverse:Op.None,swapSign:Op.None},
	{name:"acos",symbol:"acos",commutative:false,kind:OpKind.UnaryFloat,inverse:Op.None,swapSign:Op.None},
	{name:"atan",symbol:"atan",commutative:false,kind:OpKind.UnaryFloat,inverse:Op.None,swapSign:Op.None},
	{name:"sqrt",symbol:"sqrt",commutative:false,kind:OpKind.UnaryFloat,inverse:Op.None,swapSign:Op.None},
	{name:"exp",symbol:"exp",commutative:false,kind:OpKind.UnaryFloat,inverse:Op.Log,swapSign:Op.None},
	{name:"log",symbol:"log",commutative:false,kind:OpKind.UnaryFloat,inverse:Op.Exp,swapSign:Op.None},
	{name:"atan2",symbol:"atan2",commutative:false,kind:OpKind.BinaryFloat,inverse:Op.None,swapSign:Op.None},
	{name:"pow",symbol:"pow",commutative:false,kind:OpKind.BinaryFloat,inverse:Op.None,swapSign:Op.None},
	{name:"add",symbol:"+",commutative:true,kind:OpKind.BinarySigned,inverse:Op.None,swapSign:Op.None},
	{name:"sub",symbol:"-",commutative:false,kind:OpKind.BinarySigned,inverse:Op.None,swapSign:Op.None},
	{name:"mul",symbol:"*",commutative:true,kind:OpKind.BinarySigned,inverse:Op.None,swapSign:Op.None},
	{name:"div",symbol:"/",commutative:false,kind:OpKind.BinarySigned,inverse:Op.None,swapSign:Op.Udiv},
	{name:"udiv",symbol:"u/",commutative:false,kind:OpKind.BinaryUnsigned,inverse:Op.None,swapSign:Op.Div},
	{name:"rem",symbol:"%",commutative:false,kind:OpKind.BinarySigned,inverse:Op.None,swapSign:Op.Urem},
	{name:"urem",symbol:"u%",commutative:false,kind:OpKind.BinaryUnsigned,inverse:Op.None,swapSign:Op.Rem},
	{name:"bit_or",symbol:"|",commutative:true,kind:OpKind.BinaryBitwise,inverse:Op.None,swapSign:Op.None},
	{name:"bit_and",symbol:"&",commutative:true,kind:OpKind.BinaryBitwise,inverse:Op.None,swapSign:Op.None},
	{name:"bit_xor",symbol:"^",commutative:true,kind:OpKind.BinaryBitwise,inverse:Op.BitXor,swapSign:Op.None},
	{name:"bit_shl",symbol:"<<",commutative:false,kind:OpKind.BinaryBitwise,inverse:Op.None,swapSign:Op.None},
	{name:"bit_shr",symbol:">>",commutative:false,kind:OpKind.BinaryBitwise,inverse:Op.None,swapSign:Op.BitSar},
	{name:"bit_sar",symbol:"s>>",commutative:false,kind:OpKind.BinaryBitwise,inverse:Op.None,swapSign:Op.BitShr},
	{name:"bit_rol",symbol:"rotl",commutative:false,kind:OpKind.BinaryBitwise,inverse:Op.BitRor,swapSign:Op.None},
	{name:"bit_ror",symbol:"rotr",commutative:false,kind:OpKind.BinaryBitwise,inverse:Op.BitRol,swapSign:Op.None},
	{name:"max",symbol:"max",commutative:true,kind:OpKind.BinarySigned,inverse:Op.None,swapSign:Op.Umax},
	{name:"umax",symbol:"umax",commutative:true,kind:OpKind.BinaryUnsigned,inverse:Op.None,swapSign:Op.Max},
	{name:"min",symbol:"min",commutative:true,kind:OpKind.BinarySigned,inverse:Op.None,swapSign:Op.Umin},
	{name:"umin",symbol:"umin",commutative:true,kind:OpKind.BinaryUnsigned,inverse:Op.None,swapSign:Op.Min},
	{name:"gt",symbol:">",commutative:false,kind:OpKind.CmpSigned,inverse:Op.Le,swapSign:Op.Ugt},
	{name:"ugt",symbol:"u>",commutative:false,kind:OpKind.CmpUnsigned,inverse:Op.Ule,swapSign:Op.Gt},
	{name:"le",symbol:"<=",commutative:false,kind:OpKind.CmpSigned,inverse:Op.Gt,swapSign:Op.Ule},
	{name:"ule",symbol:"u<=",commutative:false,kind:OpKind.CmpUnsigned,inverse:Op.Ugt,swapSign:Op.Le},
	{name:"ge",symbol:">=",commutative:false,kind:OpKind.CmpSigned,inverse:Op.Lt,swapSign:Op.Uge},
	{name:"uge",symbol:"u>=",commutative:false,kind:OpKind.CmpUnsigned,inverse:Op.Ult,swapSign:Op.Ge},
	{name:"lt",symbol:"<",commutative:false,kind:OpKind.CmpSigned,inverse:Op.Ge,swapSign:Op.Ult},
	{name:"ult",symbol:"u<",commutative:false,kind:OpKind.CmpUnsigned,inverse:Op.Uge,swapSign:Op.Lt},
	{name:"eq",symbol:"==",commutative:true,kind:OpKind.CmpSigned,inverse:Op.Ne,swapSign:Op.None},
	{name:"ne",symbol:"!=",commutative:true,kind:OpKind.CmpSigned,inverse:Op.Eq,swapSign:Op.None},
]
// prettier-ignore
export namespace Op {
    export function reflect(i:Op) : OpDesc { return Op_DescTable[i]; }
    export function toString(i:Op) : string { return Op_DescTable[i].name; }
}
// prettier-ignore
const Intrinsic_DescTable: IntrinsicDesc[] = [
	new IntrinsicDesc(),
	{name:"alloc-stack",symbol:"alloca",args:[Type.I32],ret:Type.Pointer},
	{name:"malloc",symbol:"",args:[Type.I64],ret:Type.Pointer},
	{name:"realloc",symbol:"",args:[Type.Pointer,Type.I64],ret:Type.Pointer},
	{name:"free",symbol:"",args:[Type.Pointer],ret:Type.None},
	{name:"sfence",symbol:"",args:[],ret:Type.None},
	{name:"lfence",symbol:"",args:[],ret:Type.None},
	{name:"mfence",symbol:"",args:[],ret:Type.None},
	{name:"memcpy",symbol:"",args:[Type.Pointer,Type.Pointer,Type.I64],ret:Type.Pointer},
	{name:"memmove",symbol:"",args:[Type.Pointer,Type.Pointer,Type.I64],ret:Type.Pointer},
	{name:"memcmp",symbol:"",args:[Type.Pointer,Type.Pointer,Type.I64],ret:Type.I32},
	{name:"memset",symbol:"",args:[Type.Pointer,Type.I32,Type.I64],ret:Type.Pointer},
	{name:"memset16",symbol:"",args:[Type.Pointer,Type.I32,Type.I64],ret:Type.Pointer},
	{name:"memset32",symbol:"",args:[Type.Pointer,Type.I32,Type.I64],ret:Type.Pointer},
	{name:"memset64",symbol:"",args:[Type.Pointer,Type.I64,Type.I64],ret:Type.Pointer},
	{name:"memchr",symbol:"",args:[Type.Pointer,Type.I32,Type.I64],ret:Type.Pointer},
	{name:"strcpy",symbol:"",args:[Type.Pointer,Type.Pointer],ret:Type.Pointer},
	{name:"strcmp",symbol:"",args:[Type.Pointer,Type.Pointer],ret:Type.I32},
	{name:"strchr",symbol:"",args:[Type.Pointer,Type.I32],ret:Type.Pointer},
	{name:"strstr",symbol:"",args:[Type.Pointer,Type.Pointer],ret:Type.Pointer},
	{name:"loadnontemporal8",symbol:"__builtin_load_nontemporal8",args:[Type.Pointer],ret:Type.I8},
	{name:"loadnontemporal16",symbol:"__builtin_load_nontemporal16",args:[Type.Pointer],ret:Type.I16},
	{name:"loadnontemporal32",symbol:"__builtin_load_nontemporal32",args:[Type.Pointer],ret:Type.I32},
	{name:"loadnontemporal64",symbol:"__builtin_load_nontemporal64",args:[Type.Pointer],ret:Type.I64},
	{name:"loadnontemporal128",symbol:"__builtin_load_nontemporal128",args:[Type.Pointer],ret:Type.I32x4},
	{name:"loadnontemporal256",symbol:"__builtin_load_nontemporal256",args:[Type.Pointer],ret:Type.I32x8},
	{name:"loadnontemporal512",symbol:"__builtin_load_nontemporal512",args:[Type.Pointer],ret:Type.I32x16},
	{name:"storenontemporal8",symbol:"__builtin_store_nontemporal8",args:[Type.Pointer,Type.I8],ret:Type.None},
	{name:"storenontemporal16",symbol:"__builtin_store_nontemporal16",args:[Type.Pointer,Type.I16],ret:Type.None},
	{name:"storenontemporal32",symbol:"__builtin_store_nontemporal32",args:[Type.Pointer,Type.I32],ret:Type.None},
	{name:"storenontemporal64",symbol:"__builtin_store_nontemporal64",args:[Type.Pointer,Type.I64],ret:Type.None},
	{name:"storenontemporal128",symbol:"__builtin_store_nontemporal128",args:[Type.Pointer,Type.I32x4],ret:Type.None},
	{name:"storenontemporal256",symbol:"__builtin_store_nontemporal256",args:[Type.Pointer,Type.I32x8],ret:Type.None},
	{name:"storenontemporal512",symbol:"__builtin_store_nontemporal512",args:[Type.Pointer,Type.I32x16],ret:Type.None},
	{name:"readcyclecounter",symbol:"__builtin_readcyclecounter",args:[],ret:Type.I64},
	{name:"prefetch",symbol:"__builtin_prefetech",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-rdgsbase32",symbol:"_readgsbase_u32",args:[],ret:Type.I32},
	{name:"ia32-rdfsbase32",symbol:"_readfsbase_u32",args:[],ret:Type.I32},
	{name:"ia32-rdgsbase64",symbol:"_readgsbase_u64",args:[],ret:Type.I64},
	{name:"ia32-rdfsbase64",symbol:"_readfsbase_u64",args:[],ret:Type.I64},
	{name:"ia32-wrgsbase32",symbol:"_writegsbase_u32",args:[Type.I32],ret:Type.None},
	{name:"ia32-wrfsbase32",symbol:"_writefsbase_u32",args:[Type.I32],ret:Type.None},
	{name:"ia32-wrgsbase64",symbol:"_writegsbase_u64",args:[Type.I64],ret:Type.None},
	{name:"ia32-wrfsbase64",symbol:"_writefsbase_u64",args:[Type.I64],ret:Type.None},
	{name:"ia32-swapgs",symbol:"__swapgs",args:[],ret:Type.None},
	{name:"ia32-stmxcsr",symbol:"_mm_getcsr",args:[],ret:Type.I32},
	{name:"ia32-ldmxcsr",symbol:"_mm_setcsr",args:[Type.I32],ret:Type.None},
	{name:"ia32-pause",symbol:"_mm_pause",args:[],ret:Type.None},
	{name:"ia32-hlt",symbol:"__hlt",args:[],ret:Type.None},
	{name:"ia32-rsm",symbol:"__rsm",args:[],ret:Type.None},
	{name:"ia32-invd",symbol:"__invd",args:[],ret:Type.None},
	{name:"ia32-wbinvd",symbol:"__wbinvd",args:[],ret:Type.None},
	{name:"ia32-readpid",symbol:"__builtin_ia32_readpid",args:[],ret:Type.I32},
	{name:"ia32-cpuid",symbol:"_cpuid",args:[Type.I32,Type.I32],ret:Type.I32x4},
	{name:"ia32-xgetbv",symbol:"_xgetbv",args:[Type.I32],ret:Type.I64},
	{name:"ia32-rdpmc",symbol:"_rdpmc",args:[Type.I32],ret:Type.I64},
	{name:"ia32-xsetbv",symbol:"_xgetbv",args:[Type.I32,Type.I64],ret:Type.None},
	{name:"ia32-rdmsr",symbol:"__rdmsr",args:[Type.I32],ret:Type.I64},
	{name:"ia32-wrmsr",symbol:"__wrmsr",args:[Type.I32,Type.I64],ret:Type.None},
	{name:"ia32-invlpg",symbol:"__invlpg",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-invpcid",symbol:"__invvpcid",args:[Type.I32,Type.Pointer],ret:Type.None},
	{name:"ia32-prefetcht0",symbol:"_m_prefetcht0",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-prefetcht1",symbol:"_m_prefetcht1",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-prefetcht2",symbol:"_m_prefetcht2",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-prefetchnta",symbol:"_m_prefetchnta",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-prefetchw",symbol:"_m_prefetchw",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-prefetchwt1",symbol:"_m_prefetchwt1",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-cldemote",symbol:"_cldemote",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-clflush",symbol:"_mm_clflush",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-clflushopt",symbol:"_mm_clflushopt",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-clwb",symbol:"_mm_clwb",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-clzero",symbol:"_mm_clzero",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-outb",symbol:"__outb",args:[Type.I16,Type.I8],ret:Type.None},
	{name:"ia32-outw",symbol:"__outw",args:[Type.I16,Type.I16],ret:Type.None},
	{name:"ia32-outd",symbol:"__outd",args:[Type.I16,Type.I32],ret:Type.None},
	{name:"ia32-inb",symbol:"__inb",args:[Type.I16],ret:Type.I8},
	{name:"ia32-inw",symbol:"__inw",args:[Type.I16],ret:Type.I16},
	{name:"ia32-ind",symbol:"__inl",args:[Type.I16],ret:Type.I32},
	{name:"ia32-fxrstor",symbol:"_fxrstor",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-fxrstor64",symbol:"_fxrstor64",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-fxsave",symbol:"_fxsave",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-fxsave64",symbol:"_fxsave64",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-xrstor",symbol:"_xrstor",args:[Type.Pointer,Type.I64],ret:Type.None},
	{name:"ia32-xrstor64",symbol:"_xrstor64",args:[Type.Pointer,Type.I64],ret:Type.None},
	{name:"ia32-xrstors",symbol:"_xrstors",args:[Type.Pointer,Type.I64],ret:Type.None},
	{name:"ia32-xrstors64",symbol:"_xrstors64",args:[Type.Pointer,Type.I64],ret:Type.None},
	{name:"ia32-xsave",symbol:"_xsave",args:[Type.Pointer,Type.I64],ret:Type.None},
	{name:"ia32-xsavec",symbol:"_xsavec",args:[Type.Pointer,Type.I64],ret:Type.None},
	{name:"ia32-xsaveopt",symbol:"_xsaveopt",args:[Type.Pointer,Type.I64],ret:Type.None},
	{name:"ia32-xsaves",symbol:"_xsaves64",args:[Type.Pointer,Type.I64],ret:Type.None},
	{name:"ia32-xsave64",symbol:"_xsave64",args:[Type.Pointer,Type.I64],ret:Type.None},
	{name:"ia32-xsavec64",symbol:"_xsavec64",args:[Type.Pointer,Type.I64],ret:Type.None},
	{name:"ia32-xsaveopt64",symbol:"_xsaveopt64",args:[Type.Pointer,Type.I64],ret:Type.None},
	{name:"ia32-xsaves64",symbol:"_xsaves64",args:[Type.Pointer,Type.I64],ret:Type.None},
	{name:"ia32-sgdt",symbol:"__sgdt",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-sidt",symbol:"__sidt",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-sldt",symbol:"__sldt",args:[],ret:Type.I16},
	{name:"ia32-smsw",symbol:"__smsw",args:[],ret:Type.I16},
	{name:"ia32-str",symbol:"__str",args:[],ret:Type.I16},
	{name:"ia32-lgdt",symbol:"__lgdt",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-lidt",symbol:"__lidt",args:[Type.Pointer],ret:Type.None},
	{name:"ia32-lldt",symbol:"__lldt",args:[Type.I16],ret:Type.None},
	{name:"ia32-ltr",symbol:"__ltr",args:[Type.I16],ret:Type.None},
	{name:"ia32-lmsw",symbol:"__lmsw",args:[Type.I16],ret:Type.None},
	{name:"ia32-getes",symbol:"_getes",args:[],ret:Type.I16},
	{name:"ia32-getcs",symbol:"_getcs",args:[],ret:Type.I16},
	{name:"ia32-getss",symbol:"_getss",args:[],ret:Type.I16},
	{name:"ia32-getds",symbol:"_getds",args:[],ret:Type.I16},
	{name:"ia32-getfs",symbol:"_getfs",args:[],ret:Type.I16},
	{name:"ia32-getgs",symbol:"_getgs",args:[],ret:Type.I16},
	{name:"ia32-setes",symbol:"_setes",args:[Type.I16],ret:Type.None},
	{name:"ia32-setcs",symbol:"_setcs",args:[Type.I16],ret:Type.None},
	{name:"ia32-setss",symbol:"_setss",args:[Type.I16],ret:Type.None},
	{name:"ia32-setds",symbol:"_setds",args:[Type.I16],ret:Type.None},
	{name:"ia32-setfs",symbol:"_setfs",args:[Type.I16],ret:Type.None},
	{name:"ia32-setgs",symbol:"_setgs",args:[Type.I16],ret:Type.None},
	{name:"retaddr",symbol:"_ReturnAddress",args:[],ret:Type.Pointer},
	{name:"addr-retaddr",symbol:"_AddressOfReturnAddress",args:[],ret:Type.Pointer},
]
// prettier-ignore
export namespace Intrinsic {
    export function reflect(i:Intrinsic) : IntrinsicDesc { return Intrinsic_DescTable[i]; }
    export function toString(i:Intrinsic) : string { return Intrinsic_DescTable[i].name; }
}