#pragma once

#include "vk/Common.h"
#include <optional>

namespace nft::vulkan
{
// Forward declarations
class Instance;
class BufferManager;

//=============================================================================
// VULKAN DEVICE CLASS
//=============================================================================
// Manages the Vulkan logical device, physical device selection,
// and device-level functionality

class Device
{
  public:
	//=========================================================================
	// NESTED STRUCTURES
	//=========================================================================

	// Queue family indices for graphics and presentation
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphics_family;
		std::optional<uint32_t> present_family;

		// Convert to vector for Vulkan API calls
		std::vector<uint32_t> Vec() const
		{
			return { graphics_family.value_or(UINT32_MAX), present_family.value_or(UINT32_MAX) };
		}

		// Check if all required queue families are found
		bool IsComplete() const { return graphics_family.has_value() && present_family.has_value(); }
	};

	//=========================================================================
	// CONSTRUCTOR & DESTRUCTOR
	//=========================================================================
	Device(Instance* instance);
	~Device();

	//=========================================================================
	// INITIALIZATION METHODS
	//=========================================================================
	void Init();
	void ChoosePhysicalDevice();
	void GetExtensions();
	void GetLayers();
	void FindSuitableDevice();
	void FindQueueFamilies();
	void CreateDevice();
	void GetQueues();

	//=========================================================================
	// SYNCHRONIZATION OBJECT CREATION METHODS
	//=========================================================================

	// Create a Vulkan semaphore with proper error handling
	vk::Semaphore CreateSemaphore() const;

	// Create a Vulkan fence with proper error handling (signaled by default)
	vk::Fence CreateFence(bool signaled = true) const;

	// Create a Vulkan fence with custom create info
	vk::Fence CreateFence(const vk::FenceCreateInfo& fence_info) const;

	//=========================================================================
	// PUBLIC GETTERS (const methods for read-only access)
	//=========================================================================
	Instance* GetInstance() const { return instance; }
	App*	  GetApp() const { return app; }

	// Core Vulkan objects (read-only access)
	const vk::PhysicalDevice& GetPhysicalDevice() const { return vk_physical_device; }
	const vk::Device&		  GetDevice() const { return vk_device; }
	const vk::Queue&		  GetGraphicsQueue() const { return vk_graphics_queue; }
	const vk::Queue&		  GetPresentQueue() const { return vk_present_queue; }

	// Device information
	const QueueFamilyIndices&			GetQueueFamilyIndices() const { return queue_family_indices; }
	const vk::PhysicalDeviceFeatures&	GetDeviceFeatures() const { return device_features; }
	const vk::PhysicalDeviceProperties& GetDeviceProperties() const { return device_properties; }
	const std::vector<const char*>&		GetExtensions() const { return extensions; }
	const std::vector<const char*>&		GetLayers() const { return layers; }

	// Buffer management access
	BufferManager* GetBufferManager() { return buffer_manager.get(); }

	// Public access to core members (for easier interop between Vulkan wrappers)
	QueueFamilyIndices queue_family_indices;  // Made public for easier access
	vk::Queue vk_graphics_queue = nullptr;    // Made public for easier access
	vk::Queue vk_present_queue = nullptr;     // Made public for easier access
	vk::PhysicalDevice vk_physical_device = nullptr;  // Made public for easier access
	vk::Device vk_device;                     // Made public for easier access

  private:
	//=========================================================================
	// PRIVATE MEMBER VARIABLES
	//=========================================================================

	// Core references
	Instance* instance = nullptr;
	App*	  app	   = nullptr;

	// Resource managers
	std::unique_ptr<BufferManager> buffer_manager;  // Keep unique_ptr for ownership

	// Device selection data
	std::vector<vk::PhysicalDevice> available_devices;
	vk::PhysicalDeviceFeatures		device_features;
	vk::PhysicalDeviceProperties	device_properties;
	std::vector<const char*>		extensions;
	std::vector<const char*>		layers;

	// Device creation data
	vk::DeviceCreateInfo				   vk_device_info;
	std::vector<vk::DeviceQueueCreateInfo> vk_device_queue_info;

	//=========================================================================
	// PRIVATE HELPER METHODS
	//=========================================================================

	// Platform-specific presentation support check
	bool CheckPlatformPresentationSupport(uint32_t queue_family_index) const;
};

}	 // namespace nft::vulkan
