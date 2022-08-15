declare module "*-Debug" {
	import { ImageKind } from "../lib/core/image_kind";
	import { RegKind } from "../lib/arch/reg_kind";
	import { Type } from "../lib/ir/builtin_types";
	import { Opcode, Intrinsic, Op } from "../lib/ir";

	// Generic types used for description of native equivalents.
	//
	declare class RefCounted {
		private constructor();

		get refcount(): number;
		get unique(): boolean;
		get expired(): boolean;

		equals(other: any): boolean;
	}
	declare class IFace<T> {
		private constructor();

		get name(): string;
		static lookup(name: string): ?T;

		equals(other: any): boolean;
	}

	// Machine operands.
	//
	declare class MImm {
		private constructor();

		get width(): number;
		get isSigned(): boolean;
		get isRelative(): boolean;
		get s(): bigint;
		get u(): bigint;

		getSigned(ip: ?bigint): bigint;
		getUnsigned(ip: ?bigint): bigint;

		toString(a: ?Arch = null, ip: ?bigint = null): string;
	}
	declare class MReg {
		private constructor();

		get id(): number;
		get kind(): RegKind;
		get uid(): number;
		equals(o: MReg): boolean;

		getName(a: Arch): string;
		toString(a: ?Arch = null, ip: ?bigint = null): string;
	}
	declare class MMem {
		private constructor();

		get width(): number;
		get segVal(): number;
		get seg(): ?MReg;
		get base(): ?Mreg;
		get index(): ?Mreg;
		get scale(): number;
		get disp(): bigint;

		equals(o: MMem): boolean;

		toString(a: ?Arch = null, ip: ?bigint = null): string;
	}
	type MOp = MReg | MMem | MImm;

	// Machine instruction type.
	//
	declare class MInsn {
		private constructor();

		get name(): string;
		get arch(): ?Arch;
		get mnemonic(): number;
		get modifiers(): number;
		get effectiveWidth(): number;
		get length(): number;
		get operandCount(): number;
		get isSupervisor(): boolean;

		getOperand(idx: number): ?MOp;

		toString(ip: ?bigint = null): string;
	}

	// IR types.
	//
	declare class Const {
		private constructor();

		get type(): Type;
		get byteLength(): bigint;
		get buffer(): Buffer;

		equals(o: Const): boolean;
		toString(): string;

		bitcast(t: Type): Const;
		castZx(t: Type): Const;
		castSx(t: Type): Const;
		apply(op: Op, rhs: ?Const = null): Const;

		get i64(): bigint;
		get u64(): bigint;

		asI1(): boolean;
		static I1(value: boolean): Const;
		asI8(): number;
		static I8(value: number): Const;
		asI16(): number;
		static I16(value: number): Const;
		asI32(): number;
		static I32(value: number): Const;
		asI64(): bigint;
		static I64(value: bigint): Const;
		asF32(): number;
		static F32(value: number): Const;
		asF64(): number;
		static F64(value: number): Const;
		asStr(): string;
		static Str(value: string): Const;
		asMReg(): MReg;
		static MReg(value: MReg): Const;
		asOp(): Op;
		static Op(value: Op): Const;
		asIntrinsic(): Intrinsic;
		static Intrinsic(value: Intrinsic): Const;
		asPtr(): bigint;
		static Ptr(value: bigint): Const;
		asF32x16(): number[];
		static F32x16(value: number[]): Const;
		asF32x2(): number[];
		static F32x2(value: number[]): Const;
		asI16x32(): number[];
		static I16x32(value: number[]): Const;
		asI16x4(): number[];
		static I16x4(value: number[]): Const;
		asF32x4(): number[];
		static F32x4(value: number[]): Const;
		asF32x8(): number[];
		static F32x8(value: number[]): Const;
		asF64x2(): number[];
		static F64x2(value: number[]): Const;
		asF64x4(): number[];
		static F64x4(value: number[]): Const;
		asF64x8(): number[];
		static F64x8(value: number[]): Const;
		asI16x16(): number[];
		static I16x16(value: number[]): Const;
		asI16x8(): number[];
		static I16x8(value: number[]): Const;
		asI32x16(): number[];
		static I32x16(value: number[]): Const;
		asI32x2(): number[];
		static I32x2(value: number[]): Const;
		asI32x4(): number[];
		static I32x4(value: number[]): Const;
		asI32x8(): number[];
		static I32x8(value: number[]): Const;
		asI64x2(): bigint[];
		static I64x2(value: bigint[]): Const;
		asI64x4(): bigint[];
		static I64x4(value: bigint[]): Const;
		asI64x8(): bigint[];
		static I64x8(value: bigint[]): Const;
		asI8x16(): number[];
		static I8x16(value: number[]): Const;
		asI8x32(): number[];
		static I8x32(value: number[]): Const;
		asI8x64(): number[];
		static I8x64(value: number[]): Const;
		asI8x8(): number[];
		static I8x8(value: number[]): Const;
	}
	declare class Operand extends RefCounted {
		get type(): Type;
		toString(full: boolean = false);
		equals(o: Operand): boolean;

		get Const(): Const;
		get value(): Value;

		get isConst(): boolean;
		get isValue(): boolean;
		get user(): Value;

		set(o: Const | Value);
	}
	declare class Value extends RefCounted {
		get type(): Type;
		toString(full: boolean = false);

		get uses(): Iterable<Operand>;
		replaceAllUsesWith(o: Const | Value): number;
	}
	declare class Insn extends Value {
		//		get method(): ?Method;
		get routine(): ?Routine;
		get image(): ?Image;
		get workspace(): ?Workspace;
		get block(): ?BasicBlock;

		arch: ?Arch;
		name: number;
		ip: bigint;
		opcode: Opcode;
		templates: Type[]; /*[2]*/

		operand(i: number): ?Operand;

		get operandCount(): number;
		get isOprhan(): boolean;

		indexOf(op: Operand): number;

		validate();

		static create(o: Opcode, tmp: Type[], ...operands: any[]): Insn;
	}
	declare class BasicBlock extends Value {
		//		get method(): ?Method;
		get routine(): ?Routine;
		get image(): ?Image;
		get workspace(): ?Workspace;

		arch: ?Arch;
		name: number;
		ip: bigint;
		endIp: bigint;

		get successors(): BasicBlock[];
		get predecessors(): BasicBlock[];
		get terminator(): ?Insn;
		get phis(): Iterable<Insn>;

		validate();
		[Symbol.iterator](): Iterator<Insn>;
	}
	declare class Routine extends RefCounted {
		//		get method(): ?Method;
		get image(): ?Image;
		get workspace(): ?Workspace;

		ip: bigint;

		validate();
		renameBlocks();
		renameInsns();
		topologicalSort();
		toString(full: boolean = false);

		get entryPoint(): ?BasicBlock;
		[Symbol.iterator](): Iterator<BasicBlock>;
	}

	// Arch interface.
	//
	declare class Arch extends IFace<Arch> {
		disasm(data: Buffer): ?MInsn;
		lift(bb: BasicBlock, i: MInsn, ip: bigint);
		nameRegister(r: MReg): string;
		nameMnemonic(id: number): string;
		formatInsnModifiers(i: MInsn): string;
		get isBigEndian(): boolean;
		get isLittleEndian(): boolean;
		get ptrWidth(): number;
		get effectivePtrWidth(): number;
		get ptrType(): Type;
		get stackRegister(): MReg;
	}

	// Loader interface.
	//
	declare class Loader extends IFace<Loader> {
		match(img: Buffer): boolean;
		get extensions(): string[];
	}

	// Image instance.
	//
	declare class Image extends RefCounted {
		get name(): string;
		get kind(): ImageKind;
		get baseAddress(): bigint;
		get ldr(): Loader;
		get arch(): Arch;
		get abiName(): string;
		get envName(): string;
		get isEnvSupervisor(): boolean;
		get entryPoints(): bigint[];

		async lift(rva: bigint): Promise<?Routine>;

		slice(rva: bigint, length: bigint): Buffer;
	}

	// Workspace type.
	//
	declare class Workspace extends RefCounted {
		static create(): Workspace;
		get numImages(): number;

		async loadImage(path: string, ldr: ?Loader = null): Image;
		async loadImageInMemory(data: Buffer, ldr: ?Loader = null): Image;
	}

	// LLVM.
	//
	declare namespace Clang {
		function locate(): ?string;
		async function compile(source: string, arguments?: string = null): Buffer;
		async function format(source: string, style: ?string = null): string;
	}
}
