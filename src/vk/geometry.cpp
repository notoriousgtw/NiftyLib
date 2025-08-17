#include "vk/geometry.h"

#include "vk/handler.h"

namespace nft::vulkan
{
void SimpleMesh::AddVertex(VertexData vertex)
{
	vertices.push_back(vertex.x);
	vertices.push_back(vertex.y);
	vertices.push_back(vertex.z);
	vertices.push_back(vertex.r);
	vertices.push_back(vertex.g);
	vertices.push_back(vertex.b);
	vertices.push_back(vertex.a);
	vertices.push_back(vertex.u);
	vertices.push_back(vertex.v);
	vertex_count++;
}

GeometryBatcher::GeometryBatcher(Device* device): device(device)
{
	if (!device)
		NFT_ERROR(VKFatal, "Device Is Null!");
}

void GeometryBatcher::AddGeometry(const IMesh* mesh)
{
	// Calculate vertex count - each vertex has 9 floats (x, y, z, r, g, b, a, u, v)
	size_t vertex_count = mesh->vertices.size() / 9;
	// Calculate current vertex offset in the batched buffer (number of vertices, not bytes)
	size_t vertex_offset = vertex_data.size() / 9;

	mesh_data[mesh] = { vertex_offset, vertex_count };
	vertex_data.insert(vertex_data.end(), mesh->vertices.begin(), mesh->vertices.end());
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

void GeometryBatcher::CreateBuffer(vk::CommandBuffer command_buffer, vk::Queue queue)
{
	size_t	memory_size	   = vertex_data.size() * sizeof(float);
	Buffer* staging_buffer = device->GetBufferManager()->CreateBuffer(memory_size,
																	  vk::BufferUsageFlagBits::eTransferSrc,
																	  vk::MemoryPropertyFlagBits::eHostVisible |
																		  vk::MemoryPropertyFlagBits::eHostCoherent);

	void* memory_ptr = device->GetDevice().mapMemory(staging_buffer->vk_memory,
													 0,
													 staging_buffer->vk_memory_info.allocationSize,
													 vk::MemoryMapFlags());
	memcpy(memory_ptr, vertex_data.data(), memory_size);
	device->GetDevice().unmapMemory(staging_buffer->vk_memory);

	vertex_buffer =
		device->GetBufferManager()->CreateBuffer(memory_size,
												 vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
												 vk::MemoryPropertyFlagBits::eDeviceLocal);

	device->GetBufferManager()->CopyBuffer(staging_buffer, vertex_buffer, memory_size, command_buffer, queue);
	device->GetBufferManager()->DestroyBuffer(staging_buffer);
}

}	 // namespace nft::vulkan