#pragma once
#include <retro/common.hpp>
#include <retro/lock.hpp>
#include <retro/rc.hpp>
#include <string>
#include <vector>

namespace retro {
	template<typename T>
	struct interface_manager {
		// Instance map.
		//
	  private:
		inline static rw_lock				 list_lock = {};
		inline static std::vector<ref<T>> list		  = {};
		std::string								 name;

	  public:
		// Gets the name.
		//
		std::string_view get_name() const { return name; }

		// String conversion for formatting.
		//
		std::string to_string() const { return name; }

		// Registers an instance of the given type.
		//
		template<typename Ty>
		static bool register_as(std::string name) {
			if (name.empty()) {
				return false;
			}

			// Fail if already in the list.
			//
			std::unique_lock _g{list_lock};
			for (auto& e : list) {
				if (e->name == name) {
					return false;
				}
			}

			// Insert and set the name.
			//
			auto& entry = list.emplace_back(make_rc<Ty>());
			entry->name = std::move(name);
			return true;
		}

		// Observers for the list.
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
		static ref<T> find(std::string_view name) {
			std::shared_lock _g{list_lock};
			for (auto& e : list) {
				if (e.name == name) {
					return e;
				}
			}
			return {};
		}

		// Virtual destructor for user instances.
		//
		virtual ~interface_manager() = default;
	};
	#define RC_REGISTER_INTERFACE(name, type) RC_INITIALIZER { type::register_as<type>(name); };
};