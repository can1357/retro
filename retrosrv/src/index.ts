interface Native {
	readonly bla: number;
	abcd(x: number, c: (q: number) => number): number;

	blaInstance: any;
}

/// <reference path="native.d.ts"/>
import * as Lib from "../../build/libretro-x64-Debug";

//const native = require("../../build/libretro-x64-Debug.node");

console.log(Lib.abcd(5, (x) => 1));
console.log(Lib);

for (const k of Lib.blaInstance) {
	console.log(k);
}
/*
async function lolz() {
	console.log("async stuff start");
	const x = Lib.blaInstance.asyncStuff(5);
	console.log("async stuff ongoing");
	const y = await x;
	console.log("async stuff end", y);
}

lolz().then(() => {
	console.log("End!");
});*/

//import * as IR from "./lib/ir";

/*class Iterable {
	x: number;
	y: number;

	[Symbol.iterator]() {
		return {
			next: () => ({
				done: this.x === this.y,
				value: this.x++,
			}),
		};
	}
}*/
