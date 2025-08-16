#pragma once

#include "vk/common.h"

#include "extern/stb_image.h"

namespace nft::vulkan
{

struct Image
{
  public:
	Image(Device* device);
	~Image() = default;

  protected:
	Device* device = nullptr;

	int						width, height, channels;
	vk::ImageTiling			tiling;
	vk::ImageUsageFlags		usage;
	vk::MemoryPropertyFlags memory_properites;

	char*	 file_path;
	stbi_uc* pixels = nullptr;

	// Resources
	vk::Image		 vk_image	   = VK_NULL_HANDLE;
	vk::DeviceMemory vk_memory	   = VK_NULL_HANDLE;
	vk::ImageView	 vk_image_view = VK_NULL_HANDLE;
	vk::Sampler		 vk_sampler	   = VK_NULL_HANDLE;

	// Resource Descriptors
	DescriptorSetLayout* descriptor_set_layout;
	DescriptorPool*		 descriptor_pool;
	vk::DescriptorSet	 vk_descriptor_set = VK_NULL_HANDLE;

	vk::CommandBuffer vk_command_buffer = VK_NULL_HANDLE;
	vk::Queue		  vk_queue			= VK_NULL_HANDLE;

	void SetupCommands(vk::CommandBuffer command_buffer, vk::Queue queue);
	void CreateImage(int					 width,
					 int					 height,
					 vk::ImageTiling		 tiling,
					 vk::ImageUsageFlags	 usage,
					 vk::MemoryPropertyFlags memory_properites);
	void TransistionLayout(vk::ImageLayout	 old_layout,
						   vk::ImageLayout	 new_layout);
	void CopyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
	void CreateImageView(vk::Format format);
};

class Texture: public Image
{
  public:
	Texture(Device* device): Image(device) {};
	~Texture() = default;

	void LoadFile(std::string file_path);
	void CreateDescriptorSet(DescriptorSetLayout* descriptor_set_layout, DescriptorPool* descriptor_pool);
	void Use();
};

}	 // namespace nft::vulkan
