#include "core/parse_obj.h"

#include "core/error.h"
#include "core/string.h"

#include <fstream>

namespace nft::parse
{

// Static factory method for ObjScene
std::unique_ptr<MeshScene> MeshScene::LoadFromFile(const std::string& file_dir, const std::string& file_name)
{
	ObjLoader loader;
	return std::move(loader.ParseObjFile(file_dir, file_name));
}

std::unique_ptr<MeshScene> ObjLoader::ParseObjFile(const std::string& file_dir, const std::string& file_name)
{
	obj_scene						   = std::make_unique<MeshScene>();
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
			ParseMaterialLibrary(words[1]);

		if (!words[0].compare("o"))
		{

			index_history.clear();
			v.clear();
			vt.clear();
			vn.clear();
			//colors.clear();

			//brush_color			   = glm::vec3(1.0f);	 // Default white color
			//current_material_index = 0;
			//current_material_name  = "";

			current_mesh = std::make_shared<Mesh>();
			current_mesh->SetName(words.size() > 1 ? words[1] : "Mesh_" + current_mesh_index);
			obj_scene->meshes.push_back(current_mesh);

			current_node = &obj_scene->root_node.children.emplace_back();
			current_node->mesh_indices.push_back(current_mesh_index++);
		}

		if (!words[0].compare("v"))
		{
			if (!current_mesh.get())
			{
				current_mesh = std::make_shared<Mesh>();
				current_mesh->SetName(string::split(file_name, ".")[0]);
				obj_scene->meshes.push_back(current_mesh);

				current_node = &obj_scene->root_node;
				current_node->mesh_indices.push_back(current_mesh_index++);
			}
			ReadVertexData(words);
		}

		if (!words[0].compare("vt"))
			ReadTextureCoordData(words);

		if (!words[0].compare("vn"))
			ReadNormalData(words);

		if (!words[0].compare("usemtl"))
			ParseUseMaterial(words[1]);

		if (!words[0].compare("f"))
			ReadFaceData(words);
	}

	file.close();
	return std::move(obj_scene);
}

void ObjLoader::ParseMaterialLibrary(const std::string& file_name)
{
	ParseMtlFile("", file_name);	// Keep compatibility with existing ParseMtlFile
}

void ObjLoader::ParseUseMaterial(const std::string& material_name)
{
	current_material_index = material_name_to_index[material_name];
	if (current_material_index < obj_scene->materials.size())
		brush_color = colors[current_material_index];
	else
		brush_color = glm::vec3(1.0);
}

void ObjLoader::ParseMtlFile(const std::string& file_dir, const std::string& file_name)
{
	std::string full_path;
	if (file_dir.empty())
	{
		// Try relative to assets/models
		full_path = "./assets/models/" + file_name;
	}
	else
	{
		full_path = file_dir + "/" + file_name;
	}

	std::string				 line;
	std::vector<std::string> words;

	std::ifstream file(full_path);
	if (!file.is_open())
		NFT_ERROR(FileError, "Failed to open MTL file: " + full_path);

	std::string current_mtl_name;
	glm::vec3	ambient(0.1f);
	glm::vec3	diffuse(0.8f);
	glm::vec3	specular(0.5f);
	float		specular_intensity = 32.0f;

	while (std::getline(file, line))
	{
		words = string::split(line, " ");

		if (words.empty() || words[0].empty() || words[0][0] == '#')	// Skip empty lines and comments
			continue;

		if (!words[0].compare("newmtl"))
		{
			// Save previous material if exists
			if (!current_mtl_name.empty())
			{
				auto material =
					std::make_shared<graphics::Material>(graphics::AmbientComponent(ambient),
														 graphics::DiffuseComponent(diffuse),
														 graphics::SpecularComponent(specular, UINT32_MAX, specular_intensity));
				obj_scene->materials.push_back(material);
				material_name_to_index[current_mtl_name] = static_cast<uint32_t>(obj_scene->materials.size() - 1);
				colors.push_back(diffuse);
			}

			current_mtl_name = words[1];
			// Reset to defaults
			ambient			   = glm::vec3(0.1f);
			diffuse			   = glm::vec3(0.8f);
			specular		   = glm::vec3(0.5f);
			specular_intensity = 32.0f;
		}
		else if (!words[0].compare("Ka") && words.size() >= 4)
		{
			ambient = glm::vec3(std::stof(words[1]), std::stof(words[2]), std::stof(words[3]));
		}
		else if (!words[0].compare("Kd") && words.size() >= 4)
		{
			diffuse		= glm::vec3(std::stof(words[1]), std::stof(words[2]), std::stof(words[3]));
			brush_color = diffuse;	  // Keep for compatibility
		}
		else if (!words[0].compare("Ks") && words.size() >= 4)
		{
			specular = glm::vec3(std::stof(words[1]), std::stof(words[2]), std::stof(words[3]));
		}
		else if (!words[0].compare("Ns") && words.size() >= 2)
		{
			specular_intensity = std::stof(words[1]);
		}
	}

	// Save the last material
	if (!current_mtl_name.empty())
	{
		auto material =
			std::make_shared<graphics::Material>(graphics::AmbientComponent(ambient),
												 graphics::DiffuseComponent(diffuse),
												 graphics::SpecularComponent(specular, UINT32_MAX, specular_intensity));
		obj_scene->materials.push_back(material);
		material_name_to_index[current_mtl_name] = static_cast<uint32_t>(obj_scene->materials.size() - 1);
		colors.push_back(diffuse);
	}

	file.close();
}

void ObjLoader::ReadVertexData(const std::vector<std::string>& words)
{
	glm::vec4 new_vertex = glm::vec4(std::stof(words[1]), std::stof(words[2]), std::stof(words[3]), 1.0f);
	v.push_back(new_vertex);
}

void ObjLoader::ReadTextureCoordData(const std::vector<std::string>& words)
{
	glm::vec2 new_texture_coord = glm::vec2(std::stof(words[1]), std::stof(words[2]));
	vt.push_back(new_texture_coord);
}

void ObjLoader::ReadNormalData(const std::vector<std::string>& words)
{
	glm::vec4 new_normal = glm::vec4(std::stof(words[1]), std::stof(words[2]), std::stof(words[3]), 0.0f);
	vn.push_back(new_normal);
}

void ObjLoader::ReadFaceData(const std::vector<std::string>& words)
{
	size_t triangle_count = words.size() - 3;

	for (size_t i = 0; i < triangle_count; ++i)
		current_mesh->AddFace({ ReadCorner(words.at(1)), ReadCorner(words.at(i + 2)), ReadCorner(words.at(i + 3)) },
							  current_material_index);
}

uint32_t ObjLoader::ReadCorner(const std::string& vertex_description)
{
	if (index_history.contains(vertex_description))
		return index_history[vertex_description];
	std::vector<std::string> v_vt_vn = string::split(vertex_description, "/");

	if (v_vt_vn.size() < 1 || v_vt_vn[0].empty())
		NFT_ERROR(ParseFatal, "Invalid vertex description: " + vertex_description);

	long vertex_index = std::stol(v_vt_vn[0]);
	if (vertex_index <= 0 || static_cast<size_t>(vertex_index - 1) >= v.size())
		NFT_ERROR(ParseFatal, "Vertex index out of range: " + std::to_string(vertex_index));
	else
		index_history.insert({ vertex_description, --vertex_index });

	// Position
	glm::vec4 position = v.at(std::stol(v_vt_vn[0]) - 1);

	// Color
	glm::vec4 color = glm::vec4(brush_color, 1.0f);

	// Texture Coordinate
	glm::vec2 texture_coord = glm::vec2(0.0f);
	if ((v_vt_vn.size() == 2 || v_vt_vn.size() == 3) && !v_vt_vn[1].empty())
		texture_coord = vt.at(std::stol(v_vt_vn[1]) - 1);

	// Normal
	glm::vec3 normal = glm::vec3(0.0f);
	if (v_vt_vn.size() == 3 && !v_vt_vn[2].empty())
		normal = v.at(std::stol(v_vt_vn[2]) - 1);

	current_mesh->AddVertex(graphics::Vertex { position, texture_coord, normal, color }, vertex_index);
	return vertex_index;
}

}	 // namespace nft::parse
