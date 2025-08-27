#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "graphics/material.h"
#include "graphics/mesh.h"

namespace nft::parse
{

class ObjScene
{
  public:
	using Mesh = graphics::TriMesh;
	using Material = graphics::Material;

	class Node
	{
	  public:
		std::vector<uint32_t> GetMeshIndices() const { return mesh_indices; }
		std::vector<Node>	  GetChildren() const { return children; }

	  private:
		std::vector<uint32_t> mesh_indices;
		std::vector<Node>	  children;

		friend class ObjScene;
	};

	std::vector<Mesh>	  GetMeshes() const { return meshes; }
	std::vector<Material> GetMaterials() const { return materials; }

  private:
	Node				  root_node;
	std::vector<Mesh>	  meshes;
	std::vector<Material> materials;
};

class ObjLoader
{
  public:
	ObjLoader(const std::string& file_dir, const std::string& file_name);
	~ObjLoader() = default;

	std::unique_ptr<std::vector<float>>	   GetVertices();
	std::unique_ptr<std::vector<uint32_t>> GetIndices();

  private:
	std::unique_ptr<std::vector<float>>	   vertices;
	std::unique_ptr<std::vector<uint32_t>> indices;
	std::vector<glm::vec3>				   v;
	std::vector<glm::vec3>				   vn;
	std::vector<glm::vec2>				   vt;
	glm::mat4							   pre_transform = glm::mat4(1.0f);	   // Pre-transform matrix

	std::unordered_map<std::string, uint32_t>  index_history;
	std::unordered_map<std::string, glm::vec3> colors;
	glm::vec3								   brush_color = { 1.0f, 1.0f, 1.0f };	  // Default white color

	void ParseObjFile(const std::string& file_dir, const std::string& file_name);
	void ParseMtlFile(const std::string& file_dir, const std::string& file_name);

	void ReadVertexData(const std::vector<std::string>& words);
	void ReadTextureCoordData(const std::vector<std::string>& words);
	void ReadNormalData(const std::vector<std::string>& words);
	void ReadFaceData(const std::vector<std::string>& words);
	void ReadCorner(const std::string& vertex_description);
};
}	 // namespace nft::parse