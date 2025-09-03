#pragma once

#include "vk/common.h"

namespace nft::vulkan
{
//#define MAX_FRAMES 8        // Support up to 8 frames in flight for bindless camera data

// Forward declarations for classes that may not be included yet
class Device;
struct DescriptorPool;
struct DescriptorSetLayout;
class GlobalBindlessManager;

//=========================================================================
// DESCRIPTOR HELPER FUNCTIONS
//=========================================================================

// Creates a descriptor pool optimized for bindless resources
std::unique_ptr<DescriptorPool> CreateBindlessDescriptorPool(Device* device, uint32_t max_sets);

// Creates a descriptor set layout for bindless rendering
std::unique_ptr<DescriptorSetLayout> CreateBindlessDescriptorSetLayout(Device* device);

// Complete setup function for bindless vertex buffer system
void SetupBindlessVertexBufferSystem(Device* device, 
									 uint32_t max_frames,
									 std::unique_ptr<DescriptorPool>& out_descriptor_pool,
									 std::unique_ptr<DescriptorSetLayout>& out_descriptor_layout,
									 std::unique_ptr<GlobalBindlessManager>& out_resource_manager);

//=========================================================================
// VERTEX INPUT HELPER FUNCTIONS
//=========================================================================

// Vertex input helper functions (for compatibility with traditional vertex input)
vk::VertexInputBindingDescription				 GetVertexInputBindingDescription();
std::vector<vk::VertexInputAttributeDescription> GetVertexInputAttributeDescriptions();

// Bindless vertex input helper functions (for bindless rendering - no vertex input state needed)
vk::VertexInputBindingDescription				 GetBindlessVertexInputBindingDescription();
std::vector<vk::VertexInputAttributeDescription> GetBindlessVertexInputAttributeDescriptions();

} // namespace nft::vulkan