#pragma once

#include "NiftyVKCommon.h"

#include "NiftyErrorBase.h"
#include "NiftyLog.h"
#include "NiftyVKDevice.h"
#include "NiftyVKInstance.h"
#include "NiftyVKSurface.h"
#include "NiftyVKSwapchain.h"

#include <memory>

namespace nft
{
class App;
}

namespace nft::Vulkan
{
struct VKInitFatal: public FatalError<VKInitFatal>
{
	VKInitFatal(std::string message, std::string function_name = ""): FatalError(std::move(message), std::move(function_name)) {};
};

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