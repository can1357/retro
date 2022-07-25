#pragma once
#include <retro/common.hpp>
#include <array>
#include <span>
#include <vector>
#include <string>
#include <optional>
#include <Zycore/LibC.h>
#include <Zydis/Zydis.h>

namespace retro::zydis {
	// Rename registers.
	//
	using reg = ZydisRegister;

	// Decoding.
	//
	struct decoded_ins {
		ZydisDecodedInstruction ins;
		ZydisDecodedOperand     ops[ZYDIS_MAX_OPERAND_COUNT_VISIBLE];

		std::span<const ZydisDecodedOperand> operands() const { return {ops, ins.operand_count}; }

		// Formatting.
		//
		std::string to_string(u64 ip = 0) const {
			char           buffer[128];
			ZydisFormatter formatter;
			ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);
			if (ZYAN_FAILED(ZydisFormatterFormatInstruction(&formatter, &ins, ops, ins.operand_count_visible, buffer, sizeof(buffer), ip))) {
				return "??";
			} else {
				return std::string(&buffer[0]);
			}
		}
	};
	static std::optional<decoded_ins> decode(std::span<const u8>& in, ZydisMachineMode mode = ZYDIS_MACHINE_MODE_LONG_64) {
		ZydisStackWidth width;
		switch (mode) {
			default:
			case ZYDIS_MACHINE_MODE_LONG_64:
				width = ZYDIS_STACK_WIDTH_64;
				break;
			case ZYDIS_MACHINE_MODE_LEGACY_32:
			case ZYDIS_MACHINE_MODE_LONG_COMPAT_32:
				width = ZYDIS_STACK_WIDTH_32;
				break;
			case ZYDIS_MACHINE_MODE_LONG_COMPAT_16:
			case ZYDIS_MACHINE_MODE_LEGACY_16:
			case ZYDIS_MACHINE_MODE_REAL_16:
				width = ZYDIS_STACK_WIDTH_16;
				break;
		}
		std::optional<decoded_ins> result;
		ZydisDecoder               decoder;
		ZydisDecoderInit(&decoder, mode, width);
		auto& out = result.emplace();
		if (ZYAN_FAILED(ZydisDecoderDecodeFull(&decoder, in.data(), in.size(), &out.ins, out.ops, ZYDIS_MAX_OPERAND_COUNT_VISIBLE, ZYDIS_DFLAG_VISIBLE_OPERANDS_ONLY))) {
			result.reset();
		} else {
			in = in.subspan(out.ins.length);
		}
		return result;
	}
};