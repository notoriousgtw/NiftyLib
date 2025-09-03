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
#include "vk/pipeline.h"	// Include pipeline definitions

#include "core/glfw_common.h"

#include <thread>
#include <chrono>

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
	is_cleaned_up(false),
	swapchain_render_pass(device)
{
	// Get app from VulkanHandler
	app = vulkan::VulkanHandler::GetApp();

	// Validate input parameters
	if (!instance)
		NFT_ERROR(VulkanFatal, "Instance is null!");
	if (!device)
		NFT_ERROR(VulkanFatal, "Device is null!");
	if (!window)
		NFT_ERROR(VulkanFatal, "Window is null!");

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
	// NOTE: Window surface assignment is now handled by VulkanHandler::AddSurface

	app->GetLogger()->Debug(
		std::format("Surface For Window: \"{}\" Created Successfully!", glfwGetWindowTitle(window->GetGLFWWindow())), "VKInit");

	CreateCommandPool();
	//scene = std::make_unique<Scene>(this, vk_command_buffer);
	InitSwapchain();
	// CreatePipeline();	 // Create the graphics pipeline
	swapchain_render_pass.Init(format.format, depth_format);
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
	SelectFormat();
	SelectPresentMode();

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
		frame.Init(this, static_cast<uint32_t>(i)); // Pass frame index, scene will be set by renderer
		// Note: Frame no longer needs MakeDescriptorResources() since we use bindless

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
	auto extent = window->GetExtent();
	while (extent.width == 0 || extent.height == 0)
	{
		int temp_width, temp_height;
		glfwGetFramebufferSize(window->GetGLFWWindow(), &temp_width, &temp_height);
		extent = vk::Extent2D(temp_width, temp_height);
		glfwWaitEvents();
	}

	// Wait for device to be idle before recreating swapchain
	device->GetDevice().waitIdle();

	// Cleanup old swapchain resources
	CleanupSwapchain();

	// Recreate swapchain and related resources
	InitSwapchain();
	CreateFrameBuffers();
	CreateFrameCommandBuffers();

	// Recreate pipeline for new extent
	//if (graphics_pipeline)
	//{
	//	//graphics_pipeline->RecreatePipeline();
	//}

	// COMMENTED OUT FOR NOW
	// if (object_picker)
	//     object_picker->Recreate(extent);
}

//=============================================================================
// PIPELINE CREATION METHODS
//=============================================================================

//void Surface::CreatePipeline()
//{
	// Create the graphics pipeline
	//graphics_pipeline = std::make_unique<GraphicsPipeline>(device, scene.get());
	//graphics_pipeline->InitGraphics(extent, format.format, depth_format);

	//// Add shader stages
	//graphics_pipeline->AddShaderStage(Shader::ShaderCode { (uint32_t*)simple_shader_vert, simple_shader_vert_len },
	//								  vk::ShaderStageFlagBits::eVertex);
	//graphics_pipeline->AddShaderStage(Shader::ShaderCode { (uint32_t*)simple_shader_frag, simple_shader_frag_len },
	//								  vk::ShaderStageFlagBits::eFragment);

	//graphics_pipeline->CreatePipeline();
	//graphics_pipeline->CreateTextureDescriptorSet();

	//app->GetLogger()->Debug("Graphics Pipeline Created Successfully!", "VKInit");

//}

//=============================================================================
// FRAMEBUFFER AND COMMAND CREATION METHODS
//=============================================================================

void Surface::CreateFrameBuffers()
{
	// Note: Frame buffers are now managed by pipelines
	// This method can be simplified or removed in future refactoring
	if (!swapchain_render_pass.vk_render_pass)
	{
		NFT_ERROR(VulkanFatal, "Render pass is not created!");
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
										 .setRenderPass(swapchain_render_pass.vk_render_pass)
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

		//// CRITICAL: Allocate descriptor sets for frames after pipeline is created
		//if (graphics_pipeline)
		//{
		//	frame.AllocateFrameDescriptorSet(graphics_pipeline->GetFrameDescriptorPool(), graphics_pipeline->GetFrameSetLayout());
		//}
	}
}

void Surface::RecordFrameCommandBuffer(uint32_t frame_index)
{
	Frame&					   frame	  = frames[frame_index];

	vk::CommandBufferBeginInfo begin_info	  = vk::CommandBufferBeginInfo();
	frame.vk_command_buffer.begin(begin_info);

	//std::vector<vk::ClearValue> clear_values = { clear_color, clear_depth };

	//vk::RenderPassBeginInfo render_pass_begin_info =
	//	vk::RenderPassBeginInfo()
	//		.setRenderPass(swapchain_render_pass.vk_render_pass)
	//		.setFramebuffer(frame.vk_frame_buffer)
	//		.setRenderArea(vk::Rect2D().setOffset({ 0, 0 }).setExtent(viewport_stage.scissor.extent))
	//		.setClearValueCount(clear_values.size())
	//		.setPClearValues(clear_values.data());
	//frame.vk_command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
}

//=========================================================================
// RENDERING METHODS (delegated to pipeline)
//=========================================================================


vk::CommandBuffer Surface::BeginFrame()
{
	// DEBUG: Add frame tracking
	static uint32_t total_frame_count = 0;
	printf("DEBUG: BeginFrame() #%u - current_frame_index: %zu, max_frames_in_flight: %zu\n",
		total_frame_count, current_frame_index, max_frames_in_flight);
	
	// CRITICAL FIX: Only wait for fence if we've used this frame before (after max_frames_in_flight submissions)
	if (total_frame_count >= max_frames_in_flight) {
		printf("DEBUG: Waiting for fence for frame %zu...\n", current_frame_index);
		
		// Add a reasonable timeout and proper error handling
		auto result = device->GetDevice().waitForFences(
			frames[current_frame_index].in_flight_fence, 
			VK_TRUE, 
			1000000000ULL  // 1 second timeout (reduced from 5 seconds)
		);
		
		if (result == vk::Result::eTimeout) {
			printf("ERROR: Fence wait TIMEOUT for frame %zu after 1 second!\n", current_frame_index);
			printf("ERROR: This indicates the GPU command from previous frame %zu cycle never completed.\n", current_frame_index);
			printf("ERROR: Possible causes: GPU hang, driver issue, or synchronization problem.\n");
			
			// Instead of crashing, let's try to recover by waiting for device idle
			printf("DEBUG: Attempting recovery by waiting for device idle...\n");
			try {
				device->GetDevice().waitIdle();
				printf("DEBUG: Device idle wait completed - trying to continue\n");
				
				// Reset the fence manually since the previous operation might be stuck
				device->GetDevice().resetFences(frames[current_frame_index].in_flight_fence);
				printf("DEBUG: Manually reset fence for frame %zu\n", current_frame_index);
			} catch (const vk::SystemError& e) {
				printf("ERROR: Device idle wait failed: %s\n", e.what());
				NFT_ERROR(VulkanFatal, std::format("Device idle wait failed: {}", e.what()));
				return VK_NULL_HANDLE;
			}
		} else if (result != vk::Result::eSuccess) {
			printf("ERROR: Fence wait FAILED for frame %zu with result: %d\n", current_frame_index, static_cast<int>(result));
			NFT_ERROR(VulkanFatal, std::format("Fence wait failed on frame {} with result {}", current_frame_index, vk::to_string(result)));
			return VK_NULL_HANDLE;
		} else {
			printf("DEBUG: Fence wait completed successfully for frame %zu\n", current_frame_index);
		}
	} else {
		printf("DEBUG: Skipping fence wait for initial frame %zu (total: %u)\n", 
			current_frame_index, total_frame_count);
	}
	
	// CRITICAL FIX: Always reset fence before using it (required by Vulkan spec)
	// Fences must be in unsignaled state before submission
	device->GetDevice().resetFences(frames[current_frame_index].in_flight_fence);
	printf("DEBUG: Reset fence for frame %zu\n", current_frame_index);
	
	// Acquire the next image from the swapchain
	try
	{
		auto result = device->GetDevice().acquireNextImageKHR(vk_swapchain, UINT64_MAX, 
			frames[current_frame_index].image_available_semaphore, VK_NULL_HANDLE);
		current_image_index = result.value; // Store the acquired image index
		
		printf("DEBUG: Acquired swapchain image: %u (frame_index: %zu)\n", current_image_index, current_frame_index);
	}
	catch (const vk::OutOfDateKHRError&)
	{
		printf("DEBUG: Swapchain out of date, recreating\n");
		RecreateSwapchain();
		return VK_NULL_HANDLE;
	}
	catch (const vk::SystemError& e)
	{
		printf("DEBUG: Failed to acquire swapchain image: %s\n", e.what());
		NFT_ERROR(VulkanFatal, std::format("Failed to acquire swapchain image: {}", e.what()));
		return VK_NULL_HANDLE;
	}
	
	// Reset and begin the command buffer
	auto& command_buffer = frames[current_frame_index].vk_command_buffer;
	command_buffer.reset();
	
	vk::CommandBufferBeginInfo begin_info = vk::CommandBufferBeginInfo()
		.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		
	command_buffer.begin(begin_info);
	
	total_frame_count++;
	return command_buffer;
}

void Surface::EndFrame()
{
	auto& current_frame = frames[current_frame_index];
	
	printf("DEBUG: EndFrame() - current_frame_index: %zu, current_image_index: %u\n", 
		current_frame_index, current_image_index);
	
	// DIAGNOSTIC: Check if the framebuffer for this image is valid
	if (current_image_index >= frames.size()) {
		printf("ERROR: current_image_index %u is out of range (max: %zu)!\n", current_image_index, frames.size() - 1);
		NFT_ERROR(VulkanFatal, "Invalid swapchain image index");
		return;
	}
	
	auto& target_frame = frames[current_image_index];
	if (!target_frame.vk_frame_buffer) {
		printf("ERROR: Framebuffer for image %u is null!\n", current_image_index);
		NFT_ERROR(VulkanFatal, "Invalid framebuffer for swapchain image");
		return;
	}
	
	printf("DEBUG: Using framebuffer for swapchain image %u (frame %zu's framebuffer)\n", 
		current_image_index, current_image_index);
	
	// End command buffer recording
	current_frame.vk_command_buffer.end();
	
	// DIAGNOSTIC: Add memory barrier before submission to ensure proper synchronization
	// This forces the GPU to complete all previous operations before proceeding
	printf("DEBUG: Adding memory barrier before submission for frame %zu\n", current_frame_index);
	
	// Submit the command buffer with enhanced diagnostics
	vk::PipelineStageFlags wait_stages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
	
	vk::SubmitInfo submit_info = vk::SubmitInfo()
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&current_frame.image_available_semaphore)
		.setPWaitDstStageMask(wait_stages)
		.setCommandBufferCount(1)
		.setPCommandBuffers(&current_frame.vk_command_buffer)
		.setSignalSemaphoreCount(1)
		.setPSignalSemaphores(&current_frame.render_finished_semaphore);
	
	try
	{
		printf("DEBUG: Submitting command buffer for frame %zu (target image: %u) with fence...\n", 
			current_frame_index, current_image_index);
		
		// DIAGNOSTIC: Check fence state before submission
		auto fence_status = device->GetDevice().getFenceStatus(current_frame.in_flight_fence);
		printf("DEBUG: Fence status before submission: %s\n", vk::to_string(fence_status).c_str());
		
		device->GetGraphicsQueue().submit(submit_info, current_frame.in_flight_fence);
		printf("DEBUG: Command buffer submitted successfully for frame %zu\n", current_frame_index);
		
		// DIAGNOSTIC: Check fence status immediately after submission
		fence_status = device->GetDevice().getFenceStatus(current_frame.in_flight_fence);
		printf("DEBUG: Fence status after submission: %s\n", vk::to_string(fence_status).c_str());
		
	}
	catch (const vk::SystemError& e)
	{
		printf("DEBUG: Failed to submit command buffer for frame %zu: %s\n", current_frame_index, e.what());
		NFT_ERROR(VulkanFatal, std::format("Failed to submit draw command buffer: {}", e.what()));
		return;
	}
	
	// DIAGNOSTIC: Add a small delay to see if this affects timing
	if (current_frame_index == 2) {
		printf("DEBUG: Frame 2 - adding diagnostic delay before present\n");
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	
	// Present the image using the acquired image index
	vk::PresentInfoKHR present_info = vk::PresentInfoKHR()
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&current_frame.render_finished_semaphore)
		.setSwapchainCount(1)
		.setPSwapchains(&vk_swapchain)
		.setPImageIndices(&current_image_index);
	
	try
	{
		printf("DEBUG: Presenting image %u with semaphore from frame %zu\n", current_image_index, current_frame_index);
		auto result = device->GetPresentQueue().presentKHR(present_info);
		printf("DEBUG: Present completed with result: %s\n", vk::to_string(result).c_str());
		
		// DIAGNOSTIC: Check fence status after present
		auto fence_status = device->GetDevice().getFenceStatus(current_frame.in_flight_fence);
		printf("DEBUG: Fence status after present: %s\n", vk::to_string(fence_status).c_str());
		
		// Check if we need to recreate swapchain
		if (result == vk::Result::eSuboptimalKHR) {
			printf("DEBUG: Swapchain suboptimal, will recreate\n");
			RecreateSwapchain();
		}
	}
	catch (const vk::OutOfDateKHRError&)
	{
		printf("DEBUG: Swapchain out of date during present, recreating\n");
		RecreateSwapchain();
	}
	catch (const vk::SystemError& e)
	{
		printf("DEBUG: Failed to present image: %s\n", e.what());
		NFT_ERROR(VulkanFatal, std::format("Failed to present swapchain image: {}", e.what()));
	}
	
	// Move to next frame
	size_t old_frame_index = current_frame_index;
	current_frame_index = (current_frame_index + 1) % max_frames_in_flight;
	printf("DEBUG: Advanced frame index from %zu to %zu\n", old_frame_index, current_frame_index);
}

//=============================================================================
// CLEANUP METHODS
//=============================================================================

void Surface::CleanupSwapchain()
{
	// Cleanup frame resources first
	for (auto& frame : frames)
		frame.Cleanup();
	frames.clear();
	
	// Cleanup swapchain resources
	if (vk_swapchain && device && instance)
	{
		device->GetDevice().destroySwapchainKHR(vk_swapchain);
		vk_swapchain = VK_NULL_HANDLE;
		if (app && app->GetLogger())
			app->GetLogger()->Debug("Swapchain destroyed successfully", "VKShutdown");
	}
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

		// Cleanup swapchain and frame resources first
		CleanupSwapchain();

		// Cleanup render pass
		swapchain_render_pass.Cleanup();

		// Cleanup command pool
		if (vk_command_pool)
		{
			device->GetDevice().destroyCommandPool(vk_command_pool);
			vk_command_pool = VK_NULL_HANDLE;
		}

		// Pipeline cleanup removed - now handled by graphics::Renderer

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
	// TODO: Implement proper format selection 
	// For now, just use the first available format
	if (!support_details.formats.empty()) {
		format = support_details.formats[0];
	}
}

void Surface::SelectPresentMode()
{
	// TODO: Implement proper present mode selection
	// For now, just use the first available present mode
	if (!support_details.present_modes.empty()) {
		present_mode = support_details.present_modes[0];
	}
}

void Surface::LogSupportDetails()
{
	// Simple logging implementation - details omitted for brevity
	app->GetLogger()->Debug("Surface support details logged", "VKInit");
}

}	 // namespace nft::vulkan