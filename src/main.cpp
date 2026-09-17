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
#include <imgui_internal.h>

#include "asset_loader.hpp"
#include "error_macros.hpp"
#include "graphics/imgui_engine.hpp"
#include "graphics/sdl_engine.hpp"
#include "graphics/vulkan_engine.hpp"
#include "net/spotify.hpp"
#include "platform/platform.hpp"

AppState state{};
float AppState::main_scale = 1.f;

void SDL_AppQuit() {
	if (state.window->sdl_window) {
		SDL_HideWindow(state.window->sdl_window);
		if (state.vk_engine && state.vk_engine->device != VK_NULL_HANDLE) {
			if (state.vk_engine->queue != VK_NULL_HANDLE) {
				vkQueueWaitIdle(state.vk_engine->queue);
			}
			vkDeviceWaitIdle(state.vk_engine->device);
			if (state.imgui_engine) {
				AssetLoader::cleanup(state.vk_engine);
				SdlEngine::cleanup();
				state.imgui_engine->cleanup();
				ImGui::DestroyContext();
			}
			state.window->cleanup(state.vk_engine->instance, state.vk_engine->device, true, true);
			ERR_FAIL_COND(!state.vk_engine->device, "VkDevice already destroyed?");
			state.vk_engine->cleanup();
		}
		SDL_Quit();
	}
	if (state.spotify) {
		state.spotify->cleanup();
	}
	embdfs::cleanup();
}

SDL_HitTestResult window_hit_test_callback(SDL_Window *, const SDL_Point *p_area, void *) {
	if (p_area->y < state.main_menu_bar.height) {
		if (p_area->x > state.main_menu_bar.left_edge && p_area->x < state.main_menu_bar.right_edge) {
			return SDL_HITTEST_DRAGGABLE;
		}
	}
	return SDL_HITTEST_NORMAL;
}

int main() {
	embdfs::setup();
	ERR_FAIL_COND_RET(!SDL_Init(SDL_INIT_VIDEO), -1, SDL_GetError());

	state.main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	SDL_Window *sdl_window = SDL_CreateWindow(APP_NAME, (int)(400_scaled), (int)(220_scaled), APP_SDL_FLAGS);
	ERR_FAIL_NULL_RET_SDL(sdl_window, -1, SDL_GetError());
	Window window{};
	window.sdl_window = sdl_window;
	state.window = &window;

	VulkanEngine vk_engine{};
	ERR_FAIL_COND_RET_SDL(volkInitialize() != VK_SUCCESS, -1, "Failed to initialize volk. Is Vulkan installed?");
	state.vk_engine = &vk_engine;

	ERR_FAIL_COND_RET_SDL(!vk_engine.create_instance(), -1, "Failed to create instance.");
	ERR_FAIL_COND_RET_SDL(!vk_engine.pick_physical_device(), -1, "Failed to pick physical device.");
	ERR_FAIL_COND_RET_SDL(!vk_engine.create_device(), -1, "Failed to create device.");
	ERR_FAIL_COND_RET_SDL(!vk_engine.create_allocator(), -1, "Failed to create allocator.");
	ERR_FAIL_COND_RET_SDL(!vk_engine.create_pipeline_cache(), -1, "Failed to create pipeline cache.");

	ImGuiEngine imgui_engine{ &state, &vk_engine };
	state.imgui_engine = &imgui_engine;
	int w, h;
	ERR_FAIL_COND_RET_SDL(!SDL_GetWindowSizeInPixels(sdl_window, &w, &h), -1, SDL_GetError());
	ERR_FAIL_COND_RET_SDL(!SDL_ShowWindow(sdl_window), -1, SDL_GetError());
	ERR_FAIL_COND_RET_SDL(!vk_engine.create_surface(&window), -1, "Failed to create surface.");
	ERR_FAIL_COND_RET_SDL(!vk_engine.setup_swapchain(&window), -1, "Failed to setup swapchain.");
	ERR_FAIL_COND_RET_SDL(!vk_engine.create_window(&window, w, h), -1, "Failed to create window resources.");

	ERR_FAIL_COND_RET_SDL(!imgui_engine.setup(&window), -1, "Failed to setup imgui engine.");
	ERR_FAIL_COND_RET_SDL(!SdlEngine::setup(sdl_window), -1, "Failed to setup sdl engine.");

	ERR_FAIL_COND_RET_SDL(!AssetLoader::load_fonts(), -1, "Failed to load initial fonts.");
	ERR_FAIL_COND_RET_SDL(!AssetLoader::load_textures(&imgui_engine), -1, "Failed to load initial textures.");

	ERR_FAIL_COND_RET_SDL(!SDL_SetWindowHitTest(sdl_window, window_hit_test_callback, nullptr), -1, SDL_GetError());

	SpotifyAPI spotify;
	state.spotify = &spotify;
	spotify.setup();

	while (!state.done) {
		uint64_t target_ms = 1000 / state.target_fps;
		uint64_t start = SDL_GetTicks();
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			SdlEngine::process_event(&event);
			switch (event.type) {
				case SDL_EVENT_QUIT:
					state.done = true;
					break;
				case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
					if (event.window.windowID == SDL_GetWindowID(sdl_window)) {
						state.done = true;
					}
					break;
				default:
					[[likely]] break;
			}
		}

		if (SDL_GetWindowFlags(sdl_window) & SDL_WINDOW_MINIMIZED) {
			SDL_Delay(10);
			continue;
		}

		int fb_w, fb_h;
		ERR_FAIL_COND_RET_SDL(!SDL_GetWindowSizeInPixels(sdl_window, &fb_w, &fb_h), 999, SDL_GetError());
		if (fb_w > 0 && fb_h > 0 && (window.swapchain_rebuild || window.width != fb_w || window.height != fb_h)) {
			vk_engine.create_window(&window, fb_w, fb_h);
		}

		ERR_FAIL_COND_RET_SDL(!imgui_engine.new_frame(), 1, "Failed to create new frame.");
		if (!window.frame_acquired) {
			continue;
		}
		SdlEngine::new_frame();
		ImGui::NewFrame();

		if (state.show_demo) {
			ImGui::ShowDemoWindow();
		}

		ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(ImVec2(fb_w, fb_h - ImGui::GetTextLineHeightWithSpacing()));
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings;
		if (ImGui::Begin("SpolayMainWindow", nullptr, window_flags)) {
			ImGui::PopStyleVar(3);

			if (ImGui::BeginMainMenuBar()) {
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_MenuBarBg));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabActive));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabHovered));
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetColorU32(ImGuiCol_MenuBarBg));
				ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabActive));
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabHovered));
				state.main_menu_bar.height = ImGui::GetCurrentWindowRead()->MenuBarHeight;

				ImVec2 buttonSize = ImVec2(state.main_menu_bar.height * 2.f, state.main_menu_bar.height - 1);
				if (ImGui::BeginPopupMenuEx(ImGui::GetID("##Settings"), "##Settings", ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus)) {
					ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(8, 4));
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 0));
					if (ImGui::MenuItem("Always on Top", "", &state.always_on_top)) {
						ERR_FAIL_COND_RET_SDL(!SDL_SetWindowAlwaysOnTop(window.sdl_window, state.always_on_top), 999, SDL_GetError());
					}
					ImGui::PopStyleVar(2);
					ImGui::EndPopup();
				}

				ImGui::SetCursorPosX(0);
				if (ImGui::Button("Settings", ImVec2(0, buttonSize.y))) {
					ImGui::OpenPopup("##Settings");
				}
				if (ImGui::Button("Demo", ImVec2(0, buttonSize.y))) {
					state.show_demo = !state.show_demo;
				}

				state.main_menu_bar.left_edge = ImGui::GetCursorPosX();

				buttonSize.x = state.main_menu_bar.height * 1.5f;
				state.main_menu_bar.right_edge = ImGui::GetWindowWidth() - buttonSize.x * 2;
				ImGui::SetCursorPosX(state.main_menu_bar.right_edge);
				if (ImGui::Button(ICON_VS_CHROME_MINIMIZE, buttonSize)) {
					ERR_FAIL_COND_RET_SDL(!SDL_MinimizeWindow(sdl_window), 999, SDL_GetError());
				}

				ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF7A70F1);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF2311E8);

				if (ImGui::Button(ICON_VS_CHROME_CLOSE, buttonSize)) {
					CLEANUP_SDL();
					return 0;
				}

				ImGui::PopStyleColor(8);
				ImGui::PopStyleVar();

				ImGui::SetCursorPosX(state.main_menu_bar.left_edge);
				ImGui::InvisibleButton("##window_menu", ImVec2(state.main_menu_bar.right_edge - state.main_menu_bar.left_edge, state.main_menu_bar.height));
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
					ImGui::OpenPopup("WindowMenu");
				}
				if (ImGui::BeginPopup("WindowMenu")) {
					if (ImGui::MenuItemEx("Minimize", ICON_VS_CHROME_MINIMIZE)) {
						ERR_FAIL_COND_RET_SDL(!SDL_MinimizeWindow(sdl_window), 999, SDL_GetError());
					}

					if (ImGui::MenuItemEx("Close", ICON_VS_CHROME_CLOSE, "Alt+F4")) {
						CLEANUP_SDL();
						return 0;
					}
					ImGui::EndPopup();
				}
				ImGui::EndMainMenuBar();
			}

			if (ImGui::Button("URL")) {
				std::string auth_url = spotify.get_auth_url();
				std::cout << auth_url << std::endl;
				Platform::open_url(auth_url);
			}
			if (ImGui::Button(ICON_FA_PLAY, ImVec2(16, 16))) {
				spotify.play();
			}
			if (ImGui::Button(ICON_FA_PAUSE, ImVec2(16, 16))) {
				spotify.pause();
			}

			ImGui::PopStyleVar(2);
			ImGui::End();
		} else {
			ImGui::PopStyleVar(5);
		}

		ImGui::Render();
		ImDrawData *draw_data = ImGui::GetDrawData();
		const bool is_minimized = (draw_data->DisplaySize.x <= 0.f || draw_data->DisplaySize.y <= 0.f);
		if (!is_minimized) {
			ERR_FAIL_COND_RET_SDL(!imgui_engine.frame_render(&window, draw_data, window.width, window.height), 1, "Failed to render frame.");
			ERR_FAIL_COND_RET_SDL(!imgui_engine.frame_present(&window), 1, "Failed to present frame to queue.");
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
		uint64_t end = SDL_GetTicks();
		if (end - start < target_ms) {
			SDL_Delay(end - start);
		}
	}

	CLEANUP_SDL();
	return 0;
}
