#pragma once

#include "NiftyVKCommon.h"

namespace nft
{
class App;
}

namespace nft::Vulkan
{
class VulkanHandler;
class Surface;
class Device;

class Instance
{
  public:
	Instance(App* app);
	~Instance();

	void AddSurface();

	void Init();
	void GetExtensions();
	void GetLayers();
	void CheckSupported();
	void SetupDebugMessenger();
	void CreateInstance();

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT		message_severity,
														VkDebugUtilsMessageTypeFlagsEXT				message_type,
														const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
														void*										p_user_data);

	App* app = nullptr;

	vk::Instance			   vk_instance		  = nullptr;
	vk::DebugUtilsMessengerEXT vk_debug_messenger = nullptr;

#if defined(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC)
	vk::detail::DispatchLoaderDynamic dispatch_loader_dynamic;
	static vk::detail::DynamicLoader  dynamic_loader;
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dynamic_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
#endif

	std::vector<std::unique_ptr<Surface>> surfaces;

	vk::ApplicationInfo					 app_info = nullptr;
	std::vector<const char*>			 extensions;
	std::vector<const char*>			 layers;
	vk::InstanceCreateInfo				 instance_create_info;
	vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_create_info;
};
}	 // namespace nft::Vulkan