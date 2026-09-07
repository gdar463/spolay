#pragma once

/**
 * viewport_data.hpp
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

struct SdlViewportData {
	SDL_Window *window = nullptr;
	SDL_Window *parent = nullptr;
	SDL_WindowID window_id = 0;
	bool window_owned = false;

	void cleanup() {
		if (window_owned) {
			if (window) {
				SDL_HideWindow(window);
				SDL_DestroyWindow(window);
				window = nullptr;
			}
		}
	}
};
