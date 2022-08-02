#pragma once
#include <retro/common.hpp>
#include <retro/coro.hpp>
#include <retro/list.hpp>
#include <retro/lock.hpp>
#include <retro/rc.hpp>
#include <optional>
#include <vector>

namespace retro {
	// Work entry.
	//
	struct work {
		work* prev = this;
		work* next = this;

		// Executes the work and deletes it on completion.
		//
		virtual void run() {}

		// Queues the work.
		//
		void queue();
	};

	// Promise type.
	//
	template<typename T>
	struct promise : work {
		// Observers.
		//
		bool pending() const { return state.load(std::memory_order::relaxed) == 0; }
		bool complete() const { return state.load(std::memory_order::relaxed) != 0; }

		T& value() const {
			RC_ASSERT(complete());
			return *(T*) &data;
		}

		// Destructor.
		//
		~promise() {
			if (complete()) {
				std::destroy_at(&value());
			}
		}

		// Implement work functions.
		//
		template<typename Ty>
		void emplace(Ty&& value) {
			// Write the data and mark complete.
			//
			std::construct_at((T*) &data, std::forward<Ty>(value));
			state.store(1);
			state.notify_all();

			// Runthrough the continuation list.
			//
			std::lock_guard _g{continuation_lock};
			for (work* w : continuation) {
				w->run();
			}

			// Decrement references.
			//
			rc_header::from(this)->dec_ref();
		}
	  private:
		// Data and state.
		//
		alignas(T) u8 data[sizeof(T)];
		std::atomic<u32> state = 0;

		// Continuation list.
		//
		simple_lock		  continuation_lock = {};
		list::head<work> continuation = {};
		friend struct awaitable;

	  public:
		// Make awaitable.
		//
		struct awaitable final : work {
			mutable ref<promise<T>> ptr;
			coroutine_handle<>		hnd;

			awaitable(ref<promise<T>> ptr) : ptr(std::move(ptr)) {}

			// Implement work interface.
			//
			void run() override { hnd(); }

			// Implement await.
			// - Continuation is ran before deref so it is safe to ignore reference counting.
			//
			inline bool		 await_ready() { return !ptr->pending(); }
			inline const T& await_resume() const { return ptr->value(); }

			inline bool await_suspend(coroutine_handle<> hnd) {
				this->hnd = hnd;
				std::lock_guard _g{ptr->continuation_lock};
				if (!ptr->pending())
					return false;
				list::link_before(ptr->continuation.entry(), this);
				return true;
			}
		};

		// Make waitable.
		//
		const T& wait() {
			while (!state.load(std::memory_order::relaxed)) {
				state.wait(0, std::memory_order::relaxed);
			}
			return value();
		}
	};
	template<typename T>
	inline auto operator co_await(ref<promise<T>> ref) {
		using A = typename promise<T>::awaitable;
		return A{std::move(ref)};
	}

	// Worker implementations.
	//
	namespace detail {
		template<typename F>
		struct lambda_worker final : work {
			F fn;

			lambda_worker(F&& fn) : fn(std::move(fn)) {}
			lambda_worker(const F& fn) : fn(fn) {}

			void run() override {
				fn();
				delete this;
			}
		};
		template<typename F, typename T>
		struct lambda_promise final : promise<T> {
			F fn;

			lambda_promise(F&& fn) : fn(std::move(fn)) {}
			lambda_promise(const F& fn) : fn(fn) {}

			void run() override {
				this->emplace(fn());
			}
		};
	};

	// Schedules a work with no promise to be executed at a later point.
	//
	template<typename F>
	inline void later(F&& fn) {
		auto* w = new detail::lambda_worker<std::decay_t<F>>(std::forward<F>(fn));
		w->queue();
	}

	// Schedules a work with result and allocates a promise.
	//
	template<typename F>
	inline auto async(F&& fn) {
		using T = decltype(fn());
		auto w  = make_rc<detail::lambda_promise<std::decay_t<F>, T>>(std::forward<F>(fn));
		w->queue();
		return ref<promise<T>>(w.release());
	}

	// Helper to reschedule a routine.
	//
	struct yield {
		inline bool await_ready() { return false; }
		inline void await_suspend(coroutine_handle<> h) { later(h); }
		inline void await_resume() {}
	};

	// Simple wrapper for a coroutine starting itself and destorying itself on finalization.
	//
	struct async_task {
		struct promise_type {
			async_task	  get_return_object() { return {}; }
			suspend_never initial_suspend() noexcept { return {}; }
			suspend_never final_suspend() noexcept { return {}; }
			RC_UNHANDLED_RETHROW;
			void return_void() {}
		};
		async_task() {}
	};

	// Unique task.
	//
	template<typename T>
	struct unique_task {
		struct promise_type {
			// Symetric transfer.
			//
			coroutine_handle<> continuation = nullptr;

			// Result.
			//
			alignas(T) u8 data[sizeof(T)];
			std::atomic<u32> state = 0;

			// Make sure continuation was not leaked on destruction.
			//
			~promise_type() {
				RC_ASSERT(!continuation);	// Should not leak continuation!
				RC_ASSERT(state.load(std::memory_order::relaxed) == 2);
				std::destroy_at((T*) &data);
			}

			struct final_awaitable {
				inline bool await_ready() noexcept { return false; }

				template<typename P = promise_type>
				inline coroutine_handle<> await_suspend(coroutine_handle<P> handle) noexcept {
					auto& pr = handle.promise();
					if (auto c = std::exchange(pr.continuation, nullptr))
						return c;
					else
						return noop_coroutine();
				}
				inline void await_resume() const noexcept {}
			};

			unique_task<T>	 get_return_object() { return *this; }
			suspend_always	 initial_suspend() noexcept { return {}; }
			final_awaitable final_suspend() noexcept { return {}; }
			RC_UNHANDLED_RETHROW;

			template<typename Ty>
			void return_value(Ty&& v) {
				// Write the data and mark complete.
				//
				std::construct_at((T*) &data, std::forward<Ty>(v));
				state.store(2);
				state.notify_all();
			}
		};

		// Coroutine handle and the internal constructor.
		//
		unique_coroutine<promise_type> handle	= nullptr;
		unique_task(promise_type& pr) : handle(pr) {}

		// Null constructor and validity check.
		//
		constexpr unique_task() = default;
		constexpr unique_task(std::nullptr_t) : unique_task() {}
		constexpr bool		 has_value() const { return handle != nullptr; }
		constexpr explicit operator bool() const { return has_value(); }

		// No copy, default move.
		//
		constexpr unique_task(unique_task&&) noexcept				= default;
		constexpr unique_task(const unique_task&)						= delete;
		constexpr unique_task& operator=(unique_task&&) noexcept = default;
		constexpr unique_task& operator=(const unique_task&)		= delete;

		// State.
		//
		bool complete() const { return handle.promise().state.load(std::memory_order::relaxed) == 2; }
		bool pending() const { return handle.promise().state.load(std::memory_order::relaxed) == 1; }
		bool lazy() const { return handle.promise().state.load(std::memory_order::relaxed) == 0; }

		// Starts the task if not already done so, returns the previous state.
		//
		long signal() const {
			u32 expected = 0;
			if (handle.promise().state.compare_exchange_strong(expected, 1)) {
				handle.resume();
			}
			return expected;
		}

		// Value getter.
		//
		T& get() & {
			// If not done yet, enter the wait loop.
			//
			if (signal() != 2) [[unlikely]] {
				auto& s = handle.promise().state;
				do {
					s.wait(1, std::memory_order::relaxed);
				} while (s.load(std::memory_order::relaxed) != 2);
			}

			// Return the value.
			//
			return *(T*) &handle.promise().data;
		}
		T&& get() && { return std::move(get()); }
		const T& get() const& { return const_cast<unique_task*>(this)->get(); }

		// Cannot be destructed whilist its executing.
		//
		~unique_task() { get(); }
	};

	// Make unique task co-awaitable, note that only one coroutine may wait on unique task.
	//
	template<typename T>
	inline auto operator co_await(const unique_task<T>& ref) {
		struct awaitable {
			const unique_task<T>& ref;
			long						 state;

			inline bool await_ready() {
				state = ref.signal();
				return state == 2;
			}
			inline const auto& await_resume() const { return *(const T*) &ref.handle.promise().data; }

			inline void await_suspend(coroutine_handle<> hnd) {
				auto& pr = ref.handle.promise();
				RC_ASSERT(pr.continuation == nullptr);
				pr.continuation = hnd;
			}
		};
		return awaitable{ref};
	}
};