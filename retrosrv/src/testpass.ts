import { Builder, Type } from "./lib/ir";
import { Routine } from "./lib/native";
import { Pass } from "./lib/pass";

export class TestPass extends Pass {
	static async run(r: Routine) {
		for (const bb of r) {
			for (const ins of bb) {
				ins;
			}
		}
	}
}
TestPass.register();
