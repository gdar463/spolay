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

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vector>

#include "graphics/vulkan/frame.hpp"

struct Window {
	inline Window() {
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	SDL_Window *sdl_window = nullptr;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkPhysicalDeviceSurfaceInfo2KHR surface_info{};
	int width = 0;
	int height = 0;

	bool swapchain_rebuild = false;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkSurfaceFormatKHR swapchain_format{ VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;

	VkAttachmentDescription color_attachment{};
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

	void cleanup(VkInstance p_instance, VkDevice p_device, bool p_delete_all = false, bool p_destroy_sdl_window = false) {
		for (FrameSemaphores &semaphore : semaphores) {
			semaphore.cleanup(p_device);
		}
		semaphores.clear();
		semaphores_count = 0;
		for (Frame &frame : frames) {
			frame.cleanup(p_device);
		}
		frames.clear();
		frames_count = 0;
		if (render_pass != VK_NULL_HANDLE) {
			vkDestroyRenderPass(p_device, render_pass, nullptr);
			render_pass = VK_NULL_HANDLE;
		}
		if (p_delete_all) {
			if (swapchain != VK_NULL_HANDLE) {
				vkDestroySwapchainKHR(p_device, swapchain, nullptr);
				swapchain = VK_NULL_HANDLE;
			}
			if (surface) {
				SDL_Vulkan_DestroySurface(p_instance, surface, nullptr);
				surface = VK_NULL_HANDLE;
			}
			if (p_destroy_sdl_window) {
				if (sdl_window) {
					SDL_HideWindow(sdl_window);
					SDL_DestroyWindow(sdl_window);
					sdl_window = nullptr;
				}
			}
		}
	}
};
