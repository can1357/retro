interface Native {
	readonly bla: number;
}
const NATIVE_PATH = `..\\..\\build\\libretro-x64-Debug.node`;
const native = require(NATIVE_PATH) as Native;
console.log(native.bla);

export enum Hey {
	Bla = 5,
	Kla = 9,
}

import { Op } from "./native/ir/ops";

console.log(Op.toString(Op.reflect(Op.Lt).inverse));
