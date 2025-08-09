#pragma once

#include <optional>

#include "NiftyVKCommon.h"

namespace nft
{
class App;
}

namespace nft::Vulkan
{
class VulkanHandler;
class Instance;
class Swapchain;

class Device
{
  public:
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphics_family;
		std::optional<uint32_t> present_family;

		std::vector<uint32_t> Vec() { return { graphics_family.value_or(UINT32_MAX), present_family.value_or(UINT32_MAX) }; }
		bool				  IsComplete() { return graphics_family.has_value() && present_family.has_value(); }
	};

	Device(Instance* instance);
	~Device();

	void Init();
	void ChoosePhysicalDevice();
	void GetExtensions();
	void GetLayers();
	void FindSuitableDevice();
	void FindQueueFamilies();
	void CreateDevice();
	void GetQueues();

	Instance* instance = nullptr;
	App*	  app	   = nullptr;

	vk::PhysicalDevice vk_physical_device = nullptr;
	vk::Device		   vk_device;
	vk::Queue		   vk_graphics_queue = nullptr;
	vk::Queue		   vk_present_queue	 = nullptr;

	std::vector<std::unique_ptr<Swapchain>> swapchains;

	std::vector<vk::PhysicalDevice> available_devices;
	QueueFamilyIndices				queue_family_indices;
	vk::PhysicalDeviceFeatures		device_features;
	vk::PhysicalDeviceProperties	device_properties;
	std::vector<const char*>		extensions;
	std::vector<const char*>		layers;

	vk::DeviceCreateInfo				   device_create_info;
	std::vector<vk::DeviceQueueCreateInfo> device_queue_create_info;
};
}	 // namespace nft::Vulkan
