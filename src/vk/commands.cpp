#include "vk/commands.h"

namespace nft::vulkan::commands
{

void StartJob(vk::CommandBuffer command_buffer, vk::CommandBufferUsageFlags flags)
{
	if (flags == vk::CommandBufferUsageFlagBits())
		flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	command_buffer.reset(vk::CommandBufferResetFlags());

	try
	{
		command_buffer.begin(vk::CommandBufferBeginInfo().setFlags(flags));
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Begin Command Buffer:\n{}", err.what()));
	}
}

void EndJob(vk::CommandBuffer command_buffer, vk::Queue queue)
{
	command_buffer.end();
	vk::SubmitInfo submit_info = vk::SubmitInfo().setCommandBufferCount(1).setPCommandBuffers(&command_buffer);
	try
	{
		queue.submit(1, &submit_info, nullptr);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Submit Command Buffer:\n{}", err.what()));
	}
	queue.waitIdle();
}

}	 // namespace nft::vulkan::commands