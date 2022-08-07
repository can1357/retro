#pragma once
#include <functional>
#include <type_traits>
#include <utility>
#include <retro/common.hpp>
#include <retro/format.hpp>

// Clang and MS-STL doesnt work together, so we need to define our own std types in that case.
//
#define RC_HAS_STD_CORO ((RC_WINDOWS == RC_MSVC) && !RC_OSX)
#if RC_HAS_STD_CORO
	#include <coroutine>
#endif

// Define the core types.
//
#if !RC_HAS_STD_CORO
namespace std {
	template<typename Ret, typename... Args>
	struct coroutine_traits;
};
#endif

#define RC_UNHANDLED_RETHROW \
	void unhandled_exception() {}
//#define RC_UNHANDLED_RETHROW \
//	void unhandled_exception() { std::rethrow_exception(std::current_exception()); }

namespace retro {
#if !RC_HAS_STD_CORO
	namespace builtin {
	#ifdef __INTELLISENSE__
		void* noop();
	#else
		RC_INLINE inline void* noop() { return __builtin_coro_noop(); }
	#endif
		RC_INLINE inline bool done(void* address) { return __builtin_coro_done(address); }
		RC_INLINE inline void resume(void* address) { __builtin_coro_resume(address); }
		RC_INLINE inline void destroy(void* address) { __builtin_coro_destroy(address); }
		template<typename P>
		RC_INLINE inline void* to_handle(P& ref) {
			return __builtin_coro_promise(&ref, alignof(P), true);
		}
		template<typename P>
		RC_INLINE inline P& to_promise(void* hnd) {
			return *(P*) __builtin_coro_promise(hnd, alignof(P), false);
		}
	};
#endif
};

#if !RC_HAS_STD_CORO
namespace std {
	// Coroutine traits.
	//
	template<typename Ret, typename... Args>
	struct coroutine_traits;
	template<typename Ret, typename... Args>
		requires requires { typename Ret::promise_type; }
	struct coroutine_traits<Ret, Args...> {
		using promise_type = typename Ret::promise_type;
	};

	// Coroutine handle.
	//
	template<typename Promise = void>
	struct coroutine_handle;
	template<>
	struct coroutine_handle<void> {
		void* handle = nullptr;

		RC_INLINE constexpr coroutine_handle() noexcept : handle(nullptr){};
		RC_INLINE constexpr coroutine_handle(std::nullptr_t) noexcept : handle(nullptr){};

		RC_INLINE constexpr void*	  address() const noexcept { return handle; }
		RC_INLINE constexpr explicit operator bool() const noexcept { return handle; }

		RC_INLINE void resume() const { retro::builtin::resume(handle); }
		RC_INLINE void destroy() const noexcept { retro::builtin::destroy(handle); }
		RC_INLINE bool done() const noexcept { return retro::builtin::done(handle); }
		RC_INLINE void operator()() const { resume(); }

		RC_INLINE constexpr bool operator==(coroutine_handle<> other) const { return address() == other.address(); }
		RC_INLINE constexpr bool operator!=(coroutine_handle<> other) const { return address() != other.address(); }
		RC_INLINE constexpr bool operator<(coroutine_handle<> other) const { return address() < other.address(); }

		RC_INLINE static coroutine_handle<> from_address(void* addr) noexcept {
			coroutine_handle<> tmp{};
			tmp.handle = addr;
			return tmp;
		}
	};
	template<typename Promise>
	struct coroutine_handle : coroutine_handle<> {
		RC_INLINE constexpr coroutine_handle() noexcept {};
		RC_INLINE constexpr coroutine_handle(std::nullptr_t) noexcept : coroutine_handle<>(nullptr){};

		RC_INLINE Promise&								 promise() const { return retro::builtin::to_promise<Promise>(handle); }
		RC_INLINE static coroutine_handle<Promise> from_promise(Promise& pr) noexcept { return from_address(retro::builtin::to_handle(pr)); }
		RC_INLINE static coroutine_handle<Promise> from_address(void* addr) noexcept {
			coroutine_handle<Promise> tmp{};
			tmp.handle = addr;
			return tmp;
		}
	};

	template<typename Pr>
	struct hash<coroutine_handle<Pr>> {
		inline size_t operator()(const coroutine_handle<Pr>& hnd) const noexcept { return hash<size_t>{}((size_t) hnd.address()); }
	};

	// No-op coroutine.
	//
	struct noop_coroutine_promise {};
	template<>
	struct coroutine_handle<noop_coroutine_promise> {
		void* handle;
		inline coroutine_handle() noexcept : handle(retro::builtin::noop()) {}

		RC_INLINE constexpr			  operator coroutine_handle<>() const noexcept { return coroutine_handle<>::from_address(handle); }
		RC_INLINE constexpr void*	  address() const noexcept { return handle; }
		RC_INLINE constexpr explicit operator bool() const noexcept { return true; }

		RC_INLINE constexpr void resume() const {}
		RC_INLINE constexpr void destroy() const noexcept {}
		RC_INLINE constexpr bool done() const noexcept { return false; }
		RC_INLINE constexpr void operator()() const { resume(); }

		RC_INLINE constexpr bool operator==(coroutine_handle<> other) { return address() == other.address(); }
		RC_INLINE constexpr bool operator!=(coroutine_handle<> other) { return address() != other.address(); }
		RC_INLINE constexpr bool operator<(coroutine_handle<> other) { return address() < other.address(); }

		RC_INLINE bool operator==(coroutine_handle<noop_coroutine_promise>) { return true; }
		RC_INLINE bool operator!=(coroutine_handle<noop_coroutine_promise>) { return false; }
		RC_INLINE bool operator<(coroutine_handle<noop_coroutine_promise>) { return false; }

		RC_INLINE noop_coroutine_promise& promise() const noexcept { return retro::builtin::to_promise<noop_coroutine_promise>(handle); }
	};
	using noop_coroutine_handle = coroutine_handle<noop_coroutine_promise>;
	inline noop_coroutine_handle noop_coroutine() noexcept { return noop_coroutine_handle{}; }

	// Dummy awaitables.
	//
	struct suspend_never {
		RC_INLINE bool await_ready() const noexcept { return true; }
		RC_INLINE void await_suspend(coroutine_handle<>) const noexcept {}
		RC_INLINE void await_resume() const noexcept {}
	};
	struct suspend_always {
		RC_INLINE bool await_ready() const noexcept { return false; }
		RC_INLINE void await_suspend(coroutine_handle<>) const noexcept {}
		RC_INLINE void await_resume() const noexcept {}
	};
};
#endif

namespace retro {
	// Import the select types.
	//
	template<typename Promise = void>
	using coroutine_handle = std::coroutine_handle<Promise>;
	template<typename Ret, typename... Args>
	using coroutine_traits = std::coroutine_traits<Ret, Args...>;
	using suspend_always	  = std::suspend_always;
	using suspend_never	  = std::suspend_never;

	using noop_coroutine_promise = std::noop_coroutine_promise;
	using noop_coroutine_handle  = std::noop_coroutine_handle;
	inline noop_coroutine_handle noop_coroutine() noexcept { return std::noop_coroutine(); }

	// Coroutine traits.
	//
	template<typename T>
	concept Coroutine = requires { typename coroutine_traits<T>::promise_type; };
	template<typename T>
	concept Awaitable = requires(T x) {
								  x.await_ready();
								  x.await_suspend();
								  x.await_resume();
							  };

	// Suspend type that terminates the coroutine.
	//
	struct suspend_terminate {
		RC_INLINE bool await_ready() { return false; }
		RC_INLINE void await_suspend(coroutine_handle<> hnd) { hnd.destroy(); }
		RC_INLINE void await_resume() const { assume_unreachable(); }
	};

	// Unique coroutine that is destroyed on destruction by caller.
	//
	template<typename Pr>
	struct unique_coroutine {
		coroutine_handle<Pr> hnd{nullptr};

		// Null handle.
		//
		RC_INLINE inline constexpr unique_coroutine() {}
		RC_INLINE inline constexpr unique_coroutine(std::nullptr_t) {}

		// Constructed by handle.
		//
		RC_INLINE inline unique_coroutine(coroutine_handle<Pr> hnd) : hnd{hnd} {}

		// Constructed by promise.
		//
		RC_INLINE inline unique_coroutine(Pr& p) : hnd{coroutine_handle<Pr>::from_promise(p)} {}

		// No copy allowed.
		//
		inline unique_coroutine(const unique_coroutine&)				= delete;
		inline unique_coroutine& operator=(const unique_coroutine&) = delete;

		// Move by swap.
		//
		RC_INLINE inline unique_coroutine(unique_coroutine&& o) noexcept : hnd(o.release()) {}
		RC_INLINE inline unique_coroutine& operator=(unique_coroutine&& o) noexcept {
			std::swap(hnd, o.hnd);
			return *this;
		}

		// Redirections.
		//
		RC_INLINE inline void resume() const { hnd.resume(); }
		RC_INLINE inline bool done() const { return hnd.done(); }
		RC_INLINE inline Pr&	 promise() const { return hnd.promise(); }
		RC_INLINE inline void operator()() const { return resume(); }
		RC_INLINE inline		 operator coroutine_handle<Pr>() const { return hnd; }

		// Getter and validity check.
		//
		RC_INLINE inline coroutine_handle<Pr> get() const { return hnd; }
		RC_INLINE inline explicit				  operator bool() const { return (bool) hnd; }

		// Reset and release.
		//
		RC_INLINE inline coroutine_handle<Pr> release() { return std::exchange(hnd, nullptr); }
		RC_INLINE inline void					  reset() {
								 if (auto h = release())
				 h.destroy();
		}

		// Reset on destruction.
		//
		RC_INLINE ~unique_coroutine() { reset(); }
	};

	// Helper to get the promise from a coroutine handle.
	//
	template<typename Promise>
	RC_INLINE static Promise& get_promise(coroutine_handle<> hnd) {
		return coroutine_handle<Promise>::from_address(hnd.address()).promise();
	}

	// Helper to get the current coroutine handle / promise.
	//
#if RC_CLANG
	#define RC_THIS_CORO() coroutine_handle<>::from_address(__builtin_coro_frame())
#else
	namespace detail {
		struct coroutine_resolver {
			mutable coroutine_handle<> hnd;

			bool await_ready() const noexcept { return false; }
			bool await_suspend(coroutine_handle<> h) const noexcept {
				hnd = h;
				return false;
			}
			coroutine_handle<> await_resume() const noexcept { return hnd; }
		};
	};
	#define RC_THIS_CORO() co_await retro::detail::coroutine_resolver{}
#endif
	#define RC_THIS_PROMISE(...) retro::get_promise<__VA_ARGS__>(RC_THIS_CORO())

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
};

// Clang requires coroutine_traits under std::experimental.
//
#if RC_CLANG && __clang_major__ < 14
namespace std::experimental {
	template<typename Promise = void>
	struct coroutine_handle : retro::coroutine_handle<Promise> {
		using retro::coroutine_handle<Promise>::coroutine_handle;
		using retro::coroutine_handle<Promise>::operator=;
	};

	template<typename Ret, typename... Args>
		requires requires { typename retro::coroutine_traits<Ret, Args...>::promise_type; }
	struct coroutine_traits {
		using promise_type = typename retro::coroutine_traits<Ret, Args...>::promise_type;
	};
};
#endif
