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

	bool setup(Window *p_window);
	bool setup_objects();
	static VkPipeline create_pipeline(PipelineInfo *p_info);

	static void draw_callback__reset_render_state(const ImDrawList *, const ImDrawCmd *);
	static void draw_callback__set_sampler_linear(const ImDrawList *, const ImDrawCmd *);
	static void draw_callback__set_sampler_nearest(const ImDrawList *, const ImDrawCmd *);

	static void create_window(ImGuiViewport *p_viewport);
	static void destroy_window(ImGuiViewport *p_viewport);
	static void set_window_size(ImGuiViewport *p_viewport, ImVec2 p_size);
	static void render_window(ImGuiViewport *p_viewport, void *);
	static void swap_buffers(ImGuiViewport *p_viewport, void *);

	bool new_frame();
	static bool frame_render(Window *p_window, ImDrawData *p_draw_data, int p_width, int p_height);
	static bool render_draw_data(Window *p_window, ImDrawData *p_draw_data, VkCommandBuffer p_command_buffer, int p_width, int p_height);
	static bool frame_present(Window *p_window);

	Texture *load_texture(const uint8_t *p_buffer, int p_size);

	void cleanup();
	~ImGuiEngine();

	static ImGuiBackendData *get_backend_data();

private:
	static bool acquire_next_image(Window *p_window);
	static bool update_texture(Window *p_window, ImTextureData *p_texture);
	static void setup_render_state(VkCommandBuffer p_command_buffer, ImDrawData *p_draw_data, RenderBuffers *p_rb, int p_width, int p_height);
};
