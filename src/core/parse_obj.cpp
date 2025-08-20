#include "core/parse_obj.h"

#include "core/error.h"
#include "core/string.h"

#include <fstream>

namespace nft::parse
{

ObjLoader::ObjLoader(const std::string& file_dir, const std::string& file_name)
{
	vertices = std::make_unique<std::vector<float>>();
	indices	 = std::make_unique<std::vector<uint32_t>>();
	ParseObjFile(file_dir, file_name);
}

std::unique_ptr<std::vector<float>> ObjLoader::GetVertices()
{
	if (vertices.get() == nullptr)
		return nullptr;
	return std::move(vertices);
}

std::unique_ptr<std::vector<uint32_t>> ObjLoader::GetIndices()
{
	if (indices.get() == nullptr)
		return nullptr;
	return std::move(indices);
}

void ObjLoader::ParseObjFile(const std::string& file_dir, const std::string& file_name)
{
	std::string				 file_path = file_dir + "/" + file_name;
	std::string				 line;
	std::vector<std::string> words;

	std::ifstream file(file_path);
	if (!file.is_open())
		NFT_ERROR(FileError, "Failed to open OBJ file: " + file_path);

	while (std::getline(file, line))
	{
		words = string::split(line, " ");

		if (words.empty() || words[0].empty() || words[0][0] == '#')	// Skip empty lines and comments
			continue;

		if (!words[0].compare("mtllib"))
			ParseMtlFile(file_dir, words[1]);

		if (!words[0].compare("v"))
			ReadVertexData(words);

		if (!words[0].compare("vt"))
			ReadTextureCoordData(words);

		if (!words[0].compare("vn"))
			ReadNormalData(words);

		if (!words[0].compare("usemtl"))
		{
			if (colors.contains(words[1]))
				brush_color = colors[words[1]];	   // Use the color from the material
			else
				brush_color = glm::vec3(1.0);	 // Use the color from the material
		}

		if (!words[0].compare("f"))
			ReadFaceData(words);
	}

	file.close();
}

void ObjLoader::ParseMtlFile(const std::string& file_dir, const std::string& file_name)
{
	std::string				 file_path = file_dir + "/" + file_name;
	std::string				 line;
	std::vector<std::string> words;

	std::ifstream file(file_path);
	if (!file.is_open())
		NFT_ERROR(FileError, "Failed to open MTL file: " + file_path);

	std::string material_name;

	while (std::getline(file, line))
	{
		words = string::split(line, " ");

		if (words.empty() || words[0].empty() || words[0][0] == '#')	// Skip empty lines and comments
			continue;

		if (!words[0].compare("newmtl"))
			material_name = words[1];

		if (!words[0].compare("Kd"))
		{
			brush_color			  = glm::vec3(std::stof(words[1]), std::stof(words[2]), std::stof(words[3]));
			colors[material_name] = brush_color;	// Store the color for the material
		}
	}

	file.close();
}

void ObjLoader::ReadVertexData(const std::vector<std::string>& words)
{
	glm::vec4 new_vertex		 = glm::vec4(std::stof(words[1]), std::stof(words[2]), std::stof(words[3]), 1.0f);
	glm::vec3 transformed_vertex = pre_transform * new_vertex;
	v.push_back(transformed_vertex);
}

void ObjLoader::ReadTextureCoordData(const std::vector<std::string>& words)
{
	glm::vec2 new_texture_coord = glm::vec2(std::stof(words[1]), std::stof(words[2]));
	vt.push_back(new_texture_coord);
}

void ObjLoader::ReadNormalData(const std::vector<std::string>& words)
{
	glm::vec4 new_normal		 = glm::vec4(std::stof(words[1]), std::stof(words[2]), std::stof(words[3]), 0.0f);
	glm::vec3 transformed_normal = pre_transform * new_normal;
	vn.push_back(transformed_normal);
}

void ObjLoader::ReadFaceData(const std::vector<std::string>& words)
{
	size_t triangle_count = words.size() - 3;

	for (size_t i = 0; i < triangle_count; ++i)
	{
		ReadCorner(words.at(1));
		ReadCorner(words.at(i + 2));
		ReadCorner(words.at(i + 3));
	}
}

void ObjLoader::ReadCorner(const std::string& vertex_description)
{
	if (index_history.contains(vertex_description))
	{
		indices->push_back(index_history[vertex_description]);
		return;
	}

	uint32_t index = static_cast<uint32_t>(index_history.size());
	index_history.insert({ vertex_description, index });
	indices->push_back(index);

	std::vector<std::string> v_vt_vn  = string::split(vertex_description, "/");

	if (v_vt_vn.size() < 1 || v_vt_vn[0].empty())
	{
		NFT_ERROR(FileError, "Invalid vertex description: " + vertex_description);
		return;
	}

	long vertex_index = std::stol(v_vt_vn[0]);
	if (vertex_index <= 0 || static_cast<size_t>(vertex_index - 1) >= v.size())
	{
		NFT_ERROR(FileError, "Vertex index out of range: " + std::to_string(vertex_index));
		return;
	}
	
	// Position
	glm::vec3				 position = v.at(std::stol(v_vt_vn[0]) - 1);
	vertices->push_back(position.x);
	vertices->push_back(position.y);
	vertices->push_back(position.z);

	// Color
	glm::vec4 color = glm::vec4(brush_color, 1.0f);
	vertices->push_back(color.r);
	vertices->push_back(color.g);
	vertices->push_back(color.b);
	vertices->push_back(color.a);

	// Texture Coordinate
	glm::vec2 texture_coord = glm::vec2(0.0f, 0.0f);
	if (v_vt_vn.size() == 3 && !v_vt_vn[1].empty())
		texture_coord = vt.at(std::stol(v_vt_vn[1]) - 1);
	vertices->push_back(texture_coord.x);
	vertices->push_back(texture_coord.y);

	// Normal
	glm::vec3				 normal = v.at(std::stol(v_vt_vn[2]) - 1);
	vertices->push_back(normal.x);
	vertices->push_back(normal.y);
	vertices->push_back(normal.z);

}

}	 // namespace nft::parse
