// Define extensions.
//
import { BasicBlock, Insn, Operand, Routine } from "./native";
import * as Support from "./support";
import View from "./view";

function* sliceGenerator(from: Insn, to: Insn | null) {
	if (to) {
		if (!from.block!.equals(to.block)) {
			throw Error("Invalid slice.");
		}
	}
	let it: Insn | null = from;
	while (true) {
		if (to) {
			if (!it) {
				throw Error("Invalid slice.");
			}
			if (it!.equals(to)) {
				break;
			}
		} else if (!it) {
			break;
		}
		yield it!;
		it = it!.next;
	}
}
function* rsliceGenerator(from: Insn, to: Insn | null) {
	if (to) {
		if (!from.block!.equals(to.block)) {
			throw Error("Invalid slice.");
		}
	}
	let it: Insn | null = from;
	while (true) {
		if (to) {
			if (!it) {
				throw Error("Invalid slice.");
			}
			if (it!.equals(to)) {
				break;
			}
		} else if (!it) {
			break;
		}
		yield it!;
		it = it!.prev;
	}
}

Support.extendClass(
	Routine,
	class extends Routine {
		get exits(): View<BasicBlock> {
			return View.from(this).filter((b) => b.isExit);
		}
	}
);

Support.extendClass(
	Insn,
	class extends Insn {
		slice(to: Insn | null = null): View<Insn> {
			return View.from(sliceGenerator(this, to));
		}
		rslice(to: Insn | null = null): View<Insn> {
			return View.from(rsliceGenerator(this, to));
		}
		get operands(): View<Operand> {
			const self = this;
			const n = this.operandCount;
			return View.from(
				(function* () {
					for (let i = 0; i != n; i++) {
						yield self.opr(i);
					}
				})()
			);
		}
	}
);

export { Const, Operand, Value, Insn, BasicBlock, Routine } from "./native";
export * from "./ir/builtin_types";
export * from "./ir/ops";
export * from "./ir/opcodes";
