#pragma once

/**
 * asset_loader.hpp
 *
 * Copyright (C) 2026 gdar463 <dev@gdar463.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General
 * Public License along with this program. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <unordered_map>

#include "assets.hpp" // IWYU pragma: export
#include "graphics/imgui_engine.hpp"
#include "graphics/vulkan/texture.hpp"

class AssetLoader {
public:
	static std::vector<uint8_t *> fonts;
	static std::unordered_map<const char *, Texture *> textures;

	static bool load_fonts();
	static bool load_textures(ImGuiEngine *p_imgui);

	static void cleanup();
};

Texture *operator""_loaded(const char *p_path, size_t p_size);
