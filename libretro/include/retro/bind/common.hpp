#pragma once
#include <optional>
#include <retro/common.hpp>
#include <retro/ctti.hpp>
#include <retro/format.hpp>
#include <retro/func.hpp>
#include <retro/dyn.hpp>
#include <span>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

namespace retro::bind {
	// Value conversion.
	//   bool is(const value&)
	//  value from(engine, const T&)
	//      T as(const value&, bool coerce)
	//
	template<typename Engine, typename T>
	struct converter;

	// Type IDs and descriptors.
	//
	inline u32 next_api_type_id = 0;
	template<typename T>
	struct type_descriptor : std::nullopt_t {};
	template<typename T>
	struct user_class {
		inline static const u32 api_id = next_api_type_id++;
	};
	template<Dynamic T>
	struct user_class<T> {
		inline static const u32 api_id = (T::get_api_id_mut() = next_api_type_id++);
	};
	struct force_rc_t {};
	template<typename T>
	concept UserClass = (!std::is_base_of_v<std::nullopt_t, type_descriptor<T>>);

	template<typename T>
	concept DynUserClass = requires(const T* p) { (u32)p->get_api_id(); };


	// Type name getter.
	//
	template<typename T>
	inline static constexpr std::string_view get_type_name() {
		if constexpr (UserClass<T>) {
			return type_descriptor<T>::name;
		} else {
			return ctti::type_id<T>::name();
		}
	}

	// Implement standard wrappers.
	// - Enums.
	template<typename Engine, typename T>
		requires std::is_enum_v<T>
	struct converter<Engine, T> {
		using value		  = typename Engine::value_type;
		using underlying = std::underlying_type_t<T>;

		bool	is(const value& val) const { return val.template is<underlying>(); }
		value from(const Engine& context, T val) const { return value::make(context, underlying(val)); }
		T		as(const value& val, bool coerce) const { return (T) val.template as<underlying>(coerce); }
	};
	// - std::monostate
	template<typename Engine>
	struct converter<Engine, std::monostate> {
		using value		  = typename Engine::value_type;
		bool				is(const value& val) const { return val.template is<std::nullopt_t>(); }
		value				from(const Engine& context, std::monostate) const { return value::make(context, std::nullopt); }
		std::monostate as(const value& val, bool coerce) const {
			val.template as<std::nullopt_t>(coerce);
			return std::monostate{};
		}
	};
	// - std::optional<T>
	template<typename Engine, typename T>
	struct converter<Engine, std::optional<T>> {
		using value = typename Engine::value_type;

		bool	is(const value& val) const { return val.template is<T>() || val.template is<std::nullopt_t>(); }
		value from(const Engine& context, const std::optional<T>& val) const {
			if (val) {
				return value::make(context, *val);
			} else {
				return value::make(context, std::nullopt);
			}
		}
		std::optional<T> as(const value& val, bool coerce) const {
			if (val.template is<std::nullopt_t>()) {
				return std::nullopt;
			} else {
				return val.template as<T>(coerce);
			}
			throw std::runtime_error(fmt::concat("Expected ", get_type_name<T>(), "?"));
		}
	};
	// - std::vector<T>
	template<typename Engine, typename T>
	struct converter<Engine, std::vector<T>> {
		using value = typename Engine::value_type;
		using array = typename Engine::array_type;

		bool is(const value& val) const {
			if (!val.template is<array>()) {
				return false;
			}
			array arr{val};

			size_t length = arr.length();
			for (size_t i = 0; i != length; i++) {
				if (!arr.get(i).template is<T>()) {
					return false;
				}
			}
			return true;
		}

		array from(const Engine& context, const std::vector<T>& val) const {
			array result = array::make(context, val.size());
			for (size_t i = 0; i != val.size(); i++) {
				result.set(i, val[i]);
			}
			return result;
		}

		std::vector<T> as(const value& val, bool coerce) const {
			if (val.template is<array>()) {
				array	 arr{val};
				size_t length = arr.length();

				std::vector<T> result = {};
				result.reserve(length);
				for (size_t i = 0; i != length; i++) {
					result[i] = arr.get(i).template as<T>(coerce);
				}
				return result;
			}
			throw std::runtime_error(fmt::concat("Expected ", get_type_name<T>(), "[]"));
		}
	};
	// - std::array<T, N>
	template<typename Engine, typename T, size_t N>
	struct converter<Engine, std::array<T, N>> {
		using value = typename Engine::value_type;
		using array = typename Engine::array_type;

		bool is(const value& val) const {
			if (!val.template is<array>()) {
				return false;
			}
			array	 arr{val};
			size_t length = arr.length();
			if (length != N) {
				return false;
			}
			for (size_t i = 0; i != length; i++) {
				if (!arr.get(i).template is<T>()) {
					return false;
				}
			}
			return true;
		}
		array from(const Engine& context, const std::array<T, N>& val) const {
			array result = array::make(context, val.size());
			for (size_t i = 0; i != val.size(); i++) {
				result.set(i, val[i]);
			}
			return result;
		}
		std::array<T, N> as(const value& val, bool coerce) const {
			if (val.template is<array>()) {
				array	 arr{val};
				size_t length = arr.length();
				if (length == N) {
					std::array<T, N> result = {};
					for (size_t i = 0; i != length; i++) {
						result[i] = arr.get(i).template as<T>(coerce);
					}
					return result;
				}
			}
			throw std::runtime_error(fmt::concat("Expected ", get_type_name<T>(), "[", N, "]"));
		}
	};
	// - std::string
	template<typename Engine>
	struct converter<Engine, std::string> {
		using value		  = typename Engine::value_type;
		using underlying = converter<Engine, std::string_view>;

		bool			is(const value& val) const { return underlying{}.is(val); }
		value			from(const Engine& context, const std::string& val) const { return underlying{}.from(context, val); }
		std::string as(const value& val, bool coerce) const {
			if (coerce)
				return val.to_string();
			else
				return std::string{underlying{}.as(val, false)};
		}
	};
	// TODO: std::pair
	// TODO: std::tuple
	// TODO: std::variant
	// - Invocables, write only.
	template<typename Engine, typename F> requires CallableObject<F>
	struct converter<Engine, F> {
		using function =   typename Engine::function_type;
		using value		  = typename Engine::value_type;

		value from(const Engine& context, const F& val) const { return function::make(context, "", val); }
	};

	// Getter setter helpers.
	//
	namespace detail {

		template<typename T, auto V>
		struct setter;
		template<typename T, auto V>
		struct getter;

		template<typename C, typename M, auto V>
		struct getter<M C::*, V> {
			static constexpr auto value = [](C* cl) { return cl->*V; };
		};
		template<typename C, typename M, auto V>
		struct setter<M C::*, V> {
			static constexpr auto value = [](C* cl, M m) { cl->*V = std::move(m); };
		};
	};
	template<auto V>
	static constexpr auto getter = detail::getter<decltype(V), V>::value;
	template<auto V>
	static constexpr auto setter = detail::setter<decltype(V), V>::value;


	// Engine required conversions:
	//  std::nullopt_t
	//  bool
	//  i8,i16,i32,i64,i128
	//  u8,u16,u32,u64,u128
	//  f32,f64,f80
	//  TODO: Vector types
	//  std::string_view
	//  std::span<const u8> & std::vector<u8>
	//  ref<T>, weak<T>, T*
	//  instance::handle, instance*
	//  range::subrange<T>
	//
	
	// Engine type requirements:
	//   typename value_type
	//   typename prototype_type
	//   typename reference_type
	//   typename typedecl_type
	//   typename array_type	  | Constructible from value_type
	//   typename object_type	  |
	//   typename function_type  |
	//            object globals()
	//
	// Value:
	//   typename engine_type
	//            engine context()
	//                 ? native()
	//              bool isinstance<T>()
	//
	// explicit operator bool()
	//       std::string to_string()
	//              bool is<T>()
	//                 T as<T>(bool coerce = false)
	//      static value make<T>(const Engine&, const T&)
	//
	// Reference:
	//   typename engine_type
	//                   ctor(value)
	// explicit operator bool()
	//             value lock()
	//
	// TypeDecl:
	//   typename engine_type
	// static  typedecl* fetch(engine, u32 id)
	// static  typedecl* fetch<T>(engine)
	//
	// Prototype:
	//   typename engine_type
	//             void  set_super<T>()
	//             void  make_iterable<T>(T), T returns range::subrange
	//             void  add_method<T>(const char* name, T)
	//             void  add_async_method<T>(const char* name, T)
	//             void  add_static_method<T>(const char* name, T)
	//             void  add_async_static_method<T>(const char* name, T)
	//
	//             void  add_static<T>(const char* name, T)
	//             void  add_property<T>(const char* name, T getter)
	//             void  add_property<Tg, Ts>(const char* name, Tg getter, Ts setter)
	//             void  add_field_ro<V>(const char* name)
	//             void  add_field_rw<V>(const char* name)
	//
	// Array:Value:
	//   typename engine_type
	//      static array make(engine, size_t length = 0)
	//             bool  has(size_t idx)
	//             value get(size_t idx)
	//             void  set<T>(size_t idx, T)
	//             void  push<T>(T)
	//     	     size_t length()
	//			TODO: erase
	//
	// Object:Value:
	//   typename engine_type
	//     static object make(engine, size_t reserve = 0)
	//             bool  has(const char* field)
	//             value get(const char* field)
	//             void  set<T>(const char* field, T)
	//             bool  has(value field)
	//             value get(value field)
	//             void  set<T>(value field, T)
	//             void  for_each<F>(F&&)
	//             void  freeze()
	//             void  seal()
	//
	// Function:Value:
	//   typename engine_type
	//   static function make<HasThis, F>(engine, std::string_view name, F&&)
	//   static function make_async<HasThis, F>(engine, std::string_view name, F&&)
	//             value invoke<Tx...>(Tx&&...)
	//             value mem_invoke<This,Tx...>(This&&, Tx&&...)
	//
	// Promise:Value:
	//   TODO
	//
};