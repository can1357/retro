#pragma once
#include <retro/common.hpp>
#include <retro/graph/search.hpp>

// Naive graph algorithm implementations.
//
namespace retro::graph::naive {
	// Domination check.
	//
	template<typename T>
	inline bool dom(const T* a, const T* b) {
		auto mark  = monotonic_counter();
		auto nodom = [a, mark](auto& self, const T* b) -> bool {
			if (b->predecessors.empty())
				return true;
			b->tmp_monotonic = mark;
			for (auto& s : b->predecessors) {
				if (s->tmp_monotonic != mark && s != a) {
					if (self(self, s))
						return true;
				}
			}
			return false;
		};
		return a == b || !nodom(nodom, b);
	}

	// Post domination check.
	//
	template<typename T>
	inline bool postdom(const T* a, const T* b) {
		auto mark  = monotonic_counter();
		auto nodom = [a, mark](auto& self, const T* b) -> bool {
			if (b->successors.empty())
				return true;
			b->tmp_monotonic = mark;
			for (auto& s : b->successors) {
				if (b->tmp_monotonic != mark && s != a) {
					if (self(self, s))
						return true;
				}
			}
			return false;
		};
		return a == b || !nodom(nodom, b);
	}

	// Path check.
	//
	template<typename T>
	inline bool has_path_to(const T* src, const T* dst) {
		return graph::bfs(
			 src, [=](const T* b) { return b == dst; }, true);
	}
};