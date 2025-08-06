#include "NiftyVKInstance.h"

#include "NiftyApp.h"
#include "NiftyError.h"
#include "NiftyLog.h"
#include "NiftyVK.h"

#include <vector>

namespace nft::Vulkan
{
Instance::Instance(App* app): app(app)
{
	uint32_t version = 0;
	app->GetLogger()->Debug("Creating Vulkan Instance...", "VKInit");
	vkEnumerateInstanceVersion(&version);
	std::string version_str = std::to_string(VK_API_VERSION_MAJOR(version)) + "." +
							  std::to_string(VK_API_VERSION_MINOR(version)) + "." +
							  std::to_string(VK_API_VERSION_PATCH(version));
	app->GetLogger()->Debug(std::format("Vulkan Version: {}", version_str), "VKInit");

	version &= ~(0xFFFU);	 // Clear patch version bits for compatibility

	// version = VK_MAKE_API_VERSION(0, 1, 0, 0); // Or use version 1 for more device compatibility

	app_info = vk::ApplicationInfo(app->GetName().c_str(), 1, "Nifty Engine", 1, version);

	uint32_t	 extension_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extension_count);

	extensions = std::vector<const char*>(glfw_extensions, glfw_extensions + extension_count);

	app->GetLogger()->Debug("GLFW Extensions Required:", "VKInit");
	for (const auto& extension : extensions)
		app->GetLogger()->Debug(
			std::format("\"{}\"", extension), "", Log::Flags::Default & ~Log::Flags::ShowHeader, 0, 4);

#ifdef _DEBUG
	layers.push_back("VK_LAYER_KHRONOS_validation");
	extensions.push_back("VK_EXT_debug_utils");
#endif

	if (!CheckSupported())
		ErrorHandler::Error<VKInitFatal>("Vulkan Instance Extensions Not Supported!", __func__);

	create_info = vk::InstanceCreateInfo()
					  .setFlags(vk::InstanceCreateFlags())
					  .setPApplicationInfo(&app_info)
					  .setEnabledLayerCount(layers.size())
					  .setPpEnabledLayerNames(layers.data())
					  .setEnabledExtensionCount(extensions.size())
					  .setPpEnabledExtensionNames(extensions.data());

	try
	{
		vk_instance = vk::createInstance(create_info, nullptr);
	}
	catch (const vk::SystemError err)
	{
		ErrorHandler::Error<VKInitFatal>("Failed To Create Vulkan Instance!", __func__);
	}

#ifdef _DEBUG
	SetupDebugMessenger();
#endif
	app->GetLogger()->Debug("Vulkan Instance Created Successfully!", "VKInit");
}
bool Instance::CheckSupported()
{
	std::vector<vk::ExtensionProperties> supported_extensions =
		vk::enumerateInstanceExtensionProperties();

	app->GetLogger()->Debug("Supported Extensions:", "VKInit");
	for (vk::ExtensionProperties extension : supported_extensions)
	{
		app->GetLogger()->Debug(std::format("\"{}\"", extension.extensionName.data()),
								"",
								Log::Flags::Default & ~Log::Flags::ShowHeader,
								0,
								4);
	}

	bool found;
	for (const char* extension : extensions)
	{
		found = false;
		for (vk::ExtensionProperties supported_extension : supported_extensions)
		{
			if (strcmp(extension, supported_extension.extensionName.data()) == 0)
			{
				found = true;
				app->GetLogger()->Debug(std::format("Extension {} is supported!", extension),
										"VKInit");
			}
		}
		if (!found)
		{
			app->GetLogger()->Debug(std::format("Extension {} is not supported!", extension),
									"VKInit");
			return false;
		}
	}

	std::vector<vk::LayerProperties> supported_layers = vk::enumerateInstanceLayerProperties();

	app->GetLogger()->Debug("Supported Layers:", "VKInit");
	for (vk::LayerProperties layer : supported_layers)
	{
		app->GetLogger()->Debug(std::format("\"{}\"", layer.layerName.data()),
								"",
								Log::Flags::Default & ~Log::Flags::ShowHeader,
								0,
								4);
	}

	for (const char* layer : layers)
	{
		found = false;
		for (vk::LayerProperties supported_layer : supported_layers)
		{
			if (strcmp(layer, supported_layer.layerName.data()) == 0)
			{
				found = true;
				app->GetLogger()->Debug(std::format("Layer {} is supported!", layer), "VKInit");
			}
		}
		if (!found)
		{
			app->GetLogger()->Debug(std::format("Layer {} is not supported!", layer), "VKInit");
			return false;
		}
	}

	return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
	Instance::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT		message_severity,
							VkDebugUtilsMessageTypeFlagsEXT				message_type,
							const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
							void*										p_user_data)
{
	App* app = reinterpret_cast<Instance*>(p_user_data)->app;

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
	dispatch_loader_dynamic = vk::DispatchLoaderDynamic(vk_instance, vkGetInstanceProcAddr);
	// debug_messenger_create_info = vk::DebugUtilsMessengerCreateInfoEXT(
	//	vk::DebugUtilsMessengerCreateFlagsEXT(),
	//	vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
	//		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
	//		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
	//	vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
	//		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
	//		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
	//	DebugCallback,
	//	nullptr);

	debug_messenger_create_info =
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

	try
	{
		debug_messenger = vk_instance.createDebugUtilsMessengerEXT(
			debug_messenger_create_info, nullptr, dispatch_loader_dynamic);
	}
	catch (const vk::SystemError err)
	{
		ErrorHandler::Error<VKInitFatal>("Failed To Setup Vulkan Debug Messenger!", __func__);
	}
}
}	 // namespace nft::Vulkan