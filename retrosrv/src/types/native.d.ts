declare module "*libretro-x64-Debug" {
	declare class RefTracked {
		get refcount(): number;
		get unique(): boolean;
		get expired(): boolean;
	}
	declare class Dynamic extends RefTracked {
		get dynTypeId(): number;
		get dynTypeName(): string;
	}

	declare class MyBla extends Dynamic {
		private constructor();

		get roCount(): number;
		set count(value: number);
		get count(): number;

		add(value: number): number;
		sub(value: number): number;

		async asyncStuff(x: number): Promise<number>;

		[Symbol.iterator](): Iterator<number>;

		static getCount(x: MyBla): number;
	}
	declare class MySuperBla extends MyBla {
		set xcount(value: number);
		get xcount(): number;
	}

	const bla: number;
	const blaInstance: MyBla;
	const superBlaInstance: MySuperBla;
	function abcd(x: number, c: (q: number) => number): number;
}
