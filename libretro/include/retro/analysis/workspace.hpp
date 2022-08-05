#pragma once
#include <retro/common.hpp>
#include <retro/rc.hpp>
#include <retro/lock.hpp>
#include <retro/diag.hpp>
#include <vector>

namespace retro::analysis {
	struct workspace;

	// Workspace holds the document state and may represent multiple images.
	//
	struct image;
	struct workspace {
		// Image list.
		//
		rw_lock						image_list_lock = {};
		std::vector<ref<image>> image_list;

		// Creates a new workspace.
		//
		static ref<workspace> create() { return make_rc<workspace>(); }

		// Loads an image from filesystem/memory.
		//
		diag::expected<ref<image>> load_image(const std::filesystem::path& path);
		diag::expected<ref<image>> load_image_in_memory(std::span<const u8> data);
	};
};