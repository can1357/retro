#pragma once
#include <retro/common.hpp>
#include <retro/list.hpp>
#include <retro/dyn.hpp>
#include <retro/format.hpp>

// Automatic cross-reference tracking system, similar to LLVM's.
//
namespace retro::ir {
	struct usable;

	//
	// TODO: Introduce dyn somewhere here.
	//

	// Use instance.
	//
	struct alignas(8) use : pinned {
		// Circular linked list.
		//
		use* prev = this;
		use* next = this;

		// Offset to base of user.
		//
		iptr offset = 0;

		// Pointer to used type.
		//
		void* target = nullptr;

		// Checks if use has a value.
		//
		bool has_value() const {
			RC_ASSERT((target == nullptr) == list::is_detached(this));
			return target != nullptr;
		}

		// Sets a new use.
		//
		template<typename T>
		void reset(T* to) {
			reset();
			if (to) {
				list::link_before(&to->use_list_head, this);
				target = to;
			}
		}

		// Sets the initial use.
		//
		template<typename T>
		void reset(T* to, void* base) {
			offset = uptr(this) - uptr(base);
			reset(to);
		}

		// Clears the used value.
		//
		void reset() {
			if (has_value()) {
				list::unlink(this);
			}
			target = nullptr;
		}

		// Replaces this use with another value.
		//
		template<typename T>
		void replace_use_with(T* to) {
			RC_ASSERT(has_value());
			list::unlink(this);
			target = to;
			if (to)
				list::link_before(&to->use_list_head, this);
		}

		// Value pointer getter.
		//
		template<typename T = void>
		RC_INLINE T* value() const {
			return (T*) target;
		}

		// User pointer getter.
		//
		template<typename T = void>
		RC_INLINE T* user() const {
			RC_ASSERT(has_value());
			return (T*) (uptr(this) - offset);
		}
	};

	// Useable type.
	//
	struct usable : pinned {
		// List head.
		//
		mutable use use_list_head;

		// Gets the list of uses.
		//
		auto uses() const { return list::range(&use_list_head); }

		// Replaces all uses with another value.
		//
		template<typename T>
		void replace_all_uses_with(T* ptr) const {
			for (auto it = use_list_head.next; it != &use_list_head;) {
				auto next = it->next;
				it->replace_use_with(ptr);
				it = next;
			}
		}

		// Ensure no references left on destruction if debug mode.
		//
#if RC_DEBUG
		~usable() { RC_ASSERTS("Destroying value with lingering uses.", !uses()); }
#endif
	};
};
