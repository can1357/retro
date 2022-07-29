#pragma once
#include <retro/common.hpp>
#include <retro/lock.hpp>
#include <retro/rc.hpp>
#include <retro/hash.hpp>
#include <string>
#include <vector>

namespace retro::interface {
	// Given a string hashes it according to the interface manager rules.
	//
	inline constexpr u32 hash(std::string_view data) {
		// -- Must match tablegen.py
		//
		u32 hash = 0xd0b06e5e;
		for (size_t i = 0; i != data.size(); i++) {
			hash ^= 0x20 | data[i];
			hash *= 0x01000193;
		}
		return (hash ? hash : 1);
	}

	// Base class.
	//
	template<typename T>
	struct base {
		// Create a new identifier type for this instance.
		//
		struct RC_TRIVIAL_ABI id {
			u32 value = 0;

			// Default copy move construct.
			//
			constexpr id()									= default;
			constexpr id(id&&) noexcept				= default;
			constexpr id(const id&)						= default;
			constexpr id& operator=(id&&) noexcept = default;
			constexpr id& operator=(const id&)		= default;

			// Construction by value.
			//
			constexpr id(u32 v) : value(v) {}
			constexpr id(std::nullopt_t) : value(0) {}

			// Default comparison.
			//
			constexpr auto operator<=>(const id& o) const = default;

			// String conversion.
			//
			std::string_view to_string() const {
				if (auto r = T::find(*this)) {
					return r->get_name();
				} else if (value) {
					return "[expired-instance]";
				} else {
					return "[null-instance]";
				}
			}
		};

		// Define the handle type.
		//
		using handle = ref<T>;

	  private:
		// Instance map.
		//
		inline static rw_lock				 list_lock = {};
		inline static std::vector<ref<T>> list		  = {};

		// Private identification.
		//
		std::string name;
		id				identifier = std::nullopt;
	  public:

		// Gets the name.
		//
		RC_CONST std::string_view get_name() const { return name; }

		// Gets the ID.
		//
		RC_CONST id get_id() const { return identifier; }

		// String conversion for formatting.
		//
		std::string to_string() const { return name; }

		// Returns whether or not this register is already registered.
		//
		bool is_registered() const { return identifier != std::nullopt; }

		// Registers an instance of the interface.
		//
		RC_NOINLINE static bool register_as(std::string name, ref<T> instance) {
			// Make sure name is not empty.
			//
			if (name.empty()) {
				fmt::abort("Interface name cannot be empty.");
			}

			// Skip if already registered.
			//
			if (instance->is_registered()) {
				return true;
			}

			// Set the registration details.
			//
			instance->name			= std::move(name);
			instance->identifier = (id) hash(instance->name);

			// Acquire the lock and find the position.
			//
			std::unique_lock _g{list_lock};
			auto				  it = range::lower_bound(list, instance, [](auto& a, auto& b) { return a->identifier < b->identifier; });

			// If we found an equal match:
			//
			if (it != list.end() && it->get()->identifier == instance->identifier) {
				// If name is mismatching, hash collision, abort.
				//
				auto& e = *it;
				if (e->name != name) {
					fmt::abort("interface hash collision between %s and %s", e->name.c_str(), name.c_str());
				}

				// Otherwise fail.
				// - TODO: Log duplicate register attempt
				//
				return false;
			}

			// Insert the entry, return success.
			//
			list.insert(it, std::move(instance));
			return true;
		}
		template<typename Ty, typename... Tx>
		static ref<T> register_as(std::string name, Tx&&... args) {
			auto instance = make_rc<Ty>(std::forward<Tx>(args)...);
			if (register_as(std::move(name), instance)) {
				return instance;
			} else {
				return nullptr;
			}
		}

		// Deregisters an interface.
		// - Fails if being used unless force is set.
		//
		RC_NOINLINE static bool deregister(std::string_view name, bool force = false) {
			std::unique_lock _g {list_lock};

			auto it = std::lower_bound(list.begin(), list.end(), id(hash(name)), [](ref<T>& a, id b) { return a->identifier < b; });
			if (it != list.end() && it->get()->name == name) {
				if (force || it->unique()) {
					list.erase(it);
					return true;
				}
				// TODO: log
			}
			return false;
		}

		// Instance enumeration.
		//
		template<typename F>
		static ref<T> find_if(F&& fn) {
			std::shared_lock _g{list_lock};
			for (auto& e : list) {
				if (fn(e)) {
					return e;
				}
			}
			return {};
		}
		template<typename F>
		static void for_each(F&& fn) {
			std::shared_lock _g{list_lock};
			for (auto& e : list) {
				fn(e);
			}
		}
		static std::vector<ref<T>> all() {
			list_lock.lock_shared();
			std::vector<ref<T>> copy = list;
			list_lock.unlock_shared();
			return copy;
		}

		// Instance search.
		//
		static ref<T> find(id i) {
			std::shared_lock _g{list_lock};
			auto				  it = std::lower_bound(list.begin(), list.end(), i, [](ref<T>& a, id b) { return a->identifier < b; });
			if (it != list.end() && it->get()->identifier == i) {
				return *it;
			}
			return {};
		}
		static ref<T> find(std::string_view name) {
			std::shared_lock _g{list_lock};
			auto				  it = std::lower_bound(list.begin(), list.end(), id(hash(name)), [](ref<T>& a, id b) { return a->identifier < b; });
			if (it != list.end() && it->get()->name == name) {
				return *it;
			}
			return {};
		}

		// Virtual destructor for user instances.
		//
		virtual ~base() = default;
	};
	#define RC_ADD_INTERFACE(name, type, ...) RC_INITIALIZER { type::register_as<type>(name, __VA_ARGS__); };
};

namespace retro {
	// User literal for interface ids.
	//
	RC_INLINE consteval u32 operator""_if(const char* n, size_t i) {
		return interface::hash({n,i});
	}
};