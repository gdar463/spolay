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

#include <stb_image.h>

#include "graphics/imgui/shaders.hpp"
#include "graphics/imgui/viewport_data.hpp"

ImGuiEngine::ImGuiEngine(AppState *p_state, VulkanEngine *p_vk) {
	state = p_state;
	vk = p_vk;
}
ImGuiEngine::~ImGuiEngine() {}
void ImGuiEngine::cleanup() {
	ImGuiBackendData *bd = get_backend_data();
	if (!bd) {
		return;
	}
	ImGuiIO &io = ImGui::GetIO();
	ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();

	for (int i = 0; i < platform_io.Viewports.Size; i++) {
		ImguiViewportData *vd = (ImguiViewportData *)platform_io.Viewports[i]->RendererUserData;
		if (vd) {
			vd->cleanup(vk->instance, vk->device, vk->allocator);
			delete vd;
			platform_io.Viewports[i]->RendererUserData = nullptr;
		}
	}

	for (ImTextureData *texture : platform_io.Textures) {
		if (texture->Status != ImTextureStatus_Destroyed && texture->RefCount == 1) {
			Texture *bd_texture = (Texture *)texture->BackendUserData;
			if (bd_texture) {
				bd_texture->cleanup(vk->device, vk->allocator, bd->descriptor_pool);
				delete bd_texture;
				texture->SetStatus(ImTextureStatus_Destroyed);
			}
		}
	}
	bd->cleanup(vk->device);
	delete bd;
	io.BackendRendererName = nullptr;
	io.BackendRendererUserData = nullptr;
}

bool ImGuiEngine::setup(Window *p_window) {
	window = p_window;
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGuiStyle &style = ImGui::GetStyle();
	style.ScaleAllSizes(state->main_scale);
	style.FontSizeBase = 18.f;
	style.FontScaleDpi = 2.f;

	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigDpiScaleFonts = true;

	ERR_FAIL_COND_RET(io.BackendRendererUserData, false, "Already initialized backend renderer.");
	ImGuiBackendData *bd = new ImGuiBackendData(vk);
	ERR_FAIL_NULL_RET(bd, false, "BackendData intialiazed to null.");
	io.BackendRendererUserData = (void *)bd;
	bd = nullptr;
	ERR_FAIL_NULL_RET(io.BackendRendererUserData, false, "BackendData failed to get assigned to io.");
	ERR_FAIL_NULL_RET(get_backend_data(), false, "BackendData failed to get assigned to io.");
	io.BackendRendererName = "imgui_impl_vulkan__spolay";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

	ImGuiViewport *main_viewport = ImGui::GetMainViewport();
	main_viewport->RendererUserData = new ImguiViewportData();
	main_viewport = nullptr;

	ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
	platform_io.DrawCallback_ResetRenderState = draw_callback__reset_render_state;
	platform_io.DrawCallback_SetSamplerLinear = draw_callback__set_sampler_linear;
	platform_io.DrawCallback_SetSamplerNearest = draw_callback__set_sampler_nearest;
	platform_io.Renderer_CreateWindow = create_window;
	platform_io.Renderer_DestroyWindow = destroy_window;
	platform_io.Renderer_SetWindowSize = set_window_size;
	platform_io.Renderer_RenderWindow = render_window;
	platform_io.Renderer_SwapBuffers = swap_buffers;

	ERR_FAIL_COND_RET(!setup_objects(), false, "Failed to create device objects.");
	return true;
}
bool ImGuiEngine::setup_objects() {
	ImGuiBackendData *bd = get_backend_data();
	ERR_FAIL_NULL_RET(bd, false, "BackendData is null.");

	{
		VkDescriptorSetLayoutBinding binding{};
		binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		binding.descriptorCount = 1;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		VkDescriptorSetLayoutCreateInfo set_layout_create_info{};
		set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_create_info.bindingCount = 1;
		set_layout_create_info.pBindings = &binding;
		ERR_FAIL_VK_RET(vkCreateDescriptorSetLayout(vk->device, &set_layout_create_info, nullptr, &bd->descriptor_set_layout_texture), false, "Failed to create descriptor set layout texture.");
		DEBUG_NAME_VK(bd->descriptor_set_layout_texture, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, "BackendData/DescriptorSetLayout Texture");
	}
	{
		VkDescriptorSetLayoutBinding binding{};
		binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		binding.descriptorCount = 1;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		VkDescriptorSetLayoutCreateInfo set_layout_create_info{};
		set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_create_info.bindingCount = 1;
		set_layout_create_info.pBindings = &binding;
		ERR_FAIL_VK_RET(vkCreateDescriptorSetLayout(vk->device, &set_layout_create_info, nullptr, &bd->descriptor_set_layout_sampler), false, "Failed to create descriptor set layout sampler.");
		DEBUG_NAME_VK(bd->descriptor_set_layout_sampler, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, "BackendData/DescriptorSetLayout Sampler");
	}

	{
		VkDescriptorPoolSize pool_sizes[] = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 4 * 2 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4 * 2 },
		};

		VkDescriptorPoolCreateInfo pool_create_info = {};
		pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_create_info.maxSets = 0;
		for (const VkDescriptorPoolSize &pool_size : pool_sizes) {
			pool_create_info.maxSets += pool_size.descriptorCount;
		}
		pool_create_info.poolSizeCount = std::size(pool_sizes);
		pool_create_info.pPoolSizes = pool_sizes;

		ERR_FAIL_VK_RET(vkCreateDescriptorPool(vk->device, &pool_create_info, nullptr, &bd->descriptor_pool), false, "Failed to create descriptor pool.");
		DEBUG_NAME_VK(bd->descriptor_pool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, "BackendData/DescriptorPool");
	}

	{
		VkSamplerCreateInfo sampler_create_info{};
		sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler_create_info.magFilter = VK_FILTER_LINEAR;
		sampler_create_info.minFilter = VK_FILTER_LINEAR;
		sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_create_info.minLod = -1000;
		sampler_create_info.maxLod = 1000;
		sampler_create_info.maxAnisotropy = 1.f;
		ERR_FAIL_VK_RET(vkCreateSampler(vk->device, &sampler_create_info, nullptr, &bd->sampler_linear), false, "Failed to create linear sampler.");
		DEBUG_NAME_VK(bd->sampler_linear, VK_OBJECT_TYPE_SAMPLER, "BackendData/Sampler Linear");

		sampler_create_info.magFilter = VK_FILTER_NEAREST;
		sampler_create_info.minFilter = VK_FILTER_NEAREST;
		sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		ERR_FAIL_VK_RET(vkCreateSampler(vk->device, &sampler_create_info, nullptr, &bd->sampler_nearest), false, "Failed to create nearest sampler.");
		DEBUG_NAME_VK(bd->sampler_nearest, VK_OBJECT_TYPE_SAMPLER, "BackendData/Sampler Nearest");
	}

	{
		VkDescriptorSetAllocateInfo descriptor_set_allocate_info{};
		descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptor_set_allocate_info.descriptorPool = bd->descriptor_pool;
		descriptor_set_allocate_info.descriptorSetCount = 1;
		descriptor_set_allocate_info.pSetLayouts = &bd->descriptor_set_layout_sampler;
		ERR_FAIL_VK_RET(vkAllocateDescriptorSets(vk->device, &descriptor_set_allocate_info, &bd->sampler_linear_descriptor_set), false, "Failed to allocate linear descriptor set.");
		ERR_FAIL_VK_RET(vkAllocateDescriptorSets(vk->device, &descriptor_set_allocate_info, &bd->sampler_nearest_descriptor_set), false, "Failed to allocate nearest descriptor set.");
		DEBUG_NAME_VK(bd->sampler_linear_descriptor_set, VK_OBJECT_TYPE_DESCRIPTOR_SET, "BackendData/Sampler Linear/DescriptorSet");
		DEBUG_NAME_VK(bd->sampler_nearest_descriptor_set, VK_OBJECT_TYPE_DESCRIPTOR_SET, "BackendData/Sampler Nearest/DescriptorSet");

		VkDescriptorImageInfo desc_image{};
		desc_image.sampler = bd->sampler_linear;
		VkWriteDescriptorSet write_descriptor_set{};
		write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_descriptor_set.descriptorCount = 1;
		write_descriptor_set.dstSet = bd->sampler_linear_descriptor_set;
		write_descriptor_set.dstBinding = 0;
		write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		write_descriptor_set.pImageInfo = &desc_image;
		vkUpdateDescriptorSets(vk->device, 1, &write_descriptor_set, 0, nullptr);

		desc_image.sampler = bd->sampler_nearest;
		write_descriptor_set.dstSet = bd->sampler_nearest_descriptor_set;
		vkUpdateDescriptorSets(vk->device, 1, &write_descriptor_set, 0, nullptr);
	}

	{
		VkPushConstantRange push_constant_range{};
		push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		push_constant_range.offset = 0;
		push_constant_range.size = sizeof(float) * 4;
		VkDescriptorSetLayout set_layout[2] = { bd->descriptor_set_layout_texture, bd->descriptor_set_layout_sampler };
		VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = 2;
		pipeline_layout_create_info.pSetLayouts = set_layout;
		pipeline_layout_create_info.pushConstantRangeCount = 1;
		pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;
		ERR_FAIL_VK_RET(vkCreatePipelineLayout(vk->device, &pipeline_layout_create_info, nullptr, &bd->pipeline_layout), false, "Failed to create pipeline layout.");
		DEBUG_NAME_VK(bd->pipeline_layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "BackendData/Pipeline Layout");
	}

	{
		VkShaderModuleCreateInfo shader_vert_create_info{};
		shader_vert_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_vert_create_info.codeSize = sizeof(__glsl_shader_vert_spv);
		shader_vert_create_info.pCode = __glsl_shader_vert_spv;
		ERR_FAIL_VK_RET(vkCreateShaderModule(vk->device, &shader_vert_create_info, nullptr, &bd->shader_module_vert), false, "Failed to create vertex shader module.");
		DEBUG_NAME_VK(bd->shader_module_vert, VK_OBJECT_TYPE_SHADER_MODULE, "BackendData/ShaderModule Vert");

		VkShaderModuleCreateInfo shader_frag_create_info{};
		shader_frag_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_frag_create_info.codeSize = sizeof(__glsl_shader_frag_spv);
		shader_frag_create_info.pCode = __glsl_shader_frag_spv;
		ERR_FAIL_VK_RET(vkCreateShaderModule(vk->device, &shader_frag_create_info, nullptr, &bd->shader_module_frag), false, "Failed to create fragment shader module.");
		DEBUG_NAME_VK(bd->shader_module_frag, VK_OBJECT_TYPE_SHADER_MODULE, "BackendData/ShaderModule Frag");
	}

	vk->pipeline_info_main.render_pass = window->render_pass;
	bd->pipeline = create_pipeline(&vk->pipeline_info_main);
	ERR_FAIL_COND_RET(bd->pipeline == VK_NULL_HANDLE, false, "Failed to create main pipeline.");

	{
		VkCommandPoolCreateInfo command_pool_create_info{};
		command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		command_pool_create_info.flags = 0;
		command_pool_create_info.queueFamilyIndex = vk->queue_family;
		ERR_FAIL_VK_RET(vkCreateCommandPool(vk->device, &command_pool_create_info, nullptr, &bd->texture_command_pool), false, "Failed to create texture command pool.");
		DEBUG_NAME_VK(bd->texture_command_pool, VK_OBJECT_TYPE_COMMAND_POOL, "BackendData/CommandPool Textures");

		VkCommandBufferAllocateInfo command_buffer_allocate_info{};
		command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command_buffer_allocate_info.commandPool = bd->texture_command_pool;
		command_buffer_allocate_info.commandBufferCount = 1;
		ERR_FAIL_VK_RET(vkAllocateCommandBuffers(vk->device, &command_buffer_allocate_info, &bd->texture_command_buffer), false, "Failed to allocate texture command buffer.");
		DEBUG_NAME_VK(bd->texture_command_buffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "BackendData/CommandBuffer Textures");
	}
	return true;
}
VkPipeline ImGuiEngine::create_pipeline(PipelineInfo *p_info) {
	ImGuiBackendData *bd = get_backend_data();
	ERR_FAIL_NULL_RET(bd, VK_NULL_HANDLE, "BackendData is null.");
	VulkanEngine *vk = bd->vk;

	VkPipelineShaderStageCreateInfo stage_create_infos[2]{};
	stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stage_create_infos[0].module = bd->shader_module_vert;
	stage_create_infos[0].pName = "main";
	stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stage_create_infos[1].module = bd->shader_module_frag;
	stage_create_infos[1].pName = "main";

	VkVertexInputBindingDescription binding_description{};
	binding_description.stride = sizeof(ImDrawVert);
	binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attribute_descriptions[3]{};
	attribute_descriptions[0].location = 0;
	attribute_descriptions[0].binding = binding_description.binding;
	attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attribute_descriptions[0].offset = offsetof(ImDrawVert, pos);
	attribute_descriptions[1].location = 1;
	attribute_descriptions[1].binding = binding_description.binding;
	attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	attribute_descriptions[1].offset = offsetof(ImDrawVert, uv);
	attribute_descriptions[2].location = 2;
	attribute_descriptions[2].binding = binding_description.binding;
	attribute_descriptions[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	attribute_descriptions[2].offset = offsetof(ImDrawVert, col);

	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
	vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
	vertex_input_state_create_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_state_create_info.vertexAttributeDescriptionCount = 3;
	vertex_input_state_create_info.pVertexAttributeDescriptions = attribute_descriptions;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
	input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo viewport_state_create_info{};
	viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
	rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
	rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization_state_create_info.lineWidth = 1.f;

	VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
	multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_create_info.rasterizationSamples = (p_info->msaa_samples != 0) ? p_info->msaa_samples : VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState color_blend_attachment_state{};
	color_blend_attachment_state.blendEnable = VK_TRUE;
	color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{};
	depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	VkPipelineColorBlendStateCreateInfo color_blend_state_create_info{};
	color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state_create_info.attachmentCount = 1;
	color_blend_state_create_info.pAttachments = &color_blend_attachment_state;

	std::vector<VkDynamicState> dynamic_states = p_info->dynamic_states;
	dynamic_states.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamic_states.push_back(VK_DYNAMIC_STATE_SCISSOR);
	VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.dynamicStateCount = dynamic_states.size();
	dynamic_state_create_info.pDynamicStates = dynamic_states.data();

	VkGraphicsPipelineCreateInfo pipeline_create_info{};
	pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.stageCount = 2;
	pipeline_create_info.pStages = stage_create_infos;
	pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
	pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
	pipeline_create_info.pViewportState = &viewport_state_create_info;
	pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
	pipeline_create_info.pMultisampleState = &multisample_state_create_info;
	pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
	pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
	pipeline_create_info.pDynamicState = &dynamic_state_create_info;
	pipeline_create_info.layout = bd->pipeline_layout;
	pipeline_create_info.renderPass = p_info->render_pass;
	pipeline_create_info.subpass = p_info->subpass;

	VkPipeline pipeline;
	ERR_FAIL_VK_RET(vkCreateGraphicsPipelines(vk->device, vk->pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline), VK_NULL_HANDLE, "Failed to create pipeline.");
	DEBUG_NAME_VK(pipeline, VK_OBJECT_TYPE_PIPELINE, bd->pipeline == VK_NULL_HANDLE ? "BackendData/Pipeline" : "BackendData/Pipeline Viewports");
	return pipeline;
}

bool ImGuiEngine::new_frame() {
	ERR_FAIL_COND_RET(!acquire_next_image(window), false, "Failed to acquire next image.");
	return true;
}

bool ImGuiEngine::acquire_next_image(Window *p_window) {
	ImGuiBackendData *bd = get_backend_data();
	ERR_FAIL_NULL_RET(bd, false, "BackendData is null.");
	VulkanEngine *vk = bd->vk;
	p_window->frame_acquired = false;
	VkSemaphore image_acquired = p_window->semaphores[p_window->semaphore_index].image_acquired;
	VkResult err = vkAcquireNextImageKHR(vk->device, p_window->swapchain, UINT64_MAX, image_acquired, VK_NULL_HANDLE, &p_window->frame_index);
	if (unlikely(err == VK_SUBOPTIMAL_KHR)) {
		p_window->swapchain_rebuild = true;
		p_window->frame_acquired = true;
	} else if (unlikely(err == VK_ERROR_OUT_OF_DATE_KHR)) {
		p_window->swapchain_rebuild = true;
		return true;
	} else {
		ERR_FAIL_VK_RET(err, false, "Failed to acquire next image, error: " + itos(err) + ".");
	}
	p_window->frame_acquired = true;
	return true;
}

bool ImGuiEngine::frame_render(Window *p_window, ImDrawData *p_draw_data, int p_width, int p_height) {
	ImGuiBackendData *bd = get_backend_data();
	ERR_FAIL_NULL_RET(bd, false, "BackendData is null.");
	VulkanEngine *vk = bd->vk;
	ERR_FAIL_COND_RET(!p_window->frame_acquired, false, "Cannot render without an acquired swapchain image.");
	VkSemaphore image_acquired = p_window->semaphores[p_window->semaphore_index].image_acquired;
	VkSemaphore render_complete = p_window->semaphores[p_window->semaphore_index].render_complete;

	Frame *frame = &p_window->frames[p_window->frame_index];

	ERR_FAIL_VK_RET(vkWaitForFences(vk->device, 1, &frame->fence, VK_TRUE, UINT64_MAX), false, "Failed to wait for fences.");
	ERR_FAIL_VK_RET(vkResetFences(vk->device, 1, &frame->fence), false, "Failed to reset fence.");

	ERR_FAIL_VK_RET(vkResetCommandPool(vk->device, frame->command_pool, 0), false, "Failed to reset command pool.");

	VkCommandBufferBeginInfo command_buffer_begin_info{};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	ERR_FAIL_VK_RET(vkBeginCommandBuffer(frame->command_buffer, &command_buffer_begin_info), false, "Failed to begin command buffer.");

	VkRenderPassBeginInfo render_pass_begin_info{};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.renderPass = p_window->render_pass;
	render_pass_begin_info.framebuffer = frame->frame_buffer;
	render_pass_begin_info.renderArea.extent.width = p_width;
	render_pass_begin_info.renderArea.extent.height = p_height;
	render_pass_begin_info.clearValueCount = 1;
	render_pass_begin_info.pClearValues = &p_window->clear_value;
	vkCmdBeginRenderPass(frame->command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	ERR_FAIL_COND_RET(!render_draw_data(p_window, p_draw_data, frame->command_buffer, p_width, p_height), false, "Failed to render draw data.");

	vkCmdEndRenderPass(frame->command_buffer);
	ERR_FAIL_VK_RET(vkEndCommandBuffer(frame->command_buffer), false, "Failed to end command buffer.");

	if (vk->active_device_extensions.contains(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)) {
		VkCommandBufferSubmitInfo command_buffer_submit_info{};
		command_buffer_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		command_buffer_submit_info.commandBuffer = frame->command_buffer;

		VkPipelineStageFlags2 wait_stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSemaphoreSubmitInfo wait_semaphore_submit_info{};
		wait_semaphore_submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		wait_semaphore_submit_info.semaphore = image_acquired;
		wait_semaphore_submit_info.stageMask = wait_stage;

		VkSemaphoreSubmitInfo signal_semaphore_submit_info{};
		signal_semaphore_submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signal_semaphore_submit_info.semaphore = render_complete;
		signal_semaphore_submit_info.stageMask = wait_stage;

		VkSubmitInfo2 submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submit_info.commandBufferInfoCount = 1;
		submit_info.pCommandBufferInfos = &command_buffer_submit_info;
		submit_info.waitSemaphoreInfoCount = 1;
		submit_info.pWaitSemaphoreInfos = &wait_semaphore_submit_info;
		submit_info.signalSemaphoreInfoCount = 1;
		submit_info.pSignalSemaphoreInfos = &signal_semaphore_submit_info;

		ERR_FAIL_VK_RET(vkQueueSubmit2(vk->queue, 1, &submit_info, frame->fence), false, "Failed to submit to queue.");
	} else {
		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &frame->command_buffer;
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &image_acquired;
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &render_complete;
		submit_info.pWaitDstStageMask = &wait_stage;
		ERR_FAIL_VK_RET(vkQueueSubmit(vk->queue, 1, &submit_info, frame->fence), false, "Failed to submit to queue.");
	}

	return true;
}
bool ImGuiEngine::render_draw_data(Window *p_window, ImDrawData *p_draw_data, VkCommandBuffer p_command_buffer, int p_width, int p_height) {
	if (p_draw_data->Textures != nullptr) {
		for (ImTextureData *texture : *p_draw_data->Textures) {
			if (texture->Status != ImTextureStatus_OK) {
				ERR_FAIL_COND_RET(!update_texture(p_window, texture), false, "Failed to update texture.");
			}
		}
	}

	ImGuiBackendData *bd = get_backend_data();
	ERR_FAIL_NULL_RET(bd, false, "BackendData is null.");
	VulkanEngine *vk = bd->vk;
	ImguiViewportData *vd = (ImguiViewportData *)p_draw_data->OwnerViewport->RendererUserData;
	ERR_FAIL_NULL_RET(vd, false, "ViewportData is null.");

	WindowRenderBuffers *wrb = &vd->render_buffers;
	if (wrb->buffers.size() == 0) {
		wrb->index = 0;
		wrb->count = p_window->frames_count;
		wrb->buffers.resize(wrb->count);
	}
	wrb->index = (wrb->index + 1) % wrb->count;

	RenderBuffers *rb = &wrb->buffers[wrb->index];
	if (p_draw_data->TotalVtxCount > 0) {
		VkDeviceSize vertex_size = p_draw_data->TotalVtxCount * sizeof(ImDrawVert);
		VkDeviceSize index_size = p_draw_data->TotalIdxCount * sizeof(ImDrawIdx);
		if (rb->vertex_buffer == VK_NULL_HANDLE || rb->vertex_buffer_size < vertex_size) {
			if (rb->vertex_buffer != VK_NULL_HANDLE) {
				vmaDestroyBuffer(vk->allocator, rb->vertex_buffer, rb->vertex_allocation);
			}

			VkBufferCreateInfo buffer_create_info{};
			buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buffer_create_info.size = vertex_size;
			buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo allocation_create_info{};
			allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
			allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
			ERR_FAIL_VK_RET(vmaCreateBuffer(vk->allocator, &buffer_create_info, &allocation_create_info, &rb->vertex_buffer, &rb->vertex_allocation, nullptr), false, "Failed to create vertex buffer");
			DEBUG_NAME_VK(rb->vertex_buffer, VK_OBJECT_TYPE_BUFFER, "Viewport/RenderBuffers/" + itos(wrb->index) + "/Buffer Vertex");
			DEBUG_NAME_VMA(vk->allocator, rb->vertex_allocation, "Viewport/RenderBuffers/" + itos(wrb->index) + "/Allocation Vertex");
			rb->vertex_buffer_size = vertex_size;
		}
		if (rb->index_buffer == VK_NULL_HANDLE || rb->index_buffer_size < index_size) {
			if (rb->index_buffer != VK_NULL_HANDLE) {
				vmaDestroyBuffer(vk->allocator, rb->index_buffer, rb->index_allocation);
			}

			VkBufferCreateInfo buffer_create_info{};
			buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buffer_create_info.size = index_size;
			buffer_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo allocation_create_info{};
			allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
			allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
			ERR_FAIL_VK_RET(vmaCreateBuffer(vk->allocator, &buffer_create_info, &allocation_create_info, &rb->index_buffer, &rb->index_allocation, nullptr), false, "Failed to create index buffer");
			DEBUG_NAME_VK(rb->index_buffer, VK_OBJECT_TYPE_BUFFER, "Viewport/RenderBuffers/" + itos(wrb->index) + "/Buffer Index");
			DEBUG_NAME_VMA(vk->allocator, rb->index_allocation, "Viewport/RenderBuffers/" + itos(wrb->index) + "/Allocation Index");
			rb->index_buffer_size = index_size;
		}

		ImDrawVert *vtx_dst = nullptr;
		ImDrawIdx *idx_dst = nullptr;
		ERR_FAIL_VK_RET(vmaMapMemory(vk->allocator, rb->vertex_allocation, (void **)(&vtx_dst)), false, "Failed to map vertex allocation.");
		ERR_FAIL_VK_RET(vmaMapMemory(vk->allocator, rb->index_allocation, (void **)(&idx_dst)), false, "Failed to map index allocation.");
		for (const ImDrawList *draw_list : p_draw_data->CmdLists) {
			memcpy(vtx_dst, draw_list->VtxBuffer.Data, draw_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idx_dst, draw_list->IdxBuffer.Data, draw_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtx_dst += draw_list->VtxBuffer.Size;
			idx_dst += draw_list->IdxBuffer.Size;
		}
		VmaAllocation allocations[2]{ rb->vertex_allocation, rb->index_allocation };
		VkDeviceSize offsets[2]{ 0, 0 };
		VkDeviceSize sizes[2]{ VK_WHOLE_SIZE, VK_WHOLE_SIZE };
		ERR_FAIL_VK_RET(vmaFlushAllocations(vk->allocator, 2, allocations, offsets, sizes), false, "Failed to flush allocations.");
		vmaUnmapMemory(vk->allocator, rb->vertex_allocation);
		vmaUnmapMemory(vk->allocator, rb->index_allocation);
	}

	ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
	RenderState render_state;
	render_state.command_buffer = p_command_buffer;
	render_state.pipeline = (p_draw_data->OwnerViewport == ImGui::GetMainViewport()) ? bd->pipeline : bd->pipeline_for_viewports;
	render_state.pipeline_layout = bd->pipeline_layout;
	platform_io.Renderer_RenderState = &render_state;
	bd->render_state = &render_state;

	setup_render_state(p_command_buffer, p_draw_data, rb, p_width, p_height);

	ImVec2 clip_off = p_draw_data->DisplayPos;
	ImVec2 clip_scale = p_draw_data->FramebufferScale;

	VkDescriptorSet last_image_view = VK_NULL_HANDLE;
	int vtx_offset = 0;
	int idx_offset = 0;
	for (const ImDrawList *draw_list : p_draw_data->CmdLists) {
		for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++) {
			const ImDrawCmd *pcmd = &draw_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback != nullptr) {
				if (pcmd->UserCallback == draw_callback__reset_render_state) {
					setup_render_state(p_command_buffer, p_draw_data, rb, p_width, p_height);
					last_image_view = VK_NULL_HANDLE;
				} else {
					pcmd->UserCallback(draw_list, pcmd);
				}
			} else {
				ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
				ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

				if (clip_min.x < 0.f) {
					clip_min.x = 0.f;
				}
				if (clip_min.y < 0.f) {
					clip_min.y = 0.f;
				}
				if (clip_max.x > (float)p_width) {
					clip_max.x = (float)p_width;
				}
				if (clip_max.y > (float)p_height) {
					clip_max.y = (float)p_height;
				}
				if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y) {
					continue;
				}

				VkRect2D scissor{};
				scissor.offset.x = (int32_t)clip_min.x;
				scissor.offset.y = (int32_t)clip_min.y;
				scissor.extent.width = (uint32_t)(clip_max.x - clip_min.x);
				scissor.extent.height = (uint32_t)(clip_max.y - clip_min.y);
				vkCmdSetScissor(p_command_buffer, 0, 1, &scissor);

				VkDescriptorSet image_view = (VkDescriptorSet)pcmd->GetTexID();
				if (image_view != last_image_view) {
					vkCmdBindDescriptorSets(p_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bd->pipeline_layout, 0, 1, &image_view, 0, nullptr);
				}
				last_image_view = image_view;

				vkCmdDrawIndexed(p_command_buffer, pcmd->ElemCount, 1, pcmd->IdxOffset + idx_offset, pcmd->VtxOffset + vtx_offset, 0);
			}
		}
		vtx_offset += draw_list->VtxBuffer.Size;
		idx_offset += draw_list->IdxBuffer.Size;
	}
	platform_io.Renderer_RenderState = nullptr;
	bd->render_state = nullptr;
	return true;
}
bool ImGuiEngine::frame_present(Window *p_window) {
	ImGuiBackendData *bd = get_backend_data();
	ERR_FAIL_NULL_RET(bd, false, "BackendData is null.");
	VulkanEngine *vk = bd->vk;
	ERR_FAIL_COND_RET(!p_window->frame_acquired, false, "Cannot present without an acquired swapchain image.");

	VkSemaphore render_complete = p_window->semaphores[p_window->semaphore_index].render_complete;
	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &render_complete;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &p_window->swapchain;
	present_info.pImageIndices = &p_window->frame_index;
	VkResult err = vkQueuePresentKHR(vk->queue, &present_info);
	if (unlikely(err == VK_SUBOPTIMAL_KHR)) {
		p_window->swapchain_rebuild = true;
	} else if (unlikely(err == VK_ERROR_OUT_OF_DATE_KHR)) {
		p_window->swapchain_rebuild = true;
	} else {
		ERR_FAIL_VK_RET(err, false, "Failed to present to queue, error: " + itos(err) + ".");
	}

	p_window->semaphore_index = (p_window->semaphore_index + 1) % p_window->semaphores_count;
	p_window->frame_acquired = false;
	return true;
}

Texture *ImGuiEngine::load_texture(const uint8_t *p_buffer, int p_size) {
	ImTextureData texture_data{};
	uint8_t *image_data = stbi_load_from_memory(p_buffer, p_size, &texture_data.Width, &texture_data.Height, nullptr, 4);
	ERR_FAIL_NULL_RET(image_data, nullptr, "Failed to load image from memory.");
	texture_data.Create(ImTextureFormat_RGBA32, texture_data.Width, texture_data.Height);

	memcpy(texture_data.Pixels, image_data, texture_data.GetSizeInBytes());
	stbi_image_free(image_data);

	ERR_FAIL_COND_RET(!update_texture(window, &texture_data), nullptr, "Failed to create new texture.");

	return (Texture *)texture_data.BackendUserData;
}

bool ImGuiEngine::update_texture(Window *p_window, ImTextureData *p_texture) {
	ImGuiBackendData *bd = get_backend_data();
	ERR_FAIL_NULL_RET(bd, false, "BackendData is null.");
	VulkanEngine *vk = bd->vk;

	if (p_texture->Status == ImTextureStatus_WantCreate) {
		ERR_FAIL_COND_RET(p_texture->TexID != ImTextureID_Invalid, false, "TextureID is not invalid while creating it.");
		ERR_FAIL_COND_RET(p_texture->Format != ImTextureFormat_RGBA32, false, "Only RGBA32 (R8G8B8A8) textures are implemented. Requested: " + itos(p_texture->Format));
		Texture *bd_texture = new Texture();

		VkImageCreateInfo image_create_info{};
		image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_create_info.imageType = VK_IMAGE_TYPE_2D;
		image_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
		image_create_info.extent.width = p_texture->Width;
		image_create_info.extent.height = p_texture->Height;
		image_create_info.extent.depth = 1;
		image_create_info.mipLevels = 1;
		image_create_info.arrayLayers = 1;
		image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VmaAllocationCreateInfo allocation_create_info{};
		allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
		ERR_FAIL_VK_RET(vmaCreateImage(vk->allocator, &image_create_info, &allocation_create_info, &bd_texture->image, &bd_texture->allocation, nullptr), false, "Failed to create image.");

		VkImageViewCreateInfo image_view_create_info{};
		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_create_info.image = bd_texture->image;
		image_view_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
		image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_create_info.subresourceRange.layerCount = 1;
		image_view_create_info.subresourceRange.levelCount = 1;
		ERR_FAIL_VK_RET(vkCreateImageView(vk->device, &image_view_create_info, nullptr, &bd_texture->image_view), false, "Failed to create image view.");

		VkDescriptorSetAllocateInfo descriptor_set_allocate_info{};
		descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptor_set_allocate_info.descriptorPool = bd->descriptor_pool;
		descriptor_set_allocate_info.descriptorSetCount = 1;
		descriptor_set_allocate_info.pSetLayouts = &bd->descriptor_set_layout_texture;
		ERR_FAIL_VK_RET(vkAllocateDescriptorSets(vk->device, &descriptor_set_allocate_info, &bd_texture->descriptor_set), false, "Failed to allocate descriptor set");

		VkDescriptorImageInfo descriptor_image_info{};
		descriptor_image_info.imageView = bd_texture->image_view;
		descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkWriteDescriptorSet write_descriptor_set{};
		write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_descriptor_set.dstSet = bd_texture->descriptor_set;
		write_descriptor_set.descriptorCount = 1;
		write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write_descriptor_set.pImageInfo = &descriptor_image_info;
		vkUpdateDescriptorSets(vk->device, 1, &write_descriptor_set, 0, nullptr);
		DEBUG_NAME_VK(bd_texture->image, VK_OBJECT_TYPE_IMAGE, "Textures/" + itos((uint64_t)bd_texture) + "/Image");
		DEBUG_NAME_VK(bd_texture->image_view, VK_OBJECT_TYPE_IMAGE_VIEW, "Textures/" + itos((uint64_t)bd_texture) + "/ImageView");
		DEBUG_NAME_VK(bd_texture->descriptor_set, VK_OBJECT_TYPE_DESCRIPTOR_SET, "Textures/" + itos((uint64_t)bd_texture) + "/DescriptorSet");
		DEBUG_NAME_VMA(vk->allocator, bd_texture->allocation, "Textures/" + itos((uint64_t)bd_texture) + "/Allocation");

		p_texture->SetTexID((ImTextureID)bd_texture->descriptor_set);
		p_texture->BackendUserData = bd_texture;
		bd_texture->width = p_texture->Width;
		bd_texture->height = p_texture->Height;
	}

	if (p_texture->Status == ImTextureStatus_WantCreate || p_texture->Status == ImTextureStatus_WantUpdates) {
		Texture *bd_texture = (Texture *)p_texture->BackendUserData;

		int upload_x = (p_texture->Status == ImTextureStatus_WantCreate) ? 0 : p_texture->UpdateRect.x;
		int upload_y = (p_texture->Status == ImTextureStatus_WantCreate) ? 0 : p_texture->UpdateRect.y;
		int upload_w = (p_texture->Status == ImTextureStatus_WantCreate) ? p_texture->Width : p_texture->UpdateRect.w;
		int upload_h = (p_texture->Status == ImTextureStatus_WantCreate) ? p_texture->Height : p_texture->UpdateRect.h;

		VmaAllocation upload_buffer_allocation;
		VkBuffer upload_buffer;
		VkDeviceSize upload_pitch = upload_w * p_texture->BytesPerPixel;
		VkDeviceSize upload_size = upload_pitch * upload_h;

		VkBufferCreateInfo buffer_create_info{};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.size = upload_size;
		buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocation_create_info{};
		allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
		allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		ERR_FAIL_VK_RET(vmaCreateBuffer(vk->allocator, &buffer_create_info, &allocation_create_info, &upload_buffer, &upload_buffer_allocation, nullptr), false, "Failed to allocate upload buffer.");
		DEBUG_NAME_VK(upload_buffer, VK_OBJECT_TYPE_BUFFER, "update_texture/upload/" + itos((uint64_t)bd_texture) + "/Buffer");
		DEBUG_NAME_VMA(vk->allocator, upload_buffer_allocation, "update_texture/upload/" + itos((uint64_t)bd_texture) + "/Allocation");

		char *map = nullptr;
		ERR_FAIL_VK_RET(vmaMapMemory(vk->allocator, upload_buffer_allocation, (void **)(&map)), false, "Failed to map upload buffer.");
		for (int y = 0; y < upload_h; y++) {
			memcpy(map + upload_pitch * y, p_texture->GetPixelsAt(upload_x, upload_y + y), (size_t)upload_pitch);
		}
		ERR_FAIL_VK_RET(vmaFlushAllocation(vk->allocator, upload_buffer_allocation, 0, upload_size), false, "Failed to flush upload buffer.");
		vmaUnmapMemory(vk->allocator, upload_buffer_allocation);

		ERR_FAIL_VK_RET(vkResetCommandPool(vk->device, bd->texture_command_pool, 0), false, "Failed to reset texture command pool.");
		VkCommandBufferBeginInfo command_buffer_begin_info{};
		command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command_buffer_begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		ERR_FAIL_VK_RET(vkBeginCommandBuffer(bd->texture_command_buffer, &command_buffer_begin_info), false, "Failed to begin command buffer.");

		if (vk->active_device_extensions.contains(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)) {
			VkPipelineStageFlags2 src_stage_mask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_HOST_BIT;
			VkPipelineStageFlags2 dst_stage_mask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

			VkBufferMemoryBarrier2 upload_barrier{};
			upload_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			upload_barrier.srcAccessMask = VK_ACCESS_2_HOST_WRITE_BIT;
			upload_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			upload_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			upload_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			upload_barrier.buffer = upload_buffer;
			upload_barrier.offset = 0;
			upload_barrier.size = upload_size;
			upload_barrier.srcStageMask = src_stage_mask;
			upload_barrier.dstStageMask = dst_stage_mask;

			VkImageMemoryBarrier2 copy_barrier{};
			copy_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			copy_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			copy_barrier.oldLayout = (p_texture->Status == ImTextureStatus_WantCreate) ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			copy_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			copy_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copy_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copy_barrier.image = bd_texture->image;
			copy_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copy_barrier.subresourceRange.levelCount = 1;
			copy_barrier.subresourceRange.layerCount = 1;
			copy_barrier.srcStageMask = src_stage_mask;
			copy_barrier.dstStageMask = dst_stage_mask;

			VkDependencyInfo dependency_info{};
			dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependency_info.bufferMemoryBarrierCount = 1;
			dependency_info.pBufferMemoryBarriers = &upload_barrier;
			dependency_info.imageMemoryBarrierCount = 1;
			dependency_info.pImageMemoryBarriers = &copy_barrier;
			vkCmdPipelineBarrier2(bd->texture_command_buffer, &dependency_info);
		} else {
			VkBufferMemoryBarrier upload_barrier{};
			upload_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			upload_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			upload_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			upload_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			upload_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			upload_barrier.buffer = upload_buffer;
			upload_barrier.offset = 0;
			upload_barrier.size = upload_size;

			VkImageMemoryBarrier copy_barrier{};
			copy_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			copy_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			copy_barrier.oldLayout = (p_texture->Status == ImTextureStatus_WantCreate) ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			copy_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			copy_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copy_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copy_barrier.image = bd_texture->image;
			copy_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copy_barrier.subresourceRange.levelCount = 1;
			copy_barrier.subresourceRange.layerCount = 1;
			vkCmdPipelineBarrier(bd->texture_command_buffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &upload_barrier, 1, &copy_barrier);
		}

		if (vk->active_device_extensions.contains(VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME)) {
			VkBufferImageCopy2 buffer_image_copy{};
			buffer_image_copy.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
			buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			buffer_image_copy.imageSubresource.layerCount = 1;
			buffer_image_copy.imageExtent.width = upload_w;
			buffer_image_copy.imageExtent.height = upload_h;
			buffer_image_copy.imageExtent.depth = 1;
			buffer_image_copy.imageOffset.x = upload_x;
			buffer_image_copy.imageOffset.y = upload_y;

			VkCopyBufferToImageInfo2 copy_buffer_to_image_info{};
			copy_buffer_to_image_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
			copy_buffer_to_image_info.srcBuffer = upload_buffer;
			copy_buffer_to_image_info.dstImage = bd_texture->image;
			copy_buffer_to_image_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			copy_buffer_to_image_info.regionCount = 1;
			copy_buffer_to_image_info.pRegions = &buffer_image_copy;
			vkCmdCopyBufferToImage2(bd->texture_command_buffer, &copy_buffer_to_image_info);
		} else {
			VkBufferImageCopy buffer_image_copy = {};
			buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			buffer_image_copy.imageSubresource.layerCount = 1;
			buffer_image_copy.imageExtent.width = upload_w;
			buffer_image_copy.imageExtent.height = upload_h;
			buffer_image_copy.imageExtent.depth = 1;
			buffer_image_copy.imageOffset.x = upload_x;
			buffer_image_copy.imageOffset.y = upload_y;
			vkCmdCopyBufferToImage(bd->texture_command_buffer, upload_buffer, bd_texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
		}

		if (vk->active_device_extensions.contains(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)) {
			VkPipelineStageFlags2 src_stage_mask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			VkPipelineStageFlags2 dst_stage_mask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

			VkImageMemoryBarrier2 use_barrier{};
			use_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			use_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			use_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
			use_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			use_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			use_barrier.image = bd_texture->image;
			use_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			use_barrier.subresourceRange.levelCount = 1;
			use_barrier.subresourceRange.layerCount = 1;
			use_barrier.srcStageMask = src_stage_mask;
			use_barrier.dstStageMask = dst_stage_mask;

			VkDependencyInfo dependency_info{};
			dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependency_info.imageMemoryBarrierCount = 1;
			dependency_info.pImageMemoryBarriers = &use_barrier;
			vkCmdPipelineBarrier2(bd->texture_command_buffer, &dependency_info);

			ERR_FAIL_VK_RET(vkEndCommandBuffer(bd->texture_command_buffer), false, "Failed to end command buffer.");

			VkCommandBufferSubmitInfo command_buffer_submit_info{};
			command_buffer_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
			command_buffer_submit_info.commandBuffer = bd->texture_command_buffer;

			VkSubmitInfo2 submit_info{};
			submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
			submit_info.commandBufferInfoCount = 1;
			submit_info.pCommandBufferInfos = &command_buffer_submit_info;
			ERR_FAIL_VK_RET(vkQueueSubmit2(vk->queue, 1, &submit_info, VK_NULL_HANDLE), false, "Failed to submit to queue.");
		} else {
			VkImageMemoryBarrier use_barrier{};
			use_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			use_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			use_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			use_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			use_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			use_barrier.image = bd_texture->image;
			use_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			use_barrier.subresourceRange.levelCount = 1;
			use_barrier.subresourceRange.layerCount = 1;
			vkCmdPipelineBarrier(bd->texture_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &use_barrier);

			ERR_FAIL_VK_RET(vkEndCommandBuffer(bd->texture_command_buffer), false, "Failed to end command buffer.");

			VkSubmitInfo submit_info{};
			submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit_info.commandBufferCount = 1;
			submit_info.pCommandBuffers = &bd->texture_command_buffer;
			ERR_FAIL_VK_RET(vkQueueSubmit(vk->queue, 1, &submit_info, VK_NULL_HANDLE), false, "Failed to submit to queue.");
		}

		// TODO: use fence instead of waiting (idle is blocking, fence check when needed)
		ERR_FAIL_VK_RET(vkQueueWaitIdle(vk->queue), false, "Failed to wait for idle queue.");
		vmaDestroyBuffer(vk->allocator, upload_buffer, upload_buffer_allocation);

		p_texture->SetStatus(ImTextureStatus_OK);
	}

	if (p_texture->Status == ImTextureStatus_WantDestroy && p_texture->UnusedFrames >= (int)p_window->frames_count) {
		Texture *bd_texture = (Texture *)p_texture->BackendUserData;
		if (bd_texture) {
			bd_texture->cleanup(vk->device, vk->allocator, bd->descriptor_pool);
			delete bd_texture;

			p_texture->SetTexID(ImTextureID_Invalid);
			p_texture->BackendUserData = nullptr;
		}
		p_texture->SetStatus(ImTextureStatus_Destroyed);
	}
	return true;
}
void ImGuiEngine::setup_render_state(VkCommandBuffer p_command_buffer, ImDrawData *p_draw_data, RenderBuffers *p_rb, int p_width, int p_height) {
	ImGuiBackendData *bd = get_backend_data();
	vkCmdBindPipeline(p_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bd->render_state->pipeline);

	if (p_draw_data->TotalVtxCount > 0) {
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(p_command_buffer, 0, 1, &p_rb->vertex_buffer, &offset);
		vkCmdBindIndexBuffer(p_command_buffer, p_rb->index_buffer, 0, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
	}

	VkViewport viewport{};
	viewport.width = (float)p_width;
	viewport.height = (float)p_height;
	viewport.maxDepth = 1.f;
	vkCmdSetViewport(p_command_buffer, 0, 1, &viewport);

	float constants[4];
	constants[0] = 2.f / p_draw_data->DisplaySize.x;
	constants[1] = 2.f / p_draw_data->DisplaySize.y;
	constants[2] = -1.f - p_draw_data->DisplayPos.x * constants[0];
	constants[3] = -1.f - p_draw_data->DisplayPos.y * constants[1];
	vkCmdPushConstants(p_command_buffer, bd->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 4, constants);

	vkCmdBindDescriptorSets(bd->render_state->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bd->pipeline_layout, 1, 1, &bd->sampler_linear_descriptor_set, 0, nullptr);
}

void ImGuiEngine::draw_callback__reset_render_state(const ImDrawList *, const ImDrawCmd *) {}
void ImGuiEngine::draw_callback__set_sampler_linear(const ImDrawList *, const ImDrawCmd *) {
	ImGuiBackendData *bd = get_backend_data();
	ERR_FAIL_NULL(bd, "BackendData is null.");

	vkCmdBindDescriptorSets(bd->render_state->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bd->render_state->pipeline_layout, 1, 1, &bd->sampler_linear_descriptor_set, 0, nullptr);
}
void ImGuiEngine::draw_callback__set_sampler_nearest(const ImDrawList *, const ImDrawCmd *) {
	ImGuiBackendData *bd = get_backend_data();
	ERR_FAIL_NULL(bd, "BackendData is null.");

	vkCmdBindDescriptorSets(bd->render_state->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bd->render_state->pipeline_layout, 1, 1, &bd->sampler_nearest_descriptor_set, 0, nullptr);
}

void ImGuiEngine::create_window(ImGuiViewport *p_viewport) {
	ImGuiBackendData *bd = get_backend_data();
	ERR_FAIL_NULL(bd, "BackendData is null.");
	ImguiViewportData *vd = new ImguiViewportData();
	p_viewport->RendererUserData = vd;
	VulkanEngine *vk = bd->vk;

	SDL_Window *window = SDL_GetWindowFromID((intptr_t)p_viewport->PlatformHandle);
	vd->window.sdl_window = window;

	ERR_FAIL_COND(!vk->create_surface(&vd->window), "Failed to create surface.");
	ERR_FAIL_COND(!vk->setup_swapchain(&vd->window), "Failed to setup swapchain.");
	ERR_FAIL_COND(!vk->create_window(&vd->window, p_viewport->Size.x, p_viewport->Size.y), "Failed to create window resources.");
	if (bd->pipeline_for_viewports == VK_NULL_HANDLE) {
		vk->pipeline_info_for_viewports.render_pass = vd->window.render_pass;
		bd->pipeline_for_viewports = create_pipeline(&vk->pipeline_info_for_viewports);
	}
}
void ImGuiEngine::destroy_window(ImGuiViewport *p_viewport) {
	ImGuiBackendData *bd = get_backend_data();
	ERR_FAIL_NULL(bd, "BackendData is null.");
	VulkanEngine *vk = bd->vk;

	ImguiViewportData *vd = (ImguiViewportData *)p_viewport->RendererUserData;
	if (vd) {
		ERR_FAIL_VK(vkDeviceWaitIdle(vk->device), "Failed to wait for the device before destroying viewport resources.");
		vd->cleanup(vk->instance, vk->device, vk->allocator);
		delete vd;
		vd = nullptr;
		p_viewport->RendererUserData = nullptr;
	}
}
void ImGuiEngine::set_window_size(ImGuiViewport *p_viewport, ImVec2 p_size) {
	ImGuiBackendData *bd = get_backend_data();
	ERR_FAIL_NULL(bd, "BackendData is null.");
	VulkanEngine *vk = bd->vk;

	ImguiViewportData *vd = (ImguiViewportData *)p_viewport->RendererUserData;
	if (!vd) {
		return;
	}
	vd->window.color_attachment.loadOp = (p_viewport->Flags & ImGuiViewportFlags_NoRendererClear) ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_CLEAR;
	vd->render_buffers.cleanup(vk->allocator);
	vk->create_window(&vd->window, p_size.x, p_size.y);
}
void ImGuiEngine::render_window(ImGuiViewport *p_viewport, void *) {
	ImGuiBackendData *bd = get_backend_data();
	ERR_FAIL_NULL(bd, "BackendData is null.");

	ImguiViewportData *vd = (ImguiViewportData *)p_viewport->RendererUserData;
	if (!vd) {
		return;
	}
	ERR_FAIL_COND(!acquire_next_image(&vd->window), "Failed to acquire next image.");
	ERR_FAIL_COND(!frame_render(&vd->window, p_viewport->DrawData, p_viewport->Size.x, p_viewport->Size.y), "Failed to render frame.");
}
void ImGuiEngine::swap_buffers(ImGuiViewport *p_viewport, void *) {
	ImGuiBackendData *bd = get_backend_data();
	ERR_FAIL_NULL(bd, "BackendData is null.");

	ImguiViewportData *vd = (ImguiViewportData *)p_viewport->RendererUserData;
	if (!vd) {
		return;
	}
	ERR_FAIL_COND(!frame_present(&vd->window), "Failed to present frame.");
}

ImGuiBackendData *ImGuiEngine::get_backend_data() {
	return ImGui::GetCurrentContext() ? (ImGuiBackendData *)(ImGui::GetIO().BackendRendererUserData) : nullptr;
}
