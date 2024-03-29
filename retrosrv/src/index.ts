import * as IR from "./lib/ir";
import { Image, ImageKind, Scheduler, Workspace } from "./lib/core";
import { TestPass } from "./testpass";

const TEXT_MIN = 0x00000000200000n;
const TEXT_MAX = 0x00000000c00000n;
const IMG_BASE = 0x140000000n;

const routineMap = new Map<number, Promise<IR.Routine | null>>();

const scheduler = Scheduler.create();

async function liftRecursive(img: Image, rva: number) {
	//console.log("Lifting sub_%s", (IMG_BASE + rva).toString(16));
	// Lift the routine.
	//
	const rtn = await img.lift(rva).queue(scheduler);
	if (!rtn) return null;

	// Recurse.
	//
	for (const va of rtn.getXrefs(img)) {
		const rva2 = Number(va - IMG_BASE);
		if (TEXT_MIN <= rva2 && rva2 <= TEXT_MAX) {
			if (!routineMap.has(rva2)) {
				routineMap.set(rva2, liftRecursive(img, rva2));
			}
		}
	}

	// Run test pass.
	//
	return await TestPass.schedule(rtn).then(() => rtn);
}

const path = "S:\\Dumps\\ntoskrnl_2004.exe";

const ws = Workspace.create();
const img = await ws.loadImage(path);
console.log("Kind:       ", ImageKind[img.kind]);
console.log("Arch:       ", img.arch.name);
console.log("ABI:        ", img.abiName);
console.log("Loader:     ", img.ldr.name);
console.log("Environment:", img.envName);
console.log("BaseAddress:", img.baseAddress.toString(16));

const t0 = process.uptime();

for (const ep of img.entryPoints) {
	routineMap.set(Number(ep), liftRecursive(img, Number(ep)));
}
/* TODO:
	for (auto& sym : img->symbols) {
		if (!sym.read_only_ignore)
			analyse_rva_if_code(img, sym.rva);
	}
	for (auto& reloc : img->relocs) {
		if (std::holds_alternative<u64>(reloc.target)) {
			analyse_rva_if_code(img, std::get<u64>(reloc.target));
		}
	}
	*/

let n = 0;
while (true) {
	await scheduler.wait();
	const x = await Promise.all(routineMap.values());
	if (x.length == n) {
		break;
	}
	console.log("Iteration lifted %d routines.", x.length);
	n = x.length;
}

const t1 = process.uptime();

console.log("Finished in %fs.", t1 - t0);

//const path = "S:\\Projects\\Retro\\tests\\loop.exe";
