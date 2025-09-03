#pragma once

#include "graphics/material.h"
#include "vk/common.h"
#include "vk/descriptors.h"
#include "vk/image.h"
#include "vk/resources.h"
#include "vk/shader.h"
#include <unordered_map>

// Forward declarations to break circular dependencies
namespace nft::vulkan
{
class Surface;
}	 // namespace nft::vulkan

namespace nft::vulkan
{

//=========================================================================
// FORWARD DECLARATIONS
//=========================================================================
class Device;
class Surface;
struct GeometryBatcher;

//=========================================================================
// ENTITY RENDER DATA STRUCTURE
//=========================================================================
// Structure to track render data per entity
struct EntityRenderData
{
	uint32_t vertex_offset;
	uint32_t vertex_count;
	uint32_t index_offset;
	uint32_t index_count;
	uint32_t transform_index;
	uint32_t material_index;
};

//=========================================================================
// CONSTANTS
//=========================================================================

//=========================================================================
// PIPELINE STAGES
//=========================================================================
// Base pipeline stage - now uses device reference only
struct PipelineStage
{
	PipelineStage(Device* device): device(device) {}
	virtual ~PipelineStage() = default;

	Device* device;
};

enum class ShaderType
{
	Vertex,
	Geometry,
	Fragment,
	Compute
};

// Shader stage base
struct ShaderStage: public PipelineStage
{
	ShaderStage(Device* device): PipelineStage(device) {}

	virtual void Init(Shader::ShaderCode code) = 0;

	vk::PipelineShaderStageCreateInfo vk_shader_stage_info;
	std::unique_ptr<Shader>			  shader;
};

// Vertex shader stage
struct VertexShaderStage: public ShaderStage
{
	VertexShaderStage(Device* device): ShaderStage(device) {}
	void Init(Shader::ShaderCode code) override
	{
		if (shader.get())
			shader.reset();
		shader				 = std::make_unique<Shader>(device, code);
		vk_shader_stage_info = vk::PipelineShaderStageCreateInfo()
								   .setStage(vk::ShaderStageFlagBits::eVertex)
								   .setModule(shader->GetShaderModule())
								   .setPName("main");
	}
};

// Geometry shader stage
struct GeometryShaderStage: public ShaderStage
{
	GeometryShaderStage(Device* device): ShaderStage(device) {}
	void Init(Shader::ShaderCode code) override
	{
		if (shader.get())
			shader.reset();
		shader				 = std::make_unique<Shader>(device, code);
		vk_shader_stage_info = vk::PipelineShaderStageCreateInfo()
								   .setStage(vk::ShaderStageFlagBits::eGeometry)
								   .setModule(shader->GetShaderModule())
								   .setPName("main");
	}
};

// Fragment shader stage
struct FragmentShaderStage: public ShaderStage
{
	FragmentShaderStage(Device* device): ShaderStage(device) {}
	void Init(Shader::ShaderCode code) override
	{
		if (shader.get())
			shader.reset();
		shader				 = std::make_unique<Shader>(device, code);
		vk_shader_stage_info = vk::PipelineShaderStageCreateInfo()
								   .setStage(vk::ShaderStageFlagBits::eFragment)
								   .setModule(shader->GetShaderModule())
								   .setPName("main");
	}
};

// Vertex input stage
struct VertexInputStage: public PipelineStage
{
	VertexInputStage(Device* device): PipelineStage(device) {}
	void Init();

	vk::PipelineVertexInputStateCreateInfo			 vk_vertex_input_info;
	vk::VertexInputBindingDescription				 binding_description;
	std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;
};

// Input assembly stage
struct InputAssemblyStage: public PipelineStage
{
	InputAssemblyStage(Device* device): PipelineStage(device) {}
	void Init(vk::PrimitiveTopology topology, vk::Bool32 restart_enable = VK_FALSE);

	vk::PipelineInputAssemblyStateCreateInfo vk_input_assembly_info;
};

// Viewport stage
struct ViewportStage: public PipelineStage
{
	ViewportStage(Device* device): PipelineStage(device) {}
	void Init(vk::Extent2D extent,
			  glm::vec2	   view_pos = { 0.0f, 0.0f },
			  glm::vec2	   depth	= { 0.0f, 1.0f },
			  vk::Offset2D offset	= { 0, 0 });

	vk::Viewport						viewport;
	vk::Rect2D							scissor;
	vk::PipelineViewportStateCreateInfo vk_viewport_state_info;
};

// Rasterization stage
struct RasterizationStage: public PipelineStage
{
	RasterizationStage(Device* device): PipelineStage(device) {}
	void Init();

	vk::PipelineRasterizationStateCreateInfo vk_rasterization_info;
};

// Depth-stencil stage
struct DepthStencilStage: public PipelineStage
{
	DepthStencilStage(Device* device): PipelineStage(device) {}
	void									Init();
	vk::PipelineDepthStencilStateCreateInfo vk_depth_stencil_info;
};

// Multisample stage
struct MultisampleStage: public PipelineStage
{
	MultisampleStage(Device* device): PipelineStage(device) {}
	void Init();

	vk::PipelineMultisampleStateCreateInfo vk_multisample_info;
};

// Color blend stage
struct ColorBlendStage: public PipelineStage
{
	ColorBlendStage(Device* device): PipelineStage(device) {}
	void Init();

	vk::PipelineColorBlendAttachmentState color_blend_attachment;
	vk::PipelineColorBlendStateCreateInfo vk_color_blend_info;
};

// DescriptorSetLayout - accepts both Surface and Device for flexibility
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
	~DescriptorSetLayout() { Cleanup(); }

	void Init(std::vector<Binding> bindings);
	void Cleanup();

	vk::DescriptorSetLayout			  vk_descriptor_set_layout = VK_NULL_HANDLE;
	vk::DescriptorSetLayoutCreateInfo vk_descriptor_set_layout_info;

  private:
	Surface* surface = nullptr;
	Device*	 device	 = nullptr;
};

// DescriptorPool - optimized structure
struct DescriptorPool
{
	using Binding = DescriptorSetLayout::Binding;

	DescriptorPool(Device* device): device(device) {}
	~DescriptorPool() { Cleanup(); }

	void Init(std::vector<Binding> bindings, uint32_t count);
	void Cleanup();

	vk::DescriptorPool			 vk_descriptor_pool = VK_NULL_HANDLE;
	vk::DescriptorPoolCreateInfo vk_descriptor_pool_info;

  private:
	Device* device;
};

// PipelineLayout - optimized structure
struct PipelineLayout
{
	PipelineLayout(Device* device): device(device) {}
	~PipelineLayout() { Cleanup(); }

	void Init(std::vector<vk::DescriptorSetLayout> descriptor_set_layouts,
			  std::vector<vk::PushConstantRange>   push_constant_ranges = {});
	void Cleanup();

	vk::PipelineLayout			 vk_pipeline_layout = VK_NULL_HANDLE;
	vk::PipelineLayoutCreateInfo vk_pipeline_layout_info;

  private:
	Device* device;
};

// RenderPass - for graphics pipelines only
struct RenderPass
{
	RenderPass(Device* device): device(device) {}
	~RenderPass() { Cleanup(); }

	void Init(vk::Format color_format, vk::Format depth_format);
	void Init(const std::vector<vk::AttachmentDescription>& attachments,
			  const std::vector<vk::SubpassDescription>&	subpasses,
			  const std::vector<vk::SubpassDependency>&		dependencies = {});
	void Cleanup();

	vk::RenderPass			  vk_render_pass = VK_NULL_HANDLE;
	vk::RenderPassCreateInfo  vk_render_pass_info;
	vk::AttachmentDescription color_attachment;
	vk::AttachmentReference	  color_attachment_ref;
	vk::AttachmentDescription depth_attachment;
	vk::AttachmentReference	  depth_attachment_ref;
	vk::SubpassDescription	  vk_subpass;

  private:
	Device* device;
};

//=========================================================================
// FRAME STRUCTURE (moved from Surface)
//=========================================================================
struct Frame
{
	// Swapchain resources
	Image swapchain_image;
	Image depth_buffer;

	vk::Framebuffer			  vk_frame_buffer = VK_NULL_HANDLE;
	vk::FramebufferCreateInfo vk_frame_buffer_info;
	uint32_t				  width;
	uint32_t				  height;

	vk::CommandBuffer vk_command_buffer = VK_NULL_HANDLE;

	// Synchronization objects
	vk::Fence	  in_flight_fence			= VK_NULL_HANDLE;
	vk::Semaphore image_available_semaphore = VK_NULL_HANDLE;
	vk::Semaphore render_finished_semaphore = VK_NULL_HANDLE;

	// Frame index for bindless camera data access
	uint32_t frame_index = 0;

	// Camera data for this frame (stored in shared bindless buffer)
	CameraData camera_data;

	// Methods
	void Init(Surface* surface, uint32_t frame_idx);
	void Prepare(glm::mat4 camera_transforms);
	void Cleanup();

  private:
	Surface* surface = nullptr;	   // Pointer to the parent surface
	Device*	 device	 = nullptr;
};

//=========================================================================
// PIPELINE TYPE ENUMERATION
//=========================================================================
enum class PipelineType
{
	Graphics,
	Compute,
	RayTracing	  // For future use
};

//=========================================================================
// ABSTRACT PIPELINE BASE CLASS - Now truly abstract
//=========================================================================
class AbstractPipeline
{
  public:
	AbstractPipeline(Device* device, PipelineType type);
	virtual ~AbstractPipeline();

	// Core pipeline interface
	// virtual void Init() = 0;
	// virtual void AddShaderStage(Shader::ShaderCode shader_code, vk::ShaderStageFlagBits stage) = 0;
	virtual void Cleanup();

	// Getters
	const vk::Pipeline&		  GetVkPipeline() const { return vk_pipeline; }
	const vk::PipelineLayout& GetPipelineLayout() const { return pipeline_layout.vk_pipeline_layout; }
	PipelineType			  GetPipelineType() const { return pipeline_type; }

	// Command pool access for all pipeline types
	vk::CommandPool	  GetCommandPool() const { return vk_command_pool; }
	vk::CommandBuffer GetCommandBuffer() const { return vk_command_buffer; }

  protected:
	Device*		 device = nullptr;
	PipelineType pipeline_type;

	// Command objects - common to all pipeline types
	vk::CommandPool			  vk_command_pool	= VK_NULL_HANDLE;
	vk::CommandBuffer		  vk_command_buffer = VK_NULL_HANDLE;
	vk::CommandPoolCreateInfo vk_command_pool_info;

	// Core pipeline objects - common to all pipeline types
	vk::Pipeline												 vk_pipeline = VK_NULL_HANDLE;
	std::unordered_map<ShaderType, std::unique_ptr<ShaderStage>> shader_stages;
	std::vector<vk::DescriptorSetLayout>						 vk_descriptor_set_layouts;
	PipelineLayout												 pipeline_layout;

	// Virtual methods for customization
	virtual void SetupDescriptorLayouts() = 0;
	virtual void CreateCommandPool();
};

class ComputeComponentBase
{
  public:
	virtual ~ComputeComponentBase() = default;

	// Core compute interface

	// Pipeline management
	virtual void Create()	= 0;
	virtual void Recreate() = 0;
	// virtual void CreatePipeline() = 0;
	// virtual void RecreatePipeline() = 0;
};

//=========================================================================
// ABSTRACT RENDERER INTERFACE - For graphics pipelines
//=========================================================================
class RenderPipelineBase: public AbstractPipeline
{
  public:
	RenderPipelineBase(Device* device, PipelineType type);
	virtual ~RenderPipelineBase() = default;

	virtual void RecordDrawCommands(vk::CommandBuffer command_buffer, uint32_t image_index) = 0;
	virtual void RecordDrawCommandsWithEntities(vk::CommandBuffer command_buffer, uint32_t image_index, 
												const std::vector<EntityRenderData>& entities) = 0;
	void		 SetVertexShader(Shader::ShaderCode shader_code);
	void		 SetGeometryShader(Shader::ShaderCode shader_code);
	void		 SetFragmentShader(Shader::ShaderCode shader_code);

	// Pipeline management
	virtual void Create(const RenderPass& render_pass)	 = 0;
	virtual void Recreate(const RenderPass& render_pass) = 0;

  protected:
	// Override the abstract method from AbstractPipeline
	void SetupDescriptorLayouts() override;

	vk::GraphicsPipelineCreateInfo vk_pipeline_info;

	// Pipeline stages
	VertexInputStage	   vertex_input_stage;
	InputAssemblyStage	   input_assembly_stage;
	ViewportStage		   viewport_stage;
	RasterizationStage	   rasterization_stage;
	DepthStencilStage	   depth_stencil_stage;
	MultisampleStage	   multisample_stage;
	ColorBlendStage		   color_blend_stage;
	GlobalBindlessManager* bindless_resource_manager = nullptr;

	// Descriptor layout for bindless resources
	//DescriptorSetLayout bindless_set_layout;
};

class SwapchainRenderPipeline: public RenderPipelineBase
{
  public:
	SwapchainRenderPipeline(Surface* surface, PipelineType type);
	~SwapchainRenderPipeline() = default;

	void RecordDrawCommands(vk::CommandBuffer command_buffer, uint32_t image_index) override;
	
	// New method to record draw commands with entity data
	void RecordDrawCommandsWithEntities(vk::CommandBuffer command_buffer, uint32_t image_index, 
										const std::vector<EntityRenderData>& entities) override;

	// Bind the global bindless descriptor set for rendering
	void BindGlobalBindlessDescriptors(vk::CommandBuffer command_buffer, uint32_t frame_index);

	// RenderPipelineBase implementation
	void Create(const RenderPass& render_pass) override;
	void Recreate(const RenderPass& render_pass) override;

  protected:
	Surface* surface;
};

// class OffscreenRenderPipeline: public RenderPipelineBase
//{
//   public:
//	virtual ~OffscreenRenderPipeline() = default;
//
//	// RenderPipelineBase implementation
//	// void Create(RenderPass render_pass) override;
//	// void Recreate(RenderPass render_pass) override;
// };

//=========================================================================
// COMPUTE PIPELINE BASE CLASS
//=========================================================================
// class ComputePipelineBase: public AbstractPipeline
//{
//  public:
//	ComputePipelineBase(Device* device);
//	virtual ~ComputePipelineBase() = default;
//
//	// AbstractPipeline implementation
//	//void Init() override {  Default implementation, can be overridden */ }
//	void AddShaderStage(Shader::ShaderCode shader_code, vk::ShaderStageFlagBits stage) override;
//
//	// Compute-specific methods
//	void Dispatch(uint32_t group_count_x, uint32_t group_count_y = 1, uint32_t group_count_z = 1);
//	void Dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z, vk::CommandBuffer command_buffer);
//
//  protected:
//	vk::ComputePipelineCreateInfo vk_compute_pipeline_info;
//
//	// Virtual methods for compute pipeline customization
//	virtual void SetupComputeStage() = 0;
//};

//=========================================================================
// OFFSCREEN GRAPHICS PIPELINE - RENDERS TO CUSTOM FRAMEBUFFER
//=========================================================================
// class OffscreenGraphicsPipeline: public GraphicsPipelineBase
//{
//   public:
//	OffscreenGraphicsPipeline(Device*	   device,
//							  vk::Extent2D extent,
//							  vk::Format   color_format,
//							  vk::Format   depth_format = vk::Format::eUndefined);
//	~OffscreenGraphicsPipeline() = default;
//
//	// AbstractPipeline implementation
//	void Init() override;
//
//	// RenderPipelineBase implementation
//
//	// Offscreen-specific methods
//	void   SetRenderTarget(Image* color_target, Image* depth_target = nullptr);
//	void   CreateFramebuffer();
//	Image* GetColorTarget() const { return color_target; }
//	Image* GetDepthTarget() const { return depth_target; }
//
//  protected:
//	void SetupDescriptorLayouts() override;
//
//  private:
//	vk::Extent2D extent;
//	vk::Format	 color_format;
//	vk::Format	 depth_format;
//
//	Image*			color_target = nullptr;
//	Image*			depth_target = nullptr;
//	vk::Framebuffer framebuffer	 = VK_NULL_HANDLE;
//};

//=========================================================================
// EXAMPLE COMPUTE PIPELINE
//=========================================================================
// class ExampleComputePipeline: public ComputePipelineBase
//{
//  public:
//	ExampleComputePipeline(Device* device);
//	~ExampleComputePipeline() = default;
//
//	// AbstractPipeline implementation
//	void Init() override;
//
//  protected:
//	void SetupDescriptorLayouts() override;
//	void SetupComputeStage() override;
//
//  private:
//	DescriptorSetLayout compute_set_layout;
//	DescriptorPool		compute_descriptor_pool;
//};

// Returns a descriptor set
vk::DescriptorSet GetDescriptorSet(Device* device, DescriptorPool* pool, DescriptorSetLayout* layout);

}	 // namespace nft::vulkan