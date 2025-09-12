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
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	// Create camera data buffer array for all frames
	camera_data_buffer = device->GetBufferManager()->CreateBuffer(
		sizeof(CameraData) * max_frames,
		vk::BufferUsageFlagBits::eStorageBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
	);
	
	if (!camera_data_buffer) {
		NFT_ERROR(VulkanFatal, "Failed to create camera data buffer!");
		return;
	}
	
	try {
		camera_data_ptr = device->GetDevice().mapMemory(
			camera_data_buffer->vk_memory, 0, sizeof(CameraData) * max_frames, vk::MemoryMapFlags{}
		);
		std::memset(camera_data_ptr, 0, sizeof(CameraData) * max_frames);
		
		if (logger) {
			logger->Debug(std::format("Camera data buffer created and mapped successfully (size: {} bytes)", 
				sizeof(CameraData) * max_frames), "VKResources");
		}
	} catch (const vk::SystemError& e) {
		NFT_ERROR(VulkanFatal, std::format("Failed to map camera data buffer: {}", e.what()));
		return;
	}

	// Initialize dynamic buffer managers with proper error checking
	try {
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
			
		if (logger) {
			logger->Debug("All buffer managers initialized successfully", "VKResources");
		}
	} catch (const std::exception& e) {
		NFT_ERROR(VulkanFatal, std::format("Failed to initialize buffer managers: {}", e.what()));
	}
}

void GlobalBindlessManager::UpdateCameraData(uint32_t frame_index, const CameraData& camera_data)
{
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (frame_index >= max_frames)
	{
		if (logger) {
			logger->Error(std::format("Frame index {} exceeds max frames {}", frame_index, max_frames), "VKResources");
		}
		return; // Don't throw error, just skip invalid update
	}
	
	if (!camera_data_ptr)
	{
		if (logger) {
			logger->Error("Camera data buffer not mapped!", "VKResources");
		}
		return; // Don't throw error, just skip update
	}

	// CRITICAL FIX: Add bounds checking and synchronization
	try {
		CameraData* camera_array = static_cast<CameraData*>(camera_data_ptr);
		camera_array[frame_index] = camera_data;
		
		// For HOST_COHERENT memory, writes are automatically visible to GPU
		// But add explicit memory barrier for safety on some drivers
		std::atomic_thread_fence(std::memory_order_release);
		
		// Only log every 60 frames to reduce spam
		static int debug_counter = 0;
		if (logger && debug_counter % 60 == 0) {
			logger->Debug(std::format("Updated camera data for frame {} (counter: {})", frame_index, debug_counter), "VKResources");
		}
		debug_counter++;
		
	} catch (const std::exception& e) {
		if (logger) {
			logger->Error(std::format("Failed to update camera data: {}", e.what()), "VKResources");
		}
	}
}

ResourceHandle GlobalBindlessManager::AllocateVertexData(const std::vector<graphics::Vertex>& vertices)
{
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (vertices.empty()) {
		if (logger) {
			logger->Warn("Attempting to allocate empty vertex data", "VKResources");
		}
		return ResourceHandle::INVALID;
	}
	
	ResourceHandle handle = vertex_buffer_manager.Allocate(static_cast<uint32_t>(vertices.size()), VERTEX_BUFFER);
	if (handle.IsValid())
	{
		vertex_buffer_manager.Update(handle.allocation_id, vertices.data(), static_cast<uint32_t>(vertices.size()));
		if (logger) {
			logger->Debug(std::format("Allocated vertex data: {} vertices at offset {}", handle.count, handle.offset), "VKResources");
		}
	}
	else
	{
		if (logger) {
			logger->Error(std::format("Failed to allocate vertex buffer space for {} vertices", vertices.size()), "VKResources");
		}
	}
	return handle;
}

ResourceHandle GlobalBindlessManager::AllocateIndexData(const std::vector<uint32_t>& indices)
{
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (indices.empty()) {
		if (logger) {
			logger->Warn("Attempting to allocate empty index data", "VKResources");
		}
		return ResourceHandle::INVALID;
	}
	
	ResourceHandle handle = index_buffer_manager.Allocate(static_cast<uint32_t>(indices.size()), INDEX_BUFFER);
	if (handle.IsValid())
	{
		index_buffer_manager.Update(handle.allocation_id, indices.data(), static_cast<uint32_t>(indices.size()));
		if (logger) {
			logger->Debug(std::format("Allocated index data: {} indices at offset {}", handle.count, handle.offset), "VKResources");
		}
	}
	else
	{
		if (logger) {
			logger->Error(std::format("Failed to allocate index buffer space for {} indices", indices.size()), "VKResources");
		}
	}
	return handle;
}

ResourceHandle GlobalBindlessManager::AllocateObjectTransforms(const std::vector<glm::mat4>& transforms)
{
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (transforms.empty()) {
		if (logger) {
			logger->Warn("Attempting to allocate empty transform data", "VKResources");
		}
		return ResourceHandle::INVALID;
	}
	
	ResourceHandle handle = object_transform_manager.Allocate(static_cast<uint32_t>(transforms.size()), TRANSFORM_BUFFER);
	if (handle.IsValid())
	{
		object_transform_manager.Update(handle.allocation_id, transforms.data(), static_cast<uint32_t>(transforms.size()));
		if (logger) {
			logger->Debug(std::format("Allocated transform data: {} transforms at offset {}", handle.count, handle.offset), "VKResources");
		}
	}
	else
	{
		if (logger) {
			logger->Error(std::format("Failed to allocate transform buffer space for {} transforms", transforms.size()), "VKResources");
		}
	}
	return handle;
}

ResourceHandle GlobalBindlessManager::AllocateMaterials(const std::vector<GPUMaterial>& materials)
{
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (materials.empty()) {
		if (logger) {
			logger->Warn("Attempting to allocate empty material data", "VKResources");
		}
		return ResourceHandle::INVALID;
	}
	
	ResourceHandle handle = material_manager.Allocate(static_cast<uint32_t>(materials.size()), MATERIAL_BUFFER);
	if (handle.IsValid())
	{
		material_manager.Update(handle.allocation_id, materials.data(), static_cast<uint32_t>(materials.size()));
		if (logger) {
			logger->Debug(std::format("Allocated material data: {} materials at offset {}", handle.count, handle.offset), "VKResources");
		}
	}
	else
	{
		if (logger) {
			logger->Error(std::format("Failed to allocate material buffer space for {} materials", materials.size()), "VKResources");
		}
	}
	return handle;
}

void GlobalBindlessManager::UpdateVertexData(const ResourceHandle& handle, const std::vector<graphics::Vertex>& vertices)
{
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (handle.buffer_id == VERTEX_BUFFER && handle.IsValid())
	{
		vertex_buffer_manager.Update(handle.allocation_id, vertices.data(), static_cast<uint32_t>(vertices.size()));
	}
	else
	{
		if (logger) {
			logger->Warn("Invalid vertex handle for update", "VKResources");
		}
	}
}

void GlobalBindlessManager::UpdateIndexData(const ResourceHandle& handle, const std::vector<uint32_t>& indices)
{
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (handle.buffer_id == INDEX_BUFFER && handle.IsValid())
	{
		index_buffer_manager.Update(handle.allocation_id, indices.data(), static_cast<uint32_t>(indices.size()));
	}
	else
	{
		if (logger) {
			logger->Warn("Invalid index handle for update", "VKResources");
		}
	}
}

void GlobalBindlessManager::UpdateObjectTransforms(const ResourceHandle& handle, const std::vector<glm::mat4>& transforms)
{
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (handle.buffer_id == TRANSFORM_BUFFER && handle.IsValid())
	{
		object_transform_manager.Update(handle.allocation_id, transforms.data(), static_cast<uint32_t>(transforms.size()));
	}
	else
	{
		if (logger) {
			logger->Warn("Invalid transform handle for update", "VKResources");
		}
	}
}

void GlobalBindlessManager::UpdateMaterials(const ResourceHandle& handle, const std::vector<GPUMaterial>& materials)
{
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (handle.buffer_id == MATERIAL_BUFFER && handle.IsValid())
	{
		material_manager.Update(handle.allocation_id, materials.data(), static_cast<uint32_t>(materials.size()));
	}
	else
	{
		if (logger) {
			logger->Warn("Invalid material handle for update", "VKResources");
		}
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
			if (alloc.in_use) {
				info.vertex_offset = alloc.offset / vertex_buffer_manager.element_size;
				info.vertex_count = alloc.count;
			}
		}
	}
	
	if (index_handle.IsValid() && index_handle.buffer_id == INDEX_BUFFER)
	{
		if (index_handle.allocation_id < index_buffer_manager.allocations.size())
		{
			const auto& alloc = index_buffer_manager.allocations[index_handle.allocation_id];
			if (alloc.in_use) {
				info.index_offset = alloc.offset / index_buffer_manager.element_size;
				info.index_count = alloc.count;
			}
		}
	}
	
	return info;
}

void GlobalBindlessManager::AllocateDescriptorSet(DescriptorPool* pool, DescriptorSetLayout* layout)
{
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (!pool || !layout)
	{
		NFT_ERROR(VulkanFatal, "Descriptor pool or layout is null!");
		return;
	}

	// Only allocate and update if not already allocated
	if (bindless_descriptor_set == VK_NULL_HANDLE)
	{
		try {
			// Allocate the bindless descriptor set through helper function
			bindless_descriptor_set = GetDescriptorSet(device, pool, layout);

			// Update the descriptor set with buffer info - ONLY ONCE during initialization
			UpdateDescriptorSet();
			
			if (logger) {
				logger->Debug("Allocated and updated bindless descriptor set", "VKResources");
			}
		} catch (const std::exception& e) {
			if (logger) {
				logger->Error(std::format("Failed to allocate descriptor set: {}", e.what()), "VKResources");
			}
			NFT_ERROR(VulkanFatal, std::format("Failed to allocate descriptor set: {}", e.what()));
		}
	}
	else
	{
		if (logger) {
			logger->Debug("Bindless descriptor set already allocated, skipping", "VKResources");
		}
	}
}

void GlobalBindlessManager::Cleanup()
{
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (device)
	{
		// CRITICAL FIX: Handle device lost errors during cleanup
		try {
			device->GetDevice().waitIdle();
			if (logger) {
				logger->Debug("Device idle completed for resource cleanup", "VKResources");
			}
		} catch (const vk::SystemError& e) {
			if (logger) {
				logger->Warn(std::format("Device idle wait failed during cleanup (continuing anyway): {}", e.what()), "VKResources");
			}
			// Continue with cleanup even if device is lost
		}
		
		// Helper lambda for safe buffer cleanup with error handling
		auto safe_cleanup_buffer = [this, logger](void*& mapped_ptr, Buffer*& buffer, const char* name) {
			if (mapped_ptr && buffer) {
				try {
					device->GetDevice().unmapMemory(buffer->vk_memory);
					if (logger) {
						logger->Debug(std::format("{} buffer unmapped", name), "VKResources");
					}
				} catch (const vk::SystemError& e) {
					if (logger) {
						logger->Warn(std::format("Failed to unmap {} buffer (device lost): {}", name, e.what()), "VKResources");
					}
				}
				mapped_ptr = nullptr;
			}
			
			if (buffer) {
				try {
					device->GetBufferManager()->DestroyBuffer(buffer);
					if (logger) {
						logger->Debug(std::format("{} buffer destroyed", name), "VKResources");
					}
				} catch (const vk::SystemError& e) {
					if (logger) {
						logger->Warn(std::format("Failed to destroy {} buffer (device lost): {}", name, e.what()), "VKResources");
					}
				}
				buffer = nullptr;
			}
		};
		
		// Cleanup camera buffer
		safe_cleanup_buffer(camera_data_ptr, camera_data_buffer, "camera");
		
		// Cleanup dynamic buffers with error handling
		auto cleanup_dynamic_buffer = [&safe_cleanup_buffer](DynamicBuffer& manager, const char* name) {
			safe_cleanup_buffer(manager.mapped_ptr, manager.buffer, name);
			manager.allocations.clear();
			manager.free_list.clear();
			manager.current_size = 0;
		};
		
		cleanup_dynamic_buffer(vertex_buffer_manager, "vertex");
		cleanup_dynamic_buffer(index_buffer_manager, "index");
		cleanup_dynamic_buffer(object_transform_manager, "transform");
		cleanup_dynamic_buffer(material_manager, "material");
		
		// Reset descriptor set handle
		bindless_descriptor_set = VK_NULL_HANDLE;
		
		if (logger) {
			logger->Debug("GlobalBindlessManager cleanup completed", "VKResources");
		}
	}
}

//=========================================================================
// PRIVATE HELPER METHODS
//=========================================================================

void GlobalBindlessManager::InitializeBufferManager(DynamicBuffer& manager, uint32_t element_size, uint32_t initial_capacity, 
													 vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	manager.element_size = element_size;
	manager.capacity = initial_capacity * element_size;
	manager.current_size = 0;
	
	manager.buffer = device->GetBufferManager()->CreateBuffer(
		manager.capacity, usage, properties
	);
	
	if (!manager.buffer) {
		NFT_ERROR(VulkanFatal, "Failed to create dynamic buffer!");
		return;
	}
	
	try {
		manager.mapped_ptr = device->GetDevice().mapMemory(
			manager.buffer->vk_memory, 0, manager.capacity, vk::MemoryMapFlags{}
		);
		
		std::memset(manager.mapped_ptr, 0, manager.capacity);
		if (logger) {
			logger->Debug(std::format("Initialized buffer manager: element_size={}, capacity={} bytes", 
				element_size, manager.capacity), "VKResources");
		}
	} catch (const vk::SystemError& e) {
		NFT_ERROR(VulkanFatal, std::format("Failed to map dynamic buffer: {}", e.what()));
	}
}

void GlobalBindlessManager::UpdateDescriptorSet()
{
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (bindless_descriptor_set == VK_NULL_HANDLE)
	{
		if (logger) {
			logger->Warn("UpdateDescriptorSet called but descriptor set not allocated", "VKResources");
		}
		return;
	}

	// Ensure all buffer managers are initialized before updating descriptors
	if (!camera_data_buffer || !object_transform_manager.buffer)
	{
		if (logger) {
			logger->Warn("UpdateDescriptorSet called but buffers not initialized", "VKResources");
		}
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
		if (logger) {
			logger->Debug(std::format("Successfully updated descriptor sets with {} descriptors", descriptor_writes.size()), "VKResources");
		}
	}
	catch (const vk::SystemError& e)
	{
		if (logger) {
			logger->Error(std::format("Failed to update descriptor sets: {}", e.what()), "VKResources");
		}
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
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (element_count == 0) {
		if (logger) {
			logger->Warn("Attempting to allocate 0 elements", "VKResources");
		}
		return ResourceHandle::INVALID;
	}
	
	uint32_t required_size = element_count * element_size;
	
	// BOUNDS CHECK: Ensure we don't exceed buffer capacity
	if (required_size > capacity) {
		if (logger) {
			logger->Error(std::format("Requested allocation ({} bytes) exceeds buffer capacity ({} bytes)", 
				required_size, capacity), "VKResources");
		}
		return ResourceHandle::INVALID;
	}
	
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
			
			if (logger) {
				logger->Debug(std::format("Reused allocation {}: {} elements at offset {}", 
					free_idx, element_count, allocations[free_idx].offset), "VKResources");
			}
			return {buffer_id, free_idx, allocations[free_idx].offset, element_count};
		}
	}
	
	// No suitable free allocation found, create a new one
	uint32_t offset = FindFreeSpace(required_size);
	if (offset + required_size > capacity)
	{
		if (logger) {
			logger->Error(std::format("Not enough space in buffer: need {} bytes at offset {}, capacity {}", 
				required_size, offset, capacity), "VKResources");
		}
		// TODO: Implement buffer resizing here
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
	
	if (logger) {
		logger->Debug(std::format("New allocation {}: {} elements ({} bytes) at offset {}", 
			allocation_id, element_count, required_size, offset), "VKResources");
	}
	return {buffer_id, allocation_id, offset, element_count};
}

void GlobalBindlessManager::DynamicBuffer::Free(uint32_t allocation_id)
{
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (allocation_id < allocations.size() && allocations[allocation_id].in_use)
	{
		allocations[allocation_id].in_use = false;
		free_list.push_back(allocation_id);
		if (logger) {
			logger->Debug(std::format("Freed allocation {}", allocation_id), "VKResources");
		}
	}
	else
	{
		if (logger) {
			logger->Warn(std::format("Attempted to free invalid allocation {}", allocation_id), "VKResources");
		}
	}
}

void GlobalBindlessManager::DynamicBuffer::Update(uint32_t allocation_id, const void* data, uint32_t element_count)
{
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (allocation_id >= allocations.size() || !allocations[allocation_id].in_use)
	{
		if (logger) {
			logger->Warn(std::format("Update called on invalid allocation {}", allocation_id), "VKResources");
		}
		return;
	}
	
	if (!data) {
		if (logger) {
			logger->Warn("Update called with null data pointer", "VKResources");
		}
		return;
	}
	
	const auto& alloc = allocations[allocation_id];
	uint32_t copy_size = std::min(element_count, alloc.count) * element_size;
	
	// BOUNDS CHECK: Ensure we don't write beyond allocated space
	if (alloc.offset + copy_size > capacity) {
		if (logger) {
			logger->Error(std::format("Update would exceed buffer capacity: offset={}, copy_size={}, capacity={}", 
				alloc.offset, copy_size, capacity), "VKResources");
		}
		return;
	}
	
	if (mapped_ptr && copy_size > 0)
	{
		try {
			uint8_t* dest = static_cast<uint8_t*>(mapped_ptr) + alloc.offset;
			std::memcpy(dest, data, copy_size);
			
			// Add memory barrier for safety on some drivers
			std::atomic_thread_fence(std::memory_order_release);
			
		} catch (const std::exception& e) {
			if (logger) {
				logger->Error(std::format("Failed to update buffer allocation {}: {}", allocation_id, e.what()), "VKResources");
			}
		}
	}
	else
	{
		if (logger) {
			logger->Warn(std::format("Buffer not mapped or zero copy size for allocation {}", allocation_id), "VKResources");
		}
	}
}

uint32_t GlobalBindlessManager::DynamicBuffer::FindFreeSpace(uint32_t required_size) const
{
	// IMPROVED: Better free space management to prevent fragmentation
	if (allocations.empty()) {
		return 0;
	}
	
	// Sort allocations by offset to find gaps
	std::vector<std::pair<uint32_t, uint32_t>> used_ranges; // {offset, end}
	for (const auto& alloc : allocations) {
		if (alloc.in_use) {
			used_ranges.push_back({alloc.offset, alloc.offset + alloc.size});
		}
	}
	
	if (used_ranges.empty()) {
		return 0;
	}
	
	std::sort(used_ranges.begin(), used_ranges.end());
	
	// Check for gaps between allocations
	uint32_t search_offset = 0;
	for (const auto& range : used_ranges) {
		if (range.first - search_offset >= required_size) {
			return search_offset; // Found a gap
		}
		search_offset = range.second;
	}
	
	// No gaps found, allocate at the end
	return search_offset;
}

//=========================================================================
// FACTORY FUNCTIONS
//=========================================================================

std::unique_ptr<GlobalBindlessManager> CreateGlobalBindlessManager(Device* device, uint32_t max_frames)
{
	if (!device)
		NFT_ERROR(VulkanFatal, "Device is null!");

	auto manager = std::make_unique<GlobalBindlessManager>(device, max_frames);
	try {
		manager->Init();
		return manager;
	} catch (const std::exception& e) {
		auto* app = vulkan::VulkanHandler::GetApp();
		auto* logger = app ? app->GetLogger() : nullptr;
		if (logger) {
			logger->Error(std::format("Failed to initialize GlobalBindlessManager: {}", e.what()), "VKResources");
		}
		return nullptr;
	}
}

} // namespace nft::vulkan