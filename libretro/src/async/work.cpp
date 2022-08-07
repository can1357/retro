#include <retro/common.hpp>
#include <retro/async/work.hpp>
#include <retro/list.hpp>
#include <retro/umutex.hpp>
#include <thread>
#include <semaphore>
#include <bit>

namespace retro {
	// Work list.
	//
	struct work_list {
		// Queue and the lock.
		//
		recursive_umutex			  mtx  = {};
		list::head<work>			  queue = {};
		std::counting_semaphore<> signal{0};

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
			std::lock_guard _g{mtx};

			bool empty = queue.empty();
			list::link_before(queue.entry(), w);

			if (empty) {
				signal.release();
			}
		}

		// Pops a work entry.
		//
		work* pop() {
			std::lock_guard _g{mtx};
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
			signal.release(UINT32_MAX);
			termination_signal.store(true);
			thread.join();
		}
	};

	// Global list of workers, will not be resized.
	//
	static work_list*			g_work_lists			= nullptr;
	static u32					g_work_list_mask		= 0;
	static std::atomic<u32> g_work_list_balancer = 0;

	RC_INITIALIZER {
		u32 n				  = std::bit_ceil(std::thread::hardware_concurrency());
		g_work_lists	  = new work_list[n];
		g_work_list_mask = n - 1;
	};

	// Queues the work.
	//
	void work::queue() {
		RC_ASSERT(list::is_detached(this));

		// Pick the list and queue the work.
		//
		auto& list = g_work_lists[++g_work_list_balancer & g_work_list_mask];
		list.push(this);
	}
};