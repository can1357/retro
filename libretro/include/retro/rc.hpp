#pragma once
#include <atomic>
#include <retro/common.hpp>
#include <retro/format.hpp>

// Type-specialized ref-counting primitives.
//
namespace retro {
	template<bool Atomic>
	struct basic_rc_header;

	// Ref counter base.
	//
	template<bool Atomic>
	struct ref_counted {
		// Reference-counting traits.
		//
		using rc_header						  = basic_rc_header<Atomic>;
		static constexpr bool is_rc_atomic = Atomic;

		// Gets the reference-counter control block.
		//
		inline rc_header* get_ref_ctrl() const { return std::prev((rc_header*) this); }

		// Force virtual dtor.
		//
		virtual ~ref_counted() = default;
	};

	// Ref counter header.
	//
	template<bool Atomic>
	struct basic_rc_header {
		using refcnt_t = std::conditional_t<Atomic, std::atomic<u64>, u64>;

		// Reference counters.
		// u64 strong_refs : 32
		// u64 weaks       : 32
		//
		refcnt_t ref_counter{0x00000001'00000001};

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
			if constexpr (Atomic) {
				u64 expected = ref_counter.load(std::memory_order::relaxed);
				while ((expected & bit_mask(32)) != 0)
					[[likely]] {
						if (ref_counter.compare_exchange_strong(expected, expected + 1)) {
							return true;
						}
					}
			} else {
				if ((ref_counter & bit_mask(32)) != 0) [[likely]] {
					++ref_counter;
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
				operator delete((void*) this);
			}
		}
		RC_INLINE void dec_ref() {
			u64 leftover = --ref_counter;

			// If we were the last strong-reference:
			//
			if (!(leftover & bit_mask(32))) [[unlikely]] {
				// Call the virtual destructor.
				//
				std::destroy_at((ref_counted<Atomic>*) data());

				// Decrement the initial weak reference.
				//
				dec_ref_weak();
			}
		}

		// Data getter.
		//
		RC_INLINE void* data() const { return (void*) (this + 1); }
	};

	// Ref counted object concept.
	//
	template<typename T>
	concept RefCounted = (std::is_base_of_v<ref_counted<true>, T> || std::is_base_of_v<ref_counted<false>, T>);

	// Define the shared and weak similar to shared_ptr and weak_ptr.
	//
	template<typename T = void>
	struct RC_TRIVIAL_ABI shared {
		using rc_header = typename T::rc_header;
		rc_header* ctrl = nullptr;

		// Null construction.
		//
		constexpr shared() = default;
		RC_INLINE constexpr shared(std::nullptr_t) {}

		// Construction by pointer.
		//
		constexpr shared(T* ptr) {
			if (ptr) {
				ctrl = ptr->get_ref_ctrl();
				ctrl->inc_ref_unsafe();
			}
		}

		// Explicit construction by control block.
		//
		RC_INLINE explicit constexpr shared(rc_header* ctrl) : ctrl(ctrl) {}

		// Implement copy.
		//
		RC_INLINE constexpr shared(const shared<T>& o) : ctrl(o.ctrl) {
			if (ctrl)
				ctrl->inc_ref_unsafe();
		}
		RC_INLINE constexpr shared& operator=(const shared<T>& o) {
			if (o.ctrl)
				o.ctrl->inc_ref_unsafe();
			if (ctrl)
				ctrl->dec_ref();
			ctrl = o.ctrl;
			return *this;
		}

		// Implement move via swap.
		//
		RC_INLINE constexpr shared(shared<T>&& o) : ctrl(std::exchange(o.ctrl, nullptr)) {}
		RC_INLINE constexpr shared& operator=(shared<T>&& o) {
			swap(o);
			return *this;
		}
		RC_INLINE constexpr void swap(shared<T>& o) noexcept { std::swap(ctrl, o.ctrl); }

		// Observers.
		//
		RC_INLINE T*					  get() const { return (T*) (ctrl ? ctrl->data() : nullptr); }
		RC_INLINE T&					  operator*() const { return *(T*) ctrl->data(); }
		RC_INLINE T*					  operator->() const { return get(); }
		RC_INLINE size_t				  use_count() const { return ctrl ? ctrl->ref_counter & bit_mask(32) : 0; }
		RC_INLINE bool					  unique() const { return ctrl && ctrl->ref_counter == 1; }
		RC_INLINE explicit constexpr operator bool() const { return ctrl != nullptr; }

		// Destructor.
		//
		RC_INLINE constexpr ~shared() {
			if (ctrl)
				ctrl->dec_ref();
		}
	};
	template<typename T = void>
	struct RC_TRIVIAL_ABI weak {
		using rc_header = typename T::rc_header;
		rc_header* ctrl = nullptr;

		// Null construction.
		//
		constexpr weak() = default;
		RC_INLINE constexpr weak(std::nullptr_t) {}

		// Construction by pointer.
		//
		constexpr weak(T* ptr) {
			if (ptr) {
				ctrl = ptr->get_ref_ctrl();
				ctrl->inc_ref_weak();
			}
		}

		// Explicit construction by control block.
		//
		RC_INLINE explicit constexpr weak(rc_header* ctrl) : ctrl(ctrl) {}

		// Construction from shared<T>.
		//
		RC_INLINE constexpr weak(const shared<T>& o) : ctrl(o.ctrl) {
			if (ctrl)
				ctrl->inc_ref_weak();
		}

		// Implement copy.
		//
		RC_INLINE constexpr weak(const weak<T>& o) : ctrl(o.ctrl) {
			if (ctrl)
				ctrl->inc_ref_weak();
		}
		RC_INLINE constexpr weak& operator=(const weak<T>& o) {
			if (o.ctrl)
				o.ctrl->inc_ref_weak();
			if (ctrl)
				ctrl->dec_ref_weak();
			ctrl = o.ctrl;
			return *this;
		}

		// Implement move via swap.
		//
		RC_INLINE constexpr weak(weak<T>&& o) : ctrl(std::exchange(o.ctrl, nullptr)) {}
		RC_INLINE constexpr weak& operator=(weak<T>&& o) {
			swap(o);
			return *this;
		}
		RC_INLINE constexpr void swap(weak<T>& o) noexcept { std::swap(ctrl, o.ctrl); }

		// Observers.
		//
		RC_INLINE T* get() const {
			RC_ASSERT(!ctrl || !expired());
			return (T*) (ctrl ? ctrl->data() : nullptr);
		}
		RC_INLINE T& operator*() const {
			RC_ASSERT(!ctrl || !expired());
			return *(T*) ctrl->data();
		}
		RC_INLINE T*	  operator->() const { return get(); }
		RC_INLINE size_t use_count() const { return ctrl ? ctrl->ref_counter & bit_mask(32) : 0; }
		RC_INLINE bool	  expired() const { return !ctrl || (ctrl->ref_counter & bit_mask(32)) == 0; }
		RC_INLINE shared<T> lock() const {
			if (!ctrl || !ctrl->inc_ref())
				return nullptr;
			return shared<T>(ctrl);
		}
		RC_INLINE explicit constexpr operator bool() const { return ctrl != nullptr; }

		// Destructor.
		//
		RC_INLINE constexpr ~weak() {
			if (ctrl)
				ctrl->dec_ref_weak();
		}
	};
	template<typename T>
	weak(shared<T>) -> weak<T>;

	// Ref counted tags.
	//
	template<typename T, bool Atomic>
	struct ref_counted_tag : ref_counted<Atomic> {
		template<typename... Tx>
		inline static shared<T> make(Tx&&... args) {
			using rc_header = basic_rc_header<Atomic>;
			rc_header* rc	 = new (operator new(sizeof(T) + sizeof(rc_header))) rc_header();
			T*			  data = new (rc->data()) T(std::forward<Tx>(args)...);
			return shared<T>{rc};
		}
	};
	template<typename T>
	using rc = ref_counted_tag<T, false>;
	template<typename T>
	using arc = ref_counted_tag<T, true>;
};