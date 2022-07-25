#pragma once
#include <retro/common.hpp>
#include <retro/platform.hpp>
#include <atomic>
#include <mutex>

// Defines the fast locks that behaves like a mutex only on long pauses.
//
namespace retro {
	// No-op lock.
	//
	struct noop_lock {
		RC_INLINE bool try_lock(size_t = 0) { return true; }
		RC_INLINE bool try_lock_shared(size_t = 0) { return true; }
		RC_INLINE void lock() {}
		RC_INLINE void lock_shared() {}
		RC_INLINE void unlock() {}
		RC_INLINE void unlock_shared() {}
		RC_INLINE bool locked() const { return false; }
	};
	namespace detail {
		static constexpr u8 slow_yield_threshold = 128;

		struct signal {
			std::atomic<u16> value = 0;
			RC_COLD void wait() {
				value.store(1, std::memory_order::relaxed);
				value.wait(1);
			}
			RC_COLD void emit() {
				value.notify_all();
				value.store(0, std::memory_order::relaxed);
			}
			RC_INLINE void clear() {
				if (value.load(std::memory_order::relaxed)) [[unlikely]] {
					emit();
				}
			}
		};
		RC_INLINE static void smart_yield(u8& counter, signal& ss) {
			if (counter++ >= slow_yield_threshold) [[unlikely]] {
				ss.wait();
			}
			arch::yield();
		}
	};

	// Basic lock.
	//
	struct basic_lock {
		std::atomic<u16> flag  = 0;
		detail::signal   signal = {};

		RC_INLINE bool locked() const { return !(flag.load(std::memory_order::relaxed) & 1); }
		RC_INLINE bool try_lock() { return flag.fetch_or(1, std::memory_order::acquire) == 0; }
		RC_INLINE void lock() {
			while (!try_lock()) [[unlikely]] {
				u8 yield_counter = 0;
				do {
					detail::smart_yield(yield_counter, signal);
				} while (locked());
			}
		}
		RC_INLINE void unlock() { flag.store(0, std::memory_order::release); }
	};

	// Recursive lock.
	//
	struct rec_lock {
		std::atomic<size_t> owner  = 0;
		detail::signal      signal = {};
		u16                 depth  = 0;

		RC_INLINE bool locked() const { return owner.load(std::memory_order::relaxed) != 0; }
		RC_INLINE bool try_lock(size_t tid = platform::thread_id()) {
			size_t expected = 0;
			if (owner.compare_exchange_strong(expected, tid) || expected == tid) {
				++depth;
				return true;
			}
			return false;
		}
		RC_INLINE void lock() {
			size_t tid = platform::thread_id();
			while (!try_lock(tid)) [[unlikely]] {
				u8 yield_counter = 0;
				do {
					detail::smart_yield(yield_counter, signal);
				} while (locked());
			}
		}
		RC_INLINE void unlock() {
			if (!--depth) {
				owner.store(0, std::memory_order::release);
				signal.clear();
			}
		}
	};

	// R/W lock.
	//
	struct rw_lock {
		std::atomic<u16> counter = 0;
		detail::signal   signal  = {};

		RC_INLINE bool locked() const { return counter.load(std::memory_order::relaxed) != 0; }
		RC_INLINE bool try_lock() {
			u16 expected = 0;
			return counter.compare_exchange_strong(expected, UINT16_MAX, std::memory_order::acquire);
		}
		RC_INLINE bool try_lock_shared() {
			u16 value = counter.load(std::memory_order::relaxed);
			while (value < (UINT16_MAX - 1)) {
				if (counter.compare_exchange_strong(value, value + 1, std::memory_order::acquire))
					return true;
			}
			return false;
		}
		RC_INLINE void lock() {
			while (!try_lock()) [[unlikely]] {
				u8 yield_counter = 0;
				do {
					detail::smart_yield(yield_counter, signal);
				} while (locked());
			}
		}
		RC_INLINE void lock_shared() {
			u8 yield_counter = 0;
			while (true) {
				// Yield the CPU until the exclusive lock is gone.
				//
				u16 value;
				while ((value = counter.load(std::memory_order::relaxed)) >= (UINT16_MAX - 1)) [[unlikely]] {
					detail::smart_yield(yield_counter, signal);
				}

				// Try incrementing share count.
				//
				if (counter.compare_exchange_strong(value, value + 1, std::memory_order::acquire))
					return;
			}
		}
		RC_INLINE void unlock() {
			counter.store(0, std::memory_order::release);
			signal.clear();
		}
		RC_INLINE void unlock_shared() {
			if (!--counter)
				signal.clear();
		}
	};
};