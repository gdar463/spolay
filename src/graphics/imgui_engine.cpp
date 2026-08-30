/**
 * imgui_engine.cpp
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

#include "imgui_engine.hpp"

#include <imgui_impl_sdl3.h>

ImGuiEngine::ImGuiEngine(AppState *p_state, VulkanEngine *p_vk) {
	state = p_state;
	vk = p_vk;
}
ImGuiEngine::~ImGuiEngine() {
	if (context) {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
	}
}
void ImGuiEngine::cleanup() {
	if (context) {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
	}
}
void ImGuiEngine::cleanup_window() {
	if (window.Swapchain) {
		vk->cleanup_window();
	}
}

bool ImGuiEngine::setup_window(int p_width, int p_height) {
	window.Surface = vk->surface;
	window.SurfaceFormat.format = vk->swapchain_format;
	window.SurfaceFormat.colorSpace = vk->swapchain_color_space;
	window.PresentMode = vk->swapchain_present_mode;
	create_window(p_width, p_height);
	vk->swapchain = window.Swapchain;
	return true;
}
bool ImGuiEngine::create_window(int p_width, int p_height) {
	ERR_FAIL_COND_RET(vk->min_image_count < 2, false, "Incorrect min_image_count currently " + itos(vk->min_image_count) + " should be at least 2.");
	ImGui_ImplVulkan_SetMinImageCount(vk->min_image_count);
	vk->create_window(p_width, p_height);
	window.FrameIndex = 0;
	vk->swapchain = window.Swapchain;
	vk->swapchain_rebuild = false;
	return true;
}
bool ImGuiEngine::setup() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigDpiScaleFonts = true;

	ImGui::StyleColorsDark();

	ImGuiStyle &style = ImGui::GetStyle();
	style.ScaleAllSizes(state->main_scale);

	window.RenderPass = vk->render_pass;
#pragma region "Sanity Checks"
	ERR_FAIL_NULL_RET(vk, false, "VK IS SOMEHOW NULL????");
	ERR_FAIL_COND_RET(vk->instance == VK_NULL_HANDLE, false, "VkInstance is null.");
	ERR_FAIL_COND_RET(vk->physical_device == VK_NULL_HANDLE, false, "VkPhysicalDevice is null.");
	ERR_FAIL_COND_RET(vk->device == VK_NULL_HANDLE, false, "VkDevice is null.");
	ERR_FAIL_COND_RET(vk->queue == VK_NULL_HANDLE, false, "VkQueue is null.");
	ERR_FAIL_COND_RET(vk->pipeline_cache == VK_NULL_HANDLE, false, "VkPipelineCache is null.");
	ERR_FAIL_COND_RET(vk->descriptor_pool == VK_NULL_HANDLE, false, "VkDescriptorPool is null.");
	ERR_FAIL_COND_RET(vk->render_pass == VK_NULL_HANDLE, false, "VkRenderPass is null.");
	ERR_FAIL_COND_RET(window.RenderPass == VK_NULL_HANDLE, false, "VkRenderPass in ImGui_Window is null.");
#pragma endregion "Sanity Checks"

	ImGui_ImplSDL3_InitForVulkan(state->window);
	ImGui_ImplVulkan_InitInfo init_info{};
	init_info.ApiVersion = APP_VULKAN_API_VERSION;
	init_info.Instance = vk->instance;
	init_info.PhysicalDevice = vk->physical_device;
	init_info.Device = vk->device;
	init_info.QueueFamily = vk->queue_family;
	init_info.Queue = vk->queue;
	init_info.PipelineCache = vk->pipeline_cache;
	init_info.DescriptorPool = vk->descriptor_pool;
	init_info.MinImageCount = vk->min_image_count;
	init_info.ImageCount = init_info.MinImageCount;
	init_info.PipelineInfoMain.RenderPass = window.RenderPass;
	init_info.PipelineInfoMain.Subpass = 0;
	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	ERR_FAIL_COND_RET(!ImGui_ImplVulkan_Init(&init_info), false, "Failed to initialize ImGui_ImplVulkan.");
	return true;
}

void ImGuiEngine::frame_render(ImDrawData *p_draw_data) {
	VkSemaphore image_acquired_semaphore = window.FrameSemaphores[window.SemaphoreIndex].ImageAcquiredSemaphore;
	VkSemaphore render_complete_semaphore = window.FrameSemaphores[window.SemaphoreIndex].RenderCompleteSemaphore;
	VkResult err = vkAcquireNextImageKHR(vk->device, window.Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &window.FrameIndex);
	if (unlikely(err == VK_SUBOPTIMAL_KHR)) {
		vk->swapchain_rebuild = true;
	} else if (unlikely(err == VK_ERROR_OUT_OF_DATE_KHR)) {
		vk->swapchain_rebuild = true;
		return;
	} else {
		ERR_FAIL_COND(err != VK_SUCCESS, "Failed to acquire next image, error: " + itos(err) + ".");
	}

	ImGui_ImplVulkanH_Frame *frame = &window.Frames[window.FrameIndex];

	ERR_FAIL_COND(vkWaitForFences(vk->device, 1, &frame->Fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS, "Failed to wait for fences.");
	ERR_FAIL_COND(vkResetFences(vk->device, 1, &frame->Fence) != VK_SUCCESS, "Failed to reset fence.");

	ERR_FAIL_COND(vkResetCommandPool(vk->device, frame->CommandPool, 0) != VK_SUCCESS, "Failed to reset command pool.");

	VkCommandBufferBeginInfo command_buffer_begin_info{};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	ERR_FAIL_COND(vkBeginCommandBuffer(frame->CommandBuffer, &command_buffer_begin_info) != VK_SUCCESS, "Failed to begin command buffer.");

	VkRenderPassBeginInfo render_pass_begin_info{};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.renderPass = window.RenderPass;
	render_pass_begin_info.framebuffer = frame->Framebuffer;
	render_pass_begin_info.renderArea.extent.width = window.Width;
	render_pass_begin_info.renderArea.extent.height = window.Height;
	render_pass_begin_info.clearValueCount = 1;
	render_pass_begin_info.pClearValues = &window.ClearValue;
	vkCmdBeginRenderPass(frame->CommandBuffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	ImGui_ImplVulkan_RenderDrawData(p_draw_data, frame->CommandBuffer);

	vkCmdEndRenderPass(frame->CommandBuffer);

	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &image_acquired_semaphore;
	submit_info.pWaitDstStageMask = &wait_stage;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &frame->CommandBuffer;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &render_complete_semaphore;
	ERR_FAIL_COND(vkEndCommandBuffer(frame->CommandBuffer) != VK_SUCCESS, "Failed to end command buffer.");
	ERR_FAIL_COND(vkQueueSubmit(vk->queue, 1, &submit_info, frame->Fence) != VK_SUCCESS, "Failed to submit to queue.");
}
void ImGuiEngine::frame_present() {
	if (vk->swapchain_rebuild) {
		return;
	}

	VkSemaphore render_complete_semaphore = window.FrameSemaphores[window.SemaphoreIndex].RenderCompleteSemaphore;
	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &render_complete_semaphore;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &window.Swapchain;
	present_info.pImageIndices = &window.FrameIndex;
	VkResult err = vkQueuePresentKHR(vk->queue, &present_info);
	if (unlikely(err == VK_SUBOPTIMAL_KHR)) {
		vk->swapchain_rebuild = true;
	} else if (unlikely(err == VK_ERROR_OUT_OF_DATE_KHR)) {
		vk->swapchain_rebuild = true;
		return;
	} else {
		ERR_FAIL_COND(err != VK_SUCCESS, "Failed to present to queue, error: " + itos(err) + ".");
	}

	window.SemaphoreIndex = (window.SemaphoreIndex + 1) % window.SemaphoreCount;
}
