#pragma once

// Core includes
#include "core/app.h"
#include "core/error.h"
#include "gui/window.h"
#include "vk/common.h"
#include "vk/shader.h"
#include "vk/util.h"
#include "core/glfw_common.h"

namespace nft::vulkan
{
// Forward declarations
class Instance;
class Device;

//=============================================================================
// OBJECT PICKER CLASS
//=============================================================================
class ObjectPicker
{
  public:
	ObjectPicker(Device* device, vk::Extent2D extent);
	~ObjectPicker();

	void Init();
	void Cleanup();
	void Recreate(vk::Extent2D new_extent);

	// Render objects with color IDs and read the pixel at the given position
	uint32_t PickObject(const std::vector<ObjectData>& objects,
						const GeometryBatcher*		   geometry_batcher,
						const glm::mat4&			   view_matrix,
						const glm::mat4&			   proj_matrix,
						int							   mouse_x,
						int							   mouse_y);

  private:
	Device*		 device;
	vk::Extent2D extent;

	// Offscreen render resources
	Image			color_attachment;
	Image			depth_attachment;
	vk::Framebuffer framebuffer = VK_NULL_HANDLE;

	std::vector<ShaderStage> shader_stages;	   // Shader stages for picking

	vk::CommandPool			  vk_command_pool	= VK_NULL_HANDLE;
	vk::CommandBuffer		  vk_command_buffer = VK_NULL_HANDLE;
	vk::CommandPoolCreateInfo vk_command_pool_info;

	// Picking render pass and pipeline
	VertexInputStage			   vertex_input_stage;
	InputAssemblyStage			   input_assembly_stage;
	ViewportStage				   viewport_stage;
	RasterizationStage			   rasterization_stage;
	DepthStencilStage			   depth_stencil_stage;
	MultisampleStage			   multisample_stage;
	ColorBlendStage				   color_blend_stage;
	RenderPass					   render_pass;
	vk::Pipeline				   pipeline = VK_NULL_HANDLE;
	vk::GraphicsPipelineCreateInfo pipeline_info;
	PipelineLayout				   pipeline_layout;
	DescriptorSetLayout			   picking_set_layout;
	DescriptorPool				   picking_descriptor_pool;

	// Shader stages for picking
	std::vector<ShaderStage> picking_shader_stages;

	// Buffer for reading back pixel data
	Buffer* readback_buffer = nullptr;

	// Command buffer for picking operations
	vk::CommandBuffer command_buffer = VK_NULL_HANDLE;

	void CreateCommandPool();
	void CreateRenderPass();
	void CreatePipeline();
	void CreateFramebuffer();
	void CreateShaders();
	void RecordPickingCommands(const std::vector<ObjectData>& objects,
							   const GeometryBatcher*		  geometry_batcher,
							   const glm::mat4&				  view_matrix,
							   const glm::mat4&				  proj_matrix);
};

//=============================================================================
// SURFACE CLASS
//=============================================================================
class Surface
{
  public:
	//=========================================================================
	// STRUCTURES AND NESTED CLASSES
	//=========================================================================

	// Swapchain support information
	struct SwapchainSupportDetails
	{
		vk::SurfaceCapabilitiesKHR		  capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR>	  present_modes;
	};

	struct UniformBufferObject
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec3 pos;
	};

	// Individual frame data for rendering
	struct Frame
	{
		// swapchain
		Image swapchain_image;
		Image depth_buffer;

		vk::Framebuffer			  vk_frame_buffer = VK_NULL_HANDLE;
		vk::FramebufferCreateInfo vk_frame_buffer_info;
		uint32_t				  width;
		uint32_t				  height;

		vk::CommandBuffer vk_command_buffer = VK_NULL_HANDLE;

		// synchronization
		vk::Fence	  in_flight_fence			= VK_NULL_HANDLE;
		vk::Semaphore image_available_semaphore = VK_NULL_HANDLE;
		vk::Semaphore render_finished_semaphore = VK_NULL_HANDLE;

		// resources
		UniformBufferObject	   camera_data;
		Buffer*				   camera_data_buffer = nullptr;
		void*				   camera_data_ptr	  = nullptr;
		std::vector<glm::mat4> object_transforms;
		Buffer*				   object_transform_buffer = nullptr;
		void*				   object_transform_ptr	   = nullptr;

		// resource descriptors
		vk::DescriptorSet vk_descriptor_set = VK_NULL_HANDLE;	 // Frame data (camera + transforms)

		void Init(Surface* surface, Scene* scene);
		void MakeDescriptorResources();
		void AllocateDescriptorResources();
		void MakeDepthResources();
		void Prepare(glm::mat4 camera_transforms);
		void Cleanup();

	  private:
		Surface* surface;	 // Pointer to the parent surface
		Device*	 device;
		Scene*	 scene;
	};

	//=========================================================================
	// CONSTRUCTOR & DESTRUCTOR
	//=========================================================================
	Surface(Instance* instance, Device* device, Window* window);
	~Surface();

	//=========================================================================
	// CORE METHODS
	//=========================================================================
	void Init();
	void InitSwapchain();
	void CleanupSwapchain();
	void Cleanup();	   // Explicit cleanup method

	void SetDevice(Device* device);

	//=========================================================================
	// RENDERING METHODS
	//=========================================================================

	void PrepareScene(vk::CommandBuffer command_buffer);
	void Render();
	void RecordDrawCommands(Frame& frame, uint32_t image_index);

	//=========================================================================
	// OBJECT PICKING METHODS
	//=========================================================================
	uint32_t PickObjectAtPosition(int mouse_x, int mouse_y);
	Scene*	 GetScene() const { return scene.get(); }

	//=========================================================================
	// CREATION METHODS
	//=========================================================================
	void CreateSwapchain();
	void RecreateSwapchain();
	void CreatePipeline();
	void CreateTextureDescriptorSet();	  // Create descriptor set for material textures
	void CreateFrameBuffers();
	void CreateCommandPool();
	void CreateFrameCommandBuffers();

	//=========================================================================
	// PUBLIC GETTERS (const methods for read-only access)
	//=========================================================================
	App*		GetApp() const { return app; }
	Instance*	GetInstance() const { return instance; }
	Device*		GetDevice() const { return device; }
	GLFWwindow* GetWindow() const { return window->GetGLFWWindow(); }

	// Vulkan objects (read-only access)
	const vk::SurfaceKHR&	GetVkSurface() const { return vk_surface; }
	const vk::SwapchainKHR& GetSwapchain() const { return vk_swapchain; }
	const vk::Pipeline&		GetPipeline() const { return vk_pipeline; }
	const vk::RenderPass&	GetRenderPass() const { return render_pass.vk_render_pass; }
	const vk::CommandPool&	GetCommandPool() const { return vk_command_pool; }

	// Swapchain information
	const SwapchainSupportDetails& GetSupportDetails() const { return support_details; }
	const vk::Extent2D&			   GetExtent() const { return extent; }
	uint32_t					   GetImageCount() const { return image_count; }
	const vk::SurfaceFormatKHR&	   GetFormat() const { return format; }
	const vk::PresentModeKHR&	   GetPresentMode() const { return present_mode; }
	const std::vector<Frame>&	   GetFrames() const { return frames; }

	//=========================================================================
	// UTILITY METHODS
	//=========================================================================
	void LogSupportDetails();

	// Format selection with variadic template support
	void SelectFormat();
	template<typename... Args>
	void SelectFormat(vk::SurfaceFormatKHR surface_format, Args... args)
	{
		app->GetLogger()->Debug("Selecting Swapchain Format...", "VKInit");
		for (auto it = support_details.formats.begin(); it <= support_details.formats.end(); it++)
		{
			if (it == support_details.formats.end())
				SelectFormat(args...);
			auto supported_format = *it;
			if (supported_format == surface_format)
			{
				app->GetLogger()->Debug("Selected Swapchain Format!", "VKInit");
				format = supported_format;
				return;
			}
		}
	}

	// Present mode selection with variadic template support
	void SelectPresentMode();
	template<typename... Args>
	void SelectPresentMode(vk::SurfacePresentModeKHR surface_present_mode, Args... args)
	{
		app->GetLogger()->Debug("Selecting Swapchain Present Mode...", "VKInit");
		for (auto it = support_details.present_modes.begin(); it <= support_details.present_modes.end(); it++)
		{
			if (it == support_details.present_modes.end())
				return SelectPresentMode(args...);
			auto supported_present_mode = *it;
			if (supported_present_mode == surface_present_mode)
			{
				present_mode = supported_present_mode;
				app->GetLogger()->Debug("Selected Swapchain Present Mode!", "VKInit");
				return;
			}
		}
	}

  private:
	//=========================================================================
	// PRIVATE MEMBER VARIABLES
	//=========================================================================

	// Core references
	App*	  app	   = nullptr;
	Instance* instance = nullptr;
	Device*	  device   = nullptr;
	Window*	  window   = nullptr;

	// Vulkan surface
	vk::SurfaceKHR vk_surface = VK_NULL_HANDLE;

	// Swapchain data
	SwapchainSupportDetails	   support_details;
	vk::SwapchainKHR		   vk_swapchain = VK_NULL_HANDLE;
	std::vector<Frame>		   frames;
	vk::Extent2D			   extent	   = vk::Extent2D(0, 0);
	uint32_t				   image_count = 0;
	vk::SurfaceFormatKHR	   format;
	vk::Format				   depth_format;
	vk::PresentModeKHR		   present_mode;
	vk::SwapchainCreateInfoKHR vk_swapchain_info;

	// Command objects
	vk::CommandPool			  vk_command_pool	= VK_NULL_HANDLE;
	vk::CommandBuffer		  vk_command_buffer = VK_NULL_HANDLE;
	vk::CommandPoolCreateInfo vk_command_pool_info;

	// Pipeline objects (integrated for performance)
	vk::Pipeline						 vk_pipeline = VK_NULL_HANDLE;
	std::vector<ShaderStage>			 shader_stages;
	VertexInputStage					 vertex_input_stage;
	InputAssemblyStage					 input_assembly_stage;
	ViewportStage						 viewport_stage;
	RasterizationStage					 rasterization_stage;
	DepthStencilStage					 depth_stencil_stage;
	MultisampleStage					 multisample_stage;
	ColorBlendStage						 color_blend_stage;
	DescriptorSetLayout					 frame_set_layout;
	DescriptorSetLayout					 texture_set_layout;	// Renamed from mesh_set_layout
	std::vector<vk::DescriptorSetLayout> vk_descriptor_set_layouts;
	DescriptorPool						 frame_descriptor_pool;
	DescriptorPool						 texture_descriptor_pool;	 // Renamed from mesh_descriptor_pool
	PipelineLayout						 pipeline_layout;
	RenderPass							 render_pass;
	vk::GraphicsPipelineCreateInfo		 vk_pipeline_info;

	// Rendering state
	size_t				   max_frames_in_flight;
	size_t				   frame_index = 0;
	std::unique_ptr<Scene> scene;
	vk::DescriptorSet	   texture_descriptor_set = VK_NULL_HANDLE;	   // Global texture descriptor set

	// Object picking
	std::unique_ptr<ObjectPicker> object_picker;

	vk::ClearValue clear_color;
	vk::ClearValue clear_depth;

	// Cleanup state
	bool is_cleaned_up = false;	   // Prevents double cleanup

	//=========================================================================
	// PRIVATE HELPER METHODS
	//=========================================================================

	friend class Scene;
	friend struct ShaderStage;
	friend struct VertexShaderStage;
	friend struct FragmentShaderStage;
	friend struct VertexInputStage;
	friend struct InputAssemblyStage;
	friend struct ViewportStage;
	friend struct RasterizationStage;
	friend struct MultisampleStage;
	friend struct ColorBlendStage;
	friend struct DescriptorSetLayout;
	friend struct DescriptorPool;
	friend struct PipelineLayout;
	friend struct RenderPass;
};

}	 // namespace nft::vulkan