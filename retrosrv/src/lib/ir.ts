// Define extensions.
//
import { Insn } from "./native";
import * as Support from "./support";
import View from "./view";
Support.extendClass(
	Insn,
	class extends Insn {
		get operands() {
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
