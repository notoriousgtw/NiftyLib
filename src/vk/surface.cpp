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

		// CRITICAL FIX: Create fence in signaled state for first use, then reset it
		frame.in_flight_fence			= device->CreateFence(true); // Start signaled
		frame.image_available_semaphore = device->CreateSemaphore();
		frame.render_finished_semaphore = device->CreateSemaphore();
		frame.Init(this, static_cast<uint32_t>(i)); // Pass frame index, scene will be set by renderer
		// Note: Frame no longer needs MakeDescriptorResources() since we use bindless

		// IMPORTANT: Reset fence immediately to unsignaled state for proper use
		device->GetDevice().resetFences(frame.in_flight_fence);
		app->GetLogger()->Debug(std::format("Created and reset fence for frame {}", i), "VKInit");

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
	// Add frame tracking
	static uint32_t total_frame_count = 0;
	app->GetLogger()->Debug(std::format("BeginFrame() #{} - current_frame_index: {}, max_frames_in_flight: {}", 
		total_frame_count, current_frame_index, max_frames_in_flight), "VKRender");
	
	// CRITICAL FIX: Only wait for fence if we've used this frame before (after max_frames_in_flight submissions)
	if (total_frame_count >= max_frames_in_flight) {
		app->GetLogger()->Debug(std::format("Waiting for fence for frame {}...", current_frame_index), "VKRender");
		
		// IMPROVED ERROR HANDLING: Add progressive timeout strategy
		const uint64_t SHORT_TIMEOUT = 100000000ULL;  // 100ms
		const uint64_t LONG_TIMEOUT = 1000000000ULL;  // 1 second
		const uint64_t MAX_TIMEOUT = 5000000000ULL;   // 5 seconds
		
		// Try a short wait first
		auto result = device->GetDevice().waitForFences(
			frames[current_frame_index].in_flight_fence, 
			VK_TRUE, 
			SHORT_TIMEOUT
		);
		
		if (result == vk::Result::eTimeout) {
			app->GetLogger()->Warn(std::format("Fence wait timed out after 100ms for frame {}, trying longer wait...", current_frame_index), "VKRender");
			
			// Try a medium wait
			result = device->GetDevice().waitForFences(
				frames[current_frame_index].in_flight_fence, 
				VK_TRUE, 
				LONG_TIMEOUT
			);
			
			if (result == vk::Result::eTimeout) {
				app->GetLogger()->Error(std::format("Fence wait timed out after 1 second for frame {}, trying final wait...", current_frame_index), "VKRender");
				
				// Final attempt with longer timeout
				result = device->GetDevice().waitForFences(
					frames[current_frame_index].in_flight_fence, 
					VK_TRUE,
					MAX_TIMEOUT
				);
				
				if (result == vk::Result::eTimeout) {
					app->GetLogger()->Error(std::format("CRITICAL: Fence wait TIMEOUT for frame {} after 5+ seconds!", current_frame_index), "VKRender");
					app->GetLogger()->Error("GPU appears to be hung or in an unrecoverable state.", "VKRender");
					
					// CRITICAL: Force reset the fence to prevent infinite hangs
					app->GetLogger()->Debug("Force resetting fence to attempt recovery...", "VKRender");
					try {
						device->GetDevice().resetFences(frames[current_frame_index].in_flight_fence);
						app->GetLogger()->Debug("Fence force reset successful", "VKRender");
						
						// Skip this frame entirely to try to recover
						current_frame_index = (current_frame_index + 1) % max_frames_in_flight;
						app->GetLogger()->Debug(std::format("Skipping to next frame {} due to timeout", current_frame_index), "VKRender");
						return VK_NULL_HANDLE;
						
					} catch (const vk::SystemError& reset_error) {
						app->GetLogger()->Error(std::format("Failed to reset fence: {}", reset_error.what()), "VKRender");
						// Device is likely lost at this point
						return VK_NULL_HANDLE;
					}
				}
			}
		}
		
		if (result != vk::Result::eSuccess && result != vk::Result::eTimeout) {
			app->GetLogger()->Error(std::format("Fence wait FAILED for frame {} with result: {}", 
				current_frame_index, vk::to_string(result)), "VKRender");
			NFT_ERROR(VulkanFatal, std::format("Fence wait failed on frame {} with result {}", current_frame_index, vk::to_string(result)));
			return VK_NULL_HANDLE;
		} else if (result == vk::Result::eSuccess) {
			app->GetLogger()->Debug(std::format("Fence wait completed successfully for frame {}", current_frame_index), "VKRender");
		}
	} else {
		app->GetLogger()->Debug(std::format("Skipping fence wait for initial frame {} (total: {})", 
			current_frame_index, total_frame_count), "VKRender");
	}
	
	// CRITICAL FIX: Always reset fence before using it (required by Vulkan spec)
	// Fences must be in unsignaled state before submission
	try {
		device->GetDevice().resetFences(frames[current_frame_index].in_flight_fence);
		app->GetLogger()->Debug(std::format("Reset fence for frame {}", current_frame_index), "VKRender");
	} catch (const vk::SystemError& e) {
		app->GetLogger()->Error(std::format("Failed to reset fence for frame {}: {}", current_frame_index, e.what()), "VKRender");
		return VK_NULL_HANDLE;
	}
	
	// Acquire the next image from the swapchain
	try
	{
		auto result = device->GetDevice().acquireNextImageKHR(vk_swapchain, UINT64_MAX, 
			frames[current_frame_index].image_available_semaphore, VK_NULL_HANDLE);
		current_image_index = result.value; // Store the acquired image index
		
		app->GetLogger()->Debug(std::format("Acquired swapchain image: {} (frame_index: {})", 
			current_image_index, current_frame_index), "VKRender");
	}
	catch (const vk::OutOfDateKHRError&)
	{
		app->GetLogger()->Debug("Swapchain out of date, recreating", "VKRender");
		RecreateSwapchain();
		return VK_NULL_HANDLE;
	}
	catch (const vk::SystemError& e)
	{
		app->GetLogger()->Debug(std::format("Failed to acquire swapchain image: {}", e.what()), "VKRender");
		// Don't immediately fail - try to recreate swapchain first
		app->GetLogger()->Debug("Attempting swapchain recreation due to acquire failure", "VKRender");
		try {
			RecreateSwapchain();
			return VK_NULL_HANDLE; // Skip this frame
		} catch (const std::exception& recreate_error) {
			app->GetLogger()->Error(std::format("Swapchain recreation also failed: {}", recreate_error.what()), "VKRender");
			NFT_ERROR(VulkanFatal, std::format("Failed to acquire swapchain image: {}", e.what()));
			return VK_NULL_HANDLE;
		}
	}
	
	// SAFETY CHECK: Ensure image index is valid
	if (current_image_index >= frames.size()) {
		app->GetLogger()->Error(std::format("Acquired invalid image index {} (max: {})", 
			current_image_index, frames.size() - 1), "VKRender");
		return VK_NULL_HANDLE;
	}
	
	// Reset and begin the command buffer
	auto& command_buffer = frames[current_frame_index].vk_command_buffer;
	try {
		command_buffer.reset();
		
		vk::CommandBufferBeginInfo begin_info = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
			
		command_buffer.begin(begin_info);
	} catch (const vk::SystemError& e) {
		app->GetLogger()->Error(std::format("Failed to begin command buffer for frame {}: {}", 
			current_frame_index, e.what()), "VKRender");
		return VK_NULL_HANDLE;
	}
	
	total_frame_count++;
	return command_buffer;
}

void Surface::EndFrame()
{
	auto& current_frame = frames[current_frame_index];
	
	app->GetLogger()->Debug(std::format("EndFrame() - current_frame_index: {}, current_image_index: {}", 
		current_frame_index, current_image_index), "VKRender");
	
	// SAFETY CHECK: Validate indices
	if (current_image_index >= frames.size()) {
		app->GetLogger()->Error(std::format("current_image_index {} is out of range (max: {})!", 
			current_image_index, frames.size() - 1), "VKRender");
		NFT_ERROR(VulkanFatal, "Invalid swapchain image index");
		return;
	}
	
	auto& target_frame = frames[current_image_index];
	if (!target_frame.vk_frame_buffer) {
		app->GetLogger()->Error(std::format("Framebuffer for image {} is null!", current_image_index), "VKRender");
		NFT_ERROR(VulkanFatal, "Invalid framebuffer for swapchain image");
		return;
	}
	
	app->GetLogger()->Debug(std::format("Using framebuffer for swapchain image {} (frame {}'s framebuffer)", 
		current_image_index, current_image_index), "VKRender");
	
	// End command buffer recording
	try {
		current_frame.vk_command_buffer.end();
	} catch (const vk::SystemError& e) {
		app->GetLogger()->Error(std::format("Failed to end command buffer for frame {}: {}", 
			current_frame_index, e.what()), "VKRender");
		return;
	}
	
	app->GetLogger()->Debug(std::format("Command buffer recording completed for frame {}", current_frame_index), "VKRender");
	
	// CRITICAL FIX: Add more validation before queue submission
	if (!current_frame.image_available_semaphore || !current_frame.render_finished_semaphore || !current_frame.in_flight_fence) {
		app->GetLogger()->Error(std::format("Invalid synchronization objects for frame {}", current_frame_index), "VKRender");
		return;
	}
	
	// Submit the command buffer with enhanced error handling
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
		app->GetLogger()->Debug(std::format("Submitting command buffer for frame {} (target image: {}) with fence...", 
			current_frame_index, current_image_index), "VKRender");
		
		// Check fence state before submission
		auto fence_status = device->GetDevice().getFenceStatus(current_frame.in_flight_fence);
		app->GetLogger()->Debug(std::format("Fence status before submission: {}", vk::to_string(fence_status)), "VKRender");
		
		// CRITICAL: Ensure fence is in unsignaled state before submission
		if (fence_status == vk::Result::eSuccess) {
			app->GetLogger()->Warn("Fence is already signaled before submission! This should not happen after reset.", "VKRender");
			device->GetDevice().resetFences(current_frame.in_flight_fence);
		}
		
		// CRITICAL FIX: Add a small delay before submission to prevent race conditions
		// This ensures the previous reset operation has completed
		std::this_thread::sleep_for(std::chrono::microseconds(100));
		
		device->GetGraphicsQueue().submit(submit_info, current_frame.in_flight_fence);
		app->GetLogger()->Debug(std::format("Command buffer submitted successfully for frame {}", current_frame_index), "VKRender");
		
		// Check fence status immediately after submission (should be unsignaled since work is in progress)
		fence_status = device->GetDevice().getFenceStatus(current_frame.in_flight_fence);
		app->GetLogger()->Debug(std::format("Fence status after submission: {}", vk::to_string(fence_status)), "VKRender");
		
	}
	catch (const vk::SystemError& e)
	{
		app->GetLogger()->Error(std::format("Failed to submit command buffer for frame {}: {}", 
			current_frame_index, e.what()), "VKRender");
		// Don't throw immediately - try to continue with next frame
		app->GetLogger()->Debug("Attempting to continue despite submission failure", "VKRender");
		
		// Move to next frame anyway to avoid getting stuck
		current_frame_index = (current_frame_index + 1) % max_frames_in_flight;
		app->GetLogger()->Debug(std::format("Advanced frame index to {} due to submission failure", current_frame_index), "VKRender");
		return;
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
		app->GetLogger()->Debug(std::format("Presenting image {} with semaphore from frame {}", 
			current_image_index, current_frame_index), "VKRender");
		auto result = device->GetPresentQueue().presentKHR(present_info);
		app->GetLogger()->Debug(std::format("Present completed with result: {}", vk::to_string(result)), "VKRender");
		
		// Check fence status after present (may still be in progress)
		auto fence_status = device->GetDevice().getFenceStatus(current_frame.in_flight_fence);
		app->GetLogger()->Debug(std::format("Fence status after present: {}", vk::to_string(fence_status)), "VKRender");
		
		// Check if we need to recreate swapchain
		if (result == vk::Result::eSuboptimalKHR) {
			app->GetLogger()->Debug("Swapchain suboptimal, will recreate", "VKRender");
			RecreateSwapchain();
		}
	}
	catch (const vk::OutOfDateKHRError&)
	{
		app->GetLogger()->Debug("Swapchain out of date during present, recreating", "VKRender");
		RecreateSwapchain();
	}
	catch (const vk::SystemError& e)
	{
		app->GetLogger()->Error(std::format("Failed to present image: {}", e.what()), "VKRender");
		// Try to recreate swapchain instead of immediate failure
		app->GetLogger()->Debug("Attempting swapchain recreation due to present failure", "VKRender");
		try {
			RecreateSwapchain();
		} catch (const std::exception& recreate_error) {
			app->GetLogger()->Error(std::format("Swapchain recreation failed: {}", recreate_error.what()), "VKRender");
			NFT_ERROR(VulkanFatal, std::format("Failed to present swapchain image: {}", e.what()));
		}
	}
	
	// Move to next frame
	size_t old_frame_index = current_frame_index;
	current_frame_index = (current_frame_index + 1) % max_frames_in_flight;
	app->GetLogger()->Debug(std::format("Advanced frame index from {} to {}", old_frame_index, current_frame_index), "VKRender");
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

	// CRITICAL FIX: Handle device lost errors during cleanup
	try {
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
	}
	catch (const vk::SystemError& e) {
		// Handle device lost errors gracefully
		if (app && app->GetLogger()) {
			app->GetLogger()->Warn(std::format("Device lost error during surface cleanup: {}", e.what()), "VKRender");
		}
		// Continue with cleanup anyway - we need to release resources even if device is lost
		
		// Reset Vulkan handles to prevent double cleanup
		vk_command_pool = VK_NULL_HANDLE;
		vk_swapchain = VK_NULL_HANDLE;
		
		// Clear frame data without device operations
		frames.clear();
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
		for (const auto& mode : support_details.present_modes) {
			if (mode == vk::PresentModeKHR::eMailbox) {
				present_mode = mode;
				return;
			}
		}
		present_mode = support_details.present_modes[0];
	}
}

void Surface::LogSupportDetails()
{
	// Simple logging implementation - details omitted for brevity
	app->GetLogger()->Debug("Surface support details logged", "VKInit");
}

}	 // namespace nft::vulkan