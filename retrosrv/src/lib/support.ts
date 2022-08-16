export function extendClass(nativeType: any, extensionType: any) {
	for (const [k, v] of Object.entries(Object.getOwnPropertyDescriptors(extensionType.prototype))) {
		if (k != "constructor") {
			Object.defineProperty(nativeType.prototype, k, v);
		}
	}
}
