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
	uint32_t  material_index = 0;  // Index of the material in the materials vector
};

class Scene : Observer
{
  public:
	// Constructor
	Scene(Device* device, vk::CommandBuffer main_command_buffer);
	// Destructor
	~Scene() = default;

	void Update(Event* source) override {}

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
	Device*							 device;			  // Vulkan device
	std::vector<ObjectData>			 objects;			  // List of objects in the scene
	std::unique_ptr<GeometryBatcher> geometry_batcher;	  // Geometry batcher for efficient rendering
	std::vector<IMesh*>				 meshes;
	std::vector<Texture>			 textures;
	std::vector<Material>			 materials;           // List of materials in the scene

	friend class Surface;

	// temp
	SimpleMesh triangle_mesh = { {

		// V1 Pos (bottom vertex)
		0.0f,
		-0.5f,
		0.0f,	 // Position
				 // V1 Color
		0.0f,
		1.0f,
		0.0f,
		1.0f,	 // Green color
				 // V1 Texture Coordinate
		0.5f,
		0.0f,
		// V2 Pos (top-right vertex)
		0.5f,
		0.5f,
		0.0f,	 // Position
				 // V2 Color
		1.0f,
		0.0f,
		0.0f,
		1.0f,	 // Red color
				 // V2 Texture Coordinate
		1.0f,
		1.0f,
		// V3 Pos (top-left vertex)
		-0.5f,
		0.5f,
		0.0f,	 // Position
				 // V3 Color
		0.0f,
		0.0f,
		1.0f,
		1.0f,	 // Blue color
				 // V3 Texture Coordinate
		0.0f,
		1.0f

	} };	// Temporary mesh for testing
	SimpleMesh square_mesh { {
		// Triangle 1 (top-left, bottom-left, bottom-right)
		// V1 (top-left)
		-0.5f,
		0.5f,
		0.0f,	 // Position (3 floats: x, y, z)
		1.0f,
		0.0f,
		0.0f,
		1.0f,	 // Color (4 floats: r, g, b, a)
		0.0f,
		1.0f,	 // Texture Coordinate (2 floats: u, v)

		// V2 (bottom-left)
		-0.5f,
		-0.5f,
		0.0f,	 // Position
		1.0f,
		0.0f,
		0.0f,
		1.0f,	 // Color
		0.0f,
		0.0f,	 // Texture Coordinate

		// V3 (bottom-right)
		0.5f,
		-0.5f,
		0.0f,	 // Position
		1.0f,
		0.0f,
		0.0f,
		1.0f,	 // Color
		1.0f,
		0.0f,	 // Texture Coordinate

		// Triangle 2 (top-left, bottom-right, top-right)
		// V4 (top-left - repeated)
		-0.5f,
		0.5f,
		0.0f,	 // Position
		1.0f,
		0.0f,
		0.0f,
		1.0f,	 // Color
		0.0f,
		1.0f,	 // Texture Coordinate

		// V5 (bottom-right - repeated)
		0.5f,
		-0.5f,
		0.0f,	 // Position
		1.0f,
		0.0f,
		0.0f,
		1.0f,	 // Color
		1.0f,
		0.0f,	 // Texture Coordinate

		// V6 (top-right)
		0.5f,
		0.5f,
		0.0f,	 // Position
		1.0f,
		0.0f,
		0.0f,
		1.0f,	 // Color
		1.0f,
		1.0f	// Texture Coordinate
	} };	// Temporary mesh for testing

	SimpleMesh indexed_square_mesh = { {
										   // V1 (top-left)
										   -0.5f,
										   0.5f,
										   0.0f,	// Position (3 floats: x, y, z)
										   1.0f,
										   0.0f,
										   0.0f,
										   1.0f,	// Color (4 floats: r, g, b, a)
										   0.0f,
										   1.0f,	// Texture Coordinate (2 floats: u, v)

										   // V2 (bottom-left)
										   -0.5f,
										   -0.5f,
										   0.0f,	// Position
										   1.0f,
										   0.0f,
										   0.0f,
										   1.0f,	// Color
										   0.0f,
										   0.0f,	// Texture Coordinate

										   // V3 (bottom-right)
										   0.5f,
										   -0.5f,
										   0.0f,	// Position
										   1.0f,
										   0.0f,
										   0.0f,
										   1.0f,	// Color
										   1.0f,
										   0.0f,	// Texture Coordinate

										   // V6 (top-right)
										   0.5f,
										   0.5f,
										   0.0f,	// Position
										   1.0f,
										   0.0f,
										   0.0f,
										   1.0f,	// Color
										   1.0f,
										   1.0f	   // Texture Coordinate
									   },
									   { 0, 1, 2, 2, 3, 0 } };	  // Temporary mesh for testing
};
vk::VertexInputBindingDescription				 GetVertexInputBindingDescription();
std::vector<vk::VertexInputAttributeDescription> GetVertexInputAttributeDescriptions();

}	 // namespace nft::vulkan
