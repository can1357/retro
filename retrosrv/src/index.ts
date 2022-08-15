import * as IR from "./lib/ir";
import * as Core from "./lib/core";
import { Clang } from "./lib/llvm";

/*
let x = Retro.Const.F32x2([5.0, 6.0]);
console.log(x.toString());
x = x.castZx(Type.I32x2);
console.log(x.toString());
x = x.apply(Op.BitNot);
console.log(x.toString());
console.log(x.asI32x2());*/

const ins = IR.Builder.Binop(IR.Op.BitAnd, IR.Const.I32(1), IR.Const.I32(5));
console.log(ins.toString(true));

async function test() {
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

	const [ep, ...rest] = img.entryPoints;
	const rtn = await img.lift(ep);

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
					console.log("--> %s", u.user.operand(idx)!.toString());
					break;
				}
			}
			break;
		}
		break;
	}

	/*
	const jmp = Buffer.from([0xe8, 0x00, 0x00, 0x00, 0x00]);
	const i = img.arch.disasm(jmp);
	console.log("disasm:", i.name);
	console.log("disasm:", i.toString());*/
}

test().then(() => console.log("---------------"));

//const path = "S:\\Projects\\Retro\\tests\\loop.exe";
