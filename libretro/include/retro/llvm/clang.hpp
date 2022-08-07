#pragma once
#include <retro/common.hpp>
#include <vector>
#include <string>
#include <string_view>

// Wraps the binary interface to clang and clang tools.
//
namespace retro::llvm {
	// Attempts to find the LLVM setup directory, returns nullptr on failure.
	// - Note that this result is for invoking the binaries using shell so might not be a real directory.
	//
	const std::string* locate_install(std::string at = {});

	// Compiles a given C/C++ snippet using Clang, returns an empty vector on failure and sets err_out if given.
	//
	std::vector<u8> compile(std::string_view source, std::string_view arguments, std::string* err_out = nullptr);

	// Formats a given C++ snippet using clang-format, returns an empty string on failure and sets err_out if given.
	//
	std::string format(std::string_view source, std::string_view style = {}, std::string* err_out = nullptr);
};