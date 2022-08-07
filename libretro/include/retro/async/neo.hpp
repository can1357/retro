#pragma once
#include <retro/common.hpp>
#include <retro/coro.hpp>
#include <retro/rc.hpp>
#include <retro/umutex.hpp>
#include <retro/format.hpp>
#include <retro/list.hpp>
#include <semaphore>
#include <atomic>
#include <thread>
#include <variant>

// Neo defines a strictly structured task system. There are two kinds of coroutines:
// 
// Tasks:
//  - Should not have any side effects if cancelled.
//  - Scheduled in a specific domain (instance of neo::scheduler).
//  - Should only await on another subtask.
//  - Does not start autoamtically on creation, user has to push it into a scheduler and receive a task_ref.
//
// Subtasks:
//  - Should have no effect on globals but is not necessarily pure either.
//  - Never cancelled individually, only terminated if the owning task is also cancelled.
//  - co_yield neo::checkpoint{} 's to signal reschedulable point
//  - Starts on creation.
//
namespace retro::neo {
	// Priority groups.
	//
	enum class priority {
		low	 = 1,
		normal = 4,
		high	 = 10,
	};
	static constexpr auto time_slice_coeff = 50ms;

	// Task state.
	//
	struct task_state {
		enum class id : u8 {
			pending,
			cancelled,
			complete
		};

		mutable std::counting_semaphore<> completion_semaphore{0};
		volatile bool							 cancellation_signal = false;
		volatile id								 state					= id::pending;

		// Task state observers.
		//
		bool pending() const { return state == id::pending; }
		bool cancelled() const { return state == id::cancelled; }
		bool done() const { return state == id::complete; }

		// Signals task cancellation asynchronously.
		//
		void cancel_async() { cancellation_signal = true; }

		// Blocks until task is either completed or cancelled, returns true if completed as expected.
		//
		bool join() {
#if RC_DEBUG
			if (cancellation_signal) {
				if (!completion_semaphore.try_acquire_for(5s)) {
					fmt::abort("Task stuck too long with no yields!");
				}
				return done();
			}
#endif
			completion_semaphore.acquire();
			return done();
		}

		// Signals task cancellation, waits for the task to exit, returns true if we managed to cancel before task completion.
		//
		bool cancel() {
			cancel_async();
			return !join();
		}
	};

	// Defines the state of a state.
	//
	struct task_promise {
		// Internal task list.
		//
		task_promise*		 prev							= nullptr;
		task_promise*		 next							= nullptr;

		// End of the given time slice, priority.
		//
		timestamp			 slice_end					= {};
		neo::priority		 prio							= neo::priority::normal;

		// Last coroutine in the callstack.
		//
		coroutine_handle<> suspended_continuation = {};

		// Override delete to allocate an externally managed task_state and converts this allocation to be ref-counted.
		//
		void*			operator new(size_t n) { return make_overalloc_rc<task_state>(n).release() + 1; }
		void			operator delete(void* p) { rc_header::from(std::prev((task_state*) p))->dec_ref(); }
		task_state* get_state() const { return std::prev((task_state*) coroutine_handle<task_promise>::from_promise(*(task_promise*) this).address()); }

		// Define common interface with subtasks.
		//
		task_promise* get_task() { return this; }

		// Set current coroutine as continuation on initialization, never start or delete automatically.
		//
		task_promise& get_return_object() {
			suspended_continuation = coroutine_handle<task_promise>::from_promise(*this);
			return *this;
		}
		suspend_always initial_suspend() noexcept { return {}; }
		suspend_always final_suspend() noexcept { return {}; }
		RC_UNHANDLED_RETHROW;
		void return_void() {}
	};

	// Define subtask promises.
	//
	struct symmetric_transfer_awaitable {
		coroutine_handle<> hnd;
		bool					 await_ready() const noexcept { return false; }
		coroutine_handle<> await_suspend(coroutine_handle<>) const noexcept { return hnd; }
		void					 await_resume() const noexcept {}
	};
	struct subtask_promise_base {
		task_promise*		 owner		  = nullptr;
		coroutine_handle<> continuation = nullptr;

		task_promise* get_task() { return owner; }

		suspend_always					  initial_suspend() noexcept { return {}; }
		symmetric_transfer_awaitable final_suspend() noexcept { return {continuation}; }
		RC_UNHANDLED_RETHROW;
	};
	template<typename R>
	struct subtask_promise : subtask_promise_base {
		std::optional<R> result = std::nullopt;
		subtask_promise& get_return_object() { return *this; }
		template<typename T>
		void return_value(T&& value) {
			result.emplace(std::forward<T>(value));
		}
	};
	template<>
	struct subtask_promise<void> : subtask_promise_base {
		std::optional<std::monostate> result = std::nullopt;
		subtask_promise& get_return_object() { return *this; }
		void				  return_void() { result.emplace(); }
	};

	// Define the awaitable type for subtasks.
	//
	template<typename R>
	struct subtask_awaitable {
		subtask_promise<R>& pr;

		inline bool	  await_ready() { return pr.result.has_value(); }
		inline auto&& await_resume() const { return std::move(pr.result).value(); }

		inline coroutine_handle<> suspend_handle(coroutine_handle<> hnd) {
			RC_ASSERT(pr.continuation == nullptr);
			pr.continuation = hnd;
			return coroutine_handle<subtask_promise<R>>::from_promise(pr);
		}

		template<typename T>
		inline coroutine_handle<> await_suspend(coroutine_handle<T> hnd) {
			auto result = suspend_handle(coroutine_handle<>{hnd});
			pr.owner		= hnd.promise().get_task();
			return result;
		}
	};

	// Subtask coroutine.
	//
	template<typename R>
	struct subtask {
		using promise_type = subtask_promise<R>;

		// Coroutine handle and the internal constructor.
		//
		unique_coroutine<promise_type> handle = nullptr;
		subtask(promise_type& pr) : handle(pr) {}

		// Null constructor and validity check.
		//
		constexpr subtask() = default;
		constexpr subtask(std::nullptr_t) : handle() {}
		bool		has_value() const { return handle.get() != nullptr; }
		explicit operator bool() const { return has_value(); }

		// No copy, default move.
		//
		subtask(subtask&&) noexcept										 = default;
		subtask&							 operator=(subtask&&) noexcept = default;
		inline subtask_awaitable<R> operator co_await() && { return {handle.promise()}; }
	};

	// Task coroutine and the reference type to its state.
	//
	struct task {
		using promise_type = task_promise;

		// Coroutine handle and the internal constructor.
		//
		unique_coroutine<promise_type> handle = nullptr;
		task(promise_type& pr) : handle(pr) {}

		// Null constructor and validity check.
		//
		constexpr task() = default;
		constexpr task(std::nullptr_t) : handle() {}
		bool		has_value() const { return handle.get() != nullptr; }
		explicit operator bool() const { return has_value(); }

		// No copy, default move.
		//
		task(task&&) noexcept				= default;
		task& operator=(task&&) noexcept = default;
	};
	using task_ref = ref<task_state>;

	// Checkpoint for rescheduling.
	//
	struct checkpoint {
		inline bool await_ready() { return false; }

		template<typename T>
		inline coroutine_handle<> await_suspend(coroutine_handle<T> hnd) {
			auto*			  chain = &hnd.promise();
			task_promise* o	  = chain->get_task();
			if (!o)
				return hnd;

			if (o->get_state()->cancellation_signal || o->slice_end < now()) {
				o->suspended_continuation = hnd;
				return noop_coroutine();
			}
			return hnd;
		}
		inline void await_resume() {}
	};

	// Scheduler instance.
	//
	struct scheduler : pinned {
	  private:
		// Internals.
		//
		std::counting_semaphore<> signal{0};
		spinlock						  lock					  = {};
		list::head<task_promise>  queue					  = {};
		std::vector<std::thread>  thread_list			  = {};
		std::atomic<u64>			  remaining_task_count = 0;
		volatile bool				  termination_signal	  = false;

		// Pushes a task for rescheduling.
		void			  push(task_promise* w);
		// Pops a task promise for execution.
		task_promise* pop();
		// Worker thread entry point.
		static void	  thread_main(scheduler* schd);

	  public:
		// Constructs a scheduler with the given number of threads.
		//
		scheduler(size_t n = 0);

		// Cancels all tasks.
		//
		void clear();

		// Schedules a task.
		//
		task_ref insert(task&& t, priority prio = priority::normal);

		// Waits until all tasks in the scheduler are completed.
		//
		void wait_until_empty() {
			while (size_t n = remaining_task_count.load()) {
				remaining_task_count.wait(n);
			}
		}

		// Handle cancellation of leftover tasks and thread deletion on destruction.
		//
		~scheduler();
	};
};