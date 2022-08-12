#include <retro/directives/pattern.hpp>
#ifndef __INTELLISENSE__
#if RC_CLANG
    #pragma clang diagnostic ignored "-Wunused-variable"
#elif RC_GNU
    #pragma GCC diagnostic ignored "-Wunused-variable"
#endif

using op =  retro::ir::op;
using opr = retro::ir::operand;
using imm = retro::ir::constant;
using ins = retro::ir::insn;
using namespace retro::directives;
using namespace retro::pattern;


static bool __replace_pattern__1(ins* i, match_context& ctx){
opr* o5, *o4;
if(!match_binop(op::sub, &o5, &o4, i, ctx)) return false;
if(!match_symbol(0, o5, ctx)) return false;
if(!match_imm_symbol(1, o4, ctx)) return false;

ins* it = i;
ins* v2 = write_binop(it, op::add, ctx.symbols[0], ctx.symbols[1].const_val.apply(op::neg));

i->replace_all_uses_with(v2);
return true;
}

static bool __replace_pattern__6(ins* i, match_context& ctx){
opr* o10, *o9;
if(!match_binop(op::add, &o10, &o9, i, ctx)) return false;
opr* o12, *o11;
if(!match_binop(op::add, &o12, &o11, o10, ctx)) return false;
if(!match_symbol(0, o12, ctx)) return false;
if(!match_imm_symbol(1, o11, ctx)) return false;
if(!match_imm_symbol(2, o9, ctx)) return false;

ins* it = i;
ins* v7 = write_binop(it, op::add, ctx.symbols[0], ctx.symbols[1].const_val.apply(op::add, ctx.symbols[2].const_val));

i->replace_all_uses_with(v7);
return true;
}

static bool __replace_pattern__13(ins* i, match_context& ctx){
opr* o17, *o16;
if(!match_binop(op::add, &o17, &o16, i, ctx)) return false;
if(!match_imm_symbol(2, o17, ctx)) return false;
opr* o19, *o18;
if(!match_binop(op::add, &o19, &o18, o16, ctx)) return false;
if(!match_symbol(0, o19, ctx)) return false;
if(!match_imm_symbol(1, o18, ctx)) return false;

ins* it = i;
ins* v14 = write_binop(it, op::add, ctx.symbols[0], ctx.symbols[1].const_val.apply(op::add, ctx.symbols[2].const_val));

i->replace_all_uses_with(v14);
return true;
}

static bool __replace_pattern__20(ins* i, match_context& ctx){
opr* o24, *o23;
if(!match_binop(op::add, &o24, &o23, i, ctx)) return false;
opr* o26, *o25;
if(!match_binop(op::add, &o26, &o25, o24, ctx)) return false;
if(!match_imm_symbol(1, o26, ctx)) return false;
if(!match_symbol(0, o25, ctx)) return false;
if(!match_imm_symbol(2, o23, ctx)) return false;

ins* it = i;
ins* v21 = write_binop(it, op::add, ctx.symbols[0], ctx.symbols[1].const_val.apply(op::add, ctx.symbols[2].const_val));

i->replace_all_uses_with(v21);
return true;
}

static bool __replace_pattern__27(ins* i, match_context& ctx){
opr* o31, *o30;
if(!match_binop(op::add, &o31, &o30, i, ctx)) return false;
if(!match_imm_symbol(2, o31, ctx)) return false;
opr* o33, *o32;
if(!match_binop(op::add, &o33, &o32, o30, ctx)) return false;
if(!match_imm_symbol(1, o33, ctx)) return false;
if(!match_symbol(0, o32, ctx)) return false;

ins* it = i;
ins* v28 = write_binop(it, op::add, ctx.symbols[0], ctx.symbols[1].const_val.apply(op::add, ctx.symbols[2].const_val));

i->replace_all_uses_with(v28);
return true;
}

static bool __replace_pattern__34(ins* i, match_context& ctx){
opr* o38, *o37;
if(!match_binop(op::sub, &o38, &o37, i, ctx)) return false;
opr* o40, *o39;
if(!match_binop(op::add, &o40, &o39, o38, ctx)) return false;
if(!match_symbol(0, o40, ctx)) return false;
if(!match_imm_symbol(1, o39, ctx)) return false;
if(!match_imm_symbol(2, o37, ctx)) return false;

ins* it = i;
ins* v35 = write_binop(it, op::add, ctx.symbols[0], ctx.symbols[1].const_val.apply(op::sub, ctx.symbols[2].const_val));

i->replace_all_uses_with(v35);
return true;
}

static bool __replace_pattern__41(ins* i, match_context& ctx){
opr* o45, *o44;
if(!match_binop(op::sub, &o45, &o44, i, ctx)) return false;
opr* o47, *o46;
if(!match_binop(op::add, &o47, &o46, o45, ctx)) return false;
if(!match_imm_symbol(1, o47, ctx)) return false;
if(!match_symbol(0, o46, ctx)) return false;
if(!match_imm_symbol(2, o44, ctx)) return false;

ins* it = i;
ins* v42 = write_binop(it, op::add, ctx.symbols[0], ctx.symbols[1].const_val.apply(op::sub, ctx.symbols[2].const_val));

i->replace_all_uses_with(v42);
return true;
}

static bool __replace_pattern__48(ins* i, match_context& ctx){
opr* o52, *o51;
if(!match_binop(op::mul, &o52, &o51, i, ctx)) return false;
opr* o54, *o53;
if(!match_binop(op::mul, &o54, &o53, o52, ctx)) return false;
if(!match_symbol(0, o54, ctx)) return false;
if(!match_imm_symbol(1, o53, ctx)) return false;
if(!match_imm_symbol(2, o51, ctx)) return false;

ins* it = i;
ins* v49 = write_binop(it, op::mul, ctx.symbols[0], ctx.symbols[1].const_val.apply(op::mul, ctx.symbols[2].const_val));

i->replace_all_uses_with(v49);
return true;
}

static bool __replace_pattern__55(ins* i, match_context& ctx){
opr* o59, *o58;
if(!match_binop(op::mul, &o59, &o58, i, ctx)) return false;
if(!match_imm_symbol(2, o59, ctx)) return false;
opr* o61, *o60;
if(!match_binop(op::mul, &o61, &o60, o58, ctx)) return false;
if(!match_symbol(0, o61, ctx)) return false;
if(!match_imm_symbol(1, o60, ctx)) return false;

ins* it = i;
ins* v56 = write_binop(it, op::mul, ctx.symbols[0], ctx.symbols[1].const_val.apply(op::mul, ctx.symbols[2].const_val));

i->replace_all_uses_with(v56);
return true;
}

static bool __replace_pattern__62(ins* i, match_context& ctx){
opr* o66, *o65;
if(!match_binop(op::mul, &o66, &o65, i, ctx)) return false;
opr* o68, *o67;
if(!match_binop(op::mul, &o68, &o67, o66, ctx)) return false;
if(!match_imm_symbol(1, o68, ctx)) return false;
if(!match_symbol(0, o67, ctx)) return false;
if(!match_imm_symbol(2, o65, ctx)) return false;

ins* it = i;
ins* v63 = write_binop(it, op::mul, ctx.symbols[0], ctx.symbols[1].const_val.apply(op::mul, ctx.symbols[2].const_val));

i->replace_all_uses_with(v63);
return true;
}

static bool __replace_pattern__69(ins* i, match_context& ctx){
opr* o73, *o72;
if(!match_binop(op::mul, &o73, &o72, i, ctx)) return false;
if(!match_imm_symbol(2, o73, ctx)) return false;
opr* o75, *o74;
if(!match_binop(op::mul, &o75, &o74, o72, ctx)) return false;
if(!match_imm_symbol(1, o75, ctx)) return false;
if(!match_symbol(0, o74, ctx)) return false;

ins* it = i;
ins* v70 = write_binop(it, op::mul, ctx.symbols[0], ctx.symbols[1].const_val.apply(op::mul, ctx.symbols[2].const_val));

i->replace_all_uses_with(v70);
return true;
}

static bool __replace_pattern__76(ins* i, match_context& ctx){
opr* o80, *o79;
if(!match_binop(op::mul, &o80, &o79, i, ctx)) return false;
opr*o81;
if(!match_unop(op::neg, &o81, o80, ctx)) return false;
if(!match_symbol(0, o81, ctx)) return false;
if(!match_imm_symbol(1, o79, ctx)) return false;

ins* it = i;
ins* v77 = write_binop(it, op::mul, ctx.symbols[0], ctx.symbols[1].const_val.apply(op::neg));

i->replace_all_uses_with(v77);
return true;
}

static bool __replace_pattern__82(ins* i, match_context& ctx){
opr* o86, *o85;
if(!match_binop(op::mul, &o86, &o85, i, ctx)) return false;
if(!match_imm_symbol(1, o86, ctx)) return false;
opr*o87;
if(!match_unop(op::neg, &o87, o85, ctx)) return false;
if(!match_symbol(0, o87, ctx)) return false;

ins* it = i;
ins* v83 = write_binop(it, op::mul, ctx.symbols[0], ctx.symbols[1].const_val.apply(op::neg));

i->replace_all_uses_with(v83);
return true;
}

static bool __replace_pattern__88(ins* i, match_context& ctx){
opr* o93, *o92;
if(!match_binop(op::mul, &o93, &o92, i, ctx)) return false;
opr* o95, *o94;
if(!match_binop(op::add, &o95, &o94, o93, ctx)) return false;
if(!match_symbol(0, o95, ctx)) return false;
if(!match_imm_symbol(1, o94, ctx)) return false;
if(!match_imm_symbol(2, o92, ctx)) return false;

ins* it = i;
ins* v90 = write_binop(it, op::mul, ctx.symbols[0], ctx.symbols[1].const_val);
ins* v89 = write_binop(it, op::add, v90, ctx.symbols[1].const_val.apply(op::mul, ctx.symbols[2].const_val));

i->replace_all_uses_with(v89);
return true;
}

static bool __replace_pattern__96(ins* i, match_context& ctx){
opr* o101, *o100;
if(!match_binop(op::mul, &o101, &o100, i, ctx)) return false;
if(!match_imm_symbol(2, o101, ctx)) return false;
opr* o103, *o102;
if(!match_binop(op::add, &o103, &o102, o100, ctx)) return false;
if(!match_symbol(0, o103, ctx)) return false;
if(!match_imm_symbol(1, o102, ctx)) return false;

ins* it = i;
ins* v98 = write_binop(it, op::mul, ctx.symbols[0], ctx.symbols[1].const_val);
ins* v97 = write_binop(it, op::add, v98, ctx.symbols[1].const_val.apply(op::mul, ctx.symbols[2].const_val));

i->replace_all_uses_with(v97);
return true;
}

static bool __replace_pattern__104(ins* i, match_context& ctx){
opr* o109, *o108;
if(!match_binop(op::mul, &o109, &o108, i, ctx)) return false;
opr* o111, *o110;
if(!match_binop(op::add, &o111, &o110, o109, ctx)) return false;
if(!match_imm_symbol(1, o111, ctx)) return false;
if(!match_symbol(0, o110, ctx)) return false;
if(!match_imm_symbol(2, o108, ctx)) return false;

ins* it = i;
ins* v106 = write_binop(it, op::mul, ctx.symbols[0], ctx.symbols[1].const_val);
ins* v105 = write_binop(it, op::add, v106, ctx.symbols[1].const_val.apply(op::mul, ctx.symbols[2].const_val));

i->replace_all_uses_with(v105);
return true;
}

static bool __replace_pattern__112(ins* i, match_context& ctx){
opr* o117, *o116;
if(!match_binop(op::mul, &o117, &o116, i, ctx)) return false;
if(!match_imm_symbol(2, o117, ctx)) return false;
opr* o119, *o118;
if(!match_binop(op::add, &o119, &o118, o116, ctx)) return false;
if(!match_imm_symbol(1, o119, ctx)) return false;
if(!match_symbol(0, o118, ctx)) return false;

ins* it = i;
ins* v114 = write_binop(it, op::mul, ctx.symbols[0], ctx.symbols[1].const_val);
ins* v113 = write_binop(it, op::add, v114, ctx.symbols[1].const_val.apply(op::mul, ctx.symbols[2].const_val));

i->replace_all_uses_with(v113);
return true;
}

static bool __replace_pattern__120(ins* i, match_context& ctx){
opr* o125, *o124;
if(!match_binop(op::mul, &o125, &o124, i, ctx)) return false;
opr* o127, *o126;
if(!match_binop(op::sub, &o127, &o126, o125, ctx)) return false;
if(!match_symbol(0, o127, ctx)) return false;
if(!match_imm_symbol(1, o126, ctx)) return false;
if(!match_imm_symbol(2, o124, ctx)) return false;

ins* it = i;
ins* v122 = write_binop(it, op::mul, ctx.symbols[0], ctx.symbols[1].const_val);
ins* v121 = write_binop(it, op::sub, v122, ctx.symbols[1].const_val.apply(op::mul, ctx.symbols[2].const_val));

i->replace_all_uses_with(v121);
return true;
}

static bool __replace_pattern__128(ins* i, match_context& ctx){
opr* o133, *o132;
if(!match_binop(op::mul, &o133, &o132, i, ctx)) return false;
if(!match_imm_symbol(2, o133, ctx)) return false;
opr* o135, *o134;
if(!match_binop(op::sub, &o135, &o134, o132, ctx)) return false;
if(!match_symbol(0, o135, ctx)) return false;
if(!match_imm_symbol(1, o134, ctx)) return false;

ins* it = i;
ins* v130 = write_binop(it, op::mul, ctx.symbols[0], ctx.symbols[1].const_val);
ins* v129 = write_binop(it, op::sub, v130, ctx.symbols[1].const_val.apply(op::mul, ctx.symbols[2].const_val));

i->replace_all_uses_with(v129);
return true;
}

static bool __replace_pattern__136(ins* i, match_context& ctx){
opr* o140, *o139;
if(!match_binop(op::eq, &o140, &o139, i, ctx)) return false;
opr* o142, *o141;
if(!match_binop(op::sub, &o142, &o141, o140, ctx)) return false;
if(!match_symbol(0, o142, ctx)) return false;
if(!match_imm_symbol(1, o141, ctx)) return false;
if(!match_imm_symbol(2, o139, ctx)) return false;

ins* it = i;
ins* v137 = write_cmp(it, op::eq, ctx.symbols[0], ctx.symbols[1].const_val.apply(op::add, ctx.symbols[2].const_val));

i->replace_all_uses_with(v137);
return true;
}

static bool __replace_pattern__143(ins* i, match_context& ctx){
opr* o147, *o146;
if(!match_binop(op::eq, &o147, &o146, i, ctx)) return false;
opr* o149, *o148;
if(!match_binop(op::add, &o149, &o148, o147, ctx)) return false;
if(!match_symbol(0, o149, ctx)) return false;
if(!match_imm_symbol(1, o148, ctx)) return false;
if(!match_imm_symbol(2, o146, ctx)) return false;

ins* it = i;
ins* v144 = write_cmp(it, op::eq, ctx.symbols[0], ctx.symbols[2].const_val.apply(op::sub, ctx.symbols[1].const_val));

i->replace_all_uses_with(v144);
return true;
}

static bool __replace_pattern__150(ins* i, match_context& ctx){
opr* o154, *o153;
if(!match_binop(op::eq, &o154, &o153, i, ctx)) return false;
opr* o156, *o155;
if(!match_binop(op::add, &o156, &o155, o154, ctx)) return false;
if(!match_imm_symbol(1, o156, ctx)) return false;
if(!match_symbol(0, o155, ctx)) return false;
if(!match_imm_symbol(2, o153, ctx)) return false;

ins* it = i;
ins* v151 = write_cmp(it, op::eq, ctx.symbols[0], ctx.symbols[2].const_val.apply(op::sub, ctx.symbols[1].const_val));

i->replace_all_uses_with(v151);
return true;
}

static bool __replace_pattern__157(ins* i, match_context& ctx){
opr* o161, *o160;
if(!match_binop(op::ne, &o161, &o160, i, ctx)) return false;
opr* o163, *o162;
if(!match_binop(op::sub, &o163, &o162, o161, ctx)) return false;
if(!match_symbol(0, o163, ctx)) return false;
if(!match_imm_symbol(1, o162, ctx)) return false;
if(!match_imm_symbol(2, o160, ctx)) return false;

ins* it = i;
ins* v158 = write_cmp(it, op::ne, ctx.symbols[0], ctx.symbols[1].const_val.apply(op::add, ctx.symbols[2].const_val));

i->replace_all_uses_with(v158);
return true;
}

static bool __replace_pattern__164(ins* i, match_context& ctx){
opr* o168, *o167;
if(!match_binop(op::ne, &o168, &o167, i, ctx)) return false;
opr* o170, *o169;
if(!match_binop(op::add, &o170, &o169, o168, ctx)) return false;
if(!match_symbol(0, o170, ctx)) return false;
if(!match_imm_symbol(1, o169, ctx)) return false;
if(!match_imm_symbol(2, o167, ctx)) return false;

ins* it = i;
ins* v165 = write_cmp(it, op::ne, ctx.symbols[0], ctx.symbols[2].const_val.apply(op::sub, ctx.symbols[1].const_val));

i->replace_all_uses_with(v165);
return true;
}

static bool __replace_pattern__171(ins* i, match_context& ctx){
opr* o175, *o174;
if(!match_binop(op::ne, &o175, &o174, i, ctx)) return false;
opr* o177, *o176;
if(!match_binop(op::add, &o177, &o176, o175, ctx)) return false;
if(!match_imm_symbol(1, o177, ctx)) return false;
if(!match_symbol(0, o176, ctx)) return false;
if(!match_imm_symbol(2, o174, ctx)) return false;

ins* it = i;
ins* v172 = write_cmp(it, op::ne, ctx.symbols[0], ctx.symbols[2].const_val.apply(op::sub, ctx.symbols[1].const_val));

i->replace_all_uses_with(v172);
return true;
}

static bool __replace_pattern__178(ins* i, match_context& ctx){
opr*o179;
if(!match_unop(op::neg, &o179, i, ctx)) return false;
opr*o180;
if(!match_unop(op::neg, &o180, o179, ctx)) return false;
if(!match_symbol(0, o180, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__181(ins* i, match_context& ctx){
opr*o182;
if(!match_unop(op::bit_not, &o182, i, ctx)) return false;
opr*o183;
if(!match_unop(op::bit_not, &o183, o182, ctx)) return false;
if(!match_symbol(0, o183, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__184(ins* i, match_context& ctx){
opr*o186;
if(!match_unop(op::bit_not, &o186, i, ctx)) return false;
opr*o187;
if(!match_unop(op::neg, &o187, o186, ctx)) return false;
if(!match_symbol(0, o187, ctx)) return false;

ins* it = i;
ins* v185 = write_binop(it, op::sub, ctx.symbols[0], imm(i->get_type(), 1));

i->replace_all_uses_with(v185);
return true;
}

static bool __replace_pattern__188(ins* i, match_context& ctx){
opr*o190;
if(!match_unop(op::neg, &o190, i, ctx)) return false;
opr*o191;
if(!match_unop(op::bit_not, &o191, o190, ctx)) return false;
if(!match_symbol(0, o191, ctx)) return false;

ins* it = i;
ins* v189 = write_binop(it, op::add, ctx.symbols[0], imm(i->get_type(), 1));

i->replace_all_uses_with(v189);
return true;
}

static bool __replace_pattern__192(ins* i, match_context& ctx){
opr* o195, *o194;
if(!match_binop(op::mul, &o195, &o194, i, ctx)) return false;
if(!match_symbol(0, o195, ctx)) return false;
if(!match_imm(-1, o194, ctx)) return false;

ins* it = i;
ins* v193 = write_unop(it, op::neg, ctx.symbols[0]);

i->replace_all_uses_with(v193);
return true;
}

static bool __replace_pattern__196(ins* i, match_context& ctx){
opr* o199, *o198;
if(!match_binop(op::mul, &o199, &o198, i, ctx)) return false;
if(!match_imm(-1, o199, ctx)) return false;
if(!match_symbol(0, o198, ctx)) return false;

ins* it = i;
ins* v197 = write_unop(it, op::neg, ctx.symbols[0]);

i->replace_all_uses_with(v197);
return true;
}

static bool __replace_pattern__200(ins* i, match_context& ctx){
opr* o203, *o202;
if(!match_binop(op::add, &o203, &o202, i, ctx)) return false;
if(!match_symbol(0, o203, ctx)) return false;
opr*o204;
if(!match_unop(op::neg, &o204, o202, ctx)) return false;
if(!match_symbol(1, o204, ctx)) return false;

ins* it = i;
ins* v201 = write_binop(it, op::sub, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v201);
return true;
}

static bool __replace_pattern__205(ins* i, match_context& ctx){
opr* o208, *o207;
if(!match_binop(op::add, &o208, &o207, i, ctx)) return false;
opr*o209;
if(!match_unop(op::neg, &o209, o208, ctx)) return false;
if(!match_symbol(1, o209, ctx)) return false;
if(!match_symbol(0, o207, ctx)) return false;

ins* it = i;
ins* v206 = write_binop(it, op::sub, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v206);
return true;
}

static bool __replace_pattern__210(ins* i, match_context& ctx){
opr*o212;
if(!match_unop(op::bit_not, &o212, i, ctx)) return false;
opr* o214, *o213;
if(!match_binop(op::sub, &o214, &o213, o212, ctx)) return false;
if(!match_symbol(0, o214, ctx)) return false;
if(!match_imm(1, o213, ctx)) return false;

ins* it = i;
ins* v211 = write_unop(it, op::neg, ctx.symbols[0]);

i->replace_all_uses_with(v211);
return true;
}

static bool __replace_pattern__215(ins* i, match_context& ctx){
opr* o218, *o217;
if(!match_binop(op::sub, &o218, &o217, i, ctx)) return false;
if(!match_imm(0, o218, ctx)) return false;
if(!match_symbol(0, o217, ctx)) return false;

ins* it = i;
ins* v216 = write_unop(it, op::neg, ctx.symbols[0]);

i->replace_all_uses_with(v216);
return true;
}

static bool __replace_pattern__219(ins* i, match_context& ctx){
opr* o221, *o220;
if(!match_binop(op::add, &o221, &o220, i, ctx)) return false;
opr* o223, *o222;
if(!match_binop(op::sub, &o223, &o222, o221, ctx)) return false;
if(!match_symbol(0, o223, ctx)) return false;
if(!match_symbol(1, o222, ctx)) return false;
if(!match_symbol(1, o220, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__224(ins* i, match_context& ctx){
opr* o226, *o225;
if(!match_binop(op::add, &o226, &o225, i, ctx)) return false;
if(!match_symbol(1, o226, ctx)) return false;
opr* o228, *o227;
if(!match_binop(op::sub, &o228, &o227, o225, ctx)) return false;
if(!match_symbol(0, o228, ctx)) return false;
if(!match_symbol(1, o227, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__229(ins* i, match_context& ctx){
opr* o232, *o231;
if(!match_binop(op::sub, &o232, &o231, i, ctx)) return false;
if(!match_symbol(0, o232, ctx)) return false;
opr*o233;
if(!match_unop(op::neg, &o233, o231, ctx)) return false;
if(!match_symbol(1, o233, ctx)) return false;

ins* it = i;
ins* v230 = write_binop(it, op::add, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v230);
return true;
}

static bool __replace_pattern__234(ins* i, match_context& ctx){
opr*o236;
if(!match_unop(op::neg, &o236, i, ctx)) return false;
opr* o238, *o237;
if(!match_binop(op::sub, &o238, &o237, o236, ctx)) return false;
if(!match_symbol(0, o238, ctx)) return false;
if(!match_symbol(1, o237, ctx)) return false;

ins* it = i;
ins* v235 = write_binop(it, op::sub, ctx.symbols[1], ctx.symbols[0]);

i->replace_all_uses_with(v235);
return true;
}

static bool __replace_pattern__239(ins* i, match_context& ctx){
opr* o243, *o242;
if(!match_binop(op::mul, &o243, &o242, i, ctx)) return false;
opr*o244;
if(!match_unop(op::neg, &o244, o243, ctx)) return false;
if(!match_symbol(0, o244, ctx)) return false;
if(!match_imm_symbol(1, o242, ctx)) return false;

ins* it = i;
ins* v240 = write_binop(it, op::mul, ctx.symbols[0], ctx.symbols[1].const_val.apply(op::neg));

i->replace_all_uses_with(v240);
return true;
}

static bool __replace_pattern__245(ins* i, match_context& ctx){
opr* o249, *o248;
if(!match_binop(op::mul, &o249, &o248, i, ctx)) return false;
if(!match_imm_symbol(1, o249, ctx)) return false;
opr*o250;
if(!match_unop(op::neg, &o250, o248, ctx)) return false;
if(!match_symbol(0, o250, ctx)) return false;

ins* it = i;
ins* v246 = write_binop(it, op::mul, ctx.symbols[0], ctx.symbols[1].const_val.apply(op::neg));

i->replace_all_uses_with(v246);
return true;
}

static bool __replace_pattern__251(ins* i, match_context& ctx){
opr* o255, *o254;
if(!match_binop(op::bit_or, &o255, &o254, i, ctx)) return false;
opr* o257, *o256;
if(!match_binop(op::bit_and, &o257, &o256, o255, ctx)) return false;
if(!match_symbol(0, o257, ctx)) return false;
if(!match_symbol(1, o256, ctx)) return false;
opr* o259, *o258;
if(!match_binop(op::bit_and, &o259, &o258, o254, ctx)) return false;
if(!match_symbol(0, o259, ctx)) return false;
if(!match_symbol(2, o258, ctx)) return false;

ins* it = i;
ins* v253 = write_binop(it, op::bit_or, ctx.symbols[1], ctx.symbols[2]);
ins* v252 = write_binop(it, op::bit_and, ctx.symbols[0], v253);

i->replace_all_uses_with(v252);
return true;
}

static bool __replace_pattern__260(ins* i, match_context& ctx){
opr* o264, *o263;
if(!match_binop(op::bit_or, &o264, &o263, i, ctx)) return false;
opr* o266, *o265;
if(!match_binop(op::bit_and, &o266, &o265, o264, ctx)) return false;
if(!match_symbol(0, o266, ctx)) return false;
if(!match_symbol(2, o265, ctx)) return false;
opr* o268, *o267;
if(!match_binop(op::bit_and, &o268, &o267, o263, ctx)) return false;
if(!match_symbol(0, o268, ctx)) return false;
if(!match_symbol(1, o267, ctx)) return false;

ins* it = i;
ins* v262 = write_binop(it, op::bit_or, ctx.symbols[1], ctx.symbols[2]);
ins* v261 = write_binop(it, op::bit_and, ctx.symbols[0], v262);

i->replace_all_uses_with(v261);
return true;
}

static bool __replace_pattern__269(ins* i, match_context& ctx){
opr* o273, *o272;
if(!match_binop(op::bit_or, &o273, &o272, i, ctx)) return false;
opr* o275, *o274;
if(!match_binop(op::bit_and, &o275, &o274, o273, ctx)) return false;
if(!match_symbol(0, o275, ctx)) return false;
if(!match_symbol(1, o274, ctx)) return false;
opr* o277, *o276;
if(!match_binop(op::bit_and, &o277, &o276, o272, ctx)) return false;
if(!match_symbol(2, o277, ctx)) return false;
if(!match_symbol(0, o276, ctx)) return false;

ins* it = i;
ins* v271 = write_binop(it, op::bit_or, ctx.symbols[1], ctx.symbols[2]);
ins* v270 = write_binop(it, op::bit_and, ctx.symbols[0], v271);

i->replace_all_uses_with(v270);
return true;
}

static bool __replace_pattern__278(ins* i, match_context& ctx){
opr* o282, *o281;
if(!match_binop(op::bit_or, &o282, &o281, i, ctx)) return false;
opr* o284, *o283;
if(!match_binop(op::bit_and, &o284, &o283, o282, ctx)) return false;
if(!match_symbol(2, o284, ctx)) return false;
if(!match_symbol(0, o283, ctx)) return false;
opr* o286, *o285;
if(!match_binop(op::bit_and, &o286, &o285, o281, ctx)) return false;
if(!match_symbol(0, o286, ctx)) return false;
if(!match_symbol(1, o285, ctx)) return false;

ins* it = i;
ins* v280 = write_binop(it, op::bit_or, ctx.symbols[1], ctx.symbols[2]);
ins* v279 = write_binop(it, op::bit_and, ctx.symbols[0], v280);

i->replace_all_uses_with(v279);
return true;
}

static bool __replace_pattern__287(ins* i, match_context& ctx){
opr* o291, *o290;
if(!match_binop(op::bit_or, &o291, &o290, i, ctx)) return false;
opr* o293, *o292;
if(!match_binop(op::bit_and, &o293, &o292, o291, ctx)) return false;
if(!match_symbol(1, o293, ctx)) return false;
if(!match_symbol(0, o292, ctx)) return false;
opr* o295, *o294;
if(!match_binop(op::bit_and, &o295, &o294, o290, ctx)) return false;
if(!match_symbol(0, o295, ctx)) return false;
if(!match_symbol(2, o294, ctx)) return false;

ins* it = i;
ins* v289 = write_binop(it, op::bit_or, ctx.symbols[1], ctx.symbols[2]);
ins* v288 = write_binop(it, op::bit_and, ctx.symbols[0], v289);

i->replace_all_uses_with(v288);
return true;
}

static bool __replace_pattern__296(ins* i, match_context& ctx){
opr* o300, *o299;
if(!match_binop(op::bit_or, &o300, &o299, i, ctx)) return false;
opr* o302, *o301;
if(!match_binop(op::bit_and, &o302, &o301, o300, ctx)) return false;
if(!match_symbol(0, o302, ctx)) return false;
if(!match_symbol(2, o301, ctx)) return false;
opr* o304, *o303;
if(!match_binop(op::bit_and, &o304, &o303, o299, ctx)) return false;
if(!match_symbol(1, o304, ctx)) return false;
if(!match_symbol(0, o303, ctx)) return false;

ins* it = i;
ins* v298 = write_binop(it, op::bit_or, ctx.symbols[1], ctx.symbols[2]);
ins* v297 = write_binop(it, op::bit_and, ctx.symbols[0], v298);

i->replace_all_uses_with(v297);
return true;
}

static bool __replace_pattern__305(ins* i, match_context& ctx){
opr* o309, *o308;
if(!match_binop(op::bit_or, &o309, &o308, i, ctx)) return false;
opr* o311, *o310;
if(!match_binop(op::bit_and, &o311, &o310, o309, ctx)) return false;
if(!match_symbol(1, o311, ctx)) return false;
if(!match_symbol(0, o310, ctx)) return false;
opr* o313, *o312;
if(!match_binop(op::bit_and, &o313, &o312, o308, ctx)) return false;
if(!match_symbol(2, o313, ctx)) return false;
if(!match_symbol(0, o312, ctx)) return false;

ins* it = i;
ins* v307 = write_binop(it, op::bit_or, ctx.symbols[1], ctx.symbols[2]);
ins* v306 = write_binop(it, op::bit_and, ctx.symbols[0], v307);

i->replace_all_uses_with(v306);
return true;
}

static bool __replace_pattern__314(ins* i, match_context& ctx){
opr* o318, *o317;
if(!match_binop(op::bit_or, &o318, &o317, i, ctx)) return false;
opr* o320, *o319;
if(!match_binop(op::bit_and, &o320, &o319, o318, ctx)) return false;
if(!match_symbol(2, o320, ctx)) return false;
if(!match_symbol(0, o319, ctx)) return false;
opr* o322, *o321;
if(!match_binop(op::bit_and, &o322, &o321, o317, ctx)) return false;
if(!match_symbol(1, o322, ctx)) return false;
if(!match_symbol(0, o321, ctx)) return false;

ins* it = i;
ins* v316 = write_binop(it, op::bit_or, ctx.symbols[1], ctx.symbols[2]);
ins* v315 = write_binop(it, op::bit_and, ctx.symbols[0], v316);

i->replace_all_uses_with(v315);
return true;
}

static bool __replace_pattern__323(ins* i, match_context& ctx){
opr* o327, *o326;
if(!match_binop(op::bit_and, &o327, &o326, i, ctx)) return false;
opr* o329, *o328;
if(!match_binop(op::bit_or, &o329, &o328, o327, ctx)) return false;
if(!match_symbol(0, o329, ctx)) return false;
if(!match_symbol(1, o328, ctx)) return false;
opr* o331, *o330;
if(!match_binop(op::bit_or, &o331, &o330, o326, ctx)) return false;
if(!match_symbol(0, o331, ctx)) return false;
if(!match_symbol(2, o330, ctx)) return false;

ins* it = i;
ins* v325 = write_binop(it, op::bit_and, ctx.symbols[1], ctx.symbols[2]);
ins* v324 = write_binop(it, op::bit_or, ctx.symbols[0], v325);

i->replace_all_uses_with(v324);
return true;
}

static bool __replace_pattern__332(ins* i, match_context& ctx){
opr* o336, *o335;
if(!match_binop(op::bit_and, &o336, &o335, i, ctx)) return false;
opr* o338, *o337;
if(!match_binop(op::bit_or, &o338, &o337, o336, ctx)) return false;
if(!match_symbol(0, o338, ctx)) return false;
if(!match_symbol(2, o337, ctx)) return false;
opr* o340, *o339;
if(!match_binop(op::bit_or, &o340, &o339, o335, ctx)) return false;
if(!match_symbol(0, o340, ctx)) return false;
if(!match_symbol(1, o339, ctx)) return false;

ins* it = i;
ins* v334 = write_binop(it, op::bit_and, ctx.symbols[1], ctx.symbols[2]);
ins* v333 = write_binop(it, op::bit_or, ctx.symbols[0], v334);

i->replace_all_uses_with(v333);
return true;
}

static bool __replace_pattern__341(ins* i, match_context& ctx){
opr* o345, *o344;
if(!match_binop(op::bit_and, &o345, &o344, i, ctx)) return false;
opr* o347, *o346;
if(!match_binop(op::bit_or, &o347, &o346, o345, ctx)) return false;
if(!match_symbol(0, o347, ctx)) return false;
if(!match_symbol(1, o346, ctx)) return false;
opr* o349, *o348;
if(!match_binop(op::bit_or, &o349, &o348, o344, ctx)) return false;
if(!match_symbol(2, o349, ctx)) return false;
if(!match_symbol(0, o348, ctx)) return false;

ins* it = i;
ins* v343 = write_binop(it, op::bit_and, ctx.symbols[1], ctx.symbols[2]);
ins* v342 = write_binop(it, op::bit_or, ctx.symbols[0], v343);

i->replace_all_uses_with(v342);
return true;
}

static bool __replace_pattern__350(ins* i, match_context& ctx){
opr* o354, *o353;
if(!match_binop(op::bit_and, &o354, &o353, i, ctx)) return false;
opr* o356, *o355;
if(!match_binop(op::bit_or, &o356, &o355, o354, ctx)) return false;
if(!match_symbol(2, o356, ctx)) return false;
if(!match_symbol(0, o355, ctx)) return false;
opr* o358, *o357;
if(!match_binop(op::bit_or, &o358, &o357, o353, ctx)) return false;
if(!match_symbol(0, o358, ctx)) return false;
if(!match_symbol(1, o357, ctx)) return false;

ins* it = i;
ins* v352 = write_binop(it, op::bit_and, ctx.symbols[1], ctx.symbols[2]);
ins* v351 = write_binop(it, op::bit_or, ctx.symbols[0], v352);

i->replace_all_uses_with(v351);
return true;
}

static bool __replace_pattern__359(ins* i, match_context& ctx){
opr* o363, *o362;
if(!match_binop(op::bit_and, &o363, &o362, i, ctx)) return false;
opr* o365, *o364;
if(!match_binop(op::bit_or, &o365, &o364, o363, ctx)) return false;
if(!match_symbol(1, o365, ctx)) return false;
if(!match_symbol(0, o364, ctx)) return false;
opr* o367, *o366;
if(!match_binop(op::bit_or, &o367, &o366, o362, ctx)) return false;
if(!match_symbol(0, o367, ctx)) return false;
if(!match_symbol(2, o366, ctx)) return false;

ins* it = i;
ins* v361 = write_binop(it, op::bit_and, ctx.symbols[1], ctx.symbols[2]);
ins* v360 = write_binop(it, op::bit_or, ctx.symbols[0], v361);

i->replace_all_uses_with(v360);
return true;
}

static bool __replace_pattern__368(ins* i, match_context& ctx){
opr* o372, *o371;
if(!match_binop(op::bit_and, &o372, &o371, i, ctx)) return false;
opr* o374, *o373;
if(!match_binop(op::bit_or, &o374, &o373, o372, ctx)) return false;
if(!match_symbol(0, o374, ctx)) return false;
if(!match_symbol(2, o373, ctx)) return false;
opr* o376, *o375;
if(!match_binop(op::bit_or, &o376, &o375, o371, ctx)) return false;
if(!match_symbol(1, o376, ctx)) return false;
if(!match_symbol(0, o375, ctx)) return false;

ins* it = i;
ins* v370 = write_binop(it, op::bit_and, ctx.symbols[1], ctx.symbols[2]);
ins* v369 = write_binop(it, op::bit_or, ctx.symbols[0], v370);

i->replace_all_uses_with(v369);
return true;
}

static bool __replace_pattern__377(ins* i, match_context& ctx){
opr* o381, *o380;
if(!match_binop(op::bit_and, &o381, &o380, i, ctx)) return false;
opr* o383, *o382;
if(!match_binop(op::bit_or, &o383, &o382, o381, ctx)) return false;
if(!match_symbol(1, o383, ctx)) return false;
if(!match_symbol(0, o382, ctx)) return false;
opr* o385, *o384;
if(!match_binop(op::bit_or, &o385, &o384, o380, ctx)) return false;
if(!match_symbol(2, o385, ctx)) return false;
if(!match_symbol(0, o384, ctx)) return false;

ins* it = i;
ins* v379 = write_binop(it, op::bit_and, ctx.symbols[1], ctx.symbols[2]);
ins* v378 = write_binop(it, op::bit_or, ctx.symbols[0], v379);

i->replace_all_uses_with(v378);
return true;
}

static bool __replace_pattern__386(ins* i, match_context& ctx){
opr* o390, *o389;
if(!match_binop(op::bit_and, &o390, &o389, i, ctx)) return false;
opr* o392, *o391;
if(!match_binop(op::bit_or, &o392, &o391, o390, ctx)) return false;
if(!match_symbol(2, o392, ctx)) return false;
if(!match_symbol(0, o391, ctx)) return false;
opr* o394, *o393;
if(!match_binop(op::bit_or, &o394, &o393, o389, ctx)) return false;
if(!match_symbol(1, o394, ctx)) return false;
if(!match_symbol(0, o393, ctx)) return false;

ins* it = i;
ins* v388 = write_binop(it, op::bit_and, ctx.symbols[1], ctx.symbols[2]);
ins* v387 = write_binop(it, op::bit_or, ctx.symbols[0], v388);

i->replace_all_uses_with(v387);
return true;
}

static bool __replace_pattern__395(ins* i, match_context& ctx){
opr* o398, *o397;
if(!match_binop(op::bit_or, &o398, &o397, i, ctx)) return false;
opr* o400, *o399;
if(!match_binop(op::lt, &o400, &o399, o398, ctx)) return false;
if(!match_symbol(0, o400, ctx)) return false;
if(!match_symbol(1, o399, ctx)) return false;
opr* o402, *o401;
if(!match_binop(op::eq, &o402, &o401, o397, ctx)) return false;
if(!match_symbol(0, o402, ctx)) return false;
if(!match_symbol(1, o401, ctx)) return false;

ins* it = i;
ins* v396 = write_cmp(it, op::le, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v396);
return true;
}

static bool __replace_pattern__403(ins* i, match_context& ctx){
opr* o406, *o405;
if(!match_binop(op::bit_or, &o406, &o405, i, ctx)) return false;
opr* o408, *o407;
if(!match_binop(op::eq, &o408, &o407, o406, ctx)) return false;
if(!match_symbol(0, o408, ctx)) return false;
if(!match_symbol(1, o407, ctx)) return false;
opr* o410, *o409;
if(!match_binop(op::lt, &o410, &o409, o405, ctx)) return false;
if(!match_symbol(0, o410, ctx)) return false;
if(!match_symbol(1, o409, ctx)) return false;

ins* it = i;
ins* v404 = write_cmp(it, op::le, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v404);
return true;
}

static bool __replace_pattern__411(ins* i, match_context& ctx){
opr* o414, *o413;
if(!match_binop(op::bit_or, &o414, &o413, i, ctx)) return false;
opr* o416, *o415;
if(!match_binop(op::gt, &o416, &o415, o414, ctx)) return false;
if(!match_symbol(0, o416, ctx)) return false;
if(!match_symbol(1, o415, ctx)) return false;
opr* o418, *o417;
if(!match_binop(op::eq, &o418, &o417, o413, ctx)) return false;
if(!match_symbol(0, o418, ctx)) return false;
if(!match_symbol(1, o417, ctx)) return false;

ins* it = i;
ins* v412 = write_cmp(it, op::ge, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v412);
return true;
}

static bool __replace_pattern__419(ins* i, match_context& ctx){
opr* o422, *o421;
if(!match_binop(op::bit_or, &o422, &o421, i, ctx)) return false;
opr* o424, *o423;
if(!match_binop(op::eq, &o424, &o423, o422, ctx)) return false;
if(!match_symbol(0, o424, ctx)) return false;
if(!match_symbol(1, o423, ctx)) return false;
opr* o426, *o425;
if(!match_binop(op::gt, &o426, &o425, o421, ctx)) return false;
if(!match_symbol(0, o426, ctx)) return false;
if(!match_symbol(1, o425, ctx)) return false;

ins* it = i;
ins* v420 = write_cmp(it, op::ge, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v420);
return true;
}

static bool __replace_pattern__427(ins* i, match_context& ctx){
opr* o430, *o429;
if(!match_binop(op::bit_or, &o430, &o429, i, ctx)) return false;
opr* o432, *o431;
if(!match_binop(op::ult, &o432, &o431, o430, ctx)) return false;
if(!match_symbol(0, o432, ctx)) return false;
if(!match_symbol(1, o431, ctx)) return false;
opr* o434, *o433;
if(!match_binop(op::eq, &o434, &o433, o429, ctx)) return false;
if(!match_symbol(0, o434, ctx)) return false;
if(!match_symbol(1, o433, ctx)) return false;

ins* it = i;
ins* v428 = write_cmp(it, op::ule, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v428);
return true;
}

static bool __replace_pattern__435(ins* i, match_context& ctx){
opr* o438, *o437;
if(!match_binop(op::bit_or, &o438, &o437, i, ctx)) return false;
opr* o440, *o439;
if(!match_binop(op::eq, &o440, &o439, o438, ctx)) return false;
if(!match_symbol(0, o440, ctx)) return false;
if(!match_symbol(1, o439, ctx)) return false;
opr* o442, *o441;
if(!match_binop(op::ult, &o442, &o441, o437, ctx)) return false;
if(!match_symbol(0, o442, ctx)) return false;
if(!match_symbol(1, o441, ctx)) return false;

ins* it = i;
ins* v436 = write_cmp(it, op::ule, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v436);
return true;
}

static bool __replace_pattern__443(ins* i, match_context& ctx){
opr* o446, *o445;
if(!match_binop(op::bit_or, &o446, &o445, i, ctx)) return false;
opr* o448, *o447;
if(!match_binop(op::ugt, &o448, &o447, o446, ctx)) return false;
if(!match_symbol(0, o448, ctx)) return false;
if(!match_symbol(1, o447, ctx)) return false;
opr* o450, *o449;
if(!match_binop(op::eq, &o450, &o449, o445, ctx)) return false;
if(!match_symbol(0, o450, ctx)) return false;
if(!match_symbol(1, o449, ctx)) return false;

ins* it = i;
ins* v444 = write_cmp(it, op::uge, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v444);
return true;
}

static bool __replace_pattern__451(ins* i, match_context& ctx){
opr* o454, *o453;
if(!match_binop(op::bit_or, &o454, &o453, i, ctx)) return false;
opr* o456, *o455;
if(!match_binop(op::eq, &o456, &o455, o454, ctx)) return false;
if(!match_symbol(0, o456, ctx)) return false;
if(!match_symbol(1, o455, ctx)) return false;
opr* o458, *o457;
if(!match_binop(op::ugt, &o458, &o457, o453, ctx)) return false;
if(!match_symbol(0, o458, ctx)) return false;
if(!match_symbol(1, o457, ctx)) return false;

ins* it = i;
ins* v452 = write_cmp(it, op::uge, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v452);
return true;
}

static bool __replace_pattern__459(ins* i, match_context& ctx){
opr* o461, *o460;
if(!match_binop(op::bit_and, &o461, &o460, i, ctx)) return false;
if(!match_symbol(0, o461, ctx)) return false;
opr* o463, *o462;
if(!match_binop(op::bit_or, &o463, &o462, o460, ctx)) return false;
if(!match_symbol(0, o463, ctx)) return false;
if(!match_symbol(1, o462, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__464(ins* i, match_context& ctx){
opr* o466, *o465;
if(!match_binop(op::bit_and, &o466, &o465, i, ctx)) return false;
opr* o468, *o467;
if(!match_binop(op::bit_or, &o468, &o467, o466, ctx)) return false;
if(!match_symbol(0, o468, ctx)) return false;
if(!match_symbol(1, o467, ctx)) return false;
if(!match_symbol(0, o465, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__469(ins* i, match_context& ctx){
opr* o471, *o470;
if(!match_binop(op::bit_and, &o471, &o470, i, ctx)) return false;
if(!match_symbol(0, o471, ctx)) return false;
opr* o473, *o472;
if(!match_binop(op::bit_or, &o473, &o472, o470, ctx)) return false;
if(!match_symbol(1, o473, ctx)) return false;
if(!match_symbol(0, o472, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__474(ins* i, match_context& ctx){
opr* o476, *o475;
if(!match_binop(op::bit_and, &o476, &o475, i, ctx)) return false;
opr* o478, *o477;
if(!match_binop(op::bit_or, &o478, &o477, o476, ctx)) return false;
if(!match_symbol(1, o478, ctx)) return false;
if(!match_symbol(0, o477, ctx)) return false;
if(!match_symbol(0, o475, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__479(ins* i, match_context& ctx){
opr* o481, *o480;
if(!match_binop(op::bit_or, &o481, &o480, i, ctx)) return false;
if(!match_symbol(0, o481, ctx)) return false;
opr* o483, *o482;
if(!match_binop(op::bit_and, &o483, &o482, o480, ctx)) return false;
if(!match_symbol(0, o483, ctx)) return false;
if(!match_symbol(1, o482, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__484(ins* i, match_context& ctx){
opr* o486, *o485;
if(!match_binop(op::bit_or, &o486, &o485, i, ctx)) return false;
opr* o488, *o487;
if(!match_binop(op::bit_and, &o488, &o487, o486, ctx)) return false;
if(!match_symbol(0, o488, ctx)) return false;
if(!match_symbol(1, o487, ctx)) return false;
if(!match_symbol(0, o485, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__489(ins* i, match_context& ctx){
opr* o491, *o490;
if(!match_binop(op::bit_or, &o491, &o490, i, ctx)) return false;
if(!match_symbol(0, o491, ctx)) return false;
opr* o493, *o492;
if(!match_binop(op::bit_and, &o493, &o492, o490, ctx)) return false;
if(!match_symbol(1, o493, ctx)) return false;
if(!match_symbol(0, o492, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__494(ins* i, match_context& ctx){
opr* o496, *o495;
if(!match_binop(op::bit_or, &o496, &o495, i, ctx)) return false;
opr* o498, *o497;
if(!match_binop(op::bit_and, &o498, &o497, o496, ctx)) return false;
if(!match_symbol(1, o498, ctx)) return false;
if(!match_symbol(0, o497, ctx)) return false;
if(!match_symbol(0, o495, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__499(ins* i, match_context& ctx){
opr* o503, *o502;
if(!match_binop(op::bit_xor, &o503, &o502, i, ctx)) return false;
if(!match_symbol(0, o503, ctx)) return false;
opr* o505, *o504;
if(!match_binop(op::bit_and, &o505, &o504, o502, ctx)) return false;
if(!match_symbol(0, o505, ctx)) return false;
if(!match_symbol(1, o504, ctx)) return false;

ins* it = i;
ins* v501 = write_unop(it, op::bit_not, ctx.symbols[1]);
ins* v500 = write_binop(it, op::bit_and, ctx.symbols[0], v501);

i->replace_all_uses_with(v500);
return true;
}

static bool __replace_pattern__506(ins* i, match_context& ctx){
opr* o510, *o509;
if(!match_binop(op::bit_xor, &o510, &o509, i, ctx)) return false;
opr* o512, *o511;
if(!match_binop(op::bit_and, &o512, &o511, o510, ctx)) return false;
if(!match_symbol(0, o512, ctx)) return false;
if(!match_symbol(1, o511, ctx)) return false;
if(!match_symbol(0, o509, ctx)) return false;

ins* it = i;
ins* v508 = write_unop(it, op::bit_not, ctx.symbols[1]);
ins* v507 = write_binop(it, op::bit_and, ctx.symbols[0], v508);

i->replace_all_uses_with(v507);
return true;
}

static bool __replace_pattern__513(ins* i, match_context& ctx){
opr* o517, *o516;
if(!match_binop(op::bit_xor, &o517, &o516, i, ctx)) return false;
if(!match_symbol(0, o517, ctx)) return false;
opr* o519, *o518;
if(!match_binop(op::bit_and, &o519, &o518, o516, ctx)) return false;
if(!match_symbol(1, o519, ctx)) return false;
if(!match_symbol(0, o518, ctx)) return false;

ins* it = i;
ins* v515 = write_unop(it, op::bit_not, ctx.symbols[1]);
ins* v514 = write_binop(it, op::bit_and, ctx.symbols[0], v515);

i->replace_all_uses_with(v514);
return true;
}

static bool __replace_pattern__520(ins* i, match_context& ctx){
opr* o524, *o523;
if(!match_binop(op::bit_xor, &o524, &o523, i, ctx)) return false;
opr* o526, *o525;
if(!match_binop(op::bit_and, &o526, &o525, o524, ctx)) return false;
if(!match_symbol(1, o526, ctx)) return false;
if(!match_symbol(0, o525, ctx)) return false;
if(!match_symbol(0, o523, ctx)) return false;

ins* it = i;
ins* v522 = write_unop(it, op::bit_not, ctx.symbols[1]);
ins* v521 = write_binop(it, op::bit_and, ctx.symbols[0], v522);

i->replace_all_uses_with(v521);
return true;
}

static bool __replace_pattern__527(ins* i, match_context& ctx){
opr* o531, *o530;
if(!match_binop(op::bit_xor, &o531, &o530, i, ctx)) return false;
if(!match_symbol(0, o531, ctx)) return false;
opr* o533, *o532;
if(!match_binop(op::bit_or, &o533, &o532, o530, ctx)) return false;
if(!match_symbol(0, o533, ctx)) return false;
if(!match_symbol(1, o532, ctx)) return false;

ins* it = i;
ins* v529 = write_unop(it, op::bit_not, ctx.symbols[0]);
ins* v528 = write_binop(it, op::bit_and, ctx.symbols[1], v529);

i->replace_all_uses_with(v528);
return true;
}

static bool __replace_pattern__534(ins* i, match_context& ctx){
opr* o538, *o537;
if(!match_binop(op::bit_xor, &o538, &o537, i, ctx)) return false;
opr* o540, *o539;
if(!match_binop(op::bit_or, &o540, &o539, o538, ctx)) return false;
if(!match_symbol(0, o540, ctx)) return false;
if(!match_symbol(1, o539, ctx)) return false;
if(!match_symbol(0, o537, ctx)) return false;

ins* it = i;
ins* v536 = write_unop(it, op::bit_not, ctx.symbols[0]);
ins* v535 = write_binop(it, op::bit_and, ctx.symbols[1], v536);

i->replace_all_uses_with(v535);
return true;
}

static bool __replace_pattern__541(ins* i, match_context& ctx){
opr* o545, *o544;
if(!match_binop(op::bit_xor, &o545, &o544, i, ctx)) return false;
if(!match_symbol(0, o545, ctx)) return false;
opr* o547, *o546;
if(!match_binop(op::bit_or, &o547, &o546, o544, ctx)) return false;
if(!match_symbol(1, o547, ctx)) return false;
if(!match_symbol(0, o546, ctx)) return false;

ins* it = i;
ins* v543 = write_unop(it, op::bit_not, ctx.symbols[0]);
ins* v542 = write_binop(it, op::bit_and, ctx.symbols[1], v543);

i->replace_all_uses_with(v542);
return true;
}

static bool __replace_pattern__548(ins* i, match_context& ctx){
opr* o552, *o551;
if(!match_binop(op::bit_xor, &o552, &o551, i, ctx)) return false;
opr* o554, *o553;
if(!match_binop(op::bit_or, &o554, &o553, o552, ctx)) return false;
if(!match_symbol(1, o554, ctx)) return false;
if(!match_symbol(0, o553, ctx)) return false;
if(!match_symbol(0, o551, ctx)) return false;

ins* it = i;
ins* v550 = write_unop(it, op::bit_not, ctx.symbols[0]);
ins* v549 = write_binop(it, op::bit_and, ctx.symbols[1], v550);

i->replace_all_uses_with(v549);
return true;
}

static bool __replace_pattern__555(ins* i, match_context& ctx){
opr*o557;
if(!match_unop(op::bit_not, &o557, i, ctx)) return false;
opr* o559, *o558;
if(!match_binop(op::le, &o559, &o558, o557, ctx)) return false;
if(!match_symbol(0, o559, ctx)) return false;
if(!match_symbol(1, o558, ctx)) return false;

ins* it = i;
ins* v556 = write_cmp(it, op::gt, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v556);
return true;
}

static bool __replace_pattern__560(ins* i, match_context& ctx){
opr*o562;
if(!match_unop(op::bit_not, &o562, i, ctx)) return false;
opr* o564, *o563;
if(!match_binop(op::ge, &o564, &o563, o562, ctx)) return false;
if(!match_symbol(0, o564, ctx)) return false;
if(!match_symbol(1, o563, ctx)) return false;

ins* it = i;
ins* v561 = write_cmp(it, op::lt, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v561);
return true;
}

static bool __replace_pattern__565(ins* i, match_context& ctx){
opr*o567;
if(!match_unop(op::bit_not, &o567, i, ctx)) return false;
opr* o569, *o568;
if(!match_binop(op::ule, &o569, &o568, o567, ctx)) return false;
if(!match_symbol(0, o569, ctx)) return false;
if(!match_symbol(1, o568, ctx)) return false;

ins* it = i;
ins* v566 = write_cmp(it, op::ugt, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v566);
return true;
}

static bool __replace_pattern__570(ins* i, match_context& ctx){
opr*o572;
if(!match_unop(op::bit_not, &o572, i, ctx)) return false;
opr* o574, *o573;
if(!match_binop(op::uge, &o574, &o573, o572, ctx)) return false;
if(!match_symbol(0, o574, ctx)) return false;
if(!match_symbol(1, o573, ctx)) return false;

ins* it = i;
ins* v571 = write_cmp(it, op::ult, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v571);
return true;
}

static bool __replace_pattern__575(ins* i, match_context& ctx){
opr*o577;
if(!match_unop(op::bit_not, &o577, i, ctx)) return false;
opr* o579, *o578;
if(!match_binop(op::lt, &o579, &o578, o577, ctx)) return false;
if(!match_symbol(0, o579, ctx)) return false;
if(!match_symbol(1, o578, ctx)) return false;

ins* it = i;
ins* v576 = write_cmp(it, op::ge, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v576);
return true;
}

static bool __replace_pattern__580(ins* i, match_context& ctx){
opr*o582;
if(!match_unop(op::bit_not, &o582, i, ctx)) return false;
opr* o584, *o583;
if(!match_binop(op::gt, &o584, &o583, o582, ctx)) return false;
if(!match_symbol(0, o584, ctx)) return false;
if(!match_symbol(1, o583, ctx)) return false;

ins* it = i;
ins* v581 = write_cmp(it, op::le, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v581);
return true;
}

static bool __replace_pattern__585(ins* i, match_context& ctx){
opr*o587;
if(!match_unop(op::bit_not, &o587, i, ctx)) return false;
opr* o589, *o588;
if(!match_binop(op::ult, &o589, &o588, o587, ctx)) return false;
if(!match_symbol(0, o589, ctx)) return false;
if(!match_symbol(1, o588, ctx)) return false;

ins* it = i;
ins* v586 = write_cmp(it, op::uge, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v586);
return true;
}

static bool __replace_pattern__590(ins* i, match_context& ctx){
opr*o592;
if(!match_unop(op::bit_not, &o592, i, ctx)) return false;
opr* o594, *o593;
if(!match_binop(op::ugt, &o594, &o593, o592, ctx)) return false;
if(!match_symbol(0, o594, ctx)) return false;
if(!match_symbol(1, o593, ctx)) return false;

ins* it = i;
ins* v591 = write_cmp(it, op::ule, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v591);
return true;
}

static bool __replace_pattern__595(ins* i, match_context& ctx){
opr*o597;
if(!match_unop(op::bit_not, &o597, i, ctx)) return false;
opr* o599, *o598;
if(!match_binop(op::eq, &o599, &o598, o597, ctx)) return false;
if(!match_symbol(0, o599, ctx)) return false;
if(!match_symbol(1, o598, ctx)) return false;

ins* it = i;
ins* v596 = write_cmp(it, op::ne, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v596);
return true;
}

static bool __replace_pattern__600(ins* i, match_context& ctx){
opr*o602;
if(!match_unop(op::bit_not, &o602, i, ctx)) return false;
opr* o604, *o603;
if(!match_binop(op::ne, &o604, &o603, o602, ctx)) return false;
if(!match_symbol(0, o604, ctx)) return false;
if(!match_symbol(1, o603, ctx)) return false;

ins* it = i;
ins* v601 = write_cmp(it, op::eq, ctx.symbols[0], ctx.symbols[1]);

i->replace_all_uses_with(v601);
return true;
}

static bool __replace_pattern__605(ins* i, match_context& ctx){
opr* o608, *o607;
if(!match_binop(op::eq, &o608, &o607, i, ctx)) return false;
if(!match_symbol(0, o608, ctx)) return false;
opr*o609;
if(!match_unop(op::neg, &o609, o607, ctx)) return false;
if(!match_symbol(0, o609, ctx)) return false;

ins* it = i;
ins* v606 = write_cmp(it, op::eq, ctx.symbols[0], imm(i->get_type(), 0));

i->replace_all_uses_with(v606);
return true;
}

RC_INITIALIZER {
	replace_list.insert(replace_list.end(), { &__replace_pattern__1,&__replace_pattern__6,&__replace_pattern__13,&__replace_pattern__20,&__replace_pattern__27,&__replace_pattern__34,&__replace_pattern__41,&__replace_pattern__48,&__replace_pattern__55,&__replace_pattern__62,&__replace_pattern__69,&__replace_pattern__76,&__replace_pattern__82,&__replace_pattern__88,&__replace_pattern__96,&__replace_pattern__104,&__replace_pattern__112,&__replace_pattern__120,&__replace_pattern__128,&__replace_pattern__136,&__replace_pattern__143,&__replace_pattern__150,&__replace_pattern__157,&__replace_pattern__164,&__replace_pattern__171,&__replace_pattern__178,&__replace_pattern__181,&__replace_pattern__184,&__replace_pattern__188,&__replace_pattern__192,&__replace_pattern__196,&__replace_pattern__200,&__replace_pattern__205,&__replace_pattern__210,&__replace_pattern__215,&__replace_pattern__219,&__replace_pattern__224,&__replace_pattern__229,&__replace_pattern__234,&__replace_pattern__239,&__replace_pattern__245,&__replace_pattern__251,&__replace_pattern__260,&__replace_pattern__269,&__replace_pattern__278,&__replace_pattern__287,&__replace_pattern__296,&__replace_pattern__305,&__replace_pattern__314,&__replace_pattern__323,&__replace_pattern__332,&__replace_pattern__341,&__replace_pattern__350,&__replace_pattern__359,&__replace_pattern__368,&__replace_pattern__377,&__replace_pattern__386,&__replace_pattern__395,&__replace_pattern__403,&__replace_pattern__411,&__replace_pattern__419,&__replace_pattern__427,&__replace_pattern__435,&__replace_pattern__443,&__replace_pattern__451,&__replace_pattern__459,&__replace_pattern__464,&__replace_pattern__469,&__replace_pattern__474,&__replace_pattern__479,&__replace_pattern__484,&__replace_pattern__489,&__replace_pattern__494,&__replace_pattern__499,&__replace_pattern__506,&__replace_pattern__513,&__replace_pattern__520,&__replace_pattern__527,&__replace_pattern__534,&__replace_pattern__541,&__replace_pattern__548,&__replace_pattern__555,&__replace_pattern__560,&__replace_pattern__565,&__replace_pattern__570,&__replace_pattern__575,&__replace_pattern__580,&__replace_pattern__585,&__replace_pattern__590,&__replace_pattern__595,&__replace_pattern__600,&__replace_pattern__605 });
};
#endif
