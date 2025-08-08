#pragma once

#include <optional>

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
class Instance;

class Device
{
  public:
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphics_family;
		std::optional<uint32_t> present_family;

		bool IsComplete() { return graphics_family.has_value() && present_family.has_value(); }
	};
	Device(App* app, Instance* instance);
	~Device() = default;

	void Init();
	void ChoosePhysicalDevice();
	void LogDeviceProperties();
	void GetExtensions();
	void GetLayers();
	void FindSuitableDevice();
	void FindQueueFamilies();
	void CreateDevice();

	Instance* instance = nullptr;
	App*	  app	   = nullptr;

	vk::PhysicalDevice vk_physical_device = nullptr;
	vk::Device		   vk_device;
	vk::Queue		   vk_graphics_queue;

	std::vector<vk::PhysicalDevice> available_devices;
	QueueFamilyIndices				queue_family_indices;
	vk::PhysicalDeviceFeatures		device_features;
	std::vector<const char*>		extensions;
	std::vector<const char*>		layers;

	vk::DeviceCreateInfo	  device_create_info;
	vk::DeviceQueueCreateInfo device_queue_create_info;
};
}	 // namespace nft::Vulkan
