import { Type } from "./builtin_types";

// prettier-ignore
export enum OpcodeKind {
	None           = 0,
	Memory         = 1,
	MemoryRmw      = 2,
	Arch           = 3,
	Stack          = 4,
	Data           = 5,
	Cast           = 6,
	Numeric        = 7,
	NumericRmw     = 8,
	Predicate      = 9,
	Phi            = 10,
	Branch         = 11,
	ExternalBranch = 12,
	Intrinsic      = 13,
	Trap           = 14,
	// PSEUDO
	MAX            = 14,
	BIT_WIDTH      = 4,
}
// prettier-ignore
export enum Opcode {
	None                = 0,
	StackBegin          = 1,
	StackReset          = 2,
	ReadReg             = 3,
	WriteReg            = 4,
	LoadMem             = 5,
	StoreMem            = 6,
	Undef               = 7,
	Poison              = 8,
	Extract             = 9,
	Insert              = 10,
	ContextBegin        = 11,
	ExtractContext      = 12,
	InsertContext       = 13,
	CastSx              = 14,
	Cast                = 15,
	Bitcast             = 16,
	Binop               = 17,
	Unop                = 18,
	AtomicCmpxchg       = 19,
	AtomicXchg          = 20,
	AtomicBinop         = 21,
	AtomicUnop          = 22,
	Cmp                 = 23,
	Phi                 = 24,
	Select              = 25,
	Xcall               = 26,
	Call                = 27,
	Intrinsic           = 28,
	SideeffectIntrinsic = 29,
	Xjmp                = 30,
	Jmp                 = 31,
	Xjs                 = 32,
	Js                  = 33,
	Xret                = 34,
	Ret                 = 35,
	Annotation          = 36,
	Trap                = 37,
	Nop                 = 38,
	Unreachable         = 39,
	// PSEUDO
	MAX                 = 39,
	BIT_WIDTH           = 6,
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                      Descriptors                                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// prettier-ignore
export class OpcodeKindDesc {
	name: string = "";
}
// prettier-ignore
export class OpcodeDesc {
	name:          string     = "";
	types:         Type[]     = [];
	templates:     number[]   = [];
	names:         string[]   = [];
	constexprs:    number[]   = [];
	isAnnotation:  boolean    = false;
	isPure:        boolean    = false;
	sideEffect:    boolean    = false;
	isConst:       boolean    = false;
	unkRegUse:     boolean    = false;
	terminator:    boolean    = false;
	bbTerminator:  boolean    = false;
	templateCount: number     = 0;
	kind:          OpcodeKind = OpcodeKind.None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                         Tables                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// prettier-ignore
const OpcodeKind_DescTable: OpcodeKindDesc[] = [
	new OpcodeKindDesc(),
	{name:"memory"},
	{name:"memory-rmw"},
	{name:"arch"},
	{name:"stack"},
	{name:"data"},
	{name:"cast"},
	{name:"numeric"},
	{name:"numeric-rmw"},
	{name:"predicate"},
	{name:"phi"},
	{name:"branch"},
	{name:"external-branch"},
	{name:"intrinsic"},
	{name:"trap"},
]
// prettier-ignore
export namespace OpcodeKind {
    export function reflect(i:OpcodeKind) : OpcodeKindDesc { return OpcodeKind_DescTable[i]; }
    export function toString(i:OpcodeKind) : string { return OpcodeKind_DescTable[i].name; }
}
// prettier-ignore
const Opcode_DescTable: OpcodeDesc[] = [
	new OpcodeDesc(),
	{name:"stack_begin",types:[Type.Pointer],templates:[0],names:[""],constexprs:[],isAnnotation:true,isPure:false,sideEffect:false,isConst:false,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:0,kind:OpcodeKind.Stack},
	{name:"stack_reset",types:[Type.None,Type.Pointer],templates:[0,0],names:["","sp"],constexprs:[],isAnnotation:true,isPure:false,sideEffect:false,isConst:false,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:0,kind:OpcodeKind.Stack},
	{name:"read_reg",types:[Type.None,Type.Reg],templates:[1,0],names:["","regid"],constexprs:[1],isAnnotation:false,isPure:true,sideEffect:false,isConst:false,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.Arch},
	{name:"write_reg",types:[Type.None,Type.Reg,Type.None],templates:[0,0,1],names:["","regid","value"],constexprs:[1],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.Arch},
	{name:"load_mem",types:[Type.None,Type.Pointer,Type.I64],templates:[1,0,0],names:["","pointer","offset"],constexprs:[2],isAnnotation:false,isPure:true,sideEffect:false,isConst:false,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.Memory},
	{name:"store_mem",types:[Type.None,Type.Pointer,Type.I64,Type.None],templates:[0,0,0,1],names:["","pointer","offset","value"],constexprs:[2],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.Memory},
	{name:"undef",types:[Type.None],templates:[1],names:[""],constexprs:[],isAnnotation:false,isPure:true,sideEffect:false,isConst:true,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.Data},
	{name:"poison",types:[Type.None,Type.Str],templates:[1,0],names:["","Reason"],constexprs:[1],isAnnotation:false,isPure:true,sideEffect:false,isConst:true,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.Data},
	{name:"extract",types:[Type.None,Type.None,Type.I32],templates:[2,1,0],names:["","vector","Lane"],constexprs:[2],isAnnotation:false,isPure:true,sideEffect:false,isConst:true,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:2,kind:OpcodeKind.Data},
	{name:"insert",types:[Type.None,Type.None,Type.I32,Type.None],templates:[1,1,0,2],names:["","vector","Lane","element"],constexprs:[2],isAnnotation:false,isPure:true,sideEffect:false,isConst:true,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:2,kind:OpcodeKind.Data},
	{name:"context_begin",types:[Type.Context,Type.Pointer],templates:[0,0],names:["","sp"],constexprs:[],isAnnotation:true,isPure:true,sideEffect:false,isConst:true,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:0,kind:OpcodeKind.Data},
	{name:"extract_context",types:[Type.None,Type.Context,Type.Reg],templates:[1,0,0],names:["","ctx","regid"],constexprs:[2],isAnnotation:false,isPure:true,sideEffect:false,isConst:true,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.Data},
	{name:"insert_context",types:[Type.Context,Type.Context,Type.Reg,Type.None],templates:[0,0,0,1],names:["","ctx","regid","element"],constexprs:[2],isAnnotation:false,isPure:true,sideEffect:false,isConst:true,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.Data},
	{name:"cast_sx",types:[Type.None,Type.None],templates:[2,1],names:["","value"],constexprs:[],isAnnotation:false,isPure:true,sideEffect:false,isConst:true,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:2,kind:OpcodeKind.Cast},
	{name:"cast",types:[Type.None,Type.None],templates:[2,1],names:["","value"],constexprs:[],isAnnotation:false,isPure:true,sideEffect:false,isConst:true,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:2,kind:OpcodeKind.Cast},
	{name:"bitcast",types:[Type.None,Type.None],templates:[2,1],names:["","value"],constexprs:[],isAnnotation:false,isPure:true,sideEffect:false,isConst:true,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:2,kind:OpcodeKind.Cast},
	{name:"binop",types:[Type.None,Type.Op,Type.None,Type.None],templates:[1,0,1,1],names:["","Op","lhs","rhs"],constexprs:[1],isAnnotation:false,isPure:true,sideEffect:false,isConst:true,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.Numeric},
	{name:"unop",types:[Type.None,Type.Op,Type.None],templates:[1,0,1],names:["","Op","rhs"],constexprs:[1],isAnnotation:false,isPure:true,sideEffect:false,isConst:true,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.Numeric},
	{name:"atomic_cmpxchg",types:[Type.None,Type.Pointer,Type.None,Type.None],templates:[1,0,1,1],names:["","ptr","expected","desired"],constexprs:[],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.MemoryRmw},
	{name:"atomic_xchg",types:[Type.None,Type.Pointer,Type.None],templates:[1,0,1],names:["","ptr","desired"],constexprs:[],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.MemoryRmw},
	{name:"atomic_binop",types:[Type.None,Type.Op,Type.Pointer,Type.None],templates:[1,0,0,1],names:["","Op","lhs_ptr","rhs"],constexprs:[1],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.NumericRmw},
	{name:"atomic_unop",types:[Type.None,Type.Op,Type.Pointer],templates:[1,0,0],names:["","Op","rhs_ptr"],constexprs:[1],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.NumericRmw},
	{name:"cmp",types:[Type.I1,Type.Op,Type.None,Type.None],templates:[0,0,1,1],names:["","Op","lhs","rhs"],constexprs:[1],isAnnotation:false,isPure:true,sideEffect:false,isConst:true,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.Predicate},
	{name:"phi",types:[Type.None,Type.Pack],templates:[1,0],names:["","incoming"],constexprs:[],isAnnotation:false,isPure:true,sideEffect:false,isConst:true,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.Phi},
	{name:"select",types:[Type.None,Type.I1,Type.None,Type.None],templates:[1,0,1,1],names:["","cc","tv","fv"],constexprs:[],isAnnotation:false,isPure:true,sideEffect:false,isConst:true,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.Data},
	{name:"xcall",types:[Type.None,Type.Pointer],templates:[0,0],names:["","destination"],constexprs:[],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:true,terminator:false,bbTerminator:false,templateCount:0,kind:OpcodeKind.ExternalBranch},
	{name:"call",types:[Type.Context,Type.Pointer,Type.Context],templates:[0,0,0],names:["","destination","ctx"],constexprs:[],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:0,kind:OpcodeKind.Branch},
	{name:"intrinsic",types:[Type.Pack,Type.Intrinsic,Type.Pack],templates:[0,0,0],names:["","function","args"],constexprs:[1],isAnnotation:false,isPure:false,sideEffect:false,isConst:false,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:0,kind:OpcodeKind.Intrinsic},
	{name:"sideeffect_intrinsic",types:[Type.Pack,Type.Intrinsic,Type.Pack],templates:[0,0,0],names:["","function","args"],constexprs:[1],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:0,kind:OpcodeKind.Intrinsic},
	{name:"xjmp",types:[Type.None,Type.Pointer],templates:[0,0],names:["","destination"],constexprs:[],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:false,terminator:true,bbTerminator:true,templateCount:0,kind:OpcodeKind.ExternalBranch},
	{name:"jmp",types:[Type.None,Type.Label],templates:[0,0],names:["","destination"],constexprs:[],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:false,terminator:true,bbTerminator:false,templateCount:0,kind:OpcodeKind.Branch},
	{name:"xjs",types:[Type.None,Type.I1,Type.Pointer,Type.Pointer],templates:[0,0,0,0],names:["","cc","tb","fb"],constexprs:[2,3],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:false,terminator:true,bbTerminator:true,templateCount:0,kind:OpcodeKind.ExternalBranch},
	{name:"js",types:[Type.None,Type.I1,Type.Label,Type.Label],templates:[0,0,0,0],names:["","cc","tb","fb"],constexprs:[],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:false,terminator:true,bbTerminator:false,templateCount:0,kind:OpcodeKind.Branch},
	{name:"xret",types:[Type.None,Type.Pointer],templates:[0,0],names:["","ptr"],constexprs:[],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:true,terminator:true,bbTerminator:true,templateCount:0,kind:OpcodeKind.ExternalBranch},
	{name:"ret",types:[Type.None,Type.Context,Type.I64],templates:[0,0,0],names:["","ctx","offset"],constexprs:[2],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:false,terminator:true,bbTerminator:true,templateCount:0,kind:OpcodeKind.Branch},
	{name:"annotation",types:[Type.None,Type.Str,Type.Pack],templates:[1,0,0],names:["","Name","args"],constexprs:[1],isAnnotation:true,isPure:false,sideEffect:false,isConst:false,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:1,kind:OpcodeKind.None},
	{name:"trap",types:[Type.None,Type.Str],templates:[0,0],names:["","Reason"],constexprs:[1],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:false,terminator:true,bbTerminator:true,templateCount:0,kind:OpcodeKind.Trap},
	{name:"nop",types:[Type.Pack],templates:[0],names:[""],constexprs:[],isAnnotation:false,isPure:false,sideEffect:false,isConst:false,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:0,kind:OpcodeKind.None},
	{name:"unreachable",types:[Type.None],templates:[0],names:[""],constexprs:[],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:false,terminator:true,bbTerminator:true,templateCount:0,kind:OpcodeKind.Trap},
]
// prettier-ignore
export namespace Opcode {
    export function reflect(i:Opcode) : OpcodeDesc { return Opcode_DescTable[i]; }
    export function toString(i:Opcode) : string { return Opcode_DescTable[i].name; }
}
