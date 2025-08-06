#pragma once

#include <optional>
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

	void ChooseDevice();
	void LogDeviceProperties();
	void FindSuitableDevice();
	void FindQueueFamilies();

	Instance*						instance;
	vk::PhysicalDevice				vk_device = nullptr;
	App*							app		  = nullptr;
	std::vector<vk::PhysicalDevice> available_devices;
	std::vector<const char*>		requested_extensions;
	QueueFamilyIndices				queue_families;
};
}	 // namespace nft::Vulkan
