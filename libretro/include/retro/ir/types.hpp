#pragma once
#include <retro/ir/builtin_types.hxx>
#include <retro/ir/ops.hxx>
#include <retro/arch/mreg.hpp>
#include <retro/format.hpp>
#include <retro/heap.hpp>

// Scoped enums to create alternative types for some builtins.
//
namespace retro::ir {
	enum pointer : u64 {};
	struct value_pack_t {
		std::string to_string() const { return "(...)"; }
	};
	struct context_t {
		std::string to_string() const { return "(...)"; }
	};
};

// Finally declare the type interface.
//
namespace retro::ir {
	struct basic_block;

	// Map builtin type enums to the actual C++ types represeting them, and vice-versa.
	//
	template<type>
	struct builtin_to_type;
	template<typename T>
	struct type_to_builtin;
#define MAP_VTY(A, B)                        \
	template<>                                \
	struct builtin_to_type<type::A> {         \
		using type = B;                        \
	};                                        \
	template<>                                \
	struct type_to_builtin<B> {               \
		static constexpr auto value = type::A; \
	};

	// Map all types.
	//
	RC_VISIT_IR_TYPE(MAP_VTY)

	// Handle unsigned decay and void type.
	//
	template<> struct type_to_builtin<u128> { static constexpr auto value = type::i128; };
	template<> struct type_to_builtin<u64> { static constexpr auto value = type::i64; };
	template<> struct type_to_builtin<u32> { static constexpr auto value = type::i32; };
	template<> struct type_to_builtin<u16> { static constexpr auto value = type::i16; };
	template<> struct type_to_builtin<u8> { static constexpr auto value = type::i8; };
	MAP_VTY(none, void)
#undef MAP_VTY

	template<typename T>
	concept BuiltinType = !type_desc::all()[u32(type_to_builtin<T>::value)].pseudo;

	template<type Id>
	using type_t = typename builtin_to_type<Id>::type;
	template<typename T>
	constexpr type type_v = type_to_builtin<T>::value;

	// Helpers for creating arithmetic types.
	//
	inline constexpr type int_type(size_t n) {
		for (auto& td : type_desc::all()) {
			if (td.kind != type_kind::scalar_int)
				continue;
			if (td.bit_size == n) {
				return td.id();
			}
		}
		return type::none;
	}
	inline constexpr type fp_type(size_t n) {
		for (auto& td : type_desc::all()) {
			if (td.kind != type_kind::scalar_fp)
				continue;
			if (td.bit_size == n) {
				return td.id();
			}
		}
		return type::none;
	}
	inline constexpr type vec_type(type t, size_t n) {
		for (auto& td : type_desc::all()) {
			if (td.kind != type_kind::vector_fp && td.kind != type_kind::vector_int)
				continue;
			if (td.lane_width == n && td.underlying == t) {
				return td.id();
			}
		}
		return type::none;
	}

	// Define constant type.
	//
	struct constant {
		u64 __rsvd : 1			= 1;	// Should be one to act as a thombstone for ir::operand.
		u64 type_id : 8		= 0;	// Type id, ir::type.
		u64 data_length : 55 = 0;	// Data length, used to determine internal/external storage.
		union {
			u8		data[16];		 // Inline storage.
			void* ptr = nullptr;	 // External storage.
		};

		// Default constructor.
		//
		constexpr constant() = default;
		constexpr constant(std::nullopt_t){};

		// Construction by value.
		//
		template<BuiltinType T>
		constant(T value) {
			constexpr type Id = type_v<T>;
			type_id				= (u64) Id;

			// Decompose the type as [ptr, length].
			//
			const void* src;
			size_t		length;
			if constexpr (Id == type::str) {
				length = value.size();
				src	 = value.data();
			} else {
				length = sizeof(T);
				src	 = &value;
			}
			data_length = length;

			// Initialize the buffer.
			//
			void* dst = data;
			if (length > sizeof(data)) {
				dst = heap::allocate(length);
				ptr = dst;
			}
			memcpy(dst, src, length);
		}

		// String alternatives.
		//
		constant(const std::string& str) : constant(std::string_view{str}) {}
		constant(const char* str) : constant(std::string_view{str}) {}

		// Typed construction.
		//
		constant(type t, std::span<const u8> src) : constant() {
			if (!src.empty()) {
				RC_ASSERT(t != type::pointer);

				if (t == type::i1) {
					data[0]		= *(const bool*) src.data();
					data_length = 1;
					return;
				}

				auto bw = enum_reflect(t).bit_size;
				RC_ASSERT(bw != 0 && is_aligned(bw, 8));
				size_t length = bw / 8;
				if (src.size() >= length) {
					type_id		= (u64) t;
					data_length = length;
					void* dst	= data;
					if (length > sizeof(data)) {
						dst = heap::allocate(length);
						ptr = dst;
					}
					memcpy(dst, src.data(), length);
				}
			}
		}
		constant(type t, f32 value);
		constant(type t, f64 value);
		constant(type t, u64 value);
		constant(type t, i64 value);
		constant(type t, i32 value) : constant(t, i64(value)) {}
		constant(type t, u32 value) : constant(t, u64(value)) {}
		constant(type t, i16 value) : constant(t, i64(value)) {}
		constant(type t, u16 value) : constant(t, u64(value)) {}
		constant(type t, i8 value) : constant(t, i64(value)) {}
		constant(type t, u8 value) : constant(t, u64(value)) {}
		constant(type t, bool value) : constant(t, value ? 1u : 0u) {}

		// Copy construction and assignment.
		//
		constant(const constant& o) {
			type_id		= o.type_id;
			data_length = o.data_length;
			if (is_large()) {
				void* dst = heap::allocate(data_length);
				memcpy(dst, o.ptr, data_length);
				ptr = dst;
			} else {
				memcpy(data, o.data, sizeof(data));
			}
		}
		constant& operator=(const constant& o) {
			constant copy{o};
			swap(copy);
			return *this;
		}

		// Move construction and assignment via swap.
		//
		constant(constant&& o) noexcept : constant() { swap(o); }
		constant& operator=(constant&& o) noexcept {
			swap(o);
			return *this;
		}

		// Trivially relocatable.
		//
		void swap(constant& o) {
			using bytes = std::array<u8, sizeof(constant)>;
			std::swap((bytes&) *this, (bytes&) o);
		}

		// Expose the buffer details.
		//
		constexpr size_t		 size() const { return data_length; }
		constexpr bool			 is_large() const { return size() > sizeof(data); }
		constexpr void*		 address() { return is_large() ? ptr : (void*) data; };
		constexpr const void* address() const { return is_large() ? ptr : (void*) data; };

		// Observers.
		//
		constexpr type get_type() const { return type(type_id); }
		constexpr bool is(type t) const { return type_id == u64(t); }
		template<typename T>
		constexpr bool is() const {
			return is(type_v<T>);
		}
		template<typename T>
		decltype(auto) get() {
			constexpr type Id = type_v<T>;
			if (type_id != u64(Id))
				throw std::runtime_error("type mismatch.");

			if constexpr (Id == type::str) {
				return std::string_view{(char*) address(), size()};
			} else if (sizeof(T) > sizeof(data)) {
				return *(T*) ptr;
			} else {
				return *(T*) &data[0];
			}
		}
		template<typename T>
		decltype(auto) get() const {
			constexpr type Id = type_v<T>;
			if (type_id != u64(Id))
				throw std::runtime_error("type mismatch.");

			if constexpr (Id == type::str) {
				return std::string_view{(char*) address(), size()};
			} else if (sizeof(T) > sizeof(data)) {
				return *(const T*) ptr;
			} else {
				return *(const T*) &data[0];
			}
		}

		// Integer getters.
		//
		i64 get_i64() const {
			switch (get_type()) {
				case type::i1:
					return get<bool>() ? 1 : 0;
				case type::i8:
					return get<i8>();
				case type::i16:
					return get<i16>();
				case type::i32:
					return get<i32>();
				case type::pointer:
					return (i64) (u64) get<pointer>();
				case type::i64:
					return get<i64>();
				default:
					throw std::runtime_error("getting non-integer immediate as i64.");
			}
		}
		u64 get_u64() const {
			switch (get_type()) {
				case type::i1:
					return get<bool>() ? 1 : 0;
				case type::i8:
					return get<u8>();
				case type::i16:
					return get<u16>();
				case type::i32:
					return get<u32>();
				case type::pointer:
					return (u64) get<pointer>();
				case type::i64:
					return get<u64>();
				default:
					throw std::runtime_error("getting non-integer immediate as u64.");
			}
		}

		// Equality comparison.
		//
		bool equals(const constant& other) const {
			// Compare data_length and type_id.
			//
			if (*(u64*) this != *(u64*) &other)
				return false;

			// Compare the actual data.
			//
			return !memcmp(address(), other.address(), size());
		}
		bool operator==(const constant& other) const { return equals(other); }
		bool operator!=(const constant& other) const { return !equals(other); }

		// Void check.
		//
		explicit operator bool() const { return get_type() != type::none; }

		// Application of an operator over constants, returns "none" on failure.
		//
		constant apply(op o, const constant& rhs) const;
		constant apply(op o) const { return apply(o, *this); }

		// Cast helpers, returns "none" on failure.
		//
		constant cast_zx(type into) const;
		constant cast_sx(type into) const;
		constant bitcast(type into) const;

		// Reset handling freeing of the data.
		//
		constexpr void reset() {
			if (is_large()) {
				heap::deallocate(ptr);
			}
			data_length = 0;
			type_id		= (u64) type::none;
		}

		// String conversion.
		//
		std::string to_string() const;

		// Destruction.
		//
		constexpr ~constant() { reset(); }
	};
};