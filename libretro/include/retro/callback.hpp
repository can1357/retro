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
	template<typename R, typename... A>
	struct callback_list {
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
		mutable rw_lock	  lock = {};
		list::head<entry>	  list = {};

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
				for (auto* e : list) {
					e->fn(args...);
				}
			}
			// Callback:
			//
			else {
				std::shared_lock _g{lock};

				R result = {};
				for (auto* e : list) {
					result = e->fn(args...);
					if (result)
						break;
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
			list::link_after(list.end().get(), v);
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
			auto it = list.begin();
			while (it != list.end()) {
				delete std::exchange(it, it->next);
			}
		}
	};
	template<typename... A>
	using notification_list = callback_list<void, A...>;
	template<typename... A>
	using handler_list =      callback_list<bool, A...>;
};