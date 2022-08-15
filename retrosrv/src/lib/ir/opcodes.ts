import { MReg } from "../arch";
import { Value, Insn, Const, Op, Intrinsic, Type } from "../ir";

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
	{name:"intrinsic",types:[Type.Pack,Type.Intrinsic,Type.Pack],templates:[0,0,0],names:["","func","args"],constexprs:[1],isAnnotation:false,isPure:false,sideEffect:false,isConst:false,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:0,kind:OpcodeKind.Intrinsic},
	{name:"sideeffect_intrinsic",types:[Type.Pack,Type.Intrinsic,Type.Pack],templates:[0,0,0],names:["","func","args"],constexprs:[1],isAnnotation:false,isPure:false,sideEffect:true,isConst:false,unkRegUse:false,terminator:false,bbTerminator:false,templateCount:0,kind:OpcodeKind.Intrinsic},
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
// prettier-ignore
export namespace Builder {
	type Variant = Value | Const;
	export function StackBegin(): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [];
		return Insn.create(Opcode.StackBegin, t, o);
	}
	export function StackReset(sp: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [sp];
		return Insn.create(Opcode.StackReset, t, o);
	}
	export function ReadReg(t0: Type, regid: MReg): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [Const.MReg(regid)];
		t[0] = t0;
		return Insn.create(Opcode.ReadReg, t, o);
	}
	export function WriteReg(regid: MReg, value: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [Const.MReg(regid), value];
		t[0] = o[1].type;
		return Insn.create(Opcode.WriteReg, t, o);
	}
	export function LoadMem(t0: Type, pointer: Variant, offset: bigint): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [pointer, Const.I64(offset)];
		t[0] = t0;
		return Insn.create(Opcode.LoadMem, t, o);
	}
	export function StoreMem(pointer: Variant, offset: bigint, value: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [pointer, Const.I64(offset), value];
		t[0] = o[2].type;
		return Insn.create(Opcode.StoreMem, t, o);
	}
	export function Undef(t0: Type): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [];
		t[0] = t0;
		return Insn.create(Opcode.Undef, t, o);
	}
	export function Poison(t0: Type, reason: string): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [Const.Str(reason)];
		t[0] = t0;
		return Insn.create(Opcode.Poison, t, o);
	}
	export function Extract(t1: Type, vector: Variant, lane: number): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [vector, Const.I32(lane)];
		t[1] = t1;
		t[0] = o[0].type;
		return Insn.create(Opcode.Extract, t, o);
	}
	export function Insert(vector: Variant, lane: number, element: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [vector, Const.I32(lane), element];
		t[0] = o[0].type;
		t[1] = o[2].type;
		return Insn.create(Opcode.Insert, t, o);
	}
	export function ContextBegin(sp: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [sp];
		return Insn.create(Opcode.ContextBegin, t, o);
	}
	export function ExtractContext(t0: Type, ctx: Variant, regid: MReg): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [ctx, Const.MReg(regid)];
		t[0] = t0;
		return Insn.create(Opcode.ExtractContext, t, o);
	}
	export function InsertContext(ctx: Variant, regid: MReg, element: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [ctx, Const.MReg(regid), element];
		t[0] = o[2].type;
		return Insn.create(Opcode.InsertContext, t, o);
	}
	export function CastSx(t1: Type, value: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [value];
		t[1] = t1;
		t[0] = o[0].type;
		return Insn.create(Opcode.CastSx, t, o);
	}
	export function Cast(t1: Type, value: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [value];
		t[1] = t1;
		t[0] = o[0].type;
		return Insn.create(Opcode.Cast, t, o);
	}
	export function Bitcast(t1: Type, value: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [value];
		t[1] = t1;
		t[0] = o[0].type;
		return Insn.create(Opcode.Bitcast, t, o);
	}
	export function Binop(op: Op, lhs: Variant, rhs: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [Const.Op(op), lhs, rhs];
		t[0] = o[1].type;
		return Insn.create(Opcode.Binop, t, o);
	}
	export function Unop(op: Op, rhs: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [Const.Op(op), rhs];
		t[0] = o[1].type;
		return Insn.create(Opcode.Unop, t, o);
	}
	export function AtomicCmpxchg(ptr: Variant, expected: Variant, desired: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [ptr, expected, desired];
		t[0] = o[1].type;
		return Insn.create(Opcode.AtomicCmpxchg, t, o);
	}
	export function AtomicXchg(ptr: Variant, desired: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [ptr, desired];
		t[0] = o[1].type;
		return Insn.create(Opcode.AtomicXchg, t, o);
	}
	export function AtomicBinop(op: Op, lhs_ptr: Variant, rhs: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [Const.Op(op), lhs_ptr, rhs];
		t[0] = o[2].type;
		return Insn.create(Opcode.AtomicBinop, t, o);
	}
	export function AtomicUnop(t0: Type, op: Op, rhs_ptr: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [Const.Op(op), rhs_ptr];
		t[0] = t0;
		return Insn.create(Opcode.AtomicUnop, t, o);
	}
	export function Cmp(op: Op, lhs: Variant, rhs: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [Const.Op(op), lhs, rhs];
		t[0] = o[1].type;
		return Insn.create(Opcode.Cmp, t, o);
	}
	export function Phi(t0: Type, ...incoming: Variant[]): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [...incoming];
		t[0] = t0;
		return Insn.create(Opcode.Phi, t, o);
	}
	export function Select(cc: Variant, tv: Variant, fv: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [cc, tv, fv];
		t[0] = o[1].type;
		return Insn.create(Opcode.Select, t, o);
	}
	export function Xcall(destination: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [destination];
		return Insn.create(Opcode.Xcall, t, o);
	}
	export function Call(destination: Variant, ctx: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [destination, ctx];
		return Insn.create(Opcode.Call, t, o);
	}
	export function Intrinsic(func: Intrinsic, ...args: Variant[]): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [Const.Intrinsic(func), ...args];
		return Insn.create(Opcode.Intrinsic, t, o);
	}
	export function SideeffectIntrinsic(func: Intrinsic, ...args: Variant[]): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [Const.Intrinsic(func), ...args];
		return Insn.create(Opcode.SideeffectIntrinsic, t, o);
	}
	export function Xjmp(destination: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [destination];
		return Insn.create(Opcode.Xjmp, t, o);
	}
	export function Jmp(destination: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [destination];
		return Insn.create(Opcode.Jmp, t, o);
	}
	export function Xjs(cc: Variant, tb: bigint, fb: bigint): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [cc, Const.Ptr(tb), Const.Ptr(fb)];
		return Insn.create(Opcode.Xjs, t, o);
	}
	export function Js(cc: Variant, tb: Variant, fb: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [cc, tb, fb];
		return Insn.create(Opcode.Js, t, o);
	}
	export function Xret(ptr: Variant): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [ptr];
		return Insn.create(Opcode.Xret, t, o);
	}
	export function Ret(ctx: Variant, offset: bigint): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [ctx, Const.I64(offset)];
		return Insn.create(Opcode.Ret, t, o);
	}
	export function Annotation(t0: Type, name: string, ...args: Variant[]): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [Const.Str(name), ...args];
		t[0] = t0;
		return Insn.create(Opcode.Annotation, t, o);
	}
	export function Trap(reason: string): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [Const.Str(reason)];
		return Insn.create(Opcode.Trap, t, o);
	}
	export function Nop(): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [];
		return Insn.create(Opcode.Nop, t, o);
	}
	export function Unreachable(): Insn {
		const t: Type[] = [Type.None, Type.None];
		const o: Variant[] = [];
		return Insn.create(Opcode.Unreachable, t, o);
	}
}
