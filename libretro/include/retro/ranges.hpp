#pragma once
#include <algorithm>
#include <numeric>
#include <memory>
#include <execution>
#include <utility>

namespace retro::range {
	// Subranges.
	//
	template<typename It>
	struct subrange {
		It begin_it;
		It end_it;

		inline constexpr subrange(It beg, It end) : begin_it(std::move(beg)), end_it(std::move(end)) {}
		constexpr subrange(const subrange&)						= default;
		constexpr subrange& operator=(const subrange&)		= default;
		constexpr subrange(subrange&&) noexcept				= default;
		constexpr subrange& operator=(subrange&&) noexcept = default;

		constexpr explicit operator bool() const { return begin_it != end_it; }

		constexpr It begin() const { return begin_it; }
		constexpr It end() const { return end_it; }
	};
	template<typename A, typename B>
	subrange(A, B) -> subrange<A>;

	// Data.
	//
	template<typename R, typename V>
	inline static constexpr auto copy(R&& r, V&& val) {
		return std::copy(r.begin(), r.end(), std::forward<V>(val));
	}
	template<typename R, typename V>
	inline static constexpr auto move(R&& r, V&& val) {
		return std::copy(std::make_move_iterator(r.begin()), std::make_move_iterator(r.end()), std::forward<V>(val));
	}
	template<typename R, typename V>
	inline static constexpr auto fill(R&& r, V&& val) {
		return std::fill(r.begin(), r.end(), std::forward<V>(val));
	}
	template<typename R>
	inline static constexpr auto destroy(R&& r) {
		for (auto& e : r) {
			std::destroy_at(&e);
		}
	}

	// Search.
	//
	template<typename R, typename V>
	inline static constexpr auto find(R&& r, V&& val) {
		return std::find(r.begin(), r.end(), std::forward<V>(val));
	}
	template<typename R, typename V>
	inline static constexpr bool contains(R&& r, V&& val) {
		return std::find(r.begin(), r.end(), std::forward<V>(val)) != r.end();
	}
	template<typename R, typename F>
	inline static constexpr auto find_if(R&& r, F&& fn) {
		return std::find_if(r.begin(), r.end(), std::forward<F>(fn));
	}
	template<typename R, typename F>
	inline static constexpr auto contains_if(R&& r, F&& fn) {
		return std::find_if(r.begin(), r.end(), std::forward<F>(fn)) != r.end();
	}
	template<typename R, typename V>
	inline static constexpr size_t count(R&& r, V&& val) {
		return std::count(r.begin(), r.end(), std::forward<V>(val));
	}
	template<typename R, typename V>
	inline static constexpr auto lower_bound(R&& r, V&& val) {
		return std::lower_bound(r.begin(), r.end(), std::forward<V>(val));
	}
	template<typename R, typename V>
	inline static constexpr auto upper_bound(R&& r, V&& val) {
		return std::upper_bound(r.begin(), r.end(), std::forward<V>(val));
	}
	template<typename R, typename V, typename F>
	inline static constexpr auto lower_bound(R&& r, V&& val, F&& fn) {
		return std::lower_bound(r.begin(), r.end(), std::forward<V>(val), std::forward<F>(fn));
	}
	template<typename R, typename V, typename F>
	inline static constexpr auto upper_bound(R&& r, V&& val, F&& fn) {
		return std::upper_bound(r.begin(), r.end(), std::forward<V>(val), std::forward<F>(fn));
	}

	// Comparison.
	//
	template<typename R, typename F>
	inline static constexpr auto all_of(R&& r, F&& fn) {
		return std::all_of(r.begin(), r.end(), std::forward<F>(fn));
	}
	template<typename R, typename F>
	inline static constexpr auto any_of(R&& r, F&& fn) {
		return std::any_of(r.begin(), r.end(), std::forward<F>(fn));
	}
	template<typename R, typename F>
	inline static constexpr auto none_of(R&& r, F&& fn) {
		return std::none_of(r.begin(), r.end(), std::forward<F>(fn));
	}
	template<typename R, typename R2>
	inline static constexpr auto equal(R&& r, R2&& r2) {
		return std::equal(r.begin(), r.end(), r2.end());
	}
	template<typename R, typename R2, typename F>
	inline static constexpr auto equal(R&& r, R2&& r2, F&& fn) {
		return std::equal(r.begin(), r.end(), r2.end(), std::forward<F>(fn));
	}

	// Misc.
	//
	template<typename R>
	inline static constexpr auto sort(R&& r) {
		return std::sort(r.begin(), r.end());
	}
	template<typename R, typename F>
	inline static constexpr auto sort(R&& r, F&& fn) {
		return std::sort(r.begin(), r.end(), std::forward<F>(fn));
	}
	template<typename R, typename F>
	inline static constexpr auto for_each(R&& r, F&& fn) {
		return std::for_each(r.begin(), r.end(), std::forward<F>(fn));
	}
	template<typename E, typename R, typename F>
	inline static constexpr auto for_each(E&& exec, R&& r, F&& fn) {
		return std::for_each(exec, r.begin(), r.end(), std::forward<F>(fn));
	}
};

namespace retro::view {
	template<typename R>
	inline static constexpr auto reverse(R&& range) {
		return range::subrange(
			std::make_reverse_iterator(std::end(range)),
			std::make_reverse_iterator(std::begin(range))
		);
	}
};