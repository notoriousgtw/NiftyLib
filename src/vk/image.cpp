#include "vk/image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "extern/stb_image.h"

#include "vk/commands.h"
#include "vk/handler.h"

namespace nft::vulkan
{

Image::Image(Device* device): device(device), width(0), height(0), channels(0), pixels(nullptr)
{
	if (!device)
		NFT_ERROR(VKFatal, "Device is null!");
}

Image::~Image()
{
	if (pixels)
	{
		stbi_image_free(pixels);
		pixels = nullptr;
	}
	if (vk_image_view && device && device->vk_device)
	{
		device->vk_device.destroyImageView(vk_image_view);
		vk_image_view = VK_NULL_HANDLE;
	}
	if (vk_image && device && device->vk_device)
	{
		device->vk_device.destroyImage(vk_image);
		vk_image = VK_NULL_HANDLE;
	}
	if (vk_memory && device && device->vk_device)
	{
		device->vk_device.freeMemory(vk_memory);
		vk_memory = VK_NULL_HANDLE;
	}
}

Image::Image(Image&& other) noexcept:
	device(other.device),
	width(other.width),
	height(other.height),
	channels(other.channels),
	tiling(other.tiling),
	usage(other.usage),
	memory_properites(other.memory_properites),
	vk_command_buffer(other.vk_command_buffer),
	vk_queue(other.vk_queue),
	image_initialized(other.image_initialized),
	commands_setup(other.commands_setup),
	image_created(other.image_created),
	image_view_created(other.image_view_created),
	descriptor_set_created(other.descriptor_set_created),
	file_path(other.file_path),
	pixels(other.pixels),
	vk_image(other.vk_image),
	vk_memory(other.vk_memory),
	vk_image_view(other.vk_image_view),
	vk_sampler(other.vk_sampler),
	descriptor_set_layout(std::move(other.descriptor_set_layout)),
	descriptor_pool(std::move(other.descriptor_pool)),
	vk_descriptor_set(other.vk_descriptor_set)
{
	// Null / reset source so its destructor is safe
	other.pixels				 = nullptr;
	other.vk_image				 = VK_NULL_HANDLE;
	other.vk_memory				 = VK_NULL_HANDLE;
	other.vk_image_view			 = VK_NULL_HANDLE;
	other.vk_sampler			 = VK_NULL_HANDLE;
	other.vk_descriptor_set		 = VK_NULL_HANDLE;
	other.image_initialized		 = false;
	other.commands_setup		 = false;
	other.image_created			 = false;
	other.image_view_created	 = false;
	other.descriptor_set_created = false;
}

Image& Image::operator=(Image&& other) noexcept
{
	if (this == &other)
		return *this;

	// First cleanup current resources
	this->~Image();

	// Reconstruct via placement new using move ctor then return *this
	new (this) Image(std::move(other));
	return *this;
}

void Image::SetDevice(Device* device)
{
	if (!device)
		NFT_ERROR(VKFatal, "Device is null!");
	this->device = device;
}

void Image::Init(vk::Image vk_image, vk::ImageCreateInfo vk_image_info)
{
	if (!device)
		NFT_ERROR(VKFatal, "Device is null!");
	if (!vk_image)
		NFT_ERROR(VKFatal, "Vulkan image handle is null!");
	this->vk_image = vk_image;
	
	this->vk_image_info = vk_image_info;
	this->width			= vk_image_info.extent.width;
	this->height		= vk_image_info.extent.height;
	if (width <= 0 || height <= 0)
		NFT_ERROR(VKFatal, "Width and height must be greater than zero!");

	this->tiling = vk_image_info.tiling;
	if (tiling != vk::ImageTiling::eOptimal && tiling != vk::ImageTiling::eLinear)
		NFT_ERROR(VKFatal, "Invalid image tiling specified!");

	this->usage = vk_image_info.usage;

	if (vk_subresource_range == vk::ImageSubresourceRange())
		vk_subresource_range = vk::ImageSubresourceRange()
								   .setAspectMask(vk::ImageAspectFlagBits::eColor)
								   .setBaseMipLevel(0)
								   .setLevelCount(1)
								   .setBaseArrayLayer(0)
								   .setLayerCount(1);

	if (vk_subresource_layers == vk::ImageSubresourceLayers())
		vk_subresource_layers = vk::ImageSubresourceLayers()
									.setAspectMask(vk::ImageAspectFlagBits::eColor)
									.setMipLevel(0)
									.setBaseArrayLayer(0)
									.setLayerCount(1);

	image_created	  = true;
	image_initialized = true;
}

void Image::Init(vk::ImageCreateInfo vk_image_info, vk::MemoryPropertyFlags memory_properties)
{
	if (!device)
		NFT_ERROR(VKFatal, "Device is null!");

	this->vk_image_info = vk_image_info;
	this->width			= vk_image_info.extent.width;
	this->height		= vk_image_info.extent.height;
	if (width <= 0 || height <= 0)
		NFT_ERROR(VKFatal, "Width and height must be greater than zero!");

	this->tiling = vk_image_info.tiling;
	if (tiling != vk::ImageTiling::eOptimal && tiling != vk::ImageTiling::eLinear)
		NFT_ERROR(VKFatal, "Invalid image tiling specified!");

	this->usage = vk_image_info.usage;

	if (vk_image_info.mipLevels < 1)
		vk_image_info.mipLevels = 1;	// Ensure at least one mip level

	if (vk_image_info.arrayLayers < 1)
		vk_image_info.arrayLayers = 1;	  // Ensure at least one array layer

	try
	{
		vk_image = device->vk_device.createImage(vk_image_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Create Image:\n{}", err.what()));
	}

	if (vk_subresource_range == vk::ImageSubresourceRange())
		vk_subresource_range = vk::ImageSubresourceRange()
								   .setAspectMask(vk::ImageAspectFlagBits::eColor)
								   .setBaseMipLevel(0)
								   .setLevelCount(1)
								   .setBaseArrayLayer(0)
								   .setLayerCount(1);

	if (vk_subresource_layers == vk::ImageSubresourceLayers())
		vk_subresource_layers = vk::ImageSubresourceLayers()
									.setAspectMask(vk::ImageAspectFlagBits::eColor)
									.setMipLevel(0)
									.setBaseArrayLayer(0)
									.setLayerCount(1);

	image_initialized = true;

	if (memory_properties != vk::MemoryPropertyFlagBits())
		InitMemory(memory_properties);
	else
		image_created = true;
}

void Image::Init(vk::ImageCreateInfo	 vk_image_info,
				 vk::MemoryPropertyFlags memory_properties,
				 vk::CommandBuffer		 command_buffer,
				 vk::Queue				 queue)
{
	Init(vk_image_info, memory_properties);
	SetupCommands(command_buffer, queue);
}

void Image::InitMemory(vk::MemoryPropertyFlags memory_properties)
{

	vk_memory_requirements = device->vk_device.getImageMemoryRequirements(vk_image);

	vk_memory_allocate_info =
		vk::MemoryAllocateInfo()
			.setAllocationSize(vk_memory_requirements.size)
			.setMemoryTypeIndex(device->buffer_manager->FindMemoryType(vk_memory_requirements.memoryTypeBits, memory_properties));
	try
	{
		vk_memory = device->vk_device.allocateMemory(vk_memory_allocate_info);
		device->vk_device.bindImageMemory(vk_image, vk_memory, 0);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Allocate Image Memory:\n{}", err.what()));
	}
	memory_bound = true;
}

void Image::SetupCommands(vk::CommandBuffer command_buffer, vk::Queue queue)
{
	if (!command_buffer)
		NFT_ERROR(VKFatal, "Command buffer is null!");
	if (!queue)
		NFT_ERROR(VKFatal, "Queue is null!");
	vk_command_buffer = command_buffer;
	vk_queue		  = queue;
	commands_setup	  = true;
}

void Image::UploadPixelData(const void* pixels, size_t size, vk::ImageLayout final_layout)
{
	if (!image_initialized)
		NFT_ERROR(VKFatal, "Image is not initialized! Call Init() before uploading pixel data.");
	if (!pixels)
		NFT_ERROR(VKFatal, "Pixel data is null!");
	if (size < 1)
		NFT_ERROR(VKFatal, "Size of pixel data must be greater than zero!");
	// Create staging buffer
	Buffer* staging_buffer = device->buffer_manager->CreateBuffer(size,
																  vk::BufferUsageFlagBits::eTransferSrc,
																  vk::MemoryPropertyFlagBits::eHostCoherent |
																	  vk::MemoryPropertyFlagBits::eHostVisible);

	// Map memory and copy pixel data
	void* write_ptr = device->vk_device.mapMemory(
		staging_buffer->vk_memory, 0, staging_buffer->vk_memory_info.allocationSize, vk::MemoryMapFlags());
	if (pixels && size > 0)
		std::memcpy(write_ptr, pixels, size);
	device->vk_device.unmapMemory(staging_buffer->vk_memory);

	// Transition to transfer destination layout
	TransistionLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

	// Copy buffer to image
	CopyBufferToImage(staging_buffer->vk_buffer, vk_image, width, height);

	// Transition to shader read-only layout
	TransistionLayout(vk::ImageLayout::eTransferDstOptimal, final_layout);

	// Cleanup staging buffer
	device->buffer_manager->DestroyBuffer(staging_buffer);

	image_created = true;
	CreateImageView(vk::Format::eR8G8B8A8Unorm);
}

void Image::AllocateDescriptorSet()
{
	if (!image_initialized)
		NFT_ERROR(VKFatal, "Image is not initialized! Call Init() before allocating a descriptor set.");
	if (!descriptor_set_layout)
		NFT_ERROR(VKFatal, "Descriptor set layout is null!");
	if (!descriptor_pool)
		NFT_ERROR(VKFatal, "Descriptor pool is null!");
	vk_descriptor_set_alloc_info = vk::DescriptorSetAllocateInfo()
									   .setDescriptorPool(descriptor_pool->vk_descriptor_pool)
									   .setDescriptorSetCount(1)
									   .setPSetLayouts(&descriptor_set_layout->vk_descriptor_set_layout);
	try
	{
		vk_descriptor_set = device->vk_device.allocateDescriptorSets(vk_descriptor_set_alloc_info)[0];
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Allocate Descriptor Set:\n{}", err.what()));
	}
}

void Image::TransistionLayout(vk::ImageLayout old_layout, vk::ImageLayout new_layout)
{
	if (!image_initialized)
		NFT_ERROR(VKFatal, "Image is not initialized! Call Init() before transitioning layout.");

	commands::StartJob(vk_command_buffer, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	vk::ImageMemoryBarrier image_memory_barrier = vk::ImageMemoryBarrier()
													  .setOldLayout(old_layout)
													  .setNewLayout(new_layout)
													  .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
													  .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
													  .setImage(vk_image)
													  .setSubresourceRange(vk_subresource_range);

	vk::PipelineStageFlags src_stage;
	vk::PipelineStageFlags dst_stage;

	if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal)
	{
		image_memory_barrier.setSrcAccessMask(vk::AccessFlagBits::eNone).setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
		src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
		dst_stage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		image_memory_barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
		src_stage = vk::PipelineStageFlagBits::eTransfer;
		dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else
		NFT_ERROR(VKFatal, "Unsupported layout transition!");

	vk_command_buffer.pipelineBarrier(
		src_stage, dst_stage, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &image_memory_barrier);

	commands::EndJob(vk_command_buffer, vk_queue);
}

void Image::CopyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
{
	commands::StartJob(vk_command_buffer, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	vk::BufferImageCopy buffer_image_copy = vk::BufferImageCopy()
												.setBufferOffset(0)
												.setBufferRowLength(0)
												.setBufferImageHeight(0)
												.setImageSubresource(vk_subresource_layers)
												.setImageOffset(vk::Offset3D(0, 0, 0))
												.setImageExtent(vk::Extent3D(width, height, 1));

	vk_command_buffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &buffer_image_copy);

	commands::EndJob(vk_command_buffer, vk_queue);
}

void Image::CreateImageView(vk::Format format)
{
	if (!image_initialized)
		NFT_ERROR(VKFatal, "Image is not initialized! Call Init() before creating an image view.");
	if (image_view_created)
		NFT_ERROR(VKFatal, "Image view is already created! Call Cleanup() before creating a new image view.");

	vk_image_view_info = vk::ImageViewCreateInfo()
							 .setFlags(vk::ImageViewCreateFlags())
							 .setImage(vk_image)
							 .setViewType(vk::ImageViewType::e2D)
							 .setFormat(format)
							 .setComponents(vk::ComponentMapping())
							 .setSubresourceRange(vk_subresource_range);
	try
	{
		vk_image_view	   = device->vk_device.createImageView(vk_image_view_info);
		this->format	   = format;	// Store the format for later use
		image_view_created = true;
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Create Image View:\n{}", err.what()));
	}
}

void Image::Bind(vk::CommandBuffer	   command_buffer,
				 vk::PipelineBindPoint bind_point,
				 vk::PipelineLayout	   pipeline_layout,
				 uint32_t			   set_index)
{
	if (!descriptor_set_created)
		NFT_ERROR(VKFatal, "Descriptor set is not created! Derived classes are responsible for creating descriptor_sets.");

	command_buffer.bindDescriptorSets(bind_point, pipeline_layout, set_index, vk_descriptor_set, nullptr);
}

Texture::~Texture()
{
	device->vk_device.destroySampler(vk_sampler);
	vk_sampler		= VK_NULL_HANDLE;
	sampler_created = false;
}

void Texture::LoadFile(std::string file_path)
{
	if (!commands_setup)
		NFT_ERROR(VKFatal, "Commands are not setup! Call SetupCommands() before loading a file.");

	pixels = stbi_load(file_path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	if (!pixels)
		NFT_ERROR(VKFatal, "Failed To Load Image File: " + file_path);

	Init(vk::ImageCreateInfo()
			 .setFlags(vk::ImageCreateFlagBits())
			 .setImageType(vk::ImageType::e2D)
			 .setExtent(vk::Extent3D(width, height, 1))
			 .setMipLevels(1)
			 .setArrayLayers(1)
			 .setFormat(vk::Format::eR8G8B8A8Unorm)
			 .setTiling(vk::ImageTiling::eOptimal)
			 .setInitialLayout(vk::ImageLayout::eUndefined)
			 .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
			 .setSharingMode(vk::SharingMode::eExclusive)
			 .setSamples(vk::SampleCountFlagBits::e1),
		 vk::MemoryPropertyFlagBits::eDeviceLocal,
		 vk_command_buffer,
		 vk_queue);

	UploadPixelData(pixels, width * height * 4, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void Texture::CreateSampler(vk::SamplerCreateInfo sampler_info)
{
	// vk::SamplerCreateInfo()
	//										 .setMagFilter(vk::Filter::eLinear)
	//										 .setMinFilter(vk::Filter::eLinear)
	//										 .setAddressModeU(vk::SamplerAddressMode::eRepeat)
	//										 .setAddressModeV(vk::SamplerAddressMode::eRepeat)
	//										 .setAddressModeW(vk::SamplerAddressMode::eRepeat)
	//										 .setAnisotropyEnable(VK_FALSE)
	//										 .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
	//										 .setUnnormalizedCoordinates(VK_FALSE)
	//										 .setCompareEnable(VK_FALSE)
	//										 .setCompareOp(vk::CompareOp::eAlways)
	//										 .setMipmapMode(vk::SamplerMipmapMode::eLinear)
	//										 .setMipLodBias(0.0f)
	//										 .setMinLod(0.0f)
	//										 .setMaxLod(0.0);
	try
	{
		vk_sampler		= device->vk_device.createSampler(sampler_info);
		sampler_created = true;
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Create Sampler:\n{}", err.what()));
	}
}

void Texture::CreateDescriptorSet(vk::DescriptorSetLayout external_layout, vk::DescriptorPool external_pool)
{
	if (!sampler_created)
		NFT_ERROR(VKFatal, "Sampler is not created! Call CreateSampler() before creating a descriptor set.");
	if (!image_created)
		NFT_ERROR(VKFatal, "Image is not created! Call UploadPixelData() before creating a descriptor set.");
	if (!image_view_created)
		NFT_ERROR(VKFatal, "Image view is not created! Call CreateImageView() before creating a descriptor set.");
	if (!external_layout)
		NFT_ERROR(VKFatal, "Descriptor set layout is null!");
	if (!external_pool)
		NFT_ERROR(VKFatal, "Descriptor pool is null!");

	vk::DescriptorSetAllocateInfo alloc_info = vk::DescriptorSetAllocateInfo()
												   .setDescriptorPool(external_pool)
												   .setDescriptorSetCount(1)
												   .setPSetLayouts(&external_layout);

	try
	{
		vk_descriptor_set = device->vk_device.allocateDescriptorSets(alloc_info)[0];
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Allocate Texture Descriptor Set:\n{}", err.what()));
	}

	vk::DescriptorImageInfo image_info = vk::DescriptorImageInfo()
											 .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
											 .setImageView(vk_image_view)
											 .setSampler(vk_sampler);
	vk::WriteDescriptorSet write_set = vk::WriteDescriptorSet()
										   .setDstSet(vk_descriptor_set)
										   .setDstBinding(0)	// Assuming binding 0 for texture
										   .setDstArrayElement(0)
										   .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
										   .setDescriptorCount(1)
										   .setPImageInfo(&image_info);

	device->vk_device.updateDescriptorSets(write_set, nullptr);
	descriptor_set_created = true;
}

void Texture::Use(vk::CommandBuffer		command_buffer,
				  vk::PipelineBindPoint bind_point,
				  vk::PipelineLayout	pipeline_layout,
				  uint32_t				set_index)
{
	if (!descriptor_set_created)
		NFT_ERROR(VKFatal, "Descriptor set is not created! Call CreateDescriptorSet() before using the texture.");
	Bind(command_buffer, bind_point, pipeline_layout, set_index);
}

vk::Format
	FindFormat(Device* device, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
	for (const auto& format : candidates)
	{
		vk::FormatProperties props = device->GetPhysicalDevice().getFormatProperties(format);
		if ((tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) ||
			(tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features))
		{
			return format;
		}
	}
	return vk::Format::eUndefined;	  // Not found
}

}	 // namespace nft::vulkan
