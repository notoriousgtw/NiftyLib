#include "vk/pipeline.h"

#include "vk/geometry.h"	// For MaterialPushConstants
#include "vk/handler.h"
#include "vk/surface.h"	   // Forward declaration resolution
#include "vk/descriptors.h" // For descriptor helper functions
#include "vk/resources.h"   // For GlobalBindlessManager and related types
#include "vk/image.h"

namespace nft::vulkan
{

//=============================================================================
// PIPELINE STAGE IMPLEMENTATIONS
//=============================================================================

void VertexInputStage::Init()
{
	// Use traditional vertex input for bound vertex buffers
	binding_description = GetVertexInputBindingDescription();
	attribute_descriptions = GetVertexInputAttributeDescriptions();
	
	vk_vertex_input_info = vk::PipelineVertexInputStateCreateInfo()
							   .setFlags(vk::PipelineVertexInputStateCreateFlags())
							   .setVertexBindingDescriptionCount(1)
							   .setPVertexBindingDescriptions(&binding_description)
							   .setVertexAttributeDescriptionCount(static_cast<uint32_t>(attribute_descriptions.size()))
							   .setPVertexAttributeDescriptions(attribute_descriptions.data());
}

void InputAssemblyStage::Init(vk::PrimitiveTopology topology, vk::Bool32 restart_enable)
{
	vk_input_assembly_info = vk::PipelineInputAssemblyStateCreateInfo()
								 .setFlags(vk::PipelineInputAssemblyStateCreateFlags())
								 .setTopology(topology)
								 .setPrimitiveRestartEnable(restart_enable);
}

void ViewportStage::Init(vk::Extent2D extent, glm::vec2 pos, glm::vec2 depth, vk::Offset2D offset)
{
	viewport.setX(pos.x)
		.setY(pos.y)
		.setWidth(static_cast<float>(extent.width))
		.setHeight(static_cast<float>(extent.height))
		.setMinDepth(depth.x)
		.setMaxDepth(depth.y);
	scissor.setOffset(offset).setExtent(extent);
	vk_viewport_state_info = vk::PipelineViewportStateCreateInfo()
								 .setFlags(vk::PipelineViewportStateCreateFlags())
								 .setViewportCount(1)
								 .setPViewports(&viewport)
								 .setScissorCount(1)
								 .setPScissors(&scissor);
}

void RasterizationStage::Init()
{
	vk_rasterization_info = vk::PipelineRasterizationStateCreateInfo()
								.setFlags(vk::PipelineRasterizationStateCreateFlags())
								.setDepthClampEnable(VK_FALSE)
								.setRasterizerDiscardEnable(VK_FALSE)
								.setPolygonMode(vk::PolygonMode::eFill)
								.setLineWidth(1.0f)
								.setCullMode(vk::CullModeFlagBits::eNone)
								.setFrontFace(vk::FrontFace::eCounterClockwise)
								.setDepthBiasEnable(VK_FALSE);
}

void DepthStencilStage::Init()
{
	vk_depth_stencil_info = vk::PipelineDepthStencilStateCreateInfo()
								.setFlags(vk::PipelineDepthStencilStateCreateFlags())
								.setDepthTestEnable(VK_TRUE)
								.setDepthWriteEnable(VK_TRUE)
								.setDepthCompareOp(vk::CompareOp::eLess)
								.setDepthBoundsTestEnable(VK_FALSE)
								.setMinDepthBounds(0.0f)
								.setMaxDepthBounds(1.0f)
								.setStencilTestEnable(VK_FALSE);
}

void MultisampleStage::Init()
{
	vk_multisample_info = vk::PipelineMultisampleStateCreateInfo()
							  .setFlags(vk::PipelineMultisampleStateCreateFlags())
							  .setRasterizationSamples(vk::SampleCountFlagBits::e1)
							  .setSampleShadingEnable(VK_FALSE)
							  .setMinSampleShading(1.0f)
							  .setPSampleMask(nullptr)
							  .setAlphaToCoverageEnable(VK_FALSE)
							  .setAlphaToOneEnable(VK_FALSE);
}

void ColorBlendStage::Init()
{
	color_blend_attachment = vk::PipelineColorBlendAttachmentState().setBlendEnable(VK_FALSE).setColorWriteMask(
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA);

	vk_color_blend_info = vk::PipelineColorBlendStateCreateInfo()
							  .setFlags(vk::PipelineColorBlendStateCreateFlags())
							  .setLogicOpEnable(VK_FALSE)
							  .setLogicOp(vk::LogicOp::eCopy)
							  .setAttachmentCount(1)
							  .setPAttachments(&color_blend_attachment)
							  .setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });
}

//=============================================================================
// DESCRIPTOR SET LAYOUT IMPLEMENTATIONS
//=============================================================================

DescriptorSetLayout::DescriptorSetLayout(Surface* surface): surface(surface), device(surface->GetDevice())
{
	if (!surface)
		NFT_ERROR(VulkanFatal, "Surface Is Null!");
	if (!device)
		NFT_ERROR(VulkanFatal, "Device Is Null!");
}

DescriptorSetLayout::DescriptorSetLayout(Device* device): surface(nullptr), device(device)
{
	if (!device)
		NFT_ERROR(VulkanFatal, "Device Is Null!");
}

void DescriptorSetLayout::Init(std::vector<Binding> bindings)
{
	std::vector<vk::DescriptorSetLayoutBinding> vk_bindings;
	vk_bindings.reserve(bindings.size());

	for (const auto& binding : bindings)
	{
		vk::DescriptorSetLayoutBinding vk_binding = vk::DescriptorSetLayoutBinding()
														.setBinding(binding.index)
														.setDescriptorType(binding.type)
														.setDescriptorCount(binding.count)
														.setStageFlags(binding.stages)
														.setPImmutableSamplers(nullptr);
		vk_bindings.push_back(vk_binding);
	}

	vk_descriptor_set_layout_info = vk::DescriptorSetLayoutCreateInfo()
										.setFlags(vk::DescriptorSetLayoutCreateFlags())
										.setBindingCount(vk_bindings.size())
										.setPBindings(vk_bindings.data());
	try
	{
		vk_descriptor_set_layout = device->GetDevice().createDescriptorSetLayout(vk_descriptor_set_layout_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Create Descriptor Set Layout:\n{}", err.what()));
	}
}

void DescriptorSetLayout::Cleanup()
{
	if (vk_descriptor_set_layout != VK_NULL_HANDLE && device && device->GetDevice())
	{
		device->GetDevice().destroyDescriptorSetLayout(vk_descriptor_set_layout);
		vk_descriptor_set_layout = VK_NULL_HANDLE;
	}
}

//=============================================================================
// DESCRIPTOR POOL IMPLEMENTATIONS
//=============================================================================

void DescriptorPool::Init(std::vector<Binding> bindings, uint32_t count)
{
	std::vector<vk::DescriptorPoolSize> vk_pool_sizes;
	vk_pool_sizes.reserve(bindings.size());
	for (const auto& binding : bindings)
	{
		uint32_t total = static_cast<uint32_t>(binding.count * count);
		vk_pool_sizes.push_back(vk::DescriptorPoolSize().setType(binding.type).setDescriptorCount(total));
	}

	vk_descriptor_pool_info = vk::DescriptorPoolCreateInfo()
								  .setFlags(vk::DescriptorPoolCreateFlags())
								  .setMaxSets(count)
								  .setPoolSizeCount(vk_pool_sizes.size())
								  .setPPoolSizes(vk_pool_sizes.data());
	try
	{
		vk_descriptor_pool = device->GetDevice().createDescriptorPool(vk_descriptor_pool_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Create Descriptor Pool:\n{}", err.what()));
	}
}

void DescriptorPool::Cleanup()
{
	if (vk_descriptor_pool != VK_NULL_HANDLE && device && device->GetDevice())
	{
		device->GetDevice().destroyDescriptorPool(vk_descriptor_pool);
		vk_descriptor_pool = VK_NULL_HANDLE;
	}
}

//=============================================================================
// PIPELINE LAYOUT IMPLEMENTATIONS
//=============================================================================

void PipelineLayout::Init(std::vector<vk::DescriptorSetLayout> descriptor_set_layouts,
						  std::vector<vk::PushConstantRange>   push_constant_ranges)
{
	vk_pipeline_layout_info = vk::PipelineLayoutCreateInfo()
								  .setFlags(vk::PipelineLayoutCreateFlags())
								  .setSetLayoutCount(descriptor_set_layouts.size())
								  .setPSetLayouts(descriptor_set_layouts.data())
								  .setPushConstantRangeCount(push_constant_ranges.size())
								  .setPPushConstantRanges(push_constant_ranges.data());

	// Create pipeline layout
	try
	{
		vk_pipeline_layout = device->GetDevice().createPipelineLayout(vk_pipeline_layout_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Create Pipeline Layout:\n{}", err.what()));
	}
}

void PipelineLayout::Cleanup()
{
	if (vk_pipeline_layout != VK_NULL_HANDLE && device && device->GetDevice())
	{
		device->GetDevice().destroyPipelineLayout(vk_pipeline_layout);
		vk_pipeline_layout = VK_NULL_HANDLE;
	}
}

//=============================================================================
// RENDER PASS IMPLEMENTATIONS
//=============================================================================

void RenderPass::Init(vk::Format color_format, vk::Format depth_format)
{
	// Initialize attachment descriptions
	color_attachment = vk::AttachmentDescription()
						   .setFlags(vk::AttachmentDescriptionFlags())
						   .setFormat(color_format)
						   .setSamples(vk::SampleCountFlagBits::e1)
						   .setLoadOp(vk::AttachmentLoadOp::eClear)
						   .setStoreOp(vk::AttachmentStoreOp::eStore)
						   .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
						   .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
						   .setInitialLayout(vk::ImageLayout::eUndefined)
						   .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	color_attachment_ref = vk::AttachmentReference().setAttachment(0).setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	depth_attachment = vk::AttachmentDescription()
						   .setFlags(vk::AttachmentDescriptionFlags())
						   .setFormat(depth_format)
						   .setSamples(vk::SampleCountFlagBits::e1)
						   .setLoadOp(vk::AttachmentLoadOp::eClear)
						   .setStoreOp(vk::AttachmentStoreOp::eDontCare)
						   .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
						   .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
						   .setInitialLayout(vk::ImageLayout::eUndefined)
						   .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	depth_attachment_ref = vk::AttachmentReference().setAttachment(1).setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	std::vector<vk::AttachmentDescription> attachments = { color_attachment, depth_attachment };

	vk_subpass = vk::SubpassDescription()
					 .setFlags(vk::SubpassDescriptionFlags())
					 .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
					 .setInputAttachmentCount(0)
					 .setPInputAttachments(nullptr)
					 .setColorAttachmentCount(1)
					 .setPColorAttachments(&color_attachment_ref)
					 .setPResolveAttachments(nullptr)
					 .setPDepthStencilAttachment(&depth_attachment_ref)
					 .setPreserveAttachmentCount(0)
					 .setPPreserveAttachments(nullptr);

	vk_render_pass_info = vk::RenderPassCreateInfo()
							  .setFlags(vk::RenderPassCreateFlags())
							  .setAttachmentCount(attachments.size())
							  .setPAttachments(attachments.data())
							  .setSubpassCount(1)
							  .setPSubpasses(&vk_subpass)
							  .setDependencyCount(0)
							  .setPDependencies(nullptr);

	// Create render pass
	try
	{
		vk_render_pass = device->GetDevice().createRenderPass(vk_render_pass_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Create Render Pass:\n{}", err.what()));
	}
}

void RenderPass::Init(const std::vector<vk::AttachmentDescription>& attachments,
					  const std::vector<vk::SubpassDescription>&	subpasses,
					  const std::vector<vk::SubpassDependency>&		dependencies)
{
	vk_render_pass_info = vk::RenderPassCreateInfo()
							  .setFlags(vk::RenderPassCreateFlags())
							  .setAttachmentCount(attachments.size())
							  .setPAttachments(attachments.data())
							  .setSubpassCount(subpasses.size())
							  .setPSubpasses(subpasses.data())
							  .setDependencyCount(dependencies.size())
							  .setPDependencies(dependencies.data());

	try
	{
		vk_render_pass = device->GetDevice().createRenderPass(vk_render_pass_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Create Render Pass:\n{}", err.what()));
	}
}

void RenderPass::Cleanup()
{
	if (vk_render_pass != VK_NULL_HANDLE && device && device->GetDevice())
	{
		device->GetDevice().destroyRenderPass(vk_render_pass);
		vk_render_pass = VK_NULL_HANDLE;
	}
}

//=============================================================================
// FRAME IMPLEMENTATIONS (moved from Surface)
//=============================================================================

void Frame::Init(Surface* surface, uint32_t frame_idx)
{
	if (!surface)
		NFT_ERROR(VulkanFatal, "Surface pointer is null!");
	this->surface	= surface;
	this->device	= surface->GetDevice();
	this->frame_index = frame_idx;
	swapchain_image = Image(device);
	depth_buffer	= Image(device);
}

void Frame::Prepare(glm::mat4 camera_transforms)
{
	if (!surface)
		NFT_ERROR(VulkanFatal, "Surface pointer is null!");

	glm::vec3 eye					= glm::vec3(camera_transforms[3]);
	glm::vec3 base_center_direction = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::mat3 rotation_matrix		= glm::mat3(camera_transforms);
	glm::vec3 rotated_direction		= rotation_matrix * base_center_direction;
	glm::vec3 center				= eye + rotated_direction;
	glm::vec3 up					= glm::normalize(glm::vec3(camera_transforms[1]));

	camera_data.view = glm::lookAt(eye, center, up);
	camera_data.proj =
		glm::perspective(glm::radians(45.0f),
						 static_cast<float>(surface->GetExtent().width) / static_cast<float>(surface->GetExtent().height),
						 0.1f,
						 100.0f);
	camera_data.proj[1][1] *= -1;
	camera_data.pos = eye;

	// Camera data will be updated in the bindless resource manager
	// No need to copy to per-frame buffers anymore
}

void Frame::Cleanup()
{
	if (device && device->GetDevice())
	{
		// Cleanup framebuffer first
		if (vk_frame_buffer)
		{
			device->GetDevice().destroyFramebuffer(vk_frame_buffer);
			vk_frame_buffer = VK_NULL_HANDLE;
		}
		
		// Clean up synchronization objects
		if (in_flight_fence)
		{
			device->GetDevice().destroyFence(in_flight_fence);
			in_flight_fence = VK_NULL_HANDLE;
		}
		if (image_available_semaphore)
		{
			device->GetDevice().destroySemaphore(image_available_semaphore);
			image_available_semaphore = VK_NULL_HANDLE;
		}
		if (render_finished_semaphore)
		{
			device->GetDevice().destroySemaphore(render_finished_semaphore);
			render_finished_semaphore = VK_NULL_HANDLE;
		}

		// NOTE: Swapchain images are owned by the swapchain and should NOT be destroyed here
		// We need to properly handle the swapchain_image to prevent double-destruction
		// Only clean up the image view, not the underlying VkImage
		if (swapchain_image.GetImageView())
		{
			device->GetDevice().destroyImageView(swapchain_image.GetImageView());
			// Clear the handles in the swapchain_image to prevent double destruction
			swapchain_image.ClearImageViewHandle();
			swapchain_image.ClearImageHandle();
			swapchain_image.ClearMemoryHandle();
		}

		// The depth buffer was created by us, so its destructor will handle cleanup properly
		// But we need to ensure proper cleanup order
		// The depth_buffer Image destructor will be called automatically
	}
}

//=============================================================================
// ABSTRACT PIPELINE IMPLEMENTATIONS
//=============================================================================

AbstractPipeline::AbstractPipeline(Device* device, PipelineType type):
	device(device), pipeline_type(type), pipeline_layout(device)
{
	if (!device)
		NFT_ERROR(VulkanFatal, "Device is null!");
}

AbstractPipeline::~AbstractPipeline()
{
	Cleanup();
}

void AbstractPipeline::Cleanup()
{
	if (device && device->vk_device)
	{
		// CRITICAL: Wait for device to be idle before destroying pipeline
		// This ensures no command buffers are using the pipeline
		device->GetDevice().waitIdle();
		
		if (vk_command_pool)
		{
			device->vk_device.destroyCommandPool(vk_command_pool);
			vk_command_pool = VK_NULL_HANDLE;
		}

		if (vk_pipeline)
		{
			device->GetDevice().destroyPipeline(vk_pipeline);
			vk_pipeline = VK_NULL_HANDLE;
		}

		pipeline_layout.Cleanup();
	}
}

void AbstractPipeline::CreateCommandPool()
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

RenderPipelineBase::RenderPipelineBase(Device* device, PipelineType type):
	AbstractPipeline(device, type),
	vertex_input_stage(device),
	input_assembly_stage(device),
	viewport_stage(device),
	rasterization_stage(device),
	depth_stencil_stage(device),
	multisample_stage(device),
	color_blend_stage(device)
{
	// Constructor body can be empty since member initialization list handles everything
}

void RenderPipelineBase::SetVertexShader(Shader::ShaderCode code)
{
	if (!device)
		NFT_ERROR(VulkanFatal, "Device is null!");

	auto stage = shader_stages.find(ShaderType::Vertex);
	if (stage == shader_stages.end())
		shader_stages[ShaderType::Vertex] = std::make_unique<VertexShaderStage>(device);

	shader_stages[ShaderType::Vertex]->Init(code);
}

void RenderPipelineBase::SetGeometryShader(Shader::ShaderCode code)
{
	if (!device)
		NFT_ERROR(VulkanFatal, "Device is null!");

	auto stage = shader_stages.find(ShaderType::Geometry);
	if (stage == shader_stages.end())
		shader_stages[ShaderType::Geometry] = std::make_unique<GeometryShaderStage>(device);

	shader_stages[ShaderType::Geometry]->Init(code);
}

void RenderPipelineBase::SetFragmentShader(Shader::ShaderCode code)
{
	if (!device)
		NFT_ERROR(VulkanFatal, "Device is null!");

	auto stage = shader_stages.find(ShaderType::Fragment);
	if (stage == shader_stages.end())
		shader_stages[ShaderType::Fragment] = std::make_unique<FragmentShaderStage>(device);

	shader_stages[ShaderType::Fragment]->Init(code);
}

void RenderPipelineBase::SetupDescriptorLayouts()
{
	// Use the global bindless descriptor layout instead of creating our own
	auto* global_layout = vulkan::VulkanHandler::GetBindlessDescriptorLayout();
	if (!global_layout)
	{
		NFT_ERROR(VulkanFatal, "Global bindless descriptor layout is not available!");
		return;
	}
	
	// Use the global descriptor set layout instead of creating a new one
	vk_descriptor_set_layouts.push_back(global_layout->vk_descriptor_set_layout);

	// Push constant range for both vertex and fragment stages for future flexibility
	vk::PushConstantRange push_constant_range = vk::PushConstantRange()
													 .setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
													 .setOffset(0)
													 .setSize(sizeof(uint32_t) * 4); // frame_index + material_index + transform_index + vertex_offset
	std::vector<vk::PushConstantRange> push_constant_ranges = { push_constant_range };
	
	// Create pipeline layout using the global descriptor set layout
	pipeline_layout.Init(vk_descriptor_set_layouts, push_constant_ranges);
}

SwapchainRenderPipeline::SwapchainRenderPipeline(Surface* surface, PipelineType type):
	RenderPipelineBase(surface->GetDevice(), type), surface(surface)
{
	if (!surface)
		NFT_ERROR(VulkanFatal, "Surface is null!");
}

void SwapchainRenderPipeline::Create(const RenderPass& render_pass)
{
	// Setup descriptor layouts first
	SetupDescriptorLayouts();
	
	vertex_input_stage.Init();
	input_assembly_stage.Init(vk::PrimitiveTopology::eTriangleList, VK_FALSE);
	viewport_stage.Init(surface->GetExtent());
	rasterization_stage.Init();
	depth_stencil_stage.Init();
	multisample_stage.Init();
	color_blend_stage.Init();
	CreateCommandPool();

	// Create pipeline info with all stages
	std::vector<vk::PipelineShaderStageCreateInfo> shader_stage_info;
	shader_stage_info.reserve(shader_stages.size());
	for (const auto& [type, shader_stage] : shader_stages)
		shader_stage_info.push_back(shader_stage->vk_shader_stage_info);

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
		vk_pipeline = device->GetDevice().createGraphicsPipeline(nullptr, vk_pipeline_info).value;
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Create Graphics Pipeline:\n{}", err.what()));
	}
}

void SwapchainRenderPipeline::Recreate(const RenderPass& render_pass)
{
	if (vk_pipeline)
	{
		device->GetDevice().destroyPipeline(vk_pipeline);
		vk_pipeline = VK_NULL_HANDLE;
	}
	Create(render_pass);
}

void SwapchainRenderPipeline::RecordDrawCommands(vk::CommandBuffer command_buffer, uint32_t image_index)
{
	// Bind the graphics pipeline
	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vk_pipeline);
	
	// Get the global bindless descriptor set
	auto* global_manager = vulkan::VulkanHandler::GetGlobalResourceManager();
	if (!global_manager)
	{
		// No global manager available - skip drawing to avoid device lost
		return;
	}
	
	vk::DescriptorSet bindless_descriptor_set = global_manager->GetBindlessDescriptorSet();
	if (bindless_descriptor_set == VK_NULL_HANDLE)
	{
		// Descriptor set not allocated - skip drawing to avoid device lost
		return;
	}
	
	// Bind the global bindless descriptor set (set 0)
	command_buffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics, 
		pipeline_layout.vk_pipeline_layout, 
		0, // first set
		1, // descriptor set count
		&bindless_descriptor_set, 
		0, // dynamic offset count
		nullptr // dynamic offsets
	);
	
	// Bind vertex and index buffers for traditional drawing
	vk::Buffer vertex_buffer = global_manager->GetVertexBuffer();
	vk::Buffer index_buffer = global_manager->GetIndexBuffer();
	
	if (vertex_buffer != VK_NULL_HANDLE)
	{
		vk::DeviceSize vertex_offset = 0;
		command_buffer.bindVertexBuffers(0, 1, &vertex_buffer, &vertex_offset);
	}
	
	if (index_buffer != VK_NULL_HANDLE)
	{
		command_buffer.bindIndexBuffer(index_buffer, 0, vk::IndexType::eUint32);
	}
	
	// Set push constants for frame index and material index
	struct PushConstants {
		uint32_t frame_index;
		uint32_t material_index;
		uint32_t transform_index;
		uint32_t vertex_offset;
	} push_constants;
	
	// CRITICAL FIX: Use safe frame index bounded by MAX_FRAMES  
	push_constants.frame_index = image_index % vulkan::MAX_FRAMES;
	push_constants.material_index = 0; // Use first material
	push_constants.transform_index = 0; // Use first transform
	push_constants.vertex_offset = 0; // No vertex offset
	
	command_buffer.pushConstants(
		pipeline_layout.vk_pipeline_layout,
		vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,  // Both stages for future flexibility
		0, // offset
		sizeof(push_constants),
		&push_constants
	);
	
	// SAFETY: Only draw if we have valid vertex data
	if (vertex_buffer != VK_NULL_HANDLE)
	{
		if (index_buffer != VK_NULL_HANDLE)
		{
			// Draw indexed vertices using the bindless system
			command_buffer.drawIndexed(3, 1, 0, 0, 0); // indexCount, instanceCount, firstIndex, vertexOffset, firstInstance
		}
		else
		{
			// Fallback to non-indexed rendering
			command_buffer.draw(3, 1, 0, 0); // Draw 3 vertices using built-in vertex generation in shader
		}
	}
	else
	{
		// No vertex data available - just draw without vertex buffer binding
		// This will still clear the screen due to render pass begin
		command_buffer.draw(3, 1, 0, 0); // Draw 3 vertices using built-in vertex generation in shader
	}
}

void SwapchainRenderPipeline::RecordDrawCommandsWithEntities(vk::CommandBuffer command_buffer, uint32_t image_index, 
															const std::vector<EntityRenderData>& entities)
{
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	// Bind the graphics pipeline
	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vk_pipeline);
	
	// Get the global bindless descriptor set
	auto* global_manager = vulkan::VulkanHandler::GetGlobalResourceManager();
	if (!global_manager)
	{
		if (logger) logger->Debug("No global manager available - skipping draw", "VKRender");
		return;
	}
	
	vk::DescriptorSet bindless_descriptor_set = global_manager->GetBindlessDescriptorSet();
	if (bindless_descriptor_set == VK_NULL_HANDLE)
	{
		if (logger) logger->Debug("No bindless descriptor set available - skipping draw", "VKRender");
		return;
	}
	
	// CRITICAL: Verify descriptor set layout compatibility
	auto* global_layout = vulkan::VulkanHandler::GetBindlessDescriptorLayout();
	if (!global_layout || global_layout->vk_descriptor_set_layout == VK_NULL_HANDLE)
	{
		if (logger) logger->Debug("Global descriptor layout not available - skipping draw", "VKRender");
		return;
	}
	
	// Bind the global bindless descriptor set (set 0)
	try
	{
		command_buffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics, 
			pipeline_layout.vk_pipeline_layout, 
			0, // first set
			1, // descriptor set count
			&bindless_descriptor_set, 
			0, // dynamic offset count
			nullptr // dynamic offsets
		);
		if (logger) logger->Debug("Successfully bound descriptor sets", "VKRender");
	}
	catch (const vk::SystemError& e)
	{
		if (logger) logger->Error(std::format("Failed to bind descriptor sets: {}", e.what()), "VKRender");
		return;
	}
	
	// Bind vertex and index buffers for traditional drawing
	vk::Buffer vertex_buffer = global_manager->GetVertexBuffer();
	vk::Buffer index_buffer = global_manager->GetIndexBuffer();
	
	if (vertex_buffer != VK_NULL_HANDLE)
	{
		try
		{
			vk::DeviceSize vertex_offset = 0;
			command_buffer.bindVertexBuffers(0, 1, &vertex_buffer, &vertex_offset);
			if (logger) logger->Debug("Successfully bound vertex buffer", "VKRender");
		}
		catch (const vk::SystemError& e)
		{
			if (logger) logger->Error(std::format("Failed to bind vertex buffer: {}", e.what()), "VKRender");
			return;
		}
	}
	else
	{
		if (logger) logger->Debug("No vertex buffer available", "VKRender");
	}
	
	if (index_buffer != VK_NULL_HANDLE)
	{
		try
		{
			command_buffer.bindIndexBuffer(index_buffer, 0, vk::IndexType::eUint32);
			if (logger) logger->Debug("Successfully bound index buffer", "VKRender");
		}
		catch (const vk::SystemError& e)
		{
			if (logger) logger->Error(std::format("Failed to bind index buffer: {}", e.what()), "VKRender");
			return;
		}
	}
	else
	{
		if (logger) logger->Debug("No index buffer available", "VKRender");
	}
	
	// If no entities provided, draw a fallback triangle for testing
	if (entities.empty())
	{
		if (logger) logger->Debug("No entities provided - drawing fallback triangle", "VKRender");
		
		struct PushConstants {
			uint32_t frame_index;
			uint32_t material_index;
			uint32_t transform_index;
			uint32_t vertex_offset;
		} push_constants;
		
		push_constants.frame_index = image_index % nft::vulkan::MAX_FRAMES;
		push_constants.material_index = 0;
		push_constants.transform_index = 0;
		push_constants.vertex_offset = 0;
		
		try
		{
			command_buffer.pushConstants(
				pipeline_layout.vk_pipeline_layout,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
				0, // offset
				sizeof(push_constants),
				&push_constants
			);
			
			// Draw fallback triangle
			if (index_buffer != VK_NULL_HANDLE)
			{
				command_buffer.drawIndexed(3, 1, 0, 0, 0); // 3 indices, 1 instance
				if (logger) logger->Debug("Drew fallback triangle (indexed)", "VKRender");
			}
			else if (vertex_buffer != VK_NULL_HANDLE)
			{
				command_buffer.draw(3, 1, 0, 0); // 3 vertices, 1 instance
				if (logger) logger->Debug("Drew fallback triangle (non-indexed)", "VKRender");
			}
			else
			{
				// No buffers - use built-in triangle generation in vertex shader
				command_buffer.draw(3, 1, 0, 0);
				if (logger) logger->Debug("Drew built-in triangle", "VKRender");
			}
		}
		catch (const vk::SystemError& e)
		{
			if (logger) logger->Error(std::format("Failed to draw fallback triangle: {}", e.what()), "VKRender");
		}
		
		return;
	}
	
	// PHASE 2: Actual entity drawing
	if (logger) logger->Debug(std::format("Drawing {} entities", entities.size()), "VKRender");
	
	uint32_t entities_drawn = 0;
	for (size_t i = 0; i < entities.size(); ++i)
	{
		const auto& entity = entities[i];
		
		// Validate entity data
		if (entity.vertex_count == 0 || entity.index_count == 0)
		{
			if (logger) logger->Warn(std::format("Entity {} - Invalid vertex/index count (v={}, i={}), skipping", 
				i, entity.vertex_count, entity.index_count), "VKRender");
			continue;
		}
		
		// Set push constants for this entity
		struct PushConstants {
			uint32_t frame_index;
			uint32_t material_index;
			uint32_t transform_index;
			uint32_t vertex_offset;
		} push_constants;
		
		// CRITICAL FIX: Ensure frame index is always within bounds
		uint32_t safe_frame_index = image_index % std::min(nft::vulkan::MAX_FRAMES, 3u); // Limit to 3 frames max
		push_constants.frame_index = safe_frame_index;
		push_constants.material_index = entity.material_index;
		push_constants.transform_index = entity.transform_index;
		push_constants.vertex_offset = entity.vertex_offset;
		
		if (logger && i == 0) { // Only log first entity to reduce spam
			logger->Debug(std::format("Entity {} push constants: frame_idx={}=>{}, material={}, transform={}, vertex_offset={}",
				i, image_index, push_constants.frame_index, push_constants.material_index, 
				push_constants.transform_index, push_constants.vertex_offset), "VKRender");
		}
		
		try
		{
			command_buffer.pushConstants(
				pipeline_layout.vk_pipeline_layout,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
				0, // offset
				sizeof(push_constants),
				&push_constants
			);
			
			// Draw this entity
			if (index_buffer != VK_NULL_HANDLE)
			{
				// Use indexed drawing
				// NOTE: Our indices are already absolute within the buffer, and we bound the vertex buffer at offset 0
				// So we don't need to add vertex_offset again - just use 0 for vertexOffset
				command_buffer.drawIndexed(
					entity.index_count,    // indexCount
					1,                     // instanceCount
					entity.index_offset,   // firstIndex
					0,                     // vertexOffset - should be 0 since indices are absolute
					0                      // firstInstance
				);
				
				if (logger && i == 0) { // Only log first entity
					logger->Debug(std::format("Entity {} - Drew indexed: indices={} (offset {}), vertices={} (offset {})",
						i, entity.index_count, entity.index_offset, entity.vertex_count, entity.vertex_offset), "VKRender");
				}
			}
			else if (vertex_buffer != VK_NULL_HANDLE)
			{
				// Use non-indexed drawing
				command_buffer.draw(
					entity.vertex_count,   // vertexCount
					1,                     // instanceCount
					entity.vertex_offset,  // firstVertex
					0                      // firstInstance
				);
				
				if (logger && i == 0) { // Only log first entity
					logger->Debug(std::format("Entity {} - Drew non-indexed: vertices={} (offset {})",
						i, entity.vertex_count, entity.vertex_offset), "VKRender");
				}
			}
			else
			{
				if (logger) logger->Debug(std::format("Entity {} - No buffers available, skipping", i), "VKRender");
				continue;
			}
			
			entities_drawn++;
		}
		catch (const vk::SystemError& e)
		{
			if (logger) logger->Error(std::format("Entity {} - Draw failed: {}", i, e.what()), "VKRender");
			// Continue trying to draw other entities
		}
	}
	
	if (logger) logger->Debug(std::format("Successfully drew {} out of {} entities", entities_drawn, entities.size()), "VKRender");
}


//=============================================================================
// HELPER FUNCTIONS
//=============================================================================

vk::DescriptorSet GetDescriptorSet(Device* device, DescriptorPool* pool, DescriptorSetLayout* layout)
{
	vk::DescriptorSetAllocateInfo alloc_info = vk::DescriptorSetAllocateInfo()
												   .setDescriptorPool(pool->vk_descriptor_pool)
												   .setDescriptorSetCount(1)
												   .setPSetLayouts(&layout->vk_descriptor_set_layout);
	try
	{
		return device->GetDevice().allocateDescriptorSets(alloc_info)[0];
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Allocate Descriptor Set:\n{}", err.what()));
		return VK_NULL_HANDLE;
	}
}
}	 // namespace nft::vulkan