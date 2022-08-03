#include <retro/common.hpp>
#include <retro/task.hpp>
#include <retro/list.hpp>
#include <retro/lock.hpp>
#include <thread>
#include <semaphore>
#include <bit>

namespace retro {
	// Work list.
	//
	struct work_list {
		// Queue and the lock.
		//
		rec_lock					 lock	 = {};
		list::head<work>		 queue = {};
		std::binary_semaphore signal{0};

		// Thread running it.
		//
		std::thread thread;

		// Signals.
		//
		std::atomic<bool> termination_signal = false;

		// Worker function.
		//
		static void worker_function(work_list* self) {
			while (true) {
				// Pop a work, if non-null, execute and continue.
				//
				auto* w = self->pop();
				if (w) {
					w->run();
					continue;
				}

				// Break out if termination signal is set.
				//
				if (self->termination_signal) {
					break;
				}

				// Wait for the queue signal.
				//
				self->signal.acquire();
				continue;
			}
		}

		// Creates the thread.
		//
		work_list() : thread(&worker_function, this) {}

		// Queues a work.
		//
		void push(work* w) {
			std::lock_guard _g{lock};

			bool empty = queue.empty();
			list::link_before(queue.entry(), w);

			if (empty) {
				signal.release();
			}
		}

		// Pops a work entry.
		//
		work* pop() {
			std::lock_guard _g{lock};
			while (auto* w = queue.front()) {
				list::unlink(w);
				return w;
			}
			return nullptr;
		}

		// Destructor deletes all work.
		//
		~work_list() {
			// Set the termination signal and wait for the thread.
			//
			signal.release();
			termination_signal.store(true);
			thread.join();
		}
	};

	// Global list of workers, will not be resized.
	//
	static std::vector<work_list> work_lists(std::bit_ceil(std::thread::hardware_concurrency()));
	static std::atomic<u32>			work_balancer = 0;

	// Queues the work.
	//
	void work::queue() {
		RC_ASSERT(list::is_detached(this));

		// Pick the list and queue the work.
		//
		auto& list = work_lists[++work_balancer & (work_lists.size() - 1)];
		list.push(this);
	}
};