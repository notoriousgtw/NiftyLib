#pragma once

//=============================================================================
// MAIN VULKAN HANDLER HEADER
//=============================================================================
// This header contains the main Vulkan handler class that manages
// the overall Vulkan system initialization and coordination.

#ifdef CreateSemaphore
#undef CreateSemaphore
#endif

#include "core/log.h"
#include "gui/window.h"
#include "vk/common.h"
#include "vk/device.h"
#include "vk/instance.h"
#include "vk/surface.h"
#include "vk/shader.h"
#include "vk/buffer.h"
#include "vk/geometry.h"
#include "vk/resources.h"
#include "vk/descriptors.h"

#include <memory>

namespace nft::vulkan
{
//=============================================================================
// VULKAN HANDLER CLASS
//=============================================================================
// Static class that manages the lifecycle of the Vulkan system.
// Coordinates Instance, Device, Surface, and global bindless resources.

class VulkanHandler
{
  public:
	//=========================================================================
	// STATIC LIFECYCLE METHODS
	//=========================================================================

	// No instance creation - static class only
	VulkanHandler()	 = delete;
	~VulkanHandler() = delete;

	// System lifecycle
	static void Init(App* app);
	static void Render();
	static void ShutDown();
	static void Cleanup(); // Add explicit cleanup method

	inline bool IsInitialized() const { return is_inititialized; }

	// Surface management
	static std::shared_ptr<Surface> AddSurface(Window* window);
	// static Surface* GetPrimarySurface();

	//=========================================================================
	// GLOBAL BINDLESS RESOURCE ACCESS
	//=========================================================================
	
	// Get the global bindless resource manager (shared across all pipelines)
	static GlobalBindlessManager* GetGlobalResourceManager() { return global_resource_manager.get(); }
	
	// Get the bindless descriptor resources (shared across all pipelines)
	static DescriptorPool* GetBindlessDescriptorPool() { return bindless_descriptor_pool.get(); }
	static DescriptorSetLayout* GetBindlessDescriptorLayout() { return bindless_descriptor_layout.get(); }

	//=========================================================================
	// APP ACCESS
	//=========================================================================
	static App* GetApp() { return app; }

	//=========================================================================
	// STATIC MEMBER VARIABLES
	//=========================================================================

	// Core application reference
	static App* app;

	// Vulkan system objects
	static std::unique_ptr<Instance>			 instance;
	static std::unique_ptr<Device>				 device;
	static std::vector<std::shared_ptr<Surface>> surfaces;
	
	// Global bindless resources (shared across all pipelines)
	static std::unique_ptr<GlobalBindlessManager>  global_resource_manager;
	static std::unique_ptr<DescriptorPool>		    bindless_descriptor_pool;
	static std::unique_ptr<DescriptorSetLayout>    bindless_descriptor_layout;

  private:
	bool is_inititialized = false;
};

}	 // namespace nft::vulkan