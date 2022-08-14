/// <reference path="types/native.d.ts"/>
import * as Retro from "../../build/libretro-x64-Debug";

import { ImageKind } from "./lib/core/image_kind";
import { Opcode } from "./lib/ir";

async function test() {
	console.log(await Retro.Clang.compile("int main() { int x = 0; return 6; }"));
	console.log(await Retro.Clang.format("int main() { int x = 0; return 6; }"));

	/*
	const ws = Retro.Workspace.create();

	const path = "S:\\Projects\\Retro\\tests\\loop.exe";
	const img = await ws.loadImage(path);

	console.log(img.name);


	const [ep] = img.entryPoints;

	const rtn = await img.lift(ep);

	for (const bb of rtn!) {
		console.log("BasicBlock:", bb.name);
		for (const i of bb) {
			console.log(i.toString(true));
		}
	}
	//	console.log(rtn?.toString(true));*/

	/*
	console.log(ImageKind[img.kind]);
	console.log(img.baseAddress.toString(16));
	console.log(img.ldr.name);
	console.log(img.ldr.extensions);
	console.log("Arch:", img.arch.name);

	const jmp = Buffer.from([0xe8, 0x00, 0x00, 0x00, 0x00]);
	const i = img.arch.disasm(jmp);
	console.log("disasm:", i.name);
	console.log("disasm:", i.toString());

	for (let n = 0; n != i?.operandCount; n++) {
		console.log("Operand ", n, ":", i?.getOperand(n)?.toString());
	}

	console.log(img.abiName);
	console.log(img.envName);
	console.log(img.isEnvSupervisor);
	console.log(img.entryPoints);*/
}

test().then(() => console.log("---------------"));
