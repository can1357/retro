#!/usr/bin/env python
# coding: utf-8

# In[69]:


#!conda install --yes toml
import toml
import glob
import os
import sys
import time
import ast

# Constants
#
TEXT_WIDTH =        120
CXX_PACK_INTEGERS = True
CXX_STD_INTEGERS =  [8, 16, 32, 64]
CXX_HELP_FWD_FMT =  """
	using value_type = {0};
	static constexpr std::span<const {0}_desc> list();
	RC_INLINE constexpr const {0} id() const {{ return {0}(this - list().data()); }}
"""
CXX_HELP_DCL_FMT =  """RC_INLINE constexpr std::span<const {0}_desc> {0}_desc::list() {{ return {0}s; }}"""
CXX_ARR_DCL_FMT =   "\n\ninline constexpr {0} {1}s[] = {{\n"
CXX_DESC_SUFFIX =   "_desc"
CXX_NS_SUFFIX_PER_ENUM = """namespace retro {{ template<> struct descriptor<{0}::{1}> {{ using type = {0}::{1}_desc; }}; }};
"""

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
class CxxInteger(CxxType):
    def __init__(self, bits, signed):
        self.bits =    bits
        self.signed =  signed
        
        if bits == 1:
            assert not signed
            self.default = "false"
            self.ptype =   bool
        else:
            self.default = "0"
            self.ptype =   int

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
            return "true" if value else "false"
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
        if length != None:
            self.name =       "std::array<{0}, {1}>".format(to_cname(underlying.name), length)
            self.size =       underlying.size * length
        else:
            self.name =       "std::initializer_list<{0}>".format(to_cname(underlying.name))
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
class CxxEnum(CxxType):
    def __init__(self, enum):
        bits = enum.get_width()
        self.default =    "{}"
        self.underlying = enum
        self.ptype =      str
        self.name =       enum.name
        self.bits =       bits
        
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
        if value == None:
            assert self.underlying.optional
            value = "none"
        assert isinstance(value, str)
        if value[0] == "@":
            i = value.index(".")
            assert i != -1
            value = value[i+1:]
        return "{0}::{1}".format(to_cname(self.name), to_cname(value))
class CxxString(CxxType):
    def __init__(self):
        self.default = "{}"
        self.ptype =   str
        self.name =    "std::string_view"
        self.size =    16
    def write(self, value):
        if value == None:
            return self.default
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
        if t1.underlying == t2.underlying:
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
        return to_cxx_int(min(max(t1.bits, t2.bits)*2, 64), signed = True, packed = packed)
    
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
            if value[0] == "@":
                i = value.index(".")
                if i != -1:
                    # @arch.x86_64 x86_64 arch
                    enum_value = value[i+1:]
                    enum_type  = value[1:i]
                    enum_type = scope.find(enum_type)
                    assert isinstance(enum_type, Enum)
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
        
        if data != None:
            for k, v in data.items():
                # Properties.
                if k[0].isupper():
                    self.properties[k] = v
                    
        if "Script" in self.properties:
            script = self.properties["Script"]
            multiline_eval(script, globals(), {"parent": parent, "name": name, "data": data})
                
    # Writer functions.
    #
    def write(self):
        return ""
    def post_write(self):
        return ""
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
    
# Enumeration types.
#
class Enum(Decl):
    # Constructed by the parent, name and the raw TOML body.
    #
    def __init__(self, parent, name, data):
        super().__init__(parent, name, data)
        
        # Will this enumerator contain "none"?
        self.optional =   self.properties.get("Optional", True)
        # List of value names.
        self.values =     []
        # List of associated data with each choice.
        self.data =       []
        # Descriptor structure.
        self.desc =       None
        self.desc_init =  None
    
        for k, v in data.items():
            # Values.
            if not k[0].isupper():
                assert k != "none"
                self.values.append(k)
                self.data.append(v)
    
    # Gets the bit-width of the enumerator.
    #
    def get_width(self):
        x = len(self.values)
        if self.optional:
            x += 1
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
        if self.optional:
            choices.insert(0, "none")

        # Get the width and maximum name length.
        #
        name_width = max_name_len(choices)

        # Get the c-type associated.
        #
        ctype = self.get_underlying()

        # Write the enum.
        #
        out =  "\nenum class {0} : {1} /*:{2}*/ {{\n".format(to_cname(self.name), ctype.name, self.get_width())
        nextval = 0
        for k in choices:
            out +=     "\t{0} = {1},\n".format(k.ljust(name_width), nextval)
            nextval += 1
        out +=     "\t{0} = {1},\n".format("last".ljust(name_width), nextval-1)
        out += "};"
        return out
    
    # Post writer.
    #
    def post_write(self):
        # Must have been expanded.
        assert self.desc != None
        
        
        
        # Start the declaration.
        #
        out = CXX_ARR_DCL_FMT.format(self.desc.name, to_cname(self.name))
        if self.optional:
            out += "\t{\"none\"},\n"
        idx = 0
        for k in self.values:
            out += "\t{" + ",".join([self.desc_init[f.name][idx] for f in self.desc.fields]) + "},\n"
            idx = idx + 1
        out += "};\n"
        
        # Add the forwarded helpers.
        #
        out += CXX_HELP_DCL_FMT.format(to_cname(self.name))
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
        for name, values in init_list.items():
            ty = None
            for value in values:
                tx = to_cxx_type(value, True, self)
                ty = to_cxx_common_type(ty, tx) if ty else tx
            assert ty != None
            fields[name] =         ty
            self.desc_init[name] = [ty.write(x) for x in values]
            
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
                ext_suffix += CXX_NS_SUFFIX_PER_ENUM.format(self.name, to_cname(self.decls[k].name))
                out += self.decls[k].write()
        out += "\n\n" + make_box("Descriptors")
        for k in self.decls:
            if not isinstance(self.decls[k], Enum):
                out += self.decls[k].write()
        out += "\n\n" + make_box("Tables")
        for k in self.decls:
            out += self.decls[k].post_write()
        return "// clang-format off\nnamespace {0} {{{1}\n}};\n{2}// clang-format on".format(self.name, shift_right(out), ext_suffix)
    
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
        else:
            raise Exception("Failed to find declaration '{0}'".format(name))
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
    
    
cache = {}
def generate_all(root):
    global cache
    root = os.path.abspath(root)
    for file in glob.iglob(root + '/**/*.toml', recursive=True):
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
        
        print("-- Generating ", namespace)
        out = file.rsplit(".",1)[0] + ".hxx"
        with open(out, "w") as outf:
            ns = Namespace(None, namespace, toml.loads(filedata))
            includes = ns.properties.get("Includes", [])
            includes.append("<retro/common.hpp>")
            
            result =  "#pragma once\n"
            for inc in includes:
                result += "#include " + inc + "\n"
            
            result += "\n" + ns.write()
            outf.write(result)
                    
try:
    # Interactive shell.
    get_ipython().__class__.__name__
    generate_all(os.path.abspath(os.path.curdir + "\\.."))    
except NameError:
    path = os.path.dirname(os.path.realpath(__file__)) + "\\.."
    
    if len(sys.argv) <= 1:
        print("Watching directory {0} for changes.".format(path))
        
        while True:
            try:
                generate_all(path)
            except Exception as e:
                print("Exception:", e)
            time.sleep(0.3)
    elif len(sys.argv) >= 2:
        path = sys.argv[1]
    generate_all(path)


# In[ ]:





# In[ ]:




