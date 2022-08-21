#include <retro/common.hpp>
#include <retro/core/image.hpp>
#include <retro/core/workspace.hpp>
#include <retro/ir/basic_block.hpp>
#include <retro/ir/routine.hpp>
#include <retro/ir/insn.hpp>
#include <retro/ir/z3x.hpp>
#include <retro/llvm/clang.hpp>
#include <retro/bind/js.hpp>

#include <retro/core/callbacks.hpp>
#include <retro/graph/naive.hpp>
#include <Zydis/Zydis.h>
#undef assert

namespace retro::bind {
	// Scheduler.
	//
	template<>
	struct type_descriptor<neo::scheduler> : user_class<neo::scheduler> {
		inline static constexpr const char* name = "Scheduler";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_static_method("create", []() { return std::make_unique<neo::scheduler>(); });
			proto.add_static_method("getDefault", []() { return &neo::scheduler::default_instance; });

			proto.add_method("clear", [](neo::scheduler* sc) { sc->clear(); });
			proto.add_method("suspend", [](neo::scheduler* sc) { sc->suspend(); });
			proto.add_method("resume", [](neo::scheduler* sc) { sc->resume(); });
			proto.add_async_method("wait", [](neo::scheduler* sc) { sc->wait_until_empty(); });

			proto.add_property("suspended", [](neo::scheduler* sc) { return sc->is_suspended(); });
			proto.add_property("remainingTasks", [](neo::scheduler* sc) { return (u32) sc->num_remaining_tasks(); });
		}
	};

	// Task<T>.
	//
	struct task_wrapper {
		ref<neo::task_state> ref;
		coroutine_handle<> handle;
		void (*queue_fn)(task_wrapper* self, void* out, const void* eng, neo::scheduler* s);
		~task_wrapper() {
			if (handle)
				handle.destroy();
		}
	};
	template<typename Engine, typename T>
	struct converter<Engine, neo::task<T>> {
		using promise = neo::task_promise<T>;
		using value	  = typename Engine::value_type;

		value from(const Engine& context, neo::task<T> task) const {
			auto wrapper		= std::make_unique_for_overwrite<task_wrapper>();
			wrapper->handle	= coroutine_handle<>::from_address(task.handle.release().address());
			wrapper->queue_fn = +[](task_wrapper* self, void* out, const void* _eng, neo::scheduler* s) {
				auto			 handle = std::exchange(self->handle, nullptr);
				neo::task<T> ts{&coroutine_handle<promise>::from_address(handle.address()).promise()};
				self->ref = ts.handle.promise().get_task_state();
				*(value*) out = value::make(*(const Engine*) _eng, ts.queue(*s));
			};
			return value::make(context, std::move(wrapper));
		}
	};
	template<>
	struct type_descriptor<task_wrapper> : user_class<task_wrapper> {
		inline static constexpr const char* name = "Task";

		template<typename Proto>
		static void write(Proto& proto) {
			using engine = typename Proto::engine_type;
			using value =  typename engine::value_type;

			proto.add_property("queued", [](task_wrapper* ts) {
				return ts->ref != nullptr;
			});
			proto.add_property("pending", [](task_wrapper* ts) {
				return ts->ref && ts->ref->promise_pending();
			});
			proto.add_property("cancelled", [](task_wrapper* ts) {
				return ts->ref && ts->ref->promise_cancelled();
			});
			proto.add_property("done", [](task_wrapper* ts) {
				return ts->ref && ts->ref->promise_done();
			});
			proto.add_property("success", [](task_wrapper* ts) {
				return ts->ref && ts->ref->promise_success();
			});
			proto.add_property("error", [](task_wrapper* ts) {
				return ts->ref && ts->ref->promise_error();
			});
			proto.add_method("cancel", [](task_wrapper* ts) {
				if (ts->ref == nullptr) {
					throw std::runtime_error{"Task is not queued!"};
				}
				return ts->ref->promise_cancel();
			});
			proto.add_method("queue", [](const engine& eng, task_wrapper* ts, std::optional<neo::scheduler*> sc) {
				if (ts->ref != nullptr) {
					throw std::runtime_error{"Task is already queued!"};
				}
				value out;
				ts->queue_fn(ts, &out, &eng, sc.value_or(&neo::scheduler::default_instance));
				return out;
			});
		}
	};

	// Machine instructions.
	//
	template<>
	struct type_descriptor<arch::imm> : user_class<arch::imm> {
		inline static constexpr const char* name = "MImm";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_property("width", [](arch::imm* i) { return i->width; });
			proto.add_property("isSigned", [](arch::imm* i) { return i->is_signed; });
			proto.add_property("isRelative", [](arch::imm* i) { return i->is_relative; });
			proto.add_property("s", [](arch::imm* i) { return i->s; });
			proto.add_property("u", [](arch::imm* i) { return i->u; });

			proto.add_method("getSigned", [](arch::imm* i, std::optional<u64> ip) { return i->get_signed(ip.value_or(0)); });
			proto.add_method("getUnsigned", [](arch::imm* i, std::optional<u64> ip) { return i->get_unsigned(ip.value_or(0)); });
			
			proto.add_method("toString", [](arch::imm* m, std::optional<arch::handle>, std::optional<u64> ip) { return m->to_string(ip.value_or(0)); });
		}
	};
	template<>
	struct type_descriptor<arch::mreg> : user_class<arch::mreg> {
		inline static constexpr const char* name = "MReg";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_static_method("from", [](u32 id) { return bitcast<arch::mreg>(id); });
			proto.add_property("id", [](arch::mreg* m) { return m->id; });
			proto.add_property("kind", [](arch::mreg* m) { return m->get_kind(); });
			proto.add_property("uid", [](arch::mreg* m) { return m->uid(); });
			proto.add_method("equals", [](arch::mreg* m, arch::mreg* o) { return *m == *o; });
			proto.add_property("comperator", [](arch::mreg* m) { return m->uid(); });

			proto.add_method("getName", [](arch::mreg* m, std::optional<arch::handle> h) { return m->name(h.value_or(std::nullopt).get()); });
			proto.add_method("toString", [](arch::mreg* m, std::optional<arch::handle> h, std::optional<u64>) { return m->to_string(h.value_or(std::nullopt).get()); });
		}
	};
	template<>
	struct type_descriptor<arch::mem> : user_class<arch::mem> {
		inline static constexpr const char* name = "MMem";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_property("width", [](arch::mem* m) { return m->width; });
			proto.add_property("segVal", [](arch::mem* m) { return m->segv; });
			proto.add_property("scale", [](arch::mem* m) { return m->scale; });
			proto.add_property("disp", [](arch::mem* m) { return m->disp; });
			proto.add_property("seg", [](arch::mem* m) -> std::optional<arch::mreg> {
				if (m->segr)
					return m->segr;
				return std::nullopt;
			});
			proto.add_property("index", [](arch::mem* m) -> std::optional<arch::mreg> {
				if (m->index)
					return m->index;
				return std::nullopt;
			});
			proto.add_property("base", [](arch::mem* m) -> std::optional<arch::mreg> {
				if (m->base)
					return m->base;
				return std::nullopt;
			});
			proto.add_method("toString", [](arch::mem* m, std::optional<arch::handle> h, std::optional<u64> ip) { return m->to_string(h.value_or(std::nullopt).get(), ip.value_or(0)); });
		}
	};
	template<>
	struct type_descriptor<arch::minsn> : user_class<arch::minsn> {
		inline static constexpr const char* name = "MInsn";

		template<typename Proto>
		static void write(Proto& proto) {
			using engine = typename Proto::engine_type;

			proto.add_property("name", [](arch::minsn* i) { return i->name(); });
			proto.add_property("arch", [](arch::minsn* i) { return arch::handle(i->arch); });
			proto.add_property("mnemonic", [](arch::minsn* i) { return i->mnemonic; });
			proto.add_property("modifiers", [](arch::minsn* i) { return i->modifiers; });
			proto.add_property("effectiveWidth", [](arch::minsn* i) { return i->effective_width; });
			proto.add_property("length", [](arch::minsn* i) { return i->length; });
			proto.add_property("operandCount", [](arch::minsn* i) { return i->operand_count; });
			proto.add_property("isSupervisor", [](arch::minsn* i) { return (bool)i->is_supervisor; });
			proto.add_method("getOperand", [](const engine& e, arch::minsn* ins, u32 i) {
				using value = typename engine::value_type;
				if (ins->operand_count > i) {
					if (ins->op[i].type == arch::mop_type::reg)
						return value::make(e, ins->op[i].r);
					else if (ins->op[i].type == arch::mop_type::mem)
						return value::make(e, ins->op[i].m);
					else if (ins->op[i].type == arch::mop_type::imm)
						return value::make(e, ins->op[i].i);
				}
				return value::make(e, std::nullopt);
			});
			proto.add_method("toString", [](arch::minsn* i, std::optional<u64> ip) {
				return i->to_string(ip.value_or(0));
			});
		}
	};

	// IR types.
	//
#define _FOR_EACH_CONST_TYPE(_) \
	_(I1, bool)	_(I8, i8) _(I16, i16) _(I32, i32) _(I64, i64) _(F32, f32) _(F64, f64) _(Str, std::string_view) _(MReg, arch::mreg) _(Op, ir::op) _(Intrinsic, ir::intrinsic) _(Ptr, ir::pointer)\
	_(F32x16, f32x16) _(F32x2, f32x2) _(I16x32, i16x32) _(I16x4, i16x4) \
	_(F32x4, f32x4) _(F32x8, f32x8) _(F64x2, f64x2) _(F64x4, f64x4) _(F64x8, f64x8) _(I16x16, i16x16)\
	_(I16x8, i16x8) _(I32x16, i32x16) _(I32x2, i32x2) _(I32x4, i32x4) _(I32x8, i32x8) _(I64x2, i64x2)\
	_(I64x4, i64x4) _(I64x8, i64x8) _(I8x16, i8x16) _(I8x32, i8x32) _(I8x64, i8x64) _(I8x8, i8x8) 
	// TODO: iu128, f80

	template<>
	struct type_descriptor<ir::constant> : user_class<ir::constant> {
		inline static constexpr const char* name = "Const";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_property("type", [](ir::constant* r) { return r->get_type(); });
			proto.add_property("byteLength", [](ir::constant* r) { return r->size(); });
			proto.add_property("buffer", [](ir::constant* r) { return std::span{(const u8*) r->address(), r->size()}; });
			proto.add_property("valid", [](ir::constant* r) { return r->get_type() != ir::type::none; });
			proto.add_method("equals", [](ir::constant* c, ir::constant* o) { return *c == *o; });
			proto.add_method("toString", [](ir::constant* c) { return c->to_string(); });
			proto.add_method("toExpr", [](ir::constant* c, std::optional<u8> lane) {
				if (lane && *lane) {
					return z3x::make_value(z3x::get_context(), *c, *lane);
				} else {
					return z3x::make_value(z3x::get_context(), *c);
				}
			});

			proto.add_method("bitcast", [](ir::constant* c, ir::type t) { return c->bitcast(t); });
			proto.add_method("castZx", [](ir::constant* c, ir::type t) { return c->cast_sx(t); });
			proto.add_method("castSx", [](ir::constant* c, ir::type t) { return c->cast_zx(t); });
			proto.add_method("apply", [](ir::constant* c, ir::op o, std::optional<ir::constant*> rhs) { return c->apply(o, *rhs.value_or(c)); });
			proto.add_property("i64", [](ir::constant* r) { return r->get_i64(); });
			proto.add_property("u64", [](ir::constant* r) { return r->get_u64(); });
			
			#define _MAKE_GETSET(name, Ty)                                                       \
	proto.add_method("as" RC_STRINGIFY(name), [](ir::constant* r) { return r->get<Ty>(); }); \
	proto.add_static_method(RC_STRINGIFY(name), [](Ty x) { return ir::constant(x); });
			_FOR_EACH_CONST_TYPE(_MAKE_GETSET)
#undef _MAKE_GETSET
		}
	};
	template<>
	struct type_descriptor<ir::operand> : user_class<ir::operand> {
		inline static constexpr const char* name = "Operand";

		template<typename Proto>
		static void write(Proto& proto) {
			using engine = typename Proto::engine_type;
			using value	 = typename engine::value_type;

			proto.add_property("type", [](ir::operand* r) { return r->get_type(); });
			proto.add_method("toString", [](ir::operand* r, std::optional<bool> full) { return r->to_string(full.value_or(false) ? ir::fmt_style::full : ir::fmt_style::concise); });
			proto.add_method("equals", [](ir::operand* c, ir::operand* o) { return *c == *o; });
			proto.add_property("isConst", [](ir::operand* r) { return r->is_const(); });
			proto.add_property("isValue", [](ir::operand* r) { return r->is_value(); });
			proto.add_property("user", [](ir::operand* r) { return r->user; });
			proto.add_property("value", [](ir::operand* r) { return r->get_value(); });
			proto.add_property("constant", [](ir::operand* r) { return r->get_const(); });

			proto.add_method("toExpr", [](ir::operand* r, z3x::variable_set* vs, std::optional<u32> max_depth) {
				return z3x::to_expr(*vs, z3x::get_context(), *r, max_depth.value_or(0xFFFFFFFF));
			});

			proto.add_method("set", [](ir::operand* r, const value& v) {
				if (v.template isinstance<ir::value>()) {
					r->reset(v.template as<ir::value*>());
				} else if (v.template isinstance<ir::constant>()) {
					r->reset(v.template as<ir::constant>());
				} else {
					throw std::runtime_error("Expected value or constant.");
				}
			});
		}
	};
	template<>
	struct type_descriptor<ir::value> : user_class<ir::value> {
		inline static constexpr const char* name = "Value";

		template<typename Proto>
		static void write(Proto& proto) {
			using engine = typename Proto::engine_type;
			using value =  typename engine::value_type;

			proto.add_property("type", [](ir::value* r) { return r->get_type(); });
			proto.add_property("uses", [](ir::value* r) { return r->uses(); });
			proto.add_method("toString", [](ir::value* r, std::optional<bool> full) { return r->to_string(full.value_or(false) ? ir::fmt_style::full : ir::fmt_style::concise); });

			proto.add_method("toExpr", [](ir::value* r, z3x::variable_set* vs, std::optional<u32> max_depth) {
				return z3x::to_expr(*vs, z3x::get_context(), r, max_depth.value_or(0xFFFFFFFF));
			});

			proto.add_method("replaceAllUsesWith", [](ir::value* r, const value& v) {
				if (v.template isinstance<ir::value>()) {
					return (u32) r->replace_all_uses_with(v.template as<ir::value*>());
				} else if (v.template isinstance<ir::constant>()) {
					return (u32) r->replace_all_uses_with(v.template as<ir::constant>());
				} else {
					throw std::runtime_error("Expected value or constant.");
				}
			});
		}
	};
	template<>
	struct type_descriptor<ir::insn> : user_class<ir::insn>, force_rc_t {
		inline static constexpr const char* name = "Insn";

		template<typename Proto>
		static void write(Proto& proto) {
			using engine = typename Proto::engine_type;
			using array	 = typename engine::array_type;

			proto.template set_super<ir::value>();
			proto.add_property("image", [](ir::insn* r) { return r->get_image(); });
			proto.add_property("workspace", [](ir::insn* r) { return r->get_workspace(); });
			proto.add_property("routine", [](ir::insn* r) { return r->get_routine(); });
			//			proto.add_property("method", [](ir::insn* r) { return r->get_method(); });
			proto.add_property("block", [](ir::insn* r) { return r->bb; });

			proto.add_method("validate", [](ir::insn* r) { r->validate().raise(); });
			proto.add_property("isOprhan", [](ir::insn* r) { return r->is_orphan(); });

			proto.template add_field_rw<&ir::insn::arch>("arch");
			proto.template add_field_rw<&ir::insn::ip>("ip");
			proto.template add_field_rw<&ir::insn::name>("name");
			proto.template add_field_rw<&ir::insn::op>("opcode");
			proto.template add_field_rw<&ir::insn::template_types>("templates");

			proto.add_property("operandCount", [](ir::insn* i) { return i->operand_count; });
			proto.add_method("opr", [](ir::insn* i, u32 id) {
				if (id >= i->operand_count)
					throw std::runtime_error("Operand out of boundaries.");
				return &i->opr(id);
			});
			proto.add_method("indexOf", [](ir::insn* i, ir::operand* op) { return (u32)i->index_of(op); });

			proto.add_method("erase", [](ir::insn* i) {
				if (i->is_orphan())
					throw std::runtime_error("Removing already orphan instruction.");
				i->erase();
			});
			proto.add_method("pushFront", [](ir::insn* i, ir::basic_block* at) {
				if (!i->is_orphan())
					throw std::runtime_error("Inserting non-orphan instruction.");
				at->push_front(i);
				return i;
			});
			proto.add_method("push", [](ir::insn* i, ir::basic_block* at) {
				if (!i->is_orphan())
					throw std::runtime_error("Inserting non-orphan instruction.");
				at->push_back(i);
				return i;
			});
			proto.add_method("insert", [](ir::insn* i, ir::insn* at) {
				if (!i->is_orphan())
					throw std::runtime_error("Inserting non-orphan instruction.");
				if (at->is_orphan())
					throw std::runtime_error("Inserting before orphan instruction.");
				at->bb->insert(at, i);
				return i;
			});
			proto.add_method("insertAfter", [](ir::insn* i, ir::insn* at) {
				if (!i->is_orphan())
					throw std::runtime_error("Inserting non-orphan instruction.");
				if (at->is_orphan())
					throw std::runtime_error("Inserting before orphan instruction.");
				at->bb->insert_after(at, i);
				return i;
			});

			proto.add_property("prev", [](ir::insn* i) {
				auto n = i->prev;
				if (n == i || (i->bb && n == i->bb->entry()))
					n = nullptr;
				return n;
			});
			proto.add_property("next", [](ir::insn* i) {
				auto n = i->next;
				if (n == i || (i->bb && n == i->bb->entry()))
					n = nullptr;
				return n;
			});

			proto.add_static_method("create", [](ir::opcode op, std::array<ir::type, 2> tmp, const array& operands) {
				size_t op_count = operands.length();
				auto	 ins		 = ir::insn::allocate(op, tmp, op_count);
				for (size_t i = 0; i != op_count; i++) {
					auto& r = ins->opr(i);
					auto	v = operands.get(i);

					if (v.template isinstance<ir::value>()) {
						r.reset(v.template as<ir::value*>());
					} else if (v.template isinstance<ir::constant>()) {
						r.reset(v.template as<ir::constant>());
					} else {
						throw std::runtime_error("Expected value or constant.");
					}
				}
				ins->validate();
				return ins;
			});
		}
	};
	template<>
	struct type_descriptor<ir::basic_block> : user_class<ir::basic_block>, force_rc_t {
		inline static constexpr const char* name		= "BasicBlock";

		template<typename Proto>
		static void write(Proto& proto) {

			proto.template set_super<ir::value>();
			proto.add_property("image", [](ir::basic_block* r) { return r->get_image(); });
			proto.add_property("workspace", [](ir::basic_block* r) { return r->get_workspace(); });
			proto.add_property("routine", [](ir::basic_block* r) { return r->rtn; });
			//			proto.add_property("method", [](ir::basic_block* r) { return r->get_method(); });

			proto.add_method("validate", [](ir::basic_block* r) { r->validate().raise(); });
			proto.add_property("successors", [](ir::basic_block* r) { return r->successors; });
			proto.add_property("predecessors", [](ir::basic_block* r) { return r->predecessors; });

			proto.add_property("isExit", [](ir::basic_block* r) {
				return r->successors.empty() && (r == r->rtn->entry_point || !r->predecessors.empty());
			});
			proto.add_method("dom", [](ir::basic_block* a, ir::basic_block* b) { return graph::naive::dom(a, b); });
			proto.add_method("postdom", [](ir::basic_block* a, ir::basic_block* b) { return graph::naive::postdom(a, b); });
			proto.add_method("hasPathTo", [](ir::basic_block* a, ir::basic_block* b) { return graph::naive::has_path_to(a, b); });


			proto.template add_field_rw<&ir::basic_block::arch>("arch");
			proto.template add_field_rw<&ir::basic_block::ip>("ip");
			proto.template add_field_rw<&ir::basic_block::end_ip>("endIp");
			proto.template add_field_rw<&ir::basic_block::name>("name");

			proto.add_method("pushFront", [](ir::basic_block* b, ir::insn* v) {
				if (!v->is_orphan())
					throw std::runtime_error("Inserting non-orphan instruction.");
				b->push_front(v);
				return v;
			});
			proto.add_method("push", [](ir::basic_block* b, ir::insn* v) {
				if (!v->is_orphan())
					throw std::runtime_error("Inserting non-orphan instruction.");
				b->push_back(v);
				return v;
			});

			proto.add_method("addJump", [](ir::basic_block* s, ir::basic_block* d) { s->add_jump(d); });
			proto.add_method("delJump", [](ir::basic_block* s, ir::basic_block* d) { s->del_jump(d); });

			// TODO: Split

			proto.add_property("terminator", [](ir::basic_block* r) { return r->terminator(); });
			proto.add_property("phis", [](ir::basic_block* r) { return r->phis(); });
			proto.make_iterable([](ir::basic_block * r) { return r->insns(); });
		}
	};
	template<>
	struct type_descriptor<ir::routine> : user_class<ir::routine>, force_rc_t {
		inline static constexpr const char* name		= "Routine";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_method("toString", [](ir::routine* r, std::optional<bool> full) { return r->to_string(full.value_or(false) ? ir::fmt_style::full : ir::fmt_style::concise); });
			proto.add_property("image", [](ir::routine* r) { return r->get_image(); });
			proto.add_property("workspace", [](ir::routine* r) { return r->get_workspace(); });
			//proto.add_property("method", [](ir::routine* r) { return r->method.get(); });
			proto.add_method("validate", [](ir::routine* r) { r->validate().raise(); });
			proto.add_method("renameBlocks", [](ir::routine* r) { r->rename_blocks(); });
			proto.add_method("renameInsns", [](ir::routine* r) { r->rename_insns(); });
			proto.add_method("topologicalSort", [](ir::routine* r) { r->topological_sort(); });

			// TODO: method

			proto.template add_field_rw<&ir::routine::ip>("ip");
			proto.add_property("entryPoint", [](ir::routine* r) { return r->entry_point.get(); });

			proto.make_iterable([](ir::routine * r) -> auto& { return r->blocks; });

			proto.add_method("addBlock", [](ir::routine* r) { return weak<ir::basic_block>(r->add_block()); });
			proto.add_method("delBlock", [](ir::routine* r, ir::basic_block* b) {
				if (b->rtn != r)
					throw std::runtime_error("Block does not belong to this routine.");
				r->del_block(b);
			});

			// TODO: For demo.
			proto.add_method("getXrefs", [](ir::routine* r, core::image* img) {
				auto img_base	= img->base_address;
				auto img_limit = img_base + img->raw_data.size();

				flat_uset<u64> va_set;
				for (auto& bb : r->blocks) {
					for (auto&& ins : bb->insns()) {
						if (ins->op == ir::opcode::xjmp || ins->op == ir::opcode::xjs)
							continue;
						for (auto& op : ins->operands()) {
							if (op.is_const()) {
								if (op.get_type() == ir::type::pointer || op.get_type() == ir::type::i32 || op.get_type() == ir::type::i64) {
									auto va = op.get_const().get_u64();
									if (img_base <= va && va < img_limit)
										va_set.emplace(va);
								}
							}
						}
					}
				}
				return std::vector<u64>{va_set.begin(), va_set.end()};
			});

			proto.add_static_method("create", []() { return make_rc<ir::routine>(); });
		}
	};
	template<typename Engine>
	struct converter<Engine, ir::variant> {
		using value	  = typename Engine::value_type;
		value from(const Engine& context, ir::variant&& v) const {
			if (v.is_const()) {
				return value::make(context, std::move(v).get_const());
			} else if (v.is_value()) {
				return value::make(context, std::move(v).get_value());
			} else {
				return value::make(context, std::nullopt);
			}
		}
	};

	// Z3 types.
	//
	template<>
	struct type_descriptor<z3x::expr> : user_class<z3x::expr> {
		inline static constexpr const char* name = "Z3Expr";

		static z3x::expr assert_apply(ir::op o, const z3x::expr& lhs, const z3x::expr& rhs) {
			auto res = z3x::apply(o, lhs, rhs);
			if (!z3x::ok(res)) {
				throw std::runtime_error("Invalid expression.");
			}
			return res;
		}
		static z3x::expr assert_apply(ir::op o, const z3x::expr& rhs) {
			auto res = z3x::apply(o, rhs);
			if (!z3x::ok(res)) {
				throw std::runtime_error("Invalid expression.");
			}
			return res;
		}
		static z3x::sort assert_sortof(ir::type ty, std::optional<ir::insn*> i) {
			auto s = z3x::make_sort(z3x::get_context(), ty, i.value_or(nullptr));
			if (!z3x::ok(s)) {
				throw std::runtime_error("Invalid expression type.");
			}
			return s;
		}

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_static_method("I1", [](bool v) { return z3x::get_context().bool_val(v); });
			proto.add_static_method("I8", [](i8 v) { return z3x::get_context().bv_val(v, 8); });
			proto.add_static_method("I16", [](i16 v) { return z3x::get_context().bv_val(v, 16); });
			proto.add_static_method("I32", [](i32 v) { return z3x::get_context().bv_val(v, 32); });
			proto.add_static_method("I64", [](i64 v) { return z3x::get_context().bv_val(v, 64); });
			proto.add_static_method("F32", [](float v) { return z3x::get_context().fpa_val(v); });
			proto.add_static_method("F64", [](double v) { return z3x::get_context().fpa_val(v); });

			
			proto.add_property("type", [](z3x::expr* expr) { return z3x::type_of(*expr); });
			proto.add_property("depth", [](z3x::expr* expr) { return z3x::expr_depth(*expr); });
			proto.add_property("isConst", [](z3x::expr* expr) { return z3x::is_value(*expr); });
			proto.add_property("isVariable", [](z3x::expr* expr) { return z3x::is_variable(*expr); });
			proto.add_method("simplify", [](z3x::expr* expr) { return expr->simplify(); });
			proto.add_method("materialize", [](z3x::expr* expr, z3x::variable_set* vs, ir::basic_block* bb) { return z3x::from_expr(*vs, *expr, bb); });
			proto.add_method("materializeAt", [](z3x::expr* expr, z3x::variable_set* vs, ir::insn* i) {
				list::iterator<ir::insn> it = {i};
				return z3x::from_expr(*vs, *expr, i->bb, it);
			});

			proto.add_method("abs", [](z3x::expr* expr) { return assert_apply(ir::op::abs, *expr); });
			proto.add_method("neg", [](z3x::expr* expr) { return assert_apply(ir::op::neg, *expr); });
			proto.add_method("sqrt", [](z3x::expr* expr) { return assert_apply(ir::op::sqrt, *expr); });
			proto.add_method("bitNot", [](z3x::expr* expr) { return assert_apply(ir::op::bit_not, *expr); });
			proto.add_method("rem", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::rem, *lhs, *rhs); });
			proto.add_method("urem", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::urem, *lhs, *rhs); });
			proto.add_method("div", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::div, *lhs, *rhs); });
			proto.add_method("udiv", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::udiv, *lhs, *rhs); });
			proto.add_method("xor", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::bit_xor, *lhs, *rhs); });
			proto.add_method("or", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::bit_or, *lhs, *rhs); });
			proto.add_method("and", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::bit_and, *lhs, *rhs); });
			proto.add_method("shl", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::bit_shl, *lhs, *rhs); });
			proto.add_method("shr", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::bit_shr, *lhs, *rhs); });
			proto.add_method("sar", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::bit_sar, *lhs, *rhs); });
			proto.add_method("rol", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::bit_rol, *lhs, *rhs); });
			proto.add_method("ror", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::bit_ror, *lhs, *rhs); });
			proto.add_method("add", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::add, *lhs, *rhs); });
			proto.add_method("sub", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::sub, *lhs, *rhs); });
			proto.add_method("mul", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::mul, *lhs, *rhs); });
			proto.add_method("ge", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::ge, *lhs, *rhs); });
			proto.add_method("uge", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::uge, *lhs, *rhs); });
			proto.add_method("gt", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::gt, *lhs, *rhs); });
			proto.add_method("ugt", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::ugt, *lhs, *rhs); });
			proto.add_method("lt", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::lt, *lhs, *rhs); });
			proto.add_method("ult", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::ult, *lhs, *rhs); });
			proto.add_method("le", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::le, *lhs, *rhs); });
			proto.add_method("ule", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::ule, *lhs, *rhs); });
			proto.add_method("umax", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::umax, *lhs, *rhs); });
			proto.add_method("max", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::max, *lhs, *rhs); });
			proto.add_method("umin", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::umin, *lhs, *rhs); });
			proto.add_method("min", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::min, *lhs, *rhs); });
			proto.add_method("eq", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::eq, *lhs, *rhs); });
			proto.add_method("ne", [](z3x::expr* lhs, z3x::expr* rhs) { return assert_apply(ir::op::ne, *lhs, *rhs); });
			proto.add_method("apply", [](z3x::expr* lhs, ir::op o, std::optional<z3x::expr*> rhs) {
				if (rhs) {
					return assert_apply(o, *lhs, **rhs);
				} else {
					return assert_apply(o, *lhs);
				}
			});
			proto.add_method("toString", [](z3x::expr* expr) { return expr->to_string(); });
			proto.add_method("toConst", [](z3x::expr* expr, std::optional<bool> coerce) { return z3x::value_of(*expr, coerce.value_or(false)); });
			proto.add_method("castZx", [](z3x::expr* expr, ir::type ty, std::optional<ir::insn*> i) { return z3x::cast_sx(*expr, assert_sortof(ty, i)); });
			proto.add_method("castSx", [](z3x::expr* expr, ir::type ty, std::optional<ir::insn*> i) { return z3x::cast(*expr, assert_sortof(ty, i)); });

			proto.add_method("equals", [](z3x::expr* a, z3x::expr* b) { return z3::eq(*a, *b); });
			proto.add_property("comperator", [](z3x::expr* e) {
				return make_comperator((Z3_ast)*e);
			});
		}
	};
	template<>
	struct type_descriptor<z3x::variable_set> : user_class<z3x::variable_set> {
		inline static constexpr const char* name = "Z3VariableSet";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_static_method("create", []() {
				return std::make_unique<z3x::variable_set>();
			});
			proto.add_method("unwrap", [](z3x::variable_set* vs, z3x::expr* expr) {
				return vs->get(*expr).lock();
			});
		}
	};

	// Arch interface.
	//
	template<>
	struct type_descriptor<arch::instance> : user_class<arch::instance> {
		inline static constexpr const char* name = "Arch";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_property("ptrType", [](arch::handle i) {
				return i->ptr_type();
			});
			proto.add_property("effectivePtrWidth", [](arch::handle i) {
				return i->get_effective_pointer_width();
			});
			proto.add_property("ptrWidth", [](arch::handle i) {
				return i->get_pointer_width();
			});
			proto.add_property("isLittleEndian", [](arch::handle i) {
				return i->get_byte_order() == std::endian::little;
			});
			proto.add_property("isBigEndian", [](arch::handle i) {
				return i->get_byte_order() == std::endian::big;
			});
			proto.add_property("stackRegister", [](arch::handle i) {
				return i->get_stack_register();
			});

			proto.add_method("formatInsnModifiers", [](arch::handle i, arch::minsn* m) {
				return i->format_minsn_modifiers(*m);
			});
			proto.add_method("nameMnemonic", [](arch::handle i, u32 m) {
				return i->name_mnemonic(m);
			});
			proto.add_method("nameRegister", [](arch::handle i, arch::mreg* r) {
				return i->name_register(*r);
			});
			proto.add_method("lift", [](arch::handle i, ir::basic_block* bb, const arch::minsn& m, u64 ip) {
				i->lift(bb, m, ip).raise();
			});
			proto.add_method("disasm", [](arch::handle i, std::span<const u8> data) {
				auto m = std::make_unique<arch::minsn>();
				if (!i->disasm(data, m.get())) {
					m.reset();
				}
				return m;
			});

			/*
				TODO:
				virtual const call_conv_desc* get_cc_desc(call_conv cc) = 0;
				virtual mreg_info get_register_info(mreg r) { return {r}; }
				virtual void		for_each_subreg(mreg r, function_view<void(mreg)> f) {}
				virtual ir::insn* explode_write_reg(ir::insn* i) { return i; }
			*/
		}
	};

	template<>
	struct type_descriptor<core::image> : user_class<core::image> {
		inline static constexpr const char* name = "Image";
		
		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_property("name", [](core::image* i) { return i->name; });
			proto.add_property("kind", [](core::image* i) { return i->kind; });
			proto.add_property("baseAddress", [](core::image* i) { return i->base_address; });
			proto.add_property("ldr", [](core::image* i) { return i->ldr; });
			proto.add_property("arch", [](core::image* i) { return i->arch; });
			proto.add_property("abiName", [](core::image* i) { return i->abi_name; });
			proto.add_property("envName", [](core::image* i) { return i->env_name; });
			proto.add_property("isEnvSupervisor", [](core::image* i) { return i->env_supervisor; });
			proto.add_property("entryPoints", [](core::image* i) { return i->entry_points; });
			
			proto.add_method("slice", [](core::image* i, u64 rva, u64 len) {
				auto result = i->slice(rva);
				if (result.size() >= len)
					result = result.subspan(0, len);
				return result;
			});
			proto.add_method("lift", [] (const js::engine& eng, core::image* img, u64 rva) {
				return core::lift(img, rva);
			});


			// TODO:
			// std::vector<section> sections = {};
			// std::vector<reloc>	relocs	= {};
			// std::vector<symbol>	symbols	= {};
			//TODO: proto.add_property("defaultCc", [](core::image* i) { return i->default_cc; });
		}
	};

	template<>
	struct type_descriptor<ldr::instance> : user_class<ldr::instance> {
		inline static constexpr const char* name = "Loader";

		
		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_method("match", [](ldr::handle l, std::vector<u8> data) { return l->match(data); });
			proto.add_property("extensions", [](ldr::handle l) {
				auto e = l->get_extensions();
				return std::vector<std::string_view>{e.begin(), e.end()};
			});
		}
	};

	template<>
	struct type_descriptor<core::workspace> : user_class<core::workspace> {
		inline static constexpr const char* name = "Workspace";

		template<typename Proto>
		static void write(Proto& proto) {
			proto.add_static_method("create", []() {
				return core::workspace::create();
			});
			proto.add_property("numImages", [](core::workspace* ws) {
				std::shared_lock _g{ws->image_list_mtx};
				return (u32)ws->image_list.size();
			});
			proto.add_async_method("loadImage", [](core::workspace* ws, std::string path, std::optional<ldr::handle> ldr) {
				return ws->load_image(path, ldr.value_or(std::nullopt)).value();
			});
			proto.add_async_method("loadImageInMemory", [](core::workspace* ws, std::vector<u8> data, std::optional<ldr::handle> ldr) {
				return ws->load_image_in_memory(data, ldr.value_or(std::nullopt)).value();
			});
		}
	};
};


using namespace retro;
using Engine = bind::js::engine; // template<typename Engine>
static void export_api(const Engine& eng, const Engine::object_type& mod) {
	using object = typename Engine::object_type;
	using function = typename Engine::function_type;

	/*
	TODO:
	 Erasing&Inserting instructions

	 Try to alocate less :(
	 Optimizers
	 Section symbol reloc
	 Callbacks, interface registration
	 Z3x
	*/

	eng.export_type<bind::task_wrapper>(mod);
	eng.export_type<neo::scheduler>(mod);
	eng.export_type<ir::value>(mod);
	eng.export_type<ir::operand>(mod);
	eng.export_type<ir::constant>(mod);
	eng.export_type<ir::insn>(mod);
	eng.export_type<ir::basic_block>(mod);
	eng.export_type<ir::routine>(mod);
	eng.export_type<arch::imm>(mod);
	eng.export_type<arch::mreg>(mod);
	eng.export_type<arch::mem>(mod);
	eng.export_type<arch::minsn>(mod);
	eng.export_type<arch::instance>(mod);
	eng.export_type<ldr::instance>(mod);
	eng.export_type<core::image>(mod);
	eng.export_type<core::workspace>(mod);
	eng.export_type<z3x::expr>(mod);
	eng.export_type<z3x::variable_set>(mod);

	auto clang = object::make(eng, 3);
	clang.set("locate", function::make(eng, "clang.locate", [](std::optional<std::string> at) {
		std::optional<std::string_view> result;
		if (auto res = llvm::locate_install(at.value_or(std::string{})))
			result.emplace(*res);
		return result;
	}));
	clang.set("compile", function::make_async(eng, "clang.compile", [](std::string source, std::optional<std::string> arguments) {
		std::string err;
		if (!arguments)
			arguments.emplace();
		auto res = llvm::compile(source, *arguments, &err);
		if (!err.empty())
			throw std::runtime_error(std::move(err));
		return res;
	}));
	clang.set("compileTestCase", function::make_async(eng, "clang.compileTestCase", [](std::string source, std::optional<std::string> arguments) {
		std::string err;
		if (!arguments)
			arguments.emplace();
		auto res = llvm::compile_test_case(std::move(source), std::move(arguments).value(), &err);
		if (!err.empty())
			throw std::runtime_error(std::move(err));
		return res;
	}));
	clang.set("format", function::make_async(eng, "clang.format", [](std::string source, std::optional<std::string> style) {
		std::string err;
		if (!style)
			style.emplace();
		auto res = llvm::format(source, *style, &err);
		if (!err.empty())
			throw std::runtime_error(std::move(err));
		return res;
	}));
	clang.freeze();
	mod.set("Clang", clang);
	mod.freeze();
}

NAPI_MODULE_INIT() {
	platform::setup_ansi_escapes();
	platform::g_affinity_mask = bit_mask(std::min<i32>(i32(std::thread::hardware_concurrency() * 0.75f), 64));
	neo::scheduler::default_instance.update_affinity();

	try {
		core::on_minsn_lift.insert([](arch::handle arch, ir::basic_block* bb, arch::minsn& i, u64 va) {
			if (i.mnemonic == ZYDIS_MNEMONIC_VMREAD) {
				auto str		= (const char*) bb->get_image()->slice(i.op[0].m.disp + va + i.length - bb->get_image()->base_address).data();
				auto result = bb->push_annotation(ir::int_type(i.op[1].get_width()), std::string_view{str});
				auto write	= bb->push_write_reg(i.op[1].r, result);
				arch->explode_write_reg(write);
				return true;
			} else if (i.mnemonic == ZYDIS_MNEMONIC_VMWRITE) {
				auto str	 = (const char*) bb->get_image()->slice(i.op[1].m.disp + va + i.length - bb->get_image()->base_address).data();
				auto read = bb->push_read_reg(ir::int_type(i.op[0].get_width()), i.op[0].r);
				bb->push_annotation(ir::type::none, std::string_view{str}, read);
				return true;
			}
			return false;
		});

		bind::js::engine ctx{env};
		bind::js::object mod{env, exports};
		bind::js::local_context::init(env);

		export_api(ctx, mod);

		return exports;
	} catch (std::exception ex) {
		napi_throw_error(env, nullptr, ex.what());
		return {};
	}
}

#if RC_WINDOWS
	#include <Windows.h>
	#include <DbgHelp.h>
	#pragma comment(lib, "Dbghelp.lib")

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
	SetErrorMode(0);
	SetUnhandledExceptionFilter([](PEXCEPTION_POINTERS ep) -> LONG {
		auto file = CreateFileA("crashdump.dmp", GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

		MINIDUMP_EXCEPTION_INFORMATION ex_info;
		ex_info.ThreadId			  = GetCurrentThreadId();
		ex_info.ExceptionPointers = ep;
		ex_info.ClientPointers	  = FALSE;

		BOOL state =
			 MiniDumpWriteDump((HANDLE) -1, GetCurrentProcessId(), file, MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory), &ex_info, nullptr, nullptr);
		if (state) {
			fmt::printf("Exception 0x%08x @ %p, %s", (u32)ep->ExceptionRecord->ExceptionCode, ep->ExceptionRecord->ExceptionAddress, state ? "Written minidump." : "Failed to write minidump.");
		}
		return EXCEPTION_EXECUTE_HANDLER;
	});

	return TRUE;
}
#endif