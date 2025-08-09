#pragma once

#include "NiftyVKCommon.h"
#include <GLFW/glfw3.h>

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
	Surface(Instance* instance, GLFWwindow* window);
	~Surface();

	void Init();

	void SetDevice(Device* device);

	void LogSwapchainSupport();

	App*					   app		= nullptr;
	Instance*				   instance = nullptr;
	Device*					   device	= nullptr;
	GLFWwindow*				   window	= nullptr;
	std::unique_ptr<Swapchain> swapchain;

	vk::SurfaceKHR		 vk_surface;
};

}	 // namespace nft::Vulkan