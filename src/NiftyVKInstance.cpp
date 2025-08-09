#include "NiftyVKInstance.h"

#include "NiftyApp.h"
#include "NiftyError.h"
#include "NiftyLog.h"
#include "NiftyVK.h"

#include <vector>
#include <GLFW/glfw3.h>

namespace nft::Vulkan
{

vk::detail::DynamicLoader Instance::dynamic_loader;

Instance::Instance(App* app): app(app)
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

	for (auto& surface : surfaces)
		surface.reset();

#ifdef _DEBUG
	vk_instance.destroyDebugUtilsMessengerEXT(
		vk_debug_messenger, nullptr, dispatch_loader_dynamic);
#endif

	vk_instance.destroy(nullptr, dispatch_loader_dynamic);
}

void Instance::AddSurface() {
	surfaces.push_back(std::make_unique<Surface>(this, app->GetMainWindow()->GetGLFWWindow()));
}

void Instance::Init()
{
	dispatch_loader_dynamic.init(vkGetInstanceProcAddr);
	uint32_t version = 0;
	app->GetLogger()->Debug("Creating Instance...", "VKInit");

	vkEnumerateInstanceVersion(&version);
	std::string version_str = std::to_string(VK_API_VERSION_MAJOR(version)) + "." +
							  std::to_string(VK_API_VERSION_MINOR(version)) + "." +
							  std::to_string(VK_API_VERSION_PATCH(version));
	app->GetLogger()->Debug(std::format("Vulkan Version: {}", version_str), "VKInit");

	version &= ~(0xFFFU);	 // Clear patch version bits for compatibility

	// version = VK_MAKE_API_VERSION(0, 1, 0, 0); // Or use version 1 for more device compatibility

	app_info = vk::ApplicationInfo(app->GetName().c_str(), 1, "Nifty Engine", 1, version);
}

void Instance::GetExtensions()
{
	uint32_t	 extension_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extension_count);

	extensions = std::vector<const char*>(glfw_extensions, glfw_extensions + extension_count);

	app->GetLogger()->Debug("GLFW Extensions Required:", "VKInit");
	for (const auto& extension : extensions)
		app->GetLogger()->Debug(extension,
								"",
								Log::Flags::Default & ~Log::Flags::ShowHeader,
								0,
								4);

#ifdef _DEBUG
	extensions.push_back("VK_EXT_debug_utils");
#endif
}

void Instance::GetLayers() {
#ifdef _DEBUG
	layers.push_back("VK_LAYER_KHRONOS_validation");
#endif
}

void Instance::CheckSupported()
{
	std::vector<vk::ExtensionProperties> supported_extensions =
		vk::enumerateInstanceExtensionProperties(nullptr, dispatch_loader_dynamic);

	app->GetLogger()->Debug("Supported Extensions:", "VKInit");
	for (vk::ExtensionProperties extension : supported_extensions)
	{
		app->GetLogger()->Debug(extension.extensionName.data(),
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
			ErrorHandler::Error<VKInitFatal>(std::format("Instance Extension {} Is Not Supported!", extension), __func__);
		}
	}

	std::vector<vk::LayerProperties> supported_layers = vk::enumerateInstanceLayerProperties(dispatch_loader_dynamic);

	app->GetLogger()->Debug("Supported Layers:", "VKInit");
	for (vk::LayerProperties layer : supported_layers)
	{
		app->GetLogger()->Debug(layer.layerName.data(),
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
				app->GetLogger()->Debug(std::format("Layer {} Is Supported!", layer), "VKInit");
			}
		}
		if (!found)
		{
			ErrorHandler::Error<VKInitFatal>(std::format("Instance Layer {} Is Not Supported!", layer), __func__);
		}
	}
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
		vk_debug_messenger = vk_instance.createDebugUtilsMessengerEXT(
			debug_messenger_create_info, nullptr, dispatch_loader_dynamic);
	}
	catch (const vk::SystemError err)
	{
		ErrorHandler::Error<VKInitFatal>("Failed To Setup Debug Messenger!", __func__);
	}
}

void Instance::CreateInstance()
{
	instance_create_info = vk::InstanceCreateInfo()
							   .setFlags(vk::InstanceCreateFlags())
							   .setPApplicationInfo(&app_info)
							   .setEnabledLayerCount(layers.size())
							   .setPpEnabledLayerNames(layers.data())
							   .setEnabledExtensionCount(extensions.size())
							   .setPpEnabledExtensionNames(extensions.data());

	try
	{
		vk_instance = vk::createInstance(instance_create_info, nullptr, dispatch_loader_dynamic);
	}
	catch (const vk::SystemError err)
	{
		ErrorHandler::Error<VKInitFatal>("Failed To Create Instance!", __func__);
	}
	dispatch_loader_dynamic = vk::detail::DispatchLoaderDynamic(vk_instance, vkGetInstanceProcAddr);

#ifdef _DEBUG
	SetupDebugMessenger();
#endif
	app->GetLogger()->Debug("Instance Created Successfully!", "VKInit");
}
}	 // namespace nft::Vulkan