#pragma once

#include "core/error.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace nft::graphics
{

struct Vertex
{
	glm::vec4 position;
	glm::vec2 tex_coord;
	glm::vec3 normal;
	glm::vec4 color = { 1.0, 1.0, 1.0, 1.0 };
};

class Edge
{
  public:
	Edge(uint32_t v1_index, uint32_t v2_index): v1_index(v1_index), v2_index(v2_index) {}

	void SetVertexIndices(uint32_t v1, uint32_t v2)
	{
		v1_index = v1;
		v2_index = v2;
	}
	std::pair<uint32_t, uint32_t> GetVertexIndices() const { return { v1_index, v2_index }; }

  private:
	uint32_t v1_index;
	uint32_t v2_index;
};

template<uint32_t VertsPerFace>
class Face
{
  public:
	Face(const std::array<uint32_t, VertsPerFace>& vertex_indices, uint32_t material_index):
		vertex_indices(vertex_indices.begin(), vertex_indices.end()), material_index(material_index)
	{
		if (VertsPerFace < 3)
			NFT_ERROR(GraphicsFatal, "Face must have at least 3 vertices!");

		vertex_count = VertsPerFace;
	}

	std::vector<uint32_t> GetVertexIndices() const { return vertex_indices; }
	void				  SetVertexIndices(const std::array<uint32_t, VertsPerFace>& indices)
	{
		vertex_indices = std::vector<uint32_t>(indices.begin(), indices.end());
	}
	uint32_t GetMaterialIndex() const { return material_index; }
	void	 SetMaterialIndex(uint32_t index) { material_index = index; }
	uint32_t GetVertexCount() const { return vertex_count; }

  private:
	std::vector<uint32_t> vertex_indices;
	uint32_t			  material_index = 0;
	uint32_t			  vertex_count;

	template<uint32_t VertsPerFace>
	friend class Mesh;
};

template<uint32_t VertsPerFace>
class Mesh
{
  public:
	class VertexGroup
	{
	  public:
	  private:
		std::string			  name;
		std::vector<uint32_t> vertex_indices;
	};

	Mesh()	= default;
	~Mesh() = default;

	void AddVertex(Vertex vertex) { vertices.push_back(vertex); }
	void AddVertex(Vertex vertex, uint32_t index)
	{
		if (index <= vertices.size())
			vertices.reserve(index + 1);
		vertices.insert(vertices.begin() + index, vertex);
	}
	void AddEdge(uint32_t v1_index, uint32_t v2_index) { edges.emplace_back(v1_index, v2_index); }
	void AddFace(const std::array<uint32_t, VertsPerFace>& vertex_indices, uint32_t material_index = 0)
	{
		faces.emplace_back(vertex_indices, material_index);
	}
	void AddFace(const std::array<Vertex, VertsPerFace>& vertices, uint32_t material_index = 0)
	{
		std::array<uint32_t, VertsPerFace> vertex_indices;
		for (uint32_t i = 0; i < VertsPerFace; i++)
		{
			vertex_indices[i] = static_cast<uint32_t>(this->vertices.size());
			this->vertices.push_back(vertices[i]);
		}
		faces.emplace_back(vertex_indices, material_index);
	}

	const std::vector<Vertex>&			   GetVertices() const { return vertices; }
	const std::vector<Edge>&			   GetEdges() const { return edges; }
	const std::vector<Face<VertsPerFace>>& GetFaces() const { return faces; }

	std::string GetName() const { return name; }
	void		SetName(const std::string& name) { this->name = name; }

  private:
	std::string						name;
	std::vector<Vertex>				vertices;
	std::vector<Edge>				edges;
	std::vector<Face<VertsPerFace>> faces;
};

typedef Mesh<3> TriMesh;
}	 // namespace nft::graphics