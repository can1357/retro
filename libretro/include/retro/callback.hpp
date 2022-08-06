#pragma once
#include <functional>
#include <retro/list.hpp>
#include <retro/lock.hpp>

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
		mutable rw_lock lock		  = {};
		entry*			 list_prev = nullptr; // Can't use list::head<> as it needs to be trivially initializable.
		entry*			 list_next = nullptr;
		entry*			 get_entry() const { return (entry*) &list_prev; }

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
				std::shared_lock _g{lock};
				if (list_next) {
					for (auto it = list_next; it != get_entry(); it = it->next) {
						it->fn(args...);
					}
				}
			}
			// Callback:
			//
			else {
				std::shared_lock _g{lock};
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
			std::unique_lock _g{lock};

			auto v = new entry();
			v->fn	 = std::move(f);

			if (!list_prev) {
				list::init(get_entry());
			}
			list::link_after(get_entry(), v);
			return handle{v};
		}

		// Removes a callback.
		//
		void remove(handle h) {
			std::unique_lock _g{lock};
			if (auto* v = h.value) {
				list::unlink(v);
				delete v;
			}
		}

		// Destructor deletes all entries.
		//
		~callback_list() {
			if (auto it = list_next) {
				while (it != get_entry()) {
					delete std::exchange(it, it->next);
				}
			}
		}
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