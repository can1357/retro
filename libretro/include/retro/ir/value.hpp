#pragma once
#include <retro/common.hpp>
#include <retro/list.hpp>
#include <retro/dyn.hpp>
#include <retro/format.hpp>
#include <retro/rc.hpp>

// Automatic cross-reference tracking system, similar to LLVM's.
//
namespace retro::ir {
	// String formatting style.
	//
	enum class fmt_style : u8 { full, concise };

	// Value type.
	//
	struct use;
	struct value : rc, dyn<value>, pinned {
	  private:
		// Circular linked list for the uses.
		//
		mutable use* use_list_prev = (use*) &use_list_prev;
		mutable use* use_list_next = (use*) &use_list_prev;
		friend use;

		// List head.
		//
		use* use_list_head() const { return (use*) &use_list_prev; }

	  public:
		// String conversion and type getter.
		//
		virtual std::string to_string(fmt_style s = {}) const = 0;
		virtual type		  get_type() const						= 0;

		// Gets the list of uses.
		//
		range::subrange<list::iterator<use>> uses() const;

		// Replaces all uses with another value.
		//
		void replace_all_uses_with(const shared<value>& val) const;

		// Ensure no references left on destruction.
		//
		~value();
	};

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
		shared<value> target = nullptr;

		// Checks if use has a value.
		//
		bool has_value() const {
			RC_ASSERT((target == nullptr) == list::is_detached(this));
			return target != nullptr;
		}

		// Sets a new use.
		//
		void reset(value* to, const void* base) {
			reset();
			offset = uptr(this) - uptr(base);
			if (to) {
				list::link_before(to->use_list_head(), this);
				target = to;
			}
		}

		// Clears the used value.
		//
		void reset() {
			if (has_value()) {
				list::unlink(this);
			}
			target.reset();
		}

		// Replaces this use with another value.
		//
		void replace_use_with(shared<value> val) {
			RC_ASSERT(has_value());
			list::unlink(this);
			if (val)
				list::link_before(val->use_list_head(), this);
			target = std::move(val);
		}

		// Value pointer getter.
		//
		RC_INLINE value* get() const {
			return target.get();
		}

		// User pointer getter.
		//
		template<typename T = void>
		RC_INLINE T* user() const {
			RC_ASSERT(has_value());
			return (T*) (uptr(this) - offset);
		}

		// Reset on destruction.
		//
		~use() { reset(); }
	};

	// Gets the list of uses.
	//
	inline range::subrange<list::iterator<use>> value::uses() const { return list::range(use_list_head()); }

	// Replaces all uses with another value.
	//
	void value::replace_all_uses_with(const shared<value>& val) const {
		for (auto it = use_list_head()->next; it != use_list_head();) {
			auto next = it->next;
			it->replace_use_with(val);
			it = next;
		}
	}

	// Ensure no references left on destruction.
	//
	inline value::~value() { RC_ASSERTS("Destroying value with lingering uses.", !uses()); }
};
