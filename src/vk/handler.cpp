//=============================================================================
// VULKAN HANDLER IMPLEMENTATION
//=============================================================================
// This file implements the main Vulkan handler that coordinates the
// initialization and management of Vulkan resources.
#include "vk/handler.h"

#include "core/error.h"

namespace nft::vulkan
{

//=============================================================================
// STATIC MEMBER DEFINITIONS
//=============================================================================
App*                                     VulkanHandler::app      = nullptr;
std::unique_ptr<Instance>                VulkanHandler::instance = nullptr;
std::unique_ptr<Device>                  VulkanHandler::device   = nullptr;
std::vector<std::unique_ptr<Surface>>    VulkanHandler::surfaces = {};

//=============================================================================
// INITIALIZATION METHODS
//=============================================================================

void VulkanHandler::Init(App* app)
{
    // Validate input parameters
    if (!app) 
        NFT_ERROR(VKFatal, "App Is Null!");
    
    app->GetLogger()->Debug("Initializing Vulkan Handler...", "VKInit");
    VulkanHandler::app = app;

    instance = std::make_unique<Instance>(app);
    
    device = std::make_unique<Device>(instance.get());

#if defined(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC)
    instance->InitDispatchLoaderWithDevice(device->GetDevice());
#endif
}

//=============================================================================
// RENDERING METHODS
//=============================================================================


void VulkanHandler::Render()
{
    // Validate that the system is properly initialized
    if (!app) {
        NFT_ERROR(VKFatal, "VulkanHandler not initialized - App is null!");
        return;
    }
    
    if (!device) {
        NFT_ERROR(VKFatal, "VulkanHandler not initialized - Device is null!");
        return;
    }
    
    if (surfaces.empty() || !surfaces[0]) {
        NFT_ERROR(VKFatal, "VulkanHandler not initialized - No primary surface available!");
        return;
    }

    // Render the primary surface
    try {
        surfaces[0]->Render();
    }
    catch (const std::exception& e) {
        app->GetLogger()->Error(std::format("Rendering failed: {}", e.what()), "VKRender");
    }
}

void VulkanHandler::AddSurface(GLFWwindow* window) 
{
    surfaces.push_back(std::make_unique<Surface>(instance.get(), device.get(), window));
}

//=============================================================================
// ACCESSOR METHODS
//=============================================================================

//Surface* VulkanHandler::GetPrimarySurface() 
//{
//    if (surfaces.empty()) {
//        return nullptr;
//    }
//    return surfaces[0].get();
//}

//=============================================================================
// CLEANUP METHODS
//=============================================================================

void VulkanHandler::ShutDown()
{
	device->GetDevice().waitIdle();	   // Ensure all operations are complete before cleanup

    app->GetLogger()->Debug("Cleaning Up Vulkan Resources...", "VKShutdown");

    // CRITICAL ORDER: Clean up surfaces first while device is still valid
    // Surfaces need the device to properly destroy their Vulkan objects
    for (auto& surface : surfaces)
    {
        if (surface)
        {
            surface->Cleanup(); // Explicit cleanup while device is still valid
        }
    }
    surfaces.clear();

    // Now it's safe to destroy device and instance
    device.reset();
    instance.reset();

    app->GetLogger()->Debug("Vulkan Cleanup Complete!", "VKShutdown");
}

} // namespace nft::vulkan