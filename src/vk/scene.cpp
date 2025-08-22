#include "vk/scene.h"

#include "gui/window.h"
#include "vk/handler.h"

namespace nft::vulkan
{
Scene::Scene(Surface* surface, vk::CommandBuffer main_command_buffer):
	Observer(surface->window->event_handler.get()), surface(surface), device(surface->device)
{
	if (!device)
		NFT_ERROR(VulkanFatal, "Device Is Null!");
	geometry_batcher = std::make_unique<GeometryBatcher>(device);

	meshes.resize(1, new SimpleMesh());

	Subscribe<KeyEvent>();
	Subscribe<MouseButtonEvent>();
	Subscribe<MouseMoveEvent>();

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
	UpdateCameraFromOrbit();

	geometry_batcher->CreateBuffers(main_command_buffer, device->vk_graphics_queue);
}

void Scene::UpdateCameraFromOrbit()
{
	// Calculate camera position based on orbital parameters
	//if (selected_obj = UINT32_MAX)
	//{
	//	orbit_target = glm::vec3(0.0f, 1.0f, -2.0f);	// Default target position
	//	selected_obj = 0;
	//}
	//else if (selected_obj > 0)
	//	orbit_target = objects[selected_obj - 1].transform[3];	  // Use selected object's position

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

void Scene::Update(IEvent* source)
{

	if (auto* key_event = dynamic_cast<KeyEvent*>(source))
	{
		if (key_event->action == KeyEvent::Action::Press || key_event->action == KeyEvent::Action::Repeat)
		{
			// device->app->GetLogger()->Debug(
			//	std::format("Key Event: {} - {}", static_cast<uint32_t>(key_event->key),
			// static_cast<uint32_t>(key_event->action)));
			bool camera_changed = false;

			switch (key_event->key)
			{
			// Orbital rotation controls
			case KeyEvent::Key::A:	  // Rotate left
				orbit_angle_horizontal -= glm::radians(orbit_speed);
				camera_changed = true;
				break;
			case KeyEvent::Key::D:	  // Rotate right
				orbit_angle_horizontal += glm::radians(orbit_speed);
				camera_changed = true;
				break;
			case KeyEvent::Key::W:	  // Rotate up
				orbit_angle_vertical += glm::radians(orbit_speed);
				// Clamp vertical angle to prevent flipping
				orbit_angle_vertical = glm::clamp(orbit_angle_vertical, glm::radians(-89.0f), glm::radians(89.0f));
				camera_changed		 = true;
				break;
			case KeyEvent::Key::S:	  // Rotate down
				orbit_angle_vertical -= glm::radians(orbit_speed);
				// Clamp vertical angle to prevent flipping
				orbit_angle_vertical = glm::clamp(orbit_angle_vertical, glm::radians(-89.0f), glm::radians(89.0f));
				camera_changed		 = true;
				break;

			// Distance controls
			case KeyEvent::Key::Q:	  // Zoom in
				orbit_distance = glm::max(0.5f, orbit_distance - 0.5f);
				camera_changed = true;
				break;
			case KeyEvent::Key::E:	  // Zoom out
				orbit_distance = glm::min(50.0f, orbit_distance + 0.5f);
				camera_changed = true;
				break;

			// Reset camera
			case KeyEvent::Key::R:
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
	else if (auto* mouse_event = dynamic_cast<MouseButtonEvent*>(source))
	{
		// device->app->GetLogger()->Debug(std::format("Mouse Button Event: {} - {}",
		//									static_cast<uint32_t>(mouse_event->button),
		//									static_cast<uint32_t>(mouse_event->action)));
		if (mouse_event->action == MouseButtonEvent::Action::Press || mouse_event->action == MouseButtonEvent::Action::Repeat)
		{
			switch (mouse_event->button)
			{
			case MouseButtonEvent::Button::Left:
				selected_obj = surface->PickObjectAtPosition(last_mouse_pos.x, last_mouse_pos.y);
				orbit_target = objects[selected_obj - 1].transform[3];	  // Update orbit target to selected object position
				UpdateCameraFromOrbit();
				break;
			case MouseButtonEvent::Button::Middle:
				glm::vec2 mouse_pos = surface->window->GetMousePos();
				if (event_handler->GetKeyState(KeyEvent::Key::LeftControl) != KeyEvent::Action::Release)
				{
					// Rotate camera around selected object
					is_panning	   = true;
					is_rotating	   = false;
					last_mouse_pos = glm::vec2(mouse_pos.x, mouse_pos.y);
				}
				else
				{
					// Rotate camera around selected object
					is_panning	   = false;
					is_rotating	   = true;
					last_mouse_pos = glm::vec2(mouse_pos.x, mouse_pos.y);
				}
				last_mouse_pos = glm::vec2(mouse_pos.x, mouse_pos.y);
				break;
			default: break;
			}
		}
		else if (mouse_event->action == MouseButtonEvent::Action::Release)
		{
			switch (mouse_event->button)
			{
			case MouseButtonEvent::Button::Middle:
				// Stop rotating camera
				is_rotating = false;
				is_panning	= false;
				break;
			case MouseButtonEvent::Button::Right:
				// Stop panning camera
				// is_panning = false;
				break;
			default: break;
			}
		}
	}
	else if (auto* mouse_move_event = dynamic_cast<MouseMoveEvent*>(source))
	{
		// device->app->GetLogger()->Debug(
		//	std::format("Mouse Move Event: ({}, {})", mouse_move_event->pos.x, mouse_move_event->pos.y));
		glm::vec2 delta = mouse_move_event->pos - last_mouse_pos;
		last_mouse_pos	= mouse_move_event->pos;
		if (is_rotating)
		{
			orbit_angle_horizontal += glm::radians(delta.x * 0.1f);
			orbit_angle_vertical += glm::radians(delta.y * 0.1f);
			// Clamp vertical angle to prevent flipping
			orbit_angle_vertical = glm::clamp(orbit_angle_vertical, glm::radians(-89.0f), glm::radians(89.0f));
			UpdateCameraFromOrbit();
		}
		else if (is_panning)
		{
			// Mouse sensitivity for panning
			const float pan_sensitivity = 0.001f;

			// Calculate camera's right and up vectors from the current camera transform
			glm::vec3 camera_right = glm::normalize(glm::vec3(camera_transforms[0]));
			glm::vec3 camera_up	   = glm::normalize(glm::vec3(camera_transforms[1]));

			// Scale the movement based on distance to target for consistent feel
			float distance_scale = orbit_distance * pan_sensitivity;

			// Calculate pan offset in world space
			glm::vec3 pan_offset = camera_right * (-delta.x * distance_scale) + camera_up * (delta.y * distance_scale);

			// Apply pan offset to orbit target
			orbit_target += pan_offset;

			// Update camera transform
			UpdateCameraFromOrbit();
		}
		last_mouse_pos = mouse_move_event->pos;
	}
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