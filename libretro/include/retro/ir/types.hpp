#pragma once
#include <retro/ir/builtin_types.hxx>

namespace retro::ir {
	// Map builtin type enums to the actual C++ types represeting them.
	//
	template<type>
	struct builtin_type {
		using type = void;
	};
#define MAP_VTY(A, B)             \
	template<>                     \
	struct builtin_type<type::A> { \
		using type = B;             \
	};
	RC_VISIT_TYPE(MAP_VTY)
#undef MAP_VTY
	template<type Id>
	using builtin_t = builtin_type<Id>;

	// TODO: 
	//
};