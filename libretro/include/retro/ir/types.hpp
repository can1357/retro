#pragma once
#include <retro/ir/builtin_types.hxx>
#include <retro/ir/ops.hxx>
#include <retro/targets.hxx>
#include <retro/format.hpp>

// Scoped enums to create alternative types for some builtins.
//
namespace retro::ir {
	enum pointer : u64 {};
	struct value_pack_t {
		std::string to_string() const { return "(...)"; }
	};
};

// Define the formatters.
//
namespace std {
	inline std::string to_string(retro::ir::pointer p) { return fmt::str("0x%p", (uptr) p); }
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
	RC_VISIT_TYPE(MAP_VTY)

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
	concept BuiltinType = requires { type_to_builtin<T>::value; } && (std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_same_v<T, std::string_view>);

	template<type Id>
	using type_t = typename builtin_to_type<Id>::type;
	template<typename T>
	constexpr type type_v = type_to_builtin<T>::value;
	
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
				dst = operator new(length);
				ptr = dst;
			}
			memcpy(dst, src, length);
		}

		// String alternatives.
		//
		constant(const std::string& str) : constant(std::string_view{str}) {}
		constant(const char* str) : constant(std::string_view{str}) {}

		// Copy construction and assignment.
		//
		constant(const constant& o) {
			type_id		= o.type_id;
			data_length = o.data_length;
			if (is_large()) {
				void* dst = operator new(data_length);
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
		constant(constant&& o) noexcept { swap(o); }
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
		template<typename T>
		constexpr bool is() const {
			constexpr type Id = type_v<T>;
			return type_id == u64(Id);
		}
		template<typename T>
		decltype(auto) get() const {
			constexpr type Id = type_v<T>;
			RC_ASSERT(type_id == u64(Id));
			if constexpr (Id == type::str) {
				return std::string_view{(char*) address(), size()};
			} else if (sizeof(T) > sizeof(data)) {
				return *(const T*) ptr;
			} else {
				return *(const T*) &data[0];
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

		// Reset handling freeing of the data.
		//
		constexpr void reset() {
			if (is_large()) {
				operator delete(ptr);
			}
			data_length = 0;
			type_id		= (u64) type::none;
		}

		// String conversion.
		//
		std::string to_string() const {
			switch (get_type()) {
#define FMT_TY(A, B)                                \
	case type::A: {                                  \
		if constexpr (BuiltinType<B>) {               \
			return std::string{fmt::to_str(get<B>())}; \
		} else {                                      \
			RC_UNREACHABLE();                          \
		}                                             \
	}
				RC_VISIT_TYPE(FMT_TY)
#undef FMT_TY
				default:
					return "none";
			}
		}

		// Destruction.
		//
		constexpr ~constant() { reset(); }
	};
};