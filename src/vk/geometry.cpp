#include "vk/geometry.h"

#include "core/parse_obj.h"
#include "vk/handler.h"

namespace nft::vulkan
{

//IMesh::IMesh(): vertices(std::make_unique<std::vector<float>>()), indices(std::make_unique<std::vector<uint32_t>>())
//{
//	// Initialize empty mesh with no vertices or indices
//}

//void IMesh::LoadObj(const std::string& file_dir, const std::string& file_name)
//{
//	auto mesh_scene = parse::MeshScene::LoadFromFile(file_dir, file_name);
//
//	auto new_vertices = obj_loader.GetVertices();
//	auto new_indices  = obj_loader.GetIndices();
//
//	// Only assign if the ObjLoader successfully parsed data
//	if (new_vertices != nullptr)
//	{
//		vertices = std::move(new_vertices);
//	}
//	else
//	{
//		// Keep the existing empty vectors or create new ones if needed
//		if (!vertices)
//		{
//			vertices = std::make_unique<std::vector<float>>();
//		}
//	}
//
//	if (new_indices != nullptr)
//	{
//		indices = std::move(new_indices);
//	}
//	else
//	{
//		// Keep the existing empty vectors or create new ones if needed
//		if (!indices)
//		{
//			indices = std::make_unique<std::vector<uint32_t>>();
//		}
//	}
//}
//
//void SimpleMesh::AddVertex(VertexData vertex)
//{
//	vertices->push_back(vertex.x);
//	vertices->push_back(vertex.y);
//	vertices->push_back(vertex.z);
//	vertices->push_back(vertex.w);
//	vertices->push_back(vertex.u);
//	vertices->push_back(vertex.v);
//	vertices->push_back(vertex.nx);
//	vertices->push_back(vertex.ny);
//	vertices->push_back(vertex.nz);
//	vertices->push_back(vertex.r);
//	vertices->push_back(vertex.g);
//	vertices->push_back(vertex.b);
//	vertices->push_back(vertex.a);
//	// vertex_count++;
//}

GeometryBatcher::GeometryBatcher(Device* device): device(device)
{
	if (!device)
		NFT_ERROR(VulkanFatal, "Device Is Null!");
}

void GeometryBatcher::AddGeometry(const graphics::TriMesh* mesh)
{
	if (!mesh)
	{
		NFT_ERROR(VulkanFatal, "Mesh pointer is null!");
		return;
	}

	MeshOffsets mesh_data_entry;
	mesh_data_entry.offset = current_offset;

	// Use getter methods instead of direct access
	std::vector<float> vertices;

	for (const auto& vertex : mesh->GetVertices())
	{
		vertices.push_back(vertex.position.x);
		vertices.push_back(vertex.position.y);
		vertices.push_back(vertex.position.z);
		vertices.push_back(vertex.position.w);
		vertices.push_back(vertex.tex_coord.x);
		vertices.push_back(vertex.tex_coord.y);
		vertices.push_back(vertex.normal.x);
		vertices.push_back(vertex.normal.y);
		vertices.push_back(vertex.normal.z);
		vertices.push_back(vertex.color.r);
		vertices.push_back(vertex.color.g);
		vertices.push_back(vertex.color.b);
		vertices.push_back(vertex.color.a);
	}

	std::vector<uint32_t> indices;

	for (const auto& face : mesh->GetFaces())
	{
		const auto& face_indices = face.GetVertexIndices();
		indices.insert(indices.end(), face_indices.begin(), face_indices.end());
	}

	size_t vertex_count = vertices.size() / 13;
	mesh_data_entry.size = vertex_count;

	mesh_data_entry.index_offset = index_offset;
	mesh_data_entry.index_size = 0;
	
	if (!indices.empty())
	{
		size_t index_count = indices.size();
		mesh_data_entry.index_size = index_count;
		index_offset += index_count;
	}

	vertex_data.insert(vertex_data.end(), vertices.begin(), vertices.end());
	index_data.insert(index_data.end(), indices.begin(), indices.end());

	mesh_data[mesh] = mesh_data_entry;

	current_offset += vertex_count;
}

// void GeometryBatcher::Batch()
//{
//	size_t byte_offset = 0;
//	for (const auto& mesh_data_entry : mesh_data)
//	{
//		const IMesh* mesh	= ;
//		size_t		 offset = mesh_data_entry.offset;
//	}
// }

void GeometryBatcher::CreateBuffers(vk::CommandBuffer command_buffer, vk::Queue queue)
{
	size_t	memory_size	   = vertex_data.size() * sizeof(float);
	Buffer* staging_buffer = device->GetBufferManager()->CreateBuffer(memory_size,
																	  vk::BufferUsageFlagBits::eTransferSrc,
																	  vk::MemoryPropertyFlagBits::eHostVisible |
																		  vk::MemoryPropertyFlagBits::eHostCoherent);

	void* memory_ptr = device->GetDevice().mapMemory(
		staging_buffer->vk_memory, 0, staging_buffer->vk_memory_info.allocationSize, vk::MemoryMapFlags());
	memcpy(memory_ptr, vertex_data.data(), memory_size);
	device->GetDevice().unmapMemory(staging_buffer->vk_memory);

	vertex_buffer =
		device->GetBufferManager()->CreateBuffer(memory_size,
												 vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
												 vk::MemoryPropertyFlagBits::eDeviceLocal);

	device->GetBufferManager()->CopyBuffer(staging_buffer, vertex_buffer, memory_size, command_buffer, queue);
	device->GetBufferManager()->DestroyBuffer(staging_buffer);

	if (index_data.empty())
		return;

	memory_size	   = index_data.size() * sizeof(uint32_t);
	staging_buffer = device->GetBufferManager()->CreateBuffer(memory_size,
															  vk::BufferUsageFlagBits::eTransferSrc,
															  vk::MemoryPropertyFlagBits::eHostVisible |
																  vk::MemoryPropertyFlagBits::eHostCoherent);

	memory_ptr = device->GetDevice().mapMemory(
		staging_buffer->vk_memory, 0, staging_buffer->vk_memory_info.allocationSize, vk::MemoryMapFlags());
	memcpy(memory_ptr, index_data.data(), memory_size);
	device->GetDevice().unmapMemory(staging_buffer->vk_memory);

	index_buffer =
		device->GetBufferManager()->CreateBuffer(memory_size,
												 vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
												 vk::MemoryPropertyFlagBits::eDeviceLocal);

	device->GetBufferManager()->CopyBuffer(staging_buffer, index_buffer, memory_size, command_buffer, queue);
	device->GetBufferManager()->DestroyBuffer(staging_buffer);
}

}	 // namespace nft::vulkan