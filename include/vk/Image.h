#pragma once

#include "vk/common.h"

#include "extern/stb_image.h"

namespace nft::vulkan
{

struct Image
{
  public:
	Image(Device* device);
	~Image();

	// Disable copy
	Image(const Image&)			   = delete;
	Image& operator=(const Image&) = delete;

	// Enable move
	Image(Image&& other) noexcept;
	Image& operator=(Image&& other) noexcept;

	void Init(vk::ImageCreateInfo	  vk_image_info,
			  vk::MemoryPropertyFlags memory_properites,
			  vk::CommandBuffer		  command_buffer,
			  vk::Queue				  queue);
	void SetupCommands(vk::CommandBuffer command_buffer, vk::Queue queue);
	void UploadPixelData(const void* pixels, size_t size, vk::ImageLayout final_layout);

	void TransistionLayout(vk::ImageLayout old_layout, vk::ImageLayout new_layout);
	void CopyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
	void CreateImageView(vk::Format format);
	void Bind(vk::CommandBuffer		command_buffer,
			  vk::PipelineBindPoint bind_point,
			  vk::PipelineLayout	pipeline_layout,
			  uint32_t				set_index = 0);

  protected:
	Device*									 device = nullptr;

	int						width, height, channels;
	vk::ImageTiling			tiling;
	vk::ImageUsageFlags		usage;
	vk::MemoryPropertyFlags memory_properites;
	vk::CommandBuffer		vk_command_buffer	   = VK_NULL_HANDLE;
	vk::Queue				vk_queue			   = VK_NULL_HANDLE;
	bool					image_initialized	   = false;
	bool					commands_setup		   = false;
	bool					image_created		   = false;
	bool					image_view_created	   = false;
	bool					descriptor_set_created = false;

	char*	 file_path;
	stbi_uc* pixels = nullptr;

	// Resources
	vk::Image		 vk_image	   = VK_NULL_HANDLE;
	vk::DeviceMemory vk_memory	   = VK_NULL_HANDLE;
	vk::ImageView	 vk_image_view = VK_NULL_HANDLE;
	vk::Sampler		 vk_sampler	   = VK_NULL_HANDLE;

	// Resource Descriptors
	std::unique_ptr<DescriptorSetLayout> descriptor_set_layout;
	std::unique_ptr<DescriptorPool>		 descriptor_pool;
	vk::DescriptorSet					 vk_descriptor_set = VK_NULL_HANDLE;

	void AllocateDescriptorSet();

	friend class Scene;
};

class Texture: public Image
{
  public:
	Texture(Device* device): Image(device) {};
	~Texture() = default;

	// Disable copy
	Texture(const Texture&)			   = delete;
	Texture& operator=(const Texture&) = delete;

	// Enable move (forward to base)
	Texture(Texture&& other) noexcept			 = default;
	Texture& operator=(Texture&& other) noexcept = default;

	void LoadFile(std::string file_path);
	void CreateSampler(vk::SamplerCreateInfo sampler_info);
	void CreateDescriptorSet(vk::DescriptorSetLayout external_layout, vk::DescriptorPool external_pool);
	void Use(vk::CommandBuffer	   command_buffer,
			 vk::PipelineBindPoint bind_point,
			 vk::PipelineLayout	   pipeline_layout,
			 uint32_t			   set_index = 0);

  private:
	bool sampler_created = false;
	friend class Scene;
};

}	 // namespace nft::vulkan
