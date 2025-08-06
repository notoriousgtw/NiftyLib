#include "NiftyVKDevice.h"

#include "NiftyApp.h"
#include "NiftyError.h"
#include "NiftyVK.h"

#include <set>

namespace nft::Vulkan
{
Device::Device(App* app, Instance* instance): app(app), instance(instance)
{
	ChooseDevice();
	FindSuitableDevice();
}

void Device::ChooseDevice()
{
	app->GetLogger()->Debug("Choosing Device...", "VKInit");
	available_devices = instance->vk_instance.enumeratePhysicalDevices();

	app->GetLogger()->Debug("Found Devices:", "VKInit");
	for (const auto& device : available_devices)
	{
		vk::PhysicalDeviceProperties device_properties = device.getProperties();

		std::string device_type;
		switch (device_properties.deviceType)
		{
		case vk::PhysicalDeviceType::eCpu: device_type = "CPU"; break;
		case vk::PhysicalDeviceType::eDiscreteGpu: device_type = "Discrete GPU"; break;
		case vk::PhysicalDeviceType::eIntegratedGpu: device_type = "Integrated GPU"; break;
		case vk::PhysicalDeviceType::eVirtualGpu: device_type = "Virtual GPU"; break;
		case vk::PhysicalDeviceType::eOther: device_type = "Other"; break;
		}

		app->GetLogger()->Debug(
			std::format("\"{}\": \"{}\"", device_properties.deviceName.data(), device_type),
			"",
			Log::Flags::Default & ~Log::Flags::ShowHeader,
			0,
			4);
	};
}

void Device::LogDeviceProperties() {}

void Device::FindSuitableDevice()
{
	requested_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	app->GetLogger()->Debug("Requesting Extensions:", "VKInit");
	for (const auto& extension : requested_extensions)
		app->GetLogger()->Debug(std::format("\"{}\"", extension),
								"",
								Log::Flags::Default & ~Log::Flags::ShowHeader,
								0,
								4);

	for (const auto& device : available_devices)
	{
		std::set<std::string>		 required_extensions(requested_extensions.begin(),
													 requested_extensions.end());
		vk::PhysicalDeviceProperties device_properties = device.getProperties();
		app->GetLogger()->Debug(
			std::format("\"{}\" Supports Extensions:", device_properties.deviceName.data()),
			"VKInit");
		for (auto& extension : device.enumerateDeviceExtensionProperties())
		{
			app->GetLogger()->Debug(std::format("\"{}\"", extension.extensionName.data()),
									"",
									Log::Flags::Default & ~Log::Flags::ShowHeader,
									0,
									4);
			required_extensions.erase(extension.extensionName.data());
		}
		if (required_extensions.empty())
		{
			app->GetLogger()->Debug(
				std::format("\"{}\" Is Suitable!", device_properties.deviceName.data()),
				"VKInit");
			vk_device = device;
			return;
		}
	}
	ErrorHandler::Error<VKInitFatal>("Failed To Find Suitable Device!", __func__);
}

void Device::FindQueueFamilies()
{
	queue_families.graphics_family = std::nullopt;
	queue_families.present_family = std::nullopt;

	std::vector<vk::QueueFamilyProperties> queue_families = vk_device.getQueueFamilyProperties();
}

}	 // namespace nft::Vulkan