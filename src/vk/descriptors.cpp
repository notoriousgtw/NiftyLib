#include "vk/descriptors.h"

#include "graphics/mesh.h"
#include "vk/device.h"
#include "vk/pipeline.h"
#include "vk/buffer.h"
#include "vk/resources.h"

namespace nft::vulkan
{

//=========================================================================
// DESCRIPTOR HELPER FUNCTIONS
//=========================================================================

std::unique_ptr<DescriptorPool> CreateBindlessDescriptorPool(Device* device, uint32_t max_sets)
{
	if (!device)
		NFT_ERROR(VulkanFatal, "Device is null!");

	// Create descriptor pool for bindless resources
	std::vector<vk::DescriptorPoolSize> pool_sizes = {
		// Storage buffers for camera data, transforms, materials, vertices, and indices
		{ vk::DescriptorType::eStorageBuffer, MAX_FRAMES + 4 } // camera + vertex/index + transforms/materials
	};

	vk::DescriptorPoolCreateInfo pool_info = vk::DescriptorPoolCreateInfo()
		.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | 
				  vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
		.setMaxSets(max_sets)
		.setPoolSizeCount(static_cast<uint32_t>(pool_sizes.size()))
		.setPPoolSizes(pool_sizes.data());

	// Create DescriptorPool with proper Device reference
	auto descriptor_pool = std::make_unique<DescriptorPool>(device);
	descriptor_pool->vk_descriptor_pool_info = pool_info;
	
	try 
	{
		descriptor_pool->vk_descriptor_pool = device->GetDevice().createDescriptorPool(pool_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Create Bindless Descriptor Pool:\n{}", err.what()));
	}

	return descriptor_pool;
}

std::unique_ptr<DescriptorSetLayout> CreateBindlessDescriptorSetLayout(Device* device)
{
	if (!device)
		NFT_ERROR(VulkanFatal, "Device is null!");

	auto layout = std::make_unique<DescriptorSetLayout>(device);
	
	// Simplified bindless descriptor set layout bindings - only camera and transforms
	std::vector<DescriptorSetLayout::Binding> bindings = {
		// Binding 0: Camera data array (per-frame storage buffer) - accessible in vertex shader
		{ 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex },
		
		// Binding 1: Object transform buffer (all object transforms) 
		{ 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex }
	};

	layout->Init(bindings);
	return layout;
}

void SetupBindlessVertexBufferSystem(Device* device, 
									 uint32_t max_frames,
									 std::unique_ptr<DescriptorPool>& out_descriptor_pool,
									 std::unique_ptr<DescriptorSetLayout>& out_descriptor_layout,
									 std::unique_ptr<GlobalBindlessManager>& out_resource_manager)
{
	if (!device)
		NFT_ERROR(VulkanFatal, "Device is null!");

	// Create bindless descriptor pool
	out_descriptor_pool = CreateBindlessDescriptorPool(device, max_frames);

	// Create bindless descriptor set layout
	out_descriptor_layout = CreateBindlessDescriptorSetLayout(device);

	// Create bindless resource manager
	out_resource_manager = CreateGlobalBindlessManager(device, max_frames);

	// Allocate the bindless descriptor set
	out_resource_manager->AllocateDescriptorSet(out_descriptor_pool.get(), out_descriptor_layout.get());
}

//=========================================================================
// VERTEX INPUT FUNCTIONS
//=========================================================================

vk::VertexInputBindingDescription GetVertexInputBindingDescription()
{
	vk::VertexInputBindingDescription binding_description = {};
	binding_description.setBinding(0);
	binding_description.setStride(sizeof(graphics::Vertex));
	binding_description.setInputRate(vk::VertexInputRate::eVertex);
	return binding_description;
}

std::vector<vk::VertexInputAttributeDescription> GetVertexInputAttributeDescriptions()
{
	std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;

	// Position attribute
	vk::VertexInputAttributeDescription position_attr = {};
	position_attr.setBinding(0);
	position_attr.setLocation(0);
	position_attr.setFormat(vk::Format::eR32G32B32A32Sfloat);
	position_attr.setOffset(offsetof(graphics::Vertex, position));
	attribute_descriptions.push_back(position_attr);

	// Texture Coordinate attribute
	vk::VertexInputAttributeDescription tex_coord_attr = {};
	tex_coord_attr.setBinding(0);
	tex_coord_attr.setLocation(1);
	tex_coord_attr.setFormat(vk::Format::eR32G32Sfloat);
	tex_coord_attr.setOffset(offsetof(graphics::Vertex, tex_coord));
	attribute_descriptions.push_back(tex_coord_attr);

	// Normal attribute
	vk::VertexInputAttributeDescription normal_attr = {};
	normal_attr.setBinding(0);
	normal_attr.setLocation(2);
	normal_attr.setFormat(vk::Format::eR32G32B32Sfloat);
	normal_attr.setOffset(offsetof(graphics::Vertex, normal));
	attribute_descriptions.push_back(normal_attr);

	// Color attribute
	vk::VertexInputAttributeDescription color_attr = {};
	color_attr.setBinding(0);
	color_attr.setLocation(3);
	color_attr.setFormat(vk::Format::eR32G32B32A32Sfloat);
	color_attr.setOffset(offsetof(graphics::Vertex, color));
	attribute_descriptions.push_back(color_attr);

	return attribute_descriptions;
}

vk::VertexInputBindingDescription GetBindlessVertexInputBindingDescription()
{
	// For bindless rendering, no vertex input binding is needed since 
	// vertex data is accessed through storage buffers in shaders
	vk::VertexInputBindingDescription binding_description = {};
	binding_description.setBinding(0);
	binding_description.setStride(0);  // No stride needed for bindless
	binding_description.setInputRate(vk::VertexInputRate::eVertex);
	return binding_description;
}

std::vector<vk::VertexInputAttributeDescription> GetBindlessVertexInputAttributeDescriptions()
{
	// For bindless rendering, no vertex input attributes are needed since
	// vertex data is accessed through storage buffers in shaders
	// Return empty vector to indicate no traditional vertex input
	return {};
}

}	 // namespace nft::vulkan