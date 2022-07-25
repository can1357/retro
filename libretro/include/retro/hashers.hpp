#pragma once
#include <retro/common.hpp>
#include <retro/intrin.hpp>
#include <bit>

namespace retro {
	// Sparse string hasher.
	//
	struct sparse_hasher {
		RC_INLINE size_t operator()(std::string_view v) const noexcept {
			const char* str = v.data();
			u32			len = (u32) v.size();

#if RC_CRC32C
			u32 crc = len;
			if (len >= 4) {
				crc = intrin::crc32c(crc, *(const u32*) (str));
				crc = intrin::crc32c(crc, *(const u32*) (str + len - 4));
				crc = intrin::crc32c(crc, *(const u32*) (str + (len >> 1) - 2));
				crc = intrin::crc32c(crc, *(const u32*) (str + (len >> 2) - 1));
			} else {
				crc = intrin::crc32c(crc, *(const u8*) str);
				crc = intrin::crc32c(crc, *(const u8*) (str + len - 1));
				crc = intrin::crc32c(crc, *(const u8*) (str + (len >> 1)));
			}
			return crc;
#else
			u32 a, b;
			u32 h = len ^ 0xd3cccc57;
			if (len >= 4) {
				a = *(const u32*) (str);
				h ^= *(const u32*) (str + len - 4);
				b = *(const u32*) (str + (len >> 1) - 2);
				h ^= b;
				h -= std::rotl(b, 14);
				b += *(const u32*) (str + (len >> 2) - 1);
			} else {
				a = *(const u8*) str;
				h ^= *(const u8*) (str + len - 1);
				b = *(const u8*) (str + (len >> 1));
				h ^= b;
				h -= std::rotl(b, 14);
			}
			a ^= h;
			a -= std::rotl(h, 11);
			b ^= a;
			b -= std::rotl(a, 25);
			h ^= b;
			h -= std::rotl(b, 16);
			return h;
#endif
		}
	};

	// FNV1-A.
	//
	namespace detail {
		template<typename T, T Offset, T Prime>
		struct fnv1a_impl {
			static constexpr T offset = Offset;
			static constexpr T prime  = Prime;

			// Hashers for each size of integral.
			//
			RC_INLINE constexpr T operator()(u8 data, T hash = Offset) const noexcept {
				hash ^= data;
				hash *= Prime;
				return hash;
			}
			RC_INLINE constexpr T operator()(u16 data, T hash = Offset) const noexcept {
				hash = operator()(u8(data & 0xFF), hash);
				hash = operator()(u8((data >> 8) & 0xFF), hash);
				return hash;
			}
			RC_INLINE constexpr T operator()(u32 data, T hash = Offset) const noexcept {
				hash = operator()(u16(data & 0xFFFF), hash);
				hash = operator()(u16((data >> 16) & 0xFFFF), hash);
				return hash;
			}
			RC_INLINE constexpr T operator()(u64 data, T hash = Offset) const noexcept {
				hash = operator()(u32(data & 0xFFFFFFFF), hash);
				hash = operator()(u32((data >> 32) & 0xFFFFFFFF), hash);
				return hash;
			}
			RC_INLINE constexpr T operator()(i8 data, T hash = Offset) const noexcept { return operator()(u8(data), hash); }
			RC_INLINE constexpr T operator()(i16 data, T hash = Offset) const noexcept { return operator()(u16(data), hash); }
			RC_INLINE constexpr T operator()(i32 data, T hash = Offset) const noexcept { return operator()(u32(data), hash); }
			RC_INLINE constexpr T operator()(i64 data, T hash = Offset) const noexcept { return operator()(u64(data), hash); }
		};
	};
	
	using fnv1a_64_hasher = detail::fnv1a_impl<u64, 0xcbf29ce484222325, 0x00000100000001b3>;
	using fnv1a_32_hasher = detail::fnv1a_impl<u32, 0x811c9dc5, 0x01000193>;
#if RC_32
	using fnv1a_hasher = fnv1a_32_hasher;
#else
	using fnv1a_hasher = fnv1a_64_hasher;
#endif

	// Functional versions.
	//
	inline static constexpr fnv1a_hasher	 fnv1a_hash		= {};
	inline static constexpr fnv1a_32_hasher fnv1a_32_hash = {};
	inline static constexpr fnv1a_64_hasher fnv1a_64_hash = {};
	inline static constexpr sparse_hasher	 sparse_hash	= {};
};