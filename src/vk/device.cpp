//=============================================================================
// VULKAN DEVICE IMPLEMENTATION
//=============================================================================
// This file implements the Vulkan logical device management, including
// physical device selection, queue family discovery, and device creation.

#include "vk/device.h"

#include "core/app.h"
#include "core/error.h"

#include "vk/handler.h"
#include "vk/buffer.h"

#include <set>
#include "vulkan/vulkan_win32.h"

namespace nft::vulkan
{

//=============================================================================
// CONSTRUCTOR & DESTRUCTOR
//=============================================================================

Device::Device(Instance* instance) : instance(instance)
{
    // Validate input parameters
    if (!instance)
        NFT_ERROR(VKFatal, "Instance is null!");
    
    app = instance->GetApp();
    
    // Initialize device in proper order
    Init();
    ChoosePhysicalDevice();
    GetExtensions();
    //GetLayers(); // Commented out as not used currently
    FindSuitableDevice();
    FindQueueFamilies();
    CreateDevice();
    GetQueues();
    
    // Initialize buffer manager after device is created
    buffer_manager = std::make_unique<BufferManager>(this);
}

Device::~Device()
{
    app->GetLogger()->Debug("Cleaning Up Device...", "VKShutdown");
    
    // Clean up buffer manager before destroying device
    buffer_manager.reset();
    
    vk_device.destroy(nullptr, instance->GetDispatchLoader());
}

//=============================================================================
// INITIALIZATION METHODS
//=============================================================================

void Device::Init()
{
    app->GetLogger()->Debug("Creating Logical Device...", "VKInit");
}

void Device::ChoosePhysicalDevice()
{
    app->GetLogger()->Debug("Choosing Physical Device...", "VKInit");
    
    // Enumerate available physical devices
    available_devices = instance->GetVkInstance().enumeratePhysicalDevices(instance->GetDispatchLoader());

    app->GetLogger()->Debug("Found Physical Devices:", "VKInit");
    
    // Log information about each available device
    for (const auto& physical_device : available_devices)
    {
        device_properties = physical_device.getProperties(instance->GetDispatchLoader());

        // Convert device type to human-readable string
        std::string device_type;
        switch (device_properties.deviceType)
        {
        case vk::PhysicalDeviceType::eCpu:          device_type = "CPU"; break;
        case vk::PhysicalDeviceType::eDiscreteGpu:  device_type = "Discrete GPU"; break;
        case vk::PhysicalDeviceType::eIntegratedGpu:device_type = "Integrated GPU"; break;
        case vk::PhysicalDeviceType::eVirtualGpu:   device_type = "Virtual GPU"; break;
        case vk::PhysicalDeviceType::eOther:        device_type = "Other"; break;
        }

        app->GetLogger()->Debug(std::format("{}: {}", device_properties.deviceName.data(), device_type),
                                "",
                                Log::Flags::Default & ~Log::Flags::ShowHeader,
                                0, 4);
    }
}

void Device::GetExtensions()
{
    // Add required device extensions
    extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void Device::GetLayers()
{
    // Add validation layers if needed (currently unused)
    layers.push_back("VK_KHR_KHRONOS_validation");
}

//=============================================================================
// DEVICE SELECTION METHODS
//=============================================================================

void Device::FindSuitableDevice()
{
    // Check each available device for required extensions and layers
    for (const auto& physical_device : available_devices)
    {
        std::set<std::string> required_extensions(extensions.begin(), extensions.end());
        std::set<std::string> required_layers(layers.begin(), layers.end());

        // Check extension support
        app->GetLogger()->Debug(std::format("{} Supports Extensions:", device_properties.deviceName.data()), "VKInit");
        for (auto& extension : physical_device.enumerateDeviceExtensionProperties(nullptr, instance->GetDispatchLoader()))
        {
            app->GetLogger()->Debug(extension.extensionName.data(), 
                                    "", 
                                    Log::Flags::Default & ~Log::Flags::ShowHeader, 
                                    0, 4);
            required_extensions.erase(extension.extensionName.data());
        }

        // Check layer support
        app->GetLogger()->Debug(std::format("{} Supports Layers:", device_properties.deviceName.data()), "VKInit");
        for (auto& layer : physical_device.enumerateDeviceLayerProperties(instance->GetDispatchLoader()))
        {
            app->GetLogger()->Debug(layer.layerName.data(), 
                                    "", 
                                    Log::Flags::Default & ~Log::Flags::ShowHeader, 
                                    0, 4);
            required_layers.erase(layer.layerName.data());
        }

        // If all required extensions are supported, this device is suitable
        if (required_extensions.empty())
        {
            app->GetLogger()->Debug(std::format("Device {} Is Suitable!", device_properties.deviceName.data()), "VKInit");
            vk_physical_device = physical_device;
            return;
        }
    }

    NFT_ERROR(VKFatal, "Failed To Find Suitable Device!");
}

void Device::FindQueueFamilies()
{
    app->GetLogger()->Debug("Checking Device For Suitable Queue Families...", "VKInit");
    
    // Reset queue family indices
    queue_family_indices.graphics_family = std::nullopt;
    queue_family_indices.present_family  = std::nullopt;

    // Get available queue families
    std::vector<vk::QueueFamilyProperties> queue_families =
        vk_physical_device.getQueueFamilyProperties(instance->GetDispatchLoader());

    // Search for required queue families
    int i = 0;
    for (const vk::QueueFamilyProperties& queue_family : queue_families)
    {
        // Check for graphics queue support
        if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            queue_family_indices.graphics_family = i;
            app->GetLogger()->Debug(std::format("Queue Family {} Is Suitable For Graphics!", i), "VKInit");
        }

        // Check for presentation queue support using platform-specific functions
        bool present_support = CheckPlatformPresentationSupport(i);
        
        if (present_support)
        {
            queue_family_indices.present_family = i;
            app->GetLogger()->Debug(std::format("Queue Family {} Is Suitable For Presenting!", i), "VKInit");
        }

        // Early exit if all required queue families are found
        if (queue_family_indices.IsComplete())
            break;

        i++;
    }
    
    // Log final results
    if (queue_family_indices.IsComplete())
    {
        app->GetLogger()->Debug("All required queue families found!", "VKInit");
    }
    else
    {
        if (!queue_family_indices.graphics_family.has_value())
        {
            NFT_ERROR(VKFatal, "No graphics queue family found!");
        }
        if (!queue_family_indices.present_family.has_value())
        {
            app->GetLogger()->Warn("No presentation queue family found - falling back to graphics queue", "VKInit");
            // Fall back to using graphics queue for presentation
            queue_family_indices.present_family = queue_family_indices.graphics_family;
        }
    }
}

//=============================================================================
// DEVICE CREATION METHODS
//=============================================================================

void Device::CreateDevice()
{
    // Setup queue priorities
    float queue_priority = 1.0f;

    // Get unique queue family indices
    std::set<uint32_t> unique_indices;
    unique_indices.insert(queue_family_indices.graphics_family.value());
    unique_indices.insert(queue_family_indices.present_family.value());

    // Create queue create info for each unique queue family
    for (uint32_t queue_family_index : unique_indices)
    {
        vk_device_queue_info.push_back(
            vk::DeviceQueueCreateInfo()
                .setFlags(vk::DeviceQueueCreateFlags())
                .setQueueFamilyIndex(queue_family_index)
                .setQueueCount(1)
                .setPQueuePriorities(&queue_priority));
    }

    // Setup device features (none required currently)
    device_features = vk::PhysicalDeviceFeatures();

    // Create device info structure
    vk_device_info = vk::DeviceCreateInfo()
                         .setFlags(vk::DeviceCreateFlags())
                         .setQueueCreateInfoCount(vk_device_queue_info.size())
                         .setPQueueCreateInfos(vk_device_queue_info.data())
                         .setEnabledLayerCount(layers.size())
                         .setPpEnabledLayerNames(layers.data())
                         .setEnabledExtensionCount(extensions.size())
                         .setPpEnabledExtensionNames(extensions.data())
                         .setPEnabledFeatures(&device_features);

    // Create the logical device
    try
    {
        vk_device = vk_physical_device.createDevice(vk_device_info, nullptr, instance->GetDispatchLoader());
    }
    catch (const vk::SystemError& err)
    {
        NFT_ERROR(VKFatal, std::format("Failed To Create Logical Device:\n{}", err.what()));
    }

    app->GetLogger()->Debug("Logical Device Created Successfully!", "VKInit");
}

void Device::GetQueues()
{
    // Get handles to the created queues
    vk_graphics_queue = vk_device.getQueue(
        queue_family_indices.graphics_family.value(), 0, instance->GetDispatchLoader());
    vk_present_queue = vk_device.getQueue(
        queue_family_indices.present_family.value(), 0, instance->GetDispatchLoader());
}

//=============================================================================
// SYNCHRONIZATION OBJECT CREATION METHODS
//=============================================================================

vk::Semaphore Device::CreateSemaphore() const
{
    // Validate device is created
    if (!vk_device)
    {
        NFT_ERROR(VKFatal, "Cannot create semaphore: Device not initialized!");
    }

    vk::SemaphoreCreateInfo semaphore_info{};
    
    try
    {
        vk::Semaphore semaphore = vk_device.createSemaphore(semaphore_info, nullptr, instance->GetDispatchLoader());
        
        if (app && app->GetLogger())
        {
            app->GetLogger()->Debug("Semaphore created successfully", "VKInit");
        }
        
        return semaphore;
    }
    catch (const vk::SystemError& err)
    {
        NFT_ERROR(VKFatal, std::format("Failed to create semaphore: {}", err.what()));
    }
}

vk::Fence Device::CreateFence(bool signaled) const
{
    vk::FenceCreateInfo fence_info{};
    if (signaled)
    {
        fence_info.setFlags(vk::FenceCreateFlagBits::eSignaled);
    }
    
    return CreateFence(fence_info);
}

vk::Fence Device::CreateFence(const vk::FenceCreateInfo& fence_info) const
{
    // Validate device is created
    if (!vk_device)
    {
        NFT_ERROR(VKFatal, "Cannot create fence: Device not initialized!");
    }

    try
    {
        vk::Fence fence = vk_device.createFence(fence_info, nullptr, instance->GetDispatchLoader());
        
        if (app && app->GetLogger())
        {
            app->GetLogger()->Debug("Fence created successfully", "VKInit");
        }
        
        return fence;
    }
    catch (const vk::SystemError& err)
    {
        NFT_ERROR(VKFatal, std::format("Failed to create fence: {}", err.what()));
    }
}

//=============================================================================
// PRIVATE HELPER METHODS
//=============================================================================

bool Device::CheckPlatformPresentationSupport(uint32_t queue_family_index) const
{
    try 
    {
#ifdef _WIN32
        // Windows platform - use Win32 presentation support
        // Call the C function through the dispatch loader
        VkBool32 present_support = vkGetPhysicalDeviceWin32PresentationSupportKHR(
            vk_physical_device, queue_family_index);
        
        if (present_support)
        {
            app->GetLogger()->Debug(std::format("Queue Family {} Supports Win32 Presentation!", queue_family_index), "VKInit");
            return true;
        }
        
#elif defined(__linux__)
        // Linux platform - check multiple windowing systems
        
        // For now, we'll check if the queue family supports graphics as a reasonable assumption
        // In a real implementation, you'd want to:
        // 1. Detect the available windowing system (X11, Wayland)
        // 2. Get the appropriate display connection
        // 3. Call the specific presentation support function
        
        // Example for X11 (commented out as we don't have display connection here):
        // Display* x_display = XOpenDisplay(nullptr);
        // if (x_display) {
        //     VisualID visual_id = XDefaultVisual(x_display, 0)->visualid;
        //     VkBool32 present_support = instance->GetDispatchLoader().vkGetPhysicalDeviceXlibPresentationSupportKHR(
        //         vk_physical_device, queue_family_index, x_display, visual_id);
        //     XCloseDisplay(x_display);
        //     if (present_support) return true;
        // }
        
        // Example for Wayland (commented out as we don't have display connection here):
        // struct wl_display* wayland_display = wl_display_connect(nullptr);
        // if (wayland_display) {
        //     VkBool32 present_support = instance->GetDispatchLoader().vkGetPhysicalDeviceWaylandPresentationSupportKHR(
        //         vk_physical_device, queue_family_index, wayland_display);
        //     wl_display_disconnect(wayland_display);
        //     if (present_support) return true;
        // }
        
        // Fallback: assume presentation is supported if we have graphics capability
        std::vector<vk::QueueFamilyProperties> queue_families =
            vk_physical_device.getQueueFamilyProperties(instance->GetDispatchLoader());
        
        if (queue_family_index < queue_families.size())
        {
            bool has_graphics = queue_families[queue_family_index].queueFlags & vk::QueueFlagBits::eGraphics;
            if (has_graphics)
            {
                app->GetLogger()->Debug(std::format("Queue Family {} Assumed to Support Linux Presentation (has graphics)!", queue_family_index), "VKInit");
                return true;
            }
        }
        
#elif defined(__APPLE__)
        // macOS platform - typically supported if graphics is supported
        std::vector<vk::QueueFamilyProperties> queue_families =
            vk_physical_device.getQueueFamilyProperties(instance->GetDispatchLoader());
        
        if (queue_family_index < queue_families.size())
        {
            bool has_graphics = queue_families[queue_family_index].queueFlags & vk::QueueFlagBits::eGraphics;
            if (has_graphics)
            {
                app->GetLogger()->Debug(std::format("Queue Family {} Assumed to Support Metal Presentation (has graphics)!", queue_family_index), "VKInit");
                return true;
            }
        }
#endif

        app->GetLogger()->Debug(std::format("Queue Family {} Does Not Support Platform Presentation", queue_family_index), "VKInit");
        return false;
    }
    catch (const std::exception& err)
    {
        app->GetLogger()->Warn(
            std::format("Failed to check platform presentation support for queue family {}: {}", queue_family_index, err.what()), 
            "VKInit");
        return false;
    }
}

} // namespace nft::vulkan