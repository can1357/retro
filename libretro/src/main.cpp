#include <retro/platform.hpp>
#include <retro/format.hpp>

using namespace retro;

/*
#include <coroutine>
#include <retro/coro.hpp>
namespace retro {
	template<typename T>
	struct task {
		struct promise_type {
			// Symetric transfer.
			//
			coroutine_handle<>				  continuation				= nullptr;
			std::vector<coroutine_handle<>> extended_continuation = {};

			// Result.
			//
			std::optional<T>	 result		  = std::nullopt;

			// Make sure continuation was not leaked on destruction.
			//
			~promise_type() {
				RC_ASSERT(!continuation);	 // Should not leak continuation!
			}

			struct final_awaitable {
				inline bool await_ready() noexcept { return false; }

				template<typename P = promise_type>
				inline coroutine_handle<> await_suspend(coroutine_handle<P> handle) noexcept {
					auto& pr = handle.promise();
					for (auto& ex : pr.extended_continuation) {
						ex();
					}
					if (auto c = pr.continuation)
						return c;
					else
						return noop_coroutine();
				}
				inline void await_resume() const noexcept {}
			};

			task<T>			 get_return_object() { return *this; }
			suspend_always	 initial_suspend() noexcept { return {}; }
			final_awaitable final_suspend() noexcept { return {}; }
			RC_UNHANDLED_RETHROW;

			template<typename V>
			void return_value(V&& v) {
				result.emplace(std::forward<V>(v));
			}
		};

		// Coroutine handle and the internal constructor.
		//
		mutable bool						 started = false;
		unique_coroutine<promise_type> handle	= nullptr;
		task(promise_type& pr) : handle(pr) {}

		// Null constructor and validity check.
		//
		constexpr task() = default;
		constexpr task(std::nullptr_t) : task() {}
		constexpr bool has_value() const { return handle != nullptr; }
		constexpr explicit operator bool() const { return has_value(); }

		// No copy, default move.
		//
		constexpr task(task&&) noexcept				 = default;
		constexpr task(const task&)					 = delete;
		constexpr task& operator=(task&&) noexcept = default;
		constexpr task& operator=(const task&)		 = delete;

		// State.
		//
		bool done() const { return handle.promise().result.has_value(); }
		constexpr bool pending() const { return started && !done(); }
		constexpr bool lazy() const { return !started; }

		// Resumes the coroutine and returns the status.
		//
		bool operator()() const {
			if (!started) {
				started = true;
				handle.resume();
			}
			return done();
		}
	};

	struct barrier {
		std::vector<coroutine_handle<>> waiters;
		void signal() {
			for (auto& w : std::exchange(waiters, {}))
				w();
		}

		inline bool await_ready() { return false; }
		inline void await_resume() const {}
		inline void await_suspend(coroutine_handle<> hnd) { waiters.emplace_back(hnd); }
	};

	template<typename T>
	inline auto operator co_await(const task<T>& ref) {
		struct awaitable {
			const task<T>& ref;

			inline bool					  await_ready() { return ref.done(); }
			inline const auto&		  await_resume() const { return *ref.handle.promise().result; }
			inline coroutine_handle<> await_suspend(coroutine_handle<> hnd) {
				auto& pr = ref.handle.promise();
				if (pr.continuation) {
					pr.extended_continuation.emplace_back(pr.continuation);
				}
				pr.continuation = hnd;

				if (ref.started)
					return noop_coroutine();
				ref.started = true;
				return ref.handle.hnd;
			}
		};
		return awaitable{ref};
	}
};

barrier meme_Barrier;

task<int> say_hi() {
	co_await meme_Barrier;
	co_return printf("hi\n");
}
task<std::monostate> stuff(const char* name, task<int>& task) {
	printf("%s - x\n", name);
	int x = co_await task;
	printf("%s - z: %d\n", name, x);
	co_return std::monostate{};
}
auto hi_task = say_hi();
auto t0		 = stuff("t0", hi_task);
auto t1		 = stuff("t1", hi_task);
printf("t0: %d\n", t0());
printf("t1: %d\n", t1());
meme_Barrier.signal();
printf("t0: %d\n", t0());
printf("t1: %d\n", t1());
*/

#include <retro/targets.hxx>


#include <retro/disasm/zydis.hpp>

/*
test rcx, rcx
jz   x
  lea rax, [rdx+rcx]
  ret
x:
  lea rax, [rdx+r8]
  ret
*/
constexpr const char lift_example[] = "\x48\x85\xC9\x74\x05\x48\x8D\x04\x0A\xC3\x4A\x8D\x04\x02\xC3";


#include <retro/ctti.hpp>
#include <retro/dyn.hpp>
#include <retro/list.hpp>

#include <retro/ir/ops.hxx>
#include <retro/ir/types.hpp>
#include <retro/ir/use.hpp>

using namespace ir;

struct operand {
	use use_record = {};

	// Since use is aligned by 8, use.prev will also be aligned by 8.
	// - If misaligned (eg check bit 0 == 1), we can use it as a thombstone.
	//
};

struct instruction : dyn<instruction>, usable {
	operand operands[2] = {};

	// Given a use from this instruction, gets the operand index.
	//
	size_t index_of(const use* use) const { return ((operand*) use) - &operands[0]; }

	// Changes an operands value.
	//
	void set_operand(size_t idx, instruction* nv) {
		auto& op = operands[idx];
		if (nv) {
			op.use_record.reset(nv, this);
		} else {
			op.use_record.reset();
		}
	}

	// Test printer.
	//
	void print(std::string_view name) {
		fmt::println(name, ":", this, " (", operands[0].use_record.value, ", ", operands[1].use_record.value, ")");
		for (auto* use : uses()) {
			fmt::println(" - Used by: ", use->user(), " @ Operand #", use->user<instruction>()->index_of(use));
		}
	}
};


// safer linked listss
//


int main(int argv, const char** args) {
	platform::setup_ansi_escapes();

	instruction a;
	instruction b;

	a.set_operand(0, &b);
	a.set_operand(1, &b);
	a.set_operand(1, &b);
	a.set_operand(0, nullptr);

	b.replace_all_uses_with(&a);

	a.replace_all_uses_with((instruction*)nullptr);

	a.print("a");
	b.print("b");

	/*std::span<const u8> data = {(u8*) lift_example, sizeof(lift_example) - 1};

	while (auto i = zydis::decode(data)) {
		for (auto& op : i->operands()) {
			if (op.type == ZYDIS_OPERAND_TYPE_REGISTER) {
				fmt::println("->", reg(op.reg.value));
			}
		}
		fmt::println(i->to_string());
	}*/ 
}