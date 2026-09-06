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
#include <embdfs.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_internal.h>

#include "asset_loader.hpp"
#include "error_macros.hpp"
#include "graphics/imgui_engine.hpp"
#include "graphics/vulkan_engine.hpp"

AppState state{};
float AppState::main_scale = 1.f;

void SDL_AppQuit() {
	if (state.window) {
		SDL_HideWindow(state.window);
		if (state.vk_engine && state.vk_engine->device != VK_NULL_HANDLE) {
			if (state.vk_engine->queue != VK_NULL_HANDLE) {
				vkQueueWaitIdle(state.vk_engine->queue);
			}
			vkDeviceWaitIdle(state.vk_engine->device);
			if (state.imgui_engine) {
				AssetLoader::cleanup(state.vk_engine);
				state.imgui_engine->cleanup();
				ImGui_ImplSDL3_Shutdown();
				ImGui::DestroyContext();
			}
			ERR_FAIL_COND(!state.vk_engine->device, "VkDevice already destroyed?");
			state.vk_engine->cleanup();
		}
		SDL_DestroyWindow(state.window);
		SDL_Quit();
	}
	embdfs::cleanup();
}

int main() {
	embdfs::setup();
	ERR_FAIL_COND_RET(!SDL_Init(SDL_INIT_VIDEO), -1, SDL_GetError());

	state.main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	SDL_Window *window = SDL_CreateWindow(APP_NAME, (int)(1280_scaled), (int)(720_scaled), SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIGH_PIXEL_DENSITY);
	ERR_FAIL_NULL_RET_SDL(window, -1, SDL_GetError());
	state.window = window;

	VulkanEngine vk_engine{ window };
	ERR_FAIL_COND_RET_SDL(volkInitialize() != VK_SUCCESS, -1, "Failed to initialize volk. Is Vulkan installed?");
	state.vk_engine = &vk_engine;

	ERR_FAIL_COND_RET_SDL(!vk_engine.create_instance(), -1, "Failed to create instance.");
	ERR_FAIL_COND_RET_SDL(!vk_engine.pick_physical_device(), -1, "Failed to pick physical device.");
	ERR_FAIL_COND_RET_SDL(!vk_engine.create_device(), -1, "Failed to create device.");
	ERR_FAIL_COND_RET_SDL(!vk_engine.create_allocator(), -1, "Failed to create allocator.");
	ERR_FAIL_COND_RET_SDL(!vk_engine.create_surface(), -1, "Failed to create surface.");
	ERR_FAIL_COND_RET_SDL(!vk_engine.create_pipeline_cache(), -1, "Failed to create pipeline cache.");

	ImGuiEngine imgui_engine{ &state, &vk_engine };
	state.imgui_engine = &imgui_engine;
	int w, h;
	ERR_FAIL_COND_RET_SDL(!SDL_GetWindowSizeInPixels(window, &w, &h), -1, SDL_GetError());
	ERR_FAIL_COND_RET_SDL(!SDL_ShowWindow(window), -1, SDL_GetError());
	ERR_FAIL_COND_RET_SDL(!vk_engine.setup_swapchain(), -1, "Failed to setup swapchain.");
	ERR_FAIL_COND_RET_SDL(!vk_engine.create_window(w, h), -1, "Failed to create window resources.");

	ERR_FAIL_COND_RET_SDL(!imgui_engine.setup(), -1, "Failed to setup imgui engine.");

	ERR_FAIL_COND_RET_SDL(!AssetLoader::load_fonts(), -1, "Failed to load initial fonts.");
	ERR_FAIL_COND_RET_SDL(!AssetLoader::load_textures(&imgui_engine), -1, "Failed to load initial textures.");

	while (!state.done) {
		uint64_t target_ms = 1000 / state.target_fps;
		uint64_t start = SDL_GetTicks();
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);
			switch (event.type) {
				case SDL_EVENT_QUIT:
					state.done = true;
					break;
				case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
					if (event.window.windowID == SDL_GetWindowID(window)) {
						state.done = true;
					}
					break;
				default:
					[[likely]] break;
			}
		}

		if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
			SDL_Delay(10);
			continue;
		}

		int fb_w, fb_h;
		ERR_FAIL_COND_RET_SDL(!SDL_GetWindowSizeInPixels(window, &fb_w, &fb_h), 999, SDL_GetError());
		if (fb_w > 0 && fb_h > 0 && (vk_engine.window.swapchain_rebuild || vk_engine.window.width != fb_w || vk_engine.window.height != fb_h)) {
			vk_engine.create_window(fb_w, fb_h);
		}

		ERR_FAIL_COND_RET_SDL(!imgui_engine.new_frame(), 1, "Failed to create new frame.");
		if (!vk_engine.window.frame_acquired) {
			continue;
		}
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		ImGui::ShowDemoWindow();

		float menu_bar_height = ImGui::GetCurrentWindowRead()->MenuBarHeight;
		bool maximized = SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED;

		ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(ImVec2(fb_w, fb_h - ImGui::GetTextLineHeightWithSpacing()));
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		if (ImGui::Begin("SpolayMainWindow", nullptr, window_flags)) {
			ImGui::PopStyleVar(2);
			if (ImGui::BeginMainMenuBar()) {
				Texture *texture = TEXTURES_GUI_HAMBURGER_LOADED;
				ImGui::GetWindowDrawList()->AddImage(texture->get_imgui_id(), ImVec2(0, 0), ImVec2(texture->width, texture->height));
				ImGui::InvisibleButton("##window_menu", ImVec2(menu_bar_height, menu_bar_height));
				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
					ImGui::OpenPopup("WindowMenu");
				}

				if (ImGui::BeginPopup("WindowMenu")) {
					ImGui::BeginDisabled(!maximized);
					if (ImGui::MenuItemEx("Restore", ICON_VS_CHROME_RESTORE)) {
						SDL_RestoreWindow(window);
					}
					ImGui::EndDisabled();

					if (ImGui::MenuItemEx("Minimize", ICON_VS_CHROME_MINIMIZE)) {
						SDL_MinimizeWindow(window);
					}

					ImGui::BeginDisabled(maximized);
					if (ImGui::MenuItemEx("Maximize", ICON_VS_CHROME_MAXIMIZE)) {
						SDL_MaximizeWindow(window);
					}
					ImGui::EndDisabled();

					if (ImGui::MenuItemEx("Close", ICON_VS_CHROME_CLOSE, "Alt+F4")) {
						CLEANUP_SDL();
						return 0;
					}
					ImGui::EndPopup();
				}
				ImGui::EndMainMenuBar();
			}

			if (ImGui::BeginMainMenuBar()) {
				ImGui::Dummy({});
				ImGui::PopStyleVar(2);

				ImVec2 buttonSize = ImVec2(menu_bar_height * 1.5f, menu_bar_height - 1);

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_MenuBarBg));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabActive));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabHovered));

				ImGui::SetCursorPosX(ImGui::GetWindowWidth() - buttonSize.x * 3);
				if (ImGui::Button(ICON_VS_CHROME_MINIMIZE, buttonSize)) {
					ERR_FAIL_COND_RET_SDL(!SDL_MinimizeWindow(window), 999, SDL_GetError());
				}
				if (maximized) {
					if (ImGui::Button(ICON_VS_CHROME_RESTORE, buttonSize)) {
						ERR_FAIL_COND_RET_SDL(!SDL_ShowWindow(window), 999, SDL_GetError());
					}
				} else {
					if (ImGui::Button(ICON_VS_CHROME_MAXIMIZE, buttonSize)) {
						ERR_FAIL_COND_RET_SDL(!SDL_MaximizeWindow(window), 999, SDL_GetError());
					}
				}

				ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF7A70F1);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF2311E8);

				if (ImGui::Button(ICON_VS_CHROME_CLOSE, buttonSize)) {
					CLEANUP_SDL();
					return 0;
				}

				ImGui::PopStyleColor(5);
				ImGui::PopStyleVar();

				ImGui::EndMainMenuBar();
			}
			ImGui::End();
		} else {
			ImGui::PopStyleVar(4);
		}

		ImGui::Render();
		ImDrawData *draw_data = ImGui::GetDrawData();
		const bool is_minimized = (draw_data->DisplaySize.x <= 0.f || draw_data->DisplaySize.y <= 0.f);
		if (!is_minimized) {
			ERR_FAIL_COND_RET_SDL(!imgui_engine.frame_render(draw_data, vk_engine.window.width, vk_engine.window.height), 1, "Failed to render frame.");
			ERR_FAIL_COND_RET_SDL(!imgui_engine.frame_present(), 1, "Failed to present frame to queue.");
		}
		uint64_t end = SDL_GetTicks();
		if (end - start < target_ms) {
			SDL_Delay(end - start);
		}
	}

	CLEANUP_SDL();
	return 0;
}
