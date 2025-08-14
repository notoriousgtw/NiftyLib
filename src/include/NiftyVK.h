#pragma once

//=============================================================================
// MAIN VULKAN HANDLER HEADER
//=============================================================================
// This header contains the main Vulkan handler class that manages
// the overall Vulkan system initialization and coordination.

#ifdef CreateSemaphore
#undef CreateSemaphore
#endif

#include "NiftyErrorBase.h"
#include "NiftyLog.h"
#include "NiftyVKCommon.h"
#include "NiftyVKDevice.h"
#include "NiftyVKInstance.h"
#include "NiftyVKSurface.h"
#include "NiftyVKShader.h"
#include "NiftyVKRender.h"
#include "NiftyVKBuffer.h"
#include "NiftyVKGeometry.h"

#include <memory>

namespace nft::Vulkan
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

}	 // namespace nft::Vulkan