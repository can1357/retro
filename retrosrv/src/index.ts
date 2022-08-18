import * as IR from "./lib/ir";
import * as Core from "./lib/core";
import * as Arch from "./lib/arch";
import { Clang } from "./lib/llvm";
import View from "./lib/view";
import Builder = IR.Builder;
import { Opcode, Type } from "./lib/ir";

const TEXT_MIN = 0x00000000200000n;
const TEXT_MAX = 0x00000000c00000n;
const IMG_BASE = 0x140000000n;

const routineMap = new Map<bigint, Promise<IR.Routine | null>>();

async function liftRecursive(img: Core.Image, rva: bigint) {
	//console.log("Lifting sub_%s", (IMG_BASE + rva).toString(16));
	// Lift the routine.
	//
	const rtn = await img.lift(rva);
	if (!rtn) return null;

	// Recurse.
	//
	for (const va of rtn.getXrefs(img)) {
		const rva2 = va - IMG_BASE;
		if (TEXT_MIN <= rva2 && rva2 <= TEXT_MAX) {
			if (!routineMap.has(rva2)) {
				routineMap.set(rva2, liftRecursive(img, rva2));
			}
		}
	}
	return rtn;
}

async function main() {
	const path = "S:\\Dumps\\ntoskrnl_2004.exe";

	const ws = Core.Workspace.create();
	const img = await ws.loadImage(path);
	console.log("Kind:       ", Core.ImageKind[img.kind]);
	console.log("Arch:       ", img.arch.name);
	console.log("ABI:        ", img.abiName);
	console.log("Loader:     ", img.ldr.name);
	console.log("Environment:", img.envName);
	console.log("BaseAddress:", img.baseAddress.toString(16));

	const t0 = process.uptime();

	for (const ep of img.entryPoints) {
		routineMap.set(ep, liftRecursive(img, ep));
	}

	let n = 0;
	while (true) {
		await ws.wait();
		const x = await Promise.all(routineMap.values());
		if (x.length == n) {
			break;
		}
		console.log("Iteration lifted %d routines.", x.length);
		n = x.length;
	}

	const t1 = process.uptime();

	console.log("Finished in %fs.", t1 - t0);
}
main()
	.catch(() => console.error)
	.finally(process.exit);

//const path = "S:\\Projects\\Retro\\tests\\loop.exe";
