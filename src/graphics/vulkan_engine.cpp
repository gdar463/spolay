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

#include <cstring>

#ifdef DEBUG
#include <iostream>
#endif

VulkanEngine::VulkanEngine(SDL_Window *p_window) {
	sdl_window = p_window;
}
VulkanEngine::~VulkanEngine() {}
void VulkanEngine::cleanup() {
	if (instance != VK_NULL_HANDLE) {
		if (device != VK_NULL_HANDLE) {
			vkDeviceWaitIdle(device);

			window.cleanup(device);
			if (pipeline_cache != VK_NULL_HANDLE) {
				vkDestroyPipelineCache(device, pipeline_cache, nullptr);
			}
			if (allocator != VK_NULL_HANDLE) {
#ifdef DEBUG
				char *stats_string = nullptr;
				vmaBuildStatsString(allocator, &stats_string, VK_TRUE);
				std::cout << "VmaStats: " << stats_string << std::endl;
				vmaFreeStatsString(allocator, stats_string);
#endif
				vmaDestroyAllocator(allocator);
			}
			if (surface != VK_NULL_HANDLE) {
				vkDestroySurfaceKHR(instance, surface, nullptr);
			}
			vkDestroyDevice(device, nullptr);
		}
		vkDestroyInstance(instance, nullptr);
	}
}

bool VulkanEngine::create_surface() {
	ERR_FAIL_COND_RET(!SDL_Vulkan_CreateSurface(sdl_window, instance, nullptr, &surface), false, "Failed to create Vulkan surface.");
	return true;
}
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

	std::vector<const char *> layers{};

	uint32_t layer_properties_count;
	vkEnumerateInstanceLayerProperties(&layer_properties_count, nullptr);
	std::vector<VkLayerProperties> layer_properties(layer_properties_count);
	vkEnumerateInstanceLayerProperties(&layer_properties_count, layer_properties.data());

	for (const char *const &layer_name : optional_layers) {
		add_layer(layer_properties, layer_name, layers);
	}

	create_info.enabledLayerCount = layers.size();
	create_info.ppEnabledLayerNames = layers.data();

	uint32_t sdl_extenstions_count;
	const char *const *sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_extenstions_count);
	std::vector<const char *> extensions(sdl_extensions, sdl_extensions + sdl_extenstions_count);

	uint32_t extension_properties_count;
	ERR_FAIL_VK_RET(vkEnumerateInstanceExtensionProperties(nullptr, &extension_properties_count, nullptr), false, "Faled to enumerate instance extensions.");
	std::vector<VkExtensionProperties> extension_properties(extension_properties_count);
	ERR_FAIL_VK_RET(vkEnumerateInstanceExtensionProperties(nullptr, &extension_properties_count, extension_properties.data()), false, "Faled to enumerate instance extensions.");

	for (const char *const &ext_name : essential_instance_extensions) {
		ERR_FAIL_COND_RET(!add_essential_extension(extension_properties, ext_name, false, extensions), false, "Failed to add essential instance extension.");
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
	float queue_priority = 1.f;
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
		ERR_FAIL_COND_RET(!add_essential_extension(extension_properties, ext_name, true, extensions), false, "Failed to add essential device extension.");
	}
	for (const char *const &ext_name : optional_device_extensions) {
		add_optional_extension(extension_properties, ext_name, true, extensions);
	}
	if (active_device_extensions.contains(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)) {
		VkPhysicalDeviceSynchronization2Features synchronization_2_features{};
		synchronization_2_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
		synchronization_2_features.synchronization2 = VK_TRUE;

		device_create_info.pNext = &synchronization_2_features;
	}

	device_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	device_create_info.ppEnabledExtensionNames = extensions.data();

	ERR_FAIL_VK_RET(vkCreateDevice(physical_device, &device_create_info, nullptr, &device), false, "Failed to create logical device.");
	ERR_FAIL_COND_RET(device == VK_NULL_HANDLE, false, "VkDevice is null, but create did not error.");
	DEBUG_NAME(device, VK_OBJECT_TYPE_DEVICE, "Device");

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
bool VulkanEngine::create_pipeline_cache() {
	VkPipelineCacheCreateInfo pipeline_cache_create_info{};
	pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipeline_cache_create_info.initialDataSize = 0;
	ERR_FAIL_VK_RET(vkCreatePipelineCache(device, &pipeline_cache_create_info, nullptr, &pipeline_cache), false, "Failed to create pipeline cache.");
	ERR_FAIL_COND_RET(pipeline_cache == VK_NULL_HANDLE, false, "VkPipelineCache is null, but create did not error.");
	DEBUG_NAME(pipeline_cache, VK_OBJECT_TYPE_PIPELINE_CACHE, "PipelineCache");
	return true;
}
bool VulkanEngine::create_render_pass() {
	if (active_device_extensions.contains(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME)) {
		VkAttachmentDescription2 color_attachment{};
		color_attachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
		color_attachment.format = window.swapchain_format.format;
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
		ERR_FAIL_VK_RET(vkCreateRenderPass2(device, &render_pass_create_info, nullptr, &window.render_pass), false, "Failed to create render pass.");
		DEBUG_NAME(window.render_pass, VK_OBJECT_TYPE_RENDER_PASS, "Window/RenderPass2");
	} else {
		VkAttachmentDescription color_attachment{};
		color_attachment.format = window.swapchain_format.format;
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
		ERR_FAIL_VK_RET(vkCreateRenderPass(device, &render_pass_create_info, nullptr, &window.render_pass), false, "Failed to create render pass.");
		DEBUG_NAME(window.render_pass, VK_OBJECT_TYPE_RENDER_PASS, "Window/RenderPass");
	}
	ERR_FAIL_COND_RET(window.render_pass == VK_NULL_HANDLE, false, "VkRenderPass is null, but create did not error.");
	return true;
}
bool VulkanEngine::setup_swapchain() {
	VkBool32 res;
	ERR_FAIL_VK_RET(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family, surface, &res), false, "Physical device doesn't support WSI.");
	const std::vector<VkFormat> request_surface_info_formats{ VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
	const VkColorSpaceKHR request_surface_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

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
			window.swapchain_format.format = request_surface_info_formats[0];
			window.swapchain_format.colorSpace = request_surface_color_space;
		} else {
			window.swapchain_format = surface_formats[0].surfaceFormat;
		}
	} else {
		for (uint32_t i = 0; i < request_surface_info_formats.size(); i++) {
			for (uint32_t j = 0; j < surface_formats_count; j++) {
				if (surface_formats[j].surfaceFormat.format == request_surface_info_formats[i] && surface_formats[j].surfaceFormat.colorSpace == request_surface_color_space) {
					window.swapchain_format = surface_formats[j].surfaceFormat;
					goto endloop;
				}
			}
		}
		window.swapchain_format = surface_formats[0].surfaceFormat;
	}
endloop:
	window.swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
	return true;
}
bool VulkanEngine::create_swapchain(int p_width, int p_height) {
	VkSwapchainKHR old_swapchain = window.swapchain;
	window.swapchain = VK_NULL_HANDLE;
	ERR_FAIL_VK_RET(vkDeviceWaitIdle(device), false, "Failed to wait for idle device.");
	if (queue != VK_NULL_HANDLE) {
		ERR_FAIL_VK_RET(vkQueueWaitIdle(queue), false, "Failed to wait for idle queue.");
	}
	window.min_image_count = 2;

	if (old_swapchain != VK_NULL_HANDLE) {
		window.cleanup(device, true);
	}

	// TODO: Check for support of 2 and remove from essential extensions
	VkSurfaceCapabilities2KHR capabilities{};
	capabilities.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;
	ERR_FAIL_VK_RET(vkGetPhysicalDeviceSurfaceCapabilities2KHR(physical_device, &surface_info, &capabilities), false, "Failed to get physical device surface capabilities.");
	VkSurfaceCapabilitiesKHR surface_capabilities = capabilities.surfaceCapabilities;

	VkSwapchainCreateInfoKHR swapchain_create_info{};
	swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.surface = surface;
	if (window.min_image_count < surface_capabilities.minImageCount) {
		swapchain_create_info.minImageCount = surface_capabilities.minImageCount;
	} else if (surface_capabilities.maxImageCount != 0 && window.min_image_count > surface_capabilities.maxImageCount) {
		swapchain_create_info.minImageCount = surface_capabilities.maxImageCount;
	} else {
		swapchain_create_info.minImageCount = window.min_image_count;
	}
	swapchain_create_info.imageFormat = window.swapchain_format.format;
	swapchain_create_info.imageColorSpace = window.swapchain_format.colorSpace;
	swapchain_create_info.imageArrayLayers = 1;
	swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_create_info.preTransform = (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : surface_capabilities.currentTransform;
	if (surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
		swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	} else if (surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
		swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	} else {
		ERR_FAIL_MSG_RET(false, "No supported composite alpha mode found.");
	}
	swapchain_create_info.presentMode = window.swapchain_present_mode;
	swapchain_create_info.clipped = VK_TRUE;
	swapchain_create_info.oldSwapchain = old_swapchain;
	if (surface_capabilities.currentExtent.width == 0xffffffff) {
		swapchain_create_info.imageExtent.width = p_width;
		swapchain_create_info.imageExtent.height = p_height;
	} else {
		swapchain_create_info.imageExtent.width = surface_capabilities.currentExtent.width;
		swapchain_create_info.imageExtent.height = surface_capabilities.currentExtent.height;
	}
	window.width = static_cast<int>(swapchain_create_info.imageExtent.width);
	window.height = static_cast<int>(swapchain_create_info.imageExtent.height);
	ERR_FAIL_VK_RET(vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &window.swapchain), false, "Failed to create swapchain.");
	DEBUG_NAME(window.swapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "Window/Swapchain");

	ERR_FAIL_VK_RET(vkGetSwapchainImagesKHR(device, window.swapchain, &window.frames_count, nullptr), false, "Failed to get swapchain images.");
	std::vector<VkImage> swapchain_images(window.frames_count);
	ERR_FAIL_VK_RET(vkGetSwapchainImagesKHR(device, window.swapchain, &window.frames_count, swapchain_images.data()), false, "Failed to get swapchain images.");

	window.semaphores_count = window.frames_count + 1;
	window.frames.resize(window.frames_count);
	window.semaphores.resize(window.semaphores_count);
	for (uint32_t i = 0; i < window.frames_count; i++) {
		window.frames[i].image = swapchain_images[i];
	}
	if (old_swapchain) {
		vkDestroySwapchainKHR(device, old_swapchain, nullptr);
	}

	// TODO: Disable for dynamic rendering
	create_render_pass();

	VkImageSubresourceRange image_range{};
	image_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_range.baseMipLevel = 0;
	image_range.levelCount = 1;
	image_range.baseArrayLayer = 0;
	image_range.layerCount = 1;

	VkImageViewCreateInfo image_view_create_info{};
	image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.format = window.swapchain_format.format;
	image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
	image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
	image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
	image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
	image_view_create_info.subresourceRange = image_range;
	for (uint32_t i = 0; i < window.frames_count; i++) {
		image_view_create_info.image = window.frames[i].image;
		ERR_FAIL_VK_RET(vkCreateImageView(device, &image_view_create_info, nullptr, &window.frames[i].image_view), false, "Failed to create image view.");
		DEBUG_NAME(window.frames[i].image_view, VK_OBJECT_TYPE_IMAGE_VIEW, "Window/Frames/" + itos(i) + "/ImageView");
	}

	// TODO: Disable for dynamic rendering
	VkFramebufferCreateInfo framebuffer_create_info{};
	framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_create_info.renderPass = window.render_pass;
	framebuffer_create_info.attachmentCount = 1;
	framebuffer_create_info.width = swapchain_create_info.imageExtent.width;
	framebuffer_create_info.height = swapchain_create_info.imageExtent.height;
	framebuffer_create_info.layers = 1;
	for (uint32_t i = 0; i < window.frames_count; i++) {
		framebuffer_create_info.pAttachments = &window.frames[i].image_view;
		ERR_FAIL_VK_RET(vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &window.frames[i].frame_buffer), false, "Failed to create frame buffer.");
		DEBUG_NAME(window.frames[i].frame_buffer, VK_OBJECT_TYPE_FRAMEBUFFER, "Window/Frames/" + itos(i) + "/FrameBuffer");
	}
	return true;
}
bool VulkanEngine::create_command_buffers() {
	for (uint32_t i = 0; i < window.frames_count; i++) {
		Frame *frame = &window.frames[i];

		VkCommandPoolCreateInfo command_pool_create_info{};
		command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		command_pool_create_info.flags = 0;
		command_pool_create_info.queueFamilyIndex = queue_family;
		ERR_FAIL_VK_RET(vkCreateCommandPool(device, &command_pool_create_info, nullptr, &frame->command_pool), false, "Failed to create command pool.");
		DEBUG_NAME(window.frames[i].command_pool, VK_OBJECT_TYPE_COMMAND_POOL, "Window/Frames/" + itos(i) + "/CommandPool");

		VkCommandBufferAllocateInfo command_buffer_allocate_info{};
		command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command_buffer_allocate_info.commandPool = frame->command_pool;
		command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		command_buffer_allocate_info.commandBufferCount = 1;
		ERR_FAIL_VK_RET(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &frame->command_buffer), false, "Failed to allocate command buffer.");
		DEBUG_NAME(window.frames[i].command_buffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "Window/Frames/" + itos(i) + "/CommandBuffer");

		VkFenceCreateInfo fence_create_info{};
		fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		ERR_FAIL_VK_RET(vkCreateFence(device, &fence_create_info, nullptr, &frame->fence), false, "Failed to create fence.");
		DEBUG_NAME(window.frames[i].fence, VK_OBJECT_TYPE_FENCE, "Window/Frames/" + itos(i) + "/Fence");
	}

	for (uint32_t i = 0; i < window.semaphores_count; i++) {
		FrameSemaphores *semaphores = &window.semaphores[i];

		VkSemaphoreCreateInfo semaphore_create_info{};
		semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		ERR_FAIL_VK_RET(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &semaphores->image_acquired), false, "Failed to create image acquired semaphore.");
		ERR_FAIL_VK_RET(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &semaphores->render_complete), false, "Failed to create render complete semaphore.");
		DEBUG_NAME(window.semaphores[i].image_acquired, VK_OBJECT_TYPE_SEMAPHORE, "Window/Semaphores/" + itos(i) + "/ImageAcquired");
		DEBUG_NAME(window.semaphores[i].render_complete, VK_OBJECT_TYPE_SEMAPHORE, "Window/Semaphores/" + itos(i) + "/RenderComplete");
	}
	window.frame_index = 0;
	window.semaphore_index = 0;
	window.frame_acquired = false;
	window.swapchain_rebuild = false;
	return true;
}
bool VulkanEngine::create_window(int p_width, int p_height) {
	DEBUG_BEGIN_QUEUE_REGION("create_window", 0.349f, 0.835f, 0.878f, 1.f);
	ERR_FAIL_COND_RET(!create_swapchain(p_width, p_height), false, "Failed to create swapchain.");
	ERR_FAIL_COND_RET(!create_command_buffers(), false, "Failed to create frame synchronization resources.");
	ERR_FAIL_COND_RET(window.min_image_count < 2, false, "Incorrect min_image_count currently " + itos(window.min_image_count) + " should be at least 2.");
	DEBUG_END_QUEUE_REGION();
	return true;
}

#ifdef DEBUG
#ifndef NO_DEBUG_UTILS
void VulkanEngine::set_debug_name(uint64_t p_handle, VkObjectType p_type, const char *p_name) {
	VkDebugUtilsObjectNameInfoEXT debug_utils_object_name_info{};
	debug_utils_object_name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	debug_utils_object_name_info.objectType = p_type;
	debug_utils_object_name_info.objectHandle = p_handle;
	debug_utils_object_name_info.pObjectName = p_name;

	ERR_FAIL_VK(vkSetDebugUtilsObjectNameEXT(device, &debug_utils_object_name_info), "Failed to set object name \"" + std::string(p_name) + "\".");
}
void VulkanEngine::set_debug_name(uint64_t p_handle, VkObjectType p_type, std::string p_name) {
	VkDebugUtilsObjectNameInfoEXT debug_utils_object_name_info{};
	debug_utils_object_name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	debug_utils_object_name_info.objectType = p_type;
	debug_utils_object_name_info.objectHandle = p_handle;
	debug_utils_object_name_info.pObjectName = p_name.c_str();

	ERR_FAIL_VK(vkSetDebugUtilsObjectNameEXT(device, &debug_utils_object_name_info), "Failed to set object name \"" + p_name + "\".");
}

void VulkanEngine::begin_debug_queue_region(const char *p_name, const ImVec4 p_color) {
	VkDebugUtilsLabelEXT debug_utils_label{};
	debug_utils_label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	debug_utils_label.pLabelName = p_name;
	debug_utils_label.color[0] = p_color.x;
	debug_utils_label.color[1] = p_color.y;
	debug_utils_label.color[2] = p_color.z;
	debug_utils_label.color[3] = p_color.w;
	vkQueueBeginDebugUtilsLabelEXT(queue, &debug_utils_label);
}
void VulkanEngine::begin_debug_queue_region(std::string p_name, const ImVec4 p_color) {
	VkDebugUtilsLabelEXT debug_utils_label{};
	debug_utils_label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	debug_utils_label.pLabelName = p_name.c_str();
	debug_utils_label.color[0] = p_color.x;
	debug_utils_label.color[1] = p_color.y;
	debug_utils_label.color[2] = p_color.z;
	debug_utils_label.color[3] = p_color.w;
	vkQueueBeginDebugUtilsLabelEXT(queue, &debug_utils_label);
}
void VulkanEngine::insert_debug_queue_label(const char *p_name, const ImVec4 p_color) {
	VkDebugUtilsLabelEXT debug_utils_label{};
	debug_utils_label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	debug_utils_label.pLabelName = p_name;
	debug_utils_label.color[0] = p_color.x;
	debug_utils_label.color[1] = p_color.y;
	debug_utils_label.color[2] = p_color.z;
	debug_utils_label.color[3] = p_color.w;
	vkQueueInsertDebugUtilsLabelEXT(queue, &debug_utils_label);
}
void VulkanEngine::insert_debug_queue_label(std::string p_name, const ImVec4 p_color) {
	VkDebugUtilsLabelEXT debug_utils_label{};
	debug_utils_label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	debug_utils_label.pLabelName = p_name.c_str();
	debug_utils_label.color[0] = p_color.x;
	debug_utils_label.color[1] = p_color.y;
	debug_utils_label.color[2] = p_color.z;
	debug_utils_label.color[3] = p_color.w;
	vkQueueInsertDebugUtilsLabelEXT(queue, &debug_utils_label);
}
void VulkanEngine::end_debug_queue_region() {
	vkQueueEndDebugUtilsLabelEXT(queue);
}
#endif
#endif

bool VulkanEngine::is_layer_available(std::vector<VkLayerProperties> &p_layer_properties, const char *p_layer) {
	for (const VkLayerProperties &layer_properties : p_layer_properties) {
		if (strcmp(layer_properties.layerName, p_layer) == 0) {
			return true;
		}
	}
	return false;
}
bool VulkanEngine::add_layer(std::vector<VkLayerProperties> &p_layer_properties, const char *p_layer, std::vector<const char *> &r_layers) {
	if (likely(is_layer_available(p_layer_properties, p_layer))) {
		r_layers.push_back(p_layer);
		active_layers.insert(p_layer);
		return true;
	}
	return false;
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
