#pragma once

#include "vk/Common.h"

namespace nft::vulkan
{
// Forward declarations
class Surface;
class Device;

//=============================================================================
// VULKAN INSTANCE CLASS
//=============================================================================
// Manages the Vulkan instance, debug messenger, and instance-level functionality

class Instance
{
public:
    //=========================================================================
    // CONSTRUCTOR & DESTRUCTOR
    //=========================================================================
    Instance(App* app);
    ~Instance();

    //=========================================================================
    // INITIALIZATION METHODS
    //=========================================================================
    void Init();
    void GetExtensions();
    void GetLayers();
    void CheckSupported();
    void SetupDebugMessenger();
    void CreateInstance();

    //=========================================================================
    // PUBLIC METHODS
    //=========================================================================
    // Initialize dispatch loader with device (called after device creation)
    void InitDispatchLoaderWithDevice(const vk::Device& device) 
    {
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_instance, vkGetInstanceProcAddr, device);
    }

    //=========================================================================
    // STATIC CALLBACKS
    //=========================================================================
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT        message_severity,
        VkDebugUtilsMessageTypeFlagsEXT               message_type,
        const VkDebugUtilsMessengerCallbackDataEXT*   p_callback_data,
        void*                                         p_user_data);

    //=========================================================================
    // PUBLIC GETTERS (const methods for read-only access)
    //=========================================================================
    App* GetApp() const { return app; }
    const vk::Instance& GetVkInstance() const { return vk_instance; }
    const vk::DebugUtilsMessengerEXT& GetDebugMessenger() const { return vk_debug_messenger; }

    // Extension and layer information
    const std::vector<const char*>& GetExtensions() const { return extensions; }
    const std::vector<const char*>& GetLayers() const { return layers; }

private:
    //=========================================================================
    // PRIVATE MEMBER VARIABLES
    //=========================================================================
    
    // Core references
    App* app = nullptr;

    // Vulkan objects
    vk::Instance               vk_instance        = nullptr;
    vk::DebugUtilsMessengerEXT vk_debug_messenger = nullptr;

    // Dynamic loader support
    static vk::detail::DynamicLoader  dynamic_loader;
    PFN_vkGetInstanceProcAddr         vkGetInstanceProcAddr = 
        dynamic_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");

    // Instance creation data
    vk::ApplicationInfo                  vk_app_info = nullptr;
    std::vector<const char*>             extensions;
    std::vector<const char*>             layers;
    vk::InstanceCreateInfo               vk_instance_info;
    vk::DebugUtilsMessengerCreateInfoEXT vk_debug_messenger_info;

    //=========================================================================
    // FRIEND CLASSES (Allow controlled access to private members)
    //=========================================================================
    friend class Device;   // Needs access to vk_instance
    friend class Surface;  // Needs access to vk_instance
	friend struct ShaderStage;
	friend struct VertexShaderStage;
	friend struct FragmentShaderStage;
	friend struct VertexInputStage;
	friend struct InputAssemblyStage;
	friend struct ViewportStage;
	friend struct RasterizationStage;
	friend struct MultisampleStage;
	friend struct ColorBlendStage;
	friend struct DescriptorSetLayout;
	friend struct DescriptorPool;
	friend struct PipelineLayout;
	friend struct RenderPass;
};

} // namespace nft::vulkan