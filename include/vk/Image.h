#pragma once

#include "vk/NiftyVKCommon.h"
#include "vk/NiftyVKUtil.h"

#include "extern/stb_image.h"

namespace nft::vulkan
{
class Texture
{
  public:
	Texture(Device* device);
	~Texture() = default;

	void LoadFile(const char* file_path);
	void Use(vk::CommandBuffer command_buffer, vk::Queue queue);

  private:
	Device* device = nullptr;

	int		 width, height, channels;
	char*	 file_path;
	stbi_uc* pixels = nullptr;

	// Resources
	vk::Image		 vk_image	   = VK_NULL_HANDLE;
	vk::DeviceMemory vk_memory	   = VK_NULL_HANDLE;
	vk::ImageView	 vk_image_view = VK_NULL_HANDLE;
	vk::Sampler		 vk_sampler	   = VK_NULL_HANDLE;

	// Resource Descriptors
	DescriptorSetLayout descriptor_set_layout;
	DescriptorPool		descriptor_pool;
	vk::DescriptorSet	vk_descriptor_set = VK_NULL_HANDLE;

	vk::CommandBuffer vk_command_buffer = VK_NULL_HANDLE;
	vk::Queue		  vk_queue			= VK_NULL_HANDLE;
};
}	 // namespace nft::vulkan
