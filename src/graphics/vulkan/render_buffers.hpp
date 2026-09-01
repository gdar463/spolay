#pragma once

/**
 * render_buffers.hpp
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

#include "vma.hpp"

struct RenderBuffers {
	VmaAllocation vertex_allocation = VK_NULL_HANDLE;
	VmaAllocation index_allocation = VK_NULL_HANDLE;
	VkBuffer vertex_buffer = VK_NULL_HANDLE;
	VkDeviceSize vertex_buffer_size = 0;
	VkBuffer index_buffer = VK_NULL_HANDLE;
	VkDeviceSize index_buffer_size = 0;

	void cleanup(VmaAllocator p_allocator) {
		if (index_buffer_size > 0) {
			vmaDestroyBuffer(p_allocator, index_buffer, index_allocation);
			index_buffer = VK_NULL_HANDLE;
			index_allocation = VK_NULL_HANDLE;
		}
		if (vertex_buffer_size > 0) {
			vmaDestroyBuffer(p_allocator, vertex_buffer, vertex_allocation);
			vertex_buffer = VK_NULL_HANDLE;
			vertex_allocation = VK_NULL_HANDLE;
		}
	}
};
