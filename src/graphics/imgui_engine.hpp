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
#include <imgui_impl_vulkan.h>

#include "graphics/vulkan_engine.hpp"

class ImGuiEngine {
public:
	ImGuiEngine(AppState *p_state, VulkanEngine *p_vk);

	AppState *state = nullptr;
	VulkanEngine *vk = nullptr;
	ImGui_ImplVulkanH_Window window{};
	ImGuiIO io;
	ImGuiContext *context = nullptr;

	bool setup_window(int p_width, int p_height);
	bool create_window(int p_width, int p_height);
	bool setup();

	void frame_render(ImDrawData *p_draw_data);
	void frame_present();

	void cleanup();
	void cleanup_window();
	~ImGuiEngine();
};
