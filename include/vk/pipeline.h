#pragma once

#include "vk/common.h"
#include "vk/shader.h"
#include "vk/image.h"

// Forward declarations to break circular dependencies
namespace nft::vulkan 
{
class Surface;
class Scene;
}

namespace nft::vulkan
{

//=========================================================================
// FORWARD DECLARATIONS
//=========================================================================
class Device;
class Surface;
class Scene;
struct GeometryBatcher;

//=========================================================================
// UNIFORM BUFFER OBJECT
//=========================================================================
struct UniformBufferObject
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec3 pos;
};

//=========================================================================
// CONSTANTS
//=========================================================================
constexpr uint32_t MAX_OBJECTS = 1000;  // Maximum number of objects in the scene

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

// Shader stage base
struct ShaderStage: public PipelineStage
{
	ShaderStage(Device* device): PipelineStage(device) {}

	vk::PipelineShaderStageCreateInfo vk_shader_stage_info;
	std::unique_ptr<Shader>			  shader;
};

// Vertex shader stage
struct VertexShaderStage: public ShaderStage
{
	VertexShaderStage(Device* device): ShaderStage(device) {}
};

// Fragment shader stage
struct FragmentShaderStage: public ShaderStage
{
	FragmentShaderStage(Device* device): ShaderStage(device) {}
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
	void Init(std::vector<vk::DescriptorSetLayout> descriptor_set_layouts, 
			  std::vector<vk::PushConstantRange> push_constant_ranges = {});
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
	void Init(vk::Format color_format, vk::Format depth_format);
	void Init(const std::vector<vk::AttachmentDescription>& attachments,
			  const std::vector<vk::SubpassDescription>& subpasses,
			  const std::vector<vk::SubpassDependency>& dependencies = {});
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

	// Frame resources
	UniformBufferObject	   camera_data;
	Buffer*				   camera_data_buffer = nullptr;
	void*				   camera_data_ptr	  = nullptr;
	std::vector<glm::mat4> object_transforms;
	Buffer*				   object_transform_buffer = nullptr;
	void*				   object_transform_ptr	   = nullptr;

	// Resource descriptors
	vk::DescriptorSet vk_descriptor_set = VK_NULL_HANDLE;	 // Frame data (camera + transforms)

	// Methods
	void Init(Surface* surface, Scene* scene);
	void MakeDescriptorResources();
	void AllocateDescriptorResources();
	void AllocateFrameDescriptorSet(DescriptorPool* frame_descriptor_pool, DescriptorSetLayout* frame_set_layout);
	void MakeDepthResources();
	void Prepare(glm::mat4 camera_transforms);
	void Cleanup();

  private:
	Surface* surface = nullptr;	   // Pointer to the parent surface
	Device*	 device	 = nullptr;
	Scene*	 scene	 = nullptr;
};

//=========================================================================
// PIPELINE TYPE ENUMERATION
//=========================================================================
enum class PipelineType
{
	Graphics,
	Compute,
	RayTracing  // For future use
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
	virtual void Init() = 0;
	virtual void AddShaderStage(Shader::ShaderCode shader_code, vk::ShaderStageFlagBits stage) = 0;
	virtual void Create() = 0;
	virtual void Recreate() = 0;
	virtual void Cleanup();

	// Getters
	const vk::Pipeline& GetVkPipeline() const { return vk_pipeline; }
	const vk::PipelineLayout& GetPipelineLayout() const { return pipeline_layout.vk_pipeline_layout; }
	PipelineType GetPipelineType() const { return pipeline_type; }

	// Command pool access for all pipeline types
	vk::CommandPool GetCommandPool() const { return vk_command_pool; }
	vk::CommandBuffer GetCommandBuffer() const { return vk_command_buffer; }

  protected:
	Device* device = nullptr;
	PipelineType pipeline_type;

	// Command objects - common to all pipeline types
	vk::CommandPool			  vk_command_pool	= VK_NULL_HANDLE;
	vk::CommandBuffer		  vk_command_buffer = VK_NULL_HANDLE;
	vk::CommandPoolCreateInfo vk_command_pool_info;

	// Core pipeline objects - common to all pipeline types
	vk::Pipeline						 vk_pipeline = VK_NULL_HANDLE;
	std::vector<ShaderStage>			 shader_stages;
	std::vector<vk::DescriptorSetLayout> vk_descriptor_set_layouts;
	PipelineLayout						 pipeline_layout;

	// Virtual methods for customization
	virtual void SetupDescriptorLayouts() = 0;
	virtual void CreateCommandPool();
};

//=========================================================================
// ABSTRACT RENDERER INTERFACE - For graphics pipelines
//=========================================================================
class IRenderer
{
  public:
	virtual ~IRenderer() = default;
	
	// Core rendering interface
	virtual void PrepareScene(vk::CommandBuffer command_buffer) = 0;
	virtual void RecordDrawCommands(Frame& frame, uint32_t image_index) = 0;
	virtual void CreateTextureDescriptorSet() = 0;
	
	// Pipeline management
	virtual void CreatePipeline() = 0;
	virtual void RecreatePipeline() = 0;
};

//=========================================================================
// GRAPHICS PIPELINE BASE CLASS
//=========================================================================
class GraphicsPipelineBase : public AbstractPipeline, public IRenderer
{
  public:
	GraphicsPipelineBase(Device* device);
	virtual ~GraphicsPipelineBase() = default;

	// AbstractPipeline implementation
	void Init() override { /* Default implementation, can be overridden */ }
	void AddShaderStage(Shader::ShaderCode shader_code, vk::ShaderStageFlagBits stage) override;
	void Create() override;
	void Recreate() override;
	void Cleanup() override;

	// Graphics-specific methods
	void InitGraphics(vk::Extent2D extent, vk::Format color_format, vk::Format depth_format);
	void InitGraphics(vk::Extent2D extent, const RenderPass& render_pass);
	
	// Getters for graphics-specific objects
	const vk::RenderPass& GetRenderPass() const { return render_pass.vk_render_pass; }

  protected:
	// Graphics-specific pipeline stages
	VertexInputStage					 vertex_input_stage;
	InputAssemblyStage					 input_assembly_stage;
	ViewportStage						 viewport_stage;
	RasterizationStage					 rasterization_stage;
	DepthStencilStage					 depth_stencil_stage;
	MultisampleStage					 multisample_stage;
	ColorBlendStage						 color_blend_stage;
	RenderPass							 render_pass;
	vk::GraphicsPipelineCreateInfo		 vk_pipeline_info;

	// Virtual methods for graphics pipeline customization
	virtual void SetupPipelineStages(vk::Extent2D extent);
};

//=========================================================================
// COMPUTE PIPELINE BASE CLASS
//=========================================================================
class ComputePipelineBase : public AbstractPipeline
{
  public:
	ComputePipelineBase(Device* device);
	virtual ~ComputePipelineBase() = default;

	// AbstractPipeline implementation
	void Init() override { /* Default implementation, can be overridden */ }
	void AddShaderStage(Shader::ShaderCode shader_code, vk::ShaderStageFlagBits stage) override;
	void Create() override;
	void Recreate() override;

	// Compute-specific methods
	void Dispatch(uint32_t group_count_x, uint32_t group_count_y = 1, uint32_t group_count_z = 1);
	void Dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z, 
				  vk::CommandBuffer command_buffer);

  protected:
	vk::ComputePipelineCreateInfo vk_compute_pipeline_info;
	
	// Virtual methods for compute pipeline customization
	virtual void SetupComputeStage() = 0;
};

//=========================================================================
// SWAPCHAIN GRAPHICS PIPELINE - Renders to swapchain
//=========================================================================
class GraphicsPipeline : public GraphicsPipelineBase
{
  public:
	GraphicsPipeline(Device* device, Scene* scene);
	~GraphicsPipeline() override = default;

	// AbstractPipeline implementation
	void Init() override;

	// IRenderer implementation
	void PrepareScene(vk::CommandBuffer command_buffer) override;
	void RecordDrawCommands(Frame& frame, uint32_t image_index) override;
	void CreateTextureDescriptorSet() override;
	void CreatePipeline() override;
	void RecreatePipeline() override;

	// Specific to swapchain graphics pipeline
	void SetScene(Scene* scene) { this->scene = scene; }
	Scene* GetScene() const { return scene; }
	
	// Public access to descriptor resources for frame allocation
	DescriptorPool* GetFrameDescriptorPool() { return &frame_descriptor_pool; }
	DescriptorSetLayout* GetFrameSetLayout() { return &frame_set_layout; }

  protected:
	void SetupDescriptorLayouts() override;

  private:
	Scene* scene = nullptr;
	vk::DescriptorSet texture_descriptor_set = VK_NULL_HANDLE;

	// Scene-specific descriptor resources
	DescriptorSetLayout frame_set_layout;
	DescriptorSetLayout texture_set_layout;
	DescriptorPool frame_descriptor_pool;
	DescriptorPool texture_descriptor_pool;

	vk::ClearValue clear_color;
	vk::ClearValue clear_depth;
};

//=========================================================================
// OFFSCREEN GRAPHICS PIPELINE - Renders to custom framebuffer
//=========================================================================
class OffscreenGraphicsPipeline : public GraphicsPipelineBase
{
  public:
	OffscreenGraphicsPipeline(Device* device, vk::Extent2D extent, 
							  vk::Format color_format, vk::Format depth_format = vk::Format::eUndefined);
	~OffscreenGraphicsPipeline() = default;

	// AbstractPipeline implementation
	void Init() override;

	// IRenderer implementation
	void PrepareScene(vk::CommandBuffer command_buffer) override { /* Custom implementation */ }
	void RecordDrawCommands(Frame& frame, uint32_t image_index) override { /* Custom implementation */ }
	void CreateTextureDescriptorSet() override { /* Custom implementation */ }
	void CreatePipeline() override;
	void RecreatePipeline() override;

	// Offscreen-specific methods
	void SetRenderTarget(Image* color_target, Image* depth_target = nullptr);
	void CreateFramebuffer();
	Image* GetColorTarget() const { return color_target; }
	Image* GetDepthTarget() const { return depth_target; }

  protected:
	void SetupDescriptorLayouts() override;

  private:
	vk::Extent2D extent;
	vk::Format color_format;
	vk::Format depth_format;
	
	Image* color_target = nullptr;
	Image* depth_target = nullptr;
	vk::Framebuffer framebuffer = VK_NULL_HANDLE;
};

//=========================================================================
// EXAMPLE COMPUTE PIPELINE
//=========================================================================
class ExampleComputePipeline : public ComputePipelineBase
{
  public:
	ExampleComputePipeline(Device* device);
	~ExampleComputePipeline() = default;

	// AbstractPipeline implementation
	void Init() override;

  protected:
	void SetupDescriptorLayouts() override;
	void SetupComputeStage() override;

  private:
	DescriptorSetLayout compute_set_layout;
	DescriptorPool compute_descriptor_pool;
};

// Returns a descriptor set
vk::DescriptorSet GetDescriptorSet(Device* device, DescriptorPool* pool, DescriptorSetLayout* layout);

}	 // namespace nft::vulkan