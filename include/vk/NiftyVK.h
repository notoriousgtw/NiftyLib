#pragma once

//=============================================================================
// MAIN VULKAN HANDLER HEADER
//=============================================================================
// This header contains the main Vulkan handler class that manages
// the overall Vulkan system initialization and coordination.

#ifdef CreateSemaphore
#undef CreateSemaphore
#endif

#include "core/NiftyLog.h"
#include "vk/NiftyVKCommon.h"
#include "vk/NiftyVKDevice.h"
#include "vk/NiftyVKInstance.h"
#include "vk/NiftyVKSurface.h"
#include "vk/NiftyVKShader.h"
#include "vk/NiftyVKRender.h"
#include "vk/NiftyVKBuffer.h"
#include "vk/NiftyVKGeometry.h"

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

	// Surface management
	static void AddSurface(GLFWwindow* window);
	// static Surface* GetPrimarySurface();

	//=========================================================================
	// STATIC MEMBER VARIABLES
	//=========================================================================

	// Core application reference
	static App* app;

	// Vulkan system objects
	static std::unique_ptr<Instance>			 instance;
	static std::unique_ptr<Device>				 device;
	static std::vector<std::unique_ptr<Surface>> surfaces;
};

}	 // namespace nft::vulkan