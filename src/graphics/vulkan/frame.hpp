#pragma once

/**
 * frame.hpp
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

struct Frame {
	VkCommandPool command_pool = VK_NULL_HANDLE;
	VkCommandBuffer command_buffer = VK_NULL_HANDLE;
	VkFence fence = VK_NULL_HANDLE;
	VkImage buffer = VK_NULL_HANDLE;
	VkImageView buffer_view = VK_NULL_HANDLE;
	VkFramebuffer frame_buffer = VK_NULL_HANDLE;

	void cleanup(VkDevice p_device) {
		if (fence != VK_NULL_HANDLE) {
			vkDestroyFence(p_device, fence, nullptr);
			fence = VK_NULL_HANDLE;
		}

		if (command_pool != VK_NULL_HANDLE) {
			if (command_buffer != VK_NULL_HANDLE) {
				vkFreeCommandBuffers(p_device, command_pool, 1, &command_buffer);
				command_buffer = VK_NULL_HANDLE;
			}
			vkDestroyCommandPool(p_device, command_pool, nullptr);
			command_pool = VK_NULL_HANDLE;
		}

		if (buffer_view != VK_NULL_HANDLE) {
			vkDestroyImageView(p_device, buffer_view, nullptr);
			buffer_view = VK_NULL_HANDLE;
		}
		if (frame_buffer != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(p_device, frame_buffer, nullptr);
			frame_buffer = VK_NULL_HANDLE;
		}
	}
};

struct FrameSemaphores {
	VkSemaphore image_acquired_semaphore = VK_NULL_HANDLE;
	VkSemaphore render_complete_sempahore = VK_NULL_HANDLE;

	void cleanup(VkDevice p_device) {
		if (image_acquired_semaphore != VK_NULL_HANDLE) {
			vkDestroySemaphore(p_device, image_acquired_semaphore, nullptr);
			image_acquired_semaphore = VK_NULL_HANDLE;
		}
		if (render_complete_sempahore != VK_NULL_HANDLE) {
			vkDestroySemaphore(p_device, render_complete_sempahore, nullptr);
			render_complete_sempahore = VK_NULL_HANDLE;
		}
	}
};
