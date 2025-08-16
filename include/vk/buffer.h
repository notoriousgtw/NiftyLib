#pragma once

#include "vk/Common.h"

namespace nft::vulkan
{
class Device;

struct Buffer
{
	vk::Buffer vk_buffer = VK_NULL_HANDLE;
	vk::DeviceMemory vk_memory = VK_NULL_HANDLE;
	vk::BufferCreateInfo vk_buffer_info;
	vk::MemoryAllocateInfo vk_memory_info;
};

class BufferManager
{
  public:
	BufferManager(Device* device);
	~BufferManager();

	Buffer* CreateBuffer(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
	void	DestroyBuffer(Buffer* buffer);
	void	CopyBuffer(Buffer* src_buffer, Buffer* dst_buffer, size_t size, vk::Queue queue, vk::CommandBuffer command_buffer);

  private:
	Device*								 device;
	std::vector<std::unique_ptr<Buffer>> managed_buffers;

	uint32_t FindMemoryType(uint32_t supported_memory_indices, vk::MemoryPropertyFlags requested_properties);
};
}