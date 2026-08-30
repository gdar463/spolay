/**
 * main.cpp
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

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include "error_macros.hpp"
#include "graphics/imgui_engine.hpp"
#include "graphics/vulkan_engine.hpp"

AppState state{};

void SDL_AppQuit() {
	if (state.window) {
		if (state.vk_engine && state.vk_engine->device) {
			vkDeviceWaitIdle(state.vk_engine->device);
			if (state.imgui_engine) {
				state.imgui_engine->cleanup();
				state.imgui_engine->cleanup_window();
			}
			state.vk_engine->cleanup();
		}
		SDL_DestroyWindow(state.window);
		SDL_Quit();
	}
}

int main() {
	ERR_FAIL_COND_RET(!SDL_Init(SDL_INIT_VIDEO), -1, SDL_GetError());

	state.main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	SDL_Window *window = SDL_CreateWindow(APP_NAME, (int)(1280 * state.main_scale), (int)(720 * state.main_scale), SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	ERR_FAIL_NULL_RET_SDL(window, -1, SDL_GetError());
	state.window = window;

	VulkanEngine vk_engine{ window };
	ERR_FAIL_COND_RET_SDL(volkInitialize() != VK_SUCCESS, -1, "Failed to initialize volk. Is Vulkan installed?");
	state.vk_engine = &vk_engine;

	ERR_FAIL_COND_SILENT_RET_SDL(!vk_engine.create_instance(), -1);
	ERR_FAIL_COND_SILENT_RET_SDL(!vk_engine.pick_physical_device(), -1);
	ERR_FAIL_COND_SILENT_RET_SDL(!vk_engine.create_device(), -1);
	ERR_FAIL_COND_SILENT_RET_SDL(!vk_engine.create_allocator(), -1);
	ERR_FAIL_COND_SILENT_RET_SDL(!vk_engine.create_ImGui_pool(), -1);
	ERR_FAIL_COND_SILENT_RET_SDL(!vk_engine.create_surface(), -1);
	ERR_FAIL_COND_SILENT_RET_SDL(!vk_engine.create_pipeline_cache(), -1);
	vk_engine.min_image_count = 3;
	ERR_FAIL_COND_SILENT_RET_SDL(!vk_engine.create_swapchain(), -1);
	ERR_FAIL_COND_SILENT_RET_SDL(!vk_engine.create_render_pass(), -1);

	ImGuiEngine imgui_engine{ &state, &vk_engine };
	state.imgui_engine = &imgui_engine;
	int w, h;
	ERR_FAIL_COND_RET_SDL(!SDL_GetWindowSizeInPixels(window, &w, &h), -1, SDL_GetError());
	ERR_FAIL_COND_RET_SDL(!SDL_SetWindowPosition(window, w, h), -1, SDL_GetError());
	ERR_FAIL_COND_RET_SDL(!SDL_ShowWindow(window), -1, SDL_GetError());

	ERR_FAIL_COND_SILENT_RET_SDL(!imgui_engine.setup(), -1);
	ERR_FAIL_COND_SILENT_RET_SDL(!imgui_engine.setup_window(w, h), -1);

	while (!state.done) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);
			if (event.type == SDL_EVENT_QUIT) {
				state.done = true;
			}
			if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)) {
				state.done = true;
			}
		}

		if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
			SDL_Delay(10);
			continue;
		}

		int fb_w, fb_h;
		ERR_FAIL_COND_RET_SDL(!SDL_GetWindowSizeInPixels(window, &fb_w, &fb_h), 999, SDL_GetError());
		if (fb_w > 0 && fb_h > 0 && (vk_engine.swapchain_rebuild || imgui_engine.window.Width != fb_w || imgui_engine.window.Height != fb_h)) {
			imgui_engine.create_window(fb_w, fb_h);
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		ImGui::ShowDemoWindow();

		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");

			ImGui::Text("This is some useful text.");
			ImGui::Checkbox("Another Window", &state.show_another_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
			ImGui::ColorEdit3("clear color", (float *)&state.clear_color);

			if (ImGui::Button("Button")) {
				counter++;
			}
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / imgui_engine.io.Framerate, imgui_engine.io.Framerate);
			ImGui::End();
		}

		if (state.show_another_window) {
			ImGui::Begin("Another Window", &state.show_another_window); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me")) {
				state.show_another_window = false;
			}
			ImGui::End();
		}

		ImGui::Render();
		ImDrawData *draw_data = ImGui::GetDrawData();
		const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
		if (!is_minimized) {
			imgui_engine.window.ClearValue.color.float32[0] = state.clear_color.x * state.clear_color.w;
			imgui_engine.window.ClearValue.color.float32[1] = state.clear_color.y * state.clear_color.w;
			imgui_engine.window.ClearValue.color.float32[2] = state.clear_color.z * state.clear_color.w;
			imgui_engine.window.ClearValue.color.float32[3] = state.clear_color.w;
			imgui_engine.frame_render(draw_data);
			imgui_engine.frame_present();
		}
	}

	CLEANUP_SDL();
	return 0;
}
