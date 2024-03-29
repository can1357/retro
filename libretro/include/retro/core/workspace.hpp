#pragma once
#include <retro/common.hpp>
#include <retro/rc.hpp>
#include <retro/umutex.hpp>
#include <retro/diag.hpp>
#include <retro/neo.hpp>
#include <retro/ldr/interface.hpp>
#include <vector>

namespace retro::core {
	struct workspace;

	// Workspace holds the document state and may represent multiple images.
	//
	struct image;
	struct workspace final {
		// Image list.
		//
		mutable shared_umutex	image_list_mtx = {};
		std::vector<ref<image>> image_list;

		// Creates a new workspace.
		//
		static ref<workspace> create() { return make_rc<workspace>(); }

		// Loads an image from filesystem/memory.
		//
		diag::expected<ref<image>> load_image(const std::filesystem::path& path, ldr::handle ldr = std::nullopt);
		diag::expected<ref<image>> load_image_in_memory(std::span<const u8> data, ldr::handle ldr = std::nullopt);
	};
};