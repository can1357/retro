#pragma once
#include <retro/common.hpp>
#include <retro/platform.hpp>
#include <atomic>
#include <mutex>
#include <shared_mutex>

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
		RC_INLINE static bool yield(u8& counter) {
			if (counter++ >= slow_yield_threshold) [[unlikely]] {
				return false;
			}
			intrin::yield();
			return true;
		}
	};

	// Simple lock.
	//
	struct simple_lock {
		std::atomic<u16> flag  = 0;

		RC_INLINE bool locked() const { return !(flag.load(std::memory_order::relaxed) & 1); }
		RC_INLINE bool try_lock() { return flag.fetch_or(1, std::memory_order::acquire) == 0; }
		RC_INLINE void lock() {
			u8 yield_counter = 0;
			while (!try_lock()) [[unlikely]] {
				do {
					if (!detail::yield(yield_counter)) {
						flag.wait(1);
					}
				} while (locked());
			}
		}
		RC_INLINE void unlock() {
			flag.store(0, std::memory_order::release);
			flag.notify_all();
		}
	};

	// Recursive lock.
	//
	struct rec_lock {
		std::atomic<size_t> owner  = 0;
		u16                 depth  = 0;

		RC_INLINE bool locked() const { return owner.load(std::memory_order::relaxed) != 0; }
		RC_INLINE bool try_lock(size_t tid = platform::thread_id()) {
			size_t expected = 0;
			if (owner.compare_exchange_strong(expected, tid, std::memory_order::acquire) || expected == tid) {
				++depth;
				return true;
			}
			return false;
		}
		RC_INLINE void lock() {
			size_t tid				= platform::thread_id();
			u8		 yield_counter = 0;
			while (true) {
				size_t expected = 0;
				if (owner.compare_exchange_strong(expected, tid, std::memory_order::acquire) || expected == tid) [[likely]] {
					++depth;
					break;
				}

				if (!detail::yield(yield_counter)) {
					owner.wait(expected);
				}
			}
		}
		RC_INLINE void unlock() {
			if (!--depth) {
				owner.store(0, std::memory_order::release);
				owner.notify_all();
			}
		}
	};

	// R/W lock.
	//
	struct rw_lock {
		std::atomic<u16> counter = 0;

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
			u8 yield_counter = 0;
			while (true) {
				u16 expected = 0;
				if (counter.compare_exchange_strong(expected, UINT16_MAX, std::memory_order::acquire)) [[likely]] {
					break;
				}
				if (!detail::yield(yield_counter)) {
					counter.wait(expected);
				}
			}
		}
		RC_INLINE void lock_shared() {
			u8 yield_counter = 0;
			while (true) {
				u16 expected;
				while ((expected = counter.load(std::memory_order::relaxed)) >= (UINT16_MAX - 1)) {
					if (!detail::yield(yield_counter)) {
						if (expected == UINT16_MAX)
							counter.wait(expected);
					}
				}
				if (counter.compare_exchange_strong(expected, expected + 1, std::memory_order::acquire))
					break;
			}
		}
		RC_INLINE void unlock() {
			counter.store(0, std::memory_order::release);
			counter.notify_all();
		}
		RC_INLINE void unlock_shared() {
			if (!--counter)
				counter.notify_all();
		}
	};
};