#include "NiftyVK.h"

#include "NiftyError.h"

namespace nft::Vulkan
{
DECLARE_ERROR_CODE(VKInitFatal, "VKInitFatal")

App*					  VKHandler::app	  = nullptr;
std::unique_ptr<Instance> VKHandler::instance = nullptr;
std::unique_ptr<Device> VKHandler::device = nullptr;

void VKHandler::Init(App* app)
{
	if (!app) ErrorHandler::Error<VKInitFatal>("App Is Null!", __func__);
	app->GetLogger()->Debug("Initializing Vulkan Handler...", "VKInit");
	VKHandler::app = app;
	// Create Vulkan instance
	instance = std::make_unique<Instance>(app);
	device	 = std::make_unique<Device>(app, instance.get());
}
void VKHandler::ShutDown()
{
	app->GetLogger()->Debug("Destroying Vulkan Instance...", "VKShutdown");
	instance->vk_instance.destroyDebugUtilsMessengerEXT(
		instance->debug_messenger, nullptr, instance->dispatch_loader_dynamic);
	instance->vk_instance.destroy();
}
}	 // namespace nft::Vulkan