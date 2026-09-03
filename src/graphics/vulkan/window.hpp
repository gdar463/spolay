#pragma once

/**
 * window.hpp
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

#include <vector>

#include "graphics/vulkan/frame.hpp"

struct Window {
	int width = 0;
	int height = 0;

	bool swapchain_rebuild = false;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkSurfaceFormatKHR swapchain_format{ VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;

	VkRenderPass render_pass = VK_NULL_HANDLE;
	std::vector<Frame> frames;
	uint32_t min_image_count = 3;
	uint32_t frames_count = 0;
	uint32_t frame_index = 0;
	bool frame_acquired = false;
	std::vector<FrameSemaphores> semaphores;
	uint32_t semaphores_count = min_image_count + 1;
	uint32_t semaphore_index = 0;
	VkClearValue clear_value{ VkClearColorValue({ .int32 = { 14, 14, 14, 255 } }) };

	void cleanup(VkDevice p_device, bool p_preserve_swapchain = false) {
		for (uint32_t i = 0; i < semaphores_count; i++) {
			semaphores[i].cleanup(p_device);
		}
		for (uint32_t i = 0; i < frames_count; i++) {
			frames[i].cleanup(p_device);
		}
		if (render_pass != VK_NULL_HANDLE) {
			vkDestroyRenderPass(p_device, render_pass, nullptr);
			render_pass = VK_NULL_HANDLE;
		}
		if (!p_preserve_swapchain && swapchain != VK_NULL_HANDLE) {
			vkDestroySwapchainKHR(p_device, swapchain, nullptr);
			swapchain = VK_NULL_HANDLE;
		}
	}
};
