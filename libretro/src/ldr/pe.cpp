#include <retro/ldr/pe.hpp>
#include <retro/core/image.hpp>
#include <retro/arch/interface.hpp>
#include <retro/ldr/pe/nt/image.hpp>

namespace retro::ldr {
	// Errors.
	//
	RC_DEF_ERR(invalid_pe_header,  "invalid PE header")
	RC_DEF_ERR(invalid_opt_header, "invalid optional header magic")
	RC_DEF_ERR(oob_read_during,    "attempt to reference out of boundary data during step: %")
	RC_DEF_ERR(mapped_too_large,   "mapped image is too large: % MB")

	// Range check utilities.
	//
	template<typename T>
	static bool ok(const T* ptr, std::span<const u8> input, size_t n = 1) {
		return !n || (uptr(input.data()) <= uptr(ptr) && uptr(ptr + n) <= uptr(input.data() + input.size()));
	}
	template<typename C = char>
	static std::basic_string_view<C> read_string(const C* ptr, std::span<const u8> input) {
		if (ptr) {
			uptr beg = uptr(input.data());
			uptr lim = uptr(input.data() + input.size());
			if (beg <= uptr(ptr) && uptr(ptr) <= uptr(lim)) {
				std::basic_string_view<C> view{ptr, size_t((lim - uptr(ptr)) / sizeof(C))};
				size_t n = view.find(C(0));
				if (n != std::string::npos) {
					view = view.substr(0, n);
				}
				return view;
			}
		}
		return {};
	}

	// Templated loader handling both 32-bit and 64-bit formats.
	//
	template<bool x64>
	static diag::lazy load_pe(core::image& out, const win::image_t<x64>* img, std::span<const u8> data) {
		using va_t = std::conditional_t<x64, u64, u32>;

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
			out.kind = core::image_kind::dynamic_library;
		} else {
			out.kind = core::image_kind::executable;
		}

		// Identify architecture.
		//
		switch (nt->file_header.machine) {
			case win::machine_id::i386: {
				out.arch			= arch::instance::lookup(arch::i386);
				out.default_cc = arch::call_conv::cdecl_i386;
				break;
			}
			case win::machine_id::amd64: {
				out.arch			= arch::instance::lookup(arch::x86_64);
				out.default_cc = arch::call_conv::msabi_x86_64;
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
			case win::subsystem_id::efi_rom: {
				out.abi_name		 = "ms";
				out.env_name		 = "efi";
				out.env_supervisor = true;
				break;
			}
			case win::subsystem_id::native:
			case win::subsystem_id::native_windows: {
				out.abi_name		 = "ms";
				out.env_name		 = "windows-system";
				out.env_supervisor = true;
				break;
			}
			case win::subsystem_id::windows_boot_application: {
				out.abi_name		 = "ms";
				out.env_name		 = "windows-boot";
				out.env_supervisor = true;
				break;
			}
			case win::subsystem_id::windows_gui:
			case win::subsystem_id::windows_cui:               
			case win::subsystem_id::windows_ce_gui: {
				out.abi_name		 = "ms";
				out.env_name		 = "windows-user";
				out.env_supervisor = false;
				break;
			}          
			case win::subsystem_id::xbox:                      
			case win::subsystem_id::xbox_code_catalog: {
				out.abi_name		 = "ms";
				out.env_name		 = "windows-xbox";
				out.env_supervisor = false;
				break;
			}
			case win::subsystem_id::os2_cui:
			case win::subsystem_id::posix_cui:      
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
			core::section res = {
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

		// Parse reloc directory:
		//
		if (auto* dd = img->get_directory(win::directory_entry_basereloc)) {
			auto rel = img->template rva_to_ptr<win::reloc_directory_t>(dd->rva);

			// Get block boundaries
			//
			const auto* block_begin = &rel->first_block;
			const void* block_end	= ((u8*) block_begin + dd->size);
			if (ok((u8*)block_begin, data, dd->size)) {
				for (auto block = block_begin; block < block_end; block = block->next()) {
					// For each entry:
					//
					for (size_t i = 0; i < block->num_entries(); i++) {
						if (!ok(&block->entries[i], data)) {
							break;
						}
						u64 rva = u64(block->base_rva) + block->entries[i].offset;

						// If of expected type:
						//
						if (block->entries[i].type == (x64 ? win::rel_based_dir64 : win::rel_based_high_low)) {
							// If RVA is within boundaries:
							//
							auto data = out.slice(rva);
							if (data.size() >= sizeof(va_t)) {
								core::reloc entry = {
									 .rva		= rva,
									 .target = *(va_t*) data.data() - out.base_address,
									 .kind	= core::reloc_kind::absolute,
								};
								insert_into_rva_set(out.relocs, std::move(entry));
							}
						}
					}
				}
			}
		}

		// Parse config directory.
		//
		if (auto* dd = img->get_directory(win::directory_entry_load_config)) {
			auto cfg = img->template rva_to_ptr<win::load_config_directory_t<x64>>(dd->rva);

			auto map_va = [&](const va_t& va, std::string_view name) {
				if (ok(&va, data) && va != 0) {
					core::symbol sym = {
						 .rva					 = va - out.base_address,
						 .name				 = std::string{name},
						 .read_only_ignore = true,
					};
					insert_into_rva_set(out.symbols, std::move(sym));
				}
			};
			map_va(cfg->guard_cf_dispatch_function_ptr, "__guard_dispatch_icall_fptr");
			map_va(cfg->guard_cf_check_function_ptr, "__guard_check_icall_fptr");
		}

		// Parse import directory.
		//
		if (auto* dd = img->get_directory(win::directory_entry_import)) {
			auto imp = img->template rva_to_ptr<win::import_directory_t>(dd->rva);
			for (; ok(imp, data) && imp->characteristics; ++imp) {
				auto*	 thunk = img->template rva_to_ptr<win::image_thunk_data_t<x64>>(imp->rva_original_first_thunk);
				auto	 rva	 = imp->rva_first_thunk;
				auto	 img_name = read_string(img->template rva_to_ptr<char>(imp->rva_name), data);

				for (; ok(thunk, data) && thunk->address; rva += sizeof(va_t), ++thunk) {
					std::string name{img_name};
					if (!thunk->is_ordinal) {
						auto ni = img->template rva_to_ptr<win::image_named_import_t>((u32)thunk->address);
						name += "!";
						if (ok(ni, data)) {
							name += read_string(&ni->name[0], data);
						} else {
							name += "?";
						}
					} else {
						name += fmt::str("!Ord%llu", thunk->ordinal);
					}

					core::symbol sym = {
						 .rva					 = rva,
						 .name				 = std::move(name),
						 .read_only_ignore = true,
					};
					insert_into_rva_set(out.symbols, std::move(sym));
				}
			}
		}

		// TODO: Debug directory
		// TODO: TLS entry_points
		return diag::ok;
	}

	// Loader entry point.
	//
	diag::expected<ref<core::image>> pe_loader::load(std::span<const u8> data) {
		// First pass the data through basic checks.
		//
		if (!match(data)) {
			return err::invalid_pe_header();
		}

		// Create the image.
		//
		ref out	= make_rc<core::image>();
		out->ldr = get_handle();

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