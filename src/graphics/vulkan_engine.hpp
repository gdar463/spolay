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
#include "graphics/vulkan/pipeline_info.hpp"
#include "graphics/vulkan/window.hpp"
#include "vma.hpp"

#ifdef DEBUG
#ifndef NO_DEBUG_UTILS
#define DEBUG_NAME(m_handle, m_type, m_name) set_debug_name((uint64_t)m_handle, m_type, m_name)
#define DEBUG_NAME_VK(m_handle, m_type, m_name) vk->set_debug_name((uint64_t)m_handle, m_type, m_name)
#define DEBUG_BEGIN_QUEUE_REGION(m_name, ...) begin_debug_queue_region(m_name, ImVec4(__VA_ARGS__))
#define DEBUG_BEGIN_QUEUE_REGION_VK(m_name, ...) vk->begin_debug_queue_region(m_name, ImVec4(__VA_ARGS__))
#define DEBUG_INSERT_QUEUE_MARKER(m_name, ...) insert_debug_queue_label(m_name, ImVec4(__VA_ARGS__))
#define DEBUG_INSERT_QUEUE_MARKER_VK(m_name, ...) vk->insert_debug_queue_label(m_name, ImVec4(__VA_ARGS__))
#define DEBUG_END_QUEUE_REGION() end_debug_queue_region()
#define DEBUG_END_QUEUE_REGION_VK() vk->end_debug_queue_region()
#else
#define DEBUG_NAME(m__, m___, m___)
#define DEBUG_NAME_VK(m__, m___, m___)
#define DEBUG_BEGIN_QUEUE_REGION(m__, m___)
#define DEBUG_BEGIN_QUEUE_REGION_VK(m__, m___)
#define DEBUG_INSERT_QUEUE_MARKER(m__, m___)
#define DEBUG_INSERT_QUEUE_MARKER_VK(m__, m___)
#define DEBUG_END_QUEUE_REGION()
#define DEBUG_END_QUEUE_REGION_VK()
#endif
#else
#define DEBUG_NAME(m__, m___, m___)
#define DEBUG_NAME_VK(m__, m___, m___)
#define DEBUG_BEGIN_QUEUE_REGION(m__, m___)
#define DEBUG_BEGIN_QUEUE_REGION_VK(m__, m___)
#define DEBUG_INSERT_QUEUE_MARKER(m__, m___)
#define DEBUG_INSERT_QUEUE_MARKER_VK(m__, m___)
#define DEBUG_END_QUEUE_REGION()
#define DEBUG_END_QUEUE_REGION_VK()
#endif

const std::vector<const char *> essential_instance_extensions{};

const std::vector<const char *> essential_device_extensions{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME, // needed for app to work decently
};

const std::vector<const char *> optional_instance_extensions{
	VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, // used in VulkanEngine::pick_physical_device
	VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, // used in VulkanEngine::create_swapchain
#ifdef DEBUG
#ifndef NO_DEBUG_UTILS
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
#endif
};

const std::vector<const char *> optional_device_extensions{
	// VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
	VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, // used in VulkanEngine::create_render_pass
	VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
	VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
	VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME,
};

class VulkanEngine {
public:
	VulkanEngine(SDL_Window *p_window);

	uint32_t api_version = APP_VULKAN_API_VERSION;

	SDL_Window *sdl_window = nullptr;
	VkInstance instance = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	uint32_t queue_family = 0;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue queue = VK_NULL_HANDLE;
	VmaAllocator allocator = VK_NULL_HANDLE;
	// VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
	std::vector<uint8_t> pipeline_data;
	VkPipelineCache pipeline_cache = VK_NULL_HANDLE;

	std::set<const char *> active_instance_extensions;
	std::set<const char *> active_device_extensions;

	VkPhysicalDeviceSurfaceInfo2KHR surface_info{};
	Window window{};
	PipelineInfo pipeline_info_main{};

	bool create_instance();
	bool create_surface();
	bool pick_physical_device();
	bool create_device();
	bool create_allocator();
	bool create_pipeline_cache();
	bool create_render_pass();
	bool setup_swapchain();
	bool create_swapchain(int p_width, int p_height);
	bool create_command_buffers();
	bool create_window(int p_width, int p_height);

	void cleanup();
	~VulkanEngine();

#ifdef DEBUG
#ifndef NO_DEBUG_UTILS
	void set_debug_name(uint64_t p_handle, VkObjectType p_type, const char *p_name);
	void set_debug_name(uint64_t p_handle, VkObjectType p_type, std::string p_name);

	void begin_debug_queue_region(const char *p_name, const ImVec4 p_color);
	void begin_debug_queue_region(std::string p_name, const ImVec4 p_color);
	void insert_debug_queue_label(const char *p_name, const ImVec4 p_color);
	void insert_debug_queue_label(std::string p_name, const ImVec4 p_color);
	void end_debug_queue_region();
#endif
#endif

private:
#ifdef DEBUG
#ifndef NO_VALIDATION_LAYER
	bool check_validation_layer_support();
#endif
#endif
	bool is_extension_available(std::vector<VkExtensionProperties> &p_extension_properties, const char *p_extension);
	bool add_essential_extension(std::vector<VkExtensionProperties> &p_extension_properties, const char *p_extension, bool p_device, std::vector<const char *> &r_extensions);
	bool add_optional_extension(std::vector<VkExtensionProperties> &p_extension_properties, const char *p_extension, bool p_device, std::vector<const char *> &r_extensions);
};
