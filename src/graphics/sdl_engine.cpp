/**
 * sdl_engine.cpp
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

#include "sdl_engine.hpp"

#include <volk.h>

#include <imgui.h>

#include "SDL3/SDL_vulkan.h"
#ifdef WIN32
#include <windows.h>
#endif

#include "error_macros.hpp"
#include "graphics/sdl/viewport_data.hpp"

void SdlEngine::cleanup() {
	SdlBackendData *bd = get_backend_data();
	if (!bd) {
		return;
	}

	ImGuiIO &io = ImGui::GetIO();
	ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();

	ImGui::DestroyPlatformWindows();

	bd->cleanup();

	io.BackendPlatformName = nullptr;
	io.BackendPlatformUserData = nullptr;
	platform_io.ClearPlatformHandlers();
	delete bd;
	bd = nullptr;
}

bool SdlEngine::setup(SDL_Window *p_window) {
	ImGuiIO &io = ImGui::GetIO();
	IMGUI_CHECKVERSION();

	ERR_FAIL_COND_RET(io.BackendPlatformUserData, false, "Already initialized backend platform.");
	SdlBackendData *bd = new SdlBackendData();
	ERR_FAIL_NULL_RET(bd, false, "BackendData intialiazed to null.");
	io.BackendPlatformUserData = (void *)bd;
	ERR_FAIL_NULL_RET(io.BackendPlatformUserData, false, "BackendData failed to get assigned to io.");
	ERR_FAIL_NULL_RET(get_backend_data(), false, "BackendData failed to get assigned to io.");
	io.BackendPlatformName = "imgui_impl_sdl__spolay";
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
	io.BackendFlags |= ImGuiBackendFlags_HasParentViewport;

	bd->window = p_window;
	bd->window_id = SDL_GetWindowID(p_window);

	ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
	platform_io.Platform_SetClipboardTextFn = set_clipboard_text;
	platform_io.Platform_GetClipboardTextFn = get_clipboard_text;
	platform_io.Platform_SetImeDataFn = set_ime_data;
	platform_io.Platform_OpenInShellFn = [](ImGuiContext *, const char *p_url) { return SDL_OpenURL(p_url); };

	ERR_FAIL_COND_RET(!update_monitors(), false, "Failed to setup monitors.");

	bd->mouse_cursors[ImGuiMouseCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
	bd->mouse_cursors[ImGuiMouseCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);
	bd->mouse_cursors[ImGuiMouseCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);
	bd->mouse_cursors[ImGuiMouseCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
	bd->mouse_cursors[ImGuiMouseCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
	bd->mouse_cursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NESW_RESIZE);
	bd->mouse_cursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NWSE_RESIZE);
	bd->mouse_cursors[ImGuiMouseCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
	bd->mouse_cursors[ImGuiMouseCursor_Wait] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
	bd->mouse_cursors[ImGuiMouseCursor_Progress] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_PROGRESS);
	bd->mouse_cursors[ImGuiMouseCursor_NotAllowed] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NOT_ALLOWED);

	bd->mouse_capture_mode = MouseCaptureMode_Enabled;

	ImGuiViewport *main_viewport = ImGui::GetMainViewport();
	ERR_FAIL_COND_RET(!setup_viewport(main_viewport, p_window), false, "Failed to setup main viewport.");

	ERR_FAIL_COND_RET(!SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1"), false, SDL_GetError());
	ERR_FAIL_COND_RET(!SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0"), false, SDL_GetError());
	ERR_FAIL_COND_RET(!SDL_SetHint("SDL_BORDERLESS_WINDOWED_STYLE", "0"), false, SDL_GetError());

	ERR_FAIL_COND_RET(!setup_multi_viewport(p_window), false, "Failed to setup multi viewport support.");

	bd = nullptr;
	return true;
}
bool SdlEngine::update_monitors() {
	SdlBackendData *bd = get_backend_data();
	ERR_FAIL_NULL_RET(bd, false, "BackendData is null.");
	ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();

	platform_io.Monitors.resize(0);

	int displays_count;
	SDL_DisplayID *displays = SDL_GetDisplays(&displays_count);
	ERR_FAIL_NULL_RET(displays, false, SDL_GetError());
	bd->update_monitors = false;
	for (int i = 0; i < displays_count; i++) {
		SDL_DisplayID display_id = displays[i];
		ImGuiPlatformMonitor monitor{};
		monitor.DpiScale = SDL_GetDisplayContentScale(display_id);
		if (monitor.DpiScale <= 0.f) {
			continue;
		}
		SDL_Rect bounds;
		ERR_FAIL_COND_RET(!SDL_GetDisplayBounds(display_id, &bounds), false, "Failed to get display " + itos(i) + " bounds.");
		monitor.MainPos = ImVec2(bounds.x, bounds.y);
		monitor.MainSize = ImVec2(bounds.w, bounds.h);
		monitor.WorkPos = monitor.MainPos;
		monitor.WorkSize = monitor.MainSize;
		if (SDL_GetDisplayUsableBounds(display_id, &bounds) && bounds.w > 0 && bounds.h > 0) {
			monitor.WorkPos = ImVec2(bounds.x, bounds.y);
			monitor.WorkPos = ImVec2(bounds.x, bounds.y);
		}
		monitor.PlatformHandle = (void *)(intptr_t)i;
		platform_io.Monitors.push_back(monitor);
	}
	SDL_free(displays);
	return true;
}
bool SdlEngine::setup_multi_viewport(SDL_Window *p_window) {
	ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
	platform_io.Platform_CreateWindow = create_window;
	platform_io.Platform_DestroyWindow = destroy_window;
	platform_io.Platform_ShowWindow = show_window;
	platform_io.Platform_UpdateWindow = update_window;
	platform_io.Platform_SetWindowPos = set_window_pos;
	platform_io.Platform_GetWindowPos = get_window_pos;
	platform_io.Platform_SetWindowSize = set_window_size;
	platform_io.Platform_GetWindowSize = get_window_size;
	platform_io.Platform_GetWindowFramebufferScale = get_window_framebuffer_scale;
	platform_io.Platform_SetWindowFocus = set_window_focus;
	platform_io.Platform_GetWindowFocus = get_window_focus;
	platform_io.Platform_GetWindowMinimized = get_window_minimized;
	platform_io.Platform_SetWindowTitle = set_window_title;
	platform_io.Platform_RenderWindow = render_window;
	platform_io.Platform_SwapBuffers = swap_buffers;
	platform_io.Platform_SetWindowAlpha = set_window_alpha;
	platform_io.Platform_CreateVkSurface = create_vk_surface;

	ImGuiViewport *main_viewport = ImGui::GetMainViewport();
	SdlViewportData *vd = new SdlViewportData();
	vd->window = p_window;
	vd->window_id = SDL_GetWindowID(p_window);
	main_viewport->PlatformUserData = vd;
	main_viewport->PlatformHandle = (void *)(intptr_t)vd->window_id;
	vd = nullptr;

	return true;
}

bool SdlEngine::setup_viewport(ImGuiViewport *p_viewport, SDL_Window *p_window) {
	p_viewport->PlatformHandle = (void *)(intptr_t)SDL_GetWindowID(p_window);
	p_viewport->PlatformHandleRaw = nullptr;
#ifdef WIN32
	p_viewport->PlatformHandleRaw = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(p_window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#endif
	return true;
}

bool SdlEngine::new_frame() {
	SdlBackendData *bd = get_backend_data();
	ERR_FAIL_NULL_RET(bd, false, "BackendData is null.");
	ImGuiIO &io = ImGui::GetIO();

	get_window_size_and_framebuffer_scale(bd->window, &io.DisplaySize, &io.DisplayFramebufferScale);

	if (bd->update_monitors) {
		ERR_FAIL_COND_RET(!update_monitors(), false, "Failed to update monitors.");
	}

	static Uint64 frequency = SDL_GetPerformanceFrequency();
	Uint64 current_time = SDL_GetPerformanceCounter();
	if (current_time <= bd->time) {
		current_time = bd->time + 1;
	}
	io.DeltaTime = bd->time > 0 ? (float)((double)(current_time - bd->time) / (double)frequency) : (float)(1.f / 60.f);
	bd->time = current_time;

	if (bd->mouse_pending_leave_frame && bd->mouse_pending_leave_frame >= ImGui::GetFrameCount() && bd->mouse_buttons_down == 0) {
		bd->mouse_window_id = 0;
		bd->mouse_pending_leave_frame = 0;
		io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
	}

	if (ImGui::GetDragDropPayload() == nullptr) {
		io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;
	} else {
		io.BackendFlags &= ~ImGuiBackendFlags_HasMouseHoveredViewport;
	}

	ERR_FAIL_COND_RET(!update_mouse_data(), false, "Failed to update mouse data.");
	ERR_FAIL_COND_RET(!update_mouse_cursor(), false, "Failed to update mouse cursor.");
	ERR_FAIL_COND_RET(!update_ime(), false, "Failed to update ime.");
	return true;
}
bool SdlEngine::process_event(SDL_Event *p_event) {
	SdlBackendData *bd = get_backend_data();
	ERR_FAIL_NULL_RET(bd, false, "BackendData is null.");
	ImGuiIO &io = ImGui::GetIO();

	switch (p_event->type) {
		case SDL_EVENT_MOUSE_MOTION: {
			if (!get_viewport_from_window_id(p_event->motion.windowID)) {
				break;
			}
			float mouse_x = p_event->motion.x, mouse_y = p_event->motion.y;
			int window_x, window_y;
			ERR_FAIL_COND_RET(!SDL_GetWindowPosition(SDL_GetWindowFromID(p_event->motion.windowID), &window_x, &window_y), false, SDL_GetError());
			mouse_x += window_x;
			mouse_y += window_y;
			io.AddMouseSourceEvent(p_event->motion.which == SDL_TOUCH_MOUSEID ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
			io.AddMousePosEvent(mouse_x, mouse_y);
			return true;
		}
		case SDL_EVENT_MOUSE_WHEEL: {
			if (!get_viewport_from_window_id(p_event->wheel.windowID)) {
				break;
			}
			float wheel_x = -p_event->wheel.x, wheel_y = p_event->wheel.y;
			io.AddMouseSourceEvent(p_event->wheel.which == SDL_TOUCH_MOUSEID ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
			io.AddMouseWheelEvent(wheel_x, wheel_y);
			return true;
		}
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP: {
			if (!get_viewport_from_window_id(p_event->button.windowID)) {
				break;
			}
			int mouse_button = -1;
			if (p_event->button.button == SDL_BUTTON_LEFT) {
				mouse_button = 0;
			}
			if (p_event->button.button == SDL_BUTTON_RIGHT) {
				mouse_button = 1;
			}
			if (p_event->button.button == SDL_BUTTON_MIDDLE) {
				mouse_button = 2;
			}
			if (p_event->button.button == SDL_BUTTON_X1) {
				mouse_button = 3;
			}
			if (p_event->button.button == SDL_BUTTON_X2) {
				mouse_button = 4;
			}
			if (mouse_button == -1) {
				break;
			}
			io.AddMouseSourceEvent(p_event->button.which == SDL_TOUCH_MOUSEID ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
			io.AddMouseButtonEvent(mouse_button, (p_event->type == SDL_EVENT_MOUSE_BUTTON_DOWN));
			bd->mouse_buttons_down = (p_event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? (bd->mouse_buttons_down | (1 << mouse_button)) : (bd->mouse_buttons_down & ~(1 << mouse_button));
			return true;
		}
		case SDL_EVENT_TEXT_INPUT: {
			if (!get_viewport_from_window_id(p_event->button.windowID)) {
				break;
			}
			io.AddInputCharactersUTF8(p_event->text.text);
			return true;
		}
		case SDL_EVENT_KEY_UP:
		case SDL_EVENT_KEY_DOWN: {
			ImGuiViewport *viewport = get_viewport_from_window_id(p_event->key.windowID);
			if (!viewport) {
				break;
			}
			SDL_Keymod mod = p_event->key.mod;
			io.AddKeyEvent(ImGuiMod_Ctrl, (mod & SDL_KMOD_CTRL) != 0);
			io.AddKeyEvent(ImGuiMod_Shift, (mod & SDL_KMOD_SHIFT) != 0);
			io.AddKeyEvent(ImGuiMod_Alt, (mod & SDL_KMOD_ALT) != 0);
			io.AddKeyEvent(ImGuiMod_Super, (mod & SDL_KMOD_GUI) != 0);
			ImGuiKey key = sdl_key_to_imgui_key(p_event->key.key, p_event->key.scancode);
			io.AddKeyEvent(key, (p_event->type == SDL_EVENT_KEY_DOWN));
			return true;
		}
		case SDL_EVENT_DISPLAY_ORIENTATION:
		case SDL_EVENT_DISPLAY_ADDED:
		case SDL_EVENT_DISPLAY_REMOVED:
		case SDL_EVENT_DISPLAY_MOVED:
		case SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED:
		case SDL_EVENT_DISPLAY_USABLE_BOUNDS_CHANGED: {
			bd->update_monitors = true;
			return true;
		}
		case SDL_EVENT_WINDOW_MOUSE_ENTER: {
			if (!get_viewport_from_window_id(p_event->window.windowID)) {
				break;
			}
			bd->mouse_window_id = p_event->window.windowID;
			bd->mouse_pending_leave_frame = 0;
			return true;
		}
		case SDL_EVENT_WINDOW_FOCUS_GAINED:
		case SDL_EVENT_WINDOW_FOCUS_LOST: {
			ImGuiViewport *viewport = get_viewport_from_window_id(p_event->window.windowID);
			if (!viewport) {
				break;
			}
			io.AddFocusEvent(p_event->type == SDL_EVENT_WINDOW_FOCUS_GAINED);
			return true;
		}
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
		case SDL_EVENT_WINDOW_MOVED:
		case SDL_EVENT_WINDOW_RESIZED: {
			ImGuiViewport *viewport = get_viewport_from_window_id(p_event->window.windowID);
			if (!viewport) {
				break;
			}
			if (p_event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
				viewport->PlatformRequestClose = true;
			}
			if (p_event->type == SDL_EVENT_WINDOW_MOVED) {
				viewport->PlatformRequestMove = true;
			}
			if (p_event->type == SDL_EVENT_WINDOW_RESIZED) {
				viewport->PlatformRequestResize = true;
			}
			return true;
		}
		default:
			[[likely]] break;
	}
	return false;
}

void SdlEngine::set_clipboard_text(ImGuiContext *, const char *p_title) {
	ERR_FAIL_COND(!SDL_SetClipboardText(p_title), SDL_GetError());
}
const char *SdlEngine::get_clipboard_text(ImGuiContext *) {
	SdlBackendData *bd = get_backend_data();
	ERR_FAIL_NULL_RET(bd, nullptr, "BackendData is null.");

	if (bd->clipboard_text) {
		SDL_free((void *)bd->clipboard_text);
		bd->clipboard_text = nullptr;
	}
	if (SDL_HasClipboardText()) {
		bd->clipboard_text = SDL_GetClipboardText();
	}
	return bd->clipboard_text;
}
void SdlEngine::set_ime_data(ImGuiContext *, ImGuiViewport *, ImGuiPlatformImeData *p_data) {
	SdlBackendData *bd = get_backend_data();
	ERR_FAIL_NULL(bd, "BackendData is null.");

	bd->ime_data = *p_data;
	bd->ime_dirty = true;
	ERR_FAIL_COND(!update_ime(), "Failed to update ime.");
}

void SdlEngine::create_window(ImGuiViewport *p_viewport) {
	SdlBackendData *bd = get_backend_data();
	ERR_FAIL_NULL(bd, "BackendData is null.");

	SdlViewportData *vd = new SdlViewportData();
	p_viewport->PlatformUserData = vd;
	vd->parent = get_window_from_viewport(p_viewport);
	ERR_FAIL_NULL(vd->parent, "Failed to get window from viewport.");

	SDL_WindowFlags flags = SDL_WINDOW_HIDDEN | APP_SDL_FLAGS;
	vd->window = SDL_CreateWindow("No Title", p_viewport->Size.x, p_viewport->Size.y, flags);
	ERR_FAIL_NULL(vd->window, SDL_GetError());
	vd->window_owned = true;
	ERR_FAIL_COND(!SDL_SetWindowParent(vd->window, vd->parent), SDL_GetError());
	ERR_FAIL_COND(!SDL_SetWindowPosition(vd->window, p_viewport->Pos.x, p_viewport->Pos.y), SDL_GetError());

	ERR_FAIL_COND(!setup_viewport(p_viewport, vd->window), "Failed to setup viewport.");
	vd = nullptr;
}
void SdlEngine::destroy_window(ImGuiViewport *p_viewport) {
	SdlViewportData *vd = (SdlViewportData *)p_viewport->PlatformUserData;
	ERR_FAIL_NULL(vd, "ViewportData is null.");

	if (vd) {
		vd->cleanup();
		delete vd;
		vd = nullptr;
	}
	p_viewport->PlatformUserData = nullptr;
	p_viewport->PlatformHandle = nullptr;
}
void SdlEngine::show_window(ImGuiViewport *p_viewport) {
	SdlViewportData *vd = (SdlViewportData *)p_viewport->PlatformUserData;
	ERR_FAIL_NULL(vd, "ViewportData is null.");

#ifdef WIN32
	HWND hwnd = (HWND)p_viewport->PlatformHandleRaw;

	LONG ex_style = GetWindowLong(hwnd, GWL_EXSTYLE);
	ex_style |= WS_EX_APPWINDOW;
	ex_style &= ~WS_EX_TOOLWINDOW;
	ShowWindow(hwnd, SW_HIDE);
	SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
#else
	ERR_FAIL_COND(!SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN, "0"), SDL_GetError());
#endif
	ERR_FAIL_COND(!SDL_ShowWindow(vd->window), SDL_GetError());
}
void SdlEngine::update_window(ImGuiViewport *p_viewport) {
	SdlViewportData *vd = (SdlViewportData *)p_viewport->PlatformUserData;
	ERR_FAIL_NULL(vd, "ViewportData is null.");

	SDL_Window *parent_window = get_window_from_viewport(p_viewport->ParentViewport);
	ERR_FAIL_NULL(parent_window, "Failed to get window for parent viewport.");
	if (parent_window != vd->parent) {
		vd->parent = parent_window;
		ERR_FAIL_COND(!SDL_SetWindowParent(vd->window, vd->parent), SDL_GetError());
	}
}
void SdlEngine::set_window_pos(ImGuiViewport *p_viewport, ImVec2 p_pos) {
	SdlViewportData *vd = (SdlViewportData *)p_viewport->PlatformUserData;
	ERR_FAIL_NULL(vd, "ViewportData is null.");

	ERR_FAIL_COND(!SDL_SetWindowPosition(vd->window, p_pos.x, p_pos.y), SDL_GetError());
}
ImVec2 SdlEngine::get_window_pos(ImGuiViewport *p_viewport) {
	SdlViewportData *vd = (SdlViewportData *)p_viewport->PlatformUserData;
	ERR_FAIL_NULL_RET(vd, ImVec2(0, 0), "ViewportData is null.");

	int x = 0, y = 0;
	ERR_FAIL_COND_RET(!SDL_GetWindowPosition(vd->window, &x, &y), ImVec2(0, 0), SDL_GetError());
	return ImVec2(x, y);
}
void SdlEngine::set_window_size(ImGuiViewport *p_viewport, ImVec2 p_size) {
	SdlViewportData *vd = (SdlViewportData *)p_viewport->PlatformUserData;
	ERR_FAIL_NULL(vd, "ViewportData is null.");

	ERR_FAIL_COND(!SDL_SetWindowSize(vd->window, p_size.x, p_size.y), SDL_GetError());
}
ImVec2 SdlEngine::get_window_size(ImGuiViewport *p_viewport) {
	SdlViewportData *vd = (SdlViewportData *)p_viewport->PlatformUserData;
	ERR_FAIL_NULL_RET(vd, ImVec2(0, 0), "ViewportData is null.");

	int w = 0, h = 0;
	ERR_FAIL_COND_RET(!SDL_GetWindowSize(vd->window, &w, &h), ImVec2(0, 0), SDL_GetError());
	return ImVec2(w, h);
}
ImVec2 SdlEngine::get_window_framebuffer_scale(ImGuiViewport *p_viewport) {
	SdlViewportData *vd = (SdlViewportData *)p_viewport->PlatformUserData;
	ERR_FAIL_NULL_RET(vd, ImVec2(0, 0), "ViewportData is null.");

	ImVec2 framebuffer_scale{};
	ERR_FAIL_COND_RET(!get_window_size_and_framebuffer_scale(vd->window, nullptr, &framebuffer_scale), ImVec2(0, 0), "Failed to get framebuffer scale.");
	return framebuffer_scale;
}
void SdlEngine::set_window_focus(ImGuiViewport *p_viewport) {
	SdlViewportData *vd = (SdlViewportData *)p_viewport->PlatformUserData;
	ERR_FAIL_NULL(vd, "ViewportData is null.");

	ERR_FAIL_COND(!SDL_RaiseWindow(vd->window), SDL_GetError());
}
bool SdlEngine::get_window_focus(ImGuiViewport *p_viewport) {
	SdlViewportData *vd = (SdlViewportData *)p_viewport->PlatformUserData;
	ERR_FAIL_NULL_RET(vd, false, "ViewportData is null.");

	return (SDL_GetWindowFlags(vd->window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}
bool SdlEngine::get_window_minimized(ImGuiViewport *p_viewport) {
	SdlViewportData *vd = (SdlViewportData *)p_viewport->PlatformUserData;
	ERR_FAIL_NULL_RET(vd, false, "ViewportData is null.");

	return (SDL_GetWindowFlags(vd->window) & SDL_WINDOW_MINIMIZED) != 0;
}
void SdlEngine::set_window_title(ImGuiViewport *p_viewport, const char *p_title) {
	SdlViewportData *vd = (SdlViewportData *)p_viewport->PlatformUserData;
	ERR_FAIL_NULL(vd, "ViewportData is null.");

	ERR_FAIL_COND(!SDL_SetWindowTitle(vd->window, p_title), SDL_GetError());
}
void SdlEngine::render_window(ImGuiViewport *, void *) {}
void SdlEngine::swap_buffers(ImGuiViewport *, void *) {}
void SdlEngine::set_window_alpha(ImGuiViewport *p_viewport, float p_alpha) {
	SdlViewportData *vd = (SdlViewportData *)p_viewport->PlatformUserData;
	ERR_FAIL_NULL(vd, "ViewportData is null.");

	ERR_FAIL_COND(!SDL_SetWindowOpacity(vd->window, p_alpha), SDL_GetError());
}
int SdlEngine::create_vk_surface(ImGuiViewport *p_viewport, ImU64 p_instance, const void *p_allocator, ImU64 *r_surface) {
	SdlViewportData *vd = (SdlViewportData *)p_viewport->PlatformUserData;
	ERR_FAIL_NULL_RET(vd, VK_NOT_READY, "ViewportData is null.");

	return SDL_Vulkan_CreateSurface(vd->window, (VkInstance)p_instance, (const VkAllocationCallbacks *)p_allocator, (VkSurfaceKHR *)r_surface) ? VK_SUCCESS : VK_NOT_READY;
}

bool SdlEngine::update_mouse_data() {
	SdlBackendData *bd = get_backend_data();
	ERR_FAIL_NULL_RET(bd, false, "BackendData is null.");
	ImGuiIO &io = ImGui::GetIO();

	if (bd->mouse_capture_mode == MouseCaptureMode_Enabled) {
		SDL_CaptureMouse(bd->mouse_buttons_down != 0);
	} else if (bd->mouse_capture_mode == MouseCaptureMode_EnabledAfterDrag) {
		bool want_capture = false;
		for (int i = 0; i < ImGuiMouseButton_COUNT && !want_capture; i++) {
			if (ImGui::IsMouseDragging(i, 1.f)) {
				want_capture = true;
			}
		}
		SDL_CaptureMouse(want_capture);
	}

	SDL_Window *focused_window = SDL_GetKeyboardFocus();
	const bool is_app_focused = (focused_window && (bd->window == focused_window || get_viewport_from_window_id(SDL_GetWindowID(focused_window)) != nullptr));
	if (is_app_focused) {
		if (io.WantSetMousePos) {
			SDL_WarpMouseGlobal(io.MousePos.x, io.MousePos.y);
		}

		SDL_Window *hovered_window = SDL_GetMouseFocus();
		const bool is_relative_mouse_mode = SDL_GetWindowRelativeMouseMode(bd->window);
		if (!hovered_window && bd->mouse_buttons_down == 0 && !is_relative_mouse_mode) {
			float mouse_x, mouse_y;
			int window_x, window_y;
			SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
			SDL_GetWindowPosition(focused_window, &window_x, &window_y);
			mouse_x -= window_x;
			mouse_y -= window_y;
			io.AddMousePosEvent(mouse_x, mouse_y);
		}

		ImGuiViewport *mouse_viewport = get_viewport_from_window_id(bd->mouse_window_id);
		if (!mouse_viewport) {
			io.AddMouseViewportEvent(mouse_viewport->ID);
		} else {
			io.AddMouseViewportEvent(0);
		}
	}
	return true;
}
bool SdlEngine::update_mouse_cursor() {
	SdlBackendData *bd = get_backend_data();
	ERR_FAIL_NULL_RET(bd, false, "BackendData is null.");
	ImGuiIO &io = ImGui::GetIO();

	ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
	if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None) {
		ERR_FAIL_COND_RET(!SDL_HideCursor(), false, SDL_GetError());
	} else {
		SDL_Cursor *expected_cursor = bd->mouse_cursors[imgui_cursor] ? bd->mouse_cursors[imgui_cursor] : bd->mouse_cursors[ImGuiMouseCursor_Arrow];
		if (bd->mouse_last_cursor != expected_cursor) {
			ERR_FAIL_COND_RET(!SDL_SetCursor(expected_cursor), false, SDL_GetError());
			bd->mouse_last_cursor = expected_cursor;
		}
		ERR_FAIL_COND_RET(!SDL_ShowCursor(), false, SDL_GetError());
	}
	return true;
}
bool SdlEngine::update_ime() {
	SdlBackendData *bd = get_backend_data();
	ERR_FAIL_NULL_RET(bd, false, "BackendData is null.");
	SDL_Window *window = SDL_GetKeyboardFocus();

	if ((!(bd->ime_data.WantVisible || bd->ime_data.WantTextInput) || bd->ime_window != window) && bd->ime_window) {
		ERR_FAIL_COND_RET(!SDL_StopTextInput(bd->ime_window), false, SDL_GetError());
		bd->ime_window = nullptr;
	}
	if ((!bd->ime_dirty && bd->ime_window == window) || !window) {
		return true;
	}

	bd->ime_dirty = false;
	if (bd->ime_data.WantVisible) {
		ImVec2 pos{};
		ImGuiViewport *viewport = get_viewport_from_window_id(SDL_GetWindowID(window));
		if (viewport) {
			pos = viewport->Pos;
		}
		SDL_Rect area{};
		area.x = bd->ime_data.InputPos.x - pos.x;
		area.x = bd->ime_data.InputPos.y - pos.y;
		area.w = 1;
		area.h = bd->ime_data.InputLineHeight;
		ERR_FAIL_COND_RET(!SDL_SetTextInputArea(window, &area, 0), false, SDL_GetError());
		bd->ime_window = window;
	}
	if (!SDL_TextInputActive(window) && (bd->ime_data.WantVisible || bd->ime_data.WantTextInput)) {
		ERR_FAIL_COND_RET(!SDL_StartTextInput(window), false, SDL_GetError());
	}
	return true;
}
bool SdlEngine::get_window_size_and_framebuffer_scale(SDL_Window *p_window, ImVec2 *r_size, ImVec2 *r_framebuffer) {
	int w = 0, h = 0;
	ERR_FAIL_COND_RET(!SDL_GetWindowSize(p_window, &w, &h), false, SDL_GetError());
	if (SDL_GetWindowFlags(p_window) & SDL_WINDOW_MINIMIZED) {
		w = 0;
		h = 0;
	}

	int display_w = 0, display_h = 0;
	ERR_FAIL_COND_RET(!SDL_GetWindowSizeInPixels(p_window, &display_w, &display_h), false, SDL_GetError());
	float fb_scale_x = (w > 0) ? display_w / w : 1.f;
	float fb_scale_y = (h > 0) ? display_h / h : 1.f;

	if (r_size) {
		*r_size = ImVec2(w, h);
	}
	if (r_framebuffer) {
		*r_framebuffer = ImVec2(fb_scale_x, fb_scale_y);
	}
	return true;
}

SDL_Window *SdlEngine::get_window_from_viewport(ImGuiViewport *p_viewport) {
	if (p_viewport) {
		SDL_WindowID window_id = (SDL_WindowID)(intptr_t)p_viewport->PlatformHandle;
		SDL_Window *window = SDL_GetWindowFromID(window_id);
		ERR_FAIL_NULL_RET(window, nullptr, SDL_GetError());
		return window;
	}
	return nullptr;
}
ImGuiViewport *SdlEngine::get_viewport_from_window_id(SDL_WindowID p_id) {
	ImGuiViewport *viewport = ImGui::FindViewportByPlatformHandle((void *)(intptr_t)p_id);
	//ERR_FAIL_NULL_RET(viewport, nullptr, "Failed to get viewport from window id.");
	return viewport;
}
ImGuiKey SdlEngine::sdl_key_to_imgui_key(SDL_Keycode p_keycode, SDL_Scancode p_scancode) {
	switch (p_scancode) {
		case SDL_SCANCODE_KP_0:
			return ImGuiKey_Keypad0;
		case SDL_SCANCODE_KP_1:
			return ImGuiKey_Keypad1;
		case SDL_SCANCODE_KP_2:
			return ImGuiKey_Keypad2;
		case SDL_SCANCODE_KP_3:
			return ImGuiKey_Keypad3;
		case SDL_SCANCODE_KP_4:
			return ImGuiKey_Keypad4;
		case SDL_SCANCODE_KP_5:
			return ImGuiKey_Keypad5;
		case SDL_SCANCODE_KP_6:
			return ImGuiKey_Keypad6;
		case SDL_SCANCODE_KP_7:
			return ImGuiKey_Keypad7;
		case SDL_SCANCODE_KP_8:
			return ImGuiKey_Keypad8;
		case SDL_SCANCODE_KP_9:
			return ImGuiKey_Keypad9;
		case SDL_SCANCODE_KP_PERIOD:
			return ImGuiKey_KeypadDecimal;
		case SDL_SCANCODE_KP_DIVIDE:
			return ImGuiKey_KeypadDivide;
		case SDL_SCANCODE_KP_MULTIPLY:
			return ImGuiKey_KeypadMultiply;
		case SDL_SCANCODE_KP_MINUS:
			return ImGuiKey_KeypadSubtract;
		case SDL_SCANCODE_KP_PLUS:
			return ImGuiKey_KeypadAdd;
		case SDL_SCANCODE_KP_ENTER:
			return ImGuiKey_KeypadEnter;
		case SDL_SCANCODE_KP_EQUALS:
			return ImGuiKey_KeypadEqual;
		default:
			[[likely]] break;
	}
	switch (p_keycode) {
		case SDLK_TAB:
			return ImGuiKey_Tab;
		case SDLK_LEFT:
			return ImGuiKey_LeftArrow;
		case SDLK_RIGHT:
			return ImGuiKey_RightArrow;
		case SDLK_UP:
			return ImGuiKey_UpArrow;
		case SDLK_DOWN:
			return ImGuiKey_DownArrow;
		case SDLK_PAGEUP:
			return ImGuiKey_PageUp;
		case SDLK_PAGEDOWN:
			return ImGuiKey_PageDown;
		case SDLK_HOME:
			return ImGuiKey_Home;
		case SDLK_END:
			return ImGuiKey_End;
		case SDLK_INSERT:
			return ImGuiKey_Insert;
		case SDLK_DELETE:
			return ImGuiKey_Delete;
		case SDLK_BACKSPACE:
			return ImGuiKey_Backspace;
		case SDLK_SPACE:
			return ImGuiKey_Space;
		case SDLK_RETURN:
			return ImGuiKey_Enter;
		case SDLK_ESCAPE:
			return ImGuiKey_Escape;
		case SDLK_COMMA:
			return ImGuiKey_Comma;
		case SDLK_PERIOD:
			return ImGuiKey_Period;
		case SDLK_SEMICOLON:
			return ImGuiKey_Semicolon;
		case SDLK_CAPSLOCK:
			return ImGuiKey_CapsLock;
		case SDLK_SCROLLLOCK:
			return ImGuiKey_ScrollLock;
		case SDLK_NUMLOCKCLEAR:
			return ImGuiKey_NumLock;
		case SDLK_PRINTSCREEN:
			return ImGuiKey_PrintScreen;
		case SDLK_PAUSE:
			return ImGuiKey_Pause;
		case SDLK_LCTRL:
			return ImGuiKey_LeftCtrl;
		case SDLK_LSHIFT:
			return ImGuiKey_LeftShift;
		case SDLK_LALT:
			return ImGuiKey_LeftAlt;
		case SDLK_LGUI:
			return ImGuiKey_LeftSuper;
		case SDLK_RCTRL:
			return ImGuiKey_RightCtrl;
		case SDLK_RSHIFT:
			return ImGuiKey_RightShift;
		case SDLK_RALT:
			return ImGuiKey_RightAlt;
		case SDLK_RGUI:
			return ImGuiKey_RightSuper;
		case SDLK_APPLICATION:
			return ImGuiKey_Menu;
		case SDLK_0:
			return ImGuiKey_0;
		case SDLK_1:
			return ImGuiKey_1;
		case SDLK_2:
			return ImGuiKey_2;
		case SDLK_3:
			return ImGuiKey_3;
		case SDLK_4:
			return ImGuiKey_4;
		case SDLK_5:
			return ImGuiKey_5;
		case SDLK_6:
			return ImGuiKey_6;
		case SDLK_7:
			return ImGuiKey_7;
		case SDLK_8:
			return ImGuiKey_8;
		case SDLK_9:
			return ImGuiKey_9;
		case SDLK_A:
			return ImGuiKey_A;
		case SDLK_B:
			return ImGuiKey_B;
		case SDLK_C:
			return ImGuiKey_C;
		case SDLK_D:
			return ImGuiKey_D;
		case SDLK_E:
			return ImGuiKey_E;
		case SDLK_F:
			return ImGuiKey_F;
		case SDLK_G:
			return ImGuiKey_G;
		case SDLK_H:
			return ImGuiKey_H;
		case SDLK_I:
			return ImGuiKey_I;
		case SDLK_J:
			return ImGuiKey_J;
		case SDLK_K:
			return ImGuiKey_K;
		case SDLK_L:
			return ImGuiKey_L;
		case SDLK_M:
			return ImGuiKey_M;
		case SDLK_N:
			return ImGuiKey_N;
		case SDLK_O:
			return ImGuiKey_O;
		case SDLK_P:
			return ImGuiKey_P;
		case SDLK_Q:
			return ImGuiKey_Q;
		case SDLK_R:
			return ImGuiKey_R;
		case SDLK_S:
			return ImGuiKey_S;
		case SDLK_T:
			return ImGuiKey_T;
		case SDLK_U:
			return ImGuiKey_U;
		case SDLK_V:
			return ImGuiKey_V;
		case SDLK_W:
			return ImGuiKey_W;
		case SDLK_X:
			return ImGuiKey_X;
		case SDLK_Y:
			return ImGuiKey_Y;
		case SDLK_Z:
			return ImGuiKey_Z;
		case SDLK_F1:
			return ImGuiKey_F1;
		case SDLK_F2:
			return ImGuiKey_F2;
		case SDLK_F3:
			return ImGuiKey_F3;
		case SDLK_F4:
			return ImGuiKey_F4;
		case SDLK_F5:
			return ImGuiKey_F5;
		case SDLK_F6:
			return ImGuiKey_F6;
		case SDLK_F7:
			return ImGuiKey_F7;
		case SDLK_F8:
			return ImGuiKey_F8;
		case SDLK_F9:
			return ImGuiKey_F9;
		case SDLK_F10:
			return ImGuiKey_F10;
		case SDLK_F11:
			return ImGuiKey_F11;
		case SDLK_F12:
			return ImGuiKey_F12;
		case SDLK_F13:
			return ImGuiKey_F13;
		case SDLK_F14:
			return ImGuiKey_F14;
		case SDLK_F15:
			return ImGuiKey_F15;
		case SDLK_F16:
			return ImGuiKey_F16;
		case SDLK_F17:
			return ImGuiKey_F17;
		case SDLK_F18:
			return ImGuiKey_F18;
		case SDLK_F19:
			return ImGuiKey_F19;
		case SDLK_F20:
			return ImGuiKey_F20;
		case SDLK_F21:
			return ImGuiKey_F21;
		case SDLK_F22:
			return ImGuiKey_F22;
		case SDLK_F23:
			return ImGuiKey_F23;
		case SDLK_F24:
			return ImGuiKey_F24;
		case SDLK_AC_BACK:
			return ImGuiKey_AppBack;
		case SDLK_AC_FORWARD:
			return ImGuiKey_AppForward;
		default:
			break;
	}
	switch (p_scancode) {
		case SDL_SCANCODE_GRAVE:
			return ImGuiKey_GraveAccent;
		case SDL_SCANCODE_MINUS:
			return ImGuiKey_Minus;
		case SDL_SCANCODE_EQUALS:
			return ImGuiKey_Equal;
		case SDL_SCANCODE_LEFTBRACKET:
			return ImGuiKey_LeftBracket;
		case SDL_SCANCODE_RIGHTBRACKET:
			return ImGuiKey_RightBracket;
		case SDL_SCANCODE_NONUSBACKSLASH:
			return ImGuiKey_Oem102;
		case SDL_SCANCODE_BACKSLASH:
			return ImGuiKey_Backslash;
		case SDL_SCANCODE_SEMICOLON:
			return ImGuiKey_Semicolon;
		case SDL_SCANCODE_APOSTROPHE:
			return ImGuiKey_Apostrophe;
		case SDL_SCANCODE_COMMA:
			return ImGuiKey_Comma;
		case SDL_SCANCODE_PERIOD:
			return ImGuiKey_Period;
		case SDL_SCANCODE_SLASH:
			return ImGuiKey_Slash;
		default:
			break;
	}
	return ImGuiKey_None;
}

SdlBackendData *SdlEngine::get_backend_data() {
	return ImGui::GetCurrentContext() ? (SdlBackendData *)(ImGui::GetIO().BackendPlatformUserData) : nullptr;
}
