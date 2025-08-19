#include "vk/buffer.h"

#include "vk/commands.h"
#include "vk/handler.h"

namespace nft::vulkan
{
BufferManager::BufferManager(Device* device): device(device)
{
	if (!device)
		NFT_ERROR(VKFatal, "Device pointer is null in BufferManager constructor.");
}

BufferManager::~BufferManager()
{
	// Clear all buffers - let unique_ptr destructors handle cleanup
	for (auto& buffer : managed_buffers)
	{
		if (buffer && buffer->vk_buffer != VK_NULL_HANDLE)
		{
			try
			{
				// Check memory handle validity before freeing
				if (buffer->vk_memory != VK_NULL_HANDLE)
				{
					device->GetDevice().freeMemory(buffer->vk_memory);
					buffer->vk_memory = VK_NULL_HANDLE;
				}
				device->GetDevice().destroyBuffer(buffer->vk_buffer);
				buffer->vk_buffer = VK_NULL_HANDLE;
			}
			catch (const vk::SystemError& err)
			{
				NFT_ERROR(VKFatal, std::format("Failed To Destroy Buffer:\n{}", err.what()));
			}
		}
	}
	// Clear the vector after manual cleanup
	managed_buffers.clear();
}

Buffer* BufferManager::CreateBuffer(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
	// Create buffer on the heap to ensure proper initialization
	auto buffer = std::make_unique<Buffer>();
	
	// Ensure all handles are properly initialized to VK_NULL_HANDLE
	buffer->vk_buffer = VK_NULL_HANDLE;
	buffer->vk_memory = VK_NULL_HANDLE;
	
	buffer->vk_buffer_info = vk::BufferCreateInfo().setSize(size).setUsage(usage).setSharingMode(vk::SharingMode::eExclusive);

	try
	{
		buffer->vk_buffer = device->GetDevice().createBuffer(buffer->vk_buffer_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Create Buffer:\n{}", err.what()));
	}

	vk::MemoryRequirements memory_requirements = device->GetDevice().getBufferMemoryRequirements(buffer->vk_buffer);

	uint32_t memory_type_index = FindMemoryType(memory_requirements.memoryTypeBits, properties);

	buffer->vk_memory_info =
		vk::MemoryAllocateInfo().setAllocationSize(memory_requirements.size).setMemoryTypeIndex(memory_type_index);

	try
	{
		buffer->vk_memory = device->GetDevice().allocateMemory(buffer->vk_memory_info);
	}
	catch (const vk::SystemError& err)
	{
		// If memory allocation fails, clean up the buffer first
		if (buffer->vk_buffer != VK_NULL_HANDLE)
		{
			device->GetDevice().destroyBuffer(buffer->vk_buffer);
			buffer->vk_buffer = VK_NULL_HANDLE;
		}
		NFT_ERROR(VKFatal, std::format("Failed To Allocate Buffer Memory:\n{}", err.what()));
	}
	
	try
	{
		device->GetDevice().bindBufferMemory(buffer->vk_buffer, buffer->vk_memory, 0);
	}
	catch (const vk::SystemError& err)
	{
		// Clean up both buffer and memory if binding fails
		if (buffer->vk_memory != VK_NULL_HANDLE)
		{
			device->GetDevice().freeMemory(buffer->vk_memory);
			buffer->vk_memory = VK_NULL_HANDLE;
		}
		if (buffer->vk_buffer != VK_NULL_HANDLE)
		{
			device->GetDevice().destroyBuffer(buffer->vk_buffer);
			buffer->vk_buffer = VK_NULL_HANDLE;
		}
		NFT_ERROR(VKFatal, std::format("Failed To Bind Buffer Memory:\n{}", err.what()));
	}

	Buffer* buffer_ptr = buffer.get();
	managed_buffers.push_back(std::move(buffer));
	return buffer_ptr;
}

void BufferManager::DestroyBuffer(Buffer* buffer)
{
	if (!buffer)
		return;

	auto it = std::find_if(managed_buffers.begin(),
						   managed_buffers.end(),
						   [buffer](const std::unique_ptr<Buffer>& managed_buffer) { return managed_buffer.get() == buffer; });

	if (it != managed_buffers.end())
	{
		try
		{
			// Clean up Vulkan resources - check handles are valid before destroying
			if ((*it)->vk_memory != VK_NULL_HANDLE)
			{
				device->GetDevice().freeMemory((*it)->vk_memory);
				(*it)->vk_memory = VK_NULL_HANDLE;
			}
			if ((*it)->vk_buffer != VK_NULL_HANDLE)
			{
				device->GetDevice().destroyBuffer((*it)->vk_buffer);
				(*it)->vk_buffer = VK_NULL_HANDLE;
			}
		}
		catch (const vk::SystemError& err)
		{
			NFT_ERROR(VKFatal, std::format("Failed To Destroy Buffer:\n{}", err.what()));
		}

		// Remove from managed buffers
		managed_buffers.erase(it);
	}
}

void BufferManager::CopyBuffer(Buffer*			 src_buffer,
							   Buffer*			 dst_buffer,
							   size_t			 size,
							   vk::CommandBuffer command_buffer,
							   vk::Queue		 queue)
{
	if (!src_buffer || !dst_buffer || !src_buffer->vk_buffer || !dst_buffer->vk_buffer)
		NFT_ERROR(VKFatal, "Invalid buffer pointers in CopyBuffer");

	commands::StartJob(command_buffer, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	vk::BufferCopy copy_region = vk::BufferCopy().setSize(size);

	command_buffer.copyBuffer(src_buffer->vk_buffer, dst_buffer->vk_buffer, 1, &copy_region);

	commands::EndJob(command_buffer, queue);
}

uint32_t BufferManager::FindMemoryType(uint32_t supported_memory_indices, vk::MemoryPropertyFlags requested_properties)
{
	vk::PhysicalDeviceMemoryProperties supported_properties = device->GetPhysicalDevice().getMemoryProperties();

	for (uint32_t i = 0; i < supported_properties.memoryTypeCount; i++)
	{
		if ((supported_memory_indices & (1 << i)) &&
			(supported_properties.memoryTypes[i].propertyFlags & requested_properties) == requested_properties)
		{
			return i;	 // Return the first matching memory type index
		}
	}
	NFT_ERROR(VKFatal, "Failed To Find Suitable Memory Type: No memory type matches the requested properties.");
}

}	 // namespace nft::vulkan
