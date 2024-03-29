#pragma once
#include <functional>
#include <retro/list.hpp>
#include <retro/umutex.hpp>

// We manage all functions that user can hook into using either callbacks or notifications.
// - Callbacks always return an optional result that can be converted to a bool and stop propagating once a non-false expression is returned.
// - Notifications invoke every registered entry and must be void.
//
namespace retro {
	// Define the callback table.
	// - TODO: Optimize.
	//
	template<typename F>
	struct callback_list;
	template<typename R, typename... A>
	struct callback_list<R(A...)> {
		using return_type = R;
		using function =    std::function<R(A...)>;

		struct entry {
			entry*	prev = this;
			entry*	next = this;
			function fn	  = {};
		};

		struct handle {
			entry* value = nullptr;
		};

	  protected:
		entry*					 list_prev = nullptr;  // Can't use list::head<> as it needs to be trivially initializable.
		entry*					 list_next = nullptr;
		mutable shared_umutex mtx		  = {};
		entry*					 get_entry() const { return (entry*) &list_prev; }

	  public:
		// Default ctor, no copy.
		//
		callback_list()										  = default;
		callback_list(const callback_list&)				  = delete;
		callback_list& operator=(const callback_list&) = delete;

		// Invokes the callbacks.
		//
		R invoke(const A&... args) const {
			// Notification:
			//
			if constexpr (std::is_void_v<R>) {
				std::shared_lock _g{mtx};
				if (list_next) {
					for (auto it = list_next; it != get_entry(); it = it->next) {
						it->fn(args...);
					}
				}
			}
			// Callback:
			//
			else {
				std::shared_lock _g{mtx};
				R result = {};
				if (list_next) {
					for (auto it = list_next; it != get_entry(); it = it->next) {
						result = it->fn(args...);
						if (result)
							break;
					}
				}
				return result;
			}
		}
		R operator()(const A&... args) const {
			return invoke(args...);
		}

		// Inserts a new callback.
		//
		handle insert(function f) {
			std::unique_lock _g{mtx};

			auto v = new entry();
			v->fn	 = std::move(f);

			if (!list_next) {
				list::init(get_entry());
			}
			list::link_after(get_entry(), v);
			return handle{v};
		}

		// Removes a callback.
		//
		void remove(handle h) {
			std::unique_lock _g{mtx};
			if (auto* v = h.value) {
				list::unlink(v);
				delete v;
			}
		}

		// Clears the list.
		//
		void clear() {
			std::unique_lock _g{mtx};
			if (auto it = std::exchange(list_next, nullptr)) {
				while (it != get_entry()) {
					delete std::exchange(it, it->next);
				}
			}
		}
		~callback_list() { clear(); }
	};
	template<typename... A>
	using notification_list = callback_list<void(A...)>;
	template<typename... A>
	using handler_list =      callback_list<bool(A...)>;
};

	// Helper for builtin callbacks.
	//
#define RC_INSTALL_CB(list, name, ...)                                              \
	static typename decltype(list)::return_type RC_CONCAT(hook_, name)(__VA_ARGS__); \
	RC_INITIALIZER { list.insert(&RC_CONCAT(hook_, name)); };                        \
	static typename decltype(list)::return_type RC_CONCAT(hook_, name)(__VA_ARGS__)
