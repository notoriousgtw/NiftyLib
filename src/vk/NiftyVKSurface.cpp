//=============================================================================
// VULKAN SURFACE IMPLEMENTATION
//=============================================================================
// This file implements the Vulkan surface management, including swapchain
// creation, pipeline setup, and all related rendering resources.

#include "vk/NiftyVKSurface.h"
#include "GLFW/glfw3.h"
#include "core/NiftyApp.h"
#include "core/NiftyError.h"
#include "vk/NiftyVK.h"

#include <../generated/simple_shader.frag.spv.h>
#include <../generated/simple_shader.vert.spv.h>
#include <vk/NiftyVKRender.h>

namespace nft::vulkan
{

//=============================================================================
// CONSTRUCTOR & DESTRUCTOR
//=============================================================================

Surface::Surface(Instance* instance, Device* device, GLFWwindow* window):
	instance(instance),
	device(device),
	window(window),
	vk_surface(VK_NULL_HANDLE),
	vk_swapchain(VK_NULL_HANDLE),
	vk_pipeline(VK_NULL_HANDLE),
	vk_command_pool(VK_NULL_HANDLE),
	vk_command_buffer(VK_NULL_HANDLE),
	vertex_input_stage(device, instance),
	input_assembly_stage(device, instance),
	viewport_stage(device, instance),
	rasterization_stage(device, instance),
	multisample_stage(device, instance),
	color_blend_stage(device, instance),
	scene_set_layout(device, instance),
	mesh_set_layout(device, instance),
	descriptor_pool(device, instance),
	pipeline_layout(device, instance),
	render_pass(device, instance),
	clear_color(vk::ClearColorValue(std::array<float, 4> { 0.2f, 0.2f, 0.2f, 1.0f })),
	is_cleaned_up(false)
{
	// Validate input parameters
	if (!instance)
		NFT_ERROR(VKFatal, "Instance is null!");
	if (!device)
		NFT_ERROR(VKFatal, "Device is null!");
	if (!window)
		NFT_ERROR(VKFatal, "Window is null!");

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
		instance->vk_instance.destroySurfaceKHR(vk_surface, nullptr, instance->dispatch_loader_dynamic);
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
	app->GetLogger()->Debug(std::format("Creating Surface For Window: \"{}\"...", glfwGetWindowTitle(window)), "VKInit");

	// Create a Vulkan surface using GLFW
	VkSurfaceKHR c_style_surface;
	glfwCreateWindowSurface(instance->vk_instance, window, nullptr, &c_style_surface);
	vk_surface = c_style_surface;

	app->GetLogger()->Debug(std::format("Surface For Window: \"{}\" Created Successfully!", glfwGetWindowTitle(window)),
							"VKInit");
	CreateCommandPool();
	scene = std::make_unique<Scene>(device, vk_command_buffer);
	InitSwapchain();
	CreatePipeline();
	CreateFrameBuffers();
	CreateFrameCommandBuffers();
}

void Surface::InitSwapchain()
{
	// app->GetLogger()->Debug(std::format("Creating Swapchain For Window: \"{}\"...", glfwGetWindowTitle(window)), "VKInit");
	// app->GetLogger()->Debug("Querying Swapchain Support...", "VKInit");

	// Query swapchain support details
	support_details.capabilities =
		device->vk_physical_device.getSurfaceCapabilitiesKHR(vk_surface, instance->dispatch_loader_dynamic);
	support_details.formats = device->vk_physical_device.getSurfaceFormatsKHR(vk_surface, instance->dispatch_loader_dynamic);
	support_details.present_modes =
		device->vk_physical_device.getSurfacePresentModesKHR(vk_surface, instance->dispatch_loader_dynamic);

	LogSupportDetails();
	CreateSwapchain();

	// Create synchronization objects using Device wrapper methods
	// app->GetLogger()->Debug("Creating Synchronization Objects...", "VKInit");
	for (auto& frame : frames)
	{
		frame.in_flight_fence			= device->CreateFence(true);
		frame.image_available_semaphore = device->CreateSemaphore();
		frame.render_finished_semaphore = device->CreateSemaphore();
		frame.Init(this, scene.get());
		frame.MakeDescriptorResources();
	}
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
		vk_swapchain = device->vk_device.createSwapchainKHR(vk_swapchain_info, nullptr, instance->dispatch_loader_dynamic);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Create Swapchain:\n{}", err.what()));
	}

	// Get swapchain images and create image views
	std::vector<vk::Image> image_vec = device->vk_device.getSwapchainImagesKHR(vk_swapchain, instance->dispatch_loader_dynamic);
	this->frames.resize(image_vec.size());
	max_frames_in_flight = image_vec.size();

	for (size_t i = 0; i < image_vec.size(); i++)
	{
		auto& frame				 = frames.at(i);
		frame.image				 = image_vec.at(i);
		frame.vk_image_view_info = vk::ImageViewCreateInfo()
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
			frame.vk_image_view =
				device->vk_device.createImageView(frame.vk_image_view_info, nullptr, instance->dispatch_loader_dynamic);
		}
		catch (const vk::SystemError& err)
		{
			NFT_ERROR(VKFatal, std::format("Failed To Create Image View:\n{}", err.what()));
		}
	}

	app->GetLogger()->Debug(std::format("Swapchain For Window: \"{}\" Created Successfully!", glfwGetWindowTitle(window)),
							"VKInit");
}

void Surface::RecreateSwapchain()
{
	// app->GetLogger()->Debug(std::format("Recreating Swapchain For Window: \"{}\"...", glfwGetWindowTitle(window)), "VKInit");
	//  Wait for device to be idle before recreating swapchain
	int width  = 0;
	int height = 0;

	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	device->vk_device.waitIdle(instance->dispatch_loader_dynamic);
	// Cleanup old swapchain resources
	CleanupSwapchain();
	// Recreate swapchain and related resources
	InitSwapchain();
	CreateFrameBuffers();
	CreateFrameCommandBuffers();
	// app->GetLogger()->Debug(std::format("Swapchain For Window: \"{}\" Recreated Successfully!", glfwGetWindowTitle(window)),
	//						"VKInit");
}

//=============================================================================
// PIPELINE CREATION METHODS
//=============================================================================

void Surface::CreatePipeline()
{
	app->GetLogger()->Debug("Creating Pipeline...", "VKInit");

	// Initialize pipeline stages for better performance with inline initialization
	vertex_input_stage.Init();
	input_assembly_stage.Init();

	// Initialize shader stages
	shader_stages.clear();
	shader_stages.reserve(2);	 // Performance: reserve space for vertex and fragment shaders

	// Add vertex shader
	shader_stages.emplace_back(device, instance);
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
	shader_stages.emplace_back(device, instance);
	shader_stages.back().shader =
		std::make_unique<Shader>(device, Shader::ShaderCode { (uint32_t*)simple_shader_frag, simple_shader_frag_len });
	shader_stages.back().vk_shader_stage_info = vk::PipelineShaderStageCreateInfo()
													.setFlags(vk::PipelineShaderStageCreateFlags())
													.setStage(vk::ShaderStageFlagBits::eFragment)
													.setModule(shader_stages.back().shader->GetShaderModule())
													.setPName("main");

	multisample_stage.Init();
	color_blend_stage.Init();

	std::vector<DescriptorSetLayout::Binding> bindings = {
		{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
		{ 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex }
	};

	scene_set_layout.Init(bindings);

	descriptor_pool.Init(bindings, frames.size());

	for (auto& frame : frames)
		frame.AllocateDescriptorResources();

	pipeline_layout.Init(scene_set_layout.vk_descriptor_set_layout);
	render_pass.Init(format.format);

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
						   .setPMultisampleState(&multisample_stage.vk_multisample_info)
						   .setPColorBlendState(&color_blend_stage.vk_color_blend_info)
						   .setLayout(pipeline_layout.vk_pipeline_layout)
						   .setRenderPass(render_pass.vk_render_pass)
						   .setSubpass(0)
						   .setBasePipelineHandle(nullptr);

	try
	{
		vk_pipeline =
			device->vk_device.createGraphicsPipeline(nullptr, vk_pipeline_info, nullptr, instance->dispatch_loader_dynamic).value;
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Create Graphics Pipeline:\n{}", err.what()));
	}

	app->GetLogger()->Debug("Pipeline Created Successfully!", "VKInit");
}

//=============================================================================
// FRAMEBUFFER AND COMMAND CREATION METHODS
//=============================================================================

void Surface::CreateFrameBuffers()
{
	size_t i = 0;
	for (auto& frame : frames)
	{
		std::vector<vk::ImageView> attachments = { frame.vk_image_view };

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
			frame.vk_frame_buffer =
				device->vk_device.createFramebuffer(frame.vk_frame_buffer_info, nullptr, instance->dispatch_loader_dynamic);
		}
		catch (const vk::SystemError& err)
		{
			NFT_ERROR(VKFatal, std::format("Failed To Create Framebuffer {}:\n{}", i, err.what()));
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
		vk_command_pool = device->vk_device.createCommandPool(vk_command_pool_info, nullptr, instance->dispatch_loader_dynamic);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Create Command Pool:\n{}", err.what()));
	}

	// Allocate main command buffer
	try
	{
		vk_command_buffer = device->vk_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
																		 .setCommandPool(vk_command_pool)
																		 .setLevel(vk::CommandBufferLevel::ePrimary)
																		 .setCommandBufferCount(1),
																	 instance->dispatch_loader_dynamic)[0];
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Allocate Main Command Buffer:\n{}", err.what()));
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
																			   .setCommandBufferCount(1),
																		   instance->dispatch_loader_dynamic)[0];
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
		NFT_ERROR(VKFatal, "Scene is not set for rendering!");
		return;
	}

	vk::Buffer	 vertex_buffers[] = { scene->GetGeometryBatcher()->GetVertexBuffer()->vk_buffer };
	VkDeviceSize offsets[]		  = { 0 };	  // Start from the beginning of the buffer
	command_buffer.bindVertexBuffers(0, 1, vertex_buffers, offsets, instance->dispatch_loader_dynamic);
}

void Surface::Render()
{
	device->vk_device.waitForFences(frames[frame_index].in_flight_fence, VK_TRUE, UINT64_MAX, instance->dispatch_loader_dynamic);

	uint32_t image_index;
	try
	{
		vk::ResultValue aquire =
			device->vk_device.acquireNextImage2KHR(vk::AcquireNextImageInfoKHR()
													   .setSwapchain(vk_swapchain)
													   .setTimeout(UINT64_MAX)
													   .setSemaphore(frames[frame_index].image_available_semaphore)
													   .setFence(nullptr)
													   .setDeviceMask(1),
												   instance->dispatch_loader_dynamic);
		image_index = aquire.value;
	}
	catch (vk::OutOfDateKHRError)
	{
		RecreateSwapchain();
		return;
	}

	device->vk_device.resetFences(frames[frame_index].in_flight_fence, instance->dispatch_loader_dynamic);

	vk::CommandBuffer command_buffer = frames[frame_index].vk_command_buffer;
	command_buffer.reset(vk::CommandBufferResetFlags(), instance->dispatch_loader_dynamic);

	frames[frame_index].Prepare();
	RecordDrawCommands(frames[frame_index], image_index);

	vk::PipelineStageFlags wait_stages = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::SubmitInfo submit_info = vk::SubmitInfo()
									 .setWaitSemaphoreCount(1)
									 .setPWaitSemaphores(&frames[frame_index].image_available_semaphore)
									 .setPWaitDstStageMask(&wait_stages)
									 .setCommandBufferCount(1)
									 .setPCommandBuffers(&command_buffer)
									 .setSignalSemaphoreCount(1)
									 .setPSignalSemaphores(&frames[frame_index].render_finished_semaphore);

	try
	{
		device->vk_graphics_queue.submit(submit_info, frames[frame_index].in_flight_fence, instance->dispatch_loader_dynamic);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Submit Draw Command Buffer:\n{}", err.what()));
	}

	vk::PresentInfoKHR present_info = vk::PresentInfoKHR()
										  .setWaitSemaphoreCount(1)
										  .setPWaitSemaphores(&frames[frame_index].render_finished_semaphore)
										  .setSwapchainCount(1)
										  .setPSwapchains(&vk_swapchain)
										  .setPImageIndices(&image_index);

	vk::Result present_result;
	try
	{
		present_result = device->GetPresentQueue().presentKHR(present_info, instance->dispatch_loader_dynamic);
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
	//app->GetLogger()->Debug(std::format("Using orthographic projection with bounds: left={}, right={}, bottom={}, top={}",
	//									-2.0f * aspect,
	//									2.0f * aspect,
	//									-2.0f,
	//									2.0f),
	//						"VKRender");

	// Record commands for each frame
	auto					   command_buffer = frame.vk_command_buffer;
	vk::CommandBufferBeginInfo begin_info	  = vk::CommandBufferBeginInfo();
	command_buffer.begin(begin_info, instance->dispatch_loader_dynamic);

	vk::RenderPassBeginInfo render_pass_begin_info = vk::RenderPassBeginInfo()
														 .setRenderPass(render_pass.vk_render_pass)
														 .setFramebuffer(frames[image_index].vk_frame_buffer)
														 .setRenderArea(vk::Rect2D().setOffset({ 0, 0 }).setExtent(extent))
														 .setClearValueCount(1)
														 .setPClearValues(&clear_color);
	command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline, instance->dispatch_loader_dynamic);

	command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
									  pipeline_layout.vk_pipeline_layout,
									  0,
									  { frame.vk_descriptor_set },
									  nullptr,
									  instance->dispatch_loader_dynamic);

	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vk_pipeline, instance->dispatch_loader_dynamic);

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

		//app->GetLogger()->Debug(
		//	std::format("Mesh vertex data: count={}, offset={}, data_size={}", vertex_count, first_vertex, mesh->vertices.size()),
		//	"VKRender");


		// Draw each object separately by updating instance ID manually
		for (uint32_t instance_id = 0; instance_id < scene->objects.size(); ++instance_id)
		{
			if (scene->objects[instance_id].mesh == mesh)
			{
				//app->GetLogger()->Debug(std::format("Drawing object {}: vertex_count={}, first_vertex={}, instance_id={}",
				//									instance_id,
				//									vertex_count,
				//									first_vertex,
				//									instance_id),
				//						"VKRender");

				// Draw single instance
				command_buffer.draw(vertex_count, 1, first_vertex, instance_id, instance->dispatch_loader_dynamic);
			}
		}
	}

	//// Draw each mesh with proper instancing
	//const auto& meshes = scene->geometry_batcher->mesh_data;
	//uint32_t first_instance = 0;
	//
	//for (const auto& mesh_entry : meshes)
	//{
	//	const IMesh* mesh = mesh_entry.first;
	//	const GeometryBatcher::MeshData& mesh_data = mesh_entry.second;
	//	
	//	// Count how many objects use this mesh
	//	uint32_t instance_count = 0;
	//	for (const auto& object_data : scene->objects)
	//	{
	//		if (object_data.mesh == mesh)
	//			instance_count++;
	//	}

	//	if (instance_count > 0)
	//	{
	//		uint32_t vertex_count = static_cast<uint32_t>(mesh_data.size);
	//		uint32_t first_vertex = static_cast<uint32_t>(mesh_data.offset);

	//		app->GetLogger()->Debug(
	//			std::format("Drawing mesh: vertex_count={}, instance_count={}, first_vertex={}, first_instance={}",
	//						vertex_count,
	//						instance_count,
	//						first_vertex,
	//						first_instance),
	//			"VKRender");


	//		// Draw all instances of this mesh type at once
	//		command_buffer.draw(vertex_count, instance_count, first_vertex, first_instance, instance->dispatch_loader_dynamic);
	//		first_instance += instance_count;
	//	}
	//}

	command_buffer.endRenderPass(instance->dispatch_loader_dynamic);
	command_buffer.end(instance->dispatch_loader_dynamic);
}

//=============================================================================
// CLEANUP METHODS
//=============================================================================

void Surface::CleanupSwapchain()
{
	// Cleanup swapchain resources
	if (vk_swapchain && device && instance)
	{
		device->vk_device.destroySwapchainKHR(vk_swapchain, nullptr, instance->dispatch_loader_dynamic);
		vk_swapchain = VK_NULL_HANDLE;
		if (app && app->GetLogger())
			app->GetLogger()->Debug("Swapchain destroyed successfully", "VKShutdown");
	}
	// Cleanup frame resources
	for (auto& frame : frames)
	{
		if (frame.vk_image_view)
			device->vk_device.destroyImageView(frame.vk_image_view, nullptr, instance->dispatch_loader_dynamic);
		if (frame.vk_frame_buffer)
			device->vk_device.destroyFramebuffer(frame.vk_frame_buffer, nullptr, instance->dispatch_loader_dynamic);
		if (frame.in_flight_fence)
			device->vk_device.destroyFence(frame.in_flight_fence, nullptr, instance->dispatch_loader_dynamic);
		if (frame.image_available_semaphore)
			device->vk_device.destroySemaphore(frame.image_available_semaphore, nullptr, instance->dispatch_loader_dynamic);
		if (frame.render_finished_semaphore)
			device->vk_device.destroySemaphore(frame.render_finished_semaphore, nullptr, instance->dispatch_loader_dynamic);
		if (frame.camera_data_buffer)
		{
			device->vk_device.unmapMemory(frame.camera_data_buffer->vk_memory, instance->dispatch_loader_dynamic);
			device->buffer_manager->DestroyBuffer(frame.camera_data_buffer);
		}
		if (frame.object_transform_buffer)
		{
			device->vk_device.unmapMemory(frame.object_transform_buffer->vk_memory, instance->dispatch_loader_dynamic);
			device->buffer_manager->DestroyBuffer(frame.object_transform_buffer);
		}
		frame.vk_image_view = VK_NULL_HANDLE;
	}
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
		device->vk_device.waitIdle(instance->dispatch_loader_dynamic);

		app->GetLogger()->Debug("Cleaning up Surface Vulkan objects...", "VKShutdown");

		if (vk_command_pool)
		{
			device->vk_device.destroyCommandPool(vk_command_pool, nullptr, instance->dispatch_loader_dynamic);
			vk_command_pool = VK_NULL_HANDLE;
		}

		if (vk_pipeline)
		{
			device->vk_device.destroyPipeline(vk_pipeline, nullptr, instance->dispatch_loader_dynamic);
			vk_pipeline = VK_NULL_HANDLE;
		}

		for (auto& shader_stage : shader_stages)
			if (shader_stage.shader)
				shader_stage.shader.reset();	// This calls Shader destructor which destroys the shader module
		shader_stages.clear();

		render_pass.Cleanup();
		pipeline_layout.Cleanup();

		CleanupSwapchain();

		scene_set_layout.Cleanup();

		app->GetLogger()->Debug("Surface Vulkan objects cleaned up successfully", "VKShutdown");
	}

	is_cleaned_up = true;
}
//=============================================================================
// UTILITY AND SELECTION METHODS
//=============================================================================

void Surface::SelectFormat()
{
	NFT_ERROR(VKFatal, "Requested Format Not Supported!");
}

void Surface::SelectPresentMode()
{
	NFT_ERROR(VKFatal, "Requested Present Mode Not Supported!");
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
		NFT_ERROR(VKFatal, "Surface pointer is null!");
	if (!scene)
		NFT_ERROR(VKFatal, "Scene pointer is null!");
	this->surface = surface;
	this->device  = surface->device;
	this->scene	  = scene;
}

void Surface::Frame::MakeDescriptorResources()
{
	if (!device)
		NFT_ERROR(VKFatal, "Device pointer is null!");
	camera_data_buffer = device->buffer_manager->CreateBuffer(sizeof(UniformBufferObject),
															  vk::BufferUsageFlagBits::eUniformBuffer,
															  vk::MemoryPropertyFlagBits::eHostVisible |
																  vk::MemoryPropertyFlagBits::eHostCoherent);
	camera_data_ptr	   = device->vk_device.mapMemory(camera_data_buffer->vk_memory,
													 0,
													 camera_data_buffer->vk_memory_info.allocationSize,
													 vk::MemoryMapFlags(),
													 device->GetInstance()->GetDispatchLoader());

	size_t buffer_size = sizeof(glm::mat4) * scene->objects.size();
	size_t aligned_size = ((buffer_size + 15) / 16) * 16;	// Align to 256 bytes

	object_transform_buffer = device->buffer_manager->CreateBuffer(buffer_size,
																   vk::BufferUsageFlagBits::eStorageBuffer,
																   vk::MemoryPropertyFlagBits::eHostVisible |
																	   vk::MemoryPropertyFlagBits::eHostCoherent);
	object_transform_ptr	= device->vk_device.mapMemory(object_transform_buffer->vk_memory,
														  0,
														  object_transform_buffer->vk_memory_info.allocationSize,
														  vk::MemoryMapFlags(),
														  device->GetInstance()->GetDispatchLoader());

	// CRITICAL FIX: Clear the GPU memory to zero
	std::memset(object_transform_ptr, 0, object_transform_buffer->vk_memory_info.allocationSize);

	//surface->app->GetLogger()->Debug(std::format("Storage buffer: requested_size={}, aligned_size={}, actual_size={}",
	//											 buffer_size,
	//											 aligned_size,
	//											 object_transform_buffer->vk_memory_info.allocationSize),
	//								 "VKRender");

	object_transforms.resize(scene->objects.size());
	for (int i = 0; i < scene->objects.size(); ++i)
		object_transforms[i] = glm::mat4(1.0f);	   // Initialize with identity matrices
}

void Surface::Frame::AllocateDescriptorResources()
{
	if (!surface)
		NFT_ERROR(VKFatal, "Surface pointer is null!");
	if (!device)
		NFT_ERROR(VKFatal, "Device pointer is null!");
	if (!camera_data_buffer)
	{
		NFT_ERROR(VKFatal, "Camera data buffer is not initialized!");
		return;
	}
	vk::DescriptorSetAllocateInfo alloc_info = vk::DescriptorSetAllocateInfo()
												   .setDescriptorPool(surface->descriptor_pool.vk_descriptor_pool)
												   .setDescriptorSetCount(1)
												   .setPSetLayouts(&surface->scene_set_layout.vk_descriptor_set_layout);
	try
	{
		vk_descriptor_set = device->vk_device.allocateDescriptorSets(alloc_info, surface->instance->dispatch_loader_dynamic)[0];
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Allocate Descriptor Set:\n{}", err.what()));
	}
}

void Surface::Frame::Prepare()
{
	if (!surface)
		NFT_ERROR(VKFatal, "Surface pointer is null!");
	if (!scene)
		NFT_ERROR(VKFatal, "Scene pointer is null!");
	if (!camera_data_buffer)
		NFT_ERROR(VKFatal, "Camera data buffer is not initialized!");



	glm::vec3 eye	 = { 1.0f, 0.0f, 2.0f };
	glm::vec3 center = { 0.0f, 0.0f, 0.0f };
	glm::vec3 up	 = { 0.0f, 1.0f, 0.0f };
	camera_data.view = glm::lookAt(eye, center, up);

	camera_data.proj = glm::perspective(glm::radians(45.0f),
										static_cast<float>(surface->vk_swapchain_info.imageExtent.width) /
											static_cast<float>(surface->vk_swapchain_info.imageExtent.height),
										0.1f,
										10.0f);

	//camera_data.view = glm::mat4(1.0f);	   // Identity matrix for now
	float aspect = static_cast<float>(surface->vk_swapchain_info.imageExtent.width) /
				   static_cast<float>(surface->vk_swapchain_info.imageExtent.height);
	//camera_data.proj = glm::ortho(-2.0f * aspect, 2.0f * aspect, -2.0f, 2.0f, -10.0f, 10.0f);
	camera_data.proj[1][1] *= -1;
	camera_data.view_proj = camera_data.proj * camera_data.view;

	//surface->app->GetLogger()->Debug(
	//	std::format("Camera bounds: left={}, right={}, bottom={}, top={}", -2.0f * aspect, 2.0f * aspect, -2.0f, 2.0f),
	//	"VKRender");

	std::memcpy(camera_data_ptr, &camera_data, sizeof(UniformBufferObject));

	// Gather object transforms
	const size_t object_count = scene->objects.size();
	for (size_t idx = 0; idx < object_count; ++idx)
	{
		object_transforms[idx] = scene->objects[idx].transform;

		const auto& m = scene->objects[idx].transform;
		//surface->app->GetLogger()->Debug(std::format("Matrix {} row 0: [{:.3f},{:.3f},{:.3f},{:.3f}]",
		//											 idx, m[0][0], m[0][1], m[0][2], m[0][3]),
		//								 "VKRender");
		//surface->app->GetLogger()->Debug(std::format("Matrix {} row 1: [{:.3f},{:.3f},{:.3f},{:.3f}]",
		//											 idx, m[1][0], m[1][1], m[1][2], m[1][3]),
		//								 "VKRender");
		//surface->app->GetLogger()->Debug(std::format("Matrix {} row 2: [{:.3f},{:.3f},{:.3f},{:.3f}]",
		//											 idx, m[2][0], m[2][1], m[2][2], m[2][3]),
		//								 "VKRender");
		//surface->app->GetLogger()->Debug(std::format("Matrix {} row 3: [{:.3f},{:.3f},{:.3f},{:.3f}]",
		//											 idx, m[3][0], m[3][1], m[3][2], m[3][3]),
		//								 "VKRender");
	}

	const size_t bytes = object_count * sizeof(glm::mat4);
	//surface->app->GetLogger()->Debug(std::format("Copying {} matrices ({} bytes) to storage buffer",
	//											 object_count, bytes),
	//								 "VKRender");

	// Single contiguous copy (glm::mat4 already column-major; matches GLSL default)
	std::memcpy(object_transform_ptr, object_transforms.data(), bytes);

	// Verify first matrix
	const float* gpu_floats = static_cast<const float*>(object_transform_ptr);
	//surface->app->GetLogger()->Debug("First matrix in GPU buffer (post-copy):", "VKRender");
	//for (int row = 0; row < 4; ++row)
	//{
	//	surface->app->GetLogger()->Debug(std::format("  Row {}: [{:.3f},{:.3f},{:.3f},{:.3f}]",
	//												 row,
	//												 gpu_floats[row * 4 + 0],
	//												 gpu_floats[row * 4 + 1],
	//												 gpu_floats[row * 4 + 2],
	//												 gpu_floats[row * 4 + 3]),
	//									 "VKRender");
	//}

	std::vector<vk::DescriptorBufferInfo> buffer_infos;
	buffer_infos.push_back(
		vk::DescriptorBufferInfo()
			.setBuffer(camera_data_buffer->vk_buffer)
			.setOffset(0)
			.setRange(sizeof(UniformBufferObject)));
	buffer_infos.push_back(
		vk::DescriptorBufferInfo()
			.setBuffer(object_transform_buffer->vk_buffer)
			.setOffset(0)
			.setRange(bytes));

	//surface->app->GetLogger()->Debug(std::format("Storage buffer descriptor: buffer={}, offset=0, range={}",
	//											 (void*)object_transform_buffer->vk_buffer, bytes),
	//								 "VKRender");

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
	device->vk_device.updateDescriptorSets(
		descriptor_writes.size(), descriptor_writes.data(), 0, nullptr, surface->instance->dispatch_loader_dynamic);
}

}	 // namespace nft::vulkan