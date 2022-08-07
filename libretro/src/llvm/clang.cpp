#include <retro/llvm/clang.hpp>
#include <retro/platform.hpp>
#include <retro/umutex.hpp>
#include <retro/format.hpp>

// Wraps the binary interface to clang and clang tools.
//
namespace retro::llvm {
	static shared_umutex					 install_dir_lock = {};
	static std::optional<std::string> install_dir		= {};
	static std::mutex						 tmp_file_lock		= {};

	// Default list of tried strings.
	//
	static constexpr std::string_view base_directories[] = {
		 "",
#if RC_WINDOWS
		 "%LLVMInstallDir%/bin/",
		 "%LLVM_PATH%/bin/",
#else
		 "/usr/bin/",
		 "~/bin/",
		 "$LLVMInstallDir/bin/",
		 "$LLVM_PATH/bin/",
#endif
	};

	// Attempts to find the LLVM setup directory, returns nullptr on failure.
	// - Note that this result is for invoking the binaries using shell so might not be a real directory.
	//
	const std::string* locate_install(std::string at) {
		// Skip if already found.
		//
		{
			std::shared_lock _g{install_dir_lock};
			if (install_dir) {
				return &install_dir.value();
			}
		}

		// If user has given us something to try, give it a try.
		//
		if (!at.empty()) {
			if (platform::exec((at + "clang").c_str()).contains("no input files")) {
				std::unique_lock _g{install_dir_lock};
				if (!install_dir)
					install_dir.emplace(at);
				return &install_dir.value();
			}
		}

		// Try the defaults.
		//
		for (auto& dir : base_directories) {
			if (platform::exec((std::string(dir) += "clang").c_str()).contains("no input files")) {
				std::unique_lock _g{install_dir_lock};
				if (!install_dir)
					install_dir.emplace(std::string{dir});
				return &install_dir.value();
			}
		}
		return nullptr;
	}

	// Compiles a given C snippet using Clang, returns an empty array on failure.
	//
	std::vector<u8> compile(std::string_view source, std::string_view arguments, std::string* err_out) {
		// Delete previous temporaries.
		//
		std::lock_guard _g{tmp_file_lock};

		auto tmp_dir = std::filesystem::temp_directory_path();
		auto in		 = tmp_dir / "retrotmp.c";
		auto out		 = tmp_dir / "retrotmp.exe";

		std::error_code ec;
		std::filesystem::remove(in, ec);
		std::filesystem::remove(out, ec);

		// Write the file.
		//
		if (!platform::write_file(in, {(const u8*) source.data(), source.size()})) {
			if (err_out) {
				*err_out = "failed to write into temporary file";
			}
			return {};
		}

		// Locate LLVM.
		//
		auto idir = locate_install();
		if (!idir) {
			if (err_out) {
				*err_out = "failed to locate LLVM setup directory";
			}
			return {};
		}

		// Create and execute the command.
		//
		auto cmd		= fmt::concat(*idir, "clang \"", in.string(), "\" -o\"", out.string(), "\" ", arguments);
		auto output = platform::exec(std::move(cmd));
		if (output.contains(" error:")) {
			if (err_out) {
				*err_out = std::move(output);
			}
			return {};
		}

		// Read the output file.
		//
		bool ok = false;
		auto result = platform::read_file(out, ok);
		if (!ok || result.empty()) {
			if (err_out)
				*err_out = "failed to read the resulting binary";
		}
		return result;
	}

	// Formats a given C++ snippet using clang-format, returns an empty string on failure and sets err_out if given.
	//
	std::string format(std::string_view source, std::string_view style, std::string* err_out) {
		// Delete previous temporaries.
		//
		std::lock_guard _g{tmp_file_lock};

		auto tmp_dir = std::filesystem::temp_directory_path();
		auto in		 = tmp_dir / "retrotmp.cpp";

		std::error_code ec;
		std::filesystem::remove(in, ec);

		// Write the file.
		//
		if (!platform::write_file(in, {(const u8*) source.data(), source.size()})) {
			if (err_out) {
				*err_out = "failed to write into temporary file";
			}
			return {};
		}

		// Locate LLVM.
		//
		auto idir = locate_install();
		if (!idir) {
			if (err_out) {
				*err_out = "failed to locate LLVM setup directory";
			}
			return {};
		}

		// Create and execute the command.
		//
		std::string cmd;
		if (style.empty())
			cmd = fmt::concat(*idir, "clang-format \"", in.string(), "\"");
		else
			cmd = fmt::concat(*idir, "clang-format \"", in.string(), "\" --style=\"", style, "\"");
		return platform::exec(std::move(cmd), false);
	}
};