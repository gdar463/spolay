/**
 * vulkan_engine.cpp
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

#include "vulkan_engine.hpp"

#include <SDL3/SDL_vulkan.h>
#include <imgui_impl_vulkan.h>

#include <cstring>

VulkanEngine::VulkanEngine(SDL_Window *p_window) {
	window = p_window;
}
VulkanEngine::~VulkanEngine() {
	cleanup();
}
void VulkanEngine::cleanup() {
	if (device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(device);

		if (render_pass != VK_NULL_HANDLE) {
			vkDestroyRenderPass(device, render_pass, nullptr);
		}
		if (pipeline_cache != VK_NULL_HANDLE) {
			vkDestroyPipelineCache(device, pipeline_cache, nullptr);
		}
		if (descriptor_pool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
		}
		if (allocator != VK_NULL_HANDLE) {
			vmaDestroyAllocator(allocator);
		}
		if (surface != VK_NULL_HANDLE) {
			vkDestroySurfaceKHR(instance, surface, nullptr);
		}
		vkDestroyDevice(device, nullptr);
	}

	if (instance != VK_NULL_HANDLE) {
		vkDestroyInstance(instance, nullptr);
	}
}

bool VulkanEngine::create_surface() {
	ERR_FAIL_COND_RET(!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface), false, "Failed to create Vulkan surface.");
	return true;
}
bool VulkanEngine::pick_physical_device() {
	uint32_t physical_devices_count = 0;
	ERR_FAIL_VK_RET(vkEnumeratePhysicalDevices(instance, &physical_devices_count, nullptr), false, "Failed to enumerate physical devices.");
	ERR_FAIL_COND_RET(physical_devices_count == 0, false, "No GPU with Vulkan support found.");

	std::vector<VkPhysicalDevice> physical_devices(physical_devices_count);
	ERR_FAIL_VK_RET(vkEnumeratePhysicalDevices(instance, &physical_devices_count, physical_devices.data()), false, "Failed to enumerate physical devices.");

	if (active_instance_extensions.contains(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
		for (const VkPhysicalDevice &_physical_device : physical_devices) {
			VkPhysicalDeviceProperties2 physical_device_properties{};
			physical_device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			vkGetPhysicalDeviceProperties2(_physical_device, &physical_device_properties);

			if (physical_device_properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				physical_device = _physical_device;
				break;
			}
		}
	} else {
		for (const VkPhysicalDevice &_physical_device : physical_devices) {
			VkPhysicalDeviceProperties physical_device_properties;
			vkGetPhysicalDeviceProperties(_physical_device, &physical_device_properties);

			if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				physical_device = _physical_device;
				break;
			}
		}
	}
	if (physical_device == VK_NULL_HANDLE) {
		physical_device = physical_devices[0];
	}

	uint32_t queue_family_properties_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_properties_count, nullptr);
	ERR_FAIL_COND_RET(queue_family_properties_count == 0, false, "Failed to retrieve queue family properties for physical device.");

	std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_properties_count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_properties_count, queue_family_properties.data());

	for (uint32_t i = 0; i < queue_family_properties_count; i++) {
		if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			queue_family = i;
			return true;
		}
	}
	ERR_FAIL_MSG_RET(false, "Failed to select a queue family.");
}
bool VulkanEngine::create_device() {
	float queue_priority = 1.0f;
	VkDeviceQueueCreateInfo queue_create_info{};
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex = queue_family;
	queue_create_info.queueCount = 1;
	queue_create_info.pQueuePriorities = &queue_priority;

	VkPhysicalDeviceFeatures physical_device_features{};
	physical_device_features.multiViewport = VK_TRUE;

	VkDeviceCreateInfo device_create_info{};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.pQueueCreateInfos = &queue_create_info;
	device_create_info.queueCreateInfoCount = 1;
	device_create_info.pEnabledFeatures = &physical_device_features;

	std::vector<const char *> extensions;

	uint32_t extension_properties_count;
	ERR_FAIL_VK_RET(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_properties_count, nullptr), false, "Faled to enumerate instance extensions.");
	std::vector<VkExtensionProperties> extension_properties(extension_properties_count);
	ERR_FAIL_VK_RET(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_properties_count, extension_properties.data()), false, "Faled to enumerate instance extensions.");

	for (const char *const &ext_name : essential_device_extensions) {
		ERR_FAIL_COND_SILENT_RET(!add_essential_extension(extension_properties, ext_name, true, extensions), false);
	}
	for (const char *const &ext_name : optional_device_extensions) {
		add_optional_extension(extension_properties, ext_name, true, extensions);
	}

	device_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	device_create_info.ppEnabledExtensionNames = extensions.data();

	ERR_FAIL_VK_RET(vkCreateDevice(physical_device, &device_create_info, nullptr, &device), false, "Failed to create logical device.");
	ERR_FAIL_COND_RET(device == VK_NULL_HANDLE, false, "VkDevice is null, but create did not error.");

	volkLoadDevice(device);

	vkGetDeviceQueue(device, queue_family, 0, &queue);
	return true;
}
bool VulkanEngine::create_allocator() {
	VmaAllocatorCreateInfo allocator_create_info{};
	allocator_create_info.vulkanApiVersion = APP_VULKAN_API_VERSION;
	allocator_create_info.physicalDevice = physical_device;
	allocator_create_info.device = device;
	allocator_create_info.instance = instance;

	VmaVulkanFunctions vulkan_functions{};
	ERR_FAIL_VK_RET(vmaImportVulkanFunctionsFromVolk(&allocator_create_info, &vulkan_functions), false, "Failed to import vulkan functions from volk.");
	allocator_create_info.pVulkanFunctions = &vulkan_functions;

	ERR_FAIL_VK_RET(vmaCreateAllocator(&allocator_create_info, &allocator), false, "Failed to create VMA allocator.");
	ERR_FAIL_COND_RET(allocator == VK_NULL_HANDLE, false, "VmaAllocator is null, but create did not error.");
	return true;
}
bool VulkanEngine::create_ImGui_pool() {
	VkDescriptorPoolSize pool_sizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE * 2 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE * 2 },
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 0;
	for (const VkDescriptorPoolSize &pool_size : pool_sizes) {
		pool_info.maxSets += pool_size.descriptorCount;
	}
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	ERR_FAIL_VK_RET(vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool), false, "Failed to create descriptor pool.");
	ERR_FAIL_COND_RET(descriptor_pool == VK_NULL_HANDLE, false, "VkRenderPass is null, but create did not error.");
	return true;
}
bool VulkanEngine::create_pipeline_cache() {
	VkPipelineCacheCreateInfo pipeline_cache_create_info{};
	pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipeline_cache_create_info.initialDataSize = 0;
	ERR_FAIL_VK_RET(vkCreatePipelineCache(device, &pipeline_cache_create_info, nullptr, &pipeline_cache), false, "Failed to create pipeline cache.");
	ERR_FAIL_COND_RET(pipeline_cache == VK_NULL_HANDLE, false, "VkPipelineCache is null, but create did not error.");
	return true;
}
bool VulkanEngine::create_swapchain() {
	VkBool32 res;
	ERR_FAIL_VK_RET(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family, surface, &res), false, "Physical device doesn't support WSI.");
	const std::vector<VkFormat> request_surface_info_formats{ VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
	const VkColorSpaceKHR request_surface_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	VkPhysicalDeviceSurfaceInfo2KHR surface_info{};
	surface_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
	surface_info.surface = surface;
	uint32_t surface_formats_count = 0;
	ERR_FAIL_VK_RET(vkGetPhysicalDeviceSurfaceFormats2KHR(physical_device, &surface_info, &surface_formats_count, nullptr), false, "Failed to get physical device surface formats.");
	std::vector<VkSurfaceFormat2KHR> surface_formats(surface_formats_count);
	for (uint32_t i = 0; i < surface_formats_count; i++) {
		surface_formats[i].sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
	}
	ERR_FAIL_VK_RET(vkGetPhysicalDeviceSurfaceFormats2KHR(physical_device, &surface_info, &surface_formats_count, surface_formats.data()), false, "Failed to get physical device surface formats.");

	if (surface_formats_count == 1) {
		if (surface_formats[0].surfaceFormat.format == VK_FORMAT_UNDEFINED) {
			swapchain_format = request_surface_info_formats[0];
			swapchain_color_space = request_surface_color_space;
		} else {
			swapchain_format = surface_formats[0].surfaceFormat.format;
			swapchain_color_space = surface_formats[0].surfaceFormat.colorSpace;
		}
	} else {
		for (uint32_t i = 0; i < request_surface_info_formats.size(); i++) {
			for (uint32_t j = 0; j < surface_formats_count; j++) {
				if (surface_formats[j].surfaceFormat.format == request_surface_info_formats[i] && surface_formats[j].surfaceFormat.colorSpace == request_surface_color_space) {
					swapchain_format = surface_formats[j].surfaceFormat.format;
					swapchain_color_space = surface_formats[j].surfaceFormat.colorSpace;
					goto endloop;
				}
			}
		}
		swapchain_format = surface_formats[0].surfaceFormat.format;
		swapchain_color_space = surface_formats[0].surfaceFormat.colorSpace;
	}
endloop:
	swapchain_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
	return true;
}
bool VulkanEngine::create_render_pass() {
	if (active_device_extensions.contains(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME)) {
		VkAttachmentDescription2 color_attachment{};
		color_attachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
		color_attachment.format = swapchain_format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference2 color_attachment_ref{};
		color_attachment_ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_attachment_ref.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		VkSubpassDescription2 subpass_description{};
		subpass_description.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
		subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_description.colorAttachmentCount = 1;
		subpass_description.pColorAttachments = &color_attachment_ref;

		VkSubpassDependency2 subpass_dependency{};
		subpass_dependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
		subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpass_dependency.dstSubpass = 0;
		subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpass_dependency.srcAccessMask = 0;
		subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo2 render_pass_create_info{};
		render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
		render_pass_create_info.attachmentCount = 1;
		render_pass_create_info.pAttachments = &color_attachment;
		render_pass_create_info.subpassCount = 1;
		render_pass_create_info.pSubpasses = &subpass_description;
		render_pass_create_info.dependencyCount = 1;
		render_pass_create_info.pDependencies = &subpass_dependency;
		ERR_FAIL_VK_RET(vkCreateRenderPass2(device, &render_pass_create_info, nullptr, &render_pass), false, "Failed to create render pass.");
	} else {
		VkAttachmentDescription color_attachment{};
		color_attachment.format = swapchain_format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment_ref{};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass_description{};
		subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_description.colorAttachmentCount = 1;
		subpass_description.pColorAttachments = &color_attachment_ref;

		VkSubpassDependency subpass_dependency{};
		subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpass_dependency.dstSubpass = 0;
		subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpass_dependency.srcAccessMask = 0;
		subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo render_pass_create_info{};
		render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_create_info.attachmentCount = 1;
		render_pass_create_info.pAttachments = &color_attachment;
		render_pass_create_info.subpassCount = 1;
		render_pass_create_info.pSubpasses = &subpass_description;
		render_pass_create_info.dependencyCount = 1;
		render_pass_create_info.pDependencies = &subpass_dependency;
		ERR_FAIL_VK_RET(vkCreateRenderPass(device, &render_pass_create_info, nullptr, &render_pass), false, "Failed to create render pass.");
	}
	ERR_FAIL_COND_RET(render_pass == VK_NULL_HANDLE, false, "VkRenderPass is null, but create did not error.");
	return true;
}

#ifdef DEBUG
#define VK_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"

const std::vector<const char *> debug_layers = {
	VK_VALIDATION_LAYER_NAME
};

bool VulkanEngine::check_validation_layer_support() {
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	for (const VkLayerProperties &layer_properties : available_layers) {
		if (strcmp(VK_VALIDATION_LAYER_NAME, layer_properties.layerName) == 0) {
			return true;
		}
	}
	return false;
}
#endif

bool VulkanEngine::create_instance() {
	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = APP_NAME;
	app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	app_info.pEngineName = "No Engine";
	app_info.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	app_info.apiVersion = APP_VULKAN_API_VERSION;

	VkInstanceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;

	putenv(strdup("VK_LOADER_LAYERS_DISABLE=*implicit*"));
#ifdef DEBUG
	if (check_validation_layer_support()) {
		create_info.enabledLayerCount = debug_layers.size();
		create_info.ppEnabledLayerNames = debug_layers.data();
	} else {
		create_info.enabledLayerCount = 0;
	}
#else
	create_info.enabledLayerCount = 0;
#endif

	uint32_t sdl_extenstions_count;
	const char *const *sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_extenstions_count);
	std::vector<const char *> extensions(sdl_extensions, sdl_extensions + sdl_extenstions_count);

	uint32_t extension_properties_count;
	ERR_FAIL_VK_RET(vkEnumerateInstanceExtensionProperties(nullptr, &extension_properties_count, nullptr), false, "Faled to enumerate instance extensions.");
	std::vector<VkExtensionProperties> extension_properties(extension_properties_count);
	ERR_FAIL_VK_RET(vkEnumerateInstanceExtensionProperties(nullptr, &extension_properties_count, extension_properties.data()), false, "Faled to enumerate instance extensions.");

	for (const char *const &ext_name : essential_instance_extensions) {
		ERR_FAIL_COND_SILENT_RET(!add_essential_extension(extension_properties, ext_name, false, extensions), false);
	}
	for (const char *const &ext_name : optional_instance_extensions) {
		add_optional_extension(extension_properties, ext_name, false, extensions);
	}

	create_info.enabledExtensionCount = extensions.size();
	create_info.ppEnabledExtensionNames = extensions.data();

	ERR_FAIL_VK_RET(vkCreateInstance(&create_info, nullptr, &instance), false, "Failed to create Vulkan instance");
	ERR_FAIL_COND_RET(instance == VK_NULL_HANDLE, false, "VkInstance is null, but create did not error.");

	volkLoadInstance(instance);
	return true;
}

bool VulkanEngine::is_extension_available(std::vector<VkExtensionProperties> &p_extension_properties, const char *p_extension) {
	for (const VkExtensionProperties &extension_properties : p_extension_properties) {
		if (strcmp(extension_properties.extensionName, p_extension) == 0) {
			return true;
		}
	}
	return false;
}
bool VulkanEngine::add_essential_extension(std::vector<VkExtensionProperties> &p_extension_properties, const char *p_extension, bool p_device, std::vector<const char *> &r_extensions) {
	if (likely(is_extension_available(p_extension_properties, p_extension))) {
		r_extensions.push_back(p_extension);
		if (p_device) {
			active_device_extensions.insert(p_extension);
		} else {
			active_instance_extensions.insert(p_extension);
		}
		return true;
	}
	ERR_FAIL_MSG_RET(false, std::string("Missing essential ").append(p_extension).append(" extension."));
}
bool VulkanEngine::add_optional_extension(std::vector<VkExtensionProperties> &p_extension_properties, const char *p_extension, bool p_device, std::vector<const char *> &r_extensions) {
	if (likely(is_extension_available(p_extension_properties, p_extension))) {
		r_extensions.push_back(p_extension);
		if (p_device) {
			active_device_extensions.insert(p_extension);
		} else {
			active_instance_extensions.insert(p_extension);
		}
		return true;
	}
	return false;
}
