#pragma once

#include "graphics/mesh.h"  // This should provide graphics::TriMesh
#include "vk/common.h"

#include <map>
#include <vector>

namespace nft::vulkan
{

class GeometryBatcher
{
  public:
	struct MeshOffsets
	{
		size_t offset;				// Offset in the vertex_data vector
		size_t size;				// Number of vertices in the mesh
		size_t index_offset = 0;	// Offset in the index_data vector (if indices are used)
		size_t index_size	= 0;	// Number of indices in the mesh (if indices are used)
	};

	GeometryBatcher(Device* device);
	~GeometryBatcher() = default;

	void AddGeometry(const graphics::TriMesh* mesh);
	void CreateBuffers(vk::CommandBuffer command_buffer, vk::Queue queue);

	// Public accessors instead of friend declarations
	Buffer*												   GetVertexBuffer() const { return vertex_buffer; }
	Buffer*												   GetIndexBuffer() const { return index_buffer; }
	const std::map<const graphics::TriMesh*, MeshOffsets>& GetMeshOffsets() const { return mesh_data; }
	const std::vector<float>&							   GetVertexData() const { return vertex_data; }
	const std::vector<uint32_t>&						   GetIndexData() const { return index_data; }
	bool												   HasIndexData() const { return !index_data.empty(); }

  private:
	Device*											device;	   // Device used for Vulkan operations
	std::map<const graphics::TriMesh*, MeshOffsets> mesh_data;
	std::vector<float>								vertex_data;
	std::vector<uint32_t>							index_data;			   // Optional indices for indexed drawing
	size_t											current_offset = 0;	   // Current offset in the vertex_data vector
	size_t											index_offset   = 0;	   // Current offset in the indices_data vector

	Buffer* vertex_buffer;
	Buffer* index_buffer;
};
}	 // namespace nft::vulkan