//=============================================================================
// VULKAN SURFACE IMPLEMENTATION
//=============================================================================
// This file implements the Vulkan surface management, including swapchain
// creation, pipeline setup, and all related rendering resources.

#include "vk/surface.h"

#include "core/app.h"
#include "core/error.h"

#include "vk/handler.h"
#include "vk/image.h"
#include "vk/scene.h"

#include "core/glfw_common.h"

#include <../generated/picking_shader.frag.spv.h>
#include <../generated/picking_shader.vert.spv.h>
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
	vk_pipeline(VK_NULL_HANDLE),
	vk_command_pool(VK_NULL_HANDLE),
	vk_command_buffer(VK_NULL_HANDLE),
	vertex_input_stage(device),
	input_assembly_stage(device),
	viewport_stage(device),
	rasterization_stage(device),
	depth_stencil_stage(device),
	multisample_stage(device),
	color_blend_stage(device),
	frame_set_layout(this),		 // Keep instance pointer for surface context and debugging
	texture_set_layout(this),	 // Keep instance pointer for surface context and debugging
	frame_descriptor_pool(device),
	texture_descriptor_pool(device),
	pipeline_layout(device),
	render_pass(device),
	clear_color(vk::ClearColorValue(std::array<float, 4> { 0.2f, 0.2f, 0.2f, 1.0f })),
	clear_depth(vk::ClearDepthStencilValue(1.0f, 0)),
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
	CreatePipeline();
	CreateFrameBuffers();
	CreateFrameCommandBuffers();

	object_picker = std::make_unique<ObjectPicker>(device, extent);
}

void Surface::InitSwapchain()
{
	// app->GetLogger()->Debug(std::format("Creating Swapchain For Window: \"{}\"...", glfwGetWindowTitle(window)), "VKInit");
	// app->GetLogger()->Debug("Querying Swapchain Support...", "VKInit");

	// Query swapchain support details
	support_details.capabilities  = device->vk_physical_device.getSurfaceCapabilitiesKHR(vk_surface);
	support_details.formats		  = device->vk_physical_device.getSurfaceFormatsKHR(vk_surface);
	support_details.present_modes = device->vk_physical_device.getSurfacePresentModesKHR(vk_surface);

	LogSupportDetails();

	// Create synchronization objects using Device wrapper methods
	// app->GetLogger()->Debug("Creating Synchronization Objects...", "VKInit");
	// for (auto& frame : frames)
	//{
	//	frame.in_flight_fence			= device->CreateFence(true);
	//	frame.image_available_semaphore = device->CreateSemaphore();
	//	frame.render_finished_semaphore = device->CreateSemaphore();
	//	frame.Init(this, scene.get());
	//	frame.MakeDescriptorResources();
	//}

	CreateSwapchain();
	// app->GetLogger()->Debug("Synchronization Objects Created Successfully!", "VKInit");
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
	std::vector queue_family_indices = device->queue_family_indices.Vec();
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
	if (device->queue_family_indices.graphics_family != device->queue_family_indices.present_family)
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
		vk_swapchain = device->vk_device.createSwapchainKHR(vk_swapchain_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Create Swapchain:\n{}", err.what()));
	}
	// Get swapchain images and create image views
	// Get swapchain images and create image views
	std::vector<vk::Image> image_vec = device->vk_device.getSwapchainImagesKHR(vk_swapchain);
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

	device->vk_device.waitIdle();
	// Cleanup old swapchain resources
	CleanupSwapchain();
	frame_descriptor_pool.Cleanup();
	// Recreate swapchain and related resources
	InitSwapchain();
	CreateFrameBuffers();
	CreateFrameCommandBuffers();

	std::vector<DescriptorSetLayout::Binding> frame_bindings = {
		{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
		{ 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex }
	};
	frame_descriptor_pool.Init(frame_bindings, frames.size());

	for (auto& frame : frames)
		frame.AllocateDescriptorResources();

	if (object_picker)
		object_picker->Recreate(extent);
}

//=============================================================================
// PIPELINE CREATION METHODS
//=============================================================================

void Surface::CreatePipeline()
{

	app->GetLogger()->Debug("Creating Pipeline...", "VKInit");

	// Initialize pipeline stages for better performance with inline initialization
	vertex_input_stage.Init();
	input_assembly_stage.Init(vk::PrimitiveTopology::eTriangleList);

	// Initialize shader stages
	shader_stages.clear();
	shader_stages.reserve(2);	 // Performance: reserve space for vertex and fragment shaders

	// Add vertex shader
	shader_stages.emplace_back(device);
	shader_stages.back().shader =
		std::make_unique<Shader>(device, Shader::ShaderCode { (uint32_t*)simple_shader_vert, simple_shader_vert_len });
	shader_stages.back().vk_shader_stage_info = vk::PipelineShaderStageCreateInfo()
													.setFlags(vk::PipelineShaderStageCreateFlags())
													.setStage(vk::ShaderStageFlagBits::eVertex)
													.setModule(shader_stages.back().shader->GetShaderModule())
													.setPName("main");

	viewport_stage.Init(extent);
	rasterization_stage.Init();

	// Add fragment shader
	shader_stages.emplace_back(device);
	shader_stages.back().shader =
		std::make_unique<Shader>(device, Shader::ShaderCode { (uint32_t*)simple_shader_frag, simple_shader_frag_len });
	shader_stages.back().vk_shader_stage_info = vk::PipelineShaderStageCreateInfo()
													.setFlags(vk::PipelineShaderStageCreateFlags())
													.setStage(vk::ShaderStageFlagBits::eFragment)
													.setModule(shader_stages.back().shader->GetShaderModule())
													.setPName("main");

	depth_stencil_stage.Init();

	multisample_stage.Init();
	color_blend_stage.Init();

	// Set 0: Frame data (camera + object transforms)
	std::vector<DescriptorSetLayout::Binding> frame_bindings = {
		{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
		{ 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex }
	};

	frame_set_layout.Init(frame_bindings);

	// Set 1: Texture array (all textures in one binding)
	std::vector<DescriptorSetLayout::Binding> texture_bindings = {
		{ 0, vk::DescriptorType::eCombinedImageSampler, 32, vk::ShaderStageFlagBits::eFragment }	// Array of 32 textures
	};

	texture_set_layout.Init(texture_bindings);

	// Update descriptor set layouts vector
	vk_descriptor_set_layouts = { frame_set_layout.vk_descriptor_set_layout, texture_set_layout.vk_descriptor_set_layout };

	frame_descriptor_pool.Init(frame_bindings, frames.size());
	texture_descriptor_pool.Init(texture_bindings, 1);	  // Only need one set for all textures

	for (auto& frame : frames)
		frame.AllocateDescriptorResources();

	// Create a single texture descriptor set for all material textures
	CreateTextureDescriptorSet();

	vk::PushConstantRange material_push_constant_range = vk::PushConstantRange()
															 .setOffset(0)
															 .setSize(sizeof(MaterialPushConstants))
															 .setStageFlags(vk::ShaderStageFlagBits::eFragment);

	pipeline_layout.Init(vk_descriptor_set_layouts, material_push_constant_range);
	render_pass.Init(format.format, depth_format);

	// Create pipeline info with all stages
	std::vector<vk::PipelineShaderStageCreateInfo> shader_stage_info;
	shader_stage_info.reserve(shader_stages.size());	// Performance optimization
	for (const auto& shader_stage : shader_stages)
		shader_stage_info.push_back(shader_stage.vk_shader_stage_info);

	vk_pipeline_info = vk::GraphicsPipelineCreateInfo()
						   .setFlags(vk::PipelineCreateFlags())
						   .setStageCount(shader_stage_info.size())
						   .setPStages(shader_stage_info.data())
						   .setPVertexInputState(&vertex_input_stage.vk_vertex_input_info)
						   .setPInputAssemblyState(&input_assembly_stage.vk_input_assembly_info)
						   .setPViewportState(&viewport_stage.vk_viewport_state_info)
						   .setPRasterizationState(&rasterization_stage.vk_rasterization_info)
						   .setPDepthStencilState(&depth_stencil_stage.vk_depth_stencil_info)
						   .setPMultisampleState(&multisample_stage.vk_multisample_info)
						   .setPColorBlendState(&color_blend_stage.vk_color_blend_info)
						   .setLayout(pipeline_layout.vk_pipeline_layout)
						   .setRenderPass(render_pass.vk_render_pass)
						   .setSubpass(0)
						   .setBasePipelineHandle(nullptr);

	try
	{
		vk_pipeline = device->vk_device.createGraphicsPipeline(nullptr, vk_pipeline_info).value;
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Create Graphics Pipeline:\n{}", err.what()));
	}

	app->GetLogger()->Debug("Pipeline Created Successfully!", "VKInit");
}

void Surface::CreateTextureDescriptorSet()
{
	// Create texture array descriptor set
	if (scene->textures.empty())
	{
		NFT_ERROR(VulkanFatal, "No textures available for texture array descriptor set!");
		return;
	}

	vk::DescriptorSetAllocateInfo alloc_info = vk::DescriptorSetAllocateInfo()
												   .setDescriptorPool(texture_descriptor_pool.vk_descriptor_pool)
												   .setDescriptorSetCount(1)
												   .setPSetLayouts(&texture_set_layout.vk_descriptor_set_layout);
	try
	{
		texture_descriptor_set = device->vk_device.allocateDescriptorSets(alloc_info)[0];
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Allocate Texture Array Descriptor Set:\n{}", err.what()));
	}

	// Create array of descriptor image infos
	std::vector<vk::DescriptorImageInfo> image_infos;
	image_infos.reserve(32);	// Reserve space for 32 textures

	// Add all available textures
	for (size_t i = 0; i < scene->textures.size() && i < 32; ++i)
	{
		image_infos.push_back(vk::DescriptorImageInfo()
								  .setSampler(scene->textures[i].vk_sampler)
								  .setImageView(scene->textures[i].GetImageView())
								  .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));
	}

	// Fill remaining slots with the first texture to avoid validation errors
	for (size_t i = scene->textures.size(); i < 32; ++i)
	{
		image_infos.push_back(vk::DescriptorImageInfo()
								  .setSampler(scene->textures[0].vk_sampler)
								  .setImageView(scene->textures[0].GetImageView())
								  .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));
	}

	// Update descriptor set with texture array
	vk::WriteDescriptorSet descriptor_write = vk::WriteDescriptorSet()
												  .setDstSet(texture_descriptor_set)
												  .setDstBinding(0)
												  .setDstArrayElement(0)
												  .setDescriptorCount(32)
												  .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
												  .setPImageInfo(image_infos.data());

	device->vk_device.updateDescriptorSets(1, &descriptor_write, 0, nullptr);
}

//=============================================================================
// FRAMEBUFFER AND COMMAND CREATION METHODS
//=============================================================================

void Surface::CreateFrameBuffers()
{
	size_t i = 0;
	for (auto& frame : frames)
	{
		auto&					   frame_image_view		   = frame.swapchain_image.GetImageView();
		auto&					   frame_depth_buffer_view = frame.depth_buffer.GetImageView();
		std::vector<vk::ImageView> attachments			   = { frame_image_view, frame_depth_buffer_view };

		frame.vk_frame_buffer_info = vk::FramebufferCreateInfo()
										 .setFlags(vk::FramebufferCreateFlags())
										 .setRenderPass(render_pass.vk_render_pass)
										 .setAttachmentCount(attachments.size())
										 .setPAttachments(attachments.data())
										 .setWidth(extent.width)
										 .setHeight(extent.height)
										 .setLayers(1);

		try
		{
			frame.vk_frame_buffer = device->vk_device.createFramebuffer(frame.vk_frame_buffer_info);
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
							   .setQueueFamilyIndex(device->queue_family_indices.graphics_family.value());

	try
	{
		vk_command_pool = device->vk_device.createCommandPool(vk_command_pool_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Create Command Pool:\n{}", err.what()));
	}

	// Allocate main command buffer
	try
	{
		vk_command_buffer = device->vk_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
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
		frame.vk_command_buffer = device->vk_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
																			   .setCommandPool(vk_command_pool)
																			   .setLevel(vk::CommandBufferLevel::ePrimary)
																			   .setCommandBufferCount(1))[0];
	}
}

//=========================================================================
// RENDERING METHODS
//=========================================================================

void Surface::PrepareScene(vk::CommandBuffer command_buffer)
{
	// Prepare the scene for rendering
	if (!scene)
	{
		NFT_ERROR(VulkanFatal, "Scene is not set for rendering!");
		return;
	}

	vk::Buffer	 vertex_buffers[] = { scene->GetGeometryBatcher()->vertex_buffer->vk_buffer };
	VkDeviceSize offsets[]		  = { 0 };	  // Start from the beginning of the buffer
	command_buffer.bindVertexBuffers(0, 1, vertex_buffers, offsets);
	if (scene->geometry_batcher->index_data.empty())
		return;
	command_buffer.bindIndexBuffer(scene->geometry_batcher->index_buffer->vk_buffer, 0, vk::IndexType::eUint32);
}

void Surface::Render()
{
	Frame& current_frame = frames[frame_index];

	device->vk_device.waitForFences(current_frame.in_flight_fence, VK_TRUE, UINT64_MAX);
	device->vk_device.resetFences(current_frame.in_flight_fence);

	uint32_t image_index;
	try
	{
		vk::ResultValue aquire = device->vk_device.acquireNextImage2KHR(vk::AcquireNextImageInfoKHR()
																			.setSwapchain(vk_swapchain)
																			.setTimeout(UINT64_MAX)
																			.setSemaphore(current_frame.image_available_semaphore)
																			.setFence(nullptr)
																			.setDeviceMask(1));
		image_index			   = aquire.value;
	}
	catch (vk::OutOfDateKHRError)
	{
		RecreateSwapchain();
		return;
	}

	vk::CommandBuffer command_buffer = current_frame.vk_command_buffer;
	command_buffer.reset(vk::CommandBufferResetFlags());

	current_frame.Prepare(scene->camera_transforms);
	RecordDrawCommands(current_frame, image_index);

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
		device->vk_graphics_queue.submit(submit_info, current_frame.in_flight_fence);
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

void Surface::RecordDrawCommands(Frame& frame, uint32_t image_index)
{
	// Record commands for each frame
	auto					   command_buffer = frame.vk_command_buffer;
	vk::CommandBufferBeginInfo begin_info	  = vk::CommandBufferBeginInfo();
	command_buffer.begin(begin_info);

	std::vector<vk::ClearValue> clear_values = { clear_color, clear_depth };

	vk::RenderPassBeginInfo render_pass_begin_info = vk::RenderPassBeginInfo()
														 .setRenderPass(render_pass.vk_render_pass)
														 .setFramebuffer(frames[image_index].vk_frame_buffer)
														 .setRenderArea(vk::Rect2D().setOffset({ 0, 0 }).setExtent(extent))
														 .setClearValueCount(clear_values.size())
														 .setPClearValues(clear_values.data());
	command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

	// Bind frame descriptor set (set 0: camera + transforms)
	command_buffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics, pipeline_layout.vk_pipeline_layout, 0, { frame.vk_descriptor_set }, nullptr);

	// Bind texture descriptor set (set 1: material textures)
	command_buffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics, pipeline_layout.vk_pipeline_layout, 1, { texture_descriptor_set }, nullptr);

	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vk_pipeline);

	PrepareScene(command_buffer);

	//app->GetLogger()->Debug(std::format("Total objects: {}", scene->objects.size()), "VKRender");
	//app->GetLogger()->Debug(std::format("Mesh data entries: {}", scene->geometry_batcher->mesh_data.size()), "VKRender");

	const auto& meshes = scene->geometry_batcher->mesh_data;

	for (const auto& mesh_entry : meshes)
	{
		const IMesh*					 mesh	   = mesh_entry.first;
		const GeometryBatcher::MeshData& mesh_data = mesh_entry.second;

		uint32_t vertex_count = static_cast<uint32_t>(mesh_data.size);
		uint32_t first_vertex = static_cast<uint32_t>(mesh_data.offset);
		uint32_t index_count  = static_cast<uint32_t>(mesh_data.index_size);
		uint32_t first_index  = static_cast<uint32_t>(mesh_data.index_offset);

		//app->GetLogger()->Debug(
		//	std::format(
		//		"Mesh vertex data: count={}, offset={}, data_size={}", vertex_count, first_vertex, mesh->vertices->size()),
		//	"VKRender");

		// Draw each object separately with per-object material push constants
		for (uint32_t instance_id = 0; instance_id < scene->objects.size(); ++instance_id)
		{
			if (scene->objects[instance_id].mesh == mesh)
			{
				const auto& object = scene->objects[instance_id];

				//app->GetLogger()->Debug(std::format("Drawing object {}: vertex_count={}, first_vertex={}, instance_id={}",
				//									instance_id,
				//									vertex_count,
				//									first_vertex,
				//									instance_id),
				//						"VKRender");

				// Create material push constants for this object
				MaterialPushConstants material_push;
				if (object.material_index < scene->materials.size())
				{
					const auto& material			  = scene->materials[object.material_index];
					material_push.ambient			  = material.ambient;
					material_push.diffuse			  = material.diffuse;
					material_push.specular			  = material.specular;
					material_push.specular_highlights = material.specular_highlights;
					material_push.diffuse_texture_index =
						material.diffuse_texture_index != UINT32_MAX ? material.diffuse_texture_index : 33;
					material_push.ambient_texture_index	 = material.ambient_texture_index != UINT32_MAX
															   ? material.ambient_texture_index
															   : 33;	// Use invalid index (will be clamped in shader)
					material_push.specular_texture_index = material.specular_texture_index != UINT32_MAX
															   ? material.specular_texture_index
															   : 33;	// Use invalid index
					material_push.padding				 = 0;
				}
				else
				{
					// Default material
					material_push.ambient				 = glm::vec3(0.1f);
					material_push.diffuse				 = glm::vec3(0.8f);
					material_push.specular				 = glm::vec3(0.5f);
					material_push.specular_highlights	 = 32.0f;
					material_push.diffuse_texture_index	 = 0;	  // Use first texture as default
					material_push.ambient_texture_index	 = 31;	  // Invalid index
					material_push.specular_texture_index = 31;	  // Invalid index
					material_push.padding				 = 0;
				}

				// Push the material constants
				command_buffer.pushConstants(pipeline_layout.vk_pipeline_layout,
											 vk::ShaderStageFlagBits::eFragment,
											 0,
											 sizeof(MaterialPushConstants),
											 &material_push);

				if (index_count == 0)
				{
					// Draw without index buffer
					command_buffer.draw(vertex_count, 1, first_vertex, instance_id);
				}
				else
				{
					// Draw with index buffer
					command_buffer.drawIndexed(index_count, 1, first_index, 0, instance_id);
				}
			}
		}
	}

	command_buffer.endRenderPass();
	command_buffer.end();
}

//=============================================================================
// CLEANUP METHODS
//=============================================================================

void Surface::CleanupSwapchain()
{
	// Cleanup swapchain resources
	if (vk_swapchain && device && instance)
	{
		device->vk_device.destroySwapchainKHR(vk_swapchain);
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

	// Same cleanup logic as before, but can be called explicitly
	if (device && device->vk_device)
	{
		device->vk_device.waitIdle();

		app->GetLogger()->Debug("Cleaning up Surface Vulkan objects...", "VKShutdown");

		if (vk_command_pool)
		{
			device->vk_device.destroyCommandPool(vk_command_pool);
			vk_command_pool = VK_NULL_HANDLE;
		}

		if (vk_pipeline)
		{
			device->vk_device.destroyPipeline(vk_pipeline);
			vk_pipeline = VK_NULL_HANDLE;
		}

		for (auto& shader_stage : shader_stages)
			if (shader_stage.shader)
				shader_stage.shader.reset();	// This calls Shader destructor which destroys the shader module
		shader_stages.clear();

		render_pass.Cleanup();
		pipeline_layout.Cleanup();

		CleanupSwapchain();

		frame_descriptor_pool.Cleanup();
		texture_descriptor_pool.Cleanup();

		frame_set_layout.Cleanup();
		texture_set_layout.Cleanup();

		app->GetLogger()->Debug("Surface Vulkan objects cleaned up successfully", "VKShutdown");
	}

	if (object_picker)
		object_picker.reset();

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

//=============================================================================
// LOGGING UTILITY FUNCTIONS
//=============================================================================

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

void Surface::LogSupportDetails()
{
	Logger::DisplayFlags log_flags = Log::Flags::Default & ~Log::Flags::ShowHeader;
	log_surface_capabilities(app->GetLogger(), log_flags, support_details.capabilities);
	log_surface_format_support(app->GetLogger(), log_flags, support_details.formats);
	log_surface_present_mode(app->GetLogger(), log_flags, support_details.present_modes);
	return;
}

void Surface::Frame::Init(Surface* surface, Scene* scene)
{
	if (!surface)
		NFT_ERROR(VulkanFatal, "Surface pointer is null!");
	if (!scene)
		NFT_ERROR(VulkanFatal, "Scene pointer is null!");
	this->surface	= surface;
	this->device	= surface->device;
	this->scene		= scene;
	swapchain_image = Image(device);
	depth_buffer	= Image(device);
}

void Surface::Frame::MakeDescriptorResources()
{
	if (!device)
		NFT_ERROR(VulkanFatal, "Device pointer is null!");
	camera_data_buffer = device->buffer_manager->CreateBuffer(sizeof(UniformBufferObject),
															  vk::BufferUsageFlagBits::eUniformBuffer,
															  vk::MemoryPropertyFlagBits::eHostVisible |
																  vk::MemoryPropertyFlagBits::eHostCoherent);
	camera_data_ptr	   = device->vk_device.mapMemory(
		   camera_data_buffer->vk_memory, 0, camera_data_buffer->vk_memory_info.allocationSize, vk::MemoryMapFlags());

	size_t buffer_size	= sizeof(glm::mat4) * scene->objects.size();
	size_t aligned_size = ((buffer_size + 15) / 16) * 16;	 // Align to 256 bytes

	object_transform_buffer = device->buffer_manager->CreateBuffer(buffer_size,
																   vk::BufferUsageFlagBits::eStorageBuffer,
																   vk::MemoryPropertyFlagBits::eHostVisible |
																	   vk::MemoryPropertyFlagBits::eHostCoherent);
	object_transform_ptr	= device->vk_device.mapMemory(
		   object_transform_buffer->vk_memory, 0, object_transform_buffer->vk_memory_info.allocationSize, vk::MemoryMapFlags());

	// CRITICAL FIX: Clear the GPU memory to zero
	std::memset(object_transform_ptr, 0, object_transform_buffer->vk_memory_info.allocationSize);

	object_transforms.resize(scene->objects.size());
	for (int i = 0; i < scene->objects.size(); ++i)
		object_transforms[i] = glm::mat4(1.0f);	   // Initialize with identity matrices
}

void Surface::Frame::AllocateDescriptorResources()
{
	if (!surface)
		NFT_ERROR(VulkanFatal, "Surface pointer is null!");
	if (!device)
		NFT_ERROR(VulkanFatal, "Device pointer is null!");
	if (!camera_data_buffer)
	{
		NFT_ERROR(VulkanFatal, "Camera data buffer is not initialized!");
		return;
	}

	// Allocate frame descriptor set (camera + transforms)
	vk::DescriptorSetAllocateInfo frame_alloc_info = vk::DescriptorSetAllocateInfo()
														 .setDescriptorPool(surface->frame_descriptor_pool.vk_descriptor_pool)
														 .setDescriptorSetCount(1)
														 .setPSetLayouts(&surface->frame_set_layout.vk_descriptor_set_layout);
	try
	{
		vk_descriptor_set = device->vk_device.allocateDescriptorSets(frame_alloc_info)[0];
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Allocate Frame Descriptor Set:\n{}", err.what()));
	}
}

void Surface::Frame::MakeDepthResources()
{
	if (!surface)
		NFT_ERROR(VulkanFatal, "Surface pointer is null!");
	if (!device)
		NFT_ERROR(VulkanFatal, "Device pointer is null!");

	// vk_depth_buffer			= device->buffer_manager->CreateImage(surface->vk_swapchain_info.imageExtent.width,
	//													  surface->vk_swapchain_info.imageExtent.height,
	//													  depth_format,
	//													  vk::ImageUsageFlagBits::eDepthStencilAttachment,
	//													  vk::MemoryPropertyFlagBits::eDeviceLocal);
	// vk_depth_buffer_view		= device->buffer_manager->CreateImageView(depth_buffer, vk::ImageViewType::e2D, depth_format);
}

void Surface::Frame::Prepare(glm::mat4 camera_transforms)
{
	if (!surface)
		NFT_ERROR(VulkanFatal, "Surface pointer is null!");
	if (!scene)
		NFT_ERROR(VulkanFatal, "Scene pointer is null!");
	if (!camera_data_buffer)
		NFT_ERROR(VulkanFatal, "Camera data buffer is not initialized!");

	glm::vec3 eye = glm::vec3(camera_transforms[3]);

	// Define a base center direction (e.g., looking forward)
	glm::vec3 base_center_direction = glm::vec3(0.0f, 0.0f, -1.0f);	   // Looking down negative Z

	// Extract the rotation part of the camera transform (upper-left 3x3 matrix)
	glm::mat3 rotation_matrix = glm::mat3(camera_transforms);

	// Apply the camera rotation to the base center direction
	glm::vec3 rotated_direction = rotation_matrix * base_center_direction;

	// Calculate the center point
	glm::vec3 center = eye + rotated_direction;

	// Use the up vector from the transform
	glm::vec3 up = glm::normalize(glm::vec3(camera_transforms[1]));

	camera_data.view = glm::lookAt(eye, center, up);

	camera_data.proj = glm::perspective(glm::radians(45.0f),
										static_cast<float>(surface->vk_swapchain_info.imageExtent.width) /
											static_cast<float>(surface->vk_swapchain_info.imageExtent.height),
										0.1f,
										100.0f);

	float aspect = static_cast<float>(surface->vk_swapchain_info.imageExtent.width) /
				   static_cast<float>(surface->vk_swapchain_info.imageExtent.height);
	camera_data.proj[1][1] *= -1;
	camera_data.pos = eye;

	std::memcpy(camera_data_ptr, &camera_data, sizeof(UniformBufferObject));

	// Gather object transforms
	const size_t object_count = scene->objects.size();
	for (size_t idx = 0; idx < object_count; ++idx)
	{
		object_transforms[idx] = scene->objects[idx].transform;
	}

	const size_t bytes = object_count * sizeof(glm::mat4);
	std::memcpy(object_transform_ptr, object_transforms.data(), bytes);

	// Update frame descriptor set (camera + transforms)
	std::vector<vk::DescriptorBufferInfo> buffer_infos;
	buffer_infos.push_back(
		vk::DescriptorBufferInfo().setBuffer(camera_data_buffer->vk_buffer).setOffset(0).setRange(sizeof(UniformBufferObject)));
	buffer_infos.push_back(vk::DescriptorBufferInfo().setBuffer(object_transform_buffer->vk_buffer).setOffset(0).setRange(bytes));

	std::vector<vk::WriteDescriptorSet> descriptor_writes;
	descriptor_writes.push_back(vk::WriteDescriptorSet()
									.setDstSet(vk_descriptor_set)
									.setDstBinding(0)
									.setDstArrayElement(0)
									.setDescriptorCount(1)
									.setDescriptorType(vk::DescriptorType::eUniformBuffer)
									.setPBufferInfo(&buffer_infos[0]));
	descriptor_writes.push_back(vk::WriteDescriptorSet()
									.setDstSet(vk_descriptor_set)
									.setDstBinding(1)
									.setDstArrayElement(0)
									.setDescriptorCount(1)
									.setDescriptorType(vk::DescriptorType::eStorageBuffer)
									.setPBufferInfo(&buffer_infos[1])
									.setPTexelBufferView(nullptr)
									.setPImageInfo(nullptr));

	device->vk_device.updateDescriptorSets(descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
}

void Surface::Frame::Cleanup()
{
	if (!device)
		NFT_ERROR(VulkanFatal, "Device pointer is null!");
	if (vk_frame_buffer)
		device->vk_device.destroyFramebuffer(vk_frame_buffer);
	if (in_flight_fence)
		device->vk_device.destroyFence(in_flight_fence);
	if (image_available_semaphore)
		device->vk_device.destroySemaphore(image_available_semaphore);
	if (render_finished_semaphore)
		device->vk_device.destroySemaphore(render_finished_semaphore);
	if (camera_data_buffer)
	{
		if (camera_data_ptr)
		{
			device->vk_device.unmapMemory(camera_data_buffer->vk_memory);
			camera_data_ptr = nullptr;
		}
		device->buffer_manager->DestroyBuffer(camera_data_buffer);
		camera_data_buffer = nullptr;
	}
	if (object_transform_buffer)
	{
		if (object_transform_ptr)
		{
			device->vk_device.unmapMemory(object_transform_buffer->vk_memory);
			object_transform_ptr = nullptr;
		}
		device->buffer_manager->DestroyBuffer(object_transform_buffer);
		object_transform_buffer = nullptr;
	}
}

//=============================================================================
// OBJECT PICKER IMPLEMENTATION
//=============================================================================

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
	picking_set_layout(device),	   // Keep instance pointer for surface context and debugging
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

	// Allocate main command buffer
	try
	{
		vk_command_buffer = device->vk_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
																		 .setCommandPool(vk_command_pool)
																		 .setLevel(vk::CommandBufferLevel::ePrimary)
																		 .setCommandBufferCount(1))[0];
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Allocate Main Command Buffer:\n{}", err.what()));
	}
}

void ObjectPicker::Init()
{

	// Create offscreen images for color and depth
	color_attachment.SetDevice(device);
	color_attachment.Init(vk::ImageCreateInfo()
							  .setImageType(vk::ImageType::e2D)
							  .setExtent(vk::Extent3D(extent, 1))
							  .setFormat(vk::Format::eR32G32B32A32Uint)	   // Use UINT format for object IDs
							  .setTiling(vk::ImageTiling::eOptimal)
							  .setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc),
						  vk::MemoryPropertyFlagBits::eDeviceLocal);
	color_attachment.CreateImageView(vk::Format::eR32G32B32A32Uint);

	depth_attachment.SetDevice(device);
	vk::Format depth_format = FindFormat(device,
										 { vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint },
										 vk::ImageTiling::eOptimal,
										 vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	depth_attachment.Init(vk::ImageCreateInfo()
							  .setImageType(vk::ImageType::e2D)
							  .setExtent(vk::Extent3D(extent, 1))
							  .setFormat(depth_format)
							  .setTiling(vk::ImageTiling::eOptimal)
							  .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment),
						  vk::MemoryPropertyFlagBits::eDeviceLocal);
	depth_attachment.CreateImageView(depth_format);

	// Create readback buffer for CPU access
	readback_buffer = device->buffer_manager->CreateBuffer(4 * sizeof(uint32_t),	// RGBA32_UINT
														   vk::BufferUsageFlagBits::eTransferDst,
														   vk::MemoryPropertyFlagBits::eHostVisible |
															   vk::MemoryPropertyFlagBits::eHostCoherent);

	CreateCommandPool();
	CreateRenderPass();
	CreateShaders();
	CreatePipeline();
	CreateFramebuffer();

	// Allocate command buffer
	command_buffer =
		device->vk_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
													 .setCommandPool(vk_command_pool)	 // You'll need to expose this
													 .setLevel(vk::CommandBufferLevel::ePrimary)
													 .setCommandBufferCount(1))[0];
}

void ObjectPicker::CreateRenderPass()
{
	// Color attachment for object IDs
	vk::AttachmentDescription color_attachment_desc = vk::AttachmentDescription()
														  .setFormat(vk::Format::eR32G32B32A32Uint)
														  .setSamples(vk::SampleCountFlagBits::e1)
														  .setLoadOp(vk::AttachmentLoadOp::eClear)
														  .setStoreOp(vk::AttachmentStoreOp::eStore)
														  .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
														  .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
														  .setInitialLayout(vk::ImageLayout::eUndefined)
														  .setFinalLayout(vk::ImageLayout::eTransferSrcOptimal);

	vk::AttachmentReference color_attachment_ref =
		vk::AttachmentReference().setAttachment(0).setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	// Depth attachment
	vk::Format depth_format = FindFormat(device,
										 { vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint },
										 vk::ImageTiling::eOptimal,
										 vk::FormatFeatureFlagBits::eDepthStencilAttachment);

	vk::AttachmentDescription depth_attachment_desc = vk::AttachmentDescription()
														  .setFormat(depth_format)
														  .setSamples(vk::SampleCountFlagBits::e1)
														  .setLoadOp(vk::AttachmentLoadOp::eClear)
														  .setStoreOp(vk::AttachmentStoreOp::eDontCare)
														  .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
														  .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
														  .setInitialLayout(vk::ImageLayout::eUndefined)
														  .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::AttachmentReference depth_attachment_ref =
		vk::AttachmentReference().setAttachment(1).setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::SubpassDescription subpass = vk::SubpassDescription()
										 .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
										 .setColorAttachmentCount(1)
										 .setPColorAttachments(&color_attachment_ref)
										 .setPDepthStencilAttachment(&depth_attachment_ref);

	std::vector<vk::AttachmentDescription> attachments = { color_attachment_desc, depth_attachment_desc };

	vk::RenderPassCreateInfo render_pass_info = vk::RenderPassCreateInfo()
													.setAttachmentCount(attachments.size())
													.setPAttachments(attachments.data())
													.setSubpassCount(1)
													.setPSubpasses(&subpass);

	render_pass.vk_render_pass = device->vk_device.createRenderPass(render_pass_info);
}

void ObjectPicker::CreateShaders()
{
	// You'll need to create these shader includes similar to how the main shaders are handled
	// For now, I'll assume you have the compiled shader code available
	picking_shader_stages.clear();
	picking_shader_stages.reserve(2);

	// Vertex shader
	picking_shader_stages.emplace_back(device);
	picking_shader_stages.back().shader =
		std::make_unique<Shader>(device, Shader::ShaderCode { (uint32_t*)picking_shader_vert, picking_shader_vert_len });
	picking_shader_stages.back().vk_shader_stage_info = vk::PipelineShaderStageCreateInfo()
															.setStage(vk::ShaderStageFlagBits::eVertex)
															.setModule(picking_shader_stages.back().shader->GetShaderModule())
															.setPName("main");

	// Fragment shader
	picking_shader_stages.emplace_back(device);
	picking_shader_stages.back().shader =
		std::make_unique<Shader>(device, Shader::ShaderCode { (uint32_t*)picking_shader_frag, picking_shader_frag_len });
	picking_shader_stages.back().vk_shader_stage_info = vk::PipelineShaderStageCreateInfo()
															.setStage(vk::ShaderStageFlagBits::eFragment)
															.setModule(picking_shader_stages.back().shader->GetShaderModule())
															.setPName("main");
}

void ObjectPicker::CreatePipeline()
{
	vertex_input_stage.Init();
	pipeline_info.setPVertexInputState(&vertex_input_stage.vk_vertex_input_info);

	input_assembly_stage.Init(vk::PrimitiveTopology::eTriangleList);
	pipeline_info.setPInputAssemblyState(&input_assembly_stage.vk_input_assembly_info);

	viewport_stage.Init(extent);
	pipeline_info.setPViewportState(&viewport_stage.vk_viewport_state_info);

	rasterization_stage.Init();
	pipeline_info.setPRasterizationState(&rasterization_stage.vk_rasterization_info);

	depth_stencil_stage.Init();
	pipeline_info.setPDepthStencilState(&depth_stencil_stage.vk_depth_stencil_info);

	multisample_stage.Init();
	pipeline_info.setPMultisampleState(&multisample_stage.vk_multisample_info);

	color_blend_stage.Init();
	pipeline_info.setPColorBlendState(&color_blend_stage.vk_color_blend_info);

	std::vector<vk::PipelineShaderStageCreateInfo> shader_stage_info;
	for (const auto& shader_stage : picking_shader_stages)
		shader_stage_info.push_back(shader_stage.vk_shader_stage_info);

	// Use the same descriptor set layout as the main rendering
	// Set 0: Frame data (camera + object transforms)
	std::vector<DescriptorSetLayout::Binding> frame_bindings = {
		{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
		{ 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex }
	};

	picking_set_layout.Init(frame_bindings);

	// Push constant for object ID
	vk::PushConstantRange push_constant_range =
		vk::PushConstantRange().setStageFlags(vk::ShaderStageFlagBits::eFragment).setOffset(0).setSize(sizeof(uint32_t));

	pipeline_layout.Init({ picking_set_layout.vk_descriptor_set_layout }, { push_constant_range });
	pipeline_info.setLayout(pipeline_layout.vk_pipeline_layout);

	// Create descriptor pool
	picking_descriptor_pool.Init(frame_bindings, 1);

	pipeline_info.setStageCount(shader_stage_info.size())
		.setPStages(shader_stage_info.data())
		.setRenderPass(render_pass.vk_render_pass)
		.setSubpass(0);

	pipeline = device->vk_device.createGraphicsPipeline(nullptr, pipeline_info).value;
}

void ObjectPicker::CreateFramebuffer()
{
	std::vector<vk::ImageView> attachments = { color_attachment.GetImageView(), depth_attachment.GetImageView() };

	vk::FramebufferCreateInfo framebuffer_info = vk::FramebufferCreateInfo()
													 .setRenderPass(render_pass.vk_render_pass)
													 .setAttachmentCount(attachments.size())
													 .setPAttachments(attachments.data())
													 .setWidth(extent.width)
													 .setHeight(extent.height)
													 .setLayers(1);

	framebuffer = device->vk_device.createFramebuffer(framebuffer_info);
}

uint32_t ObjectPicker::PickObject(const std::vector<ObjectData>& objects,
								  const GeometryBatcher*		 geometry_batcher,
								  const glm::mat4&				 view_matrix,
								  const glm::mat4&				 proj_matrix,
								  int							 mouse_x,
								  int							 mouse_y)
{
	// Record picking commands
	RecordPickingCommands(objects, geometry_batcher, view_matrix, proj_matrix);

	// Submit command buffer
	vk::SubmitInfo submit_info = vk::SubmitInfo().setCommandBufferCount(1).setPCommandBuffers(&command_buffer);

	device->vk_graphics_queue.submit(submit_info, nullptr);
	device->vk_device.waitIdle();	 // Wait for completion

	// Copy pixel data from the color attachment to readback buffer
	vk::CommandBuffer copy_cmd = device->vk_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
																			  .setCommandPool(vk_command_pool)
																			  .setLevel(vk::CommandBufferLevel::ePrimary)
																			  .setCommandBufferCount(1))[0];

	copy_cmd.begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

	// Copy single pixel at mouse position
	vk::BufferImageCopy copy_region = vk::BufferImageCopy()
										  .setBufferOffset(0)
										  .setBufferRowLength(0)
										  .setBufferImageHeight(0)
										  .setImageSubresource(vk::ImageSubresourceLayers()
																   .setAspectMask(vk::ImageAspectFlagBits::eColor)
																   .setMipLevel(0)
																   .setBaseArrayLayer(0)
																   .setLayerCount(1))
										  .setImageOffset({ mouse_x, mouse_y, 0 })
										  .setImageExtent({ 1, 1, 1 });

	copy_cmd.copyImageToBuffer(
		color_attachment.vk_image, vk::ImageLayout::eTransferSrcOptimal, readback_buffer->vk_buffer, 1, &copy_region);

	copy_cmd.end();

	vk::SubmitInfo copy_submit = vk::SubmitInfo().setCommandBufferCount(1).setPCommandBuffers(&copy_cmd);

	device->vk_graphics_queue.submit(copy_submit, nullptr);
	device->vk_device.waitIdle();

	// Read the object ID from the buffer
	uint32_t* pixel_data = static_cast<uint32_t*>(
		device->vk_device.mapMemory(readback_buffer->vk_memory, 0, readback_buffer->vk_memory_info.allocationSize));
	uint32_t object_id = pixel_data[0];	   // R component contains object ID
	device->vk_device.unmapMemory(readback_buffer->vk_memory);

	return object_id;
}

void ObjectPicker::RecordPickingCommands(const std::vector<ObjectData>& objects,
										 const GeometryBatcher*			geometry_batcher,
										 const glm::mat4&				view_matrix,
										 const glm::mat4&				proj_matrix)
{
	command_buffer.reset(vk::CommandBufferResetFlags());
	command_buffer.begin(vk::CommandBufferBeginInfo());

	// Clear values
	std::vector<vk::ClearValue> clear_values = { vk::ClearValue().setColor(
													 vk::ClearColorValue(std::array<uint32_t, 4> { 0, 0, 0, 0 })),
												 vk::ClearValue().setDepthStencil(vk::ClearDepthStencilValue(1.0f, 0)) };

	vk::RenderPassBeginInfo render_pass_begin = vk::RenderPassBeginInfo()
													.setRenderPass(render_pass.vk_render_pass)
													.setFramebuffer(framebuffer)
													.setRenderArea(vk::Rect2D().setOffset({ 0, 0 }).setExtent(extent))
													.setClearValueCount(clear_values.size())
													.setPClearValues(clear_values.data());

	command_buffer.beginRenderPass(render_pass_begin, vk::SubpassContents::eInline);

	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	// Bind vertex buffer
	vk::Buffer	 vertex_buffers[] = { geometry_batcher->vertex_buffer->vk_buffer };
	VkDeviceSize offsets[]		  = { 0 };
	command_buffer.bindVertexBuffers(0, 1, vertex_buffers, offsets);
	command_buffer.bindIndexBuffer(geometry_batcher->index_buffer->vk_buffer, 0, vk::IndexType::eUint32);

	// TODO: Bind descriptor set with camera data (you'll need to create this)

	// Render each object with its unique ID
	for (uint32_t i = 0; i < objects.size(); ++i)
	{
		const auto& object	  = objects[i];
		uint32_t	object_id = i + 1;	  // Object IDs start from 1 (0 = background)

		// Push object ID as push constant
		command_buffer.pushConstants(
			pipeline_layout.vk_pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(uint32_t), &object_id);

		// Find mesh data for this object
		auto mesh_it = geometry_batcher->mesh_data.find(object.mesh);
		if (mesh_it != geometry_batcher->mesh_data.end())
		{
			const auto& mesh_data	= mesh_it->second;
			uint32_t	index_count = static_cast<uint32_t>(mesh_data.index_size);
			uint32_t	first_index = static_cast<uint32_t>(mesh_data.index_offset);

			command_buffer.drawIndexed(index_count, 1, first_index, 0, i);
		}
	}

	command_buffer.endRenderPass();
	command_buffer.end();
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
		if (framebuffer)
		{
			device->vk_device.destroyFramebuffer(framebuffer);
			framebuffer = VK_NULL_HANDLE;
		}
		if (pipeline)
		{
			device->vk_device.destroyPipeline(pipeline);
			pipeline = VK_NULL_HANDLE;
		}
		render_pass.Cleanup();
		pipeline_layout.Cleanup();
		picking_set_layout.Cleanup();
		picking_descriptor_pool.Cleanup();

		for (auto& shader_stage : picking_shader_stages)
			if (shader_stage.shader)
				shader_stage.shader.reset();
		picking_shader_stages.clear();

		if (readback_buffer)
		{
			device->buffer_manager->DestroyBuffer(readback_buffer);
			readback_buffer = nullptr;
		}
	}
}

// Add to Surface class methods:

uint32_t Surface::PickObjectAtPosition(int mouse_x, int mouse_y)
{
	if (!object_picker || !scene)
		return 0;

	Frame& current_frame = frames[frame_index];
	return object_picker->PickObject(scene->objects,
									 scene->geometry_batcher.get(),
									 current_frame.camera_data.view,
									 current_frame.camera_data.proj,
									 mouse_x,
									 mouse_y);
}

}	 // namespace nft::vulkan