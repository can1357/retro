// prettier-ignore
export enum TypeKind {
	None      = 0,
	Memory    = 1,
	ScalarInt = 2,
	ScalarFp  = 3,
	VectorInt = 4,
	VectorFp  = 5,
	// PSEUDO
	MAX       = 5,
	BIT_WIDTH = 3,
}
// prettier-ignore
export enum Type {
	None      = 0,
	Pack      = 1,
	Context   = 2,
	Reg       = 3,
	Op        = 4,
	Intrinsic = 5,
	Label     = 6,
	Str       = 7,
	I1        = 8,
	I8        = 9,
	I16       = 10,
	I32       = 11,
	I64       = 12,
	I128      = 13,
	F32       = 14,
	F64       = 15,
	F80       = 16,
	Pointer   = 17,
	I32x2     = 18,
	I16x4     = 19,
	I8x8      = 20,
	F32x2     = 21,
	I64x2     = 22,
	I32x4     = 23,
	I16x8     = 24,
	I8x16     = 25,
	F64x2     = 26,
	F32x4     = 27,
	I64x4     = 28,
	I32x8     = 29,
	I16x16    = 30,
	I8x32     = 31,
	F64x4     = 32,
	F32x8     = 33,
	I64x8     = 34,
	I32x16    = 35,
	I16x32    = 36,
	I8x64     = 37,
	F64x8     = 38,
	F32x16    = 39,
	// PSEUDO
	MAX       = 39,
	BIT_WIDTH = 6,
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                      Descriptors                                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// prettier-ignore
export class TypeKindDesc {
	name: string = "";
}
// prettier-ignore
export class TypeDesc {
	name:       string   = "";
	bitSize:    number   = 0;
	pseudo:     boolean  = false;
	kind:       TypeKind = TypeKind.None;
	underlying: Type     = Type.None;
	laneWidth:  number   = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                         Tables                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// prettier-ignore
const TypeKind_DescTable: TypeKindDesc[] = [
	new TypeKindDesc(),
	{name:"memory"},
	{name:"scalar-int"},
	{name:"scalar-fp"},
	{name:"vector-int"},
	{name:"vector-fp"},
]
// prettier-ignore
export namespace TypeKind {
    export function reflect(i:TypeKind) : TypeKindDesc { return TypeKind_DescTable[i]; }
    export function toString(i:TypeKind) : string { return TypeKind_DescTable[i].name; }
}
// prettier-ignore
const Type_DescTable: TypeDesc[] = [
	new TypeDesc(),
	{name:"pack",bitSize:0,pseudo:true,kind:TypeKind.None,underlying:Type.None,laneWidth:0},
	{name:"context",bitSize:0,pseudo:true,kind:TypeKind.None,underlying:Type.None,laneWidth:0},
	{name:"reg",bitSize:0,pseudo:false,kind:TypeKind.None,underlying:Type.None,laneWidth:0},
	{name:"op",bitSize:0,pseudo:false,kind:TypeKind.None,underlying:Type.None,laneWidth:0},
	{name:"intrinsic",bitSize:0,pseudo:false,kind:TypeKind.None,underlying:Type.None,laneWidth:0},
	{name:"label",bitSize:0,pseudo:true,kind:TypeKind.None,underlying:Type.None,laneWidth:0},
	{name:"str",bitSize:0,pseudo:false,kind:TypeKind.None,underlying:Type.None,laneWidth:0},
	{name:"i1",bitSize:1,pseudo:false,kind:TypeKind.ScalarInt,underlying:Type.None,laneWidth:0},
	{name:"i8",bitSize:8,pseudo:false,kind:TypeKind.ScalarInt,underlying:Type.None,laneWidth:0},
	{name:"i16",bitSize:16,pseudo:false,kind:TypeKind.ScalarInt,underlying:Type.None,laneWidth:0},
	{name:"i32",bitSize:32,pseudo:false,kind:TypeKind.ScalarInt,underlying:Type.None,laneWidth:0},
	{name:"i64",bitSize:64,pseudo:false,kind:TypeKind.ScalarInt,underlying:Type.None,laneWidth:0},
	{name:"i128",bitSize:128,pseudo:false,kind:TypeKind.ScalarInt,underlying:Type.None,laneWidth:0},
	{name:"f32",bitSize:32,pseudo:false,kind:TypeKind.ScalarFp,underlying:Type.None,laneWidth:0},
	{name:"f64",bitSize:64,pseudo:false,kind:TypeKind.ScalarFp,underlying:Type.None,laneWidth:0},
	{name:"f80",bitSize:80,pseudo:false,kind:TypeKind.ScalarFp,underlying:Type.None,laneWidth:0},
	{name:"pointer",bitSize:64,pseudo:false,kind:TypeKind.Memory,underlying:Type.None,laneWidth:0},
	{name:"i32x2",bitSize:64,pseudo:false,kind:TypeKind.VectorInt,underlying:Type.I32,laneWidth:2},
	{name:"i16x4",bitSize:64,pseudo:false,kind:TypeKind.VectorInt,underlying:Type.I16,laneWidth:4},
	{name:"i8x8",bitSize:64,pseudo:false,kind:TypeKind.VectorInt,underlying:Type.I8,laneWidth:8},
	{name:"f32x2",bitSize:64,pseudo:false,kind:TypeKind.VectorFp,underlying:Type.F32,laneWidth:2},
	{name:"i64x2",bitSize:128,pseudo:false,kind:TypeKind.VectorInt,underlying:Type.I64,laneWidth:2},
	{name:"i32x4",bitSize:128,pseudo:false,kind:TypeKind.VectorInt,underlying:Type.I32,laneWidth:4},
	{name:"i16x8",bitSize:128,pseudo:false,kind:TypeKind.VectorInt,underlying:Type.I16,laneWidth:8},
	{name:"i8x16",bitSize:128,pseudo:false,kind:TypeKind.VectorInt,underlying:Type.I8,laneWidth:16},
	{name:"f64x2",bitSize:128,pseudo:false,kind:TypeKind.VectorFp,underlying:Type.F64,laneWidth:2},
	{name:"f32x4",bitSize:128,pseudo:false,kind:TypeKind.VectorFp,underlying:Type.F32,laneWidth:4},
	{name:"i64x4",bitSize:256,pseudo:false,kind:TypeKind.VectorInt,underlying:Type.I64,laneWidth:4},
	{name:"i32x8",bitSize:256,pseudo:false,kind:TypeKind.VectorInt,underlying:Type.I32,laneWidth:8},
	{name:"i16x16",bitSize:256,pseudo:false,kind:TypeKind.VectorInt,underlying:Type.I16,laneWidth:16},
	{name:"i8x32",bitSize:256,pseudo:false,kind:TypeKind.VectorInt,underlying:Type.I8,laneWidth:32},
	{name:"f64x4",bitSize:256,pseudo:false,kind:TypeKind.VectorFp,underlying:Type.F64,laneWidth:4},
	{name:"f32x8",bitSize:256,pseudo:false,kind:TypeKind.VectorFp,underlying:Type.F32,laneWidth:8},
	{name:"i64x8",bitSize:512,pseudo:false,kind:TypeKind.VectorInt,underlying:Type.I64,laneWidth:8},
	{name:"i32x16",bitSize:512,pseudo:false,kind:TypeKind.VectorInt,underlying:Type.I32,laneWidth:16},
	{name:"i16x32",bitSize:512,pseudo:false,kind:TypeKind.VectorInt,underlying:Type.I16,laneWidth:32},
	{name:"i8x64",bitSize:512,pseudo:false,kind:TypeKind.VectorInt,underlying:Type.I8,laneWidth:64},
	{name:"f64x8",bitSize:512,pseudo:false,kind:TypeKind.VectorFp,underlying:Type.F64,laneWidth:8},
	{name:"f32x16",bitSize:512,pseudo:false,kind:TypeKind.VectorFp,underlying:Type.F32,laneWidth:16},
]
// prettier-ignore
export namespace Type {
    export function reflect(i:Type) : TypeDesc { return Type_DescTable[i]; }
    export function toString(i:Type) : string { return Type_DescTable[i].name; }
}
