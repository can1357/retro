#pragma once
#include <retro/common.hpp>

namespace retro::heap {
	void* allocate(size_t n);
	void	deallocate(void* p);
	void* resize(void* p, size_t n);
	void  shrink(void* p, size_t n);
};