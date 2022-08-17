type MapFn<T, Ty> = (v: T) => Ty;
type PredFn<T> = (v: T) => boolean;
type EnumFn<T> = (v: T) => void;

export default class View<T> implements Iterable<T> {
	#base: Iterable<T>;

	// Internal constructor.
	//
	constructor(base: Iterable<T>) {
		this.#base = base;
	}

	// Subview constructor.
	//
	static from<T>(base: Iterable<T>): View<T> {
		return new View<T>(base);
	}

	// Iota constructor.
	//
	static iota(start: number = 0, count: number = Infinity): View<number> {
		let it = start;
		return new View<number>(
			(function* () {
				while (count-- > 0) {
					yield it++;
				}
			})()
		);
	}

	// Iterator.
	//
	*[Symbol.iterator]() {
		for (const v of this.#base) {
			yield v;
		}
	}

	// Array and string conversion.
	//
	toArray(): Array<T> {
		return Array.from(this);
	}
	toString() {
		return this.toArray().toString();
	}

	// Utilities.
	//
	forEach(fn: EnumFn<T>) {
		for (const x of this) {
			fn(x);
		}
	}
	find(fn: PredFn<T>): T | null {
		for (const x of this) {
			if (fn(x)) {
				return x;
			}
		}
		return null;
	}
	indexOf(_val: T): number {
		const val: any = _val;

		let n = 0;
		if (val.equals) {
			for (const x of this) {
				if (val.equals(x)) {
					return n;
				} else {
					++n;
				}
			}
		} else {
			for (const x of this) {
				if (val == x) {
					return n;
				} else {
					++n;
				}
			}
		}
		return -1;
	}
	all(fn: PredFn<T>): boolean {
		for (const x of this) {
			if (!fn(x)) {
				return false;
			}
		}
		return true;
	}
	any(fn: PredFn<T>): boolean {
		for (const x of this) {
			if (fn(x)) {
				return true;
			}
		}
		return false;
	}
	at(i: number): T | null {
		for (const k of this) {
			if (i == 0) {
				return k;
			}
			--i;
		}
		return null;
	}

	// Transformers.
	//
	filter(fn: PredFn<T>): View<T> {
		const self = this;
		return new View<T>(
			(function* () {
				for (const v of self.#base) {
					if (fn(v)) {
						yield v;
					}
				}
			})()
		);
	}
	map<Ty>(fn: MapFn<T, Ty>): View<Ty> {
		const self = this;
		return new View<Ty>(
			(function* () {
				for (const v of self.#base) {
					yield fn(v);
				}
			})()
		);
	}
	skip(i: number): View<T> {
		const self = this;
		return new View<T>(
			(function* () {
				for (const v of self.#base) {
					if (i-- <= 0) {
						yield v;
					}
				}
			})()
		);
	}
	take(i: number): View<T> {
		const self = this;
		return new View<T>(
			(function* () {
				for (const v of self.#base) {
					if (i-- <= 0) {
						break;
					}
					yield v;
				}
			})()
		);
	}
	slice(offset: number, count: number): View<T> {
		const self = this;
		return new View<T>(
			(function* () {
				for (const v of self.#base) {
					if (offset-- <= 0) {
						if (count-- <= 0) {
							break;
						}
						yield v;
					}
				}
			})()
		);
	}
}
