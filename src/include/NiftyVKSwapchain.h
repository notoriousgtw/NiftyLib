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
class Surface;

class Swapchain
{
  public:
	struct SwapchainSupportDetails
	{
		vk::SurfaceCapabilitiesKHR		  capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR>	  present_modes;
	};

	Swapchain(Surface* surface);
	~Swapchain();
	void			 Init();
	void			 CreateSwapchain(uint32_t width, uint32_t height);
	void			 CreateImageViews() {};
	void			 CreateFramebuffers() {};
	Surface*		 GetSurface() { return surface; }
	vk::SwapchainKHR GetSwapchain() { return vk_swapchain; }

	App*	  app	   = nullptr;
	Surface*  surface  = nullptr;
	Instance* instance = nullptr;
	Device*	  device   = nullptr;

	SwapchainSupportDetails support_details;

	vk::SwapchainKHR vk_swapchain;

	std::vector<vk::Image> images;
	// std::vector<vk::ImageView>	 image_views;
	// std::vector<vk::Framebuffer> framebuffers;
	vk::Format	 format;
	vk::Extent2D extent;
};
}	 // namespace nft::Vulkan
