#pragma once
#include <retro/common.hpp>
#include <retro/ir/builtin_types.hxx>

// clang-format off
namespace retro::ir {
	enum class opcode_kind : u8 /*:4*/ {
		none            = 0,
		memory          = 1,
		memory_rmw      = 2,
		arch            = 3,
		stack           = 4,
		data            = 5,
		cast            = 6,
		numeric         = 7,
		numeric_rmw     = 8,
		predicate       = 9,
		phi             = 10,
		branch          = 11,
		external_branch = 12,
		intrinsic       = 13,
		trap            = 14,
		// PSEUDO
		last            = 14,
		bit_width       = 4,
	};
	#define RC_VISIT_IR_OPCODE_KIND(_) _(memory) _(memory_rmw) _(arch) _(stack) _(data) _(cast) _(numeric) _(numeric_rmw) _(predicate) _(phi) _(branch) _(external_branch) _(intrinsic) _(trap)
	enum class opcode : u8 /*:6*/ {
		none                 = 0,
		stack_begin          = 1,
		stack_reset          = 2,
		read_reg             = 3,
		write_reg            = 4,
		load_mem             = 5,
		store_mem            = 6,
		undef                = 7,
		poison               = 8,
		extract              = 9,
		insert               = 10,
		context_begin        = 11,
		extract_context      = 12,
		insert_context       = 13,
		cast_sx              = 14,
		cast                 = 15,
		bitcast              = 16,
		binop                = 17,
		unop                 = 18,
		atomic_cmpxchg       = 19,
		atomic_xchg          = 20,
		atomic_binop         = 21,
		atomic_unop          = 22,
		cmp                  = 23,
		phi                  = 24,
		select               = 25,
		xcall                = 26,
		call                 = 27,
		intrinsic            = 28,
		sideeffect_intrinsic = 29,
		xjmp                 = 30,
		jmp                  = 31,
		xjs                  = 32,
		js                   = 33,
		xret                 = 34,
		ret                  = 35,
		annotation           = 36,
		trap                 = 37,
		nop                  = 38,
		unreachable          = 39,
		// PSEUDO
		last                 = 39,
		bit_width            = 6,
	};
	#define RC_VISIT_IR_OPCODE(_) _(stack_begin,RC_IDENTITY(inline ref<insn> make_stack_begin() {auto r = insn::allocate(0);r->op = opcode::stack_begin;r->validate().raise();return r;}),RC_IDENTITY(inline insn* push_stack_begin() {auto r = insn::allocate(0);r->op = opcode::stack_begin;r->validate().raise();return this->push_back(std::move(r));})) _(stack_reset,RC_IDENTITY(template<typename Tsp>inline ref<insn> make_stack_reset(Tsp&& sp) {auto r = insn::allocate(1);r->op = opcode::stack_reset;r->set_operands(0, std::forward<Tsp>(sp));r->validate().raise();return r;}),RC_IDENTITY(template<typename Tsp>inline insn* push_stack_reset(Tsp&& sp) {auto r = insn::allocate(1);r->op = opcode::stack_reset;r->set_operands(0, std::forward<Tsp>(sp));r->validate().raise();return this->push_back(std::move(r));})) _(read_reg,RC_IDENTITY(inline ref<insn> make_read_reg(type t0, type_t<type::reg> regid) {auto r = insn::allocate(1);r->op = opcode::read_reg;r->template_types[0] = t0;r->set_operands(0, regid);r->validate().raise();return r;}),RC_IDENTITY(inline insn* push_read_reg(type t0, type_t<type::reg> regid) {auto r = insn::allocate(1);r->op = opcode::read_reg;r->template_types[0] = t0;r->set_operands(0, regid);r->validate().raise();return this->push_back(std::move(r));})) _(write_reg,RC_IDENTITY(template<typename Tvalue>inline ref<insn> make_write_reg(type_t<type::reg> regid, Tvalue&& value) {auto r = insn::allocate(2);r->op = opcode::write_reg;r->set_operands(0, regid);r->set_operands(1, std::forward<Tvalue>(value));r->template_types[0] = r->operands()[1].get_type();r->validate().raise();return r;}),RC_IDENTITY(template<typename Tvalue>inline insn* push_write_reg(type_t<type::reg> regid, Tvalue&& value) {auto r = insn::allocate(2);r->op = opcode::write_reg;r->set_operands(0, regid);r->set_operands(1, std::forward<Tvalue>(value));r->template_types[0] = r->operands()[1].get_type();r->validate().raise();return this->push_back(std::move(r));})) _(load_mem,RC_IDENTITY(template<typename Tpointer>inline ref<insn> make_load_mem(type t0, Tpointer&& pointer, type_t<type::i64> offset) {auto r = insn::allocate(2);r->op = opcode::load_mem;r->template_types[0] = t0;r->set_operands(0, std::forward<Tpointer>(pointer));r->set_operands(1, offset);r->validate().raise();return r;}),RC_IDENTITY(template<typename Tpointer>inline insn* push_load_mem(type t0, Tpointer&& pointer, type_t<type::i64> offset) {auto r = insn::allocate(2);r->op = opcode::load_mem;r->template_types[0] = t0;r->set_operands(0, std::forward<Tpointer>(pointer));r->set_operands(1, offset);r->validate().raise();return this->push_back(std::move(r));})) _(store_mem,RC_IDENTITY(template<typename Tvalue, typename Tpointer>inline ref<insn> make_store_mem(Tpointer&& pointer, type_t<type::i64> offset, Tvalue&& value) {auto r = insn::allocate(3);r->op = opcode::store_mem;r->set_operands(0, std::forward<Tpointer>(pointer));r->set_operands(1, offset);r->set_operands(2, std::forward<Tvalue>(value));r->template_types[0] = r->operands()[2].get_type();r->validate().raise();return r;}),RC_IDENTITY(template<typename Tvalue, typename Tpointer>inline insn* push_store_mem(Tpointer&& pointer, type_t<type::i64> offset, Tvalue&& value) {auto r = insn::allocate(3);r->op = opcode::store_mem;r->set_operands(0, std::forward<Tpointer>(pointer));r->set_operands(1, offset);r->set_operands(2, std::forward<Tvalue>(value));r->template_types[0] = r->operands()[2].get_type();r->validate().raise();return this->push_back(std::move(r));})) _(undef,RC_IDENTITY(inline ref<insn> make_undef(type t0) {auto r = insn::allocate(0);r->op = opcode::undef;r->template_types[0] = t0;r->validate().raise();return r;}),RC_IDENTITY(inline insn* push_undef(type t0) {auto r = insn::allocate(0);r->op = opcode::undef;r->template_types[0] = t0;r->validate().raise();return this->push_back(std::move(r));})) _(poison,RC_IDENTITY(inline ref<insn> make_poison(type t0, type_t<type::str> reason) {auto r = insn::allocate(1);r->op = opcode::poison;r->template_types[0] = t0;r->set_operands(0, reason);r->validate().raise();return r;}),RC_IDENTITY(inline insn* push_poison(type t0, type_t<type::str> reason) {auto r = insn::allocate(1);r->op = opcode::poison;r->template_types[0] = t0;r->set_operands(0, reason);r->validate().raise();return this->push_back(std::move(r));})) _(extract,RC_IDENTITY(template<typename Tvector>inline ref<insn> make_extract(type t1, Tvector&& vector, type_t<type::i32> lane) {auto r = insn::allocate(2);r->op = opcode::extract;r->template_types[1] = t1;r->set_operands(0, std::forward<Tvector>(vector));r->set_operands(1, lane);r->template_types[0] = r->operands()[0].get_type();r->validate().raise();return r;}),RC_IDENTITY(template<typename Tvector>inline insn* push_extract(type t1, Tvector&& vector, type_t<type::i32> lane) {auto r = insn::allocate(2);r->op = opcode::extract;r->template_types[1] = t1;r->set_operands(0, std::forward<Tvector>(vector));r->set_operands(1, lane);r->template_types[0] = r->operands()[0].get_type();r->validate().raise();return this->push_back(std::move(r));})) _(insert,RC_IDENTITY(template<typename Telement, typename Tvector>inline ref<insn> make_insert(Tvector&& vector, type_t<type::i32> lane, Telement&& element) {auto r = insn::allocate(3);r->op = opcode::insert;r->set_operands(0, std::forward<Tvector>(vector));r->set_operands(1, lane);r->set_operands(2, std::forward<Telement>(element));r->template_types[0] = r->operands()[0].get_type();r->template_types[1] = r->operands()[2].get_type();r->validate().raise();return r;}),RC_IDENTITY(template<typename Telement, typename Tvector>inline insn* push_insert(Tvector&& vector, type_t<type::i32> lane, Telement&& element) {auto r = insn::allocate(3);r->op = opcode::insert;r->set_operands(0, std::forward<Tvector>(vector));r->set_operands(1, lane);r->set_operands(2, std::forward<Telement>(element));r->template_types[0] = r->operands()[0].get_type();r->template_types[1] = r->operands()[2].get_type();r->validate().raise();return this->push_back(std::move(r));})) _(context_begin,RC_IDENTITY(template<typename Tsp>inline ref<insn> make_context_begin(Tsp&& sp) {auto r = insn::allocate(1);r->op = opcode::context_begin;r->set_operands(0, std::forward<Tsp>(sp));r->validate().raise();return r;}),RC_IDENTITY(template<typename Tsp>inline insn* push_context_begin(Tsp&& sp) {auto r = insn::allocate(1);r->op = opcode::context_begin;r->set_operands(0, std::forward<Tsp>(sp));r->validate().raise();return this->push_back(std::move(r));})) _(extract_context,RC_IDENTITY(template<typename Tctx>inline ref<insn> make_extract_context(type t0, Tctx&& ctx, type_t<type::reg> regid) {auto r = insn::allocate(2);r->op = opcode::extract_context;r->template_types[0] = t0;r->set_operands(0, std::forward<Tctx>(ctx));r->set_operands(1, regid);r->validate().raise();return r;}),RC_IDENTITY(template<typename Tctx>inline insn* push_extract_context(type t0, Tctx&& ctx, type_t<type::reg> regid) {auto r = insn::allocate(2);r->op = opcode::extract_context;r->template_types[0] = t0;r->set_operands(0, std::forward<Tctx>(ctx));r->set_operands(1, regid);r->validate().raise();return this->push_back(std::move(r));})) _(insert_context,RC_IDENTITY(template<typename Telement, typename Tctx>inline ref<insn> make_insert_context(Tctx&& ctx, type_t<type::reg> regid, Telement&& element) {auto r = insn::allocate(3);r->op = opcode::insert_context;r->set_operands(0, std::forward<Tctx>(ctx));r->set_operands(1, regid);r->set_operands(2, std::forward<Telement>(element));r->template_types[0] = r->operands()[2].get_type();r->validate().raise();return r;}),RC_IDENTITY(template<typename Telement, typename Tctx>inline insn* push_insert_context(Tctx&& ctx, type_t<type::reg> regid, Telement&& element) {auto r = insn::allocate(3);r->op = opcode::insert_context;r->set_operands(0, std::forward<Tctx>(ctx));r->set_operands(1, regid);r->set_operands(2, std::forward<Telement>(element));r->template_types[0] = r->operands()[2].get_type();r->validate().raise();return this->push_back(std::move(r));})) _(cast_sx,RC_IDENTITY(template<typename Tvalue>inline ref<insn> make_cast_sx(type t1, Tvalue&& value) {auto r = insn::allocate(1);r->op = opcode::cast_sx;r->template_types[1] = t1;r->set_operands(0, std::forward<Tvalue>(value));r->template_types[0] = r->operands()[0].get_type();r->validate().raise();return r;}),RC_IDENTITY(template<typename Tvalue>inline insn* push_cast_sx(type t1, Tvalue&& value) {auto r = insn::allocate(1);r->op = opcode::cast_sx;r->template_types[1] = t1;r->set_operands(0, std::forward<Tvalue>(value));r->template_types[0] = r->operands()[0].get_type();r->validate().raise();return this->push_back(std::move(r));})) _(cast,RC_IDENTITY(template<typename Tvalue>inline ref<insn> make_cast(type t1, Tvalue&& value) {auto r = insn::allocate(1);r->op = opcode::cast;r->template_types[1] = t1;r->set_operands(0, std::forward<Tvalue>(value));r->template_types[0] = r->operands()[0].get_type();r->validate().raise();return r;}),RC_IDENTITY(template<typename Tvalue>inline insn* push_cast(type t1, Tvalue&& value) {auto r = insn::allocate(1);r->op = opcode::cast;r->template_types[1] = t1;r->set_operands(0, std::forward<Tvalue>(value));r->template_types[0] = r->operands()[0].get_type();r->validate().raise();return this->push_back(std::move(r));})) _(bitcast,RC_IDENTITY(template<typename Tvalue>inline ref<insn> make_bitcast(type t1, Tvalue&& value) {auto r = insn::allocate(1);r->op = opcode::bitcast;r->template_types[1] = t1;r->set_operands(0, std::forward<Tvalue>(value));r->template_types[0] = r->operands()[0].get_type();r->validate().raise();return r;}),RC_IDENTITY(template<typename Tvalue>inline insn* push_bitcast(type t1, Tvalue&& value) {auto r = insn::allocate(1);r->op = opcode::bitcast;r->template_types[1] = t1;r->set_operands(0, std::forward<Tvalue>(value));r->template_types[0] = r->operands()[0].get_type();r->validate().raise();return this->push_back(std::move(r));})) _(binop,RC_IDENTITY(template<typename Trhs, typename Tlhs>inline ref<insn> make_binop(type_t<type::op> op, Tlhs&& lhs, Trhs&& rhs) {auto r = insn::allocate(3);r->op = opcode::binop;r->set_operands(0, op);r->set_operands(1, std::forward<Tlhs>(lhs));r->set_operands(2, std::forward<Trhs>(rhs));r->template_types[0] = r->operands()[1].get_type();r->validate().raise();return r;}),RC_IDENTITY(template<typename Trhs, typename Tlhs>inline insn* push_binop(type_t<type::op> op, Tlhs&& lhs, Trhs&& rhs) {auto r = insn::allocate(3);r->op = opcode::binop;r->set_operands(0, op);r->set_operands(1, std::forward<Tlhs>(lhs));r->set_operands(2, std::forward<Trhs>(rhs));r->template_types[0] = r->operands()[1].get_type();r->validate().raise();return this->push_back(std::move(r));})) _(unop,RC_IDENTITY(template<typename Trhs>inline ref<insn> make_unop(type_t<type::op> op, Trhs&& rhs) {auto r = insn::allocate(2);r->op = opcode::unop;r->set_operands(0, op);r->set_operands(1, std::forward<Trhs>(rhs));r->template_types[0] = r->operands()[1].get_type();r->validate().raise();return r;}),RC_IDENTITY(template<typename Trhs>inline insn* push_unop(type_t<type::op> op, Trhs&& rhs) {auto r = insn::allocate(2);r->op = opcode::unop;r->set_operands(0, op);r->set_operands(1, std::forward<Trhs>(rhs));r->template_types[0] = r->operands()[1].get_type();r->validate().raise();return this->push_back(std::move(r));})) _(atomic_cmpxchg,RC_IDENTITY(template<typename Tdesired, typename Texpected, typename Tptr>inline ref<insn> make_atomic_cmpxchg(Tptr&& ptr, Texpected&& expected, Tdesired&& desired) {auto r = insn::allocate(3);r->op = opcode::atomic_cmpxchg;r->set_operands(0, std::forward<Tptr>(ptr));r->set_operands(1, std::forward<Texpected>(expected));r->set_operands(2, std::forward<Tdesired>(desired));r->template_types[0] = r->operands()[1].get_type();r->validate().raise();return r;}),RC_IDENTITY(template<typename Tdesired, typename Texpected, typename Tptr>inline insn* push_atomic_cmpxchg(Tptr&& ptr, Texpected&& expected, Tdesired&& desired) {auto r = insn::allocate(3);r->op = opcode::atomic_cmpxchg;r->set_operands(0, std::forward<Tptr>(ptr));r->set_operands(1, std::forward<Texpected>(expected));r->set_operands(2, std::forward<Tdesired>(desired));r->template_types[0] = r->operands()[1].get_type();r->validate().raise();return this->push_back(std::move(r));})) _(atomic_xchg,RC_IDENTITY(template<typename Tdesired, typename Tptr>inline ref<insn> make_atomic_xchg(Tptr&& ptr, Tdesired&& desired) {auto r = insn::allocate(2);r->op = opcode::atomic_xchg;r->set_operands(0, std::forward<Tptr>(ptr));r->set_operands(1, std::forward<Tdesired>(desired));r->template_types[0] = r->operands()[1].get_type();r->validate().raise();return r;}),RC_IDENTITY(template<typename Tdesired, typename Tptr>inline insn* push_atomic_xchg(Tptr&& ptr, Tdesired&& desired) {auto r = insn::allocate(2);r->op = opcode::atomic_xchg;r->set_operands(0, std::forward<Tptr>(ptr));r->set_operands(1, std::forward<Tdesired>(desired));r->template_types[0] = r->operands()[1].get_type();r->validate().raise();return this->push_back(std::move(r));})) _(atomic_binop,RC_IDENTITY(template<typename Trhs, typename Tlhs_ptr>inline ref<insn> make_atomic_binop(type_t<type::op> op, Tlhs_ptr&& lhs_ptr, Trhs&& rhs) {auto r = insn::allocate(3);r->op = opcode::atomic_binop;r->set_operands(0, op);r->set_operands(1, std::forward<Tlhs_ptr>(lhs_ptr));r->set_operands(2, std::forward<Trhs>(rhs));r->template_types[0] = r->operands()[2].get_type();r->validate().raise();return r;}),RC_IDENTITY(template<typename Trhs, typename Tlhs_ptr>inline insn* push_atomic_binop(type_t<type::op> op, Tlhs_ptr&& lhs_ptr, Trhs&& rhs) {auto r = insn::allocate(3);r->op = opcode::atomic_binop;r->set_operands(0, op);r->set_operands(1, std::forward<Tlhs_ptr>(lhs_ptr));r->set_operands(2, std::forward<Trhs>(rhs));r->template_types[0] = r->operands()[2].get_type();r->validate().raise();return this->push_back(std::move(r));})) _(atomic_unop,RC_IDENTITY(template<typename Trhs_ptr>inline ref<insn> make_atomic_unop(type t0, type_t<type::op> op, Trhs_ptr&& rhs_ptr) {auto r = insn::allocate(2);r->op = opcode::atomic_unop;r->template_types[0] = t0;r->set_operands(0, op);r->set_operands(1, std::forward<Trhs_ptr>(rhs_ptr));r->validate().raise();return r;}),RC_IDENTITY(template<typename Trhs_ptr>inline insn* push_atomic_unop(type t0, type_t<type::op> op, Trhs_ptr&& rhs_ptr) {auto r = insn::allocate(2);r->op = opcode::atomic_unop;r->template_types[0] = t0;r->set_operands(0, op);r->set_operands(1, std::forward<Trhs_ptr>(rhs_ptr));r->validate().raise();return this->push_back(std::move(r));})) _(cmp,RC_IDENTITY(template<typename Trhs, typename Tlhs>inline ref<insn> make_cmp(type_t<type::op> op, Tlhs&& lhs, Trhs&& rhs) {auto r = insn::allocate(3);r->op = opcode::cmp;r->set_operands(0, op);r->set_operands(1, std::forward<Tlhs>(lhs));r->set_operands(2, std::forward<Trhs>(rhs));r->template_types[0] = r->operands()[1].get_type();r->validate().raise();return r;}),RC_IDENTITY(template<typename Trhs, typename Tlhs>inline insn* push_cmp(type_t<type::op> op, Tlhs&& lhs, Trhs&& rhs) {auto r = insn::allocate(3);r->op = opcode::cmp;r->set_operands(0, op);r->set_operands(1, std::forward<Tlhs>(lhs));r->set_operands(2, std::forward<Trhs>(rhs));r->template_types[0] = r->operands()[1].get_type();r->validate().raise();return this->push_back(std::move(r));})) _(phi,RC_IDENTITY(template<typename ...Tincoming>inline ref<insn> make_phi(type t0, Tincoming&&... incoming) {auto r = insn::allocate(sizeof...(Tincoming)+0);r->op = opcode::phi;r->template_types[0] = t0;r->set_operands(0, std::forward<Tincoming>(incoming)...);r->validate().raise();return r;}),RC_IDENTITY(template<typename ...Tincoming>inline insn* push_phi(type t0, Tincoming&&... incoming) {auto r = insn::allocate(sizeof...(Tincoming)+0);r->op = opcode::phi;r->template_types[0] = t0;r->set_operands(0, std::forward<Tincoming>(incoming)...);r->validate().raise();return this->push_back(std::move(r));})) _(select,RC_IDENTITY(template<typename Tfv, typename Ttv, typename Tcc>inline ref<insn> make_select(Tcc&& cc, Ttv&& tv, Tfv&& fv) {auto r = insn::allocate(3);r->op = opcode::select;r->set_operands(0, std::forward<Tcc>(cc));r->set_operands(1, std::forward<Ttv>(tv));r->set_operands(2, std::forward<Tfv>(fv));r->template_types[0] = r->operands()[1].get_type();r->validate().raise();return r;}),RC_IDENTITY(template<typename Tfv, typename Ttv, typename Tcc>inline insn* push_select(Tcc&& cc, Ttv&& tv, Tfv&& fv) {auto r = insn::allocate(3);r->op = opcode::select;r->set_operands(0, std::forward<Tcc>(cc));r->set_operands(1, std::forward<Ttv>(tv));r->set_operands(2, std::forward<Tfv>(fv));r->template_types[0] = r->operands()[1].get_type();r->validate().raise();return this->push_back(std::move(r));})) _(xcall,RC_IDENTITY(template<typename Tdestination>inline ref<insn> make_xcall(Tdestination&& destination) {auto r = insn::allocate(1);r->op = opcode::xcall;r->set_operands(0, std::forward<Tdestination>(destination));r->validate().raise();return r;}),RC_IDENTITY(template<typename Tdestination>inline insn* push_xcall(Tdestination&& destination) {auto r = insn::allocate(1);r->op = opcode::xcall;r->set_operands(0, std::forward<Tdestination>(destination));r->validate().raise();return this->push_back(std::move(r));})) _(call,RC_IDENTITY(template<typename Tctx, typename Tdestination>inline ref<insn> make_call(Tdestination&& destination, Tctx&& ctx) {auto r = insn::allocate(2);r->op = opcode::call;r->set_operands(0, std::forward<Tdestination>(destination));r->set_operands(1, std::forward<Tctx>(ctx));r->validate().raise();return r;}),RC_IDENTITY(template<typename Tctx, typename Tdestination>inline insn* push_call(Tdestination&& destination, Tctx&& ctx) {auto r = insn::allocate(2);r->op = opcode::call;r->set_operands(0, std::forward<Tdestination>(destination));r->set_operands(1, std::forward<Tctx>(ctx));r->validate().raise();return this->push_back(std::move(r));})) _(intrinsic,RC_IDENTITY(template<typename ...Targs>inline ref<insn> make_intrinsic(type_t<type::intrinsic> func, Targs&&... args) {auto r = insn::allocate(sizeof...(Targs)+1);r->op = opcode::intrinsic;r->set_operands(0, func);r->set_operands(1, std::forward<Targs>(args)...);r->validate().raise();return r;}),RC_IDENTITY(template<typename ...Targs>inline insn* push_intrinsic(type_t<type::intrinsic> func, Targs&&... args) {auto r = insn::allocate(sizeof...(Targs)+1);r->op = opcode::intrinsic;r->set_operands(0, func);r->set_operands(1, std::forward<Targs>(args)...);r->validate().raise();return this->push_back(std::move(r));})) _(sideeffect_intrinsic,RC_IDENTITY(template<typename ...Targs>inline ref<insn> make_sideeffect_intrinsic(type_t<type::intrinsic> func, Targs&&... args) {auto r = insn::allocate(sizeof...(Targs)+1);r->op = opcode::sideeffect_intrinsic;r->set_operands(0, func);r->set_operands(1, std::forward<Targs>(args)...);r->validate().raise();return r;}),RC_IDENTITY(template<typename ...Targs>inline insn* push_sideeffect_intrinsic(type_t<type::intrinsic> func, Targs&&... args) {auto r = insn::allocate(sizeof...(Targs)+1);r->op = opcode::sideeffect_intrinsic;r->set_operands(0, func);r->set_operands(1, std::forward<Targs>(args)...);r->validate().raise();return this->push_back(std::move(r));})) _(xjmp,RC_IDENTITY(template<typename Tdestination>inline ref<insn> make_xjmp(Tdestination&& destination) {auto r = insn::allocate(1);r->op = opcode::xjmp;r->set_operands(0, std::forward<Tdestination>(destination));r->validate().raise();return r;}),RC_IDENTITY(template<typename Tdestination>inline insn* push_xjmp(Tdestination&& destination) {auto r = insn::allocate(1);r->op = opcode::xjmp;r->set_operands(0, std::forward<Tdestination>(destination));r->validate().raise();return this->push_back(std::move(r));})) _(jmp,RC_IDENTITY(template<typename Tdestination>inline ref<insn> make_jmp(Tdestination&& destination) {auto r = insn::allocate(1);r->op = opcode::jmp;r->set_operands(0, std::forward<Tdestination>(destination));r->validate().raise();return r;}),RC_IDENTITY(template<typename Tdestination>inline insn* push_jmp(Tdestination&& destination) {auto r = insn::allocate(1);r->op = opcode::jmp;r->set_operands(0, std::forward<Tdestination>(destination));r->validate().raise();return this->push_back(std::move(r));})) _(xjs,RC_IDENTITY(template<typename Tcc>inline ref<insn> make_xjs(Tcc&& cc, type_t<type::pointer> tb, type_t<type::pointer> fb) {auto r = insn::allocate(3);r->op = opcode::xjs;r->set_operands(0, std::forward<Tcc>(cc));r->set_operands(1, tb);r->set_operands(2, fb);r->validate().raise();return r;}),RC_IDENTITY(template<typename Tcc>inline insn* push_xjs(Tcc&& cc, type_t<type::pointer> tb, type_t<type::pointer> fb) {auto r = insn::allocate(3);r->op = opcode::xjs;r->set_operands(0, std::forward<Tcc>(cc));r->set_operands(1, tb);r->set_operands(2, fb);r->validate().raise();return this->push_back(std::move(r));})) _(js,RC_IDENTITY(template<typename Tfb, typename Ttb, typename Tcc>inline ref<insn> make_js(Tcc&& cc, Ttb&& tb, Tfb&& fb) {auto r = insn::allocate(3);r->op = opcode::js;r->set_operands(0, std::forward<Tcc>(cc));r->set_operands(1, std::forward<Ttb>(tb));r->set_operands(2, std::forward<Tfb>(fb));r->validate().raise();return r;}),RC_IDENTITY(template<typename Tfb, typename Ttb, typename Tcc>inline insn* push_js(Tcc&& cc, Ttb&& tb, Tfb&& fb) {auto r = insn::allocate(3);r->op = opcode::js;r->set_operands(0, std::forward<Tcc>(cc));r->set_operands(1, std::forward<Ttb>(tb));r->set_operands(2, std::forward<Tfb>(fb));r->validate().raise();return this->push_back(std::move(r));})) _(xret,RC_IDENTITY(template<typename Tptr>inline ref<insn> make_xret(Tptr&& ptr) {auto r = insn::allocate(1);r->op = opcode::xret;r->set_operands(0, std::forward<Tptr>(ptr));r->validate().raise();return r;}),RC_IDENTITY(template<typename Tptr>inline insn* push_xret(Tptr&& ptr) {auto r = insn::allocate(1);r->op = opcode::xret;r->set_operands(0, std::forward<Tptr>(ptr));r->validate().raise();return this->push_back(std::move(r));})) _(ret,RC_IDENTITY(template<typename Tctx>inline ref<insn> make_ret(Tctx&& ctx, type_t<type::i64> offset) {auto r = insn::allocate(2);r->op = opcode::ret;r->set_operands(0, std::forward<Tctx>(ctx));r->set_operands(1, offset);r->validate().raise();return r;}),RC_IDENTITY(template<typename Tctx>inline insn* push_ret(Tctx&& ctx, type_t<type::i64> offset) {auto r = insn::allocate(2);r->op = opcode::ret;r->set_operands(0, std::forward<Tctx>(ctx));r->set_operands(1, offset);r->validate().raise();return this->push_back(std::move(r));})) _(annotation,RC_IDENTITY(template<typename ...Targs>inline ref<insn> make_annotation(type t0, type_t<type::str> name, Targs&&... args) {auto r = insn::allocate(sizeof...(Targs)+1);r->op = opcode::annotation;r->template_types[0] = t0;r->set_operands(0, name);r->set_operands(1, std::forward<Targs>(args)...);r->validate().raise();return r;}),RC_IDENTITY(template<typename ...Targs>inline insn* push_annotation(type t0, type_t<type::str> name, Targs&&... args) {auto r = insn::allocate(sizeof...(Targs)+1);r->op = opcode::annotation;r->template_types[0] = t0;r->set_operands(0, name);r->set_operands(1, std::forward<Targs>(args)...);r->validate().raise();return this->push_back(std::move(r));})) _(trap,RC_IDENTITY(inline ref<insn> make_trap(type_t<type::str> reason) {auto r = insn::allocate(1);r->op = opcode::trap;r->set_operands(0, reason);r->validate().raise();return r;}),RC_IDENTITY(inline insn* push_trap(type_t<type::str> reason) {auto r = insn::allocate(1);r->op = opcode::trap;r->set_operands(0, reason);r->validate().raise();return this->push_back(std::move(r));})) _(nop,RC_IDENTITY(inline ref<insn> make_nop() {auto r = insn::allocate(0);r->op = opcode::nop;r->validate().raise();return r;}),RC_IDENTITY(inline insn* push_nop() {auto r = insn::allocate(0);r->op = opcode::nop;r->validate().raise();return this->push_back(std::move(r));})) _(unreachable,RC_IDENTITY(inline ref<insn> make_unreachable() {auto r = insn::allocate(0);r->op = opcode::unreachable;r->validate().raise();return r;}),RC_IDENTITY(inline insn* push_unreachable() {auto r = insn::allocate(0);r->op = opcode::unreachable;r->validate().raise();return this->push_back(std::move(r));}))
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//                                                      Descriptors                                                      //
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	struct opcode_kind_desc {
		std::string_view name = {};
	
		using value_type = opcode_kind;
		static constexpr std::span<const opcode_kind_desc> all();
		RC_INLINE constexpr const opcode_kind id() const { return opcode_kind(this - all().data()); }
	};
	struct opcode_desc {
		std::string_view              name               = {};
		small_array<type>             types;            
		small_array<u8>               templates;        
		small_array<std::string_view> names;            
		small_array<u8>               constexprs;       
		u8                            is_annotation : 1  = 0;
		u8                            is_pure : 1        = 0;
		u8                            side_effect : 1    = 0;
		u8                            is_const : 1       = 0;
		u8                            unk_reg_use : 1    = 0;
		u8                            terminator : 1     = 0;
		u8                            bb_terminator : 1  = 0;
		u8                            template_count : 2 = 0;
		opcode_kind                   kind : 4           = {};
	
		using value_type = opcode;
		static constexpr std::span<const opcode_desc> all();
		RC_INLINE constexpr const opcode id() const { return opcode(this - all().data()); }
	};
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//                                                         Tables                                                         //
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	inline constexpr opcode_kind_desc opcode_kinds[] = {
		{"none"},
		{"memory"},
		{"memory-rmw"},
		{"arch"},
		{"stack"},
		{"data"},
		{"cast"},
		{"numeric"},
		{"numeric-rmw"},
		{"predicate"},
		{"phi"},
		{"branch"},
		{"external-branch"},
		{"intrinsic"},
		{"trap"},
	};
	RC_INLINE constexpr std::span<const opcode_kind_desc> opcode_kind_desc::all() { return opcode_kinds; }
	inline constexpr opcode_desc opcodes[] = {
		{"none"},
		{"stack_begin",{type::pointer},{0},{""},{},1,0,0,0,0,0,0,0,opcode_kind::stack},
		{"stack_reset",{type::none,type::pointer},{0,0},{"","sp"},{},1,0,0,0,0,0,0,0,opcode_kind::stack},
		{"read_reg",{type::none,type::reg},{1,0},{"","regid"},{1},0,1,0,0,0,0,0,1,opcode_kind::arch},
		{"write_reg",{type::none,type::reg,type::none},{0,0,1},{"","regid","value"},{1},0,0,1,0,0,0,0,1,opcode_kind::arch},
		{"load_mem",{type::none,type::pointer,type::i64},{1,0,0},{"","pointer","offset"},{2},0,1,0,0,0,0,0,1,opcode_kind::memory},
		{"store_mem",{type::none,type::pointer,type::i64,type::none},{0,0,0,1},{"","pointer","offset","value"},{2},0,0,1,0,0,0,0,1,opcode_kind::memory},
		{"undef",{type::none},{1},{""},{},0,1,0,1,0,0,0,1,opcode_kind::data},
		{"poison",{type::none,type::str},{1,0},{"","Reason"},{1},0,1,0,1,0,0,0,1,opcode_kind::data},
		{"extract",{type::none,type::none,type::i32},{2,1,0},{"","vector","Lane"},{2},0,1,0,1,0,0,0,2,opcode_kind::data},
		{"insert",{type::none,type::none,type::i32,type::none},{1,1,0,2},{"","vector","Lane","element"},{2},0,1,0,1,0,0,0,2,opcode_kind::data},
		{"context_begin",{type::context,type::pointer},{0,0},{"","sp"},{},1,1,0,1,0,0,0,0,opcode_kind::data},
		{"extract_context",{type::none,type::context,type::reg},{1,0,0},{"","ctx","regid"},{2},0,1,0,1,0,0,0,1,opcode_kind::data},
		{"insert_context",{type::context,type::context,type::reg,type::none},{0,0,0,1},{"","ctx","regid","element"},{2},0,1,0,1,0,0,0,1,opcode_kind::data},
		{"cast_sx",{type::none,type::none},{2,1},{"","value"},{},0,1,0,1,0,0,0,2,opcode_kind::cast},
		{"cast",{type::none,type::none},{2,1},{"","value"},{},0,1,0,1,0,0,0,2,opcode_kind::cast},
		{"bitcast",{type::none,type::none},{2,1},{"","value"},{},0,1,0,1,0,0,0,2,opcode_kind::cast},
		{"binop",{type::none,type::op,type::none,type::none},{1,0,1,1},{"","Op","lhs","rhs"},{1},0,1,0,1,0,0,0,1,opcode_kind::numeric},
		{"unop",{type::none,type::op,type::none},{1,0,1},{"","Op","rhs"},{1},0,1,0,1,0,0,0,1,opcode_kind::numeric},
		{"atomic_cmpxchg",{type::none,type::pointer,type::none,type::none},{1,0,1,1},{"","ptr","expected","desired"},{},0,0,1,0,0,0,0,1,opcode_kind::memory_rmw},
		{"atomic_xchg",{type::none,type::pointer,type::none},{1,0,1},{"","ptr","desired"},{},0,0,1,0,0,0,0,1,opcode_kind::memory_rmw},
		{"atomic_binop",{type::none,type::op,type::pointer,type::none},{1,0,0,1},{"","Op","lhs_ptr","rhs"},{1},0,0,1,0,0,0,0,1,opcode_kind::numeric_rmw},
		{"atomic_unop",{type::none,type::op,type::pointer},{1,0,0},{"","Op","rhs_ptr"},{1},0,0,1,0,0,0,0,1,opcode_kind::numeric_rmw},
		{"cmp",{type::i1,type::op,type::none,type::none},{0,0,1,1},{"","Op","lhs","rhs"},{1},0,1,0,1,0,0,0,1,opcode_kind::predicate},
		{"phi",{type::none,type::pack},{1,0},{"","incoming"},{},0,1,0,1,0,0,0,1,opcode_kind::phi},
		{"select",{type::none,type::i1,type::none,type::none},{1,0,1,1},{"","cc","tv","fv"},{},0,1,0,1,0,0,0,1,opcode_kind::data},
		{"xcall",{type::none,type::pointer},{0,0},{"","destination"},{},0,0,1,0,1,0,0,0,opcode_kind::external_branch},
		{"call",{type::context,type::pointer,type::context},{0,0,0},{"","destination","ctx"},{},0,0,1,0,0,0,0,0,opcode_kind::branch},
		{"intrinsic",{type::pack,type::intrinsic,type::pack},{0,0,0},{"","func","args"},{1},0,0,0,0,0,0,0,0,opcode_kind::intrinsic},
		{"sideeffect_intrinsic",{type::pack,type::intrinsic,type::pack},{0,0,0},{"","func","args"},{1},0,0,1,0,0,0,0,0,opcode_kind::intrinsic},
		{"xjmp",{type::none,type::pointer},{0,0},{"","destination"},{},0,0,1,0,0,1,1,0,opcode_kind::external_branch},
		{"jmp",{type::none,type::label},{0,0},{"","destination"},{},0,0,1,0,0,1,0,0,opcode_kind::branch},
		{"xjs",{type::none,type::i1,type::pointer,type::pointer},{0,0,0,0},{"","cc","tb","fb"},{2,3},0,0,1,0,0,1,1,0,opcode_kind::external_branch},
		{"js",{type::none,type::i1,type::label,type::label},{0,0,0,0},{"","cc","tb","fb"},{},0,0,1,0,0,1,0,0,opcode_kind::branch},
		{"xret",{type::none,type::pointer},{0,0},{"","ptr"},{},0,0,1,0,1,1,1,0,opcode_kind::external_branch},
		{"ret",{type::none,type::context,type::i64},{0,0,0},{"","ctx","offset"},{2},0,0,1,0,0,1,1,0,opcode_kind::branch},
		{"annotation",{type::none,type::str,type::pack},{1,0,0},{"","Name","args"},{1},1,0,0,0,0,0,0,1,opcode_kind::none},
		{"trap",{type::none,type::str},{0,0},{"","Reason"},{1},0,0,1,0,0,1,1,0,opcode_kind::trap},
		{"nop",{type::pack},{0},{""},{},0,0,0,0,0,0,0,0,opcode_kind::none},
		{"unreachable",{type::none},{0},{""},{},0,0,1,0,0,1,1,0,opcode_kind::trap},
	};
	RC_INLINE constexpr std::span<const opcode_desc> opcode_desc::all() { return opcodes; }
};
namespace retro { template<> struct descriptor<retro::ir::opcode_kind> { using type = retro::ir::opcode_kind_desc; }; };
RC_DEFINE_STD_VISITOR_FOR(retro::ir::opcode_kind, RC_VISIT_IR_OPCODE_KIND)
namespace retro { template<> struct descriptor<retro::ir::opcode> { using type = retro::ir::opcode_desc; }; };
RC_DEFINE_STD_VISITOR_FOR(retro::ir::opcode, RC_VISIT_IR_OPCODE)
// clang-format on