#pragma once

#include "vk/common.h"
#include <map>
#include <vector>

namespace nft::vulkan
{

struct Material
{
	glm::vec3 ambient;				  // Ambient color
	glm::vec3 diffuse;				  // Diffuse color
	glm::vec3 specular;				  // Specular color
	float	  specular_highlights;	  // Specular highlights intensity
	uint32_t  ambient_texture_index  = UINT32_MAX;   // Index into texture array (UINT32_MAX = no texture)
	uint32_t  diffuse_texture_index  = UINT32_MAX;   // Index into texture array (UINT32_MAX = no texture)  
	uint32_t  specular_texture_index = UINT32_MAX;   // Index into texture array (UINT32_MAX = no texture)
};

// Push constant structure for per-object material data (must be <= 128 bytes)
struct MaterialPushConstants
{
	alignas(16) glm::vec3 ambient;
	alignas(16) glm::vec3 diffuse;
	alignas(16) glm::vec3 specular;
	alignas(4)  float specular_highlights;
	alignas(4)  uint32_t diffuse_texture_index;     // Index into texture array
	alignas(4)  uint32_t ambient_texture_index;     // Index into texture array  
	alignas(4)  uint32_t specular_texture_index;    // Index into texture array
	alignas(4)  uint32_t padding;                   // Ensure proper alignment
};

class IMesh
{
  public:
	struct VertexData
	{
		float x, y, z;		 // Position
		float r, g, b, a;	 // Color
		float u, v;			 // Texture Coordinate
	};

	IMesh(): vertices(std::make_unique<std::vector<float>>()), indices(std::make_unique<std::vector<uint32_t>>()) {}
	IMesh(std::vector<float> vertices_data): vertices(std::make_unique<std::vector<float>>(vertices_data)) {}
	IMesh(std::vector<float> vertices_data, std::vector<uint32_t> indices_data):
		vertices(std::make_unique<std::vector<float>>(vertices_data)),
		indices(std::make_unique<std::vector<uint32_t>>(indices_data))
	{
	}
	~IMesh()									   = default;
	virtual void AddVertex(VertexData vertex_data) = 0;
	void		 LoadObj(const std::string& file_dir, const std::string& file_name);	// Load mesh from OBJ file

  protected:
	std::unique_ptr<std::vector<float>>	   vertices;
	std::unique_ptr<std::vector<uint32_t>> indices;	   // Optional indices for indexed drawing

	friend class GeometryBatcher;
	friend class Surface;
};

class SimpleMesh: public IMesh
{
  public:
	SimpleMesh(): IMesh() {}
	SimpleMesh(std::vector<float> vertices_data): IMesh(std::move(vertices_data)) {}
	SimpleMesh(std::vector<float> vertices_data, std::vector<uint32_t> indices_data):
		IMesh(std::move(vertices_data), std::move(indices_data))
	{
	}
	~SimpleMesh() = default;

	void AddVertex(VertexData vertex_data) override;

  private:
	// Additional properties or methods specific to SimpleMesh can be added here
};

class GeometryBatcher
{
  public:
	struct MeshData
	{
		size_t offset;				// Offset in the vertex_data vector
		size_t size;				// Number of vertices in the mesh
		size_t index_offset = 0;	// Offset in the index_data vector (if indices are used)
		size_t index_size	= 0;	// Number of indices in the mesh (if indices are used)
	};

	GeometryBatcher(Device* device);
	~GeometryBatcher() = default;

	void AddGeometry(const IMesh* mesh);
	void	CreateBuffers(vk::CommandBuffer command_buffer, vk::Queue queue);
	Buffer* GetVertexBuffer() const { return vertex_buffer; }

  private:
	Device*							 device;	// Device used for Vulkan operations
	std::map<const IMesh*, MeshData> mesh_data;
	std::vector<float>				 vertex_data;
	std::vector<uint32_t>			 index_data;			// Optional indices for indexed drawing
	size_t							 current_offset = 0;	// Current offset in the vertex_data vector
	size_t							 index_offset	= 0;	// Current offset in the indices_data vector

	Buffer* vertex_buffer;
	Buffer* index_buffer;

	friend class Scene;
	friend class Surface;
};
}	 // namespace nft::vulkan