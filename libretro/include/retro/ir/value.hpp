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
	struct value;

	// String formatting style.
	//
	enum class fmt_style : u8 { full, concise };
	
	// Common type for initializing operands.
	//
	struct operand;
	union variant {
		// Since value is aligned by 8, if misaligned (eg check bit 0 == 1), we can use it as a thombstone.
		//
		weak<value> value_ref;
		constant		const_val;

		// Default constructor creates type::none.
		//
		variant() : const_val{} {}
		variant(std::nullopt_t) : variant() {}

		// Constructed by either possible type.
		//
		variant(value* val) : value_ref(val) {}
		variant(weak<value> val) : value_ref(std::move(val)) {}
		variant(constant val) : const_val(std::move(val)) {}
		template<BuiltinType T>
		variant(T val) : const_val(val) {}
		explicit variant(const operand& op);

		// Copy construction and assignment.
		//
		variant(const variant& o) {
			if (o.is_const()) {
				std::construct_at(&const_val, o.get_const());
			} else {
				std::construct_at(&value_ref, o.get_value());
			}
		}
		variant& operator=(const variant& o) {
			variant clone{o};
			swap(clone);
			return *this;
		}

		// Move construction and assignment via swap.
		//
		variant(variant&& o) noexcept : variant() { swap(o); }
		variant& operator=(variant&& o) noexcept {
			swap(o);
			return *this;
		}

		// Trivially relocatable.
		//
		void swap(variant& o) {
			using bytes = std::array<u8, sizeof(variant)>;
			std::swap((bytes&) *this, (bytes&) o);
		}

		// Observers.
		//
		bool is_null() const { return is_const() && const_val.get_type() == type::none; }
		bool is_const() const { return const_val.__rsvd == 1; }
		bool is_value() const { return !is_const(); }

		std::string to_string(fmt_style s = {}) const;
		type			get_type() const;

		constant&& get_const() && {
			RC_ASSERT(is_const());
			return std::move(const_val);
		}
		constant& get_const() & {
			RC_ASSERT(is_const());
			return const_val;
		}
		const constant& get_const() const& {
			RC_ASSERT(is_const());
			return const_val;
		}
		weak<value>&& get_value() && {
			RC_ASSERT(is_value());
			return std::move(value_ref);
		}
		weak<value>& get_value() & {
			RC_ASSERT(is_value());
			return value_ref;
		}
		const weak<value>& get_value() const& {
			RC_ASSERT(is_value());
			return value_ref;
		}

		// Equality comparison.
		//
		bool equals(const variant& other) const {
			bool isc = is_const();
			if (other.is_const() != isc)
				return false;

			if (isc) {
				return const_val == other.const_val;
			} else {
				return value_ref == other.value_ref;
			}
		}
		bool operator==(const variant& other) const { return equals(other); }
		bool operator!=(const variant& other) const { return !equals(other); }

		// Cast to bool for null check.
		//
		explicit operator bool() const { return !is_null(); }

		// Reset on destruction.
		//
		~variant() {
			if (is_const()) {
				const_val.reset();
			} else {
				value_ref.reset();
			}
		}
	};

	// Use instance.
	//
	struct operand : pinned {
		// Since value is aligned by 8, if misaligned (eg check bit 0 == 1), we can use it as a thombstone.
		//
		union {
			variant variant_val = {};
			struct {
				void*		__value_ref;
				operand* prev;
				operand* next;
			};
			constant const_val;
		};
		static_assert(sizeof(weak<value>) == sizeof(void*), "update union.");

		// Pointer to user.
		//
		value* const user;

		// Constructed by reference to user.
		//
		operand(value* user) : user(user) {}

		// Assignment.
		//
		void reset() {
			if (is_const()) {
				const_val.reset();
			} else {
				list::unlink(this);
				variant_val.value_ref = nullptr;
				std::construct_at(&const_val);
			}
		}
		template<typename T>
		void reset(T&& val) {
			if constexpr (std::is_same_v<std::decay_t<T>, variant> || std::is_same_v<std::decay_t<T>, operand>) {
				if (val.is_const()) {
					return reset(val.get_const());
				} else {
					return reset(val.get_value());
				}
			} else {
				reset();
				if constexpr (std::is_convertible_v<T, weak<value>>) {
					if (val) {
						list::init(this);
						list::link_before(val->use_list.entry(), this);
						std::construct_at(&variant_val.value_ref, std::forward<T>(val));
					}
				} else {
					std::construct_at(&const_val, std::forward<T>(val));
				}
			}
		}
		template<typename T>
		operand& operator=(T&& val) {
			reset<T>(std::forward<T>(val));
			return *this;
		}

		// Observers.
		//
		bool is_const() const { return const_val.__rsvd == 1; }
		bool is_value() const { return !is_const(); }
		bool is(type t) const { return get_type() == t; }

		constant&& get_const() && {
			RC_ASSERT(is_const());
			return std::move(const_val);
		}
		constant& get_const() & {
			RC_ASSERT(is_const());
			return const_val;
		}
		const constant& get_const() const& {
			RC_ASSERT(is_const());
			return const_val;
		}
		value* get_value() const {
			RC_ASSERT(!is_const());
			return variant_val.value_ref.get();
		}

		// Redirect utilities to variant.
		//
		std::string to_string(fmt_style s = {}) const { return variant_val.to_string(s); }
		type			get_type() const { return variant_val.get_type(); }
		bool			operator==(const operand& other) const { return variant_val.equals(other.variant_val); }
		bool			operator!=(const operand& other) const { return !variant_val.equals(other.variant_val); }

		// Reset on destruction.
		//
		~operand() { reset(); }
	};

	// Value type.
	//
	struct alignas(8) value : dyn<value>, pinned {
	  private:
		// Circular linked list for the uses.
		//
		list::head<operand> use_list;
		friend operand;

	  public:
		// Temporaries for algorithms.
		//
		mutable u64 tmp_monotonic = 0;
		mutable u64 tmp_mapping	  = 0;

		// String conversion and type getter.
		//
		virtual std::string to_string(fmt_style s = {}) const = 0;
		virtual type		  get_type() const						= 0;

		// Gets the list of uses.
		//
		range::subrange<list::iterator<operand>> uses() const { return list::range(use_list.entry()); }

		// Replaces all uses with another value.
		//
		template<typename T>
		size_t replace_all_uses_with(T&& val) const {
			size_t n = 0;
			for (auto it = use_list.begin(); it != use_list.end();) {
				auto next = std::next(it);
				it->reset(val);
				it = next;
				n++;
			}
			return n;
		}

		// Ensure no references left on destruction.
		//
		~value() { RC_ASSERTS("Destroying value with lingering uses.", !uses()); }
	};

	// Forwards.
	//
	inline variant::variant(const operand& o) {
		if (o.is_const()) {
			std::construct_at(&const_val, o.get_const());
		} else {
			std::construct_at(&value_ref, o.get_value());
		}
	}
	inline std::string variant::to_string(fmt_style s) const { return is_const() ? get_const().to_string() : get_value()->to_string(s); }
	inline type			 variant::get_type() const { return is_const() ? get_const().get_type() : get_value()->get_type(); }
};
