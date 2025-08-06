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

	bool CheckSupported();
	static VKAPI_ATTR VkBool32 VKAPI_CALL
		 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT	   message_severity,
					   VkDebugUtilsMessageTypeFlagsEXT			   message_type,
					   const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
					   void*									   p_user_data);
	void SetupDebugMessenger();

	App*								 app		 = nullptr;
	vk::Instance						 vk_instance = nullptr;
	vk::ApplicationInfo					 app_info	 = nullptr;
	std::vector<const char*>			 extensions;
	std::vector<const char*>			 layers;
	vk::InstanceCreateInfo				 create_info;
	vk::DebugUtilsMessengerEXT			 debug_messenger = nullptr;
	vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_create_info;
	vk::DispatchLoaderDynamic			 dispatch_loader_dynamic;
};
}	 // namespace nft::Vulkan