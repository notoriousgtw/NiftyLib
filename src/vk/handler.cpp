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
App*								  VulkanHandler::app	  = nullptr;
std::unique_ptr<Instance>			  VulkanHandler::instance = nullptr;
std::unique_ptr<Device>				  VulkanHandler::device	  = nullptr;
std::vector<std::shared_ptr<Surface>> VulkanHandler::surfaces = {};

// Global bindless resources
std::unique_ptr<GlobalBindlessManager>  VulkanHandler::global_resource_manager = nullptr;
std::unique_ptr<DescriptorPool>		    VulkanHandler::bindless_descriptor_pool = nullptr;
std::unique_ptr<DescriptorSetLayout>    VulkanHandler::bindless_descriptor_layout = nullptr;

//=============================================================================
// INITIALIZATION METHODS
//=============================================================================

void VulkanHandler::Init(App* app)
{
	// Validate input parameters
	if (!app)
		NFT_ERROR(VulkanFatal, "App Is Null!");

	app->GetLogger()->Debug("Initializing Vulkan Handler...", "VKInit");
	VulkanHandler::app = app;

	instance = std::make_unique<Instance>(app);

	device = std::make_unique<Device>(instance.get());

#if defined(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC)
	instance->InitDispatchLoaderWithDevice(device->GetDevice());
#endif

	// Initialize global bindless resources after device is ready
	app->GetLogger()->Debug("Initializing global bindless resources...", "VKInit");
	
	try
	{
		// Setup bindless system with support for up to MAX_FRAMES frames in flight
		SetupBindlessVertexBufferSystem(
			device.get(), 
			MAX_FRAMES,
			bindless_descriptor_pool,
			bindless_descriptor_layout,
			global_resource_manager
		);
		
		app->GetLogger()->Debug("Global bindless resources initialized successfully!", "VKInit");
	}
	catch (const std::exception& e)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed to initialize global bindless resources: {}", e.what()));
	}
}

//=============================================================================
// RENDERING METHODS
//=============================================================================

void VulkanHandler::Render()
{
	// Validate that the system is properly initialized
	if (!app)
		NFT_ERROR(VulkanFatal, "VulkanHandler not initialized - App is null!");

	if (!device)
		NFT_ERROR(VulkanFatal, "VulkanHandler not initialized - Device is null!");

	if (surfaces.empty() || !surfaces[0])
		NFT_ERROR(VulkanFatal, "VulkanHandler not initialized - No surface available!");

	// Render the primary surface
	try
	{
		//surfaces[0]->Render();
	}
	catch (const std::exception& e)
	{
		app->GetLogger()->Error(std::format("Rendering failed: {}", e.what()), "VKRender");
	}
}

std::shared_ptr<Surface> VulkanHandler::AddSurface(Window* window)
{
	surfaces.push_back(std::make_unique<Surface>(instance.get(), device.get(), window));
	return surfaces.back();
}

//=============================================================================
// CLEANUP METHODS
//=============================================================================

void VulkanHandler::ShutDown()
{
	// CRITICAL FIX: Use try-catch to handle potential device lost errors during cleanup
	try {
		if (device)
			device->GetDevice().waitIdle();   // Ensure all operations are complete before cleanup
		
		if (app && app->GetLogger())
			app->GetLogger()->Debug("Device idle wait completed successfully", "VKShutdown");
	}
	catch (const vk::SystemError& e) {
		// Device lost errors are expected during shutdown in some cases
		if (app && app->GetLogger()) {
			app->GetLogger()->Warn(std::format("Failed to safely cleanup renderer resources: {}", e.what()), "VKShutdown");
		}
		// Continue with cleanup anyway - we need to release resources even if device is lost
	}

	if (app && app->GetLogger())
		app->GetLogger()->Debug("Cleaning Up Vulkan Resources...", "VKShutdown");

	// Clear window surface references to prevent double cleanup
	for (auto& surface : surfaces)
	{
		if (surface && surface->GetWindow())
		{
			surface->GetWindow()->ClearSurface();
		}
	}

	// Clean up global bindless resources first
	if (global_resource_manager)
	{
		global_resource_manager.reset();
		if (app && app->GetLogger())
			app->GetLogger()->Debug("Global resource manager destroyed", "VKShutdown");
	}
	
	if (bindless_descriptor_layout)
	{
		bindless_descriptor_layout.reset();
		if (app && app->GetLogger())
			app->GetLogger()->Debug("Bindless descriptor layout destroyed", "VKShutdown");
	}
	
	if (bindless_descriptor_pool)
	{
		bindless_descriptor_pool.reset();
		if (app && app->GetLogger())
			app->GetLogger()->Debug("Bindless descriptor pool destroyed", "VKShutdown");
	}

	// CRITICAL ORDER: Clean up surfaces first while device is still valid
	// Surfaces need the device to properly destroy their Vulkan objects
	for (auto& surface : surfaces)
		if (surface)
			surface->Cleanup();
	surfaces.clear();

	// Now safely clean up device and instance
	if (device)
	{
		device.reset();
		if (app && app->GetLogger())
			app->GetLogger()->Debug("Device destroyed successfully", "VKShutdown");
	}

	if (instance)
	{
		instance.reset();
		if (app && app->GetLogger())
			app->GetLogger()->Debug("Instance destroyed successfully", "VKShutdown");
	}

	if (app && app->GetLogger())
		app->GetLogger()->Debug("Vulkan Resources Cleaned Up Successfully!", "VKShutdown");
}

void VulkanHandler::Cleanup()
{
	// Alias for ShutDown for consistency
	ShutDown();
}

}	 // namespace nft::vulkan