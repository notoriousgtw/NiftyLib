#pragma once

#include <vulkan/vulkan.hpp>

namespace nft
{
class App;
}

namespace nft::Vulkan
{
class VKHandler;

class Instance
{
  public:
	Instance(App* app);
	~Instance() = default;

	void Init();
	void GetExtensions();
	void GetLayers();
	void CheckSupported();
	void SetupDebugMessenger();
	void CreateInstance();

	static VKAPI_ATTR VkBool32 VKAPI_CALL
		 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT	   message_severity,
					   VkDebugUtilsMessageTypeFlagsEXT			   message_type,
					   const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
					   void*									   p_user_data);

	App*								 app		 = nullptr;

	vk::Instance						 vk_instance = nullptr;
	vk::DebugUtilsMessengerEXT			 vk_debug_messenger = nullptr;
	vk::DispatchLoaderDynamic			 dispatch_loader_dynamic;

	vk::ApplicationInfo					 app_info	 = nullptr;
	std::vector<const char*>			 extensions;
	std::vector<const char*>			 layers;
	vk::InstanceCreateInfo				 instance_create_info;
	vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_create_info;
};
}	 // namespace nft::Vulkan