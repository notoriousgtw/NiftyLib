#include "NiftyVKSurface.h"

#include "GLFW/glfw3.h"

#include "NiftyApp.h"
#include "NiftyError.h"
#include "NiftyVK.h"

namespace nft::Vulkan
{
Surface::Surface(Instance* instance, GLFWwindow* window): instance(instance), window(window)
{
	if (!instance)
		NFT_ERROR(VKInitFatal, "Instance is null!");
	if (!window)
		NFT_ERROR(VKInitFatal, "Window is null!");
	app = this->instance->app;
	Init();
}
Surface::~Surface()
{
	instance->vk_instance.destroySurfaceKHR(vk_surface, nullptr, instance->dispatch_loader_dynamic);
}

void Surface::SetDevice(Device* device)
{
	this->device = device;
	device->swapchains.push_back(std::make_unique<Swapchain>(this));
}

void Surface::Init()
{
	app->GetLogger()->Debug(std::format("Creating Surface For Window: \"{}\"...", glfwGetWindowTitle(window)), "VKInit");
	// Create a Vulkan surface using GLFW
	VkSurfaceKHR c_style_surface;
	glfwCreateWindowSurface(instance->vk_instance, app->GetMainWindow()->GetGLFWWindow(), nullptr, &c_style_surface);
	vk_surface = c_style_surface;
	app->GetLogger()->Debug(std::format("Surface For Window: \"{}\" Created Successfully!", glfwGetWindowTitle(window)), "VKInit");
}
}	 // namespace nft::Vulkan