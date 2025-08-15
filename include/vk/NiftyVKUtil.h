#pragma once

//=============================================================================
// VULKAN UTILITY FUNCTIONS
//=============================================================================
// This header contains utility functions for common Vulkan operations
// such as creating synchronization objects.

#include "vk/NiftyVKCommon.h"
#include "vk/NiftyVKShader.h"

namespace nft::vulkan
{

	//=========================================================================
	// PIPELINE STAGES (Integrated for performance)
	//=========================================================================

	// Base pipeline stage
	struct PipelineStage
	{
		PipelineStage(Device* device, Instance* instance): device(device), instance(instance) {};
		Device*	  device;
		Instance* instance;
	};

	// Shader stage base
	struct ShaderStage: public PipelineStage
	{
		ShaderStage(Device* device, Instance* instance): PipelineStage(device, instance) {};

		vk::PipelineShaderStageCreateInfo vk_shader_stage_info;
		std::unique_ptr<Shader>			  shader;
	};

	// Vertex shader stage
	struct VertexShaderStage: public ShaderStage
	{
		VertexShaderStage(Device* device, Instance* instance): ShaderStage(device, instance) {};
	};

	// Fragment shader stage
	struct FragmentShaderStage: public ShaderStage
	{
		FragmentShaderStage(Device* device, Instance* instance): ShaderStage(device, instance) {};
	};

	// Vertex input stage
	struct VertexInputStage: public PipelineStage
	{
		VertexInputStage(Device* device, Instance* instance): PipelineStage(device, instance) {};
		void Init();

		vk::PipelineVertexInputStateCreateInfo			 vk_vertex_input_info;
		vk::VertexInputBindingDescription				 binding_description;
		std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;
	};

	// Input assembly stage
	struct InputAssemblyStage: public PipelineStage
	{
		InputAssemblyStage(Device* device, Instance* instance): PipelineStage(device, instance) {};
		void Init();

		vk::PipelineInputAssemblyStateCreateInfo vk_input_assembly_info;
	};

	// Viewport stage
	struct ViewportStage: public PipelineStage
	{
		ViewportStage(Device* device, Instance* instance): PipelineStage(device, instance) {};
		void Init(vk::Extent2D extent);

		vk::Viewport						viewport;
		vk::Rect2D							scissor;
		vk::PipelineViewportStateCreateInfo vk_viewport_state_info;
	};

	// Rasterization stage
	struct RasterizationStage: public PipelineStage
	{
		RasterizationStage(Device* device, Instance* instance): PipelineStage(device, instance) {};
		void Init();

		vk::PipelineRasterizationStateCreateInfo vk_rasterization_info;
	};

	// Multisample stage
	struct MultisampleStage: public PipelineStage
	{
		MultisampleStage(Device* device, Instance* instance): PipelineStage(device, instance) {};
		void Init();

		vk::PipelineMultisampleStateCreateInfo vk_multisample_info;
	};

	// Color blend stage
	struct ColorBlendStage: public PipelineStage
	{
		ColorBlendStage(Device* device, Instance* instance): PipelineStage(device, instance) {};
		void Init();

		vk::PipelineColorBlendAttachmentState color_blend_attachment;
		vk::PipelineColorBlendStateCreateInfo vk_color_blend_info;
	};

	// Pipeline layout
	struct DescriptorSetLayout
	{
		struct Binding
		{
			int					 index;
			vk::DescriptorType	 type;
			int					 count;
			vk::ShaderStageFlags stages;
		};

		DescriptorSetLayout(Device* device, Instance* instance): device(device), instance(instance) {};
		void Init(std::vector<Binding> bindings);
		void Cleanup();

		vk::DescriptorSetLayout			  vk_descriptor_set_layout = VK_NULL_HANDLE;
		vk::DescriptorSetLayoutCreateInfo vk_descriptor_set_layout_info;

	  private:
		Device*	  device;
		Instance* instance;
	};

	// Descriptor pool
	struct DescriptorPool
	{
		using Binding = DescriptorSetLayout::Binding;

		DescriptorPool(Device* device, Instance* instance): device(device), instance(instance) {};
		void Init(std::vector<Binding> bindings, uint32_t count);
		void Cleanup();

		vk::DescriptorPool			 vk_descriptor_pool = VK_NULL_HANDLE;
		vk::DescriptorPoolCreateInfo vk_descriptor_pool_info;

	  private:
		Device*	  device;
		Instance* instance;
	};

	// Pipeline layout
	struct PipelineLayout
	{
		PipelineLayout(Device* device, Instance* instance): device(device), instance(instance) {};
		void Init(vk::DescriptorSetLayout descriptor_set_layout);
		void Cleanup();

		vk::PipelineLayout			 vk_pipeline_layout = VK_NULL_HANDLE;
		vk::PipelineLayoutCreateInfo vk_pipeline_layout_info;

	  private:
		Device*	  device;
		Instance* instance;
	};

	// Render pass
	struct RenderPass
	{
		RenderPass(Device* device, Instance* instance): device(device), instance(instance) {};
		void Init(vk::Format format);
		void Cleanup();

		vk::RenderPass			  vk_render_pass = VK_NULL_HANDLE;
		vk::RenderPassCreateInfo  vk_render_pass_info;
		vk::AttachmentDescription color_attachment;
		vk::AttachmentReference	  color_attachment_ref;
		vk::SubpassDescription	  vk_subpass;

	  private:
		Device*	  device;
		Instance* instance;
	};

	// Returns a descriptor set
	vk::DescriptorSet GetDescriptorSet(Device* device, DescriptorPool* pool, DescriptorSetLayout* layout);

}	 // namespace nft::vulkan