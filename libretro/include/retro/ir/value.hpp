#pragma once
#include <retro/common.hpp>
#include <retro/dyn.hpp>
#include <retro/format.hpp>
#include <retro/ir/types.hpp>
#include <retro/list.hpp>
#include <retro/rc.hpp>

// Automatic cross-reference tracking system, similar to LLVM's.
//
namespace retro::ir {
	// String formatting style.
	//
	enum class fmt_style : u8 { full, concise };

	// Value type.
	//
	struct operand;
	struct value : rc, dyn<value>, pinned {
	  private:
		// Circular linked list for the uses.
		//
		mutable operand* use_list_prev = (operand*) &use_list_prev;
		mutable operand* use_list_next = (operand*) &use_list_prev;
		friend operand;

		// List head.
		//
		operand* use_list_head() const { return (operand*) &use_list_prev; }

	  public:
		// String conversion and type getter.
		//
		virtual std::string to_string(fmt_style s = {}) const = 0;
		virtual type		  get_type() const						= 0;

		// Gets the list of uses.
		//
		range::subrange<list::iterator<operand>> uses() const;

		// Replaces all uses with another value.
		//
		template<typename T>
		void replace_all_uses_with(T&& val) const {
			for (auto it = use_list_head()->next; it != use_list_head();) {
				auto next = it->next;
				it->reset(std::forward<T>(val));
				it = next;
			}
		}

		// Ensure no references left on destruction.
		//
		~value();
	};

	// Use instance.
	//
	struct alignas(8) operand : pinned {
		// Since operand is aligned by 8, operand.prev will also be aligned by 8.
		// - If misaligned (eg check bit 0 == 1), we can use it as a thombstone.
		//
		union {
			struct {
				operand*		  prev;
				operand*		  next;
				shared<value> value_ptr;
			};
			constant const_val;
		};

		// Pointer to user.
		//
		value* const user;

		// Constructed by reference to user, creates a constant of type::none.
		//
		operand(value* user) : user(user), const_val{} {}

		// Assignment.
		//
		void reset() {
			if (is_const()) {
				const_val.reset();
			} else {
				list::unlink(this);
				value_ptr.reset();
				std::construct_at(&const_val);
			}
		}
		void reset(constant value) {
			reset();
			std::construct_at(&const_val, std::move(value));
			RC_ASSERT(is_const());
		}
		void reset(shared<value> value) {
			reset();
			if (value) {
				prev			= this;
				next			= this;
				list::link_before(value->use_list_head(), this);
				std::construct_at(&value_ptr, std::move(value));
				RC_ASSERT(!is_const());
			}
		}
		void reset(operand&& o) {
			if (o.is_const()) {
				return reset(std::move(o.const_val));
			} else {
				return reset(std::move(o.value_ptr));
			}
		}
		void reset(const operand& o) {
			if (o.is_const()) {
				return reset(o.get_const());
			} else {
				return reset(o.value_ptr);
			}
		}
		RC_INLINE operand& operator=(constant o) { reset(std::move(o)); return *this; }
		RC_INLINE operand& operator=(shared<value> o) { reset(std::move(o)); return *this; }
		RC_INLINE operand& operator=(operand&& o) { reset(std::move(o)); return *this; }
		RC_INLINE operand& operator=(const operand& o) { reset(o); return *this; }

		// Observers.
		//
		bool is_const() const { return const_val.__rsvd == 1; }
		bool is_value() const { return !is_const(); }

		const constant& get_const() const {
			RC_ASSERT(is_const());
			return const_val;
		}
		value* get_value() const {
			RC_ASSERT(is_value());
			return value_ptr.get();
		}
		std::string to_string(fmt_style s = {}) const { return is_const() ? get_const().to_string() : get_value()->to_string(s); }
		type			get_type() const { return is_const() ? get_const().get_type() : get_value()->get_type(); }

		// Equality comparison.
		//
		bool equals(const operand& other) const {
			bool isc = is_const();
			if (other.is_const() != isc)
				return false;

			if (isc) {
				return const_val == other.const_val;
			} else {
				return value_ptr == other.value_ptr;
			}
		}
		bool operator==(const operand& other) const { return equals(other); }
		bool operator!=(const operand& other) const { return !equals(other); }

		// Reset on destruction.
		//
		~operand() { reset(); }
	};

	// Gets the list of uses.
	//
	inline range::subrange<list::iterator<operand>> value::uses() const { return list::range(use_list_head()); }

	// Ensure no references left on destruction.
	//
	inline value::~value() { RC_ASSERTS("Destroying value with lingering uses.", !uses()); }
};
