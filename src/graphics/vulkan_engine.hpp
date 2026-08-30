#pragma once

/**
 * vulkan_engine.hpp
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

#include <set>
#include <vector>

#include "error_macros.hpp"
#include "graphics/vulkan/frame.hpp"
#include "vma.hpp"

const std::vector<const char *> essential_instance_extensions{};

const std::vector<const char *> essential_device_extensions{
	// VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
	VK_KHR_SWAPCHAIN_EXTENSION_NAME, // needed for app to work decently
};

const std::vector<const char *> optional_instance_extensions{
	VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, // used in VulkanEngine::pick_physical_device
	VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, // used in VulkanEngine::create_swapchain
#ifdef DEBUG
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
};

const std::vector<const char *> optional_device_extensions{
	// VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
	VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, // used in VulkanEngine::create_render_pass
	VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
};

class VulkanEngine {
public:
	VulkanEngine(SDL_Window *p_window);

	SDL_Window *window = nullptr;
	VkInstance instance = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	uint32_t queue_family = 0;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue queue = VK_NULL_HANDLE;
	VmaAllocator allocator = VK_NULL_HANDLE;
	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
	std::vector<uint8_t> pipeline_data;
	VkPipelineCache pipeline_cache = VK_NULL_HANDLE;

	std::set<const char *> active_instance_extensions;
	std::set<const char *> active_device_extensions;

	VkPhysicalDeviceSurfaceInfo2KHR surface_info{};

	VkRenderPass render_pass = VK_NULL_HANDLE;
	uint32_t min_image_count = 3;
	VkFormat swapchain_format = VK_FORMAT_UNDEFINED;
	VkColorSpaceKHR swapchain_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	uint32_t swapchain_images_count;
	bool swapchain_rebuild = false;

	std::vector<Frame> frames;
	std::vector<FrameSemaphores> frame_semaphores;
	uint32_t semaphore_count = min_image_count + 1;

	bool create_instance();
	bool create_surface();
	bool pick_physical_device();
	bool create_device();
	bool create_allocator();
	bool create_descriptor_pool();
	bool create_pipeline_cache();
	bool create_render_pass();
	bool setup_swapchain();
	bool create_swapchain(int p_width, int p_height);
	bool create_command_buffers();
	bool create_window(int p_width, int p_height);

	void cleanup_window();
	void cleanup();
	~VulkanEngine();

private:
#ifdef DEBUG
	bool check_validation_layer_support();
#endif
	bool is_extension_available(std::vector<VkExtensionProperties> &p_extension_properties, const char *p_extension);
	bool add_essential_extension(std::vector<VkExtensionProperties> &p_extension_properties, const char *p_extension, bool p_device, std::vector<const char *> &r_extensions);
	bool add_optional_extension(std::vector<VkExtensionProperties> &p_extension_properties, const char *p_extension, bool p_device, std::vector<const char *> &r_extensions);
};
