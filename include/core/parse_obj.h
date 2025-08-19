#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

namespace nft::parse
{

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
	std::vector<glm::vec3>	   v;
	std::vector<glm::vec3>	   vn;
	std::vector<glm::vec2>	   vt;
	glm::mat4								   pre_transform = glm::mat4(1.0f);	   // Pre-transform matrix

	std::unordered_map<std::string, uint32_t> index_history;
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