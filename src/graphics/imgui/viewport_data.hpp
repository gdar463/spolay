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

#include "graphics/vulkan/render_buffers.hpp"
#include "graphics/vulkan/window.hpp"

struct WindowRenderBuffers {
	std::vector<RenderBuffers> buffers;
	uint32_t count;
	uint32_t index;

	void cleanup(VmaAllocator p_allocator) {
		if (count > 0) {
			for (uint32_t i = 0; i < count; i++) {
				buffers[i].cleanup(p_allocator);
			}
		}
	}
};

struct ViewportData {
	Window window;
	WindowRenderBuffers render_buffers;

	void cleanup(VmaAllocator p_allocator) {
		render_buffers.cleanup(p_allocator);
	}
};
