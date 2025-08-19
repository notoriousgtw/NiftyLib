#include "vk/scene.h"

#include "vk/handler.h"

namespace nft::vulkan
{
Scene::Scene(Device* device, vk::CommandBuffer main_command_buffer):
	device(device)
{
	if (!device)
		NFT_ERROR(VKFatal, "Device Is Null!");
	geometry_batcher = std::make_unique<GeometryBatcher>(device);

	meshes.resize(1, new SimpleMesh());

	ObjectData loaded_object;
	loaded_object.mesh = meshes[0];
	loaded_object.mesh->LoadObj("./assets/models", "plane.obj");
	loaded_object.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	loaded_object.material_index = 0;  // Use the first material
	objects.push_back(loaded_object);
	geometry_batcher->AddGeometry(loaded_object.mesh);

	// Load multiple textures for demonstration
	textures.push_back(Texture(device));
	textures[0].SetupCommands(main_command_buffer, device->vk_graphics_queue);
	textures[0].LoadFile("./assets/textures/chancy.png");
	textures[0].CreateSampler(vk::SamplerCreateInfo()
								  .setMagFilter(vk::Filter::eLinear)
								  .setMinFilter(vk::Filter::eLinear)
								  .setAddressModeU(vk::SamplerAddressMode::eRepeat)
								  .setAddressModeV(vk::SamplerAddressMode::eRepeat)
								  .setAddressModeW(vk::SamplerAddressMode::eRepeat)
								  .setAnisotropyEnable(VK_TRUE)
								  .setMaxAnisotropy(16.0f)
								  .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
								  .setUnnormalizedCoordinates(VK_FALSE)
								  .setCompareEnable(VK_FALSE)
								  .setCompareOp(vk::CompareOp::eAlways)
								  .setMipmapMode(vk::SamplerMipmapMode::eLinear)
								  .setMipLodBias(0.0f));

	// Try to load additional textures if they exist (for demonstration)
	// In a real application, you'd load these from your material definitions
	try {
		textures.push_back(Texture(device));
		textures[1].SetupCommands(main_command_buffer, device->vk_graphics_queue);
		// Try loading a different texture - fallback to first texture if not found
		textures[1].LoadFile("./assets/textures/texture.png"); // You could add this texture
		textures[1].CreateSampler(vk::SamplerCreateInfo()
									  .setMagFilter(vk::Filter::eLinear)
									  .setMinFilter(vk::Filter::eLinear)
									  .setAddressModeU(vk::SamplerAddressMode::eRepeat)
									  .setAddressModeV(vk::SamplerAddressMode::eRepeat)
									  .setAddressModeW(vk::SamplerAddressMode::eRepeat)
									  .setAnisotropyEnable(VK_TRUE)
									  .setMaxAnisotropy(16.0f)
									  .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
									  .setUnnormalizedCoordinates(VK_FALSE)
									  .setCompareEnable(VK_FALSE)
									  .setCompareOp(vk::CompareOp::eAlways)
									  .setMipmapMode(vk::SamplerMipmapMode::eLinear)
									  .setMipLodBias(0.0f));
	} catch (...) {
		// If second texture fails to load, remove it
		if (textures.size() > 1) {
			textures.pop_back();
		}
	}

	// Add example materials with texture references
	Material default_material = {
		glm::vec3(0.1f, 0.1f, 0.1f),   // ambient - low ambient lighting
		glm::vec3(0.6f, 0.6f, 0.9f),   // diffuse - main color contribution
		glm::vec3(0.1f, 0.1f, 0.1f),   // specular - reflective highlights
		0.0f,                          // specular_highlights - shininess
		UINT32_MAX,                     // ambient_texture_index - no ambient texture
		UINT32_MAX,                              // diffuse_texture_index - use first texture
		UINT32_MAX                      // specular_texture_index - no specular texture
	};
	
	Material textured_material = {
		glm::vec3(0.0f, 0.0f, 0.0f),  // ambient - slight blue tint
		glm::vec3(1.0f, 1.0f, 1.0f),    // diffuse - blue-ish
		glm::vec3(0.0f, 0.0f, 0.0f),    // specular - very reflective
		0.0f,                         // specular_highlights - very shiny
		UINT32_MAX, // ambient_texture_index - use second texture if available
		0,                              // diffuse_texture_index - use first texture
		UINT32_MAX                      // specular_texture_index - no specular texture
	};
	
	materials.push_back(default_material);
	materials.push_back(textured_material);

	geometry_batcher->CreateBuffers(main_command_buffer, device->vk_graphics_queue);
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
	attribute_descriptions.reserve(3);
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
}	 // namespace nft::vulkan