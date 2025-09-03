#include "vk/resources.h"

#include "vk/handler.h"
#include "vk/device.h"
#include "vk/pipeline.h"
#include "core/error.h"

namespace nft::vulkan
{

// Static constant definition
const ResourceHandle ResourceHandle::INVALID = {UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX};

//=========================================================================
// GLOBAL BINDLESS MANAGER IMPLEMENTATION
//=========================================================================

GlobalBindlessManager::GlobalBindlessManager(Device* device, uint32_t max_frames)
	: device(device), max_frames(max_frames)
{
	if (!device)
		NFT_ERROR(VulkanFatal, "Device pointer is null!");
}

GlobalBindlessManager::~GlobalBindlessManager()
{
	Cleanup();
}

void GlobalBindlessManager::Init()
{
	// Create camera data buffer array for all frames
	camera_data_buffer = device->GetBufferManager()->CreateBuffer(
		sizeof(CameraData) * max_frames,
		vk::BufferUsageFlagBits::eStorageBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
	);
	camera_data_ptr = device->GetDevice().mapMemory(
		camera_data_buffer->vk_memory, 0, sizeof(CameraData) * max_frames, vk::MemoryMapFlags{}
	);
	std::memset(camera_data_ptr, 0, sizeof(CameraData) * max_frames);

	// Initialize dynamic buffer managers
	InitializeBufferManager(vertex_buffer_manager, sizeof(graphics::Vertex), INITIAL_VERTEX_COUNT,
		vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	InitializeBufferManager(index_buffer_manager, sizeof(uint32_t), INITIAL_INDEX_COUNT,
		vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	InitializeBufferManager(object_transform_manager, sizeof(glm::mat4), INITIAL_TRANSFORM_COUNT,
		vk::BufferUsageFlagBits::eStorageBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	InitializeBufferManager(material_manager, sizeof(GPUMaterial), INITIAL_MATERIAL_COUNT,
		vk::BufferUsageFlagBits::eStorageBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
}

void GlobalBindlessManager::UpdateCameraData(uint32_t frame_index, const CameraData& camera_data)
{
	if (frame_index >= max_frames)
	{
		NFT_ERROR(VulkanFatal, std::format("Frame index {} exceeds max frames {}", frame_index, max_frames));
		return;
	}
	
	if (!camera_data_ptr)
	{
		NFT_ERROR(VulkanFatal, "Camera data buffer not mapped!");
		return;
	}

	// Ensure we're not updating the same frame data that might be in use by GPU
	// This is safe for HOST_COHERENT memory but good practice
	CameraData* camera_array = static_cast<CameraData*>(camera_data_ptr);
	camera_array[frame_index] = camera_data;
	
	// For HOST_COHERENT memory, the writes are automatically visible to GPU
	// No explicit flush needed, but we could add a memory barrier here if needed
}

ResourceHandle GlobalBindlessManager::AllocateVertexData(const std::vector<graphics::Vertex>& vertices)
{
	ResourceHandle handle = vertex_buffer_manager.Allocate(static_cast<uint32_t>(vertices.size()), VERTEX_BUFFER);
	if (handle.IsValid())
	{
		vertex_buffer_manager.Update(handle.allocation_id, vertices.data(), static_cast<uint32_t>(vertices.size()));
	}
	return handle;
}

ResourceHandle GlobalBindlessManager::AllocateIndexData(const std::vector<uint32_t>& indices)
{
	ResourceHandle handle = index_buffer_manager.Allocate(static_cast<uint32_t>(indices.size()), INDEX_BUFFER);
	if (handle.IsValid())
	{
		index_buffer_manager.Update(handle.allocation_id, indices.data(), static_cast<uint32_t>(indices.size()));
	}
	return handle;
}

ResourceHandle GlobalBindlessManager::AllocateObjectTransforms(const std::vector<glm::mat4>& transforms)
{
	ResourceHandle handle = object_transform_manager.Allocate(static_cast<uint32_t>(transforms.size()), TRANSFORM_BUFFER);
	if (handle.IsValid())
	{
		object_transform_manager.Update(handle.allocation_id, transforms.data(), static_cast<uint32_t>(transforms.size()));
	}
	return handle;
}

ResourceHandle GlobalBindlessManager::AllocateMaterials(const std::vector<GPUMaterial>& materials)
{
	ResourceHandle handle = material_manager.Allocate(static_cast<uint32_t>(materials.size()), MATERIAL_BUFFER);
	if (handle.IsValid())
	{
		material_manager.Update(handle.allocation_id, materials.data(), static_cast<uint32_t>(materials.size()));
	}
	return handle;
}

void GlobalBindlessManager::UpdateVertexData(const ResourceHandle& handle, const std::vector<graphics::Vertex>& vertices)
{
	if (handle.buffer_id == VERTEX_BUFFER && handle.IsValid())
	{
		vertex_buffer_manager.Update(handle.allocation_id, vertices.data(), static_cast<uint32_t>(vertices.size()));
	}
}

void GlobalBindlessManager::UpdateIndexData(const ResourceHandle& handle, const std::vector<uint32_t>& indices)
{
	if (handle.buffer_id == INDEX_BUFFER && handle.IsValid())
	{
		index_buffer_manager.Update(handle.allocation_id, indices.data(), static_cast<uint32_t>(indices.size()));
	}
}

void GlobalBindlessManager::UpdateObjectTransforms(const ResourceHandle& handle, const std::vector<glm::mat4>& transforms)
{
	if (handle.buffer_id == TRANSFORM_BUFFER && handle.IsValid())
	{
		object_transform_manager.Update(handle.allocation_id, transforms.data(), static_cast<uint32_t>(transforms.size()));
	}
}

void GlobalBindlessManager::UpdateMaterials(const ResourceHandle& handle, const std::vector<GPUMaterial>& materials)
{
	if (handle.buffer_id == MATERIAL_BUFFER && handle.IsValid())
	{
		material_manager.Update(handle.allocation_id, materials.data(), static_cast<uint32_t>(materials.size()));
	}
}

void GlobalBindlessManager::FreeVertexData(const ResourceHandle& handle)
{
	if (handle.buffer_id == VERTEX_BUFFER && handle.IsValid())
	{
		vertex_buffer_manager.Free(handle.allocation_id);
	}
}

void GlobalBindlessManager::FreeIndexData(const ResourceHandle& handle)
{
	if (handle.buffer_id == INDEX_BUFFER && handle.IsValid())
	{
		index_buffer_manager.Free(handle.allocation_id);
	}
}

void GlobalBindlessManager::FreeObjectTransforms(const ResourceHandle& handle)
{
	if (handle.buffer_id == TRANSFORM_BUFFER && handle.IsValid())
	{
		object_transform_manager.Free(handle.allocation_id);
	}
}

void GlobalBindlessManager::FreeMaterials(const ResourceHandle& handle)
{
	if (handle.buffer_id == MATERIAL_BUFFER && handle.IsValid())
	{
		material_manager.Free(handle.allocation_id);
	}
}

vk::Buffer GlobalBindlessManager::GetVertexBuffer() const
{
	return vertex_buffer_manager.buffer ? vertex_buffer_manager.buffer->vk_buffer : VK_NULL_HANDLE;
}

vk::Buffer GlobalBindlessManager::GetIndexBuffer() const
{
	return index_buffer_manager.buffer ? index_buffer_manager.buffer->vk_buffer : VK_NULL_HANDLE;
}

GlobalBindlessManager::DrawInfo GlobalBindlessManager::GetDrawInfo(const ResourceHandle& vertex_handle, const ResourceHandle& index_handle) const
{
	DrawInfo info = {};
	
	if (vertex_handle.IsValid() && vertex_handle.buffer_id == VERTEX_BUFFER)
	{
		if (vertex_handle.allocation_id < vertex_buffer_manager.allocations.size())
		{
			const auto& alloc = vertex_buffer_manager.allocations[vertex_handle.allocation_id];
			info.vertex_offset = alloc.offset / vertex_buffer_manager.element_size;
			info.vertex_count = alloc.count;
		}
	}
	
	if (index_handle.IsValid() && index_handle.buffer_id == INDEX_BUFFER)
	{
		if (index_handle.allocation_id < index_buffer_manager.allocations.size())
		{
			const auto& alloc = index_buffer_manager.allocations[index_handle.allocation_id];
			info.index_offset = alloc.offset / index_buffer_manager.element_size;
			info.index_count = alloc.count;
		}
	}
	
	return info;
}

void GlobalBindlessManager::AllocateDescriptorSet(DescriptorPool* pool, DescriptorSetLayout* layout)
{
	if (!pool || !layout)
	{
		NFT_ERROR(VulkanFatal, "Descriptor pool or layout is null!");
		return;
	}

	// Only allocate and update if not already allocated
	if (bindless_descriptor_set == VK_NULL_HANDLE)
	{
		// Allocate the bindless descriptor set through helper function
		bindless_descriptor_set = GetDescriptorSet(device, pool, layout);

		// Update the descriptor set with buffer info - ONLY ONCE during initialization
		UpdateDescriptorSet();
		
		printf("DEBUG: Allocated and updated bindless descriptor set\n");
	}
	else
	{
		printf("DEBUG: Bindless descriptor set already allocated, skipping\n");
	}
}

void GlobalBindlessManager::Cleanup()
{
	if (device)
	{
		if (camera_data_ptr)
		{
			device->GetDevice().unmapMemory(camera_data_buffer->vk_memory);
			camera_data_ptr = nullptr;
		}
		if (camera_data_buffer)
		{
			device->GetBufferManager()->DestroyBuffer(camera_data_buffer);
			camera_data_buffer = nullptr;
		}
		
		// Cleanup dynamic buffers
		auto cleanup_buffer = [this](DynamicBuffer& manager) {
			if (manager.mapped_ptr && manager.buffer)
			{
				device->GetDevice().unmapMemory(manager.buffer->vk_memory);
				manager.mapped_ptr = nullptr;
			}
			if (manager.buffer)
			{
				device->GetBufferManager()->DestroyBuffer(manager.buffer);
				manager.buffer = nullptr;
			}
		};
		
		cleanup_buffer(vertex_buffer_manager);
		cleanup_buffer(index_buffer_manager);
		cleanup_buffer(object_transform_manager);
		cleanup_buffer(material_manager);
	}
}

//=========================================================================
// PRIVATE HELPER METHODS
//=========================================================================

void GlobalBindlessManager::InitializeBufferManager(DynamicBuffer& manager, uint32_t element_size, uint32_t initial_capacity, 
													 vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
	manager.element_size = element_size;
	manager.capacity = initial_capacity * element_size;
	manager.current_size = 0;
	
	manager.buffer = device->GetBufferManager()->CreateBuffer(
		manager.capacity, usage, properties
	);
	
	manager.mapped_ptr = device->GetDevice().mapMemory(
		manager.buffer->vk_memory, 0, manager.capacity, vk::MemoryMapFlags{}
	);
	
	std::memset(manager.mapped_ptr, 0, manager.capacity);
}

void GlobalBindlessManager::UpdateDescriptorSet()
{
	if (bindless_descriptor_set == VK_NULL_HANDLE)
	{
		printf("DEBUG: WARNING - UpdateDescriptorSet called but descriptor set not allocated\n");
		return;
	}

	// Ensure all buffer managers are initialized before updating descriptors
	if (!camera_data_buffer || !object_transform_manager.buffer)
	{
		printf("DEBUG: WARNING - UpdateDescriptorSet called but buffers not initialized\n");
		return;
	}

	std::vector<vk::WriteDescriptorSet> descriptor_writes;

	// Camera data array (binding 0)
	vk::DescriptorBufferInfo camera_buffer_info = vk::DescriptorBufferInfo()
		.setBuffer(camera_data_buffer->vk_buffer)
		.setOffset(0)
		.setRange(sizeof(CameraData) * max_frames);

	descriptor_writes.push_back(vk::WriteDescriptorSet()
		.setDstSet(bindless_descriptor_set)
		.setDstBinding(0)
		.setDstArrayElement(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eStorageBuffer)
		.setPBufferInfo(&camera_buffer_info));

	// Object transforms (binding 1)
	vk::DescriptorBufferInfo object_buffer_info = vk::DescriptorBufferInfo()
		.setBuffer(object_transform_manager.buffer->vk_buffer)
		.setOffset(0)
		.setRange(object_transform_manager.capacity);

	descriptor_writes.push_back(vk::WriteDescriptorSet()
		.setDstSet(bindless_descriptor_set)
		.setDstBinding(1)
		.setDstArrayElement(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eStorageBuffer)
		.setPBufferInfo(&object_buffer_info));

	// Update descriptors - only camera and transforms now
	try
	{
		device->GetDevice().updateDescriptorSets(descriptor_writes, nullptr);
		printf("DEBUG: Successfully updated descriptor sets with %zu descriptors\n", descriptor_writes.size());
	}
	catch (const vk::SystemError& e)
	{
		printf("DEBUG: ERROR - Failed to update descriptor sets: %s\n", e.what());
		NFT_ERROR(VulkanFatal, std::format("Failed to update descriptor sets: {}", e.what()));
	}
}

GlobalBindlessManager::DynamicBuffer& GlobalBindlessManager::GetBufferManager(BufferType type)
{
	switch (type)
	{
		case VERTEX_BUFFER: return vertex_buffer_manager;
		case INDEX_BUFFER: return index_buffer_manager;
		case TRANSFORM_BUFFER: return object_transform_manager;
		case MATERIAL_BUFFER: return material_manager;
		default: 
			NFT_ERROR(VulkanFatal, "Invalid buffer type!");
			return vertex_buffer_manager; // Return something to avoid compiler warnings
	}
}

const GlobalBindlessManager::DynamicBuffer& GlobalBindlessManager::GetBufferManager(BufferType type) const
{
	return const_cast<GlobalBindlessManager*>(this)->GetBufferManager(type);
}

//=========================================================================
// DYNAMIC BUFFER METHODS
//=========================================================================

ResourceHandle GlobalBindlessManager::DynamicBuffer::Allocate(uint32_t element_count, uint32_t buffer_id)
{
	uint32_t required_size = element_count * element_size;
	
	// Try to find a suitable free allocation first
	for (uint32_t free_idx : free_list)
	{
		if (free_idx < allocations.size() && !allocations[free_idx].in_use && allocations[free_idx].size >= required_size)
		{
			allocations[free_idx].in_use = true;
			allocations[free_idx].count = element_count;
			
			// Remove from free list
			auto it = std::find(free_list.begin(), free_list.end(), free_idx);
			if (it != free_list.end())
				free_list.erase(it);
			
			return {buffer_id, free_idx, allocations[free_idx].offset, element_count};
		}
	}
	
	// No suitable free allocation found, create a new one
	uint32_t offset = FindFreeSpace(required_size);
	if (offset + required_size > capacity)
	{
		// Need to resize buffer
		// For now, return invalid handle - proper resizing would be implemented here
		return ResourceHandle::INVALID;
	}
	
	BufferAllocation alloc;
	alloc.offset = offset;
	alloc.size = required_size;
	alloc.count = element_count;
	alloc.in_use = true;
	
	allocations.push_back(alloc);
	uint32_t allocation_id = static_cast<uint32_t>(allocations.size() - 1);
	
	current_size = std::max(current_size, offset + required_size);
	
	return {buffer_id, allocation_id, offset, element_count};
}

void GlobalBindlessManager::DynamicBuffer::Free(uint32_t allocation_id)
{
	if (allocation_id < allocations.size() && allocations[allocation_id].in_use)
	{
		allocations[allocation_id].in_use = false;
		free_list.push_back(allocation_id);
	}
}

void GlobalBindlessManager::DynamicBuffer::Update(uint32_t allocation_id, const void* data, uint32_t element_count)
{
	if (allocation_id >= allocations.size() || !allocations[allocation_id].in_use)
		return;
	
	const auto& alloc = allocations[allocation_id];
	uint32_t copy_size = std::min(element_count, alloc.count) * element_size;
	
	if (mapped_ptr && copy_size > 0)
	{
		uint8_t* dest = static_cast<uint8_t*>(mapped_ptr) + alloc.offset;
		std::memcpy(dest, data, copy_size);
	}
}

uint32_t GlobalBindlessManager::DynamicBuffer::FindFreeSpace(uint32_t required_size) const
{
	// Simple linear allocation for now
	// More sophisticated free space management could be implemented here
	return current_size;
}

//=========================================================================
// SETUP FUNCTIONS
//=========================================================================

//=========================================================================
// FACTORY FUNCTIONS
//=========================================================================

std::unique_ptr<GlobalBindlessManager> CreateGlobalBindlessManager(Device* device, uint32_t max_frames)
{
	if (!device)
		NFT_ERROR(VulkanFatal, "Device is null!");

	auto manager = std::make_unique<GlobalBindlessManager>(device, max_frames);
	manager->Init();
	return manager;
}

} // namespace nft::vulkan