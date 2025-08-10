#include "NiftyVKSwapchain.h"

#include "NiftyApp.h"
#include "NiftyError.h"
#include "NiftyVK.h"

namespace nft::Vulkan
{
Swapchain::Swapchain(Surface* surface): surface(surface)
{
	if (!surface)
		NFT_ERROR(VKInitFatal, "Surface Is Null!");
	this->instance = surface->instance;
	this->device   = surface->device;
	app			   = this->instance->app;
	Init();
	CreateSwapchain();
}

Swapchain::~Swapchain()
{
	for (auto& frame : images)
	{
		if (frame.image_view)
		{
			device->vk_device.destroyImageView(frame.image_view, nullptr, instance->dispatch_loader_dynamic);
			frame.image_view = nullptr;
		}
	}
	device->vk_device.destroySwapchainKHR(vk_swapchain, nullptr, instance->dispatch_loader_dynamic);
}

void Swapchain::Init()
{
	app->GetLogger()->Debug(std::format("Creating Swapchain For Window: \"{}\"...", glfwGetWindowTitle(surface->window)),
							"VKInit");
}
void Swapchain::CreateSwapchain()
{
	app->GetLogger()->Debug("Querying Swapchain Support...", "VKInit");
	support_details.capabilities =
		device->vk_physical_device.getSurfaceCapabilitiesKHR(surface->vk_surface, instance->dispatch_loader_dynamic);
	support_details.formats =
		device->vk_physical_device.getSurfaceFormatsKHR(surface->vk_surface, instance->dispatch_loader_dynamic);
	support_details.present_modes =
		device->vk_physical_device.getSurfacePresentModesKHR(surface->vk_surface, instance->dispatch_loader_dynamic);

	if (support_details.capabilities.currentExtent.width != UINT32_MAX)
	{
		extent.width  = support_details.capabilities.currentExtent.width;
		extent.height = support_details.capabilities.currentExtent.height;
	}
	else
	{
		extent.width  = support_details.capabilities.maxImageExtent.width;
		extent.height = support_details.capabilities.maxImageExtent.height;
	}

	image_count = std::min(support_details.capabilities.maxImageCount, support_details.capabilities.minImageCount + 1);

	app->GetLogger()->Debug("Selecting Swapchain Format...", "VKInit");
	for (auto it = support_details.formats.begin(); it <= support_details.formats.end(); it++)
	{
		if (it == support_details.formats.end())
		{
			format = support_details.formats.at(0);
			break;
		}
		auto supported_format = *it;
		if (supported_format.format == vk::Format::eB8G8R8A8Unorm &&
			supported_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			format = supported_format;
			break;
		}
	}
	app->GetLogger()->Debug("Selecting Swapchain Present Mode...", "VKInit");
	for (auto it = support_details.present_modes.begin(); it <= support_details.present_modes.end(); it++)
	{
		if (it == support_details.present_modes.end())
		{
			present_mode = support_details.present_modes.at(0);
			break;
		}
		auto supported_present_mode = *it;
		if (supported_present_mode == vk::PresentModeKHR::eFifo)
		{
			present_mode = supported_present_mode;
			break;
		}
	}

	std::vector queue_family_indices = device->queue_family_indices.Vec();
	create_info						 = vk::SwapchainCreateInfoKHR()
					  .setFlags(vk::SwapchainCreateFlagsKHR())
					  .setSurface(surface->vk_surface)
					  .setMinImageCount(image_count)
					  .setImageFormat(format.format)
					  .setImageColorSpace(format.colorSpace)
					  .setImageExtent(extent)
					  .setImageArrayLayers(1)
					  .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

	if (device->queue_family_indices.graphics_family != device->queue_family_indices.present_family)
		create_info.setImageSharingMode(vk::SharingMode::eConcurrent)
			.setQueueFamilyIndexCount(queue_family_indices.size())
			.setPQueueFamilyIndices(queue_family_indices.data());
	else
		create_info.setImageSharingMode(vk::SharingMode::eExclusive);

	create_info.setPreTransform(support_details.capabilities.currentTransform)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setPresentMode(present_mode)
		.setClipped(vk::True);

	create_info.setOldSwapchain(vk::SwapchainKHR(nullptr));

	try
	{
		vk_swapchain = device->vk_device.createSwapchainKHR(create_info, nullptr, instance->dispatch_loader_dynamic);
	}
	catch (const vk::SystemError err)
	{
		NFT_ERROR(VKInitFatal, err.what());
	}

	std::vector<vk::Image> image_vec = device->vk_device.getSwapchainImagesKHR(vk_swapchain, instance->dispatch_loader_dynamic);
	this->images.resize(image_vec.size());

	for (size_t i = 0; i < image_vec.size(); i++)
	{
		auto& frame		  = images.at(i);
		frame.image		  = image_vec.at(i);
		frame.create_info = vk::ImageViewCreateInfo()
								.setFlags(vk::ImageViewCreateFlags())
								.setImage(frame.image)
								.setViewType(vk::ImageViewType::e2D)
								.setFormat(format.format)
								.setComponents(vk::ComponentMapping())
								.setSubresourceRange(vk::ImageSubresourceRange()
														 .setAspectMask(vk::ImageAspectFlagBits::eColor)
														 .setBaseMipLevel(0)
														 .setLevelCount(1)
														 .setBaseArrayLayer(0)
														 .setLayerCount(1));
		try
		{
			frame.image_view = device->vk_device.createImageView(frame.create_info, nullptr, instance->dispatch_loader_dynamic);
		}
		catch (const vk::SystemError& err)
		{
			NFT_ERROR(VKInitFatal, std::format("Failed To Create Image View:\n    {}", err.what()));
		}
	}

	app->GetLogger()->Debug(
		std::format("Swapchain For Window: \"{}\" Created Successfully!", glfwGetWindowTitle(surface->window)), "VKInit");
}

void log_image_capabilites(Logger* logger, Logger::DisplayFlags log_flags, const vk::SurfaceCapabilitiesKHR& capabilities)
{
	logger->Debug(std::format("Minimum Image Count: {}", capabilities.minImageCount), "", log_flags, 0, 4);
	logger->Debug(std::format("Maximum Image Count: {}", capabilities.maxImageCount), "", log_flags, 0, 4);
	logger->Debug(std::format("Maximum Image Array Layers: {}", capabilities.maxImageArrayLayers), "", log_flags, 0, 4);
	logger->Debug(std::format("Minimum Image Size:"), "", log_flags, 0, 4);
	logger->Debug(std::format("Width: {}", capabilities.minImageExtent.width), "", log_flags, 0, 8);
	logger->Debug(std::format("Height: {}", capabilities.minImageExtent.height), "", log_flags, 0, 8);
	logger->Debug(std::format("Maximum Image Size:"), "", log_flags, 0, 4);
	logger->Debug(std::format("Width: {}", capabilities.maxImageExtent.width), "", log_flags, 0, 8);
	logger->Debug(std::format("Height: {}", capabilities.maxImageExtent.height), "", log_flags, 0, 8);
	logger->Debug(std::format("Current Image Size:"), "", log_flags, 0, 4);
	logger->Debug(std::format("Width: {}", capabilities.currentExtent.width), "", log_flags, 0, 8);
	logger->Debug(std::format("Height: {}", capabilities.currentExtent.height), "", log_flags, 0, 8);
}

void log_transform_bits(Logger* logger, Logger::DisplayFlags log_flags, const vk::SurfaceCapabilitiesKHR& capabilities)
{
	std::vector<std::string> supported_transforms;
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
		supported_transforms.push_back("Identity - No transformation applied.");
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eRotate90)
		supported_transforms.push_back("Rotate 90 - Rotates the surface 90 degrees clockwise.");
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eRotate180)
		supported_transforms.push_back("Rotate 180 - Rotates the surface 180 degrees.");
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eRotate270)
		supported_transforms.push_back("Rotate 270 - Rotates the surface 270 degrees clockwise.");
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirror)
		supported_transforms.push_back("Horizontal Mirror - Flips the surface horizontally.");
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate90)
		supported_transforms.push_back("Horizontal Mirror Rotate 90 - Flips horizontally and rotates 90 degrees.");
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate180)
		supported_transforms.push_back("Horizontal Mirror Rotate 180 - Flips horizontally and rotates 180 degrees.");
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate270)
		supported_transforms.push_back("Horizontal Mirror Rotate 270 - Flips horizontally and rotates 270 degrees.");
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eInherit)
		supported_transforms.push_back("Inherit - Uses the window system's transform settings.");

	logger->Debug("Supported Transforms:", "", log_flags, 0, 4);
	for (const auto& transform : supported_transforms)
		logger->Debug(transform, "", log_flags, 0, 8);

	std::vector<std::string> current_transforms;
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
		current_transforms.push_back("Identity");
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eRotate90)
		current_transforms.push_back("Rotate 90");
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eRotate180)
		current_transforms.push_back("Rotate 180");
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eRotate270)
		current_transforms.push_back("Rotate 270");
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirror)
		current_transforms.push_back("Horizontal Mirror");
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate90)
		current_transforms.push_back("Horizontal Mirror Rotate 90");
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate180)
		current_transforms.push_back("Horizontal Mirror Rotate 180");
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate270)
		current_transforms.push_back("Horizontal Mirror Rotate 270");
	if (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eInherit)
		current_transforms.push_back("Inherit");

	logger->Debug("Current Transforms:", "", log_flags, 0, 4);
	for (const auto& transform : current_transforms)
		logger->Debug(transform, "", log_flags, 0, 8);
}

void log_composite_alpha_bits(Logger* logger, Logger::DisplayFlags log_flags, const vk::SurfaceCapabilitiesKHR& capabilities)
{
	std::vector<std::string> supported_composite_alphas;
	if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque)
		supported_composite_alphas.push_back("Opaque - No transparency, fully opaque surface.");
	if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
		supported_composite_alphas.push_back("PreMultiplied - Transparency is pre-applied to color values.");
	if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied)
		supported_composite_alphas.push_back("PostMultiplied - Transparency is applied after blending.");
	if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)
		supported_composite_alphas.push_back("Inherit - Uses the window system's alpha blending settings.");

	logger->Debug("Supported Alpha Operations:", "", log_flags, 0, 4);
	for (const auto& alpha : supported_composite_alphas)
		logger->Debug(alpha, "", log_flags, 0, 8);
}

void log_image_usage_bits(Logger* logger, Logger::DisplayFlags log_flags, const vk::SurfaceCapabilitiesKHR& capabilities)
{
	std::vector<std::string> usage_flags;
	if (capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferSrc)
		usage_flags.push_back("Transfer Source - Image can be used as a source for transfer operations.");
	if (capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst)
		usage_flags.push_back("Transfer Destination - Image can be used as a destination for transfer operations.");
	if (capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eSampled)
		usage_flags.push_back("Sampled - Image can be used as a sampled image in shaders.");
	if (capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eStorage)
		usage_flags.push_back("Storage - Image can be used as a storage image in shaders.");
	if (capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eColorAttachment)
		usage_flags.push_back("Color Attachment - Image can be used as a color attachment in render passes.");
	if (capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eDepthStencilAttachment)
		usage_flags.push_back("Depth Stencil Attachment - Image can be used as a depth/stencil attachment in render passes.");
	if (capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransientAttachment)
		usage_flags.push_back("Transient Attachment - Image can be used as a transient attachment in render passes.");
	if (capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eInputAttachment)
		usage_flags.push_back("Input Attachment - Image can be used as an input attachment in render passes.");
	if (capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eFragmentDensityMapEXT)
		usage_flags.push_back("Fragment Density Map - Image can be used for fragment density mapping.");
	if (capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eFragmentShadingRateAttachmentKHR)
		usage_flags.push_back("Fragment Shading Rate Attachment - Image can be used for fragment shading rate control.");

	logger->Debug("Supported Image Usage Flags:", "", log_flags, 0, 4);
	for (const auto& usage_flag : usage_flags)
		logger->Debug(usage_flag, "", log_flags, 0, 8);
}

void log_surface_capabilities(Logger* logger, Logger::DisplayFlags log_flags, const vk::SurfaceCapabilitiesKHR& capabilities)
{

	logger->Debug(std::format("Surface Capabilities:"), "VKInit");

	log_image_capabilites(logger, log_flags, capabilities);
	log_transform_bits(logger, log_flags, capabilities);
	log_composite_alpha_bits(logger, log_flags, capabilities);
	log_image_usage_bits(logger, log_flags, capabilities);
}

void log_surface_format_support(Logger* logger, Logger::DisplayFlags log_flags, const std::vector<vk::SurfaceFormatKHR> formats)
{
	logger->Debug(std::format("Supported Surface Formats:"), "", log_flags, 0, 4);

	for (const auto& format : formats)
	{
		std::string color_space;
		switch (format.colorSpace)
		{
		case vk::ColorSpaceKHR::eSrgbNonlinear: color_space = "sRGB Nonlinear"; break;
		case vk::ColorSpaceKHR::eDisplayP3NonlinearEXT: color_space = "Display P3 Nonlinear"; break;
		case vk::ColorSpaceKHR::eExtendedSrgbLinearEXT: color_space = "Extended sRGB Linear"; break;
		case vk::ColorSpaceKHR::eDciP3NonlinearEXT: color_space = "DCI-P3 Nonlinear"; break;
		case vk::ColorSpaceKHR::eBt709LinearEXT: color_space = "BT709 Linear"; break;
		case vk::ColorSpaceKHR::eBt2020LinearEXT: color_space = "BT2020 Linear"; break;
		case vk::ColorSpaceKHR::eDisplayNativeAMD: color_space = "Display Native AMD"; break;
		case vk::ColorSpaceKHR::eDciP3LinearEXT: color_space = "DCI-P3 Linear"; break;
		case vk::ColorSpaceKHR::eHdr10HlgEXT: color_space = "HDR10 HLG"; break;
		case vk::ColorSpaceKHR::eHdr10St2084EXT: color_space = "HDR10 ST2084"; break;
		default: color_space = "Unknown Color Space"; break;
		}
		logger->Debug(std::format("{}: {}", vk::to_string(format.format), color_space), "", log_flags, 0, 8);
	}
}

void log_surface_present_mode(Logger* logger, Logger::DisplayFlags log_flags, const std::vector<vk::PresentModeKHR> present_modes)
{
	logger->Debug(std::format("Supported Present Modes:"), "", log_flags, 0, 4);

	for (const auto& present_modes : present_modes)
	{
		std::string mode;
		switch (present_modes)
		{
		case vk::PresentModeKHR::eImmediate: mode = "Immediate"; break;
		case vk::PresentModeKHR::eMailbox: mode = "Mailbox"; break;
		case vk::PresentModeKHR::eFifo: mode = "FIFO"; break;
		case vk::PresentModeKHR::eFifoRelaxed: mode = "FIFO Relaxed"; break;
		case vk::PresentModeKHR::eSharedContinuousRefresh: mode = "Shared Continuous Refresh"; break;
		case vk::PresentModeKHR::eSharedDemandRefresh: mode = "Shared Demand Refresh"; break;
		default: mode = "Unknown"; break;
		}
		logger->Debug(mode, "", log_flags, 0, 8);
	}
}

void Swapchain::LogSupportDetails()
{

	Logger::DisplayFlags log_flags = Log::Flags::Default & ~Log::Flags::ShowHeader;
	log_surface_capabilities(app->GetLogger(), log_flags, support_details.capabilities);
	log_surface_format_support(app->GetLogger(), log_flags, support_details.formats);
	log_surface_present_mode(app->GetLogger(), log_flags, support_details.present_modes);
	return;
}
}	 // namespace nft::Vulkan