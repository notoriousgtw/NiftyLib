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
	for (auto& buffer : managed_buffers)
	{
		if (buffer->vk_buffer)
		{
			try
			{
				device->GetDevice().freeMemory(buffer->vk_memory);
				device->GetDevice().destroyBuffer(buffer->vk_buffer);
			}
			catch (const vk::SystemError& err)
			{
				NFT_ERROR(VKFatal, std::format("Failed To Destroy Buffer:\n{}", err.what()));
			}
		}
		managed_buffers.erase(std::remove(managed_buffers.begin(), managed_buffers.end(), buffer), managed_buffers.end());
	}
}

Buffer* BufferManager::CreateBuffer(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
	Buffer buffer;
	buffer.vk_buffer_info = vk::BufferCreateInfo().setSize(size).setUsage(usage).setSharingMode(vk::SharingMode::eExclusive);

	try
	{
		buffer.vk_buffer = device->GetDevice().createBuffer(buffer.vk_buffer_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Create Buffer:\n{}", err.what()));
	}

	vk::MemoryRequirements memory_requirements =
		device->GetDevice().getBufferMemoryRequirements(buffer.vk_buffer);

	uint32_t memory_type_index = FindMemoryType(memory_requirements.memoryTypeBits, properties);

	buffer.vk_memory_info =
		vk::MemoryAllocateInfo().setAllocationSize(memory_requirements.size).setMemoryTypeIndex(memory_type_index);

	try
	{
		buffer.vk_memory = device->GetDevice().allocateMemory(buffer.vk_memory_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Allocate Buffer Memory:\n{}", err.what()));
	}
	device->GetDevice().bindBufferMemory(buffer.vk_buffer, buffer.vk_memory, 0);

	managed_buffers.push_back(std::make_unique<Buffer>(std::move(buffer)));
	return managed_buffers.back().get();
}

void BufferManager::DestroyBuffer(Buffer* buffer)
{
	for (auto& managed_buffer : managed_buffers)
	{
		if (managed_buffer.get() == buffer)
		{
			if (managed_buffer->vk_buffer)
			{
				try
				{
					device->GetDevice().freeMemory(managed_buffer->vk_memory);
					device->GetDevice().destroyBuffer(managed_buffer->vk_buffer);
				}
				catch (const vk::SystemError& err)
				{
					NFT_ERROR(VKFatal, std::format("Failed To Destroy Buffer:\n{}", err.what()));
				}
			}
			managed_buffers.erase(std::remove(managed_buffers.begin(), managed_buffers.end(), managed_buffer),
								  managed_buffers.end());
			return;
		}
	}
}

void BufferManager::CopyBuffer(Buffer*			 src_buffer,
							   Buffer*			 dst_buffer,
							   size_t			 size,
							   vk::CommandBuffer command_buffer,
							   vk::Queue		 queue)
{
	commands::StartJob(command_buffer, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	vk::BufferCopy copy_region = vk::BufferCopy().setSize(size);

	command_buffer.copyBuffer(src_buffer->vk_buffer, dst_buffer->vk_buffer, 1, &copy_region);

	commands::EndJob(command_buffer, queue);
}

uint32_t BufferManager::FindMemoryType(uint32_t supported_memory_indices, vk::MemoryPropertyFlags requested_properties)
{
	vk::PhysicalDeviceMemoryProperties supported_properties =
		device->GetPhysicalDevice().getMemoryProperties();

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
