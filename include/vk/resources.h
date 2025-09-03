#pragma once

#include "vk/common.h"
#include "vk/buffer.h"
#include "graphics/mesh.h"

namespace nft::vulkan
{

// Forward declarations
class Device;
struct DescriptorPool;
struct DescriptorSetLayout;

//=========================================================================
// BUFFER ALLOCATION STRUCTURES
//=========================================================================

// Dynamic buffer allocation info
struct BufferAllocation
{
	uint32_t offset;        // Offset in the buffer
	uint32_t size;          // Size of the allocation in bytes
	uint32_t count;         // Number of elements
	bool in_use = false;    // Whether this allocation is currently in use
};

// Resource handle for tracking allocations
struct ResourceHandle
{
	uint32_t buffer_id;     // Which buffer type (vertex, index, etc.)
	uint32_t allocation_id; // Index into the allocation array
	uint32_t offset;        // Byte offset in buffer
	uint32_t count;         // Number of elements
	
	// Invalid handle marker
	static const ResourceHandle INVALID;
	bool IsValid() const { return allocation_id != UINT32_MAX; }
};

//=========================================================================
// CAMERA AND MATERIAL DATA STRUCTURES
//=========================================================================

// Camera data structure
struct CameraData
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec3 pos;
	float padding; // Align to 16 bytes
};

// GPU material structure
struct GPUMaterial
{
	glm::vec3 ambient_color;
	uint32_t  ambient_texture_index;
	glm::vec3 diffuse_color;
	uint32_t  diffuse_texture_index;
	glm::vec3 specular_color;
	uint32_t  specular_texture_index;
	float     specular_intensity;
	float     padding[3]; // Align to 16 bytes
};

//=========================================================================
// GLOBAL BINDLESS RESOURCE MANAGER
//=========================================================================

class GlobalBindlessManager
{
public:
	GlobalBindlessManager(Device* device, uint32_t max_frames);
	~GlobalBindlessManager();
	
	void Init();
	void Cleanup();
	
	// Camera data management (per-frame)
	void UpdateCameraData(uint32_t frame_index, const CameraData& camera_data);
	
	// Dynamic allocation methods
	ResourceHandle AllocateVertexData(const std::vector<graphics::Vertex>& vertices);
	ResourceHandle AllocateIndexData(const std::vector<uint32_t>& indices);
	ResourceHandle AllocateObjectTransforms(const std::vector<glm::mat4>& transforms);
	ResourceHandle AllocateMaterials(const std::vector<GPUMaterial>& materials);
	
	// Update methods for existing allocations
	void UpdateVertexData(const ResourceHandle& handle, const std::vector<graphics::Vertex>& vertices);
	void UpdateIndexData(const ResourceHandle& handle, const std::vector<uint32_t>& indices);
	void UpdateObjectTransforms(const ResourceHandle& handle, const std::vector<glm::mat4>& transforms);
	void UpdateMaterials(const ResourceHandle& handle, const std::vector<GPUMaterial>& materials);
	
	// Free allocations
	void FreeVertexData(const ResourceHandle& handle);
	void FreeIndexData(const ResourceHandle& handle);
	void FreeObjectTransforms(const ResourceHandle& handle);
	void FreeMaterials(const ResourceHandle& handle);
	
	// Descriptor set management for multiple pipelines
	vk::DescriptorSet GetBindlessDescriptorSet() const { return bindless_descriptor_set; }
	void AllocateDescriptorSet(DescriptorPool* pool, DescriptorSetLayout* layout);
	
	// Get buffer info for external binding (traditional vertex input compatibility)
	vk::Buffer GetVertexBuffer() const;
	vk::Buffer GetIndexBuffer() const;
	
	// Get allocation info for draw commands
	struct DrawInfo
	{
		uint32_t vertex_offset;
		uint32_t index_offset;
		uint32_t vertex_count;
		uint32_t index_count;
	};
	
	DrawInfo GetDrawInfo(const ResourceHandle& vertex_handle, const ResourceHandle& index_handle) const;

private:
	Device* device;
	uint32_t max_frames;
	
	// Dynamic buffer management
	struct DynamicBuffer
	{
		Buffer* buffer = nullptr;
		void* mapped_ptr = nullptr;
		uint32_t current_size = 0;      // Current used size in bytes
		uint32_t capacity = 0;          // Maximum capacity in bytes
		uint32_t element_size = 0;      // Size of each element
		std::vector<BufferAllocation> allocations;
		std::vector<uint32_t> free_list; // Indices of free allocations
		
		void Resize(Device* device, uint32_t new_capacity, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
		ResourceHandle Allocate(uint32_t element_count, uint32_t buffer_id);
		void Free(uint32_t allocation_id);
		void Update(uint32_t allocation_id, const void* data, uint32_t element_count);
		uint32_t FindFreeSpace(uint32_t required_size) const;
	};
	
	// Buffer type IDs
	enum BufferType : uint32_t
	{
		VERTEX_BUFFER = 0,
		INDEX_BUFFER = 1,
		TRANSFORM_BUFFER = 2,
		MATERIAL_BUFFER = 3
	};
	
	// Fixed camera buffer (per-frame data)
	Buffer* camera_data_buffer = nullptr;
	void* camera_data_ptr = nullptr;
	
	// Dynamic buffers
	DynamicBuffer vertex_buffer_manager;
	DynamicBuffer index_buffer_manager;
	DynamicBuffer object_transform_manager;
	DynamicBuffer material_manager;
	
	// Single descriptor set shared by all pipelines
	vk::DescriptorSet bindless_descriptor_set = VK_NULL_HANDLE;
	
	// Initial buffer sizes (will grow as needed)
	static constexpr uint32_t INITIAL_VERTEX_COUNT = 4096;
	static constexpr uint32_t INITIAL_INDEX_COUNT = 12288;
	static constexpr uint32_t INITIAL_TRANSFORM_COUNT = 256;
	static constexpr uint32_t INITIAL_MATERIAL_COUNT = 64;
	
	// Helper methods
	void InitializeBufferManager(DynamicBuffer& manager, uint32_t element_size, uint32_t initial_capacity, 
								 vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
	void UpdateDescriptorSet();
	DynamicBuffer& GetBufferManager(BufferType type);
	const DynamicBuffer& GetBufferManager(BufferType type) const;
};

//=============================================================================
// CONSTANTS AND SETUP FUNCTIONS
//=========================================================================

// Maximum number of frames in flight
constexpr uint32_t MAX_FRAMES = 8;

// Setup function for bindless vertex buffer system
void SetupBindlessVertexBufferSystem(Device* device, 
                                   uint32_t max_frames,
                                   std::unique_ptr<DescriptorPool>& bindless_descriptor_pool,
                                   std::unique_ptr<DescriptorSetLayout>& bindless_descriptor_layout,
                                   std::unique_ptr<GlobalBindlessManager>& global_resource_manager);

//=========================================================================
// FACTORY FUNCTIONS
//=========================================================================

// Creates and initializes a global bindless resource manager
std::unique_ptr<GlobalBindlessManager> CreateGlobalBindlessManager(Device* device, uint32_t max_frames);

} // namespace nft::vulkan