declare module "*-Debug" {
	import { ImageKind } from "../lib/core/image_kind";
	import { RegKind } from "../lib/arch/reg_kind";
	import { Type } from "../lib/ir/builtin_types";
	import { Opcode } from "../lib/ir";

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
	declare class Insn extends RefCounted {
		//		get method(): ?Method;
		get routine(): ?Routine;
		get image(): ?Image;
		get workspace(): ?Workspace;
		get block(): ?BasicBlock;

		get arch(): ?Arch;
		get name(): number;
		get ip(): bigint;

		get opcode(): Opcode;
		get operandCount(): number;

		get templates(): Type[];

		get isOprhan(): boolean;

		validate();
		toString(full: boolean = false);
	}
	declare class BasicBlock extends RefCounted {
		//		get method(): ?Method;
		get routine(): ?Routine;
		get image(): ?Image;
		get workspace(): ?Workspace;

		get arch(): ?Arch;
		get name(): number;
		get ip(): bigint;
		get endIp(): bigint;

		get successors(): BasicBlock[];
		get predecessors(): BasicBlock[];
		get terminator(): ?Insn;
		get phis(): Iterator<Insn>;

		validate();
		toString(full: boolean = false);
		[Symbol.iterator](): Iterator<Insn>;
	}
	declare class Routine extends RefCounted {
		//		get method(): ?Method;
		get image(): ?Image;
		get workspace(): ?Workspace;

		get ip(): bigint;

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
