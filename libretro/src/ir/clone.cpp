#include <retro/ir/insn.hpp>
#include <retro/ir/basic_block.hpp>
#include <retro/ir/routine.hpp>

namespace retro::ir {
	// Post clone helpers.
	//
	static void post_clone(value*& v, u64 mark) {
		if (v && v->tmp_monotonic == mark) {
			v = (value*) v->tmp_mapping;
		}
	}
	static void post_clone(weak<value>& v, u64 mark) { post_clone(v.ptr, mark); }
	static void post_clone(ref<value>& v, u64 mark) { post_clone(v.ptr, mark); }
	static void post_clone(operand& o, u64 mark) {
		if (o.is_value()) {
			value* v = o.get_value();
			post_clone(v, mark);
			if (v != o.get_value()) {
				o = v;
			}
		}
	}

	// Instruction cloning.
	//
	static ref<insn> pre_clone(const insn* ins, u64 mark) {
		auto new_ins = insn::allocate(ins->operand_count);

		// Copy basic information and save the mapping.
		//
		ins->tmp_monotonic		= mark;
		ins->tmp_mapping			= (u64) new_ins.get();
		new_ins->name				= ins->name;
		new_ins->arch				= ins->arch;
		new_ins->op					= ins->op;
		new_ins->template_types = ins->template_types;
		new_ins->ip					= ins->ip;
		for (size_t n = 0; n != ins->operand_count; n++) {
			new_ins->opr(n).reset(ins->opr(n));
		}
		return new_ins;
	}
	static void post_clone(insn* ins, u64 mark) {
		for (auto& op : ins->operands()) {
			post_clone(op, mark);
		}
	}

	// Basic block cloning.
	//
	static ref<basic_block> pre_clone(const basic_block* blk, u64 mark) {
		auto new_blk = make_rc<basic_block>();

		// Copy basic information and save the mapping.
		//
		blk->tmp_monotonic				= mark;
		blk->tmp_mapping					= (u64) new_blk.get();
		new_blk->rtn						= blk->rtn;
		new_blk->name						= blk->name;
		new_blk->ip							= blk->ip;
		new_blk->end_ip					= blk->end_ip;
		new_blk->arch						= blk->arch;
		new_blk->predecessors			= blk->predecessors;
		new_blk->successors				= blk->successors;
		for (auto ins : blk->insns()) {
			auto new_ins = pre_clone(ins, mark);
			new_ins->bb = new_blk;
			list::link_before(new_blk->end().get(), new_ins.release());
		}
		return new_blk;
	}
	static void post_clone(basic_block* blk, u64 mark) {
		// Fix references.
		//
		for (auto& ref : blk->predecessors) {
			RC_ASSERT(ref->tmp_monotonic == mark);
			ref = (basic_block*) ref->tmp_mapping;
		}
		for (auto& ref : blk->successors) {
			RC_ASSERT(ref->tmp_monotonic == mark);
			ref = (basic_block*) ref->tmp_mapping;
		}
		for (auto ins : blk->insns()) {
			post_clone(ins, mark);
		}
	}

	// Routine cloning.
	//
	static ref<routine> pre_clone(const routine* rtn, u64 mark) {
		// Copy basic data.
		//
		auto new_rtn						 = make_rc<routine>();
		new_rtn->ip							 = rtn->ip;
		new_rtn->method					 = rtn->method;
		new_rtn->next_blk_name			 = rtn->next_blk_name;
		new_rtn->next_ins_name			 = rtn->next_ins_name;
		new_rtn->last_cfg_modify_timer = rtn->last_cfg_modify_timer;

		// Copy each basic block.
		//
		size_t num_blocks = rtn->blocks.size();
		new_rtn->blocks.resize(num_blocks);
		for (size_t n = 0; n != num_blocks; n++) {
			auto new_blk		 = pre_clone(rtn->blocks[n], mark);
			new_blk->rtn		 = new_rtn;
			if (rtn->blocks[n] == rtn->entry_point)
				new_rtn->entry_point = new_blk;
			new_rtn->blocks[n] = std::move(new_blk);
		}
		return new_rtn;
	}
	static void post_clone(routine* rtn, u64 mark) {
		for (size_t n = 0; n != rtn->blocks.size(); n++) {
			post_clone(rtn->blocks[n], mark);
		}
	}

	// Exposed interface.
	//
	ref<routine> routine::clone() const {
		auto mark	= intrin::cycle_counter();
		auto result = pre_clone(this, mark);
		post_clone(result, mark);
		return result;
	}
};