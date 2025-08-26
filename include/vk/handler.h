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
#include "vk/scene.h"
#include "vk/buffer.h"
#include "vk/geometry.h"

#include <memory>

namespace nft::vulkan
{
//=============================================================================
// VULKAN HANDLER CLASS
//=============================================================================
// Static class that manages the lifecycle of the Vulkan system.
// Coordinates Instance, Device, and Surface objects.

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

	inline bool IsInitialized() const { return is_inititialized; }

	// Surface management
	static std::shared_ptr<Surface> AddSurface(Window* window);
	// static Surface* GetPrimarySurface();

	//=========================================================================
	// STATIC MEMBER VARIABLES
	//=========================================================================

	// Core application reference
	static App* app;

	// Vulkan system objects
	static std::unique_ptr<Instance>			 instance;
	static std::unique_ptr<Device>				 device;
	static std::vector<std::shared_ptr<Surface>> surfaces;
  private:
	bool is_inititialized = false;
};

}	 // namespace nft::vulkan