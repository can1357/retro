import { Type } from "../ir";

// prettier-ignore
export enum RegKind {
	None        = 0,
	Gpr8        = 1,
	Gpr16       = 2,
	Gpr32       = 3,
	Gpr64       = 4,
	Gpr128      = 5,
	Fp32        = 6,
	Fp64        = 7,
	Fp80        = 8,
	Simd64      = 9,
	Simd128     = 10,
	Simd256     = 11,
	Simd512     = 12,
	Instruction = 13,
	Flag        = 14,
	Segment     = 15,
	Control     = 16,
	Misc        = 17,
	// PSEUDO
	MAX         = 17,
	BIT_WIDTH   = 5,
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                      Descriptors                                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// prettier-ignore
export class RegKindDesc {
	name:      string  = "";
	type:      Type    = Type.None;
	width:     number  = 0;
	isPointer: boolean = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                         Tables                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// prettier-ignore
const RegKind_DescTable: RegKindDesc[] = [
	new RegKindDesc(),
	{name:"gpr8",type:Type.I8,width:8,isPointer:false},
	{name:"gpr16",type:Type.I16,width:16,isPointer:false},
	{name:"gpr32",type:Type.I32,width:32,isPointer:false},
	{name:"gpr64",type:Type.I64,width:64,isPointer:false},
	{name:"gpr128",type:Type.I128,width:128,isPointer:false},
	{name:"fp32",type:Type.F32,width:32,isPointer:false},
	{name:"fp64",type:Type.F64,width:64,isPointer:false},
	{name:"fp80",type:Type.F80,width:80,isPointer:false},
	{name:"simd64",type:Type.I32x2,width:64,isPointer:false},
	{name:"simd128",type:Type.I32x4,width:128,isPointer:false},
	{name:"simd256",type:Type.I32x8,width:256,isPointer:false},
	{name:"simd512",type:Type.I32x16,width:512,isPointer:false},
	{name:"instruction",type:Type.Pointer,width:0,isPointer:true},
	{name:"flag",type:Type.I1,width:1,isPointer:false},
	{name:"segment",type:Type.I16,width:16,isPointer:false},
	{name:"control",type:Type.None,width:0,isPointer:false},
	{name:"misc",type:Type.None,width:0,isPointer:false},
]
// prettier-ignore
export namespace RegKind {
    export function reflect(i:RegKind) : RegKindDesc { return RegKind_DescTable[i]; }
    export function toString(i:RegKind) : string { return RegKind_DescTable[i].name; }
}
