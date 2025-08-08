#pragma once

#define NOMINMAX
#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include <vulkan/vulkan.hpp>

#include "NiftyErrorBase.h"
#include "NiftyLog.h"
#include "NiftyVKInstance.h"
#include "NiftyVKDevice.h"

#include <memory>

namespace nft
{
class App;
}

namespace nft::Vulkan
{
DEFINE_ERROR(VKInitFatal, FatalError)

class VKHandler
{
  public:
	VKHandler()	 = delete;
	~VKHandler() = delete;

	static void Init(App* app);
	static void ShutDown();
	// static vk::Instance CreateInstance();

	static App*						 app;
	static std::unique_ptr<Instance> instance;
	static std::unique_ptr<Device>	 device;
};
}	 // namespace nft::Vulkan