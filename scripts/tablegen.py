#!/usr/bin/env python
# coding: utf-8

# In[83]:


#!conda install --yes toml
import toml
import glob
import os
import sys
import time
import ast
import traceback


# In[73]:


# Directive implementation.
#
umap = {}
bmap = {}
cmplist = []
cmtlist = []
def update_operators(op_tbl):
    global umap
    global bmap
    global cmplist
    global cmtlist
    umap = {}
    bmap = {}
    cmtlist = []
    cmplist = []
    for k,v in op_tbl["op"].items():
        cname = "op::" + to_cname(k)
        if "unary" in v["kind"]:
            umap[v["symbol"]] = cname
        else:
            bmap[v["symbol"]] = cname
            if "cmp" in v["kind"]:
                cmplist.append(cname)
            elif v.get("commutative", False):
                cmtlist.append(cname)
dtmpcounter = 0 

class DirectiveValue:
    def __init__(self, value):
        self.value = value
    def write_match(self, ref):        
        return "if(!match_imm({0}, {1}, ctx)) return false;\n".format(self.value, ref)
    def write_create(self):
        return "", "imm(i->get_type(), {0})".format(self.value)
    def to_string(self):
        return self.value
    def permutate(self):
        return [self]
    def is_imm(self):
        return True
class DirectiveValueWild:
    def __init__(self, name):
        self.name = name
    def write_match(self, ref):
        return "if(!match_imm_symbol({0}, {1}, ctx)) return false;\n".format(ord(self.name)-ord('A'), ref)
    def write_create(self):
        return "", "ctx.symbols[{0}].const_val".format(ord(self.name)-ord('A'))
    def to_string(self):
        return self.name
    def permutate(self):
        return [self]
    def is_imm(self):
        return True
class DirectiveIdentifier:
    def __init__(self, name):
        self.name = name
    def write_match(self, ref):
        return "if(!match_symbol({0}, {1}, ctx)) return false;\n".format(ord(self.name)-ord('A'), ref)
    def write_create(self):
        return "", "ctx.symbols[{0}]".format(ord(self.name)-ord('A'))
    def to_string(self):
        return self.name
    def permutate(self):
        return [self]
    def is_imm(self):
        return False
class DirectiveExpr:
    def __init__(self, op, lhs, rhs = None, no_lookup = False):
        if rhs == None:
            self.op =  op if no_lookup else umap[op]
            self.lhs = None
            self.rhs = lhs
        else:
            self.op =  op if no_lookup else bmap[op]
            self.lhs = lhs
            self.rhs = rhs
    def is_unary(self):
        return self.lhs == None
    def permutate(self):
        global cmtlist
        
        if self.is_unary():
            l = self.rhs.permutate()
            for i in range(len(l)):
                l[i] = DirectiveExpr(self.op, l[i], no_lookup=True)
            return l
        else:
            cmt = self.op in cmtlist
            l = self.lhs.permutate()
            r = self.rhs.permutate()
            result = []
            for i in range(len(l)):
                for j in range(len(r)):
                    result.append(DirectiveExpr(self.op, l[i], r[j], no_lookup=True))
                    if cmt:
                        result.append(DirectiveExpr(self.op, r[j], l[i], no_lookup=True))
            return result
    def is_imm(self):
        return (not(self.lhs) or self.lhs.is_imm()) and self.rhs.is_imm()
    def write_create(self):
        global dtmpcounter
        global cmplist
        dtmpcounter += 1
        res_name = "v{0}".format(dtmpcounter)
        
        if self.is_imm():
            _, rv = self.rhs.write_create()
            if self.is_unary():
                return "", "{rhs}.apply({op})".format(rhs=rv, op=self.op)
            else:
                _, lv = self.lhs.write_create()
                return "", "{lhs}.apply({op}, {rhs})".format(rhs=rv, lhs=lv, op=self.op)
        elif self.is_unary():
            rhst, rhsv = self.rhs.write_create()
            res = rhst + "ins* {res_name} = write_unop(it, {op}, {rhs});\n".format(
                res_name=res_name, op=self.op,
                rhs=rhsv
            )
            return res, res_name
        else:
            wrtr = "write_cmp" if (self.op in cmplist) else "write_binop"
            lhts, lhsv = self.lhs.write_create()
            rhst, rhsv = self.rhs.write_create()
            res = lhts + rhst + "ins* {res_name} = {wrtr}(it, {op}, {lhs}, {rhs});\n".format(
                res_name=res_name, op=self.op,
                lhs=lhsv, rhs=rhsv, wrtr=wrtr
            )
            return res, res_name
            
    def write_match(self, ref):
        global dtmpcounter
        
        dtmpcounter += 1
        rhs_name = "o{0}".format(dtmpcounter)
        
        if self.is_unary():
            return """opr*{rhs_name};
if(!match_unop({op}, &{rhs_name}, {ref}, ctx)) return false;
{rest}""".format(
                rhs_name=rhs_name,
                op=self.op, ref=ref,
                rest=self.rhs.write_match(rhs_name)
            )
        else:
            dtmpcounter += 1
            lhs_name = "o{0}".format(dtmpcounter)
            return """opr* {lhs_name}, *{rhs_name};
if(!match_binop({op}, &{lhs_name}, &{rhs_name}, {ref}, ctx)) return false;
{rest}""".format(
                lhs_name=lhs_name,rhs_name=rhs_name,
                op=self.op, ref=ref,
                rest=self.lhs.write_match(lhs_name)+self.rhs.write_match(rhs_name)
            )

    def to_string(self):
        if self.is_unary():
            return "Unary<{0}>({1})".format(self.op, self.rhs.to_string())
        else:
            return "Binary<{0}>({1}, {2})".format(self.op, self.lhs.to_string(), self.rhs.to_string())

def split_until(s, f):
    for i in range(len(s)):
        if f(s[i]) or s[i] == "(" or s[i] == ")" or s[i] == "[" or s[i] == "," or s[i] == "@":
            return s[:i], s[i:]
    return s, ""
def split_until_noesc(s, f):
    for i in range(len(s)):
        if f(s[i]):
            return s[:i], s[i:]
    return s, ""
def consume_binop(lhs,s):
    op,s = split_until(s, lambda a: a.isnumeric() or a.isupper())
    rhs,s = consume_expr(s)
    return DirectiveExpr(op, lhs, rhs), s
def consume_expr(s):
    i = s[0]
    if i == "[":
        val,s = split_until_noesc(s[1:], lambda a: a == "]")
        assert s[0] == "]"
        return DirectiveValue(val), s[1:]
    elif i == "(":
        e,s = consume_expr(s[1:])
        if s[0] != ")":
            e,s = consume_binop(e,s)
        assert s[0] == ")"
        return e, s[1:]
    elif i == "@":
        return DirectiveValueWild(s[1]), s[2:]
    elif i.isnumeric() or i == ".":
        val,s = split_until(s, lambda a: not(a.isnumeric() or a == "."))
        return DirectiveValue(val), s
    elif i.isalpha() and i.isupper():
        return DirectiveIdentifier(i[0]), s[1:]
    elif i.isalpha() and i.islower():
        op,s = split_until(s, lambda a: a == "(")
        lhs,s = consume_expr(s[1:])
        if s[0]==")":
            return DirectiveExpr(op, lhs), s[1:]
        else:
            rhs,s = consume_expr(s[1:])
            assert s[0] == ")"
            return DirectiveExpr(op, lhs, rhs), s[1:]
    else:
        op,s = split_until(s, lambda a: a.isalpha() or a.isnumeric())
        rhs,s = consume_expr(s)
        return DirectiveExpr(op, rhs), s
def parse_expr(s):
    e,l = consume_expr(s.replace(" ", ""))
    if len(l) != 0:
        e,l = consume_binop(e,l)
    assert len(l) == 0
    return e


# In[147]:


# Constants
#
TEXT_WIDTH =        120
CXX_PACK_INTEGERS = True
CXX_STD_INTEGERS =  [8, 16, 32, 64]
CXX_HELP_FWD_FMT =  """
	using value_type = {0};
	static constexpr std::span<const {0}_desc> all();
	RC_INLINE constexpr const {0} id() const {{ return {0}(this - all().data()); }}
"""
CXX_HELP_DCL_FMT =  """RC_INLINE constexpr std::span<const {0}_desc> {0}_desc::all() {{ return {0}s; }}"""
CXX_ARR_DCL_FMT =   "\ninline constexpr {0} {1}s[] = {{\n"
CXX_DESC_SUFFIX =   "_desc"
CXX_NS_SUFFIX_PER_ENUM = """namespace retro {{ template<> struct descriptor<{0}::{1}> {{ using type = {0}::{1}_desc; }}; }};
RC_DEFINE_STD_VISITOR_FOR({0}::{1}, {2})
""" 
CXX_NULL_ENUM = "none"
JS_NULL_ENUM = "None"
CXX_USE_BOOL = False
JS_MAX_INT = 51
JS_ARR_DCL_FMT =   "\n// prettier-ignore\nconst {1}_DescTable: {0}[] = [\n"
JS_ENUM_SUFFIX = """// prettier-ignore
export namespace {0} {{
    export function reflect(i:{0}) : {1} {{ return {0}_DescTable[i]; }}
    export function toString(i:{0}) : string {{ return {0}_DescTable[i].name; }}
}}"""

JS_TYPE_MAP = {
    "@type.i1": ("boolean", "I1"),
    "@type.i8": ("number", "I8"),  
    "@type.i16": ("number", "I16"),  
    "@type.i32": ("number", "I32"),  
    "@type.i64": ("bigint", "I64"),  
    "@type.f32": ("number", "F32"),  
    "@type.f64": ("number", "F64"),  
    "@type.str": ("string", "Str"),  
    "@type.reg": ("MReg", "MReg"),  
    "@type.op": ("Op", "Op"),  
    "@type.intrinsic": ("Intrinsic", "Intrinsic"),  
    "@type.pointer": ("bigint", "Ptr"), 
    "@type.f32x16": ("number[]", "F32x16"),  
    "@type.f32x2": ("number[]", "F32x2"),  
    "@type.i16x32": ("number[]", "I16x32"),  
    "@type.i16x4": ("number[]", "I16x4"),  
    "@type.f32x4": ("number[]", "F32x4"),  
    "@type.f32x8": ("number[]", "F32x8"),  
    "@type.f64x2": ("number[]", "F64x2"),  
    "@type.f64x4": ("number[]", "F64x4"),  
    "@type.f64x8": ("number[]", "F64x8"),  
    "@type.i16x16": ("number[]", "I16x16"), 
    "@type.i16x8": ("number[]", "I16x8"),  
    "@type.i32x16": ("number[]", "I32x16"),  
    "@type.i32x2": ("number[]", "I32x2"),  
    "@type.i32x4": ("number[]", "I32x4"),  
    "@type.i32x8": ("number[]", "I32x8"),  
    "@type.i64x2": ("bigint[]", "I64x2"), 
    "@type.i64x4": ("bigint[]", "I64x4"),  
    "@type.i64x8": ("bigint[]", "I64x8"),  
    "@type.i8x16": ("number[]", "I8x16"),  
    "@type.i8x32": ("number[]", "I8x32"),  
    "@type.i8x64": ("number[]", "I8x64"),  
    "@type.i8x8": ("number[]", "I8x8"),
}

# Bit helpers
#
def bitcount(x):
    n = 0
    while x != 0:
        x >>= 1
        n += 1
    return n


def bit_intmax(n):
    return (1 << (n-1))-1
def bit_intmin(n):
    return -bit_intmax(n) - 1
def bit_uintmax(n):
    return (1 << (n))-1
    
# Text helpers.
#
def to_cname(name):
    if "-" in name:
        return name.replace("-", "_")
    else:
        return name
def to_jname(name, value = False):
    if "::" in name:
        return to_jname(name.split("::")[-1], value)
    parts = name.replace("-", "_").split("_")
    for i in range(0, len(parts)):
        if (not value) or i != 0:
            parts[i] = parts[i].capitalize()
    return "".join(parts)
def max_name_len(names):
    value = 0
    for k in names:
        if isinstance(k, list):
            value = max(value, len(k[0]))
        else:
            value = max(value, len(k))
    return value
def make_box(text, h = 0):
    pad = " " * ((TEXT_WIDTH - len(text))//2)
    text = "//" + pad + text + pad + "//"
    fpad = len(text) * "/" + "\n"
    epad = ("//" + ((len(text)-4) * " ") + "//\n") * h
    return fpad + epad + text + "\n" + epad + fpad
def shift_right(data):
    if data[0] != '\n':
        data = "\t" + data
    return data.replace("\n", "\n\t")
def multiline_eval(expr, context, local_list):
    tree = ast.parse(expr)
    eval_exprs = []
    exec_exprs = []
    for module in tree.body:
        if isinstance(module, ast.Expr):
            eval_exprs.append(module.value)
        else:
            exec_exprs.append(module)
    exec_expr = ast.Module(exec_exprs, type_ignores=[])
    exec(compile(exec_expr, 'file', 'exec'), context, local_list)
    for eval_expr in eval_exprs:
        exec(compile(ast.Expression((eval_expr)), 'file', 'eval'), context, local_list)

# Helper for writing C++/JS functions.
#
from copy import deepcopy
class CxxFunction:
	def __init__(self):
		self.args =        []
		self.tmps =        []
		self.stmts =       []
		self.post_args =   []
		self.post_tmps =   []
		self.post_stmts =  []
		self.return_type = "decltype(auto)"
		self.qualifiers = "inline"
		
	def add_argument(self, type, name):
		if type == None:
			type = "T" + name
			self.post_tmps.insert(0, type)
			self.args.append(type + "&& " + name)
		else:
			self.args.append(type + " " + name)
		return type

	def add_argument_pack(self, name):
		self.post_tmps.append("...T"+name)
		self.post_args.append("T{0}&&... {0}".format(name))
		return "T" + name

	def clone(self):
		return deepcopy(self)

	def write(self, name):
		result = ""

		# Add templates.
		#
		if len(self.tmps) != 0 or len(self.post_tmps) != 0:
			tmps = self.tmps + self.post_tmps
			for i in range(len(tmps)):
				tmps[i] = "typename " + tmps[i]
			result = "template<" + ", ".join(tmps) + ">\n"

		# Create the decl.
		#
		result += self.qualifiers + " " + self.return_type + " " + name + "("
		if len(self.args) != 0 or len(self.post_args) != 0:
			args = self.args + self.post_args
			result += ", ".join(args)
		result += ") {\n"

		result += ";\n".join(self.stmts + self.post_stmts)
		result += ";\n}"
		return result
class JFunction:
	def __init__(self):
		self.args =        []
		self.stmts =       []
		self.post_args =   []
		self.post_stmts =  []
		self.return_type = "any"
		self.qualifiers = "function"

	def add_argument(self, type, name):
		if type == None:
			type = "any"
		self.args.append(name + ": " + type)
		return type
	def add_argument_pack(self, type, name):
		if type == None:
			type = "any"
		self.post_args.append("..."+name+": "+type+"[]")
		return name
	def clone(self):
		return deepcopy(self)
	def write(self, name):
		result = ""

		# Create the decl.
		#
		result += self.qualifiers + " " + name + "("
		if len(self.args) != 0 or len(self.post_args) != 0:
			args = self.args + self.post_args
			result += ", ".join(args)
		result += ")"
		if self.return_type != "any":
			result += ": " + self.return_type
		result += " {\n\t"
		result += ";\n\t".join(self.stmts + self.post_stmts)
		result += ";\n}"
		return result

# C++ Types.
#
class CxxType:
    def __init__(self, traits):
        self.default = traits.get("default", "{}")
        self.ptype =   traits["ptype"]
        self.name =    traits["name"]
        self.size =    traits["size"]
    def write(self, value):
        if value == None:
            return self.default
        return str(value)
    def declare(self, field):
        return [to_cname(self.name), to_cname(field), " = " + self.default + ";"]
    def jwrite(self, value):
        if value == None:
            return self.jdefault
        return str(value)
    def jdeclare(self, field):
        return [to_jname(field, value=True)+":", self.jname, " = " + self.jdefault + ";"]
class CxxInteger(CxxType):
    def __init__(self, bits, signed):
        self.bits =    bits
        self.signed =  signed
        
        if bits == 1:
            assert not signed
            self.default = "false" if CXX_USE_BOOL else "0"
            self.ptype =   bool
            self.jdefault = "false"
            self.jname =    "boolean"
        else:
            self.default = "0"
            self.ptype =   int
            self.jdefault = "0n" if bits > JS_MAX_INT else "0"
            self.jname =    "bigint" if bits > JS_MAX_INT else "number"

        for k in CXX_STD_INTEGERS:
            if k >= bits:
                self.name =         ("i" if signed else "u") + str(k)
                self.is_bitfield =  k != bits
                self.size =         k / 8
                break
            assert k != 64

    def declare(self, field):
        field = to_cname(field)
        if self.is_bitfield:
             field += " : " + str(self.bits)
        return [to_cname(self.name), field, " = " + self.default + ";"]
    def write(self, value):
        if value == None:
            return self.default
        if self.bits == 1:
            if CXX_USE_BOOL:
                return "true" if value else "false"
            else:
                return "1" if value else "0"
        elif self.bits >= 32:
            return hex(value)
        else:
            return str(value)
        
    def jdeclare(self, field):
        field = to_jname(field, value=True)
        return [field+":", self.jname, " = " + self.jdefault + ";"]
    def jwrite(self, value):
        if value == None:
            return self.jdefault
        if self.bits == 1:
            return "true" if value else "false"
        elif self.bits >= JS_MAX_INT:
            return hex(value) + "n"
        elif self.bits >= 32:
            return hex(value)
        else:
            return str(value)
        
class CxxArray(CxxType):
    def __init__(self, underlying, length = None):
        self.default =    "{}"
        self.ptype =      list
        self.underlying = underlying
        self.length =     length
        self.jname =      "{0}[]".format(underlying.jname)
        self.jdefault =   "[]"
        if length != None:
            self.name =       "std::array<{0}, {1}>".format(to_cname(underlying.name), length)
            self.size =       underlying.size * length
        else:
            self.name =       "small_array<{0}>".format(to_cname(underlying.name))
            self.size =       16
    def write(self, value):
        if value and len(value) != 0:
            return "{" + ",".join([self.underlying.write(v) for v in value]) + "}"
        else:
            return "{}"
    def declare(self, field):
        if self.length == None:
            return [to_cname(self.name), to_cname(field) + ";", ""]
        else:
            return [to_cname(self.name), to_cname(field), " = " + self.default + ";"]
        
    def jwrite(self, value):
        if value and len(value) != 0:
            return "[" + ",".join([self.underlying.jwrite(v) for v in value]) + "]"
        else:
            return "[]"
    def jdeclare(self, field):
        return [to_jname(field, value=True)+":", self.jname, " = " + self.jdefault + ";"]
        
        
class CxxEnum(CxxType):
    def __init__(self, enum):
        bits = enum.get_width()
        self.default =    "{}"
        self.underlying = enum
        self.ptype =      str
        self.name =       enum.name
        self.bits =       bits
        
        self.jname =      to_jname(enum.name)
        self.jdefault =   self.jname + "." + JS_NULL_ENUM
        
        for k in CXX_STD_INTEGERS:
            if k >= bits:
                self.is_bitfield =  k != bits
                self.size =         k / 8
                break
            assert k != 64
        
    def declare(self, field):
        field = to_cname(field)
        if self.is_bitfield:
             field += " : " + str(self.bits)
        return [to_cname(self.name), field, " = " + self.default + ";"]
    def write(self, value):
        if value == None or len(value) == 0:
            value = CXX_NULL_ENUM
        assert isinstance(value, str)
        if value[0] == "@":
            i = value.index(".")
            assert i != -1
            value = value[i+1:]
        return "{0}::{1}".format(to_cname(self.name), to_cname(value))
        
    def jdeclare(self, field):
        field = to_jname(field, value=True)
        return [field+":", self.jname, " = " + self.jdefault + ";"]
    def jwrite(self, value):
        if value == None or len(value) == 0:
            value = JS_NULL_ENUM
        assert isinstance(value, str)
        if value[0] == "@":
            i = value.index(".")
            assert i != -1
            value = value[i+1:]
        return "{0}.{1}".format(self.jname, to_jname(value))
class CxxString(CxxType):
    def __init__(self):
        self.default = "{}"
        self.ptype =   str
        self.name =    "std::string_view"
        self.jname =   "string"
        self.jdefault = '""'
        self.size =    16
    def write(self, value):
        if value == None:
            return self.default
        return '"{0}"'.format(value)
    def jwrite(self, value):
        if value == None:
            return self.jdefault
        return '"{0}"'.format(value)
        
C_STRING = CxxString()
C_BOOL =   CxxInteger(1,  False)
C_INT8 =   CxxInteger(8,  True)
C_INT16 =  CxxInteger(16, True)
C_INT32 =  CxxInteger(32, True)
C_INT64 =  CxxInteger(64, True)
C_UINT8 =  CxxInteger(8,  False)
C_UINT16 = CxxInteger(16, False)
C_UINT32 = CxxInteger(32, False)
C_UINT64 = CxxInteger(64, False)

def to_cxx_int(bits, signed = False, packed = False):
    if packed and not (bits in CXX_STD_INTEGERS):
        if signed:
            bits = max(bits, 2)
        return CxxInteger(bits, signed)
    if bits <= 8:
        return signed and C_INT8 or C_UINT8
    if bits <= 16:
        return signed and C_INT16 or C_UINT16
    if bits <= 32:
        return signed and C_INT32 or C_UINT32
    if bits <= 64:
        return signed and C_INT32 or C_UINT64
    raise Exception("Tried to create a Cxx integer of {0} bits.".format(bits))

def to_cxx_common_type(t1, t2, packed = CXX_PACK_INTEGERS):
    # Handle identity.
    #
    if t1 == t2:
        return t1
    if t1 == None:
        if isinstance(t2, CxxArray):
            return CxxArray(t2.underlying, None)
        return t2
    if t2 == None:
        if isinstance(t1, CxxArray):
            return CxxArray(t1.underlying, None)
        return t1
    
    # Handle enum types.
    #
    if isinstance(t1, CxxEnum) and isinstance(t2, CxxEnum):
        if t1.underlying.name == t2.underlying.name:
            return t1
    
    # Handle integer promotion:
    #
    if isinstance(t1, CxxInteger) and isinstance(t2, CxxInteger):
        # Return largest:
        if t1.signed == t2.signed:
            if t1.bits > t2.bits:
                return t1
            else:
                return t2
        # Promote with signed mismatch
        t1b = t1.bits + (0 if t1.signed else 1)
        t2b = t2.bits + (0 if t2.signed else 1)
        return to_cxx_int(min(max(t1b, t2b), 64), signed = True, packed = packed)
    
    # Handle array demotion:
    #
    if isinstance(t1, CxxArray) and isinstance(t2, CxxArray):
        if t1.length != t2.length:
            return CxxArray(to_cxx_common_type(t1.underlying, t2.underlying, packed), None)
        if t1.underlying == t2.underlying:
            return t1
        return CxxArray(to_cxx_common_type(t1.underlying, t2.underlying, packed), t1.length)
    
    # Error.
    raise Exception("Cannot convert from {0} to {1}", t2.name, t1.name)
    
def to_cxx_type(value, packed = True, scope = None):
    # Null.
    #
    if value == None:
        return None
    
    # Trivial types.
    #
    if isinstance(value, str):
        # Enum-escape.
        if scope:
            if len(value) != 0 and value[0] == "@":
                i = value.index(".")
                if i != -1:
                    # @arch.x86_64 x86_64 arch
                    enum_value = value[i+1:]
                    enum_type  = value[1:i]
                    enum_type = scope.find(enum_type)
                    if isinstance(enum_type, Enum):
                        if enum_type.index(enum_value) == -1:
                            raise Exception("Enum type {0} does not contain {1}.", enum_type.name, enum_value)
                    return CxxEnum(enum_type)
        return C_STRING
    if isinstance(value, bool):
        return C_BOOL
    if isinstance(value, int):
        if packed:
            if value > 0:
                return to_cxx_int(bitcount(value),        signed = False, packed = True)
            elif value == 0:
                return to_cxx_int(1,                      signed = False, packed = True)
            else:
                return to_cxx_int(bitcount(-(value+1))+1, signed = True, packed = True)
        elif value < 0:
            if bit_intmin(8) < value:
                return C_INT8
            elif bit_intmin(16) < value:
                return C_INT16
            elif bit_intmin(32) < value:
                return C_INT32
            else:
                return C_INT64
        else:
            if bit_uintmax(8) > value:
                return C_UINT8
            elif bit_uintmax(16) > value:
                return C_UINT16
            elif bit_uintmax(32) > value:
                return C_UINT32
            else:
                return C_UINT64
    # List types.
    #
    if isinstance(value, list):
        if len(value) == 0:
            return None
        ty = to_cxx_type(value[0], packed, scope)
        for i in range(1, len(value)):
            ty = to_cxx_common_type(ty, to_cxx_type(value[i], packed, scope))
        return CxxArray(ty, len(value))
    raise Exception("Failed to convert type '{0}' into a cxx type.", type(value))
    
# Declaration types.
#
class Decl:
    # Parser list.
    #
    Parser = {}
    
    # Constructed by the parent, name and the raw TOML body.
    #
    def __init__(self, parent, name, data = None):
        # Name
        self.name =      name
        # All user-specified properties.
        self.properties = {}
        # Parent
        self.parent =     parent
        # JS suffix
        self.jsuffix =    ""
        
        if data != None:
            for k, v in data.items():
                # Properties.
                if k[0].isupper():
                    self.properties[k] = v
                    
        if "Script" in self.properties:
            script = self.properties["Script"]
            multiline_eval(script, globals(), {"parent": parent, "name": name, "data": data, "invokingDecl":self})
                
    # Writer functions.
    #
    def write(self):
        return ""
    def post_write(self):
        return ""
    def post_jwrite(self):
        return self.jsuffix
    def expand(self):
        pass
    
    # Creates a new declaration.
    #
    def create(self, ty, name, data):
        assert self.parent != None
        return self.parent.create(ty, name, data)

    # Finds a declaration.
    #
    def find(self, name):
        assert self.parent != None
        return self.parent.find(name)

    # Gets the namepace name.
    #
    def get_ns(self):
        assert self.parent != None
        return self.parent.get_ns()
    
# Enumeration types.
#
class EnumFwd:
    def __init__(self, name):
        self.name = name
        pass
    def get_width(self):
        return 32
class Enum(Decl):
    # Constructed by the parent, name and the raw TOML body.
    #
    def __init__(self, parent, name, data):
        super().__init__(parent, name, data)
        
        # List of value names.
        self.values =     []
        # List of associated data with each choice.
        self.data =       []
        # Descriptor structure.
        self.desc =       None
        self.desc_init =  None
        # Visitor name.
        self.visitor_name = ("RC_VISIT_" + self.get_ns().replace("retro::","").replace("::","_") + "_" + to_cname(self.name)).upper()
    
        for k, v in data.items():
            # Values.
            if not k[0].isupper():
                assert k != CXX_NULL_ENUM
                self.values.append(k)
                self.data.append(v)
    
    # Gets the bit-width of the enumerator.
    #
    def get_width(self):
        x = len(self.values) + 1
        return bitcount(x-1)
    
    # Gets the underlying C-type associated.
    #
    def get_underlying(self):
        return to_cxx_int(self.get_width(), signed=False, packed=False)
    
    # Gets the index of a value by name.
    #
    def index(self, val):
        return self.values.index(val)
                
    # Writer.
    #
    def write(self):
        # Create the choice list.
        #
        choices = [to_cname(k) for k in self.values]
        choices.insert(0, CXX_NULL_ENUM)

        # Get the width and maximum name length.
        #
        name_width = max_name_len(choices)

        # Get the c-type associated.
        #
        ctype = self.get_underlying()

        # Write the enum.
        #
        width = self.get_width()
        out =  "\nenum class {0} : {1} /*:{2}*/ {{\n".format(to_cname(self.name), ctype.name, width)
        nextval = 0
        for k in choices:
            out +=     "\t{0} = {1},\n".format(k.ljust(name_width), nextval)
            nextval += 1
        out +=     "\t// PSEUDO\n"
        out +=     "\t{0} = {1},\n".format("last".ljust(name_width), nextval-1)
        out +=     "\t{0} = {1},\n".format("bit_width".ljust(name_width), width)
        out += "};"
        
        # Write the visitor.
        #
        max_visitor_list_len = 1
        for i in range(len(self.values)):
            if "VisitorArgs" in self.data[i]:
                max_visitor_list_len = max(max_visitor_list_len, 1+len(self.data[i]["VisitorArgs"]))
        visitor_list = [[""]*max_visitor_list_len]*len(self.values)
        
        out += "\n#define {0}(_)".format(self.visitor_name)
        for i in range(len(self.values)):
            visitor_list[i][0] = to_cname(self.values[i])
            if "VisitorArgs" in self.data[i]:
                args = self.data[i]["VisitorArgs"]
                for j in range(len(args)):
                    visitor_list[i][j+1] = str(args[j])
            out += " _(" + (",".join(visitor_list[i])) + ")" 
        return out
    def jwrite(self):
        # Create the choice list.
        #
        choices = [to_jname(k) for k in self.values]
        choices.insert(0, JS_NULL_ENUM)

        # Get the width and maximum name length.
        #
        name_width = max_name_len(choices)

        # Write the enum.
        #
        width = self.get_width()
        out =  "\n// prettier-ignore\nexport enum {0} {{\n".format(to_jname(self.name))
        nextval = 0
        for k in choices:
            out +=     "\t{0} = {1},\n".format(k.ljust(name_width), nextval)
            nextval += 1
        out +=     "\t// PSEUDO\n"
        out +=     "\t{0} = {1},\n".format("MAX".ljust(name_width), nextval-1)
        out +=     "\t{0} = {1},\n".format("BIT_WIDTH".ljust(name_width), width)
        out += "}"
        return out
    
    # Post writer.
    #
    def post_write(self):
        # Must have been expanded.
        assert self.desc != None
        
        # Start the declaration.
        #
        out = CXX_ARR_DCL_FMT.format(self.desc.name, to_cname(self.name))
        out += "\t{\""+CXX_NULL_ENUM+"\"},\n"
        idx = 0
        for k in self.values:
            out += "\t{" + ",".join([self.desc_init[f.name][idx] for f in self.desc.fields]) + "},\n"
            idx = idx + 1
        out += "};\n"
        
        # Add the forwarded helpers.
        #
        out += CXX_HELP_DCL_FMT.format(to_cname(self.name))
        return out#
    def post_jwrite(self):
        # Must have been expanded.
        assert self.desc != None
        
        # Start the declaration.
        #
        out = self.jsuffix + JS_ARR_DCL_FMT.format(to_jname(self.desc.name), to_jname(self.name))
        out += "\tnew "+to_jname(self.desc.name)+"(),\n"
        idx = 0
        for k in self.values:
            out += "\t{" + ",".join([
                to_jname(f.name, value=True)+":"+self.desc_jinit[f.name][idx] 
                for f in self.desc.fields]) + "},\n"
            idx = idx + 1
        out += "]\n"
        out += JS_ENUM_SUFFIX.format(to_jname(self.name), to_jname(self.desc.name))
        return out
    
    # Expander.
    #
    def expand(self):
        # Skip if already expanded.
        #
        if self.desc != None:
            return
        
        # Flatten the initializers into a matrix.
        #
        init_list = {}
        idx =    0
        for entry in self.data:
            for k,v in entry.items():
                if not k[0].isupper():
                    if not (k in init_list):
                        init_list[k] = [None] * len(self.data)
                    init_list[k][idx] = v
            idx += 1        
                
        # Append the name field if it does not already exist.
        #
        if not ("name" in init_list):
            init_list["name"] = [entry for entry in self.values]
        
        # Find a common type for each field and convert each initialization value to cxx values.
        #
        fields = {}
        self.desc_init = {}
        self.desc_jinit = {}
        for name, values in init_list.items():
            ty = None
            for value in values:
                tx = to_cxx_type(value, True, self)
                ty = to_cxx_common_type(ty, tx)
            assert ty != None
            fields[name] =         ty
            self.desc_init[name] = [ty.write(x) for x in values]
            self.desc_jinit[name] = [ty.jwrite(x) for x in values]
            
        # Create a new descriptor struct.
        #
        self.desc = self.create(Struct, to_cname(self.name) + CXX_DESC_SUFFIX, fields)
        
        # Add the suffix for lookup.
        #
        self.desc.suffix = CXX_HELP_FWD_FMT.format(to_cname(self.name))
    
Decl.Parser["Enum"] = Enum

# Namespace type.
#
class Namespace(Decl):
    # Constructed by the parent, name and the raw TOML body.
    #
    def __init__(self, parent, name, data):
        super().__init__(parent, name, data)
        
        # Declarations.
        self.decls = {}
        
        # For each declaration:
        #
        for k,v in data.items():
            if not k[0].isupper():
                if isinstance(v, list):
                    newv = {}

                    if isinstance(v[0], str):
                        for k2 in v:
                            assert isinstance(k2, str)
                            newv[k2] = {}
                    elif isinstance(v[0], dict):
                        for k2 in v:
                            assert isinstance(k2, dict)
                            assert "List" in k2
                            for k3 in k2.pop("List"):
                                data = k2.copy()
                                newv[k3] = k2
                    if len(newv) == 0:
                        raise Exception("Invalid shorthand table decl.")
                    v = newv
                
                # Get the type:
                #
                ty = v.get("Type", "Enum")
                
                # Pass to the parser and insert into type list.
                #
                unit = Decl.Parser[ty](self, k, v)
                unit.parent = self
                self.decls[k] = unit
                
        # Expand each type.
        #
        for v in list(self.decls.values()):
            v.expand()
        
    # Writer.
    #
    def write(self):
        ext_suffix = ""
        out = ""
        for k in self.decls:
            if isinstance(self.decls[k], Enum):
                print("Generating {}...".format(k))
                ext_suffix += CXX_NS_SUFFIX_PER_ENUM.format(self.name, to_cname(self.decls[k].name), self.decls[k].visitor_name)
                out += self.decls[k].write()
        out += "\n\n" + make_box("Descriptors")
        for k in self.decls:
            if not isinstance(self.decls[k], Enum):
                print("Generating {}...".format(k))
                out += self.decls[k].write()
        out += "\n\n" + make_box("Tables")
        for k in self.decls:
            out += self.decls[k].post_write()
        return "// clang-format off\nnamespace {0} {{{1}\n}};\n{2}// clang-format on".format(self.name, shift_right(out), ext_suffix)
    
    def jwrite(self):
        out = ""
        for k in self.decls:
            if isinstance(self.decls[k], Enum):
                print("JS-Generating {}...".format(k))
                out += self.decls[k].jwrite()
        out += "\n\n" + make_box("Descriptors")
        for k in self.decls:
            if not isinstance(self.decls[k], Enum):
                print("JS-Generating {}...".format(k))
                out += self.decls[k].jwrite()
        out += "\n\n" + make_box("Tables")
        for k in self.decls:
            out += self.decls[k].post_jwrite()
        out += self.jsuffix
        return out
    
    # Creates a new declaration.
    #
    def create(self, ty, name, data):
        unit = ty(self, name, data)
        self.decls[name] = unit
        return unit
    # Finds a declaration.
    #
    def find(self, name):
        if name in self.decls:
            return self.decls[name]
        elif name in self.properties.get("Forwards", []):
            return EnumFwd(name)
        else:
            raise Exception("Failed to find declaration '{0}'".format(name))
            
    # Gets the namepace name.
    #
    def get_ns(self):
        return self.name
    
Decl.Parser["Namespace"] = Namespace

# Struct type.
#
class StructField:
    # Constructed by type and name.
    #
    def __init__(self, ty, name):
        self.name = name
        self.type = ty
        
    # Comperator for sorting.
    #
    def to_comperator(self):
        a = self.name != "name"
        b = float("-inf")
        c = 0
        if isinstance(self.type, CxxInteger) or isinstance(self.type, CxxEnum):
            b = -self.type.size
            c = self.type.bits
        return (a,b,c)
    def __lt__(self, other):
        return self.to_comperator() < other.to_comperator()
        
    
class Struct(Decl):
    # Constructed by list of flattened field-value pairs to determine field types from, name and the parent.
    #
    def __init__(self, parent, name, fields):
        super().__init__(parent, name)
        self.suffix = ""
        
        # Create the field list and sort it.
        #
        self.fields = [StructField(t,n) for n,t in fields.items()]
        self.fields.sort()
        
    # Writer.
    #
    def write(self):
        # Convert field-list into [type-as-str, name-as-str, default-as-str].
        #
        fields = [f.type.declare(f.name) for f in self.fields]
        
        # Pad type-name by max, merge into field name, recalculate pad.
        #
        pad = max_name_len(fields)
        fields = [[k[0].ljust(pad) + " " + k[1], k[2]] for k in fields]
        pad = max_name_len(fields)
        
        # Write the structure.
        #
        out =  "\nstruct {0} {{\n".format(self.name)
        for k in fields:
            out +=     "\t{0}{1}\n".format(k[0].ljust(pad), k[1])
        out += self.suffix
        out += "};"
        return out
    def jwrite(self):
        # Convert field-list into [name-as-str, type-as-str, default-as-str].
        #
        fields = [f.type.jdeclare(f.name) for f in self.fields]
        
        # Pad field-name by max, merge into type name, recalculate pad.
        #
        pad = max_name_len(fields)
        fields = [[k[0].ljust(pad) + " " + k[1], k[2]] for k in fields]
        pad = max_name_len(fields)
        
        # Write the structure.
        #
        out =  "\n// prettier-ignore\nexport class {0} {{\n".format(to_jname(self.name))
        for k in fields:
            out +=     "\t{0}{1}\n".format(k[0].ljust(pad), k[1])
        out += "}"
        return out
    
cache = {}


CXX_DIR_FUNC = """static bool {name}(ins* i, match_context& ctx){{
{mbody}
ins* it = i;
{wbody}
i->replace_all_uses_with({wname});
return true;
}}
"""
def generate_directive_table(data):
    global dtmpcounter
    dtmpcounter = 0
    
    result = "#include <retro/directives/pattern.hpp>\n"
    result += "#ifndef __INTELLISENSE__\n"
    result += "#if RC_CLANG\n"
    result += "    #pragma clang diagnostic ignored \"-Wunused-variable\"\n"
    result += "#elif RC_GNU\n"
    result += "    #pragma GCC diagnostic ignored \"-Wunused-variable\"\n"
    result += "#endif\n\n"
    result += "using op =  retro::ir::op;\n"
    result += "using opr = retro::ir::operand;\n"
    result += "using imm = retro::ir::constant;\n"
    result += "using ins = retro::ir::insn;\n"
    result += "using namespace retro::directives;\n"
    result += "using namespace retro::pattern;\n\n"
    
    init = []
    for k,v in data.items():
        flist = []
        for e in v:
            srcx = parse_expr(e["src"])
            dstx = parse_expr(e["dst"])
            for srcp in srcx.permutate():
                dtmpcounter += 1
                name = "__{0}_pattern__{1}".format(k, dtmpcounter)
                flist.append("&"+name)

                wbody,wname = dstx.write_create()
                result += "\n" + CXX_DIR_FUNC.format(
                    name=name, 
                    mbody=srcp.write_match("i"), 
                    wbody=wbody, wname=wname
                )
        init.append("\t{0}_list.insert({0}_list.end(), {{ {1} }});".format(k, ",".join(flist)))
    result += "\nRC_INITIALIZER {\n"
    result += "\n".join(init)
    result += "\n};\n"
    result += "#endif\n"
    return result

def generate_all(root):
    global cache
    root = os.path.abspath(root)
    
    # Sort the file list so that we parse ".d.toml" after the op table.
    #
    filelist = list(glob.iglob(root + '/**/*.toml', recursive=True))
    filelist.sort(key = lambda a: a.endswith(".d.toml"))
    
    # For each file:
    #
    for file in filelist:
        # Find the namespace.
        #
        idx = file.find("include")
        if idx == -1:
            continue
        namespace = os.path.dirname(file)[idx+len("include")+1:].replace("/", "\\").replace("\\", "::")
        
        # Read the file.
        #
        filedata = None
        with open(file, "r") as inf:
            filedata = inf.read()
        if file in cache and cache[file] == filedata:
            continue
        cache[file] = filedata
        tomldata =    toml.loads(filedata)
        
        # If operator decl, update the operator map.
        #
        fname =       os.path.basename(file)
        if fname == "ops.toml":
            update_operators(tomldata)
        
        # Write the document:
        #
        is_drc = fname.endswith(".d.toml")
        print("\n[{0}] {1}".format(namespace, fname))
        out = file.rsplit(".",1)[0] + (".cxx" if is_drc else ".hxx")
        with open(out, "w") as outf:
            if is_drc:
                outf.write(generate_directive_table(tomldata))
                continue
            
            ns = Namespace(None, namespace, tomldata)
            includes = ns.properties.get("Includes", [])
            includes.insert(0, "<retro/common.hpp>")

            result =  "#pragma once\n"
            for inc in includes:
                result += "#include " + inc + "\n"

            result += "\n" + ns.write()
            outf.write(result)
                
            outjs = file.rsplit(".",1)[0].replace("libretro\\include\\retro", "retrosrv\\src\\lib") + ".ts"
            os.makedirs(os.path.dirname(outjs), exist_ok=True)
            with open(outjs, "w") as outjsf:
                result = ""
                for path, types in ns.properties.get("JIncludes", {}).items():
                    result += "import {{ {0} }} from \"{1}\";\n".format(", ".join(types), path)        
                result += ns.jwrite()
                
                outjsf.write(result.strip('\n') + "\n")

            
def in_notebook():
    try:
        get_ipython().__class__.__name__
        return True
    except NameError:
        return False
    
def main():
    if in_notebook():
        generate_all(os.path.abspath(os.path.curdir + "\\.."))
    else:
        path = os.path.dirname(os.path.realpath(__file__)) + "\\.."

        if len(sys.argv) <= 1:
            print("Watching directory {0} for changes.".format(path))
            while True:
                try:
                    generate_all(path)
                except Exception:
                    print("##### Exception #####")
                    traceback.print_exc()
                    time.sleep(0.3)
        elif len(sys.argv) >= 2:
            path = sys.argv[1]
        generate_all(path)
main()


# In[ ]:





# In[ ]:




