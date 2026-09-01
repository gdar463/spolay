#pragma once

/**
 * pipeline_info.hpp
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

struct PipelineInfo {
	VkRenderPass render_pass = VK_NULL_HANDLE;

	uint32_t subpass = 0;
	VkSampleCountFlagBits msaa_samples{};
	std::vector<VkDynamicState> dynamic_states;

	// VkImageUsageFlags swapchain_image_usage;

	void cleanup(VkDevice p_device) {
		if (render_pass != VK_NULL_HANDLE) {
			vkDestroyRenderPass(p_device, render_pass, nullptr);
			render_pass = VK_NULL_HANDLE;
		}
	}
};
