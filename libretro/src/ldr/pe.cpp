#include <retro/ldr/pe.hpp>
#include <retro/ldr/pe/image.hpp>
#include <nt/image.hpp>

namespace retro::ldr {
	// Errors.
	//
	RC_DEF_ERR(invalid_pe_header,  "invalid PE header")
	RC_DEF_ERR(invalid_opt_header, "invalid optional header magic")

	// Range check utility.
	//
	template<typename T>
	static bool ok(const T* ptr, std::span<const u8> input) {
		return uptr(std::next(ptr)) <= uptr(input.data() + input.size());
	}






	
	// TODO: Place holder for dev reasons, template the one below.
	RC_DEF_ERR(image_is_very_stupid, "expected smart image got dumbass one instead")
	static diag::lazy load_pe(pe::image& out, const win::image_t<false>* in, std::span<const u8> data) { return err::image_is_very_stupid(); }
	// template<bool x64>
	constexpr bool x64 = true;

	// Templated loader handling both 32-bit and 64-bit formats.
	//
	static diag::lazy load_pe(pe::image& out, const win::image_t<x64>* in, std::span<const u8> data) {

		return diag::ok;
	}

	// Loader entry point.
	//
	diag::expected<ref<image>> pe_loader::load(std::span<const u8> data) {
		// First pass the data through basic checks.
		//
		if (!match(data)) {
			return err::invalid_pe_header();
		}

		// Create the image.
		//
		ref out		  = make_rc<pe::image>();
		out->ldr_hash = get_hash();

		// Visit based on optional header magic.
		//
		auto* img = (const win::image_x64_t*) data.data();
		auto* nt	 = img->get_nt_headers();
		if (nt->optional_header.magic == win::OPT_HDR32_MAGIC) {
			if (auto err = load_pe(*out, (const win::image_x86_t*)img, data)) {
				return {std::move(err)};
			}
		} else if (nt->optional_header.magic == win::OPT_HDR64_MAGIC) {
			if (auto err = load_pe(*out, (const win::image_x64_t*) img, data)) {
				return {std::move(err)};
			}
		} else {
			return err::invalid_opt_header();
		}

		// Return the result.
		//
		return {std::move(out)};
	}

	// Extension list.
	//
	static constexpr std::string_view extension_list[] = {
		 "exe",
		 "dll",
		 "sys",
		 "efi"
		 "acm",
		 "ax",
		 "cpl",
		 "drv",
		 "mui",
		 "ocx",
		 "scr",
		 "tsp",
	};
	std::span<const std::string_view> pe_loader::get_extensions() { return extension_list; }

	// Lossy match via the magic.
	//
	bool pe_loader::match(std::span<const u8> data) {
		// Check if we can read a DOS header.
		//
		if (data.size() < sizeof(win::dos_header_t)) {
			return false;
		}

		// Check if the magic matches.
		//
		auto* img = (const win::image_x64_t*) data.data();
		if (img->dos_header.e_magic != win::DOS_HDR_MAGIC) {
			return false;
		}

		// Check if we can read the NT headers.
		//
		auto* nt = img->get_nt_headers();
		if (!ok(nt, data)) {
			return false;
		}

		// Check if the magic matches.
		//
		return nt->signature == win::NT_HDR_MAGIC;
	}

	// Create the instances.
	//
	RC_ADD_INTERFACE("coff-pe", pe_loader);
};