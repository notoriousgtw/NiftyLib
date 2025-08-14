#include "NiftyVKRender.h"

#include "NiftyVK.h"

namespace nft::Vulkan
{
Scene::Scene(Device* device, vk::CommandBuffer main_command_buffer): device(device)
{
	if (!device)
		NFT_ERROR(VKFatal, "Device Is Null!");
	geometry_batcher = std::make_unique<GeometryBatcher>(device);

	// Create a grid of squares with proper Z positioning
	for (float x = -0.6f; x <= 0.6f; x += 1.2f)
	{
		for (float y = 0.0f; y <= 0.6f; y += 0.6f)
		{
			ObjectData object;
			// Position objects at Z=0 instead of Z=1 to avoid camera clipping
			object.transform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
			object.mesh		 = &triangle_mesh;
			objects.push_back(object);
		}
	}

	geometry_batcher->AddGeometry(&triangle_mesh);

	//geometry_batcher->Batch();
	geometry_batcher->CreateBuffer(device->vk_graphics_queue, main_command_buffer);
}

vk::VertexInputBindingDescription GetVertexInputBindingDescription()
{
	vk::VertexInputBindingDescription binding_description;
	binding_description.binding	  = 0;								 // Binding index
	binding_description.stride	  = 9 * sizeof(float);				 // Size of each vertex
	binding_description.inputRate = vk::VertexInputRate::eVertex;	 // Per-vertex data
	return binding_description;
}

std::vector<vk::VertexInputAttributeDescription> GetVertexInputAttributeDescriptions()
{
	std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;
	attribute_descriptions.reserve(2);
	// Position attribute
	attribute_descriptions.push_back(
		vk::VertexInputAttributeDescription().setBinding(0).setLocation(0).setFormat(vk::Format::eR32G32B32Sfloat).setOffset(0));
	// Color attribute
	attribute_descriptions.push_back(vk::VertexInputAttributeDescription()
										 .setBinding(0)
										 .setLocation(1)
										 .setFormat(vk::Format::eR32G32B32A32Sfloat)
										 .setOffset(3 * sizeof(float)));
	// Texture Coordinate attribute
	attribute_descriptions.push_back(vk::VertexInputAttributeDescription()
										 .setBinding(0)
										 .setLocation(2)
										 .setFormat(vk::Format::eR32G32Sfloat)
										 .setOffset(7 * sizeof(float)));
	return attribute_descriptions;
}
}	 // namespace nft::Vulkan