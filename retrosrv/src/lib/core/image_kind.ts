export enum ImageKind {
	Unknown = 0,
	Executable, // Executable.
	DynamicLibrary, // Dynamic library.
	StaticLibrary, // Static library.
	ObjectFile, // Single unit of compilation.
	MemoryDump, // Process memory dump, meaning multiple images are present.
}
