#include "NiftyVKSurface.h"

#include "GLFW/glfw3.h"

#include "NiftyApp.h"
#include "NiftyError.h"
#include "NiftyVK.h"

namespace nft::Vulkan
{
Surface::Surface(Instance* instance): instance(instance)
{
	if (!instance)
		ErrorHandler::Error<VKInitFatal>("Instance is null!", __func__);
	app = this->instance->app;
	app->GetLogger()->Debug("Creating Vulkan Surface...", "VKInit");
	Init();
}
Surface::~Surface()
{
	instance->vk_instance.destroySurfaceKHR(vk_surface, nullptr, instance->dispatch_loader_dynamic);
}

void Surface::SetDevice(Device* device)
{
	this->device = device;
	swapchain	 = std::make_unique<Swapchain>(this);
	ChooseFormat();
	ChoosePresentMode();
}

void Surface::Init()
{
	// Create a Vulkan surface using GLFW
	VkSurfaceKHR c_style_surface;
	glfwCreateWindowSurface(instance->vk_instance, app->GetMainWindow()->GetGLFWWindow(), nullptr, &c_style_surface);
	vk_surface = c_style_surface;
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

void Surface::LogSwapchainSupport()
{

	Logger::DisplayFlags log_flags = Log::Flags::Default & ~Log::Flags::ShowHeader;
	log_surface_capabilities(app->GetLogger(), log_flags, swapchain->support_details.capabilities);
	log_surface_format_support(app->GetLogger(), log_flags, swapchain->support_details.formats);
	log_surface_present_mode(app->GetLogger(), log_flags, swapchain->support_details.present_modes);
	return;
}

void Surface::ChooseFormat()
{
	for (auto& supported_format : swapchain->support_details.formats)
		if (supported_format.format == vk::Format::eB8G8R8A8Unorm &&
			supported_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			this->format = supported_format;
			return;
		}
	this->format = swapchain->support_details.formats[0];
}

void Surface::ChoosePresentMode()
{
	for (auto& supported_present_mode : swapchain->support_details.present_modes)
		if (supported_present_mode == vk::PresentModeKHR::eMailbox)
		{
			this->present_mode = supported_present_mode;
			return;
		}
	this->present_mode = swapchain->support_details.present_modes[0];
}
}	 // namespace nft::Vulkan