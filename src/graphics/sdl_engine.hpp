#pragma once

/**
 * sdl_engine.hpp
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

#include "graphics/sdl/backend_data.hpp"

class SdlEngine {
public:
	static bool setup(SDL_Window *p_window);
	static bool update_monitors();
	static bool setup_multi_viewport(SDL_Window *p_window);

	static bool setup_viewport(ImGuiViewport *p_viewport, SDL_Window *p_window);

	static bool new_frame();
	static bool process_event(SDL_Event *p_event);

	static void set_clipboard_text(ImGuiContext *, const char *p_text);
	static const char *get_clipboard_text(ImGuiContext *);
	static void set_ime_data(ImGuiContext *, ImGuiViewport *, ImGuiPlatformImeData *p_data);

	static void create_window(ImGuiViewport *p_viewport);
	static void destroy_window(ImGuiViewport *p_viewport);
	static void show_window(ImGuiViewport *p_viewport);
	static void update_window(ImGuiViewport *p_viewport);
	static void set_window_pos(ImGuiViewport *p_viewport, ImVec2 p_pos);
	static ImVec2 get_window_pos(ImGuiViewport *p_viewport);
	static void set_window_size(ImGuiViewport *p_viewport, ImVec2 p_size);
	static ImVec2 get_window_size(ImGuiViewport *p_viewport);
	static ImVec2 get_window_framebuffer_scale(ImGuiViewport *p_viewport);
	static void set_window_focus(ImGuiViewport *p_viewport);
	static bool get_window_focus(ImGuiViewport *p_viewport);
	static bool get_window_minimized(ImGuiViewport *p_viewport);
	static void set_window_title(ImGuiViewport *p_viewport, const char *p_title);
	static void render_window(ImGuiViewport *, void *);
	static void swap_buffers(ImGuiViewport *, void *);
	static void set_window_alpha(ImGuiViewport *p_viewport, float p_alpha);
	static int create_vk_surface(ImGuiViewport *p_viewport, ImU64 p_instance, const void *p_allocator, ImU64 *r_surface);

	static bool update_mouse_data();
	static bool update_mouse_cursor();
	static bool update_ime();
	static bool get_window_size_and_framebuffer_scale(SDL_Window *p_window, ImVec2 *r_size, ImVec2 *r_framebuffer);

	static SDL_Window *get_window_from_viewport(ImGuiViewport *p_viewport);
	static ImGuiViewport *get_viewport_from_window_id(SDL_WindowID p_id);
	static ImGuiKey sdl_key_to_imgui_key(SDL_Keycode p_keycode, SDL_Scancode p_scancode);

	static void cleanup();

private:
	static SdlBackendData *get_backend_data();
};
