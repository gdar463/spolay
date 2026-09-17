#ifdef WIN32

/**
 * windows.cpp
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
#include <windows.h>

#include "error_macros.hpp"
#include "platform.hpp"

bool Platform::begin_native_window(SDL_Window *p_window) {
	SDL_PropertiesID properties = SDL_GetWindowProperties(p_window);
	HWND hwnd = (HWND)SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
	ERR_FAIL_NULL_RET(hwnd, false, SDL_GetError());

	LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
	style |= WS_OVERLAPPEDWINDOW;
	style &= ~WS_POPUP;
	SetWindowLongPtr(hwnd, GWL_STYLE, style);

	LONG_PTR ex_style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
	ex_style |= WS_EX_COMPOSITED | WS_EX_LAYERED;
	SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex_style);
	return true;
}
void Platform::open_url(std::string &p_url) {
	ShellExecute(nullptr, nullptr, p_url.c_str(), nullptr, nullptr, 0);
}
#endif
