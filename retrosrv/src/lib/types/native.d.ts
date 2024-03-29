type MapFn<T, Ty> = (v: T) => Ty;
type PredFn<T> = (v: T) => boolean;
type EnumFn<T> = (v: T) => void;

declare class View<T> implements Iterable<T> {
	[Symbol.iterator](): Iterator<T>;
	toArray(): Array<T>;
	toString();
	forEach(fn: EnumFn<T>);
	find(fn: PredFn<T>): T | null;
	indexOf(val: T): number;
	all(fn: PredFn<T>): boolean;
	any(fn: PredFn<T>): boolean;
	at(i: number): T | null;
	filter(fn: PredFn<T>): View<T>;
	map<Ty>(fn: MapFn<T, Ty>): View<Ty>;
	skip(i: number): View<T>;
	take(i: number): View<T>;
	slice(offset: number, count: number): View<T>;
}

declare module LibRetro {
	import type { ImageKind } from "../core/image_kind";
	import type { RegKind } from "../arch/reg_kind";
	import type { Type } from "../ir/builtin_types";
	import type { Opcode } from "../ir/opcodes";
	import type { Intrinsic, Op } from "../ir/ops";

	// Generic types used for description of native equivalents.
	//
	declare class RefCounted {
		protected constructor();

		get refcount(): number;
		get unique(): boolean;
		get expired(): boolean;

		get comperator(): number;
		equals(other: any): boolean;
	}

	// Machine operands.
	//
	declare class MImm {
		protected constructor();

		get width(): number;
		get isSigned(): boolean;
		get isRelative(): boolean;
		get s(): bigint;
		get u(): bigint;

		getSigned(ip: ?(bigint | number)): bigint;
		getUnsigned(ip: ?(bigint | number)): bigint;

		toString(a: ?Arch = null, ip: ?(bigint | number) = null): string;
	}
	declare class MReg {
		protected constructor();

		static from(uid: number): MReg;

		get id(): number;
		get kind(): RegKind;
		get uid(): number;
		equals(o: MReg): boolean;
		get comperator(): number;

		getName(a: Arch): string;
		toString(a: ?Arch = null, ip: ?(bigint | number) = null): string;
	}
	declare class MMem {
		protected constructor();

		get width(): number;
		get segVal(): number;
		get seg(): ?MReg;
		get base(): ?Mreg;
		get index(): ?Mreg;
		get scale(): number;
		get disp(): bigint;

		equals(o: MMem): boolean;

		toString(a: ?Arch = null, ip: ?(bigint | number) = null): string;
	}
	type MOp = MReg | MMem | MImm;

	// Machine instruction type.
	//
	declare class MInsn {
		protected constructor();

		get name(): string;
		get arch(): ?Arch;
		get mnemonic(): number;
		get modifiers(): number;
		get effectiveWidth(): number;
		get length(): number;
		get operandCount(): number;
		get isSupervisor(): boolean;

		getOperand(idx: number): ?MOp;

		toString(ip: ?(bigint | number) = null): string;
	}

	// Z3 types.
	//
	type Variant = ?(Const | Insn);
	declare class Z3VariableSet {
		protected constructor();

		static create(): Z3VariableSet;
		unwrap(expr: Z3Expr): ?Value;
	}
	declare class Z3Expr {
		protected constructor();

		static I1(value: boolean): Z3Expr;
		static I8(value: number): Z3Expr;
		static I16(value: number): Z3Expr;
		static I32(value: number): Z3Expr;
		static I64(value: bigint): Z3Expr;
		static F32(value: number): Z3Expr;
		static F64(value: number): Z3Expr;

		get type(): Type;
		get depth(): number;
		get isConst(): boolean;
		get isVariable(): boolean;

		simplify(): Z3Expr;
		materialize(vs: Z3VariableSet, bb: BasicBlock): Variant;
		materializeAt(vs: Z3VariableSet, at: Insn): Variant;

		abs(): Z3Expr;
		neg(): Z3Expr;
		bitNot(): Z3Expr;
		sqrt(): Z3Expr;
		rem(rhs: Z3Expr): Z3Expr;
		urem(rhs: Z3Expr): Z3Expr;
		div(rhs: Z3Expr): Z3Expr;
		udiv(rhs: Z3Expr): Z3Expr;
		xor(rhs: Z3Expr): Z3Expr;
		or(rhs: Z3Expr): Z3Expr;
		and(rhs: Z3Expr): Z3Expr;
		shl(rhs: Z3Expr): Z3Expr;
		shr(rhs: Z3Expr): Z3Expr;
		sar(rhs: Z3Expr): Z3Expr;
		rol(rhs: Z3Expr): Z3Expr;
		ror(rhs: Z3Expr): Z3Expr;
		add(rhs: Z3Expr): Z3Expr;
		sub(rhs: Z3Expr): Z3Expr;
		mul(rhs: Z3Expr): Z3Expr;
		ge(rhs: Z3Expr): Z3Expr;
		uge(rhs: Z3Expr): Z3Expr;
		gt(rhs: Z3Expr): Z3Expr;
		ugt(rhs: Z3Expr): Z3Expr;
		lt(rhs: Z3Expr): Z3Expr;
		ult(rhs: Z3Expr): Z3Expr;
		le(rhs: Z3Expr): Z3Expr;
		ule(rhs: Z3Expr): Z3Expr;
		umax(rhs: Z3Expr): Z3Expr;
		max(rhs: Z3Expr): Z3Expr;
		umin(rhs: Z3Expr): Z3Expr;
		min(rhs: Z3Expr): Z3Expr;
		eq(rhs: Z3Expr): Z3Expr;
		ne(rhs: Z3Expr): Z3Expr;
		apply(op: Op, rhs: ?Z3Expr = null): Z3Expr;

		toString(): string;

		castZx(t: Type, ctx: ?Insn = null): Z3Expr;
		castSx(t: Type, ctx: ?Insn = null): Z3Expr;
		toConst(coerce: boolean = false): Const;

		get comperator(): number;
		equals(other: Z3Expr): boolean;
	}

	// IR types.
	//
	declare class Const {
		protected constructor();

		get type(): Type;
		get byteLength(): bigint;
		get buffer(): Buffer;
		get valid(): boolean;

		equals(o: Const): boolean;
		toString(): string;

		bitcast(t: Type): Const;
		castZx(t: Type): Const;
		castSx(t: Type): Const;
		apply(op: Op, rhs: ?Const = null): Const;

		toExpr(lane: number = 0): ?Z3Expr;

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
		static Ptr(value: bigint | number): Const;
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

		toExpr(vs: Z3VariableSet, maxDepth: ?number): ?Z3Expr;

		get constant(): Const;
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

		toExpr(vs: Z3VariableSet, maxDepth: ?number): ?Z3Expr;
	}
	declare class Insn extends Value {
		//		get method(): ?Method;
		get routine(): ?Routine;
		get image(): ?Image;
		get workspace(): ?Workspace;
		get block(): ?BasicBlock;

		get next(): ?Insn;
		get prev(): ?Insn;

		arch: ?Arch;
		name: number;
		ip: bigint;
		opcode: Opcode;
		templates: [Type, Type];

		// Generates a slice/reverse slice [from, to), end of block if to is not given.
		//
		slice(to: Insn | null = null): View<Insn>;
		rslice(to: Insn | null = null): View<Insn>;

		get operands(): View<Operand>;
		opr(i: number): Operand;
		get operandCount(): number;
		get isOprhan(): boolean;

		erase();
		push(at: BasicBlock): Insn;
		pushFront(at: BasicBlock): Insn;
		insert(at: Insn): Insn;
		insertAfter(at: Insn): Insn;

		indexOf(op: Operand): number;

		validate();

		static create(o: Opcode, tmp: Type[], ...operands: any[]): Insn;
	}
	declare class BasicBlock extends Value {
		refUnsafe(): number;
		static fromRefUnsafe(x: number): BasicBlock;

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

		get isExit(): boolean;
		dom(other: BasicBlock): boolean;
		postdom(other: BasicBlock): boolean;
		hasPathTo(other: BasicBlock): boolean;

		push(v: Insn): Insn;
		pushFront(v: Insn): Insn;

		addJump(to: BasicBlock);
		delJump(to: BasicBlock);

		validate();
		[Symbol.iterator](): Iterator<Insn>;
	}
	declare class Routine extends RefCounted {
		refUnsafe(): number;
		static fromRefUnsafe(x: number): Routine;

		//		get method(): ?Method;
		get image(): ?Image;
		get workspace(): ?Workspace;

		ip: bigint;

		validate();
		renameBlocks();
		renameInsns();
		topologicalSort();
		toString(full: boolean = false);

		addBlock(): BasicBlock;
		delBlock(bb: BasicBlock);

		getXrefs(img: Image): bigint[];

		get exits(): View<BasicBlock>;

		get entryPoint(): ?BasicBlock;
		[Symbol.iterator](): Iterator<BasicBlock>;

		static create(): Routine;
	}

	// Arch interface.
	//
	declare class Arch {
		protected constructor();

		disasm(data: Buffer): ?MInsn;
		lift(bb: BasicBlock, i: MInsn, ip: bigint | number);
		nameRegister(r: MReg): string;
		nameMnemonic(id: number): string;
		formatInsnModifiers(i: MInsn): string;
		get isBigEndian(): boolean;
		get isLittleEndian(): boolean;
		get ptrWidth(): number;
		get effectivePtrWidth(): number;
		get ptrType(): Type;
		get stackRegister(): MReg;

		get name(): string;
		static lookup(name: string): ?Arch;
		equals(other: any): boolean;
		get comperator(): number;
	}

	// Loader interface.
	//
	declare class Loader {
		protected constructor();

		match(img: Buffer): boolean;
		get extensions(): string[];

		get name(): string;
		static lookup(name: string): ?Loader;
		equals(other: any): boolean;
		get comperator(): number;
	}

	// Scheduler and tasks.
	//
	declare class Scheduler extends RefCounted {
		static create(): Scheduler;
		static getDefault(): Scheduler;

		clear();
		suspend();
		resume();
		async wait();

		get suspended(): boolean;
		get remainingTasks(): number;
	}
	declare class Task<T> extends RefCounted {
		get queued(): boolean;
		get pending(): boolean;
		get cancelled(): boolean;
		get done(): boolean;
		get success(): boolean;
		get error(): boolean;

		cancel(): boolean;
		queue(sc: ?Scheduler = null): Promise<T>;
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

		lift(rva: bigint | number): Task<?Routine>;

		slice(rva: bigint | number, length: bigint | number): Buffer;
	}

	// Workspace type.
	//
	declare class Workspace extends RefCounted {
		static create(): Workspace;
		get numImages(): number;

		async loadImage(path: string, ldr: ?Loader = null): Promise<Image>;
		async loadImageInMemory(data: Buffer, ldr: ?Loader = null): Promise<Image>;
	}

	// LLVM.
	//
	declare namespace Clang {
		function locate(): ?string;
		async function format(source: string, style: ?string = null): Promise<string>;
		async function compile(source: string, arguments?: string = null): Promise<Buffer>;
		async function compileTestCase(source: string, arguments?: string = null): Promise<Buffer>;
	}
}
