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

#include <retro/common.hpp>
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




// safer linked listss
//


#include <vector>
#include <memory>
#include <retro/graph/search.hpp>

#include <retro/ir/ops.hxx>

#include <retro/dyn.hpp>
#include <retro/ir/types.hpp>
#include <retro/ir/value.hpp>
#include <memory>
#include <retro/rc.hpp>
#include <retro/ir/insn.hpp>

namespace retro::doc {
	// Image type.
	//
	struct image {
		// TODO: ??
		//
	};

	// Workspace type.
	//
	struct workspace {
		// TODO: ??
		//
	};
};

namespace retro::ir {
	// Basic block type.
	//
	struct procedure;
	namespace detail {
		static u32 next_name(const procedure*);
	};

	struct basic_block final : dyn<basic_block, value> {
		// Owning procedure.
		//
		procedure* proc = nullptr;

		// Name and Label IP.
		//
		u32 name = 0;
		u64 ip	= NO_LABEL;

		// Instruction list.
		//
		mutable insn insn_list_head{0}; // TODO: Lame

		// Temporary for algorithms.
		//
		mutable u64 tmp_monotonic = 0;
		mutable u64 tmp_mapping	  = 0;

		// Successor and predecesor list.
		//
		std::vector<weak<basic_block>> successors	= {};
		std::vector<weak<basic_block>> predecessors = {};

		// Container observers.
		//
		list::iterator<insn> begin() const { return {insn_list_head.next}; }
		list::iterator<insn> end() const { return {&insn_list_head}; }
		auto						rbegin() const { return std::reverse_iterator(end()); }
		auto						rend() const { return std::reverse_iterator(begin()); }
		bool						empty() const { return insn_list_head.next == &insn_list_head; }
		insn*						front() const { return (!empty()) ? (insn_list_head.next) : nullptr; }
		insn*						back() const { return (!empty()) ? (insn_list_head.prev) : nullptr; }
		list::iterator<insn> end_phi() const {
			return range::find_if(begin(), end(), [](insn* i) { return i->op != opcode::phi; });
		}
		auto phis() const { return range::subrange(begin(), end_phi()); }
		auto insns() const { return range::subrange(begin(), end()); }
		auto after(insn* i) const { return range::subrange(i ? list::iterator<insn>(i->next) : begin(), end()); }
		auto before(insn* i) const { return range::subrange(begin(), i ? list::iterator<insn>(i) : end()); }

		// Insertion.
		//
		list::iterator<insn> insert(list::iterator<insn> position, ref<insn> v) {
			RC_ASSERT(v->is_orphan());
			v->block = this;
			v->name	= detail::next_name(proc);
			list::link_before(position.at, v.get());
			return {v.release()};
		}
		list::iterator<insn> push_front(ref<insn> v) { return insert(begin(), std::move(v)); }
		list::iterator<insn> push_back(ref<insn> v) { return insert(end(), std::move(v)); }

		// Erasing.
		//
		list::iterator<insn> erase(list::iterator<insn> it) {
			auto next = it->next;
			it->erase();
			return next;
		}
		list::iterator<insn> erase(ref<insn> it) {
			auto r = erase(list::iterator<insn>(it.get()));
			it.reset();
			return r;
		}
		template<typename F>
		size_t erase_if(F&& f) {
			size_t n = 0;
			for (auto it = begin(); it != end();) {
				if (f(it.at)) {
					n++;
					it = erase(it);
				} else {
					++it;
				}
			}
			return n;
		}

		// Adds or removes a jump from this basic-block to another.
		//
		void add_jump(basic_block* to) {
			auto sit = range::find(successors, to);
			auto pit = range::find(to->predecessors, this);
			successors.emplace_back(to);
			to->predecessors.emplace_back(this);
		}
		void del_jump(basic_block* to) {
			auto sit = range::find(successors, to);
			auto pit = range::find(to->predecessors, this);
			// TODO:
			/*if (fix_phi) {
				size_t n = pit - to->predecessors.begin();
				for (auto phi : to->phis())
					phi->operands.erase(phi->operands.begin() + n);
			}*/
			successors.erase(sit);
			to->predecessors.erase(pit);
		}

		// TODO:
		// split
		// clone()

		

		// Validation.
		//
		diag::lazy validate() const {
			for (auto&& ins : insns()) {
				if (auto err = ins->validate()) {
					return err;
				}
			}
			return diag::ok;
		}

		// Declare string conversion and type getter.
		//
		std::string to_string(fmt_style s = {}) const override {
			if (s == fmt_style::concise) {
				return fmt::str(RC_CYAN "$%x" RC_RESET, name);
			} else {
				std::string result = fmt::str(RC_CYAN "$%x:" RC_RESET, name);
				for (insn* i : *this) {
					result += "\n\t" + i->to_string();
				}
				result += RC_RESET;
				return result;
			}
		}
		type get_type() const override { return type::label; }
	};

	// Procedure type.
	//
	struct procedure : range::view_base {
		using container		= std::vector<ref<basic_block>>;
		using iterator			= typename container::iterator;
		using const_iterator = typename container::const_iterator;

		// Entry point ip if relevant.
		//
		u64 start_ip = NO_LABEL;

		// Counters.
		//
		mutable u32 next_ins_name	 = 0;	 // Next instruction name.
		mutable u32 next_blk_name = 0;  // Next basic-block name.

		// List of basic-blocks.
		//
		container basic_blocks = {};

		// Container observers.
		//
		iterator			begin() { return basic_blocks.begin(); }
		iterator			end() { return basic_blocks.end(); }
		const_iterator begin() const { return basic_blocks.begin(); }
		const_iterator end() const { return basic_blocks.end(); }
		size_t			size() const { return basic_blocks.size(); }
		bool				empty() const { return basic_blocks.empty(); }

		// Gets the entry point.
		//
		basic_block* get_entry() const { return basic_blocks.empty() ? nullptr : basic_blocks.front().get(); }

		// Creates or removes a block.
		//
		basic_block* add_block() {
			auto* blk = basic_blocks.emplace_back(make_rc<basic_block>()).get();
			blk->proc = this;
			blk->name = next_blk_name++;
			return blk;
		}
		auto del_block(basic_block* b) {
			RC_ASSERT(b->predecessors.empty());
			RC_ASSERT(b->successors.empty());
			for (auto it = basic_blocks.begin();; ++it) {
				RC_ASSERT(it != basic_blocks.end());
				if (it->get() == b) {
					return basic_blocks.erase(it);
				}
			}
			RC_UNREACHABLE();
		}

		// Topologically sorts the basic block list.
		//
		void topological_sort() {
			u32 tmp = narrow_cast<u32>(basic_blocks.size());

			basic_block* ep = get_entry();
			graph::dfs(ep, [&](basic_block* b) { b->name = --tmp; });
			RC_ASSERT(ep->name == 0);

			range::sort(basic_blocks, [](auto& a, auto& b) { return a->name < b->name; });
		}

		// Simple renaming by order.
		//
		void rename_insns() {
			next_ins_name = 0;
			for (auto& bb : basic_blocks) {
				for (auto i : *bb) {
					i->name = next_ins_name++;
				}
			}
		}
		void rename_blocks() {
			next_blk_name = 0;
			for (auto& bb : basic_blocks) {
				bb->name = next_blk_name++;
			}
		}

		// Validation.
		//
		diag::lazy validate() const {
			for (auto&& blk : *this) {
				if (auto err = blk->validate()) {
					return err;
				}
			}
			return diag::ok;
		}

		// String conversion.
		//
		std::string to_string(fmt_style s = {}) const {
			std::string result = fmt::str(RC_ORANGE "proc-%llx" RC_RESET, this);
			if (start_ip != NO_LABEL) {
				result += fmt::str("[%p]", start_ip);
			}
			if (s == fmt_style::concise) {
				return result;
			} else {
				for (auto&& blk : *this) {
					result += "\n" + fmt::shift(blk->to_string());
				}
				result += RC_RESET;
				return result;
			}
		}

		// TODO:
		// clone()
		// validate()
	};

	namespace detail {
		static u32 next_name(const procedure* p) { return p->next_ins_name++; }
	};
};



int main(int argv, const char** args) {
	platform::setup_ansi_escapes();

	auto	proc = make_rc<ir::procedure>();
	auto* bb	  = proc->add_block();

	auto i0 = bb->push_back(ir::make_binop(ir::op::add, 2, 3));
	auto i1 = bb->push_back(ir::make_binop(ir::op::add, i0, 3));

	fmt::println(proc->to_string());
	i0->replace_all_uses_with(5);

	fmt::println(proc->to_string());


	/*


	std::string_view tmp_types[] = {"T", "Ty"};

	for (auto& ins : ir::insn_desc::list()) {
		if (ins.id() == ir::insn::none)
			continue;

		std::string_view ret_type = "!";
		if (!ins.terminator) {
			if (ins.templates.begin()[0] == 0) {
				ret_type = enum_name(ins.types[0]);
			} else {
				ret_type = tmp_types[ins.templates[0] - 1];
			}
		}

		fmt::print(RC_RED, ret_type, " " RC_YELLOW, ins.name);
		if (ins.template_count == 1) {
			fmt::print(RC_BLUE "<T>");
		} else if (ins.template_count == 2) {
			fmt::print(RC_BLUE "<T,Ty>");
		}
		fmt::print(" " RC_RESET);


		size_t num_args = ins.types.size() - 1;
		for (size_t i = 0; i != num_args; i++) {
			auto tmp	  = ins.templates[i + 1];
			auto type  = ins.types[i + 1];
			auto name  = ins.names[i + 1];

			std::string_view ty;
			if (tmp == 0) {
				ty = enum_name(type);
			} else {
				ty = tmp_types[tmp-1];
			}

			if (range::count(ins.constexprs, i + 1)) {
				fmt::print(RC_GREEN "constexpr ");
			}
			fmt::print(RC_RED, ty, RC_RESET ":" RC_WHITE, name, RC_RESET " ");
		}
		fmt::print("\n");
	}*/


	//auto procedure = ir::procedure::make();
	//
	//ir::constant kek = "abcddsads";
	//ir::constant kek2 = std::move(kek);
	//kek					= f32x4{1.0f, 2.0f, 3.0f, 4.0f};
	//
	//fmt::println("kek holds: ", kek.get_type(), "=", kek);
	//fmt::println("kek2 holds: ", kek2.get_type(), "=", kek2);

	/*instruction a;
	instruction b;

	a.set_operand(0, &b);
	a.set_operand(1, &b);
	a.set_operand(1, &b);
	a.set_operand(0, nullptr);

	b.replace_all_uses_with(&a);

	a.replace_all_uses_with((instruction*)nullptr);

	a.print("a");
	b.print("b");*/

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