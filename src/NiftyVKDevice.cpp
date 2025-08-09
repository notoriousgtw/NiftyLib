#include "NiftyVKDevice.h"

#include "NiftyApp.h"
#include "NiftyError.h"
#include "NiftyVK.h"

#include <set>

namespace nft::Vulkan
{
Device::Device(Instance* instance): app(app), instance(instance)
{
	if (!instance)
		ErrorHandler::Error<VKInitFatal>("Instance is null!", __func__);
	app = this->instance->app;
	Init();
	ChoosePhysicalDevice();
	FindSuitableDevice();
	FindQueueFamilies();
	CreateDevice();
	GetQueues();
}

Device::~Device()
{
	app->GetLogger()->Debug("Cleaning Up Device...", "VKShutdown");
	vk_device.destroy();
}

void Device::Init()
{
	app->GetLogger()->Debug("Creating Logical Device...", "VKInit");
}

void Device::ChoosePhysicalDevice()
{
	app->GetLogger()->Debug("Choosing Physical Device...", "VKInit");
	available_devices = instance->vk_instance.enumeratePhysicalDevices(instance->dispatch_loader_dynamic);

	app->GetLogger()->Debug("Found Physical Devices:", "VKInit");
	for (const auto& physical_device : available_devices)
	{
		device_properties = physical_device.getProperties(instance->dispatch_loader_dynamic);

		std::string device_type;
		switch (device_properties.deviceType)
		{
		case vk::PhysicalDeviceType::eCpu: device_type = "CPU"; break;
		case vk::PhysicalDeviceType::eDiscreteGpu: device_type = "Discrete GPU"; break;
		case vk::PhysicalDeviceType::eIntegratedGpu: device_type = "Integrated GPU"; break;
		case vk::PhysicalDeviceType::eVirtualGpu: device_type = "Virtual GPU"; break;
		case vk::PhysicalDeviceType::eOther: device_type = "Other"; break;
		}

		app->GetLogger()->Debug(std::format("{}: {}", device_properties.deviceName.data(), device_type),
								"",
								Log::Flags::Default & ~Log::Flags::ShowHeader,
								0,
								4);
	};
}

void Device::GetExtensions()
{
	extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void Device::GetLayers()
{
	layers.push_back("VK_KHR_KHRONOS_validation");
}

void Device::FindSuitableDevice()
{
	for (const auto& physical_device : available_devices)
	{
		std::set<std::string> required_extensions(extensions.begin(), extensions.end());
		std::set<std::string> required_layers(layers.begin(), layers.end());

		app->GetLogger()->Debug(std::format("{} Supports Extensions:", device_properties.deviceName.data()), "VKInit");
		for (auto& extension : physical_device.enumerateDeviceExtensionProperties(nullptr, instance->dispatch_loader_dynamic))
		{
			app->GetLogger()->Debug(extension.extensionName.data(), "", Log::Flags::Default & ~Log::Flags::ShowHeader, 0, 4);
			required_extensions.erase(extension.extensionName.data());
		}
		app->GetLogger()->Debug(std::format("{} Supports Layers:", device_properties.deviceName.data()), "VKInit");
		for (auto& layer : physical_device.enumerateDeviceLayerProperties(instance->dispatch_loader_dynamic))
		{
			app->GetLogger()->Debug(layer.layerName.data(), "", Log::Flags::Default & ~Log::Flags::ShowHeader, 0, 4);
			required_layers.erase(layer.layerName.data());
		}
		if (required_extensions.empty())
		{
			app->GetLogger()->Debug(std::format("Device {} Is Suitable!", device_properties.deviceName.data()), "VKInit");
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
		vk_physical_device.getQueueFamilyProperties(instance->dispatch_loader_dynamic);

	int i = 0;
	for (const vk::QueueFamilyProperties& queue_family : queue_families)
	{
		if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			queue_family_indices.graphics_family = i;
			app->GetLogger()->Debug(std::format("Queue Family {} Is Suitable For Graphics!", i), "VKInit");
		}

		if (vk_physical_device.getSurfaceSupportKHR(i, instance->surfaces[0]->vk_surface, instance->dispatch_loader_dynamic))
		{
			queue_family_indices.present_family = i;
			app->GetLogger()->Debug(std::format("Queue Family {} Is Suitable For Presenting!", i), "VKInit");
		}

		if (queue_family_indices.IsComplete())
			break;

		i++;
	}
}

void Device::CreateDevice()
{
	float queue_priority = 1.0f;

	std::set<uint32_t> unique_indices;
	unique_indices.insert(queue_family_indices.graphics_family.value());
	unique_indices.insert(queue_family_indices.present_family.value());

	for (uint32_t queue_family_index : unique_indices)
	{
		device_queue_create_info.push_back(vk::DeviceQueueCreateInfo()
											   .setFlags(vk::DeviceQueueCreateFlags())
											   .setQueueFamilyIndex(queue_family_index)
											   .setQueueCount(1)
											   .setPQueuePriorities(&queue_priority));
	}

	device_features = vk::PhysicalDeviceFeatures();

	device_create_info = vk::DeviceCreateInfo()
							 .setFlags(vk::DeviceCreateFlags())
							 .setQueueCreateInfoCount(device_queue_create_info.size())
							 .setPQueueCreateInfos(device_queue_create_info.data())
							 .setEnabledLayerCount(layers.size())
							 .setPpEnabledLayerNames(layers.data())
							 .setEnabledExtensionCount(extensions.size())
							 .setPpEnabledExtensionNames(extensions.data())
							 .setPEnabledFeatures(&device_features);

	try
	{
		vk_device = vk_physical_device.createDevice(device_create_info, nullptr, instance->dispatch_loader_dynamic);
	}
	catch (const vk::SystemError err)
	{
		ErrorHandler::Error<VKInitFatal>("Failed To Create Logical Device!", __func__);
	}

	app->GetLogger()->Debug("Logical Device Created Successfully!", "VKInit");
}

void Device::GetQueues()
{
	vk_graphics_queue = vk_device.getQueue(queue_family_indices.graphics_family.value(), 0, instance->dispatch_loader_dynamic);
	vk_present_queue  = vk_device.getQueue(queue_family_indices.present_family.value(), 0, instance->dispatch_loader_dynamic);
}
}	 // namespace nft::Vulkan