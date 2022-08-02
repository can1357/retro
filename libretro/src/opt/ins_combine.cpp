#include <retro/opt/interface.hpp>
#include <retro/opt/utility.hpp>
#include <retro/directives/pattern.hpp>

namespace retro::ir::opt {
	// Local instruction combination.
	//
	size_t ins_combine(basic_block* bb) {
		size_t n = 0;
		for (auto it = bb->begin(); it != bb->end();) {
			auto ins = it++;

			// Numeric rules:
			//
			if (ins->op == opcode::binop || ins->op == opcode::unop || ins->op == opcode::cmp) {
				for (auto& match : directives::replace_list) {
					pattern::match_context ctx{};
					if (match(ins, ctx)) {
						n++;
						break;
					}
				}
			}
			// Casts.
			//
			else if (ins->op == opcode::bitcast) {
				// If cast between same types, no op.
				//
				auto ty1 = ins->template_types[0];
				auto ty2 = ins->template_types[1];
				if (ty1 == ty2) {
					ins->replace_all_uses_with(ins->opr(0));
					ins->erase();
					n++;
					continue;
				}

				// If RHS is not an instruction, nothing else to match.
				//
				auto& rhsv = ins->opr(0);
				if (rhsv.is_const())
					continue;
				auto rhs = rhsv.get_value()->get_if<insn>();
				if (!rhs)
					continue;

				// If RHS is also a bitcast, propagate.
				//
				if (rhs->op == opcode::bitcast) {
					ins->template_types[0] = rhs->opr(0).get_type();
					ins->opr(0)				  = rhs->opr(0);
					n++;
				}
			} else if (ins->op == opcode::cast || ins->op == opcode::cast_sx) {
				// If cast between same types, no op.
				//
				auto ty1 = ins->template_types[0];
				auto ty2 = ins->template_types[1];
				if (ty1 == ty2) {
					n += 1 + ins->replace_all_uses_with(ins->opr(0));
					ins->erase();
					continue;
				}

				// If RHS is not an instruction, nothing else to match.
				//
				auto& rhsv = ins->opr(0);
				if (rhsv.is_const())
					continue;
				auto rhs = rhsv.get_value()->get_if<insn>();
				if (!rhs)
					continue;

				// If RHS is also a cast:
				//
				if (rhs->op == opcode::cast || rhs->op == opcode::cast_sx) {
					auto& val = rhs->opr(0);
					auto	ty0 = rhs->template_types[0];
					auto& ti0 = enum_reflect(ty0);
					auto& ti1 = enum_reflect(ty1);
					auto& ti2 = enum_reflect(ty2);

					// If cast between same type kinds:
					//
					if (ti0.kind == ti1.kind && ti1.kind == ti2.kind && ti0.lane_width == ti1.lane_width && ti1.lane_width == ti2.lane_width) {
						bool sx0_1 = rhs->op == opcode::cast_sx;
						bool sx1_2 = ins->op == opcode::cast_sx;

						// If no information lost during middle translation:
						//
						if (ti1.bit_size >= ti0.bit_size) {
							// [e.g. i16->i32->i16]
							//
							if (ti2.bit_size == ti0.bit_size) {
								n += 1 + ins->replace_all_uses_with(val);
								ins->erase();
								continue;
							}
							// [e.g. i16->i32->i16]
							//
							else if (ti2.bit_size < ti0.bit_size) {
								ins->template_types[0] = val.get_type();
								rhsv						  = val;
								continue;
							}
							// [e.g. i16->i32->i64]
							//
							else if (ti2.bit_size >= ti1.bit_size && sx0_1 == sx1_2) {
								ins->template_types[0] = val.get_type();
								rhsv						  = val;
								continue;
							}
						} else {
							// [e.g. i32->i16->i8]
							//
							if (ti2.bit_size <= ti1.bit_size) {
								ins->template_types[0] = val.get_type();
								rhsv						  = val;
								continue;
							}
						}

						// fmt::println(rhs->to_string());
						// fmt::println(ins->to_string());
						// fmt::println(ty0, sx0_1 ? "->s" : "->", ty1, sx1_2 ? "->s" : "->", ty2);
					}
				} else {
					// TODO: convert unop/binop s
				}
			}
			// Nops.
			//
			else if (ins->op == opcode::select) {
				if (util::is_identical(ins->opr(1), ins->opr(2))) {
					ins->replace_all_uses_with(ins->opr(1));
				}
			}

			// TODO:
			// cast simplification
			// write_reg with op == read_reg
		}
		return util::complete(bb, n);
	}
};
