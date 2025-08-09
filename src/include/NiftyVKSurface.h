#pragma once

#include "NiftyVKCommon.h"

namespace nft
{
class App;
}

namespace nft::Vulkan
{
class VulkanHandler;
class Instance;
class Device;
class Swapchain;

class Surface
{
  public:
	Surface(Instance* instance);
	~Surface();

	void Init();

	void SetDevice(Device* device);

	void LogSwapchainSupport();
	void ChooseFormat();
	void ChoosePresentMode();
	void ChooseSwapchainExtent(uint32_t width, uint32_t height);

	App*					   app		= nullptr;
	Instance*				   instance = nullptr;
	Device*					   device	= nullptr;
	std::unique_ptr<Swapchain> swapchain;

	vk::SurfaceKHR		 vk_surface;
	vk::SurfaceFormatKHR format;
	vk::PresentModeKHR	 present_mode;
};

}	 // namespace nft::Vulkan