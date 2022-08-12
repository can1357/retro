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
opr* o3, *o2;
if(!match_binop(op::add, &o3, &o2, i, ctx)) return false;
if(!match_symbol(0, o3, ctx)) return false;
if(!match_imm(0, o2, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__4(ins* i, match_context& ctx){
opr* o6, *o5;
if(!match_binop(op::add, &o6, &o5, i, ctx)) return false;
if(!match_imm(0, o6, ctx)) return false;
if(!match_symbol(0, o5, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__7(ins* i, match_context& ctx){
opr* o9, *o8;
if(!match_binop(op::sub, &o9, &o8, i, ctx)) return false;
if(!match_symbol(0, o9, ctx)) return false;
if(!match_imm(0, o8, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__10(ins* i, match_context& ctx){
opr* o12, *o11;
if(!match_binop(op::bit_or, &o12, &o11, i, ctx)) return false;
if(!match_symbol(0, o12, ctx)) return false;
if(!match_symbol(0, o11, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__13(ins* i, match_context& ctx){
opr* o15, *o14;
if(!match_binop(op::bit_or, &o15, &o14, i, ctx)) return false;
if(!match_symbol(0, o15, ctx)) return false;
if(!match_symbol(0, o14, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__16(ins* i, match_context& ctx){
opr* o18, *o17;
if(!match_binop(op::bit_or, &o18, &o17, i, ctx)) return false;
if(!match_symbol(0, o18, ctx)) return false;
if(!match_imm(0, o17, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__19(ins* i, match_context& ctx){
opr* o21, *o20;
if(!match_binop(op::bit_or, &o21, &o20, i, ctx)) return false;
if(!match_imm(0, o21, ctx)) return false;
if(!match_symbol(0, o20, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__22(ins* i, match_context& ctx){
opr* o24, *o23;
if(!match_binop(op::bit_and, &o24, &o23, i, ctx)) return false;
if(!match_symbol(0, o24, ctx)) return false;
if(!match_symbol(0, o23, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__25(ins* i, match_context& ctx){
opr* o27, *o26;
if(!match_binop(op::bit_and, &o27, &o26, i, ctx)) return false;
if(!match_symbol(0, o27, ctx)) return false;
if(!match_symbol(0, o26, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__28(ins* i, match_context& ctx){
opr* o30, *o29;
if(!match_binop(op::bit_xor, &o30, &o29, i, ctx)) return false;
if(!match_symbol(0, o30, ctx)) return false;
if(!match_imm(0, o29, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__31(ins* i, match_context& ctx){
opr* o33, *o32;
if(!match_binop(op::bit_xor, &o33, &o32, i, ctx)) return false;
if(!match_imm(0, o33, ctx)) return false;
if(!match_symbol(0, o32, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__34(ins* i, match_context& ctx){
opr* o36, *o35;
if(!match_binop(op::bit_and, &o36, &o35, i, ctx)) return false;
if(!match_symbol(0, o36, ctx)) return false;
if(!match_imm(-1, o35, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__37(ins* i, match_context& ctx){
opr* o39, *o38;
if(!match_binop(op::bit_and, &o39, &o38, i, ctx)) return false;
if(!match_imm(-1, o39, ctx)) return false;
if(!match_symbol(0, o38, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__40(ins* i, match_context& ctx){
opr* o42, *o41;
if(!match_binop(op::mul, &o42, &o41, i, ctx)) return false;
if(!match_symbol(0, o42, ctx)) return false;
if(!match_imm(1, o41, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__43(ins* i, match_context& ctx){
opr* o45, *o44;
if(!match_binop(op::mul, &o45, &o44, i, ctx)) return false;
if(!match_imm(1, o45, ctx)) return false;
if(!match_symbol(0, o44, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__46(ins* i, match_context& ctx){
opr* o48, *o47;
if(!match_binop(op::div, &o48, &o47, i, ctx)) return false;
if(!match_symbol(0, o48, ctx)) return false;
if(!match_imm(1, o47, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__49(ins* i, match_context& ctx){
opr* o51, *o50;
if(!match_binop(op::udiv, &o51, &o50, i, ctx)) return false;
if(!match_symbol(0, o51, ctx)) return false;
if(!match_imm(1, o50, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__52(ins* i, match_context& ctx){
opr* o54, *o53;
if(!match_binop(op::bit_rol, &o54, &o53, i, ctx)) return false;
if(!match_symbol(0, o54, ctx)) return false;
if(!match_imm(0, o53, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__55(ins* i, match_context& ctx){
opr* o57, *o56;
if(!match_binop(op::bit_ror, &o57, &o56, i, ctx)) return false;
if(!match_symbol(0, o57, ctx)) return false;
if(!match_imm(0, o56, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__58(ins* i, match_context& ctx){
opr* o60, *o59;
if(!match_binop(op::bit_shr, &o60, &o59, i, ctx)) return false;
if(!match_symbol(0, o60, ctx)) return false;
if(!match_imm(0, o59, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__61(ins* i, match_context& ctx){
opr* o63, *o62;
if(!match_binop(op::bit_sar, &o63, &o62, i, ctx)) return false;
if(!match_symbol(0, o63, ctx)) return false;
if(!match_imm(0, o62, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__64(ins* i, match_context& ctx){
opr* o66, *o65;
if(!match_binop(op::bit_shl, &o66, &o65, i, ctx)) return false;
if(!match_symbol(0, o66, ctx)) return false;
if(!match_imm(0, o65, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__67(ins* i, match_context& ctx){
opr* o69, *o68;
if(!match_binop(op::bit_xor, &o69, &o68, i, ctx)) return false;
opr* o71, *o70;
if(!match_binop(op::bit_xor, &o71, &o70, o69, ctx)) return false;
if(!match_symbol(1, o71, ctx)) return false;
if(!match_symbol(0, o70, ctx)) return false;
if(!match_symbol(1, o68, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__72(ins* i, match_context& ctx){
opr* o74, *o73;
if(!match_binop(op::bit_xor, &o74, &o73, i, ctx)) return false;
if(!match_symbol(1, o74, ctx)) return false;
opr* o76, *o75;
if(!match_binop(op::bit_xor, &o76, &o75, o73, ctx)) return false;
if(!match_symbol(1, o76, ctx)) return false;
if(!match_symbol(0, o75, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__77(ins* i, match_context& ctx){
opr* o79, *o78;
if(!match_binop(op::bit_xor, &o79, &o78, i, ctx)) return false;
opr* o81, *o80;
if(!match_binop(op::bit_xor, &o81, &o80, o79, ctx)) return false;
if(!match_symbol(0, o81, ctx)) return false;
if(!match_symbol(1, o80, ctx)) return false;
if(!match_symbol(1, o78, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__82(ins* i, match_context& ctx){
opr* o84, *o83;
if(!match_binop(op::bit_xor, &o84, &o83, i, ctx)) return false;
if(!match_symbol(1, o84, ctx)) return false;
opr* o86, *o85;
if(!match_binop(op::bit_xor, &o86, &o85, o83, ctx)) return false;
if(!match_symbol(0, o86, ctx)) return false;
if(!match_symbol(1, o85, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(ctx.symbols[0]);
return true;
}

static bool __replace_pattern__87(ins* i, match_context& ctx){
opr* o89, *o88;
if(!match_binop(op::sub, &o89, &o88, i, ctx)) return false;
if(!match_symbol(0, o89, ctx)) return false;
if(!match_symbol(0, o88, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), 0));
return true;
}

static bool __replace_pattern__90(ins* i, match_context& ctx){
opr* o92, *o91;
if(!match_binop(op::bit_and, &o92, &o91, i, ctx)) return false;
if(!match_symbol(0, o92, ctx)) return false;
if(!match_imm(0, o91, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), 0));
return true;
}

static bool __replace_pattern__93(ins* i, match_context& ctx){
opr* o95, *o94;
if(!match_binop(op::bit_and, &o95, &o94, i, ctx)) return false;
if(!match_imm(0, o95, ctx)) return false;
if(!match_symbol(0, o94, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), 0));
return true;
}

static bool __replace_pattern__96(ins* i, match_context& ctx){
opr* o98, *o97;
if(!match_binop(op::bit_and, &o98, &o97, i, ctx)) return false;
if(!match_symbol(0, o98, ctx)) return false;
opr*o99;
if(!match_unop(op::bit_not, &o99, o97, ctx)) return false;
if(!match_symbol(0, o99, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), 0));
return true;
}

static bool __replace_pattern__100(ins* i, match_context& ctx){
opr* o102, *o101;
if(!match_binop(op::bit_and, &o102, &o101, i, ctx)) return false;
opr*o103;
if(!match_unop(op::bit_not, &o103, o102, ctx)) return false;
if(!match_symbol(0, o103, ctx)) return false;
if(!match_symbol(0, o101, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), 0));
return true;
}

static bool __replace_pattern__104(ins* i, match_context& ctx){
opr* o106, *o105;
if(!match_binop(op::bit_xor, &o106, &o105, i, ctx)) return false;
if(!match_symbol(0, o106, ctx)) return false;
if(!match_symbol(0, o105, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), 0));
return true;
}

static bool __replace_pattern__107(ins* i, match_context& ctx){
opr* o109, *o108;
if(!match_binop(op::bit_xor, &o109, &o108, i, ctx)) return false;
if(!match_symbol(0, o109, ctx)) return false;
if(!match_symbol(0, o108, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), 0));
return true;
}

static bool __replace_pattern__110(ins* i, match_context& ctx){
opr* o112, *o111;
if(!match_binop(op::bit_or, &o112, &o111, i, ctx)) return false;
if(!match_symbol(0, o112, ctx)) return false;
if(!match_imm(-1, o111, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), -1));
return true;
}

static bool __replace_pattern__113(ins* i, match_context& ctx){
opr* o115, *o114;
if(!match_binop(op::bit_or, &o115, &o114, i, ctx)) return false;
if(!match_imm(-1, o115, ctx)) return false;
if(!match_symbol(0, o114, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), -1));
return true;
}

static bool __replace_pattern__116(ins* i, match_context& ctx){
opr* o118, *o117;
if(!match_binop(op::add, &o118, &o117, i, ctx)) return false;
if(!match_symbol(0, o118, ctx)) return false;
opr*o119;
if(!match_unop(op::bit_not, &o119, o117, ctx)) return false;
if(!match_symbol(0, o119, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), -1));
return true;
}

static bool __replace_pattern__120(ins* i, match_context& ctx){
opr* o122, *o121;
if(!match_binop(op::add, &o122, &o121, i, ctx)) return false;
opr*o123;
if(!match_unop(op::bit_not, &o123, o122, ctx)) return false;
if(!match_symbol(0, o123, ctx)) return false;
if(!match_symbol(0, o121, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), -1));
return true;
}

static bool __replace_pattern__124(ins* i, match_context& ctx){
opr* o126, *o125;
if(!match_binop(op::bit_xor, &o126, &o125, i, ctx)) return false;
if(!match_symbol(0, o126, ctx)) return false;
opr*o127;
if(!match_unop(op::bit_not, &o127, o125, ctx)) return false;
if(!match_symbol(0, o127, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), -1));
return true;
}

static bool __replace_pattern__128(ins* i, match_context& ctx){
opr* o130, *o129;
if(!match_binop(op::bit_xor, &o130, &o129, i, ctx)) return false;
opr*o131;
if(!match_unop(op::bit_not, &o131, o130, ctx)) return false;
if(!match_symbol(0, o131, ctx)) return false;
if(!match_symbol(0, o129, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), -1));
return true;
}

static bool __replace_pattern__132(ins* i, match_context& ctx){
opr* o134, *o133;
if(!match_binop(op::div, &o134, &o133, i, ctx)) return false;
if(!match_symbol(0, o134, ctx)) return false;
if(!match_symbol(0, o133, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), 1));
return true;
}

static bool __replace_pattern__135(ins* i, match_context& ctx){
opr* o137, *o136;
if(!match_binop(op::udiv, &o137, &o136, i, ctx)) return false;
if(!match_symbol(0, o137, ctx)) return false;
if(!match_symbol(0, o136, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), 1));
return true;
}

static bool __replace_pattern__138(ins* i, match_context& ctx){
opr* o140, *o139;
if(!match_binop(op::rem, &o140, &o139, i, ctx)) return false;
if(!match_symbol(0, o140, ctx)) return false;
if(!match_symbol(0, o139, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), 0));
return true;
}

static bool __replace_pattern__141(ins* i, match_context& ctx){
opr* o143, *o142;
if(!match_binop(op::urem, &o143, &o142, i, ctx)) return false;
if(!match_symbol(0, o143, ctx)) return false;
if(!match_symbol(0, o142, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), 0));
return true;
}

static bool __replace_pattern__144(ins* i, match_context& ctx){
opr* o146, *o145;
if(!match_binop(op::mul, &o146, &o145, i, ctx)) return false;
if(!match_symbol(0, o146, ctx)) return false;
if(!match_imm(0, o145, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), 0));
return true;
}

static bool __replace_pattern__147(ins* i, match_context& ctx){
opr* o149, *o148;
if(!match_binop(op::mul, &o149, &o148, i, ctx)) return false;
if(!match_imm(0, o149, ctx)) return false;
if(!match_symbol(0, o148, ctx)) return false;

ins* it = i;

i->replace_all_uses_with(imm(i->get_type(), 0));
return true;
}

RC_INITIALIZER {
	replace_list.insert(replace_list.end(), { &__replace_pattern__1,&__replace_pattern__4,&__replace_pattern__7,&__replace_pattern__10,&__replace_pattern__13,&__replace_pattern__16,&__replace_pattern__19,&__replace_pattern__22,&__replace_pattern__25,&__replace_pattern__28,&__replace_pattern__31,&__replace_pattern__34,&__replace_pattern__37,&__replace_pattern__40,&__replace_pattern__43,&__replace_pattern__46,&__replace_pattern__49,&__replace_pattern__52,&__replace_pattern__55,&__replace_pattern__58,&__replace_pattern__61,&__replace_pattern__64,&__replace_pattern__67,&__replace_pattern__72,&__replace_pattern__77,&__replace_pattern__82,&__replace_pattern__87,&__replace_pattern__90,&__replace_pattern__93,&__replace_pattern__96,&__replace_pattern__100,&__replace_pattern__104,&__replace_pattern__107,&__replace_pattern__110,&__replace_pattern__113,&__replace_pattern__116,&__replace_pattern__120,&__replace_pattern__124,&__replace_pattern__128,&__replace_pattern__132,&__replace_pattern__135,&__replace_pattern__138,&__replace_pattern__141,&__replace_pattern__144,&__replace_pattern__147 });
};
#endif
