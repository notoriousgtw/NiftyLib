#pragma once

#include "NiftyVKCommon.h"

#include "NiftyErrorBase.h"
#include "NiftyLog.h"
#include "NiftyVKInstance.h"
#include "NiftyVKDevice.h"
#include "NiftyVKSurface.h"
#include "NiftyVKSwapchain.h"

#include <memory>

namespace nft
{
class App;
}

namespace nft::Vulkan
{
DEFINE_ERROR(VKInitFatal, FatalError)

class VulkanHandler
{
  public:
	VulkanHandler()	 = delete;
	~VulkanHandler() = delete;

	static void Init(App* app);
	static void ShutDown();
	// static vk::Instance CreateInstance();

	static App*						 app;
	static std::unique_ptr<Instance> instance;
	static std::unique_ptr<Device>	 device;
};
}	 // namespace nft::Vulkan