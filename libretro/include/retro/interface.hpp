#pragma once
#include <retro/common.hpp>
#include <retro/lock.hpp>
#include <retro/rc.hpp>
#include <retro/dyn.hpp>
#include <string>
#include <vector>
#include <atomic>

namespace retro::interface {

	// Maximum number of instances implemented for an interface.
	//
	static constexpr size_t max_instances = 1024;

	// Given a string hashes it according to the interface manager rules.
	//
	using hash = u32;
	inline constexpr hash make_hash(std::string_view data) {
		// -- Must match tablegen.py
		//
		hash hash = 0xd0b06e5e;
		for (size_t i = 0; i != data.size(); i++) {
			hash ^= 0x20 | data[i];
			hash *= 0x01000193;
		}
		return (hash ? hash : 1);
	}

	// Define the handle type.
	//
	template<typename T>
	struct RC_TRIVIAL_ABI handle {
		u32 value = 0;

		// Default copy move construct.
		//
		constexpr handle()									  = default;
		constexpr handle(handle&&) noexcept				  = default;
		constexpr handle(const handle&)					  = default;
		constexpr handle& operator=(handle&&) noexcept = default;
		constexpr handle& operator=(const handle&)	  = default;

		// Construction by value.
		//
		constexpr handle(u32 v) : value(v) {}
		constexpr handle(std::nullopt_t) : value(0) {}

		// Default comparison.
		//
		constexpr auto operator<=>(const handle& o) const = default;

		// Conversion to native value.
		//
		explicit constexpr operator u32() const { return value; }

		// Behave like a pointer.
		//
		T*						 get() const { return T::resolve(*this); }
		T&						 operator*() const { return *get(); }
		T*						 operator->() const { return get(); }
		explicit constexpr operator bool() const { return value != 0; }
		constexpr			 operator T*() const { return get(); }

		// Dynamic cast.
		//
		template<typename X>
		bool is() const {
			return get() && get()->template is<X>();
		}
		template<typename X>
		X* get_if() const {
			if (T* ptr = get()) {
				return ptr->template get_if<X>();
			}
			return nullptr;
		}

		// String conversion.
		//
		std::string_view to_string() const {
			if (T* ptr = get()) {
				return ptr->get_name();
			} else {
				return "[null-instance]";
			}
		}
	};

	// Base class.
	//
	template<typename T>
	struct base : dyn<T> {
		using handle = handle<T>;

	  private:
		// Instance map.
		//
		inline static ref<T>				 list[max_instances] = {};	 // Padded with null to avoid branch for 0 case.
		inline static basic_lock		 list_lock				= {};
		inline static std::atomic<u32> list_last_handle		= 0;

		// Private identification.
		//
		std::string name;
		hash			name_hash = 0;
		handle		hnd		 = std::nullopt;

	  public:
		// Getters.
		//
		RC_CONST std::string_view get_name() const { return name; }
		RC_CONST handle			  get_handle() const { return hnd; }
		RC_CONST hash				  get_hash() const { return name_hash; }

		// String conversion for formatting.
		//
		std::string to_string() const { return name; }

		// Returns whether or not this register is already registered.
		//
		bool is_registered() const { return (bool) hnd; }

		// Registers an instance of the interface.
		//
		RC_NOINLINE static handle register_as(std::string name, ref<T> instance) {
			// Make sure name is not empty.
			//
			if (name.empty()) {
				fmt::abort("Interface name cannot be empty.");
			}

			// Skip if already registered.
			//
			if (instance->is_registered()) {
				return instance->hnd;
			}

			// Set the name.
			//
			instance->name_hash = make_hash(name);
			instance->name		  = std::move(name);

			// Acquire the lock and make sure there's no other entry colliding.
			//
			std::unique_lock _g{list_lock};
			for (auto& e : all()) {
				if (e->name_hash == instance->name_hash) {
					if (e->name == instance->name)
						fmt::abort("interface name collision %s", name.c_str());
					else
						fmt::abort("interface hash collision %s vs %s", e->name.c_str(), instance->name.c_str());
				}
			}

			// Write to the list and return the handle.
			//
			u32 idx				  = ++list_last_handle;
			instance->hnd.value = idx;
			list[idx]			  = std::move(instance);
			return handle(idx);
		}
		template<typename Ty, typename... Tx>
		static handle register_as(std::string name, Tx&&... args) {
			auto instance = make_rc<Ty>(std::forward<Tx>(args)...);
			return register_as(std::move(name), std::move(instance));
		}

		// Instance enumeration.
		//
		static std::span<const ref<T>> all() { return {&list[1], list_last_handle.load(std::memory_order::relaxed)}; }

		// Instance search.
		//
		template<typename F>
		static handle find_if(F&& fn) {
			for (auto& e : all()) {
				if (fn(e)) {
					return handle(u32(&e - &list[0]));
				}
			}
			return std::nullopt;
		}
		static handle lookup(hash name_hash) {
			// TODO: Optimize later.
			for (auto& e : all()) {
				if (e->name_hash == name_hash) {
					return handle(u32(&e - &list[0]));
				}
			}
			return std::nullopt;
		}
		static handle lookup(std::string_view name) {
			for (auto& e : all()) {
				if (e->name == name) {
					return handle(u32(&e - &list[0]));
				}
			}
			return std::nullopt;
		}
		static T* resolve(handle h) { return list[h.value].get(); }

		// Virtual destructor for user instances.
		//
		virtual ~base() = default;
	};
	#define RC_ADD_INTERFACE(name, type, ...) RC_INITIALIZER { type::register_as<type>(name, __VA_ARGS__); };
};

namespace retro {
	// User literal for interface hashes.
	//
	RC_INLINE consteval interface::hash operator""_ihash(const char* n, size_t i) { return interface::make_hash({n, i}); }
};