#pragma once  

#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include <vulkan/vulkan.hpp>  

namespace nft  
{  
class App;  
}  

namespace nft::Vulkan
{
class VKHandler;
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

	vk::Instance						  vk_instance		 = nullptr;
	vk::DebugUtilsMessengerEXT			  vk_debug_messenger = nullptr;
	vk::DispatchLoaderDynamic			  dispatch_loader_dynamic;
	std::vector<std::unique_ptr<Surface>> surfaces;

	vk::ApplicationInfo					 app_info = nullptr;
	std::vector<const char*>			 extensions;
	std::vector<const char*>			 layers;
	vk::InstanceCreateInfo				 instance_create_info;
	vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_create_info;
};
}	 // namespace nft::Vulkan