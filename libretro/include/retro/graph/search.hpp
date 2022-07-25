#pragma once
#include <retro/common.hpp>
#include <retro/intrin.hpp>

// Basic graph search algorithms.
//
namespace retro::graph {
	// Returns the next monotonic integer with no wrap-around expected.
	//
	inline u64 monotonic_counter() {
		return intrin::cycle_counter();
	}

	// Depth-first search.
	//
	template<typename T, typename F>
	inline bool dfs(T* n, const F& fn, bool no_self = false, u64 mark = monotonic_counter()) {
		n->tmp_monotonic = mark;
		for (auto& s : n->successors)
			if (s->tmp_monotonic != mark)
				if (dfs<T, F>(s, fn, false, mark))
					return true;
		return !no_self && apply_novoid<bool>(fn, n);
	}

	// Breadth-first search.
	//
	template<typename T, typename F>
	inline bool bfs(T* n, const F& fn, bool no_self = false, u64 mark = monotonic_counter()) {
		n->tmp_monotonic = mark;
		if (!no_self && apply_novoid<bool>(fn, n))
			return true;
		for (auto& s : n->successors)
			if (s->tmp_monotonic != mark)
				if (bfs<T, F>(s, fn, false, mark))
					return true;
		return false;
	}
};