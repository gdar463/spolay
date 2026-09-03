#pragma once

/**
 * imgui_engine.hpp
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

#include <volk.h>

#include <imgui.h>

#include "graphics/imgui/backend_data.hpp"
#include "graphics/vulkan/render_buffers.hpp"
#include "graphics/vulkan/texture.hpp"
#include "graphics/vulkan_engine.hpp"

class ImGuiEngine {
public:
	ImGuiEngine(AppState *p_state, VulkanEngine *p_vk);

	AppState *state = nullptr;
	VulkanEngine *vk = nullptr;
	Window *window = nullptr;

	bool setup();
	bool setup_objects();

	static void draw_callback__reset_render_state(const ImDrawList *, const ImDrawCmd *);
	static void draw_callback__set_sampler_linear(const ImDrawList *, const ImDrawCmd *);
	static void draw_callback__set_sampler_nearest(const ImDrawList *, const ImDrawCmd *);

	bool new_frame();
	bool frame_render(ImDrawData *p_draw_data, int p_width, int p_height);
	bool render_draw_data(ImDrawData *p_draw_data, VkCommandBuffer p_command_buffer, int p_width, int p_height);
	bool frame_present();

	Texture *load_texture(const uint8_t *p_buffer, int p_size);

	void cleanup();
	~ImGuiEngine();

	static BackendData *get_backend_data();

private:
	bool acquire_next_image();
	bool update_texture(ImTextureData *p_texture);
	void setup_render_state(VkCommandBuffer p_command_buffer, ImDrawData *p_draw_data, RenderBuffers *p_rb, int p_width, int p_height);
};
