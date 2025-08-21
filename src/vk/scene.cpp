#include "vk/scene.h"

#include "gui/window.h"
#include "vk/handler.h"

namespace nft::vulkan
{
Scene::Scene(Surface* surface, vk::CommandBuffer main_command_buffer): surface(surface), device(surface->device)
{
	if (!device)
		NFT_ERROR(VKFatal, "Device Is Null!");
	geometry_batcher = std::make_unique<GeometryBatcher>(device);

	meshes.resize(1, new SimpleMesh());

	Subscribe<KeyEvent>(surface->window->event_handler.get());

	ObjectData loaded_object;
	loaded_object.mesh = meshes[0];
	loaded_object.mesh->LoadObj("./assets/models", "monkey.obj");
	loaded_object.transform		 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, -3.0f));
	loaded_object.transform		 = glm::rotate(loaded_object.transform, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	loaded_object.material_index = 0;	 // Use the first material
	objects.push_back(loaded_object);
	geometry_batcher->AddGeometry(loaded_object.mesh);

	// Load multiple textures for demonstration
	textures.push_back(Texture(device));
	textures[0].SetupCommands(main_command_buffer, device->vk_graphics_queue);
	textures[0].LoadFile("./assets/textures/default.png");
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

	//// Try to load additional textures if they exist (for demonstration)
	//// In a real application, you'd load these from your material definitions
	// try {
	//	textures.push_back(Texture(device));
	//	textures[1].SetupCommands(main_command_buffer, device->vk_graphics_queue);
	//	// Try loading a different texture - fallback to first texture if not found
	//	textures[1].LoadFile("./assets/textures/texture.png"); // You could add this texture
	//	textures[1].CreateSampler(vk::SamplerCreateInfo()
	//								  .setMagFilter(vk::Filter::eLinear)
	//								  .setMinFilter(vk::Filter::eLinear)
	//								  .setAddressModeU(vk::SamplerAddressMode::eRepeat)
	//								  .setAddressModeV(vk::SamplerAddressMode::eRepeat)
	//								  .setAddressModeW(vk::SamplerAddressMode::eRepeat)
	//								  .setAnisotropyEnable(VK_TRUE)
	//								  .setMaxAnisotropy(16.0f)
	//								  .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
	//								  .setUnnormalizedCoordinates(VK_FALSE)
	//								  .setCompareEnable(VK_FALSE)
	//								  .setCompareOp(vk::CompareOp::eAlways)
	//								  .setMipmapMode(vk::SamplerMipmapMode::eLinear)
	//								  .setMipLodBias(0.0f));
	// } catch (...) {
	//	// If second texture fails to load, remove it
	//	if (textures.size() > 1) {
	//		textures.pop_back();
	//	}
	// }

	// Add example materials with texture references
	Material default_material = {
		glm::vec3(0.1f, 0.1f, 0.1f),	// ambient - low ambient lighting
		glm::vec3(0.6f, 0.6f, 0.9f),	// diffuse - main color contribution
		glm::vec3(0.1f, 0.1f, 0.1f),	// specular - reflective highlights
		0.0f,							// specular_highlights - shininess
		UINT32_MAX,						// ambient_texture_index - no ambient texture
		UINT32_MAX,						// diffuse_texture_index - use first texture
		UINT32_MAX						// specular_texture_index - no specular texture
	};

	Material default_textured_material = {
		glm::vec3(0.0f, 0.0f, 0.0f),	// ambient - slight blue tint
		glm::vec3(1.0f, 1.0f, 1.0f),	// diffuse - blue-ish
		glm::vec3(0.0f, 0.0f, 0.0f),	// specular - very reflective
		0.0f,							// specular_highlights - very shiny
		UINT32_MAX,						// ambient_texture_index - use second texture if available
		0,								// diffuse_texture_index - use first texture
		UINT32_MAX						// specular_texture_index - no specular texture
	};

	materials.push_back(default_material);
	materials.push_back(default_textured_material);
	camera_transforms = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	geometry_batcher->CreateBuffers(main_command_buffer, device->vk_graphics_queue);
}

void Scene::UpdateCameraFromOrbit()
{
	// Calculate camera position based on orbital parameters
	float x = orbit_target.x + orbit_distance * cos(orbit_angle_vertical) * cos(orbit_angle_horizontal);
	float y = orbit_target.y + orbit_distance * sin(orbit_angle_vertical);
	float z = orbit_target.z + orbit_distance * cos(orbit_angle_vertical) * sin(orbit_angle_horizontal);

	glm::vec3 camera_position = glm::vec3(x, y, z);

	// Create look-at matrix
	glm::vec3 up		  = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::mat4 view_matrix = glm::lookAt(camera_position, orbit_target, up);

	// Convert view matrix to camera transform (inverse of view matrix)
	camera_transforms = glm::inverse(view_matrix);
}

void Scene::Update(IEventBase* source)
{
	if (source->GetCode() == KeyEvent::GetCode())
	{
		KeyEvent* key_event = static_cast<KeyEvent*>(source);
		if (key_event->action == GLFW_PRESS || key_event->action == GLFW_REPEAT)
		{
			bool camera_changed = false;

			switch (key_event->key)
			{
			// Orbital rotation controls
			case GLFW_KEY_A:	// Rotate left
				orbit_angle_horizontal -= glm::radians(orbit_speed);
				camera_changed = true;
				break;
			case GLFW_KEY_D:	// Rotate right
				orbit_angle_horizontal += glm::radians(orbit_speed);
				camera_changed = true;
				break;
			case GLFW_KEY_W:	// Rotate up
				orbit_angle_vertical += glm::radians(orbit_speed);
				// Clamp vertical angle to prevent flipping
				orbit_angle_vertical = glm::clamp(orbit_angle_vertical, glm::radians(-89.0f), glm::radians(89.0f));
				camera_changed		 = true;
				break;
			case GLFW_KEY_S:	// Rotate down
				orbit_angle_vertical -= glm::radians(orbit_speed);
				// Clamp vertical angle to prevent flipping
				orbit_angle_vertical = glm::clamp(orbit_angle_vertical, glm::radians(-89.0f), glm::radians(89.0f));
				camera_changed		 = true;
				break;

			// Distance controls
			case GLFW_KEY_Q:	// Zoom in
				orbit_distance = glm::max(0.5f, orbit_distance - 0.5f);
				camera_changed = true;
				break;
			case GLFW_KEY_E:	// Zoom out
				orbit_distance = glm::min(50.0f, orbit_distance + 0.5f);
				camera_changed = true;
				break;

			// Reset camera
			case GLFW_KEY_R:
				orbit_distance		   = 5.0f;
				orbit_angle_horizontal = 0.0f;
				orbit_angle_vertical   = 0.0f;
				camera_changed		   = true;
				break;

			default: break;
			}

			if (camera_changed)
			{
				UpdateCameraFromOrbit();
			}
		}
	}
	// if (source->GetType() == EventType::KeyEvent)
	//{
	//	KeyEvent* key_event = static_cast<KeyEvent*>(source);
	//	if (key_event->GetAction() == KeyAction::Press && key_event->GetKey() == KeyCode::F5)
	//	{
	//		// Reload the scene or perform any other action
	//		app->GetLogger()->Debug("Reloading scene on F5 press", "VKScene");
	//		// For now, just log it
	//	}
	// }
}

vk::VertexInputBindingDescription GetVertexInputBindingDescription()
{
	vk::VertexInputBindingDescription binding_description;
	binding_description.binding	  = 0;								 // Binding index
	binding_description.stride	  = 12 * sizeof(float);				 // Size of each vertex
	binding_description.inputRate = vk::VertexInputRate::eVertex;	 // Per-vertex data
	return binding_description;
}

std::vector<vk::VertexInputAttributeDescription> GetVertexInputAttributeDescriptions()
{
	std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;
	attribute_descriptions.reserve(4);
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
	// Normal attribute
	attribute_descriptions.push_back(vk::VertexInputAttributeDescription()
										 .setBinding(0)
										 .setLocation(3)
										 .setFormat(vk::Format::eR32G32B32Sfloat)
										 .setOffset(9 * sizeof(float)));
	return attribute_descriptions;
}
}	 // namespace nft::vulkan