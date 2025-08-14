//=============================================================================
// VULKAN INSTANCE IMPLEMENTATION
//=============================================================================
// This file implements the Vulkan instance management, including extension
// and layer validation, debug messenger setup, and instance creation.

#include "NiftyVKInstance.h"
#include "NiftyApp.h"
#include "NiftyError.h"
#include "NiftyLog.h"
#include "NiftyVK.h"

#include <vector>
#include <GLFW/glfw3.h>

namespace nft::Vulkan
{

//=============================================================================
// STATIC MEMBER DEFINITIONS
//=============================================================================
vk::detail::DynamicLoader Instance::dynamic_loader;

//=============================================================================
// CONSTRUCTOR & DESTRUCTOR
//=============================================================================

Instance::Instance(App* app) : app(app)
{
	Init();
	GetExtensions();
	GetLayers();
	CheckSupported();
	CreateInstance();
}

Instance::~Instance()
{
	app->GetLogger()->Debug("Cleaning Up Instance...", "VKShutdown");

	// Cleanup debug messenger if in debug mode
#ifdef _DEBUG
		vk_instance.destroyDebugUtilsMessengerEXT(
			vk_debug_messenger, nullptr, dispatch_loader_dynamic);
#endif

	// Cleanup instance
	vk_instance.destroy(nullptr, dispatch_loader_dynamic);
}

//=============================================================================
// INITIALIZATION METHODS
//=============================================================================

void Instance::Init()
{
	// Initialize dynamic loader
	dispatch_loader_dynamic.init(vkGetInstanceProcAddr);
	
	app->GetLogger()->Debug("Creating Instance...", "VKInit");

	// Get and log Vulkan version
	uint32_t version = 0;
	vkEnumerateInstanceVersion(&version);
	
	std::string version_str = std::to_string(VK_API_VERSION_MAJOR(version)) + "." +
							  std::to_string(VK_API_VERSION_MINOR(version)) + "." +
							  std::to_string(VK_API_VERSION_PATCH(version));
	app->GetLogger()->Debug(std::format("Vulkan Version: {}", version_str), "VKInit");

	// Clear patch version bits for compatibility
	version &= ~(0xFFFU);

	// Create application info structure
	vk_app_info = vk::ApplicationInfo(
		app->GetName().c_str(), 1, 
		"Nifty Engine", 1, 
		version);
}

void Instance::GetExtensions()
{
	// Get required GLFW extensions
	uint32_t extension_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extension_count);

	extensions = std::vector<const char*>(glfw_extensions, glfw_extensions + extension_count);

	// Log required extensions
	app->GetLogger()->Debug("GLFW Extensions Required:", "VKInit");
	for (const auto& extension : extensions)
	{
		app->GetLogger()->Debug(extension,
								"",
								Log::Flags::Default & ~Log::Flags::ShowHeader,
								0, 4);
	}

	// Add debug extensions in debug builds
#ifdef _DEBUG
	extensions.push_back("VK_EXT_debug_utils");
#endif
}

void Instance::GetLayers() 
{
	// Add validation layers in debug builds
#ifdef _DEBUG
		layers.push_back("VK_LAYER_KHRONOS_validation");
#endif
}

void Instance::CheckSupported()
{
	// Check extension support
	std::vector<vk::ExtensionProperties> supported_extensions =
		vk::enumerateInstanceExtensionProperties(nullptr, dispatch_loader_dynamic);

	app->GetLogger()->Debug("Supported Extensions:", "VKInit");
	for (const vk::ExtensionProperties& extension : supported_extensions)
	{
		app->GetLogger()->Debug(extension.extensionName.data(),
								"",
								Log::Flags::Default & ~Log::Flags::ShowHeader,
								0, 4);
	}

	// Validate required extensions
	bool found;
	for (const char* extension : extensions)
	{
		found = false;
		for (const vk::ExtensionProperties& supported_extension : supported_extensions)
		{
			if (strcmp(extension, supported_extension.extensionName.data()) == 0)
			{
				found = true;
				app->GetLogger()->Debug(std::format("Extension {} is supported!", extension), "VKInit");
			}
		}
		if (!found)
		{
			NFT_ERROR(VKFatal, std::format("Instance Extension {} Is Not Supported!", extension));
		}
	}

	// Check layer support
	std::vector<vk::LayerProperties> supported_layers = 
		vk::enumerateInstanceLayerProperties(dispatch_loader_dynamic);

	app->GetLogger()->Debug("Supported Layers:", "VKInit");
	for (const vk::LayerProperties& layer : supported_layers)
	{
		app->GetLogger()->Debug(layer.layerName.data(),
								"",
								Log::Flags::Default & ~Log::Flags::ShowHeader,
								0, 4);
	}

	// Validate required layers
	for (const char* layer : layers)
	{
		found = false;
		for (const vk::LayerProperties& supported_layer : supported_layers)
		{
			if (strcmp(layer, supported_layer.layerName.data()) == 0)
			{
				found = true;
				app->GetLogger()->Debug(std::format("Layer {} Is Supported!", layer), "VKInit");
			}
		}
		if (!found)
		{
			NFT_ERROR(VKFatal, std::format("Instance Layer {} Is Not Supported!", layer));
		}
	}
}

void Instance::CreateInstance()
{
	// Create instance info structure
	vk_instance_info = vk::InstanceCreateInfo()
						   .setFlags(vk::InstanceCreateFlags())
						   .setPApplicationInfo(&vk_app_info)
						   .setEnabledLayerCount(layers.size())
						   .setPpEnabledLayerNames(layers.data())
						   .setEnabledExtensionCount(extensions.size())
						   .setPpEnabledExtensionNames(extensions.data());

	// Create the Vulkan instance
	try
	{
		vk_instance = vk::createInstance(vk_instance_info, nullptr, dispatch_loader_dynamic);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Create Instance:\n{}", err.what()));
	}
	
	// Update dispatch loader with instance
	dispatch_loader_dynamic = vk::detail::DispatchLoaderDynamic(vk_instance, vkGetInstanceProcAddr);

	// Setup debug messenger in debug builds
#ifdef _DEBUG
		SetupDebugMessenger();
#endif

	app->GetLogger()->Debug("Instance Created Successfully!", "VKInit");
}

//=============================================================================
// DEBUG METHODS
//=============================================================================

VKAPI_ATTR VkBool32 VKAPI_CALL
	Instance::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT		message_severity,
							VkDebugUtilsMessageTypeFlagsEXT				message_type,
							const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
							void*										p_user_data)
{
	App* app = reinterpret_cast<Instance*>(p_user_data)->app;

	// Route messages to appropriate log levels
	if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		app->GetLogger()->Error(p_callback_data->pMessage, "VKDebug");
	}
	else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		app->GetLogger()->Warn(p_callback_data->pMessage, "VKDebug");
	}
	else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		app->GetLogger()->Info(p_callback_data->pMessage, "VKDebug");
	}
	else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
	{
		app->GetLogger()->Debug(p_callback_data->pMessage, "VKDebug");
	}
	
	return VK_FALSE;
}

void Instance::SetupDebugMessenger()
{
	// Configure debug messenger
	vk_debug_messenger_info =
		vk::DebugUtilsMessengerCreateInfoEXT()
			.setFlags(vk::DebugUtilsMessengerCreateFlagsEXT())
			.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
								vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
								vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
			.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
							vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
							vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
			.setPfnUserCallback(&Instance::DebugCallback)
			.setPUserData(this);

	// Create debug messenger
	try
	{
		vk_debug_messenger = vk_instance.createDebugUtilsMessengerEXT(
			vk_debug_messenger_info, nullptr, dispatch_loader_dynamic);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Setup Debug Messenger:\n{}", err.what()));
	}
}

} // namespace nft::Vulkan