/**
 * asset_loader.cpp
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

#include "asset_loader.hpp"

#include <embdfs.hpp>

std::vector<uint8_t *> AssetLoader::fonts{};
std::unordered_map<const char *, Texture *> AssetLoader::textures{};

const std::pair<const char *, bool> initial_fonts[]{
	{ FONTS_NUNITO, false },
	{ FONTS_NERDFONT, true }
};

bool AssetLoader::load_fonts() {
	ImGuiIO &io = ImGui::GetIO();
	for (const std::pair<const char *, bool> font_pair : initial_fonts) {
		const embdfs::Resource *font_resource = embdfs::get(font_pair.first);
		uint8_t *font_data = new uint8_t[font_resource->size()]{};
		memcpy(font_data, font_resource->data(), font_resource->size());
		ImFontConfig font_config{};
		font_config.FontDataOwnedByAtlas = false;
		font_config.MergeMode = font_pair.second;
		ImFont *font = io.Fonts->AddFontFromMemoryTTF((void *)font_data, font_resource->size(), 1.f, &font_config);
		ERR_FAIL_NULL_PRERET(font, false, delete[] font_data, "Failed to load font \"" + std::string(font_pair.first) + "\"");
		ERR_FAIL_COND_PRERET(!font->IsLoaded(), false, delete[] font_data, "Failed to load font \"" + std::string(font_pair.first) + "\"");
		AssetLoader::fonts.push_back(font_data);
	}
	return true;
}

const char *const initial_textures[]{
	TEXTURES_GUI_HAMBURGER
};

bool AssetLoader::load_textures(ImGuiEngine *p_imgui) {
	for (const char *const texture_path : initial_textures) {
		const embdfs::Resource *texture_resource = embdfs::get(texture_path);
		Texture *texture = p_imgui->load_texture(texture_resource->data(), texture_resource->size());
		ERR_FAIL_NULL_RET(texture, false, "Failed to load texture \"" + std::string(texture_path) + "\".");
#ifdef DEBUG
		texture->path = new char[strlen(texture_path)]{};
		texture->path = strdup(texture_path);
#endif
		AssetLoader::textures.insert({ texture_path, texture });
	}
	return true;
}

void AssetLoader::cleanup(VulkanEngine *p_vk) {
	if (!AssetLoader::fonts.empty()) {
		for (uint8_t *font : AssetLoader::fonts) {
			delete[] font;
		}
		AssetLoader::fonts.clear();
	}

	if (!AssetLoader::textures.empty()) {
		for (std::pair<const char *, Texture *> texture : AssetLoader::textures) {
			texture.second->cleanup(p_vk->device, p_vk->allocator, ImGuiEngine::get_backend_data()->descriptor_pool);
			delete texture.second;
		}
		AssetLoader::textures.clear();
	}
}

Texture *operator""_loaded(const char *p_path, size_t) {
	return AssetLoader::textures.at(p_path);
}
