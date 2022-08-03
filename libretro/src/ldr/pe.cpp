#include <retro/ldr/pe.hpp>
#include <retro/ldr/image.hpp>
#include <retro/arch/interface.hpp>
#include <nt/image.hpp>

namespace retro::ldr {
	// Errors.
	//
	RC_DEF_ERR(invalid_pe_header,  "invalid PE header")
	RC_DEF_ERR(invalid_opt_header, "invalid optional header magic")
	RC_DEF_ERR(oob_read_during,    "attempt to reference out of boundary data during step: %")
	RC_DEF_ERR(mapped_too_large,   "mapped image is too large: % MB")

	// Range check utility.
	//
	template<typename T>
	static bool ok(const T* ptr, std::span<const u8> input, size_t n = 1) {
		return (!n || ptr) && uptr((ptr + n)) <= uptr(input.data() + input.size());
	}

	// Templated loader handling both 32-bit and 64-bit formats.
	//
	template<bool x64>
	static diag::lazy load_pe(image& out, const win::image_t<x64>* img, std::span<const u8> data) {
		auto* nt = img->get_nt_headers();

		// Set image base and entry point.
		//
		out.base_address = nt->optional_header.image_base;
		if (u32 ep = nt->optional_header.entry_point) {
			out.entry_points.emplace_back(ep);
		}

		// Identify image type.
		//
		if (nt->file_header.characteristics.dll_file) {
			out.kind = image_kind::dynamic_library;
		} else {
			out.kind = image_kind::executable;
		}

		// Identify architecture.
		//
		switch (nt->file_header.machine) {
			case win::machine_id::i386: {
				out.arch_hash = arch::i386;
				out.default_call_conv = arch::call_conv::cdecl_i386;
				break;
			}
			case win::machine_id::amd64: {
				out.arch_hash = arch::x86_64;
				out.default_call_conv = arch::call_conv::msabi_x86_64;
				break;
			}
			// TODO: arm/thumb/armnt/powerpc/powerpcfp/mips16/mipsfpu/mipsfpu16...
			default:
				break;
		}

		// TODO: Further environment information like type libraries and so on.
		//
		switch (nt->optional_header.subsystem) {
			case win::subsystem_id::efi_application:
			case win::subsystem_id::efi_boot_service_driver:
			case win::subsystem_id::efi_runtime_driver:
			case win::subsystem_id::efi_rom:
			case win::subsystem_id::native:
			case win::subsystem_id::native_windows: {
				out.env_privileged = true;
				break;
			}           
			case win::subsystem_id::windows_gui:
			case win::subsystem_id::windows_cui:               
			case win::subsystem_id::os2_cui:                   
			case win::subsystem_id::posix_cui:                
			case win::subsystem_id::windows_ce_gui:            
			case win::subsystem_id::xbox:                      
			case win::subsystem_id::windows_boot_application:  
			case win::subsystem_id::xbox_code_catalog: {
				break;
			}
			default:
				break;
		}

		// Allocate space for the entire image and copy the headers.
		//
		size_t image_size = nt->optional_header.size_image;
		size_t header_size = nt->optional_header.size_headers;
		if (image_size > 1_gb) {
			return err::mapped_too_large(image_size / 1_mb);
		}
		if (!ok(img->template raw_to_ptr<u8>((u32)header_size), data)) {
			return err::oob_read_during("copying headers");
		}
		out.raw_data.resize(image_size, 0);
		memcpy(out.raw_data.data(), img, header_size);

		// Map and validate all sections.
		//
		if (!ok(nt->sections().end() - 1, data)) {
			return err::oob_read_during("parsing sections");
		}
		for (auto& scn : nt->sections()) {
			// Make sure virtual memory range is within the image and does not overlap the header.
			//
			if ((scn.virtual_address + (u64) scn.virtual_size) > image_size || scn.virtual_address < header_size) {
				return err::oob_read_during("validating sections");
			}

			// Make sure the file ranges are valid as well.
			//
			if ((u64(scn.ptr_raw_data) + scn.size_raw_data) > out.raw_data.size()) {
				return err::oob_read_during("copying sections");
			}

			// Copy raw data.
			//
			memcpy(out.raw_data.data() + scn.virtual_address, img->raw_to_ptr(scn.ptr_raw_data), std::min(scn.virtual_size, scn.size_raw_data));

			// Create the section descriptor and insert it.
			//
			section res = {
				 .rva		 = scn.virtual_address,
				 .rva_end = u64(scn.virtual_address) + scn.virtual_size,
				 .name	 = std::string{scn.name.to_string()},
				 .write	 = scn.characteristics.mem_write != 0,
				 .execute = scn.characteristics.mem_execute != 0,
			};
			insert_into_rva_set(out.sections, res);

			// TODO: Check overlaps?
		}

		// Parse export directory:
		//
		if (auto* dd = img->get_directory(win::directory_entry_export)) {
			auto exp = img->template rva_to_ptr<win::export_directory_t>(dd->rva);
			if (ok(exp, data)) {
				size_t	  num_func			  = exp->num_functions;
				size_t	  num_names			  = exp->num_names;
				const u32* rvas				  = img->template rva_to_ptr<u32>(exp->rva_functions);
				const u32* rva_names			  = img->template rva_to_ptr<u32>(exp->rva_names);
				const u16* rva_name_ordinals = img->template rva_to_ptr<u16>(exp->rva_name_ordinals);
				if (ok(rvas, data, num_func) && ok(rva_names, data, num_names) && ok(rva_name_ordinals, data, num_names)) {
					// Write ordinal list.
					//
					out.symbols.resize(num_func);
					for (size_t i = 0; i != num_func; i++) {
						out.symbols[i].rva = rvas[i];
					}

					// Write entries with symbols.
					//
					for (size_t i = 0; i != num_names; i++) {
						// Invalid ordinal.
						//
						auto o = rva_name_ordinals[i];
						if (o >= out.symbols.size()) {
							continue;
						}

						// Map the name.
						//
						auto s = img->template rva_to_ptr<char>(rva_names[i]);
						if (ok(s, data)) {
							auto e = std::find(s, (const char*)data.data() + data.size(), 0);
							out.symbols[o].name = {s, e};
						}
					}

					// Fill the rest of the names.
					//
					for (size_t i = 0; i != num_func; i++) {
						if (out.symbols[i].name.empty()) {
							out.symbols[i].name = fmt::str("Ordinal%x", (u32) (i + exp->base));
						}
					}

					// Sort by RVA.
					//
					range::sort(out.symbols, [](auto& a, auto& b) { return a.rva < b.rva; });
				}
			}
		}

		// TODO: Relocation info.
		// TODO: PDB info + image_name from PDB info
		// TODO: TLS entry_points
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
		ref out		  = make_rc<image>();
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