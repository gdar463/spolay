#pragma once

/**
 * backend_data.hpp
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

#include "graphics/vulkan/render_state.hpp"

class VulkanEngine;

struct ImGuiBackendData {
	VulkanEngine *vk = nullptr;
	RenderState *render_state = nullptr;
	VkDescriptorSetLayout descriptor_set_layout_texture = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptor_set_layout_sampler = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipeline pipeline_for_viewports = VK_NULL_HANDLE;
	VkShaderModule shader_module_vert = VK_NULL_HANDLE;
	VkShaderModule shader_module_frag = VK_NULL_HANDLE;
	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

	VkSampler sampler_linear = VK_NULL_HANDLE;
	VkSampler sampler_nearest = VK_NULL_HANDLE;
	VkDescriptorSet sampler_linear_descriptor_set = VK_NULL_HANDLE;
	VkDescriptorSet sampler_nearest_descriptor_set = VK_NULL_HANDLE;
	VkCommandPool texture_command_pool = VK_NULL_HANDLE;
	VkCommandBuffer texture_command_buffer = VK_NULL_HANDLE;

	constexpr ImGuiBackendData(VulkanEngine *p_vk) : vk(p_vk) {}

	void cleanup(VkDevice p_device) {
		if (render_state) {
			delete render_state;
			render_state = nullptr;
		}
		if (texture_command_pool != VK_NULL_HANDLE) {
			if (texture_command_buffer != VK_NULL_HANDLE) {
				vkFreeCommandBuffers(p_device, texture_command_pool, 1, &texture_command_buffer);
				texture_command_buffer = VK_NULL_HANDLE;
			}
			vkDestroyCommandPool(p_device, texture_command_pool, nullptr);
			texture_command_pool = VK_NULL_HANDLE;
		}
		if (shader_module_frag != VK_NULL_HANDLE) {
			vkDestroyShaderModule(p_device, shader_module_frag, nullptr);
			shader_module_frag = VK_NULL_HANDLE;
		}
		if (shader_module_vert != VK_NULL_HANDLE) {
			vkDestroyShaderModule(p_device, shader_module_vert, nullptr);
			shader_module_vert = VK_NULL_HANDLE;
		}
		if (pipeline_for_viewports != VK_NULL_HANDLE) {
			vkDestroyPipeline(p_device, pipeline_for_viewports, nullptr);
			pipeline_for_viewports = VK_NULL_HANDLE;
		}
		if (pipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(p_device, pipeline, nullptr);
			pipeline = VK_NULL_HANDLE;
		}
		if (pipeline_layout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(p_device, pipeline_layout, nullptr);
			pipeline_layout = VK_NULL_HANDLE;
		}
		if (descriptor_pool != VK_NULL_HANDLE) {
			if (sampler_nearest_descriptor_set != VK_NULL_HANDLE) {
				vkFreeDescriptorSets(p_device, descriptor_pool, 1, &sampler_nearest_descriptor_set);
				sampler_nearest_descriptor_set = VK_NULL_HANDLE;
			}
			if (sampler_linear_descriptor_set != VK_NULL_HANDLE) {
				vkFreeDescriptorSets(p_device, descriptor_pool, 1, &sampler_linear_descriptor_set);
				sampler_linear_descriptor_set = VK_NULL_HANDLE;
			}

			vkDestroyDescriptorPool(p_device, descriptor_pool, nullptr);
			descriptor_pool = VK_NULL_HANDLE;
		}
		if (sampler_nearest != VK_NULL_HANDLE) {
			vkDestroySampler(p_device, sampler_nearest, nullptr);
			sampler_nearest = VK_NULL_HANDLE;
		}
		if (sampler_linear != VK_NULL_HANDLE) {
			vkDestroySampler(p_device, sampler_linear, nullptr);
			sampler_linear = VK_NULL_HANDLE;
		}
		if (descriptor_set_layout_sampler != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(p_device, descriptor_set_layout_sampler, nullptr);
			descriptor_set_layout_sampler = VK_NULL_HANDLE;
		}
		if (descriptor_set_layout_texture != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(p_device, descriptor_set_layout_texture, nullptr);
			descriptor_set_layout_texture = VK_NULL_HANDLE;
		}
	}
};
