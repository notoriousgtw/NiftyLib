#pragma once

// Core includes
#include "core/app.h"
#include "core/error.h"
#include "core/glfw_common.h"
#include "gui/window.h"
#include "vk/common.h"
#include "vk/shader.h"
#include "vk/util.h"

namespace nft::vulkan
{
	// Forward declarations
	class Instance;
	class Device;
	class AbstractPipeline;
	class GraphicsPipeline;
	struct Frame; // Now defined in pipeline.h

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
		// RENDERING METHODS (now delegated to pipeline)
		//=========================================================================
		void Render();

		//=========================================================================
		// OBJECT PICKING METHODS (COMMENTED OUT FOR NOW)
		//=========================================================================
		// uint32_t PickObjectAtPosition(int mouse_x, int mouse_y);
		Scene*	 GetScene() const { return scene.get(); }

		//=========================================================================
		// CREATION METHODS
		//=========================================================================
		void CreateSwapchain();
		void RecreateSwapchain();
		void CreateFrameBuffers();
		void CreateCommandPool();
		void CreateFrameCommandBuffers();

		//=========================================================================
		// PUBLIC GETTERS (const methods for read-only access)
		//=========================================================================
		App*		GetApp() const { return app; }
		Instance*	GetInstance() const { return instance; }
		Device*		GetDevice() const { return device; }
		Window*		GetWindow() const { return window; }
		GLFWwindow* GetGLFWWindow() const { return window->GetGLFWWindow(); }

		// Vulkan objects (read-only access)
		const vk::SurfaceKHR&	GetVkSurface() const { return vk_surface; }
		const vk::SwapchainKHR& GetSwapchain() const { return vk_swapchain; }
		const vk::CommandPool& GetCommandPool() const { return vk_command_pool; }

		// Swapchain information
		const SwapchainSupportDetails& GetSupportDetails() const { return support_details; }
		const vk::Extent2D&			   GetExtent() const { return extent; }
		uint32_t					   GetImageCount() const { return image_count; }
		const vk::SurfaceFormatKHR&	   GetFormat() const { return format; }
		const vk::PresentModeKHR&	   GetPresentMode() const { return present_mode; }
		const std::vector<Frame>&	   GetFrames() const { return frames; }

		//=========================================================================
		// PIPELINE ACCESS
		//=========================================================================
		GraphicsPipeline* GetPipeline() const { return graphics_pipeline.get(); }

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
		std::vector<Frame>		   frames;  // Frame struct now defined in pipeline.h
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

		// Rendering state
		size_t				   max_frames_in_flight;
		size_t				   frame_index = 0;
		std::unique_ptr<Scene> scene;

		// Pipeline (composition over inheritance)
		std::unique_ptr<GraphicsPipeline> graphics_pipeline;

		// Cleanup state
		bool is_cleaned_up = false;	   // Prevents double cleanup

		//=========================================================================
		// PRIVATE HELPER METHODS
		//=========================================================================
		void CreatePipeline();  // Creates the graphics pipeline instance
	};

}	 // namespace nft::vulkan