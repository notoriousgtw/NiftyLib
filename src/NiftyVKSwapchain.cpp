#include "NiftyVKSwapchain.h"

#include "NiftyApp.h"
#include "NiftyError.h"
#include "NiftyVK.h"

namespace nft::Vulkan
{
Swapchain::Swapchain(Surface* surface): surface(surface)
{
	if (!surface)
		ErrorHandler::Error<VKInitFatal>("Surface is null!", __func__);
	this->instance = surface->instance;
	this->device   = surface->device;
	app			   = this->instance->app;
	Init();
}

Swapchain::~Swapchain() {}

void Swapchain::Init()
{
	support_details.capabilities =
		device->vk_physical_device.getSurfaceCapabilitiesKHR(surface->vk_surface, instance->dispatch_loader_dynamic);
	support_details.formats =
		device->vk_physical_device.getSurfaceFormatsKHR(surface->vk_surface, instance->dispatch_loader_dynamic);
	support_details.present_modes =
		device->vk_physical_device.getSurfacePresentModesKHR(surface->vk_surface, instance->dispatch_loader_dynamic);
}
void Swapchain::CreateSwapchain(uint32_t width, uint32_t height)
{
	if (support_details.capabilities.currentExtent.width != UINT32_MAX)
	{
		extent.width  = support_details.capabilities.currentExtent.width;
		extent.height = support_details.capabilities.currentExtent.height;
	}
	else
	{
		extent.width  = std::min(support_details.capabilities.maxImageExtent.width,
								 std::max(support_details.capabilities.minImageExtent.width, width));
		extent.height = std::min(support_details.capabilities.maxImageExtent.height,
								 std::max(support_details.capabilities.minImageExtent.height, height));
	}
}
}	 // namespace nft::Vulkan