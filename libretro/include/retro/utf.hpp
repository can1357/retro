#pragma once
#include <bit>
#include <retro/common.hpp>
#include <span>
#include <string_view>

namespace retro::utf {
	template<typename C, bool ForeignEndianness = false>
	struct codepoint_cvt;

	// UTF-8.
	//
	template<typename T, bool ForeignEndianness>
		requires(sizeof(T) == 1)
	struct codepoint_cvt<T, ForeignEndianness> {
		//    7 bits
		// 0xxxxxxx
		//    5 bits  6 bits
		// 110xxxxx 10xxxxxx
		//    4 bits  6 bits  6 bits
		// 1110xxxx 10xxxxxx 10xxxxxx
		//    3 bits  6 bits  6 bits  6 bits
		// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		//
		static constexpr size_t max_out = 4;

		inline static constexpr u8 rlength(T _front) {
			u8 front  = u8(_front);
			u8 result = (front >> 7) + 1;
			result += front >= 0b11100000;
			result += front >= 0b11110000;
			return result;
		}
		inline static constexpr u8 length(u32 cp) {
			u8 result = 1;
			result += bool(cp >> 7);
			result += bool(cp >> (5 + 6));
			result += bool(cp >> (4 + 6 + 6));
			return result;
		}
		inline static constexpr void encode(u32 cp, T*& out) {
			// Handle single character case.
			//
			if (cp <= 0x7F) [[likely]] {
				*out++ = T(cp);
				return;
			}

			auto write = [&]<auto I>(std::integral_constant<size_t, I>) RC_INLINE {
				auto p = out;
				for (size_t i = 0; i != I; i++) {
					u8  flag  = i ? 0x80 : (u8) bit_mask(I, 8 - I);
					u32 value = cp >> 6 * (I - i - 1);
					value &= bit_mask(i ? 6 : 7 - I);
					p[i] = (T) u8(flag | value);
				}

				out += I;
			};

			if ((cp >> 6) <= bit_mask(5)) [[likely]]
				write(std::integral_constant<size_t, 2ull>{});
			else if ((cp >> 12) <= bit_mask(4)) [[likely]]
				write(std::integral_constant<size_t, 3ull>{});
			else
				write(std::integral_constant<size_t, 4ull>{});
		}
		inline static constexpr u32 decode(std::basic_string_view<T>& in) {
			T front = in[0];
			if (i8(front) >= 0) [[likely]] {
				in.remove_prefix(1);
				return front;
			}

			auto read = [&]<auto I>(std::integral_constant<size_t, I>) RC_INLINE->u32 {
				if (in.size() < I) [[unlikely]] {
					in.remove_prefix(in.size());
					return 0;
				}
				u32 cp = (front & bit_mask(7 - I)) << (6 * (I - 1));
				for (size_t i = 1; i != I; i++)
					cp |= u32(in[i] & bit_mask(6)) << (6 * (I - 1 - i));
				in.remove_prefix(I);
				return cp;
			};

			if (u8(front) < 0b11100000) [[likely]]
				return read(std::integral_constant<size_t, 2ull>{});
			if (u8(front) < 0b11110000) [[likely]]
				return read(std::integral_constant<size_t, 3ull>{});
			return read(std::integral_constant<size_t, 4ull>{});
		}
	};

	// UTF-16.
	//
	template<typename T, bool ForeignEndianness>
		requires(sizeof(T) == 2)
	struct codepoint_cvt<T, ForeignEndianness> {
		static constexpr size_t max_out = 2;

		inline static constexpr u8 rlength(T front) {
			if constexpr (ForeignEndianness)
				front = (T) bswap((u16) front);
			u8 result = 1 + ((u16(front) >> 10) == 0x36);
			return result;
		}
		inline static constexpr u8 length(u32 cp) {
			// Assuming valid codepoint outside the surrogates.
			u8 result = 1 + bool(cp >> 16);
			return result;
		}
		inline static constexpr void encode(u32 cp, T*& out) {
			T* p = out;

			// Save the old CP, determine length.
			//
			const u16  word_cp  = u16(cp);
			const bool has_high = cp != word_cp;
			out += has_high + 1;

			// Adjust the codepoint, encode as extended.
			//
			cp -= 0x10000;
			u16 lo = 0xD800 | u16(cp >> 10);
			u16 hi = 0xDC00 | u16(cp);

			// Swap the beginning with 1-byte version if not extended.
			//
			if (!has_high)
				lo = word_cp;

			// Write the data and return the size.
			//
			if constexpr (ForeignEndianness)
				lo = bswap(lo), hi = bswap(hi);
			p[has_high] = T(hi);
			p[0]        = T(lo);
		}
		inline static constexpr u32 decode(std::basic_string_view<T>& in) {
			// Read the low pair, rotate.
			//
			u16 lo = in[0];
			if constexpr (ForeignEndianness)
				lo = bswap(lo);
			u16 lo_flg = lo & bit_mask(6, 10);

			// Read the high pair.
			//
			const bool has_high = lo_flg == 0xD800 && in.size() != 1;
			u16        hi       = in[has_high];
			if constexpr (ForeignEndianness)
				hi = bswap(hi);

			// Adjust the codepoint accordingly.
			//
			u32 cp = hi - 0xDC00 + 0x10000 - (0xD800 << 10);
			cp += lo << 10;
			if (!has_high)
				cp = hi;

			// Adjust the string view and return.
			//
			in.remove_prefix(has_high + 1);
			return cp;
		}
	};

	// UTF-32.
	//
	template<typename T, bool ForeignEndianness>
		requires(sizeof(T) == 4)
	struct codepoint_cvt<T, ForeignEndianness> {
		static constexpr size_t max_out = 1;

		inline static constexpr u8   rlength(T) { return 1; }
		inline static constexpr u8   length(u32) { return 1; }
		inline static constexpr void encode(u32 cp, T*& out) {
			if constexpr (ForeignEndianness)
				cp = bswap(cp);
			*out++ = (T) cp;
		}
		inline static constexpr u32 decode(std::basic_string_view<T>& in) {
			u32 cp = (u32) in.front();
			in.remove_prefix(1);
			if constexpr (ForeignEndianness)
				cp = bswap(cp);
			return cp;
		}
	};

	template<typename To, typename From, bool Foreign = false>
	inline static std::basic_string<To> convert(std::basic_string_view<From> in) {
		// Construct a view and compute the maximum length.
		//
		std::basic_string_view<From> view{in};
		size_t                        max_out = codepoint_cvt<To>::max_out * view.size();

		// Reserve the maximum length and invoke the ranged helper.
		//
		std::basic_string<To> result(max_out, '\0');

		// Re-encode every character.
		//
		To* it = result.data();
		while (!in.empty()) {
			u32 cp = codepoint_cvt<From, Foreign>::decode(in);
			codepoint_cvt<To>::encode(cp, it);
		}

		// Shrink the buffer and return.
		//
		result.resize(it - result.data());
		return result;
	}

	// Given a string-view, calculates the length in codepoints.
	//
	template<typename C>
	inline static size_t length(std::basic_string_view<C> data) {
		size_t result = 0;
		while (!data.empty()) {
			size_t n = codepoint_cvt<C>::rlength(data.front());
			data.remove_prefix(std::min(n, data.size()));
			result++;
		}
		return result;
	}

	// Given a string-view, strips UTF-8 byte order mark if included, otherwise, returns true if it includes UTF-16/32 marks.
	//
	inline static bool is_bom(std::string& data) {
		// Skip UTF-8.
		//
		if (data.size() >= 3 && !memcmp(data.data(), "\xEF\xBB\xBF", 3)) {
			data.erase(data.begin(), data.begin() + 3);
			return false;
		}

		// Try matching against UTF-32 LE/BE:
		//
		if (std::u32string_view view{(const char32_t*) data.data(), data.size() / sizeof(char32_t)}; !view.empty()) {
			if (view.front() == 0xFEFF || view.front() == bswap<char32_t>(0xFEFF))
				return true;
		}
		// Try matching against UTF-16 LE/BE:
		//
		if (std::u16string_view view{(const char16_t*) data.data(), data.size() / sizeof(char16_t)}; !view.empty()) {
			if (view.front() == 0xFEFF || view.front() == bswap<char16_t>(0xFEFF))
				return true;
		}
		return false;
	}

	// Given a span of raw-data, identifies the encoding using the byte order mark
	// if relevant and re-encodes into the requested codepoint.
	//
	template<typename To>
	inline static std::basic_string<To> convert(std::span<const u8> data) {
		// If stream does not start with UTF8 BOM:
		//
		if (data.size() < 3 || memcmp(data.data(), "\xEF\xBB\xBF", 3)) {
			// Try matching against UTF-32 LE/BE:
			//
			if (std::u32string_view view{(const char32_t*) data.data(), data.size() / sizeof(char32_t)}; !view.empty()) {
				if (view.front() == 0xFEFF) [[unlikely]]
					return convert<To, char32_t, false>(view.substr(1));
				if (view.front() == bswap<char32_t>(0xFEFF)) [[unlikely]]
					return convert<To, char32_t, true>(view.substr(1));
			}
			// Try matching against UTF-16 LE/BE:
			//
			if (std::u16string_view view{(const char16_t*) data.data(), data.size() / sizeof(char16_t)}; !view.empty()) {
				if (view.front() == 0xFEFF)
					return convert<To, char16_t, false>(view.substr(1));
				if (view.front() == bswap<char16_t>(0xFEFF)) [[unlikely]]
					return convert<To, char16_t, true>(view.substr(1));
			}
		}
		// Otherwise remove the header and continue with the default.
		//
		else {
			data = data.subspan(3);
		}

		// Decode as UTF-8 and return.
		//
		return convert<To>(std::u8string_view{(const char8_t*) data.data(), data.size() / sizeof(char8_t)});
	}
};