#pragma once

#include "core/event.h"

#include "vk/common.h"
#include "vk/geometry.h"
#include "vk/image.h"

namespace nft::vulkan
{
struct ObjectData
{
	IMesh*	  mesh;
	glm::mat4 transform;
	uint32_t  material_index = 0;	 // Index of the material in the materials vector
};

class Scene: Observer
{
  public:
	// Constructor
	Scene(Surface* surface, vk::CommandBuffer main_command_buffer);
	// Destructor
	~Scene() = default;

	void Update(IEventBase* source) override;

	// Add an object to the scene
	void AddObject(const ObjectData& object) { objects.push_back(object); }

	// Get all objects in the scene
	const std::vector<ObjectData>& GetObjects() const { return objects; }

	// Add a material to the scene
	void AddMaterial(const Material& material) { materials.push_back(material); }
	// Get all materials in the scene
	const std::vector<Material>& GetMaterials() const { return materials; }

	// Get the geometry batcher
	const GeometryBatcher* GetGeometryBatcher() { return geometry_batcher.get(); }

  private:
	Surface* surface;
	Device*	 device;	// Vulkan device

	glm::mat4 camera_transforms = glm::mat4(1.0f);	  // Camera transformation matrix

	// Orbital camera variables
	glm::vec3 orbit_target			 = glm::vec3(0.0f, 1.0f, -3.0f);	   // Point to orbit around
	float	  orbit_distance		 = 5.0f;						   // Distance from target
	float	  orbit_angle_horizontal = 0.0f;						   // Horizontal rotation angle
	float	  orbit_angle_vertical	 = 0.0f;						   // Vertical rotation angle
	float	  orbit_speed			 = 2.0f;						   // Rotation speed multiplier

	// Helper function to update camera transform from orbital parameters
	void UpdateCameraFromOrbit();

	std::vector<ObjectData>			 objects;			  // List of objects in the scene
	std::unique_ptr<GeometryBatcher> geometry_batcher;	  // Geometry batcher for efficient rendering
	std::vector<IMesh*>				 meshes;
	std::vector<Texture>			 textures;
	std::vector<Material>			 materials;	   // List of materials in the scene

	friend class Surface;
};
vk::VertexInputBindingDescription				 GetVertexInputBindingDescription();
std::vector<vk::VertexInputAttributeDescription> GetVertexInputAttributeDescriptions();

}	 // namespace nft::vulkan
