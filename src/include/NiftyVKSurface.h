#pragma once

#include <vulkan/vulkan.hpp>

namespace nft
{
class App;
}

namespace nft::Vulkan
{
class VKHandler;
class Instance;
class Device;

class Surface
{
  public:
	struct SwapchainSupportDetails
	{
		vk::SurfaceCapabilitiesKHR		  capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR>	  present_modes;
	};

	Surface(Instance* instance);
	~Surface();

	void Init();

	void SetDevice(Device* device);

	void QuerySwapchainSupport();
	void ChooseFormat();
	void ChoosePresentMode();

	App*	  app	   = nullptr;
	Instance* instance = nullptr;
	Device*	  device   = nullptr;

	vk::SurfaceKHR			vk_surface;
	vk::SurfaceFormatKHR	format;
	vk::PresentModeKHR		present_mode;
	SwapchainSupportDetails swapchain_support_details;
};

}	 // namespace nft::Vulkan