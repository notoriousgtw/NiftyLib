#pragma once

#include "vk/common.h"
#include <map>

namespace nft::vulkan
{

class IMesh
{
  public:
	struct VertexData
	{
		float x, y, z;		 // Position
		float r, g, b, a;	 // Color
		float u, v;			 // Texture Coordinate
	};

	IMesh() = default;
	IMesh(std::vector<float> vertices_data): vertices(std::move(vertices_data)) { vertex_count = vertices.size() / 5; }
	~IMesh()									   = default;
	virtual void AddVertex(VertexData vertex_data) = 0;

  protected:
	std::vector<float> vertices;
	size_t			   vertex_count;

	friend class GeometryBatcher;
	friend class Surface;
};

class SimpleMesh: public IMesh
{
  public:
	SimpleMesh() = default;
	SimpleMesh(std::vector<float> vertices_data): IMesh(std::move(vertices_data)) {}
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
		size_t offset;	  // Offset in the vertex_data vector
		size_t size;	  // Number of vertices in the mesh
	};

	GeometryBatcher(Device* device);
	~GeometryBatcher() = default;

	void AddGeometry(const IMesh* mesh);
	// void Batch();
	void	CreateBuffer(vk::Queue queue, vk::CommandBuffer command_buffer);
	Buffer* GetVertexBuffer() const { return vertex_buffer; }

  private:
	Device*							 device;	// Device used for Vulkan operations
	std::map<const IMesh*, MeshData> mesh_data;
	std::vector<float>				 vertex_data;
	size_t							 current_offset = 0;	// Current offset in the vertex_data vector

	Buffer* vertex_buffer;

	friend class Scene;
	friend class Surface;
};
}	 // namespace nft::vulkan