#pragma once
#include <algorithm>
#include <memory>
#include <utility>
#include <retro/common.hpp>
#include <retro/format.hpp>

// Circular linked list helpers.
//
namespace retro::list {
	// Forward decl of head, utilities for manual management.
	//
	template<typename T>
	struct head;
	namespace util {
		template<typename T, typename T2 = T>
		static void link_before(T* at, T2* val) {
			RC_ASSERT(val->prev == val);
			auto* prev = std::exchange(at->prev, val);
			prev->next = val;
			val->prev  = prev;
			val->next  = at;
		}
		template<typename T, typename T2 = T>
		static void link_after(T* at, T2* val) {
			RC_ASSERT(val->prev == val);
			auto* next = std::exchange(at->next, val);
			next->prev = val;
			val->prev  = at;
			val->next  = next;
		}
		template<typename T>
		static void unlink(T* at) {
			RC_ASSERT(at->prev != at);
			auto* prev = std::exchange(at->prev, at);
			auto* next = std::exchange(at->next, at);
			prev->next = next;
			next->prev = prev;
		}
	};

	// Define a CRTT list entry.
	//
	template<typename T>
	struct entry {
		using BaseEntryType = T;

		// Linked list.
		//
		T*			prev			= get();
		T*			next			= get();
		head<T>* owning_list = nullptr;

		// List traits.
		//
		bool is_detached() const { return prev == this; }

		// Erasing and insertion.
		//
		void insert_before(T* at) {
			util::link_before(at, get());
			owning_list = at->owning_list;
		}
		void insert_after(T* at) {
			util::link_after(at, get());
			owning_list = at->owning_list;
		}
		std::unique_ptr<T> erase() {
			util::unlink(get());
			owning_list = nullptr;
			return std::unique_ptr<T>{get()};
		}

		// Offsetter getter.
		//
	  private:
		RC_CONST inline T* get() const { return (T*) this; }
	};

	// Define a generic iterator.
	//
	template<typename T>
	struct iterator {
		// Traits.
		//
		using iterator_category = std::bidirectional_iterator_tag;
		using difference_type	= ptrdiff_t;
		using value_type			= T*;
		using pointer				= T*;
		using reference			= T*;

		// Current position.
		//
		T* at = nullptr;

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

	// Generic list head.
	//
	template<typename T>
	struct head : range::view_base {
		using iterator = iterator<T>;

		T*		last		  = dummy();  // .prev
		T*		first		  = dummy();  // .next
		void* __self_ref = this;	  // .owning_list

		// Default construction.
		//
		head() = default;

		// No copy or move.
		//
		head(const head&)				  = delete;
		head(head&& o)					  = delete;
		head& operator=(const head&) = delete;
		head& operator=(head&& o)	  = delete;

		// Observers.
		//
		iterator begin() const { return {first}; }
		iterator end() const { return {dummy()}; }
		auto		rbegin() const { return std::reverse_iterator(end()); }
		auto		rend() const { return std::reverse_iterator(begin()); }
		bool		empty() const { return first == dummy(); }
		T*			front() const { return (!empty()) ? first : nullptr; }
		T*			back() const { return (!empty()) ? last : nullptr; }

		// Subrange.
		//
		auto after(T* i) const { return range::subrange(i ? iterator(i->next) : begin(), end()); }
		auto before(T* i) const { return range::subrange(begin(), i ? iterator(i) : end()); }

		// Insertion.
		//
		iterator push_back(T* ptr) {
			ptr->insert_after(last);
			return iterator(ptr);
		}
		iterator push_front(T* ptr) {
			ptr->insert_before(first);
			return iterator(ptr);
		}
		template<typename Ty = T, typename... Tx>
		iterator emplace_back(Tx&&... args) {
			T* res = new Ty(std::forward<Tx>(args)...);
			return push_back(res);
		}
		template<typename Ty = T, typename... Tx>
		iterator emplace_front(Tx&&... args) {
			T* res = new Ty(std::forward<Tx>(args)...);
			return push_front(res);
		}

		// Reset the list.
		//
		void clear() {
			auto it = first;
			while (it != dummy()) {
				delete std::exchange(it, it->next);
			}
			first = dummy();
			last = dummy();
		}
		~head() { clear(); }

	  private:
		T* dummy() const { return (T*) (uptr(this) + offsetof(head, last) - offsetof(T, prev)); }
	};
};