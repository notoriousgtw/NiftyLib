#include "NiftyVK.h"

#include "NiftyError.h"

namespace nft::Vulkan
{

App*					  VulkanHandler::app	  = nullptr;
std::unique_ptr<Instance> VulkanHandler::instance = nullptr;
std::unique_ptr<Device>	  VulkanHandler::device	  = nullptr;

void VulkanHandler::Init(App* app)
{
	if (!app) NFT_ERROR(VKInitFatal, "App Is Null!");
	app->GetLogger()->Debug("Initializing Vulkan Handler...", "VKInit");
	VulkanHandler::app = app;

	// Create Vulkan Instance
	instance = std::make_unique<Instance>(app);
	
	// Create Vulkan Surface
	instance->AddSurface();

	// Create Vulkan Logical Device
	device	 = std::make_unique<Device>(instance.get());

	instance->dispatch_loader_dynamic.init(instance->vk_instance, device->vk_device);

	instance->surfaces.at(0)->SetDevice(device.get());
}
void VulkanHandler::ShutDown()
{
	app->GetLogger()->Debug("Cleaning Up Vulkan Resources...", "VKShutdown");

	device.reset();
	instance.reset();

	app->GetLogger()->Debug("Vulkan Cleanup Complete!", "VKShutdown");
}
}	 // namespace nft::Vulkan