#include "NiftyVKDevice.h"

#include "NiftyApp.h"
#include "NiftyError.h"
#include "NiftyVK.h"

#include <set>

namespace nft::Vulkan
{
Device::Device(App* app, Instance* instance): app(app), instance(instance)
{
	Init();
	ChoosePhysicalDevice();
	FindSuitableDevice();
	FindQueueFamilies();
	CreateDevice();
}

void Device::Init()
{
	app->GetLogger()->Debug("Creating Vulkan Logical Device...", "VKInit");
	if (!instance) { ErrorHandler::Error<VKInitFatal>("Vulkan Instance Is Null!", __func__); }
}

void Device::ChoosePhysicalDevice()
{
	app->GetLogger()->Debug("Choosing Physical Device...", "VKInit");
	available_devices = instance->vk_instance.enumeratePhysicalDevices();

	app->GetLogger()->Debug("Found Physical Devices:", "VKInit");
	for (const auto& physical_device : available_devices)
	{
		vk::PhysicalDeviceProperties device_properties = physical_device.getProperties();

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

void Device::GetExtensions() { extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME); }

void Device::GetLayers() { layers.push_back("VK_KHR_KHRONOS_validation"); }
void Device::FindSuitableDevice()
{
	for (const auto& physical_device : available_devices)
	{
		std::set<std::string>		 required_extensions(extensions.begin(), extensions.end());
		vk::PhysicalDeviceProperties device_properties = physical_device.getProperties();
		app->GetLogger()->Debug(
			std::format("\"{}\" Supports Extensions:", device_properties.deviceName.data()),
			"VKInit");
		for (auto& extension : physical_device.enumerateDeviceExtensionProperties())
		{
			app->GetLogger()->Debug(std::format("\"{}\"", extension.extensionName.data()),
									"",
									Log::Flags::Default & ~Log::Flags::ShowHeader,
									0,
									4);
			required_extensions.erase(extension.extensionName.data());
		}
		for (auto& layer : physical_device.enumerateDeviceLayerProperties())
		{
			app->GetLogger()->Debug(std::format("\"{}\"", layer.layerName.data()),
									"",
									Log::Flags::Default & ~Log::Flags::ShowHeader,
									0,
									4);
			required_extensions.erase(layer.layerName.data());
		}
		if (required_extensions.empty())
		{
			app->GetLogger()->Debug(
				std::format("Device \"{}\" Is Suitable!", device_properties.deviceName.data()),
				"VKInit");
			vk_physical_device = physical_device;
			return;
		}
	}

#ifdef _DEBUG
	layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

	ErrorHandler::Error<VKInitFatal>("Failed To Find Suitable Device!", __func__);
}

void Device::FindQueueFamilies()
{
	app->GetLogger()->Debug("Checking Device For Suitable Queue Families...", "VKInit");
	queue_family_indices.graphics_family = std::nullopt;
	queue_family_indices.present_family	 = std::nullopt;

	std::vector<vk::QueueFamilyProperties> queue_families =
		vk_physical_device.getQueueFamilyProperties();

	int i = 0;
	for (const vk::QueueFamilyProperties& queue_family : queue_families)
	{
		if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			queue_family_indices.graphics_family = i;
			queue_family_indices.present_family	 = i;
			app->GetLogger()->Debug(
				std::format("Queue Family {} Is Suitable For Graphics And Presenting!", i),
				"VKInit");
		}
		if (queue_family_indices.IsComplete()) break;

		i++;
	}
}

void Device::CreateDevice()
{
	float queue_priority = 1.0f;

	device_queue_create_info =
		vk::DeviceQueueCreateInfo()
			.setFlags(vk::DeviceQueueCreateFlags())
			.setQueueFamilyIndex(queue_family_indices.graphics_family.value())
			.setQueueCount(1)
			.setPQueuePriorities(&queue_priority);

	device_features = vk::PhysicalDeviceFeatures();

	device_create_info = vk::DeviceCreateInfo()
							 .setFlags(vk::DeviceCreateFlags())
							 .setQueueCreateInfoCount(1)
							 .setPQueueCreateInfos(&device_queue_create_info)
							 .setEnabledLayerCount(layers.size())
							 .setPpEnabledLayerNames(layers.data())
							 .setEnabledExtensionCount(extensions.size())
							 .setPpEnabledExtensionNames(extensions.data())
							 .setPEnabledFeatures(&device_features);

	try
	{
		vk_device = vk_physical_device.createDevice(
			device_create_info, nullptr, instance->dispatch_loader_dynamic);
	}
	catch (const vk::SystemError err)
	{
		ErrorHandler::Error<VKInitFatal>("Failed To Create Vulkan Logical Device!", __func__);
	}

	app->GetLogger()->Debug("Vulkan Logical Device Created Successfully!", "VKInit");
}

}	 // namespace nft::Vulkan