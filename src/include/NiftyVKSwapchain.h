#pragma once

#include "NiftyVKCommon.h"
#include <GLFW/glfw3.h>

namespace nft
{
class App;
}

namespace nft::Vulkan
{
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

	struct Frame
	{
		vk::Image				image	   = nullptr;
		vk::ImageView			image_view = nullptr;
		vk::ImageViewCreateInfo create_info;

		//bool IsComplete() const { return (!image || !image_view) ? false : true; }
	};

	Swapchain(Surface* surface);
	~Swapchain();
	void			 Init();
	void			 CreateSwapchain();
	void			 CreateImageViews() {};
	void			 CreateFramebuffers() {};
	Surface*		 GetSurface() { return surface; }
	vk::SwapchainKHR GetSwapchain() { return vk_swapchain; }

	void LogSupportDetails();

	App*	  app	   = nullptr;
	Surface*  surface  = nullptr;
	Instance* instance = nullptr;
	Device*	  device   = nullptr;

	SwapchainSupportDetails support_details;

	vk::SwapchainKHR vk_swapchain;

	std::vector<Frame> images;
	// std::vector<vk::ImageView>	 image_views;
	// std::vector<vk::Framebuffer> framebuffers;
	vk::Extent2D		 extent		 = vk::Extent2D(0, 0);
	uint32_t			 image_count = 0;
	vk::SurfaceFormatKHR format;
	vk::PresentModeKHR	 present_mode;

	vk::SwapchainCreateInfoKHR create_info;
};
}	 // namespace nft::Vulkan
