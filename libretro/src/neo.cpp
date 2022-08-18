#include <retro/neo.hpp>
#include <retro/platform.hpp>

namespace retro::neo {
	static size_t ideal_thread_count				= std::thread::hardware_concurrency();
	scheduler	  scheduler::default_instance = {std::thread::hardware_concurrency()};

	// Pushes a task for rescheduling.
	//
	void scheduler::push(task_state* w) {
		std::lock_guard _g{lock};
		list::link_before(queue.entry(), w);
		signal.release();
	}

	// Pops a task promise for execution.
	//
	task_state* scheduler::pop() {
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
		u64 affinity = schd->affinity_mask;
		platform::set_affinity(affinity);

		while (true) {
			// Wait for a signal.
			//
			schd->signal.acquire();

			// If affinity changed, update.
			//
			if (u64 new_affinity = schd->affinity_mask; new_affinity != affinity) [[unlikely]] {
				platform::set_affinity(new_affinity);
				affinity = new_affinity;
			}

			// If termination is requested, return.
			//
			if (schd->termination_signal) [[unlikely]]
				return;

			// Wait until scheduler is resumed.
			//
			while(auto n = schd->suspended.load()) [[unlikely]] {
				schd->suspended.wait(n);
			}

			// Pop a task and execute it.
			//
			if (task_state* ts = schd->pop()) {
				// Resume the task if no cancellation signal is set.
				//
				if (!ts->cancellation_signal) {
					ts->slice_end = now() + u32(ts->prio) * time_slice_coeff;
					ts->coro_current.resume();

					if (ts->state == task_state::id::pending && !ts->coro_promise.done()) {
						schd->push(ts);
						continue;
					}
				}
				// Otherwise cancel.
				//
				else {
					ts->task_cancel();
				}

				// Decrement remaining task count, notify if it reaches zero.
				//
				if (!--schd->remaining_task_count)
					schd->remaining_task_count.notify_all();
				ts->coro_promise.destroy();
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
		while (auto* ts = queue.front()) {
			ts->task_cancel();
			list::unlink(ts);
			ts->coro_promise.destroy();
		}

		remaining_task_count = 0;
		remaining_task_count.notify_all();
	}

	// Schedules a task.
	//
	void scheduler::insert(task_state* ts, priority prio) {
		ts->prio = prio;
		remaining_task_count++;
		push(ts);
	}

	// Handle cancellation of leftover tasks and thread deletion on destruction.
	//
	scheduler::~scheduler() {
		termination_signal = true;
		signal.release(UINT32_MAX);
		resume();
		for (auto& t : thread_list)
			t.join();
		clear();
	}
};
