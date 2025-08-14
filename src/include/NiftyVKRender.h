#pragma once

#include "NiftyVKCommon.h"
#include "NiftyVKGeometry.h"

namespace nft::Vulkan
{
struct ObjectData
{
	IMesh*	  mesh;
	glm::mat4 transform;
};

class Scene
{
  public:
	// Constructor
	Scene(Device* device, vk::CommandBuffer main_command_buffer);
	// Destructor
	~Scene() = default;
	// Add an object to the scene
	void AddObject(const ObjectData& object) { objects.push_back(object); }
	// Get all objects in the scene
	const std::vector<ObjectData>& GetObjects() const { return objects; }

	// Get the geometry batcher
	const GeometryBatcher* GetGeometryBatcher() { return geometry_batcher.get(); }

  private:
	Device*							 device;			  // Vulkan device
	std::vector<ObjectData>			 objects;			  // List of objects in the scene
	std::unique_ptr<GeometryBatcher> geometry_batcher;	  // Geometry batcher for efficient rendering

	friend class Surface;

	// temp
	SimpleMesh triangle_mesh = { { // Temp
		// V1 Pos (bottom vertex)
		0.0f,
		-0.5f,	  // Position - Made 10x larger
		// V1 Color
		0.0f,
		1.0f,
		0.0f,	 // Green color
		// V2 Pos (top-right vertex)
		0.5f,
		0.5f,	 // Position - Made 10x larger
		// V2 Color
		1.0f,
		0.0f,
		0.0f,	 // Green color
		// V3 Pos (top-left vertex)
		-0.5f,
		0.5f,	 // Position - Made 10x larger
		// V3 Color
		0.0f,
		0.0f,
		1.0f	// Green color

	} };	// Temporary mesh for testing
	SimpleMesh square_mesh { {
		// Triangle 1 (top-left, bottom-left, bottom-right)
		// V1 (top-left)
		-0.2f,
		0.2f,	 // Position - Made 4x larger
		1.0f,
		0.0f,
		0.0f,	 // Red color
		// V2 (bottom-left)
		-0.2f,
		-0.2f,	  // Position
		1.0f,
		0.0f,
		0.0f,	 // Red color
				 // V3 (bottom-right)
		0.2f,
		-0.2f,	  // Position
		1.0f,
		0.0f,
		0.0f,	 // Red color

		// Triangle 2 (top-left, bottom-right, top-right)
		// V4 (top-left - repeated)
		-0.2f,
		0.2f,	 // Position
		1.0f,
		0.0f,
		0.0f,	 // Red color
				 // V5 (bottom-right - repeated)
		0.2f,
		-0.2f,	  // Position
		1.0f,
		0.0f,
		0.0f,	 // Red color
				 // V6 (top-right)
		0.2f,
		0.2f,	 // Position
		1.0f,
		0.0f,
		0.0f	// Red color
	} };	// Temporary mesh for testing
};

vk::VertexInputBindingDescription				 GetVertexInputBindingDescription();
std::vector<vk::VertexInputAttributeDescription> GetVertexInputAttributeDescriptions();

}	 // namespace nft::Vulkan
