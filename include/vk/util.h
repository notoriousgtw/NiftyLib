#pragma once

//=============================================================================
// VULKAN UTILITY FUNCTIONS
//=============================================================================
// This header contains utility functions for common Vulkan operations
// such as creating synchronization objects.

#include "vk/Common.h"
#include "vk/Shader.h"

namespace nft::vulkan
{

	//=========================================================================
	// PIPELINE STAGES (Optimized for performance)
	//=========================================================================

	// Base pipeline stage - now uses device reference only
	struct PipelineStage
	{
		PipelineStage(Device* device) : device(device) {}
		
		Device* device;
	};

	// Shader stage base
	struct ShaderStage: public PipelineStage
	{
		ShaderStage(Device* device) : PipelineStage(device) {}

		vk::PipelineShaderStageCreateInfo vk_shader_stage_info;
		std::unique_ptr<Shader>			  shader;
	};

	// Vertex shader stage
	struct VertexShaderStage: public ShaderStage
	{
		VertexShaderStage(Device* device) : ShaderStage(device) {}
	};

	// Fragment shader stage
	struct FragmentShaderStage: public ShaderStage
	{
		FragmentShaderStage(Device* device) : ShaderStage(device) {}
	};

	// Vertex input stage
	struct VertexInputStage: public PipelineStage
	{
		VertexInputStage(Device* device) : PipelineStage(device) {}
		void Init();

		vk::PipelineVertexInputStateCreateInfo			 vk_vertex_input_info;
		vk::VertexInputBindingDescription				 binding_description;
		std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;
	};

	// Input assembly stage
	struct InputAssemblyStage: public PipelineStage
	{
		InputAssemblyStage(Device* device) : PipelineStage(device) {}
		void Init();

		vk::PipelineInputAssemblyStateCreateInfo vk_input_assembly_info;
	};

	// Viewport stage
	struct ViewportStage: public PipelineStage
	{
		ViewportStage(Device* device) : PipelineStage(device) {}
		void Init(vk::Extent2D extent);

		vk::Viewport						viewport;
		vk::Rect2D							scissor;
		vk::PipelineViewportStateCreateInfo vk_viewport_state_info;
	};

	// Rasterization stage
	struct RasterizationStage: public PipelineStage
	{
		RasterizationStage(Device* device) : PipelineStage(device) {}
		void Init();

		vk::PipelineRasterizationStateCreateInfo vk_rasterization_info;
	};

	// Multisample stage
	struct MultisampleStage: public PipelineStage
	{
		MultisampleStage(Device* device) : PipelineStage(device) {}
		void Init();

		vk::PipelineMultisampleStateCreateInfo vk_multisample_info;
	};

	// Color blend stage
	struct ColorBlendStage: public PipelineStage
	{
		ColorBlendStage(Device* device) : PipelineStage(device) {}
		void Init();

		vk::PipelineColorBlendAttachmentState color_blend_attachment;
		vk::PipelineColorBlendStateCreateInfo vk_color_blend_info;
	};

	// DescriptorSetLayout - keeps instance pointer for surface context and potential debugging
	struct DescriptorSetLayout
	{
		struct Binding
		{
			int					 index;
			vk::DescriptorType	 type;
			int					 count;
			vk::ShaderStageFlags stages;
		};

		DescriptorSetLayout(Surface* surface);
		DescriptorSetLayout(Device* device);
		void Init(std::vector<Binding> bindings);
		void Cleanup();

		vk::DescriptorSetLayout			  vk_descriptor_set_layout = VK_NULL_HANDLE;
		vk::DescriptorSetLayoutCreateInfo vk_descriptor_set_layout_info;

	  private:
		Surface*  surface = nullptr;
		Device*	  device = nullptr;
	};

	// DescriptorPool - optimized structure
	struct DescriptorPool
	{
		using Binding = DescriptorSetLayout::Binding;

		DescriptorPool(Device* device) : device(device) {}
		void Init(std::vector<Binding> bindings, uint32_t count);
		void Cleanup();

		vk::DescriptorPool			 vk_descriptor_pool = VK_NULL_HANDLE;
		vk::DescriptorPoolCreateInfo vk_descriptor_pool_info;

	  private:
		Device*	device;
	};

	// PipelineLayout - optimized structure
	struct PipelineLayout
	{
		PipelineLayout(Device* device) : device(device) {}
		void Init(std::vector<vk::DescriptorSetLayout> descriptor_set_layouts);
		void Cleanup();

		vk::PipelineLayout			 vk_pipeline_layout = VK_NULL_HANDLE;
		vk::PipelineLayoutCreateInfo vk_pipeline_layout_info;

	  private:
		Device*	device;
	};

	// RenderPass - optimized structure
	struct RenderPass
	{
		RenderPass(Device* device) : device(device) {}
		void Init(vk::Format format);
		void Cleanup();

		vk::RenderPass			  vk_render_pass = VK_NULL_HANDLE;
		vk::RenderPassCreateInfo  vk_render_pass_info;
		vk::AttachmentDescription color_attachment;
		vk::AttachmentReference	  color_attachment_ref;
		vk::SubpassDescription	  vk_subpass;

	  private:
		Device*	device;
	};

	// Returns a descriptor set
	vk::DescriptorSet GetDescriptorSet(Device* device, DescriptorPool* pool, DescriptorSetLayout* layout);

}	 // namespace nft::vulkan