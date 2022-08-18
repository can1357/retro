#include <retro/heap.hpp>
#include <retro/format.hpp>

#if RC_WINDOWS
#include <Windows.h>
namespace retro::heap {
	void* allocate(size_t n) { return HeapAlloc(GetProcessHeap(), 0, n); }
	void	deallocate(void* p) { HeapFree(GetProcessHeap(), 0, p); }
	void* resize(void* p, size_t n) { return HeapReAlloc(GetProcessHeap(), 0, p, n); }
	void	shrink(void* p, size_t n) {
		 void* p2 = HeapReAlloc(GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY, p, n);
		 RC_ASSERT(!p2 || p == p2);
	}
};
#else
namespace retro::heap {
	void* allocate(size_t n) { return malloc(n); }
	void	deallocate(void* p) { free(p); }
	void* resize(void* p, size_t n) { return realloc(p, n); }
	void	shrink(void* p, size_t n) { RC_ASSERT(realloc(p, n) == p); }
};
#endif