import { Routine } from "./native";
import * as Threads from "node:worker_threads";
import { stacktrace } from "./util";

//
// TODO: Cleanup
//

const THREAD_COUNT = 8;

class Transaction<T> implements PromiseLike<T> {
	#deleter: () => void;
	#res: ((value: T) => any) | null;
	#rej: ((value: any) => any) | null;
	#val: T | undefined;
	#exc: any;

	constructor(deleter: () => void) {
		this.#res = null;
		this.#rej = null;
		this.#deleter = deleter || (() => {});
	}

	isSettled() {
		return this.isResolved() || this.isRejected();
	}
	isResolved() {
		return this.#val !== undefined;
	}
	isRejected() {
		return this.#exc !== undefined;
	}

	resolve(value: T) {
		if (this.isSettled()) {
			return;
		}
		this.#deleter();
		this.#val = value;
		if (this.#res) {
			this.#res(value);
			this.#res = null;
		}
	}

	reject(value: any) {
		if (this.isSettled()) {
			return;
		}
		this.#deleter();
		this.#exc = value;
		if (this.#rej) {
			this.#rej(value);
			this.#rej = null;
		}
	}

	then<Tx = T, Ty = never>(resolve?: (value: T) => Tx, reject?: (reason: any) => Ty): Transaction<Tx | Ty> {
		if (this.isResolved()) {
			if (resolve) resolve(this.#val!);
		} else if (this.isRejected()) {
			if (reject) reject(this.#exc!);
		} else {
			if (resolve) {
				const prev = this.#res;
				if (prev) {
					this.#res = (v) => resolve(prev(v));
				} else {
					this.#res = resolve;
				}
			}
			if (reject) {
				const prev = this.#rej;
				if (prev) {
					this.#rej = (v) => reject(prev(v));
				} else {
					this.#rej = reject;
				}
			}
		}
		// This changes type at this point, so hack.
		return this as any as Transaction<Tx | Ty>;
	}
}
class TransactionManager {
	pending = new Map<number, Transaction<unknown>>();
	txCount = 0;

	create() {
		const idx = ++this.txCount;
		const tx = new Transaction<unknown>(() => this.pending.delete(idx));
		this.pending.set(idx, tx);
		return { tx, idx };
	}
	lookup(idx: number) {
		return this.pending.get(idx);
	}
	clear() {
		this.pending.forEach((val) => {
			try {
				val.reject(Error("Transaction destroyed."));
			} catch (_) {}
		});
		this.pending.clear();
	}
}

interface PassInfo {
	instance: any;
	fileName: string;
}
interface PassResult {
	idx: number;
	err?: any;
	res?: any;
}
interface PassRequest {
	idx: number;
	name: string;
	file: string;
	rtn: number;
}

const registry = new Map<string, PassInfo>();
const workers: Threads.Worker[] = [];
const mgr = new TransactionManager();

export class Pass {
	static async run(r: Routine) {}

	static schedule(r: Routine): PromiseLike<void> {
		if (!Threads.isMainThread) {
			return this.run(r);
		} else {
			const pi = registry.get(this.name)!;
			const { tx, idx } = mgr.create();
			const wi = idx % THREAD_COUNT;

			workers[wi].postMessage({
				name: pi.instance.name,
				file: pi.fileName,
				rtn: r.refUnsafe(),
				idx: idx,
			});

			return tx as Transaction<void>;
		}
	}

	static register() {
		const className = this.name;
		if (registry.has(className)) {
			throw Error(`Pass ${className} is already registered!`);
		}

		let fileName = "?";
		if (Threads.isMainThread) {
			fileName = stacktrace()[2].getFileName()!;
			console.log(`Registering ${className} @ ${fileName}.`);
		}
		registry.set(className, { instance: this, fileName });
	}
}

import url from "node:url";
if (Threads.isMainThread) {
	for (let n = 0; n != THREAD_COUNT; n++) {
		const worker = new Threads.Worker(url.fileURLToPath(new URL(".", import.meta.url)));
		workers.push(worker);

		worker.on("message", (msg: PassResult) => {
			const tx = mgr.lookup(msg.idx);
			if (msg.res !== undefined) {
				tx!.resolve(msg.res);
			} else {
				tx!.reject(msg.err ?? "Unknown error");
			}
		});
	}
} else {
	Threads.parentPort!.on("message", (msg: PassRequest) => {
		let entry = registry.get(msg.name);
		if (!entry) {
			require(msg.file);
			entry = registry.get(msg.name);
		}

		entry!.instance
			.run(Routine.fromRefUnsafe(msg.rtn))
			.then(() => {
				Threads.parentPort!.postMessage({
					idx: msg.idx,
					res: null,
				});
			})
			.catch((err: any) => {
				Threads.parentPort!.postMessage({
					idx: msg.idx,
					err: err,
				});
			});
	});
}
