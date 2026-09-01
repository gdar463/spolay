#pragma once

/**
 * texture.hpp
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

struct Texture {
	VmaAllocation allocation = VK_NULL_HANDLE;
	VkImage image = VK_NULL_HANDLE;
	VkImageView image_view = VK_NULL_HANDLE;
	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;

	void cleanup(VkDevice p_device, VmaAllocator p_allocator, VkDescriptorPool p_descriptor_pool) {
		if (descriptor_set != VK_NULL_HANDLE) {
			vkFreeDescriptorSets(p_device, p_descriptor_pool, 1, &descriptor_set);
			descriptor_set = VK_NULL_HANDLE;
		}
		if (image_view != VK_NULL_HANDLE) {
			vkDestroyImageView(p_device, image_view, nullptr);
			image_view = VK_NULL_HANDLE;
		}
		if (image != VK_NULL_HANDLE) {
			vmaDestroyImage(p_allocator, image, allocation);
			image = VK_NULL_HANDLE;
		}
	}
};
