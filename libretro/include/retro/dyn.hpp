#pragma once
#include <retro/common.hpp>
#include <retro/ctti.hpp>
#include <retro/format.hpp>
#include <span>
#include <memory>
#include <utility>
#include <algorithm>
#include <string_view>

// Fast alternative to RTTI & dynamic_cast for types with a single base class.
//
namespace retro {
	namespace detail {
		struct dyn_tag_t {};

		// Dynamic info in .rdata.
		//
#pragma pack(push, 1)
		template<typename Self, typename BaseInfo>
		struct dyn_info : BaseInfo {
			ctti::id entry = ctti::of<Self>;
			constexpr dyn_info() {
				this->name			  = ctti::type_id<Self>::name();
				this->highest_entry = ctti::of<Self>;
				this->size			  = sizeof(Self);
				this->class_index++;
			}
		};
		template<>
		struct dyn_info<void, void> {
			std::string_view			  name			 = {};
			ctti::id						  highest_entry = 0;
			u32							  size			 = 0;
			i32							  class_index	 = -1;
			RC_INLINE std::span<const ctti::id> bases() const { return {(const ctti::id*) (this + 1), size_t(class_index + 1)}; }
		};
		template<typename Self>
		struct dyn_info<Self, void> : dyn_info<void, void> {
			constexpr dyn_info() {
				this->name			  = ctti::type_id<Self>::name();
				this->highest_entry = ctti::of<Self>;
				this->size			  = sizeof(Self);
			}
		};
#pragma pack(pop)

		// Base class implementing the interface.
		//
		template<typename Base>
		struct dyn_base : dyn_tag_t {
			using base_type = Base;

			// Default construction, copy and move.
			//
			constexpr dyn_base()											= default;
			constexpr dyn_base(dyn_base&&) noexcept				= default;
			constexpr dyn_base(const dyn_base&)						= default;
			constexpr dyn_base& operator=(dyn_base&&) noexcept = default;
			constexpr dyn_base& operator=(const dyn_base&)		= default;

			// Dynamic traits.
			//
			virtual Base* clone() const = 0;
			virtual ~dyn_base() = default;

			RC_INLINE constexpr std::string_view type_name() const { return type_info().name; }
			RC_INLINE constexpr ctti::id			 identify() const { return type_info().highest_entry; }
			RC_INLINE constexpr auto&				 type_info() const { return *info; }
			RC_INLINE constexpr u64					 size_in_memory() const { return type_info().size; }

			// Type check.
			//
			template<typename X>
			RC_INLINE constexpr bool is() const {
				// If X is below or equal to base type, always true.
				//
				if constexpr (std::is_base_of_v<X, Base>) {
					return true;
				}
				// If X is not above base type, invalid check.
				//
				else if constexpr (!std::is_base_of_v<Base, X>) {
					static_assert(sizeof(X) == -1, "Invalid dyn check.");
				}
				// If final emit pointer comparison.
				//
				else if constexpr (std::is_final_v<X>) {
					return info == &static_instance<typename X::Info>;
				}
				// Otherwise, check the hierarchy.
				//
				else {
					constexpr auto& target_info = static_instance<typename X::Info>;
					return info->class_index >= target_info.class_index && info->bases().data()[target_info.class_index] == ctti::of<X>;
				}
			}

			// Implement the getters using it.
			//
			template<typename X>
			RC_INLINE X& as() {
				RC_ASSERT(is<X>());
				return *(X*) this;
			}
			template<typename X>
			RC_INLINE const X& as() const {
				RC_ASSERT(is<X>());
				return *(const X*) this;
			}
			template<typename X>
			RC_INLINE X* opt() {
				if (!is<X>())
					return nullptr;
				return (X*) this;
			}
			template<typename X>
			RC_INLINE const X* opt() const {
				if (!is<X>())
					return nullptr;
				return (const X*) this;
			}

		  protected:
			const dyn_info<void, void>* info = nullptr;
		};

		// Tag inheriting from the base class, sets the information pointer.
		//
		template<typename Self, typename Base, typename Info>
		struct dyn_tag : Base {
			// Implement the traits.
			//
			typename Base::base_type* clone() const override {
				if constexpr (std::is_copy_constructible_v<Self>) {
					return new Self(*(const Self*) this);
				} else {
					assume_unreachable();
				}
			}

			// Set the info on construction.
			//
			RC_INLINE constexpr dyn_tag() { this->info = &static_instance<Info>; }
			constexpr dyn_tag(dyn_tag&&) noexcept				 = default;
			constexpr dyn_tag(const dyn_tag&)					 = default;
			constexpr dyn_tag& operator=(dyn_tag&&) noexcept = default;
			constexpr dyn_tag& operator=(const dyn_tag&)		 = default;
		};
	};

	// Dynamic base type with inheritence.
	//
	template<typename Self, typename Base = void>
	struct dyn : detail::dyn_tag<Self, Base, detail::dyn_info<Self, typename Base::Info>> {
	  private:
		using Info = detail::dyn_info<Self, typename Base::Info>;

		template<typename Ty, typename Ty2>
		friend struct dyn;
		friend struct detail::dyn_base<typename Base::base_type>;
	};

	// Dynamic base type.
	//
	template<typename Self>
	struct dyn<Self, void> : detail::dyn_tag<Self, detail::dyn_base<Self>, detail::dyn_info<Self, void>> {
	  private:
		using Info = detail::dyn_info<Self, void>;

		template<typename Ty, typename Ty2>
		friend struct dyn;
		friend struct detail::dyn_base<Self>;
	};

	// Concept for checking if type dynamic.
	//
	template<typename T>
	concept Dynamic = std::is_base_of_v<detail::dyn_tag_t, T>;

	// Unique ptr with copy.
	//
	template<typename T>
	struct RC_TRIVIAL_ABI dyn_box {
		T* pointer = nullptr;

		// Simple construction.
		//
		constexpr dyn_box() = default;
		constexpr dyn_box(std::nullptr_t) {}
		template<typename Ty> requires(std::is_convertible_v<Ty*, T*>)
		explicit constexpr dyn_box(Ty* ptr) : pointer(ptr) {}
		constexpr dyn_box(std::unique_ptr<T> other) : dyn_box(other.release()) {}

		// Convertible types.
		//
		template<typename T2> requires(std::is_convertible_v<T2*, T*> && !std::is_same_v<T, T2>)
		dyn_box(dyn_box<T2> o) : dyn_box((T*) o.release()) {}
		template<typename T2> requires(std::is_convertible_v<T2*, T*> && !std::is_same_v<T, T2>)
		dyn_box& operator=(dyn_box<T2> o) {
			reset((T*) o.release());
			return *this;
		}

		// Move.
		//
		constexpr dyn_box(dyn_box&& o) noexcept : dyn_box(o.release()) {}
		constexpr dyn_box& operator=(dyn_box&& o) noexcept {
			reset(o.release());
			return *this;
		}

		// Copy.
		//
		dyn_box(const dyn_box& o) : dyn_box{o.clone()} {}
		dyn_box& operator=(const dyn_box& o) {
			reset(o.clone().release());
			return *this;
		}

		// Observers.
		//
		constexpr T* get() const { return pointer; }
		constexpr explicit operator bool() const { return pointer != nullptr; }
		constexpr T*		 operator->() const { return get(); }
		constexpr T&		 operator*() const { return *get(); }

		dyn_box clone() const {
			if (pointer) {
				return dyn_box<T>{(T*) pointer->clone()};
			}
			return nullptr;
		}

		// Modifiers.
		//
		constexpr void swap(dyn_box& other) { std::swap(pointer, other.pointer); }
		constexpr T*	release() { return std::exchange(pointer, nullptr); }
		constexpr void reset(T* ptr = nullptr) {
			std::swap(ptr, pointer);
			if (ptr)
				delete ptr;
		}

		// Reset on destruction.
		//
		constexpr ~dyn_box() { reset(); }
	};

	// Deduction guides.
	//
	template<typename T>
	dyn_box(std::unique_ptr<T>) -> dyn_box<T>;
	template<typename T>
	dyn_box(T*) -> dyn_box<T>;

	// std::make_unique equivalent.
	//
	template<typename T, typename... Tx>
	inline dyn_box<T> make_dyn(Tx&&... args) {
		return dyn_box{new T(std::forward<Tx>(args)...)};
	}
};
