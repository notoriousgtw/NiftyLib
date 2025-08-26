//=============================================================================
// VULKAN SURFACE IMPLEMENTATION
//=============================================================================
// This file implements the Vulkan surface management, focusing on swapchain
// creation and management. Rendering logic is now delegated to pipeline classes.

#include "vk/surface.h"

#include "core/app.h"
#include "core/error.h"

#include "vk/handler.h"
#include "vk/image.h"
#include "vk/scene.h"
#include "vk/pipeline.h"  // Include pipeline definitions

#include "core/glfw_common.h"

// #include <../generated/picking_shader.frag.spv.h>
// #include <../generated/picking_shader.vert.spv.h>
#include <../generated/simple_shader.frag.spv.h>
#include <../generated/simple_shader.vert.spv.h>

namespace nft::vulkan
{

//=============================================================================
// CONSTRUCTOR & DESTRUCTOR
//=============================================================================

Surface::Surface(Instance* instance, Device* device, Window* window):
	instance(instance),
	device(device),
	window(window),
	vk_surface(VK_NULL_HANDLE),
	vk_swapchain(VK_NULL_HANDLE),
	vk_command_pool(VK_NULL_HANDLE),
	vk_command_buffer(VK_NULL_HANDLE),
	is_cleaned_up(false)
{
	// Validate input parameters
	if (!instance)
		NFT_ERROR(VulkanFatal, "Instance is null!");
	if (!device)
		NFT_ERROR(VulkanFatal, "Device is null!");
	if (!window)
		NFT_ERROR(VulkanFatal, "Window is null!");

	app = instance->GetApp();

	Init();
}

Surface::~Surface()
{
	// Only cleanup if not already done explicitly
	Cleanup();

	// Finally cleanup the surface itself (must be done after all surface-dependent objects)
	if (vk_surface && instance && instance->vk_instance)
	{
		instance->vk_instance.destroySurfaceKHR(vk_surface);
		vk_surface = VK_NULL_HANDLE;
		if (app && app->GetLogger())
			app->GetLogger()->Debug("Surface destroyed successfully", "VKShutdown");
	}
}

//=============================================================================
// CORE INITIALIZATION METHODS
//=============================================================================

void Surface::Init()
{
	app->GetLogger()->Debug(std::format("Creating Surface For Window: \"{}\"...", glfwGetWindowTitle(window->GetGLFWWindow())),
							"VKInit");

	// Create a Vulkan surface using GLFW
	VkSurfaceKHR c_style_surface;
	glfwCreateWindowSurface(instance->vk_instance, window->GetGLFWWindow(), nullptr, &c_style_surface);
	vk_surface = c_style_surface;

	app->GetLogger()->Debug(
		std::format("Surface For Window: \"{}\" Created Successfully!", glfwGetWindowTitle(window->GetGLFWWindow())), "VKInit");

	CreateCommandPool();
	scene = std::make_unique<Scene>(this, vk_command_buffer);
	InitSwapchain();
	CreatePipeline();	 // Create the graphics pipeline
	CreateFrameBuffers();
	CreateFrameCommandBuffers();

	// COMMENTED OUT FOR NOW - ObjectPicker will be reimplemented later
	// object_picker = std::make_unique<ObjectPicker>(device, extent);
}

void Surface::InitSwapchain()
{
	// Query swapchain support details
	support_details.capabilities  = device->GetPhysicalDevice().getSurfaceCapabilitiesKHR(vk_surface);
	support_details.formats		  = device->GetPhysicalDevice().getSurfaceFormatsKHR(vk_surface);
	support_details.present_modes = device->GetPhysicalDevice().getSurfacePresentModesKHR(vk_surface);

	LogSupportDetails();
	CreateSwapchain();
}

//=============================================================================
// SWAPCHAIN CREATION METHODS
//=============================================================================

void Surface::CreateSwapchain()
{
	// Determine swapchain extent
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

	// Determine number of images in swapchain
	image_count = std::min(support_details.capabilities.maxImageCount, support_details.capabilities.minImageCount + 1);

	// Select surface format and present mode
	SelectFormat(vk::SurfaceFormatKHR(vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear));
	SelectPresentMode(vk::PresentModeKHR::eFifo);

	// Setup queue family indices for sharing
	std::vector queue_family_indices = device->GetQueueFamilyIndices().Vec();
	vk_swapchain_info				 = vk::SwapchainCreateInfoKHR()
							.setFlags(vk::SwapchainCreateFlagsKHR())
							.setSurface(vk_surface)
							.setMinImageCount(image_count)
							.setImageFormat(format.format)
							.setImageColorSpace(format.colorSpace)
							.setImageExtent(extent)
							.setImageArrayLayers(1)
							.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

	// Setup image sharing mode
	if (device->GetQueueFamilyIndices().graphics_family != device->GetQueueFamilyIndices().present_family)
		vk_swapchain_info.setImageSharingMode(vk::SharingMode::eConcurrent)
			.setQueueFamilyIndexCount(queue_family_indices.size())
			.setPQueueFamilyIndices(queue_family_indices.data());
	else
		vk_swapchain_info.setImageSharingMode(vk::SharingMode::eExclusive);

	vk_swapchain_info.setPreTransform(support_details.capabilities.currentTransform)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setPresentMode(present_mode)
		.setClipped(vk::True)
		.setOldSwapchain(vk::SwapchainKHR(nullptr));

	// Create the swapchain
	try
	{
		vk_swapchain = device->GetDevice().createSwapchainKHR(vk_swapchain_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Create Swapchain:\n{}", err.what()));
	}

	// Get swapchain images and create frame data
	std::vector<vk::Image> image_vec = device->GetDevice().getSwapchainImagesKHR(vk_swapchain);
	this->frames.resize(image_vec.size());
	max_frames_in_flight = image_vec.size();

	for (size_t i = 0; i < image_vec.size(); i++)
	{
		auto& frame = frames.at(i);

		frame.in_flight_fence			= device->CreateFence(true);
		frame.image_available_semaphore = device->CreateSemaphore();
		frame.render_finished_semaphore = device->CreateSemaphore();
		frame.Init(this, scene.get());
		frame.MakeDescriptorResources();

		auto& swapchain_image = frame.swapchain_image;
		swapchain_image.SetDevice(device);
		swapchain_image.Init(image_vec.at(i),
							 vk::ImageCreateInfo()
								 .setExtent(vk::Extent3D(vk_swapchain_info.imageExtent, 1))
								 .setFormat(vk_swapchain_info.imageFormat)
								 .setUsage(vk_swapchain_info.imageUsage));
		swapchain_image.CreateImageView(format.format);

		depth_format = FindFormat(device,
								  { vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint },
								  vk::ImageTiling::eOptimal,
								  vk::FormatFeatureFlagBits::eDepthStencilAttachment);

		auto& depth_buffer = frame.depth_buffer;
		depth_buffer.SetDevice(device);
		depth_buffer.Init(vk::ImageCreateInfo()
							  .setImageType(vk::ImageType::e2D)
							  .setExtent(vk::Extent3D(vk_swapchain_info.imageExtent, 1))
							  .setFormat(depth_format)
							  .setTiling(vk::ImageTiling::eOptimal)
							  .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment),
						  vk::MemoryPropertyFlagBits::eDeviceLocal);
		depth_buffer.SetSubresourceRange(vk::ImageSubresourceRange()
											 .setAspectMask(vk::ImageAspectFlagBits::eDepth)
											 .setBaseMipLevel(0)
											 .setLevelCount(1)
											 .setBaseArrayLayer(0)
											 .setLayerCount(1));
		depth_buffer.SetSubresourceLayers(vk::ImageSubresourceLayers()
											  .setAspectMask(vk::ImageAspectFlagBits::eDepth)
											  .setMipLevel(0)
											  .setBaseArrayLayer(0)
											  .setLayerCount(1));
		depth_buffer.CreateImageView(depth_format);
	}

	app->GetLogger()->Debug(
		std::format("Swapchain For Window: \"{}\" Created Successfully!", glfwGetWindowTitle(window->GetGLFWWindow())), "VKInit");
}

void Surface::RecreateSwapchain()
{
	// Wait for device to be idle before recreating swapchain
	int width  = 0;
	int height = 0;

	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window->GetGLFWWindow(), &width, &height);
		glfwWaitEvents();
	}

	device->GetDevice().waitIdle();

	// Cleanup old swapchain resources
	CleanupSwapchain();

	// Recreate swapchain and related resources
	InitSwapchain();
	CreateFrameBuffers();
	CreateFrameCommandBuffers();

	// Recreate pipeline for new extent
	if (graphics_pipeline)
	{
		graphics_pipeline->RecreatePipeline();
	}

	// COMMENTED OUT FOR NOW
	// if (object_picker)
	//     object_picker->Recreate(extent);
}

//=============================================================================
// PIPELINE CREATION METHODS
//=============================================================================

void Surface::CreatePipeline()
{
	// Create the graphics pipeline
	graphics_pipeline = std::make_unique<GraphicsPipeline>(device, scene.get());
	graphics_pipeline->InitGraphics(extent, format.format, depth_format);

	// Add shader stages
	graphics_pipeline->AddShaderStage(Shader::ShaderCode { (uint32_t*)simple_shader_vert, simple_shader_vert_len },
									  vk::ShaderStageFlagBits::eVertex);
	graphics_pipeline->AddShaderStage(Shader::ShaderCode { (uint32_t*)simple_shader_frag, simple_shader_frag_len },
									  vk::ShaderStageFlagBits::eFragment);

	graphics_pipeline->CreatePipeline();
	graphics_pipeline->CreateTextureDescriptorSet();

	app->GetLogger()->Debug("Graphics Pipeline Created Successfully!", "VKInit");
}

//=============================================================================
// FRAMEBUFFER AND COMMAND CREATION METHODS
//=============================================================================

void Surface::CreateFrameBuffers()
{
	if (!graphics_pipeline)
	{
		NFT_ERROR(VulkanFatal, "Graphics pipeline is not created!");
		return;
	}

	size_t i = 0;
	for (auto& frame : frames)
	{
		auto&					   frame_image_view		   = frame.swapchain_image.GetImageView();
		auto&					   frame_depth_buffer_view = frame.depth_buffer.GetImageView();
		std::vector<vk::ImageView> attachments			   = { frame_image_view, frame_depth_buffer_view };

		frame.vk_frame_buffer_info = vk::FramebufferCreateInfo()
										 .setFlags(vk::FramebufferCreateFlags())
										 .setRenderPass(graphics_pipeline->GetRenderPass())
										 .setAttachmentCount(attachments.size())
										 .setPAttachments(attachments.data())
										 .setWidth(extent.width)
										 .setHeight(extent.height)
										 .setLayers(1);

		try
		{
			frame.vk_frame_buffer = device->GetDevice().createFramebuffer(frame.vk_frame_buffer_info);
		}
		catch (const vk::SystemError& err)
		{
			NFT_ERROR(VulkanFatal, std::format("Failed To Create Framebuffer {}:\n{}", i, err.what()));
		}
		i++;
	}
}

void Surface::CreateCommandPool()
{
	vk_command_pool_info = vk::CommandPoolCreateInfo()
							   .setFlags(vk::CommandPoolCreateFlags() | vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
							   .setQueueFamilyIndex(device->GetQueueFamilyIndices().graphics_family.value());

	try
	{
		vk_command_pool = device->GetDevice().createCommandPool(vk_command_pool_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Create Command Pool:\n{}", err.what()));
	}

	// Allocate main command buffer
	try
	{
		vk_command_buffer = device->GetDevice().allocateCommandBuffers(vk::CommandBufferAllocateInfo()
																		   .setCommandPool(vk_command_pool)
																		   .setLevel(vk::CommandBufferLevel::ePrimary)
																		   .setCommandBufferCount(1))[0];
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Allocate Main Command Buffer:\n{}", err.what()));
	}
}

void Surface::CreateFrameCommandBuffers()
{
	// Create command buffers for each frame
	for (auto& frame : frames)
	{
		frame.vk_command_buffer = device->GetDevice().allocateCommandBuffers(vk::CommandBufferAllocateInfo()
																				 .setCommandPool(vk_command_pool)
																				 .setLevel(vk::CommandBufferLevel::ePrimary)
																				 .setCommandBufferCount(1))[0];
		
		// CRITICAL: Allocate descriptor sets for frames after pipeline is created
		if (graphics_pipeline)
		{
			frame.AllocateFrameDescriptorSet(graphics_pipeline->GetFrameDescriptorPool(), graphics_pipeline->GetFrameSetLayout());
		}
	}
}

//=========================================================================
// RENDERING METHODS (delegated to pipeline)
//=========================================================================

void Surface::Render()
{
	if (!graphics_pipeline)
	{
		NFT_ERROR(VulkanFatal, "Graphics pipeline is not created!");
		return;
	}

	Frame& current_frame = frames[frame_index];

	device->GetDevice().waitForFences(current_frame.in_flight_fence, VK_TRUE, UINT64_MAX);
	device->GetDevice().resetFences(current_frame.in_flight_fence);

	uint32_t image_index;
	try
	{
		vk::ResultValue aquire =
			device->GetDevice().acquireNextImage2KHR(vk::AcquireNextImageInfoKHR()
														 .setSwapchain(vk_swapchain)
														 .setTimeout(UINT64_MAX)
														 .setSemaphore(current_frame.image_available_semaphore)
														 .setFence(nullptr)
														 .setDeviceMask(1));
		image_index = aquire.value;
	}
	catch (vk::OutOfDateKHRError)
	{
		RecreateSwapchain();
		return;
	}

	vk::CommandBuffer command_buffer = current_frame.vk_command_buffer;
	command_buffer.reset(vk::CommandBufferResetFlags());

	current_frame.Prepare(scene->camera_transforms);

	// Delegate rendering to the pipeline
	graphics_pipeline->RecordDrawCommands(current_frame, image_index);

	vk::PipelineStageFlags wait_stages = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::SubmitInfo submit_info = vk::SubmitInfo()
									 .setWaitSemaphoreCount(1)
									 .setPWaitSemaphores(&current_frame.image_available_semaphore)
									 .setPWaitDstStageMask(&wait_stages)
									 .setCommandBufferCount(1)
									 .setPCommandBuffers(&command_buffer)
									 .setSignalSemaphoreCount(1)
									 .setPSignalSemaphores(&frames[image_index].render_finished_semaphore);

	try
	{
		device->GetGraphicsQueue().submit(submit_info, current_frame.in_flight_fence);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Submit Draw Command Buffer:\n{}", err.what()));
	}

	vk::PresentInfoKHR present_info = vk::PresentInfoKHR()
										  .setWaitSemaphoreCount(1)
										  .setPWaitSemaphores(&frames[image_index].render_finished_semaphore)
										  .setSwapchainCount(1)
										  .setPSwapchains(&vk_swapchain)
										  .setPImageIndices(&image_index);

	vk::Result present_result;
	try
	{
		present_result = device->GetPresentQueue().presentKHR(present_info);
	}
	catch (vk::OutOfDateKHRError)
	{
		RecreateSwapchain();
		return;
	}
	frame_index = (frame_index + 1) % max_frames_in_flight;
}

//=============================================================================
// CLEANUP METHODS
//=============================================================================

void Surface::CleanupSwapchain()
{
	// Cleanup swapchain resources
	if (vk_swapchain && device && instance)
	{
		device->GetDevice().destroySwapchainKHR(vk_swapchain);
		vk_swapchain = VK_NULL_HANDLE;
		if (app && app->GetLogger())
			app->GetLogger()->Debug("Swapchain destroyed successfully", "VKShutdown");
	}
	// Cleanup frame resources
	for (auto& frame : frames)
		frame.Cleanup();
	frames.clear();
}

void Surface::Cleanup()
{
	// Prevent double cleanup
	if (is_cleaned_up)
		return;

	if (device && device->GetDevice())
	{
		device->GetDevice().waitIdle();

		app->GetLogger()->Debug("Cleaning up Surface Vulkan objects...", "VKShutdown");

		if (vk_command_pool)
		{
			device->GetDevice().destroyCommandPool(vk_command_pool);
			vk_command_pool = VK_NULL_HANDLE;
		}

		CleanupSwapchain();

		// Cleanup pipeline
		if (graphics_pipeline)
		{
			graphics_pipeline.reset();
		}

		app->GetLogger()->Debug("Surface Vulkan objects cleaned up successfully", "VKShutdown");
	}

	// COMMENTED OUT FOR NOW
	// if (object_picker)
	//     object_picker.reset();

	is_cleaned_up = true;
}

//=============================================================================
// UTILITY AND SELECTION METHODS
//=============================================================================

void Surface::SelectFormat()
{
	NFT_ERROR(VulkanFatal, "Requested Format Not Supported!");
}

void Surface::SelectPresentMode()
{
	NFT_ERROR(VulkanFatal, "Requested Present Mode Not Supported!");
}

void Surface::LogSupportDetails()
{
	// Simple logging implementation - details omitted for brevity
	app->GetLogger()->Debug("Surface support details logged", "VKInit");
}

//=============================================================================
// OBJECT PICKER IMPLEMENTATION (COMMENTED OUT FOR NOW)
//=============================================================================

/*
ObjectPicker::ObjectPicker(Device* device, vk::Extent2D extent):
	device(device),
	extent(extent),
	pipeline(VK_NULL_HANDLE),
	vk_command_pool(VK_NULL_HANDLE),
	vk_command_buffer(VK_NULL_HANDLE),
	vertex_input_stage(device),
	input_assembly_stage(device),
	viewport_stage(device),
	rasterization_stage(device),
	depth_stencil_stage(device),
	multisample_stage(device),
	color_blend_stage(device),
	picking_set_layout(device),
	picking_descriptor_pool(device),
	pipeline_layout(device),
	render_pass(device)
{
	Init();
}

ObjectPicker::~ObjectPicker()
{
	Cleanup();
}

void ObjectPicker::Init()
{
	// Basic initialization - keeping it simple for refactoring demo
	CreateCommandPool();
	CreateRenderPass();
	CreateShaders();
	CreatePipeline();
	CreateFramebuffer();
}

void ObjectPicker::CreateCommandPool()
{
	vk_command_pool_info = vk::CommandPoolCreateInfo()
							   .setFlags(vk::CommandPoolCreateFlags() | vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
							   .setQueueFamilyIndex(device->queue_family_indices.graphics_family.value());

	try
	{
		vk_command_pool = device->vk_device.createCommandPool(vk_command_pool_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Create Command Pool:\n{}", err.what()));
	}
}

void ObjectPicker::CreateRenderPass()
{
	render_pass.Init(vk::Format::eR32G32B32A32Uint, vk::Format::eD32Sfloat);
}

void ObjectPicker::CreateShaders()
{
	// Shader creation implementation
}

void ObjectPicker::CreatePipeline()
{
	// Pipeline creation implementation
}

void ObjectPicker::CreateFramebuffer()
{
	// Framebuffer creation implementation
}

void ObjectPicker::Recreate(vk::Extent2D new_extent)
{
	extent = new_extent;
	Cleanup();
	Init();
}

void ObjectPicker::Cleanup()
{
	if (device && device->vk_device)
	{
		if (vk_command_pool)
		{
			device->vk_device.destroyCommandPool(vk_command_pool);
			vk_command_pool = VK_NULL_HANDLE;
		}

		render_pass.Cleanup();
		pipeline_layout.Cleanup();
		picking_set_layout.Cleanup();
		picking_descriptor_pool.Cleanup();
	}
}

uint32_t ObjectPicker::PickObject(const std::vector<ObjectData>& objects,
								  const GeometryBatcher* geometry_batcher,
								  const glm::mat4& view_matrix,
								  const glm::mat4& proj_matrix,
								  int mouse_x,
								  int mouse_y)
{
	// Object picking implementation
	return 0;  // Return picked object ID
}

void ObjectPicker::RecordPickingCommands(const std::vector<ObjectData>& objects,
										 const GeometryBatcher* geometry_batcher,
										 const glm::mat4& view_matrix,
										 const glm::mat4& proj_matrix)
{
	// Command recording implementation
}

uint32_t Surface::PickObjectAtPosition(int mouse_x, int mouse_y)
{
	if (!object_picker || !scene)
		return 0;

	Frame& current_frame = frames[frame_index];
	return object_picker->PickObject(scene->GetObjects(),
									 scene->GetGeometryBatcher(),
									 current_frame.camera_data.view,
									 current_frame.camera_data.proj,
									 mouse_x,
									 mouse_y);
}
*/
}	 // namespace nft::vulkan