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
using Mesh	   = graphics::TriMesh;
using Material = graphics::Material;

class MeshScene
{
  public:
	class Node
	{
	  public:
		std::vector<uint32_t> GetMeshIndices() const { return mesh_indices; }
		std::vector<Node>	  GetChildren() const { return children; }

	  private:
		std::vector<uint32_t> mesh_indices;
		std::string			  name;
		std::vector<Node>	  children;

		friend class MeshScene;
		friend class ObjLoader;
	};

	std::vector<std::shared_ptr<Mesh>>	   GetMeshes() const { return meshes; }
	std::vector<std::shared_ptr<Material>> GetMaterials() const { return materials; }
	Node								   GetRootNode() const { return root_node; }

	// Factory method to create an MeshScene from an OBJ file
	static std::unique_ptr<MeshScene> LoadFromFile(const std::string& file_dir, const std::string& file_name);

  private:
	Node								   root_node;
	std::vector<std::shared_ptr<Mesh>>	   meshes;
	std::vector<std::shared_ptr<Material>> materials;

	friend class ObjLoader;
};

class ObjLoader
{
  public:
	ObjLoader() = default;
	~ObjLoader() = default;

	// New methods for working with MeshScene
	// std::unique_ptr<MeshScene> CreateMeshScene();
	std::unique_ptr<MeshScene> ParseObjFile(const std::string& file_dir, const std::string& file_name);

  private:
	std::unique_ptr<MeshScene> obj_scene;
	std::vector<glm::vec4>	  v;
	std::vector<glm::vec2>	  vt;
	std::vector<glm::vec3>	  vn;

	std::unordered_map<std::string, uint32_t> index_history;
	std::shared_ptr<Mesh>					  current_mesh;
	uint32_t								  current_mesh_index = 0;
	MeshScene::Node*							  current_node;
	uint32_t								  current_material_index = 0;
	std::string								  current_material_name;
	std::unordered_map<std::string, uint32_t> material_name_to_index;	 // Map material names to indices
	std::vector<glm::vec3>					  colors;
	glm::vec3								  brush_color = { 1.0f, 1.0f, 1.0f };	 // Default white color

	// Material data for MeshScene creation

	void ParseMtlFile(const std::string& file_dir, const std::string& file_name);

	void	 ReadVertexData(const std::vector<std::string>& words);
	void	 ReadTextureCoordData(const std::vector<std::string>& words);
	void	 ReadNormalData(const std::vector<std::string>& words);
	void	 ReadFaceData(const std::vector<std::string>& words);
	uint32_t ReadCorner(const std::string& vertex_description);

	// New material parsing methods
	void ParseMaterialLibrary(const std::string& file_name);
	void ParseUseMaterial(const std::string& material_name);

	friend class MeshScene;
};
}	 // namespace nft::parse