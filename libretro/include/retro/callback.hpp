#pragma once
#include <functional>
#include <list>
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
		using function = std::function<R(A...)>;
		struct handle {
			using T = typename std::list<function>::const_iterator;
			T value;
		};

	  protected:
		mutable rw_lock	  lock = {};
		std::list<function> list = {};

	  public:
		// Invokes the callbacks.
		//
		R invoke(const A&... args) const {
			// Notification:
			//
			if constexpr (std::is_void_v<R>) {
				std::shared_lock _g{lock};
				for (auto& e : list) {
					e(args...);
				}
			}
			// Callback:
			//
			else {
				std::shared_lock _g{lock};

				R result = {};
				for (auto& e : list) {
					result = e(args...);
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
			return handle{list.insert(list.begin(), std::move(f))};
		}

		// Removes a callback.
		//
		void remove(handle h) {
			std::unique_lock _g{lock};
			if (h != list.end()) {
				list.erase(std::exchange(h.value, list.end()));
			}
		}
	};
	template<typename... A>
	using notification_list = callback_list<void, A...>;
	template<typename... A>
	using handler_list =      callback_list<bool, A...>;
};