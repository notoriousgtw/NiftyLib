#pragma once

#include "core/error.h"

#include "vk/common.h"

namespace nft::vulkan::commands
{
void StartJob(vk::CommandBuffer command_buffer, vk::CommandBufferUsageFlags flags);
void EndJob(vk::CommandBuffer command_buffer, vk::Queue queue);
}	 // namespace nft::vulkan::commands
