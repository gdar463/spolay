#pragma once

/**
 * backend_data.hpp
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

#include <SDL3/SDL.h>
#include <imgui.h>

enum MouseCaptureMode {
	MouseCaptureMode_Enabled,
	MouseCaptureMode_EnabledAfterDrag,
	MouseCaptureMode_Disabled
};

struct SdlBackendData {
	SDL_Window *window = nullptr;
	SDL_WindowID window_id = 0;
	Uint64 time = 0;
	const char *clipboard_text = nullptr;
	SDL_Window *ime_window = nullptr;
	ImGuiPlatformImeData ime_data{};
	bool ime_dirty = false;
	bool update_monitors = true;

	SDL_WindowID mouse_window_id = 0;
	SDL_Cursor *mouse_cursors[ImGuiMouseCursor_COUNT];
	SDL_Cursor *mouse_last_cursor = nullptr;
	int mouse_buttons_down = 0;
	int mouse_pending_leave_frame = 0;
	MouseCaptureMode mouse_capture_mode;

	void cleanup() {
		if (clipboard_text) {
			SDL_free((void *)clipboard_text);
			clipboard_text = nullptr;
		}
		for (ImGuiMouseCursor i = 0; i < ImGuiMouseCursor_COUNT; i++) {
			SDL_DestroyCursor(mouse_cursors[i]);
		}
	}
};
