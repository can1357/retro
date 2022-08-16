import * as IR from "./lib/ir";
import * as Core from "./lib/core";
import * as Arch from "./lib/arch";
import { Clang } from "./lib/llvm";
import View from "./lib/view";
import Builder = IR.Builder;

(async () => {
	// Create the test block.
	//
	const testRoutine = IR.Routine.create();
	const testBlock = testRoutine.addBlock();

	// Add the instructions.
	//
	const testAnd = Builder.Binop(IR.Op.BitAnd, IR.Const.I32(1), IR.Const.I32(5)).push(testBlock);
	Builder.Binop(IR.Op.BitOr, testAnd, IR.Const.I32(9)).push(testBlock);
	Builder.Xret(IR.Const.Ptr(0n)).push(testBlock);
	console.log("%s", testRoutine.toString(true));

	// Run demo constant folding.
	//
	let optCount = 0;
	const ilist = View.from(testBlock).filter((i) => i.opcode == IR.Opcode.Binop);
	for (const i of ilist) {
		const [op, lhs, rhs] = i.operands.map((k) => k.isConst && k.constant);
		if (op && lhs && rhs) {
			optCount += i.replaceAllUsesWith(lhs.apply(op.asOp(), rhs));
		}
	}
	console.log("Optimized %d instructions.", optCount);
	console.log("%s", testRoutine.toString(true));

	// Disasm demo.
	//
	const amd64 = Arch.Interface.lookup("x86_64")!;
	console.log("%s: %s\n", amd64.name, amd64.disasm(Buffer.from([0xe8, 0x00, 0x00, 0x00, 0x00]))?.toString());

	// Clang demo.
	//
	const src = await Clang.format("int main() { int x = 0; return 6; }");
	console.log(src);
	const bin = await Clang.compile(src);

	const ws = Core.Workspace.create();
	const img = await ws.loadImageInMemory(bin);
	console.log("Kind:       ", Core.ImageKind[img.kind]);
	console.log("Arch:       ", img.arch.name);
	console.log("ABI:        ", img.abiName);
	console.log("Loader:     ", img.ldr.name);
	console.log("Environment:", img.envName);
	console.log("BaseAddress:", img.baseAddress.toString(16));

	const rtn = await img.lift(img.entryPoints[0]);
	for (const bb of rtn!) {
		console.log("BasicBlock:", bb.name);
		for (const i of bb) {
			console.log(i.toString(true), "->", IR.Opcode[i.opcode]);

			for (const u of i.uses) {
				if (u.user instanceof IR.Insn) {
					i.replaceAllUsesWith(i);
					i.replaceAllUsesWith(IR.Const.Ptr(0n));

					const idx = u.user.indexOf(u);
					console.log("User(Operand #%d): %s", idx, u.user.toString(true));
					console.log("--> %s", u.user.opr(idx).toString());
					break;
				}
			}
			break;
		}
		break;
	}
})().catch(() => console.error);
//const path = "S:\\Projects\\Retro\\tests\\loop.exe";
