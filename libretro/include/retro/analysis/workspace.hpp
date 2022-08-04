#pragma once
#include <retro/common.hpp>
#include <retro/rc.hpp>
#include <retro/lock.hpp>
#include <retro/arch/callconv.hpp>
#include <retro/arch/interface.hpp>
#include <retro/ldr/interface.hpp>
#include <retro/robin_hood.hpp>
#include <retro/analysis/method.hpp>
#include <atomic>

namespace retro::analysis {
	struct workspace;

	// Domain represents the analysis state associated with a single image.
	//
	struct domain {
		// Owning workspace and the relevant image.
		// - Constant after initialization.
		//
		weak<workspace> parent = {};
		ref<ldr::image> img	 = {};

		// Method table.
		// - RVA -> Method.
		//
		mutable rw_lock				 method_map_lock = {};
		flat_umap<u64, ref<method>> method_map		  = {};

		// Loader provided defaults.
		//
		arch::handle					 arch			= {};
		const arch::call_conv_desc* default_cc = nullptr;

		// Observers.
		//
		ref<method> lookup_method(u64 rva) const {
			std::shared_lock _g{method_map_lock};
			if (auto it = method_map.find(rva); it != method_map.end()) {
				return it->second;
			}
			return nullptr;
		}
	};

	// Workspace holds the document state and may represent multiple images.
	//
	struct workspace {
		// Workspace list.
		//
		rw_lock						 domain_list_lock = {};
		std::vector<ref<domain>> domain_list;

		// Creates a new workspace.
		//
		static ref<workspace> create() { return make_rc<workspace>(); }

		// Creates a domain for the image.
		//
		ref<domain> add_image(ref<ldr::image> img) {
			auto arch = arch::instance::lookup(img->arch_hash);
			if (!arch)
				return {};

			std::unique_lock _g{domain_list_lock};
			auto				  dom = domain_list.emplace_back(make_rc<domain>());
			dom->parent				= this;
			dom->img					= img;
			dom->arch				= arch;
			dom->default_cc		= arch->get_cc_desc(img->default_call_conv);
			return dom;
		}
	};
};