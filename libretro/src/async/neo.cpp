#include <retro/async/neo.hpp>

namespace retro::neo {
	inline static size_t ideal_thread_count = std::thread::hardware_concurrency();

	// Pushes a task for rescheduling.
	//
	void scheduler::push(task_promise* w) {
		std::lock_guard _g{lock};
		list::link_before(queue.entry(), w);
		signal.release();
	}

	// Pops a task promise for execution.
	//
	task_promise* scheduler::pop() {
		std::lock_guard _g{lock};
		if (auto* w = queue.front()) {
			list::unlink(w);
			return w;
		}
		return nullptr;
	}

	// Worker thread entry point.
	//
	void scheduler::thread_main(scheduler* schd) {
		while (true) {
			schd->signal.acquire();
			if (schd->termination_signal)
				return;
			if (task_promise* t = schd->pop()) {
				auto	hnd = coroutine_handle<task_promise>::from_promise(*t);
				auto* ts	 = t->get_state();

				if (!ts->cancellation_signal) {
					t->slice_end = now() + u32(t->prio) * time_slice_coeff;

					t->suspended_continuation.resume();
					if (!hnd.done()) {
						schd->push(t);
						continue;
					}
					ts->state = task_state::id::complete;
				} else {
					ts->state = task_state::id::cancelled;
				}

				--schd->remaining_task_count;
				schd->remaining_task_count.notify_all();
				ts->completion_semaphore.release(INT32_MAX);
				hnd.destroy();
			}
		}
	}

	// Constructs a scheduler with the given number of threads.
	//
	scheduler::scheduler(size_t n) {
		if (n == 0)
			n = ideal_thread_count;
		while (n) {
			thread_list.emplace_back(thread_main, this);
			--n;
		}
	}
	
	// Cancels all tasks.
	//
	void scheduler::clear() {
		std::lock_guard _g{lock};
		while (auto* w = queue.front()) {
			list::unlink(w);
			--remaining_task_count;
			remaining_task_count.notify_all();
			auto* ts = w->get_state();
			ts->state = task_state::id::cancelled;
			ts->completion_semaphore.release(INT32_MAX);
			coroutine_handle<task_promise>::from_promise(*w).destroy();
		}
	}

	// Schedules a task.
	//
	task_ref scheduler::insert(task&& t, priority prio) {
		auto w = &t.handle.release().promise();
		list::init(w);
		w->prio = prio;
		remaining_task_count++;

		task_ref ts{w->get_state()};
		push(w);
		return ts;
	}

	// Handle cancellation of leftover tasks and thread deletion on destruction.
	//
	scheduler::~scheduler() {
		termination_signal = true;
		signal.release(UINT32_MAX);
		for (auto& t : thread_list)
			t.join();
		clear();
	}
};
