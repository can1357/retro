#pragma once
#include <algorithm>
#include <memory>
#include <utility>
#include <retro/common.hpp>
#include <retro/format.hpp>

// Circular linked list helpers.
//
namespace retro::list {
	// Define a generic iterator.
	//
	template<typename T>
	struct iterator {
		// Traits.
		//
		using iterator_category = std::bidirectional_iterator_tag;
		using difference_type	= iptr;
		using value_type			= T*;
		using pointer				= T*;
		using reference			= T*;

		// Current position.
		//
	  private:
		T* at = nullptr;
	  public:

		// Default construction, construction from pointer, copy and move.
		//
		constexpr iterator() = default;
		constexpr iterator(T* at) : at(at) {}
		constexpr iterator(const iterator&)						= default;
		constexpr iterator(iterator&&) noexcept				= default;
		constexpr iterator& operator=(const iterator&)		= default;
		constexpr iterator& operator=(iterator&&) noexcept = default;

		// Converting construction.
		//
		template<typename Ty>
			requires(!std::is_same_v<T, Ty> && std::is_convertible_v<Ty, T>)
		constexpr iterator(iterator<Ty> other) : at((T*) other.at) {}

		// Common iteration operators.
		//
		T*						  get() const { return at; }
		constexpr			  operator T*() const { return at; }
		constexpr reference operator*() const { return at; }
		constexpr pointer	  operator->() const { return at; }
		constexpr iterator& operator++() {
			at = at->next;
			return *this;
		}
		constexpr iterator& operator--() {
			at = at->prev;
			return *this;
		}
		constexpr iterator operator++(int) {
			iterator tmp = *this;
			++*this;
			return tmp;
		}
		constexpr iterator operator--(int) {
			iterator tmp = *this;
			++*this;
			return tmp;
		}
		constexpr bool operator==(const iterator& o) const { return at == o.at; };
		constexpr bool operator!=(const iterator& o) const { return at != o.at; };
	};

	// Utilities for manual management.
	// - Initializes the list head.
	template<typename T>
	RC_INLINE static void init(T* at) {
		at->prev = at;
		at->next = at;
	}
	// - Returns true if entry is detached.
	template<typename T>
	RC_INLINE static bool is_detached(T* at) {
		return at->prev == at;
	}
	// - Links val before at.
	template<typename T, typename T2 = T>
	RC_INLINE static void link_before(T* at, T2* val) {
		RC_ASSERT(is_detached(val));
		auto* prev = std::exchange(at->prev, val);
		prev->next = val;
		val->prev  = prev;
		val->next  = at;
	}
	// - Links val after at.
	template<typename T, typename T2 = T>
	RC_INLINE static void link_after(T* at, T2* val) {
		RC_ASSERT(is_detached(val));
		auto* next = std::exchange(at->next, val);
		next->prev = val;
		val->prev  = at;
		val->next  = next;
	}
	// - Unlinks the value from the list.
	template<typename T>
	RC_INLINE static void unlink(T* val) {
		RC_ASSERT(!is_detached(val));
		auto* prev = std::exchange(val->prev, val);
		auto* next = std::exchange(val->next, val);
		prev->next = next;
		next->prev = prev;
	}
	// - Returns an enumerable subrange for each entry between begin and end.
	//    If skipping list head, you may get an invalid entry.
	template<typename T>
	RC_INLINE static auto subrange(T* begin, T* end) {
		return range::subrange(iterator<T>(begin), iterator<T>(end));
	}
	// - Returns an enumerable range starting at entry following this one.
	//    If not applied to list head, you may get an invalid entry.
	template<typename T>
	RC_INLINE static auto range(T* at) {
		return list::subrange<T>(at->next, at);
	}

	// List head helper.
	//
	template<typename T>
	struct head : pinned {
	  private:
		T* prev = entry();
		T* next = entry();
	  public:

		// Explicitly cast to value type.
		//
		T* entry() const { return (T*) (uptr(&prev) - (offsetof(T, prev))); }

		// Observers.
		//
		iterator<T>				begin() const { return {next}; }
		iterator<T>				end() const { return {entry()}; }
		auto						rbegin() const { return std::reverse_iterator(end()); }
		auto						rend() const { return std::reverse_iterator(begin()); }
		bool						empty() const { return next == entry(); }
		size_t					size() const { return std::distance(begin(), end()); };
		T*							front() const { return (!empty()) ? (next) : nullptr; }
		T*							back() const { return (!empty()) ? (prev) : nullptr; }
		auto						slice(iterator<T> it) const { return range::subrange(it, end()); }
		auto						rslice(iterator<T> it) const { return range::subrange(std::reverse_iterator(it), rend()); }
	};
};