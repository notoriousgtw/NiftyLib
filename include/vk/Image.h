#pragma once

#include "vk/common.h"

#include "extern/stb_image.h"

namespace nft::vulkan
{

class Image
{
  public:
	Image() = default;
	Image(Device* device);
	virtual ~Image();

	// Disable copy
	Image(const Image&)			   = delete;
	Image& operator=(const Image&) = delete;

	// Enable move
	Image(Image&& other) noexcept;
	Image& operator=(Image&& other) noexcept;

	inline void SetSubresourceRange(vk::ImageSubresourceRange subresource_range) { vk_subresource_range = subresource_range; }
	inline void SetSubresourceLayers(vk::ImageSubresourceLayers subresource_layers)
	{
		vk_subresource_layers = subresource_layers;
	}

	void SetDevice(Device* device);

	void Init(vk::Image vk_image, vk::ImageCreateInfo vk_image_info);
	void Init(vk::ImageCreateInfo vk_image_info, vk::MemoryPropertyFlags memory_properties);
	void Init(vk::ImageCreateInfo	  vk_image_info,
			  vk::MemoryPropertyFlags memory_properites,
			  vk::CommandBuffer		  command_buffer,
			  vk::Queue				  queue);
	void InitMemory(vk::MemoryPropertyFlags memory_properites);
	void SetupCommands(vk::CommandBuffer command_buffer, vk::Queue queue);
	void UploadPixelData(const void* pixels, size_t size, vk::ImageLayout final_layout);

	void TransistionLayout(vk::ImageLayout old_layout, vk::ImageLayout new_layout);
	void CopyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
	void CreateImageView(vk::Format format);
	void Bind(vk::CommandBuffer		command_buffer,
			  vk::PipelineBindPoint bind_point,
			  vk::PipelineLayout	pipeline_layout,
			  uint32_t				set_index = 0);

	inline vk::ImageView& GetImageView() { return vk_image_view; }

  protected:
	Device* device = nullptr;

	int						width, height, channels;
	vk::ImageTiling			tiling;
	vk::ImageUsageFlags		usage;
	vk::Format				format = vk::Format::eUndefined;	// Default format, can be set by derived classes
	vk::MemoryPropertyFlags memory_properites;
	vk::CommandBuffer		vk_command_buffer	   = VK_NULL_HANDLE;
	vk::Queue				vk_queue			   = VK_NULL_HANDLE;
	bool					image_initialized	   = false;
	bool					memory_bound		   = false;
	bool					commands_setup		   = false;
	bool					image_created		   = false;
	bool					image_view_created	   = false;
	bool					descriptor_set_created = false;

	char*	 file_path;
	stbi_uc* pixels = nullptr;

	// Resources
	vk::Image				   vk_image = VK_NULL_HANDLE;
	vk::ImageCreateInfo		   vk_image_info;
	vk::ImageSubresourceRange  vk_subresource_range;
	vk::ImageSubresourceLayers vk_subresource_layers;
	vk::DeviceMemory		   vk_memory = VK_NULL_HANDLE;
	vk::MemoryRequirements	   vk_memory_requirements;
	vk::MemoryAllocateInfo	   vk_memory_allocate_info;
	vk::ImageView			   vk_image_view = VK_NULL_HANDLE;
	vk::ImageViewCreateInfo	   vk_image_view_info;
	vk::Sampler				   vk_sampler = VK_NULL_HANDLE;
	vk::SamplerCreateInfo	   vk_sampler_info;

	// Resource Descriptors
	std::unique_ptr<DescriptorSetLayout> descriptor_set_layout;
	std::unique_ptr<DescriptorPool>		 descriptor_pool;
	vk::DescriptorSet					 vk_descriptor_set = VK_NULL_HANDLE;
	vk::DescriptorSetAllocateInfo		 vk_descriptor_set_alloc_info;

	void AllocateDescriptorSet();

	friend class Scene;
};

class Texture: public Image
{
  public:
	Texture(Device* device): Image(device) {};
	~Texture() override;

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
	friend class Scene;
	friend class Surface;

  private:
	bool sampler_created = false;
	friend class Scene;
	friend class Surface;
};

vk::Format FindFormat(Device*						 device,
					  const std::vector<vk::Format>& candidates,
					  vk::ImageTiling				 tiling,
					  vk::FormatFeatureFlags		 features);
}	 // namespace nft::vulkan
