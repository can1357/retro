#pragma once
#include <atomic>
#include <retro/common.hpp>
#include <retro/format.hpp>
#include <retro/dyn.hpp>
#include <retro/heap.hpp>

// Type-specialized ref-counting primitives.
//
namespace retro {
	// Ref counter header.
	//
	struct rc_header {
		using refcnt_t = std::atomic<u64>;

		// Reference counters.
		// u64 strong_refs : 32
		// u64 weaks       : 32
		//
		refcnt_t ref_counter{0x00000001'00000001};

		// Destructor.
		//
		void (*dtor)(rc_header*) = nullptr;

		// Manual ref-management.
		//
		RC_INLINE void inc_ref_weak() {
			u64 newrefs = (ref_counter += (1ull << 32));
			RC_ASSERT((newrefs & bit_mask(32)) != 0);
		}
		RC_INLINE void inc_ref_unsafe() {
			u64 newrefs = ++ref_counter;
			RC_ASSERT((newrefs & bit_mask(32)) != 1);
		}
		RC_INLINE bool inc_ref() {
			u64 expected = ref_counter.load(std::memory_order::relaxed);
			while ((expected & bit_mask(32)) != 0)
				[[likely]] {
					if (ref_counter.compare_exchange_strong(expected, expected + 1)) {
						return true;
					}
				}
			return false;
		}
		RC_INLINE void dec_ref_weak() {
			u64 leftover = (ref_counter -= (1ull << 32));

			// If no more weak-references left, deallocate the block.
			//
			if (!leftover) [[unlikely]] {
				heap::deallocate(this);
			}
		}
		RC_INLINE void dec_ref() {
			u64 leftover = --ref_counter;

			// If we were the last strong-reference:
			//
			if (!(leftover & bit_mask(32))) [[unlikely]] {
				// Call the destructor.
				//
				dtor(this);

				// Decrement the initial weak reference.
				//
				dec_ref_weak();
			}
		}

		// Data getter.
		//
		RC_INLINE void* data() const { return (void*) (this + 1); }

		inline static rc_header* from(const void* a) { return std::prev((rc_header*) a); }
	};

	// Define the ref and weak similar to shared_ptr and weak_ptr.
	//
	template<typename T>
	struct RC_TRIVIAL_ABI ref {
		T* ptr = nullptr;

		// Null construction.
		//
		constexpr ref() = default;
		RC_INLINE constexpr ref(std::nullptr_t) {}

		// Construction by pointer.
		//
		constexpr ref(T* ptr) : ptr(ptr) {
			if (ptr) {
				rc_header::from(ptr)->inc_ref_unsafe();
			}
		}

		// Explicit construction by control block.
		//
		RC_INLINE explicit constexpr ref(rc_header* hdr) : ptr((T*)hdr->data()) {}

		// Adopting.
		//
		RC_INLINE static ref<T> adopt(T* ptr) { return ref<T>{rc_header::from(ptr)}; }

		// Construction by type decay.
		//
		template<typename T2>
			requires(!std::is_same_v<T, T2> && std::is_convertible_v<T2*, T*>)
		RC_INLINE constexpr ref(ref<T2> o) : ptr(std::exchange(o.ptr, nullptr)) {}

		// Implement copy.
		//
		RC_INLINE constexpr ref(const ref<T>& o) : ptr(o.ptr) {
			if (ptr)
				rc_header::from(ptr)->inc_ref_unsafe();
		}
		RC_INLINE constexpr ref& operator=(const ref<T>& o) {
			if (o.ptr)
				rc_header::from(o.ptr)->inc_ref_unsafe();
			if (ptr)
				rc_header::from(ptr)->dec_ref();
			ptr = o.ptr;
			return *this;
		}

		// Implement move via swap.
		//
		RC_INLINE constexpr ref(ref<T>&& o) noexcept : ptr(std::exchange(o.ptr, nullptr)) {}
		RC_INLINE constexpr ref& operator=(ref<T>&& o) noexcept {
			swap(o);
			return *this;
		}
		RC_INLINE constexpr void swap(ref<T>& o) noexcept { std::swap(ptr, o.ptr); }

		// Observers.
		//
		RC_INLINE T*					  get() const { return (T*) ptr; }
		RC_INLINE T&					  operator*() const { return *ptr; }
		RC_INLINE T*					  operator->() const { return get(); }
		RC_INLINE size_t				  use_count() const { return ptr ? rc_header::from(ptr)->ref_counter & bit_mask(32) : 0; }
		RC_INLINE bool					  unique() const { return ptr && rc_header::from(ptr)->ref_counter == 1; }
		RC_INLINE explicit constexpr operator bool() const { return ptr != nullptr; }

		// Decay to pointer.
		//
		RC_INLINE constexpr operator T*() const { return get(); }

		// Comparison.
		//
		RC_INLINE constexpr bool		  operator==(T* o) const { return ptr == o; }
		RC_INLINE constexpr bool		  operator==(const ref<T>& o) const { return ptr == o.ptr; }
		RC_INLINE constexpr bool		  operator==(std::nullptr_t) const { return ptr == ((T*) nullptr); }
		RC_INLINE friend constexpr bool operator==(T* o, const ref<T>& s) { return s.ptr == o; }

		RC_INLINE constexpr bool		  operator!=(T* o) const { return ptr != o; }
		RC_INLINE constexpr bool		  operator!=(const ref<T>& o) const { return ptr != o.ptr; }
		RC_INLINE constexpr bool		  operator!=(std::nullptr_t) const { return ptr != ((T*) nullptr); }
		RC_INLINE friend constexpr bool operator!=(T* o, const ref<T>& s) { return s.ptr != o; }

		RC_INLINE constexpr bool		  operator<(T* o) const { return ptr < o; }
		RC_INLINE constexpr bool		  operator<(const ref<T>& o) const { return ptr < o.ptr; }
		RC_INLINE constexpr bool		  operator<(std::nullptr_t) const { return ptr < ((T*) nullptr); }
		RC_INLINE friend constexpr bool operator<(T* o, const ref<T>& s) { return s.ptr < o; }

		// Release without reference tracking.
		//
		RC_INLINE constexpr T* release() { return std::exchange(ptr, nullptr); }

		// Destructor.
		//
		RC_INLINE constexpr void reset() {
			if (ptr)
				rc_header::from(release())->dec_ref();
		}
		RC_INLINE constexpr ~ref() { reset(); }
	};
	template<typename T>
	struct RC_TRIVIAL_ABI weak {
		T* ptr = nullptr;

		// Null construction.
		//
		constexpr weak() = default;
		RC_INLINE constexpr weak(std::nullptr_t) {}

		// Construction by pointer.
		//
		RC_INLINE constexpr weak(T* ptr) : ptr(ptr) {
			if (ptr) {
				rc_header::from(ptr)->inc_ref_weak();
			}
		}

		// Construction by type decay.
		//
		template<typename T2>
			requires(!std::is_same_v<T, T2> && std::is_convertible_v<T2*, T*>)
		RC_INLINE constexpr weak(const ref<T2>& o) : ptr(o.ptr) {
			if (ptr)
				rc_header::from(ptr)->inc_ref_weak();
		}
		template<typename T2>
			requires(!std::is_same_v<T, T2> && std::is_convertible_v<T2*, T*>)
		RC_INLINE constexpr weak(weak<T2> o) : ptr(std::exchange(o.ptr, nullptr)) {}

		// Explicit construction by control block.
		//
		RC_INLINE explicit constexpr weak(rc_header* hdr) : ptr((T*) hdr->data()) {}

		// Adopting.
		//
		RC_INLINE static weak<T> adopt(T* ptr) { return weak<T>{rc_header::from(ptr)}; }

		// Construction from ref<T>.
		//
		RC_INLINE constexpr weak(const ref<T>& o) : ptr(o.ptr) {
			if (ptr)
				rc_header::from(ptr)->inc_ref_weak();
		}

		// Implement copy.
		//
		RC_INLINE constexpr weak(const weak<T>& o) : ptr(o.ptr) {
			if (ptr)
				rc_header::from(ptr)->inc_ref_weak();
		}
		RC_INLINE constexpr weak& operator=(const weak<T>& o) {
			if (o.ptr)
				rc_header::from(o.ptr)->inc_ref_weak();
			if (ptr)
				rc_header::from(ptr)->dec_ref_weak();
			ptr = o.ptr;
			return *this;
		}

		// Implement move via swap.
		//
		RC_INLINE constexpr weak(weak<T>&& o) noexcept : ptr(std::exchange(o.ptr, nullptr)) {}
		RC_INLINE constexpr weak& operator=(weak<T>&& o) noexcept {
			swap(o);
			return *this;
		}
		RC_INLINE constexpr void swap(weak<T>& o) noexcept { std::swap(ptr, o.ptr); }

		// Observers.
		//
		RC_INLINE T* get() const {
			RC_ASSERT(!ptr || !expired());
			return ptr;
		}
		RC_INLINE T& operator*() const {
			RC_ASSERT(!expired());
			return *ptr;
		}
		RC_INLINE T*	  operator->() const { return get(); }
		RC_INLINE size_t use_count() const { return ptr ? rc_header::from(ptr)->ref_counter & bit_mask(32) : 0; }
		RC_INLINE bool	  expired() const { return ptr && (rc_header::from(ptr)->ref_counter & bit_mask(32)) == 0; }
		RC_INLINE ref<T> lock() const {
			if (!ptr || !rc_header::from(ptr)->inc_ref())
				return nullptr;
			return ref<T>(rc_header::from(ptr));
		}
		RC_INLINE explicit constexpr operator bool() const { return ptr != nullptr; }

		// Explicit cast to pointer.
		//
		RC_INLINE explicit constexpr operator T*() const { return get(); }

		// Comparison.
		//
		RC_INLINE constexpr bool		  operator==(T* o) const { return ptr == o; }
		RC_INLINE constexpr bool		  operator==(const weak<T>& o) const { return ptr == o.ptr; }
		RC_INLINE constexpr bool		  operator==(const ref<T>& o) const { return ptr == o.ptr; }
		RC_INLINE constexpr bool		  operator==(std::nullptr_t) const { return ptr == ((T*) nullptr); }
		RC_INLINE friend constexpr bool operator==(T* o, const weak<T>& s) { return s.ptr == o; }
		RC_INLINE friend constexpr bool operator==(const ref<T>& o, const weak<T>& s) { return s.ptr == o.ptr; }

		RC_INLINE constexpr bool		  operator!=(T* o) const { return ptr != o; }
		RC_INLINE constexpr bool		  operator!=(const weak<T>& o) const { return ptr != o.ptr; }
		RC_INLINE constexpr bool		  operator!=(const ref<T>& o) const { return ptr != o.ptr; }
		RC_INLINE constexpr bool		  operator!=(std::nullptr_t) const { return ptr != ((T*) nullptr); }
		RC_INLINE friend constexpr bool operator!=(T* o, const weak<T>& s) { return s.ptr != o; }
		RC_INLINE friend constexpr bool operator!=(const ref<T>& o, const weak<T>& s) { return s.ptr != o.ptr; }

		RC_INLINE constexpr bool		  operator<(T* o) const { return ptr < o; }
		RC_INLINE constexpr bool		  operator<(const weak<T>& o) const { return ptr < o.ptr; }
		RC_INLINE constexpr bool		  operator<(const ref<T>& o) const { return ptr < o.ptr; }
		RC_INLINE constexpr bool		  operator<(std::nullptr_t) const { return ptr < ((T*) nullptr); }
		RC_INLINE friend constexpr bool operator<(T* o, const weak<T>& s) { return s.ptr < o; }
		RC_INLINE friend constexpr bool operator<(const ref<T>& o, const weak<T>& s) { return s.ptr < o.ptr; }

		// Release without reference tracking.
		//
		RC_INLINE constexpr T* release() { return std::exchange(ptr, nullptr); }

		// Destructor.
		//
		RC_INLINE constexpr void reset() {
			if (ptr)
				rc_header::from(release())->dec_ref_weak();
		}
		RC_INLINE constexpr ~weak() { reset(); }
	};
	template<typename T>
	weak(ref<T>) -> weak<T>;

	// std::make_shared equivalent.
	//
	template<typename T, typename... Tx>
	inline static ref<T> make_overalloc_rc(size_t overalloc, Tx&&... args) {
		rc_header* rc = new (heap::allocate(sizeof(T) + sizeof(rc_header) + overalloc)) rc_header();
		rc->dtor		  = +[](rc_header* p) { std::destroy_at((T*) p->data()); };
		T* data		  = new (rc->data()) T(std::forward<Tx>(args)...);
		return ref<T>{rc};
	}
	template<typename T, typename... Tx>
	inline static ref<T> make_rc(Tx&&... args) {
		return make_overalloc_rc<T, Tx...>(0, std::forward<Tx>(args)...);
	};

	// Static and dynamic cast for ref/weak.
	//
	template<typename Ty, typename T>
	inline static ref<Ty> static_rc_cast(ref<T> ptr) {
		ref<Ty> result = {};
		result.ptr		= (Ty*) ptr.release();
		return result;
	}
	template<typename Ty, typename T>
	inline static weak<Ty> static_rc_cast(weak<T> ptr) {
		weak<Ty> result = {};
		result.ptr		= (Ty*) ptr.release();
		return result;
	}
	template<typename Ty, typename T>
	inline static ref<Ty> dynamic_rc_cast(ref<T> ptr) {
		ref<Ty> result = {};
		if (ptr && ptr->template is<Ty>()) {
			result.adopt((Ty*) ptr.release());
		}
		return result;
	}
	template<typename Ty, typename T>
	inline static ref<Ty> dynamic_rc_cast(const ref<T>& ptr) {
		if (ptr && ptr->template is<Ty>()) {
			return ref<Ty>((Ty*) ptr.get());
		}
		return nullptr;
	}
}