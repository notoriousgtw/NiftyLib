#include "vk/image.h"

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
		device->vk_device.destroyImageView(vk_image_view, nullptr, instance->dispatch_loader_dynamic);
		vk_image_view = VK_NULL_HANDLE;
	}
	if (vk_image && device && device->vk_device)
	{
		device->vk_device.destroyImage(vk_image, nullptr, instance->dispatch_loader_dynamic);
		vk_image = VK_NULL_HANDLE;
	}
	if (vk_memory && device && device->vk_device)
	{
		device->vk_device.freeMemory(vk_memory, nullptr, instance->dispatch_loader_dynamic);
		vk_memory = VK_NULL_HANDLE;
	}
}

void Image::CreateImage(int						width,
						int						height,
						vk::ImageTiling			tiling,
						vk::ImageUsageFlags		usage,
						vk::MemoryPropertyFlags memory_properites)
{
	this->width = width;
	this->height = height;
	this->tiling = tiling;
	this->usage	 = usage;
	this->memory_properites = memory_properites;
}

void Texture::LoadFile(std::string file_path)
{

	CreateImage(width, height, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);

	// TODO Poluplate image

	CreateImageView(vk::Format::eR8G8B8A8Srgb);
}

void Texture::CreateDescriptorSet(DescriptorSetLayout* descriptor_set_layout, DescriptorPool* descriptor_pool)
{
	this->descriptor_set_layout = descriptor_set_layout;
	this->descriptor_pool		= descriptor_pool;
	if (!descriptor_set_layout || !descriptor_pool)
		NFT_ERROR(VKFatal, "Descriptor set layout or pool is null!");

}

}	 // namespace nft::vulkan