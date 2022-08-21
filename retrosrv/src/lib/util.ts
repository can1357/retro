import View from "./view";

// Native class extension.
//
export function extendClass(nativeType: any, extensionType: any) {
	for (const [k, v] of Object.entries(Object.getOwnPropertyDescriptors(extensionType.prototype))) {
		if (k != "constructor") {
			Object.defineProperty(nativeType.prototype, k, v);
		}
	}
}

// Native object aware Map and Set alternatives.
//
interface HasComperator {
	get comperator(): number;
}
export class NMap<K extends HasComperator, V> {
	#data: Map<number, [K, V]>;

	constructor(data?: Iterable<[K, V]>) {
		if (!data) {
			this.#data = new Map();
		} else {
			this.#data = new Map(View.from(data).map(([k, v]) => [k.comperator, [k, v]]));
		}
	}

	get size() {
		return this.#data.size;
	}
	clear() {
		this.#data.clear();
	}
	delete(key: K): boolean {
		return this.#data.delete(key.comperator);
	}
	get(key: K): V | undefined {
		return this.#data.get(key.comperator)?.[1];
	}
	has(key: K): boolean {
		return this.#data.has(key.comperator);
	}
	set(key: K, value: V): this {
		this.#data.set(key.comperator, [key, value]);
		return this;
	}
	*entries(): Iterator<[K, V]> {
		for (const [_, pair] of this.#data) {
			yield pair;
		}
	}
	*keys(): Iterator<K> {
		for (const [_, [k, v]] of this.#data) {
			yield k;
		}
	}
	*values(): Iterator<V> {
		for (const [_, [k, v]] of this.#data) {
			yield v;
		}
	}
	forEach(callbackfn: (value: V, key: K, map: NMap<K, V>) => void, thisArg?: any) {
		for (const [_, [k, v]] of this.#data) {
			callbackfn.apply(thisArg, [v, k, this]);
		}
	}
	[Symbol.iterator](): Iterator<[K, V]> {
		return this.entries();
	}
}

export class NSet<K extends HasComperator> {
	#data: Map<number, K>;

	constructor(data?: Iterable<K>) {
		if (!data) {
			this.#data = new Map();
		} else {
			this.#data = new Map(View.from(data).map((k) => [k.comperator, k]));
		}
	}

	get size() {
		return this.#data.size;
	}
	clear() {
		this.#data.clear();
	}
	delete(key: K): boolean {
		return this.#data.delete(key.comperator);
	}
	has(key: K): boolean {
		return this.#data.has(key.comperator);
	}
	add(key: K): this {
		this.#data.set(key.comperator, key);
		return this;
	}
	*keys(): Iterator<K> {
		for (const [_, k] of this.#data) {
			yield k;
		}
	}
	values(): Iterator<K> {
		return this.keys();
	}
	forEach(callbackfn: (value: K, key: K, map: NSet<K>) => void, thisArg?: any) {
		for (const [_, k] of this.#data) {
			callbackfn.apply(thisArg, [k, k, this]);
		}
	}
	[Symbol.iterator](): Iterator<K> {
		return this.keys();
	}
}
