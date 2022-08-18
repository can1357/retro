#pragma once
#include <retro/common.hpp>
#include <retro/coro.hpp>
#include <retro/rc.hpp>
#include <retro/umutex.hpp>
#include <retro/func.hpp>
#include <retro/format.hpp>
#include <retro/list.hpp>
#include <retro/platform.hpp>
#include <retro/heap.hpp>
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
	enum class priority : u16 {
		low	 = 1,
		normal = 4,
		high	 = 10,
		inf	 = 0xFFFF,
	};
	static constexpr auto time_slice_coeff = 50ms;

	// Task cancellation exception.
	//
	struct task_cancelled_exception : std::exception {
		task_cancelled_exception() noexcept : std::exception() {}
		const char* what() const override { return "Task cancelled."; }
	};

	// Finally clause.
	//
	struct task_state;
	struct finally_clause {
		finally_clause* next											= nullptr;
		virtual void	 on_finally(task_state* f, bool ok) = 0;
		virtual ~finally_clause()									= default;
	};

	// Task state.
	//
	struct task_state {
		enum class id : u16 { pending, cancelled, success, error };

		// Internal linked list.
		//
		task_state* prev = this;
		task_state* next = this;

		// End of the given time slice, priority.
		//
		timestamp	  slice_end = {};
		neo::priority prio		= neo::priority::normal;

		// First and the last coroutine in the callstack.
		//
		coroutine_handle<> coro_promise = {};
		coroutine_handle<> coro_current = {};

		// Task state.
		//
		std::atomic<id> state					= id::pending;
		volatile bool	 cancellation_signal = false;

		// Finally list.
		//
		spinlock			 fin_lock = {};
		finally_clause* fin_list = nullptr;

		// Data and the destructor.
		//
		void (*dtor)(void*) = nullptr;
		void* data_address() const { return (void*) (this + 1); }

		// Task implementations.
		//
		void task_signal(id s) {
			RC_ASSERT(state == id::pending && s != id::pending);
			state = s;
			state.notify_all();

			// Lock the finally list and acquire it.
			//
			std::unique_lock flock{fin_lock};
			auto				  list = std::exchange(fin_list, nullptr);
			flock.unlock();

			// Until we're out of entries, notify all.
			//
			while (list) {
				auto it = std::exchange(list, list->next);
				it->on_finally(this, s == id::success);
				delete it;
			}
		}
		void task_emplace_void() {
			task_signal(id::success);
		}
		template<typename Ty, typename V>
		void task_emplace(V&& v) {
			dtor = [](void* p) { std::destroy_at((Ty*) p); };
			new (data_address()) Ty(std::forward<V>(v));
			task_signal(id::success);
		}
		void task_raise(std::exception_ptr e) {
			dtor = [](void* p) { std::destroy_at((std::exception_ptr*) p); };
			new (data_address()) std::exception_ptr(std::move(e));
			task_signal(id::error);
		}
		void task_cancel() {
			if (state != id::cancelled)
				task_signal(id::cancelled);
		}

		// Promise implementations.
		//
		id promise_wait() const {
			id st;
			while (true) {
				st = state.load(std::memory_order::relaxed);
				if (st != id::pending) {
					break;
				}
				state.wait(st);
			}
			return st;
		}
		template<typename Ty = void>
		decltype(auto) promise_get() const {
			id st = promise_wait();

			// Return the result if complete.
			//
			if (st == id::success) [[likely]] {
				if constexpr (std::is_void_v<Ty>) {
					return;
				} else {
					return *(const Ty*) data_address();
				}
			} else if (st == id::error) {
				std::rethrow_exception(*(const std::exception_ptr*) data_address());
			} else {
				RC_ASSERT(st == id::cancelled);
				throw task_cancelled_exception{};
			}
		}
		bool promise_pending() const { return state == id::pending; }
		bool promise_cancelled() const { return state == id::cancelled; }
		bool promise_done() const { return state > id::cancelled; }
		bool promise_success() const { return state == id::success; }
		bool promise_error() const { return state == id::error; }
		bool promise_cancel() {
			cancellation_signal = true;
			return promise_wait() == id::cancelled;
		}
		void promise_add_finally(finally_clause* f) {
			std::unique_lock flock{fin_lock};

			// If already executed, do not insert anything into the list.
			//
			if (auto st = state.load(std::memory_order::relaxed); st > id::cancelled) [[unlikely]] {
				flock.unlock();
				f->on_finally(this, st == id::success);
				delete f;
				return;
			}

			// Link it into the list.
			//
			f->next	= fin_list;
			fin_list = f;
		}

		// Size calculation and coroutine allocation helper.
		//
		template<typename T>
		static constexpr size_t size_of() {
			size_t data_size = sizeof(std::exception_ptr);
			if constexpr (!std::is_void_v<T>) {
				data_size = std::max(data_size, sizeof(T));
			}
			return align_up(sizeof(task_state) + data_size, 0x10);
		}
		template<typename T>
		static void* alloc_coro(size_t coro_size) {
			constexpr size_t state_size = size_of<T>();

			auto ts	= make_overalloc_rc<task_state>(state_size - sizeof(task_state) + coro_size).release();
			auto hnd = coroutine_handle<>::from_address(((u8*) ts) + state_size);

			ts->coro_promise = hnd;
			ts->coro_current = hnd;
			return hnd.address();
		}
		template<typename T, typename Pr = void>
		static task_state* from_coro(coroutine_handle<Pr> hnd) {
			return (task_state*)(uptr(hnd.address()) - size_of<T>());
		}
		template<typename T, typename Pr>
		static task_state* from_promise(Pr& promise) {
			return from_coro<T>(coroutine_handle<Pr>::from_promise(promise));
		}
		template<typename T>
		static void delete_coro(void* coro_address) {
			auto ts	= from_coro<T>(coroutine_handle<>::from_address(coro_address));
			ts->coro_promise = nullptr;

			auto rc = rc_header::from(ts);
			heap::shrink(rc, size_of<T>() + sizeof(rc_header));
			rc->dec_ref();
		}

		// Invoke dtor on destruction.
		//
		~task_state() {
			RC_ASSERT(!fin_list);
			if (dtor)
				dtor(data_address());
		}
	};

	// Scheduler instance.
	//
	struct scheduler : pinned {
	  private:
		// Internals.
		//
		std::counting_semaphore<> signal{0};
		std::vector<std::thread>  thread_list			  = {};
		spinlock						  lock					  = {};
		std::atomic<u32>			  suspended				  = false;
		volatile bool				  termination_signal	  = false;
		list::head<task_state>	  queue					  = {};
		std::atomic<u64>			  remaining_task_count = 0;
		volatile u64				  affinity_mask		  = platform::g_affinity_mask;

		// Pushes a task for rescheduling.
		void push(task_state* w);
		// Pops a task promise for execution.
		task_state* pop();
		// Worker thread entry point.
		static void thread_main(scheduler* schd);

	  public:
		// Default scheduler.
		//
		static scheduler default_instance;

		// Constructs a scheduler with the given number of threads.
		//
		scheduler(size_t n = 0);

		// Sets scheduler affinity.
		//
		void update_affinity(u64 affinity = platform::g_affinity_mask) {
			affinity_mask = affinity;
		}

		// Schedules a task.
		//
		void insert(task_state* ts, priority prio = priority::normal);

		// Cancels all tasks.
		//
		void clear();

		// Waits until all tasks in the scheduler are completed.
		//
		void wait_until_empty() {
			while (size_t n = remaining_task_count.load()) {
				remaining_task_count.wait(n);
			}
		}

		// Suspends or resumes the scheduler.
		//
		void suspend() { suspended.store(1, std::memory_order::relaxed); }
		void resume() {
			if (suspended.exchange(0)) {
				suspended.notify_all();
			}
		}

		// Observers.
		//
		u64  num_remaining_tasks() const { return remaining_task_count.load(std::memory_order::relaxed); }
		bool is_suspended() const { return suspended.load(std::memory_order::relaxed) != 0; }

		// Handle cancellation of leftover tasks and thread deletion on destruction.
		//
		~scheduler();
	};

	// Simple async coroutine scheduler.
	//
	void defer(coroutine_handle<>);

	// Define subtask promises.
	//
	struct symmetric_transfer_awaitable {
		coroutine_handle<>						hnd;
		RC_INLINE inline bool					await_ready() const noexcept { return false; }
		RC_INLINE inline void					await_resume() const noexcept {}
		RC_INLINE inline coroutine_handle<> await_suspend(coroutine_handle<>) const noexcept { return hnd; }
	};
	struct subtask_promise_base {
		task_state*			 owner				 = nullptr;
		coroutine_handle<> continuation		 = nullptr;
		std::exception_ptr pending_exception = nullptr;

		task_state* get_task_state() const { return owner; }

		RC_INLINE inline suspend_always					 initial_suspend() noexcept { return {}; }
		RC_INLINE inline symmetric_transfer_awaitable final_suspend() noexcept { return {continuation}; }
		RC_INLINE inline void unhandled_exception() { pending_exception = std::current_exception(); }
	};
	template<typename R>
	struct subtask_promise : subtask_promise_base {
		std::optional<R> result = std::nullopt;
		subtask_promise* get_return_object() { return this; }
		template<typename T>
		void return_value(T&& value) {
			result.emplace(std::forward<T>(value));
		}
	};
	template<>
	struct subtask_promise<void> : subtask_promise_base {
		bool				  done = false;
		subtask_promise* get_return_object() { return this; }
		void				  return_void() { done = true; }
	};

	// Define the awaitable type for subtasks.
	//
	template<typename R>
	struct subtask_awaitable {
		subtask_promise<R>& pr;

		RC_INLINE inline bool await_ready() {
			if constexpr (!std::is_void_v<R>)
				return pr.result.has_value();
			else
				return pr.done;
		}

		RC_INLINE inline decltype(auto) await_resume() const {
			if (pr.pending_exception) [[unlikely]] {
				std::rethrow_exception(std::move(pr.pending_exception));
			}

			if constexpr (!std::is_void_v<R>)
				return std::move(pr.result).value();
		}

		template<typename T>
		RC_INLINE inline coroutine_handle<> await_suspend(coroutine_handle<T> hnd) {
			RC_ASSERT(pr.continuation == nullptr);
			pr.continuation = hnd;
			pr.owner			 = hnd.promise().get_task_state();
			return coroutine_handle<subtask_promise<R>>::from_promise(pr);
		}
	};

	// Subtask coroutine.
	//
	template<typename R = void>
	struct subtask {
		using promise_type = subtask_promise<R>;

		// Coroutine handle and the internal constructor.
		//
		unique_coroutine<promise_type> handle = nullptr;
		subtask(promise_type* pr) : handle(*pr) {}

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
		RC_INLINE inline subtask_awaitable<R> operator co_await() && { return {handle.promise()}; }
	};

	// Checkpoint for rescheduling.
	//
	struct checkpoint {
		RC_INLINE inline bool await_ready() { return false; }

		template<typename T>
		RC_INLINE inline coroutine_handle<> await_suspend(coroutine_handle<T> hnd) {
			auto*			chain = &hnd.promise();
			task_state* o		= chain->get_task_state();
			if (!o)
				return hnd;

			if (o->cancellation_signal) [[unlikely]] {
				o->task_cancel();
			}
			if (o->slice_end < now()) {
				o->coro_current = hnd;
				return noop_coroutine();
			}
			return hnd;
		}
		RC_INLINE inline void await_resume() {}
	};

	// Defines the task promises.
	//
	template<typename Ty>
	struct task_promise {
		void*			operator new(size_t n) { return task_state::alloc_coro<Ty>(n); }
		void			operator delete(void* p) { task_state::delete_coro<Ty>(p); }
		task_state* get_task_state() const { return task_state::from_promise<Ty>(*this); }

		task_promise<Ty>* get_return_object() { return this; }
		suspend_always initial_suspend() noexcept { return {}; }
		suspend_always final_suspend() noexcept { return {}; }

		RC_INLINE inline void unhandled_exception() { get_task_state()->task_raise(std::current_exception()); }

		template<typename T>
		void return_value(T&& val) {
			get_task_state()->task_emplace<Ty, T>(std::forward<T>(val));
		}
	};
	template<>
	struct task_promise<void> {
		void*			operator new(size_t n) { return task_state::alloc_coro<void>(n); }
		void			operator delete(void* p) { task_state::delete_coro<void>(p); }
		task_state* get_task_state() const { return task_state::from_promise<void>(*this); }

		task_promise<void>* get_return_object() { return this; }
		suspend_always		  initial_suspend() noexcept { return {}; }
		suspend_always		  final_suspend() noexcept { return {}; }

		RC_INLINE inline void unhandled_exception() { get_task_state()->task_raise(std::current_exception()); }
		void						 return_void() { get_task_state()->task_emplace_void(); }
	};

	// Implement coroutine wrapper and the finally clause.
	//
	template<typename Ty = void>
	struct promise;
	namespace detail {
		template<typename Ty, typename F>
		static finally_clause* make_then(F&& fn) {
			struct store final : finally_clause {
				std::decay_t<F> fn;
				store(F&& fn) : fn(std::forward<F>(fn)) {}
				void on_finally(task_state* t, bool ok) override {
					if (ok) {
						if constexpr (std::is_void_v<Ty>)
							fn();
						else
							fn(*(const Ty*)t->data_address());
					}
				}
			};
			return new store(std::forward<F>(fn));
		}
		template<typename Ty, typename F>
		static finally_clause* make_finally(F&& fn) {
			struct store final : finally_clause {
				std::decay_t<F> fn;
				store(F&& fn) : fn(std::forward<F>(fn)) {}
				void on_finally(task_state* t, bool) override {
					static_assert(sizeof(promise<Ty>) == sizeof(task_state*));
					fn((const promise<Ty>&) t); // Major hack.
				}
			};
			return new store(std::forward<F>(fn));
		}

		template<typename F, typename R, typename... A>
		static R coro_wrapper(F fn, A... args)
#ifdef __INTELLISENSE__
			 ;
#else
		{
			if constexpr (std::is_void_v<R>) {
				fn(std::move(args)...);
				co_return;
			} else {
				co_return fn(std::move(args)...);
			}
		}
#endif
	};

	// Promise type.
	//
	template<typename Ty>
	struct promise {
		ref<task_state> state = {};

		// Null constructor and validity check.
		//
		constexpr promise() = default;
		constexpr promise(std::nullptr_t) {}
		constexpr bool		 has_value() const { return state != nullptr; }
		constexpr explicit operator bool() const { return has_value(); }

		// By-value construction, default move/copy.
		//
		constexpr promise(task_state* r) : state(r) {}
		constexpr promise(const promise&)					 = default;
		constexpr promise(promise&&) noexcept				 = default;
		constexpr promise& operator=(const promise&)		 = default;
		constexpr promise& operator=(promise&&) noexcept = default;

		// Waits until task finishes executing.
		//
		void wait() const { return state->promise_wait(); }

		// Waits in a blocking fashion until the task is complete, returns the result.
		//
		decltype(auto) get() const { return state->template promise_get<Ty>(); }

		// Forward the state checks.
		//
		bool pending() const { return state->promise_pending(); }
		bool cancelled() const { return state->promise_cancelled(); }
		bool done() const { return state->promise_done(); }
		bool success() const { return state->promise_success(); }
		bool error() const { return state->promise_error(); }

		// Cancells the promise, returns false on failure.
		//
		bool cancel() const { return state->promise_cancel(); }

		// Adds a then callback in the form of void(Ty).
		//
		template<typename F>
			requires(std::is_void_v<Ty> ? Invocable<F, void> : Invocable<F, void, Ty>)
		void and_then(F&& fn) const {
			state->promise_add_finally(detail::make_then<Ty>(std::forward<F>(fn)));
		}

		// Adds a finally callback in the form of void(promise<Ty>).
		//
		template<typename F>
			requires Invocable<F, void, promise<Ty>>
		void and_finally(F&& fn) const {
			state->promise_add_finally(detail::make_finally<Ty>(std::forward<F>(fn)));
		}
	};

	// Task coroutine and the reference type to its state.
	//
	template<typename Ty = void>
	struct task {
		using promise_type = task_promise<Ty>;

		// Coroutine handle and the internal constructor.
		//
		unique_coroutine<promise_type> handle = nullptr;
		task(promise_type* pr) : handle(*pr) {}

		// Null constructor and validity check.
		//
		constexpr task() = default;
		constexpr task(std::nullptr_t) : handle() {}
		bool		has_value() const { return handle.get() != nullptr; }
		explicit operator bool() const { return has_value(); }

		// Queues the task and returns the associated promise.
		//
		[[nodiscard]] promise<Ty> queue(scheduler& sched = scheduler::default_instance, priority prio = priority::normal) {
			if (!handle) [[unlikely]] {
				return {};
			}
			auto* ts = handle.release().promise().get_task_state();
			promise<Ty> promise{ts};
			sched.insert(ts, prio);
			return promise;
		}

		// Queues a task and discards the promise.
		//
		void detach(scheduler& sched = scheduler::default_instance, priority prio = priority::normal) {
			if (!handle) [[unlikely]] {
				return;
			}
			sched.insert(handle.release().promise().get_task_state(), prio);
		}

		// No copy, default move.
		//
		task(task&&) noexcept				= default;
		task& operator=(task&&) noexcept = default;
	};

	// Creates an async task given a lambda.
	//
	template<typename F, typename... A>
	static promise<typename function_traits<F>::return_type> async(F&& fn, A&&... args) {
		return detail::coro_wrapper<task<typename function_traits<F>::return_type>>(std::forward<F>(fn), std::forward<A>(args)...).queue();
	}
	template<typename F, typename... A>
	static promise<typename function_traits<F>::return_type> async(scheduler& sched, F&& fn, A&&... args) {
		return detail::coro_wrapper<task<typename function_traits<F>::return_type>>(std::forward<F>(fn), std::forward<A>(args)...).queue(sched);
	}

	// Creates a deferred non-tracked task given a lambda.
	//
	template<typename F, typename... A>
	static void defer(F&& fn, A&&... args) {
		detail::coro_wrapper<task<void>>(std::forward<F>(fn), std::forward<A>(args)...).detach();
	}
	template<typename F, typename... A>
	static void defer(scheduler& sched, F&& fn, A&&... args) {
		detail::coro_wrapper<task<void>>(std::forward<F>(fn), std::forward<A>(args)...).detach(sched);
	}
};