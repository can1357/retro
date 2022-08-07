#include <retro/opt/interface.hpp>
#include <retro/opt/utility.hpp>
#include <retro/core/method.hpp>
#include <retro/core/image.hpp>
#include <retro/core/workspace.hpp>

namespace retro::ir::opt {
	// Local constant folding.
	//
	size_t const_fold(basic_block* bb) {
		size_t n = 0;

		// For each instruction:
		//
		for (auto* ins : bb->insns()) {
			// If we can evaluate the instruction in a constant manner, do so.
			//
			if (ins->op == opcode::binop || ins->op == opcode::cmp) {
				auto& opc = ins->opr(0);
				auto& lhs = ins->opr(1);
				auto& rhs = ins->opr(2);
				if (lhs.is_const() && rhs.is_const()) {
					if (auto res = lhs.get_const().apply(opc.get_const().get<op>(), rhs.get_const())) {
						n += 1 + ins->replace_all_uses_with(res);
					}
				}
			} else if (ins->op == opcode::unop) {
				auto& opc = ins->opr(0);
				auto& lhs = ins->opr(1);
				if (lhs.is_const()) {
					if (auto res = lhs.get_const().apply(opc.get_const().get<op>())) {
						n += 1 + ins->replace_all_uses_with(res);
					}
				}
			} else if (ins->op == opcode::insert) {
				auto& vec  = ins->opr(0);
				auto& lane = ins->opr(1);
				auto& val  = ins->opr(2);
				if (vec.is_const() && lane.is_const() && val.is_const()) {
					auto& data = vec.get_const();

					auto& vec_info = enum_reflect(vec.get_type());
					RC_ASSERT(vec_info.underlying == val.get_type());

					size_t data_len = vec_info.bit_size / (8 * vec_info.lane_width);
					size_t offset	 = data_len * lane.get_const().get_u64();
					offset %= data.size();
					RC_ASSERT(data.size() >= (offset + data_len));

					constant clone = data;
					memcpy(((u8*) clone.address()) + offset, val.get_const().address(), data_len);

					n += 1 + ins->replace_all_uses_with(clone);
				}
			} else if (ins->op == opcode::extract) {
				auto& vec  = ins->opr(0);
				auto& lane = ins->opr(1);
				if (vec.is_const() && lane.is_const()) {
					auto& data = vec.get_const();

					auto& vec_info = enum_reflect(vec.get_type());
					RC_ASSERT(vec_info.underlying == ins->template_types[1]);

					size_t data_len = vec_info.bit_size / (8 * vec_info.lane_width);
					size_t offset	 = data_len * lane.get_const().get_u64();
					offset %= data.size();
					RC_ASSERT(data.size() >= (offset + data_len));


					constant element{ins->template_types[1], std::span{((const u8*) data.address()) + offset, data_len}};
					RC_ASSERT(element);
					n += 1 + ins->replace_all_uses_with(element);
				}
			} else if (ins->op == opcode::cast) {
				auto	into = ins->template_types[1];
				auto& val  = ins->opr(0);
				if (val.is_const()) {
					if (auto res = val.get_const().cast_zx(into)) {
						n += 1 + ins->replace_all_uses_with(res);
					}
				}
			} else if (ins->op == opcode::cast_sx) {
				auto	into = ins->template_types[1];
				auto& val  = ins->opr(0);
				if (val.is_const()) {
					if (auto res = val.get_const().cast_sx(into)) {
						n += 1 + ins->replace_all_uses_with(res);
					}
				}
			} else if (ins->op == opcode::bitcast) {
				auto	into = ins->template_types[1];
				auto& val  = ins->opr(0);
				if (val.is_const()) {
					auto cv = val.get_const().bitcast(into);
					RC_ASSERT(cv);
					n += ins->replace_all_uses_with(cv);
				}
			} else if (ins->op == opcode::select) {
				auto& val = ins->opr(0);
				if (val.is_const()) {
					n += 1 + ins->replace_all_uses_with(ins->opr(val.const_val.get<bool>() ? 1 : 2));
				}
			}
		}
		return util::complete(bb, n);
	}
};
