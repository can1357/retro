#include <retro/ldr/pe.hpp>
#include <retro/ldr/pe/image.hpp>
#include <nt/image.hpp>

namespace retro::ldr {
	// Loader errors.
	//
	RC_DEF_ERR(image_is_very_stupid, "expected smart image got dumbass % bytes instead")

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

	// Range check utility.
	//
	template<typename T>
	static bool ok(const T* ptr, std::span<const u8> input) {
		return uptr(std::next(ptr)) <= uptr(input.data() + input.size());
	}




	
	// Loader entry point.
	//
	diag::expected<ref<image>> pe_loader::load(std::span<const u8> data) {
		// Create an image.
		//
		ref img = make_rc<pe::image>();
		img->ldr_hash = get_hash();


		// TODO: stuff.


		if (true) {
			return err::image_is_very_stupid(data.size());
		} else {
			return {std::move(img)};
		}
	}

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
		if (nt->signature != win::NT_HDR_MAGIC) {
			return false;
		}

		// Check optional header magic and size.
		//
		if (nt->optional_header.magic == win::OPT_HDR32_MAGIC) {
			return ok((win::nt_headers_x64_t*) nt, data);
		} else if (nt->optional_header.magic == win::OPT_HDR64_MAGIC) {
			return ok((win::nt_headers_x86_t*) nt, data);
		} else {
			return false;
		}
	}

	// Create the instances.
	//
	RC_ADD_INTERFACE("coff-pe", pe_loader);
};