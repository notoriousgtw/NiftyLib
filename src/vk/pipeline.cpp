#include "vk/pipeline.h"

#include "vk/geometry.h"	// For MaterialPushConstants
#include "vk/handler.h"
#include "vk/surface.h"	   // Forward declaration resolution

#include "vk/image.h"
#include "vk/scene.h"
namespace nft::vulkan
{

//=============================================================================
// PIPELINE STAGE IMPLEMENTATIONS
//=============================================================================

void VertexInputStage::Init()
{
	binding_description	   = GetVertexInputBindingDescription();
	attribute_descriptions = GetVertexInputAttributeDescriptions();

	vk_vertex_input_info = vk::PipelineVertexInputStateCreateInfo()
							   .setFlags(vk::PipelineVertexInputStateCreateFlags())
							   .setVertexBindingDescriptionCount(1)
							   .setPVertexBindingDescriptions(&binding_description)
							   .setVertexAttributeDescriptionCount(attribute_descriptions.size())
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
	if (vk_descriptor_set_layout && device && device->GetDevice())
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
	if (vk_descriptor_pool && device && device->GetDevice())
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
	if (vk_pipeline_layout && device && device->GetDevice())
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
	if (vk_render_pass && device && device->GetDevice())
	{
		device->GetDevice().destroyRenderPass(vk_render_pass);
		vk_render_pass = VK_NULL_HANDLE;
	}
}

//=============================================================================
// FRAME IMPLEMENTATIONS (moved from Surface)
//=============================================================================

void Frame::Init(Surface* surface, Scene* scene)
{
	if (!surface)
		NFT_ERROR(VulkanFatal, "Surface pointer is null!");
	if (!scene)
		NFT_ERROR(VulkanFatal, "Scene pointer is null!");
	this->surface	= surface;
	this->device	= surface->GetDevice();
	this->scene		= scene;
	swapchain_image = Image(device);
	depth_buffer	= Image(device);
}

void Frame::MakeDescriptorResources()
{
	if (!device)
		NFT_ERROR(VulkanFatal, "Device pointer is null!");

	camera_data_buffer = device->GetBufferManager()->CreateBuffer(sizeof(UniformBufferObject),
																  vk::BufferUsageFlagBits::eUniformBuffer,
																  vk::MemoryPropertyFlagBits::eHostVisible |
																	  vk::MemoryPropertyFlagBits::eHostCoherent);
	camera_data_ptr =
		device->GetDevice().mapMemory(camera_data_buffer->vk_memory, 0, sizeof(UniformBufferObject), vk::MemoryMapFlags {});

	std::memset(camera_data_ptr, 0, sizeof(UniformBufferObject));

	object_transform_buffer = device->GetBufferManager()->CreateBuffer(sizeof(glm::mat4) * MAX_OBJECTS,
																	   vk::BufferUsageFlagBits::eStorageBuffer,
																	   vk::MemoryPropertyFlagBits::eHostVisible |
																		   vk::MemoryPropertyFlagBits::eHostCoherent);
	object_transform_ptr =
		device->GetDevice().mapMemory(object_transform_buffer->vk_memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags {});

	// Zero out the buffer
	std::memset(object_transform_ptr, 0, MAX_OBJECTS * sizeof(glm::mat4));
}

void Frame::AllocateFrameDescriptorSet(DescriptorPool* frame_descriptor_pool, DescriptorSetLayout* frame_set_layout)
{
	if (!frame_descriptor_pool || !frame_set_layout)
	{
		NFT_ERROR(VulkanFatal, "Descriptor pool or layout is null!");
		return;
	}

	// Allocate the frame descriptor set
	vk_descriptor_set = GetDescriptorSet(device, frame_descriptor_pool, frame_set_layout);

	// Update the descriptor set with buffer info
	std::vector<vk::WriteDescriptorSet> descriptor_writes;

	// Camera data (binding 0)
	vk::DescriptorBufferInfo camera_buffer_info =
		vk::DescriptorBufferInfo().setBuffer(camera_data_buffer->vk_buffer).setOffset(0).setRange(sizeof(UniformBufferObject));

	descriptor_writes.push_back(vk::WriteDescriptorSet()
									.setDstSet(vk_descriptor_set)
									.setDstBinding(0)
									.setDstArrayElement(0)
									.setDescriptorCount(1)
									.setDescriptorType(vk::DescriptorType::eUniformBuffer)
									.setPBufferInfo(&camera_buffer_info));

	// Object transforms (binding 1)
	vk::DescriptorBufferInfo object_buffer_info = vk::DescriptorBufferInfo()
													  .setBuffer(object_transform_buffer->vk_buffer)
													  .setOffset(0)
													  .setRange(sizeof(glm::mat4) * MAX_OBJECTS);

	descriptor_writes.push_back(vk::WriteDescriptorSet()
									.setDstSet(vk_descriptor_set)
									.setDstBinding(1)
									.setDstArrayElement(0)
									.setDescriptorCount(1)
									.setDescriptorType(vk::DescriptorType::eStorageBuffer)
									.setPBufferInfo(&object_buffer_info));

	// Update all descriptors
	device->GetDevice().updateDescriptorSets(descriptor_writes, nullptr);
}

void Frame::MakeDepthResources()
{
	// Depth resources are created during swapchain creation
	// This method is kept for compatibility
}

void Frame::Prepare(glm::mat4 camera_transforms)
{
	if (!surface)
		NFT_ERROR(VulkanFatal, "Surface pointer is null!");
	if (!scene)
		NFT_ERROR(VulkanFatal, "Scene pointer is null!");
	if (!camera_data_buffer)
		NFT_ERROR(VulkanFatal, "Camera data buffer is not initialized!");

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

	std::memcpy(camera_data_ptr, &camera_data, sizeof(UniformBufferObject));

	// Use getter method to access objects
	const auto&	 objects	  = scene->GetObjects();
	const size_t object_count = objects.size();
	for (size_t idx = 0; idx < object_count; ++idx)
	{
		// object_transforms[idx] = objects[idx].transform;
		object_transforms.emplace_back(objects[idx].transform);
	}

	const size_t bytes = object_count * sizeof(glm::mat4);
	std::memcpy(object_transform_ptr, object_transforms.data(), bytes);
}

void Frame::Cleanup()
{
	if (device)
	{
		device->GetDevice().destroyFramebuffer(vk_frame_buffer);

		device->GetDevice().destroyFence(in_flight_fence);

		device->GetDevice().destroySemaphore(image_available_semaphore);

		device->GetDevice().destroySemaphore(render_finished_semaphore);

		if (camera_data_ptr)
		{
			device->GetDevice().unmapMemory(camera_data_buffer->vk_memory);
			camera_data_ptr = nullptr;
		}
		if (camera_data_buffer)
		{
			device->GetBufferManager()->DestroyBuffer(camera_data_buffer);
			camera_data_buffer = nullptr;
		}
		if (object_transform_ptr)
		{
			device->GetDevice().unmapMemory(object_transform_buffer->vk_memory);
			object_transform_ptr = nullptr;
		}
		if (object_transform_buffer)
		{
			device->GetBufferManager()->DestroyBuffer(object_transform_buffer);
			object_transform_buffer = nullptr;
		}
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

		for (auto& shader_stage : shader_stages)
			if (shader_stage.shader)
				shader_stage.shader.reset();
		shader_stages.clear();

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

//=============================================================================
// GRAPHICS PIPELINE BASE IMPLEMENTATIONS
//=============================================================================

GraphicsPipelineBase::GraphicsPipelineBase(Device* device):
	AbstractPipeline(device, PipelineType::Graphics),
	vertex_input_stage(device),
	input_assembly_stage(device),
	viewport_stage(device),
	rasterization_stage(device),
	depth_stencil_stage(device),
	multisample_stage(device),
	color_blend_stage(device),
	render_pass(device)
{
}

void GraphicsPipelineBase::AddShaderStage(Shader::ShaderCode shader_code, vk::ShaderStageFlagBits stage)
{
	shader_stages.emplace_back(device);
	shader_stages.back().shader				  = std::make_unique<Shader>(device, shader_code);
	shader_stages.back().vk_shader_stage_info = vk::PipelineShaderStageCreateInfo()
													.setFlags(vk::PipelineShaderStageCreateFlags())
													.setStage(stage)
													.setModule(shader_stages.back().shader->GetShaderModule())
													.setPName("main");
}

void GraphicsPipelineBase::Create()
{
	// Create pipeline info with all stages
	std::vector<vk::PipelineShaderStageCreateInfo> shader_stage_info;
	shader_stage_info.reserve(shader_stages.size());
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
		vk_pipeline = device->GetDevice().createGraphicsPipeline(nullptr, vk_pipeline_info).value;
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Create Graphics Pipeline:\n{}", err.what()));
	}
}

void GraphicsPipelineBase::Recreate()
{
	if (vk_pipeline)
	{
		device->GetDevice().destroyPipeline(vk_pipeline);
		vk_pipeline = VK_NULL_HANDLE;
	}
	Create();
}

void GraphicsPipelineBase::Cleanup()
{
	// Cleanup graphics-specific resources
	if (device && device->vk_device)
	{
		render_pass.Cleanup();
	}

	// Call base cleanup
	AbstractPipeline::Cleanup();
}

void GraphicsPipelineBase::InitGraphics(vk::Extent2D extent, vk::Format color_format, vk::Format depth_format)
{
	CreateCommandPool();
	SetupPipelineStages(extent);
	SetupDescriptorLayouts();
	render_pass.Init(color_format, depth_format);
}

void GraphicsPipelineBase::InitGraphics(vk::Extent2D extent, const RenderPass& render_pass)
{
	CreateCommandPool();
	SetupPipelineStages(extent);
	SetupDescriptorLayouts();
	this->render_pass = render_pass;
}

void GraphicsPipelineBase::SetupPipelineStages(vk::Extent2D extent)
{
	vertex_input_stage.Init();
	input_assembly_stage.Init(vk::PrimitiveTopology::eTriangleList);
	viewport_stage.Init(extent);
	rasterization_stage.Init();
	depth_stencil_stage.Init();
	multisample_stage.Init();
	color_blend_stage.Init();
}

//=============================================================================
// COMPUTE PIPELINE BASE IMPLEMENTATIONS
//=============================================================================

ComputePipelineBase::ComputePipelineBase(Device* device): AbstractPipeline(device, PipelineType::Compute) {}

void ComputePipelineBase::AddShaderStage(Shader::ShaderCode shader_code, vk::ShaderStageFlagBits stage)
{
	if (stage != vk::ShaderStageFlagBits::eCompute)
	{
		NFT_ERROR(VulkanFatal, "Compute pipeline can only accept compute shaders!");
		return;
	}

	// Clear previous stages (compute pipeline should only have one stage)
	shader_stages.clear();
	shader_stages.emplace_back(device);
	shader_stages.back().shader				  = std::make_unique<Shader>(device, shader_code);
	shader_stages.back().vk_shader_stage_info = vk::PipelineShaderStageCreateInfo()
													.setFlags(vk::PipelineShaderStageCreateFlags())
													.setStage(stage)
													.setModule(shader_stages.back().shader->GetShaderModule())
													.setPName("main");
}

void ComputePipelineBase::Create()
{
	if (shader_stages.empty())
	{
		NFT_ERROR(VulkanFatal, "No compute shader stage added to compute pipeline!");
		return;
	}

	vk_compute_pipeline_info = vk::ComputePipelineCreateInfo()
								   .setFlags(vk::PipelineCreateFlags())
								   .setStage(shader_stages[0].vk_shader_stage_info)
								   .setLayout(pipeline_layout.vk_pipeline_layout)
								   .setBasePipelineHandle(nullptr);

	try
	{
		vk_pipeline = device->GetDevice().createComputePipeline(nullptr, vk_compute_pipeline_info).value;
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Create Compute Pipeline:\n{}", err.what()));
	}
}

void ComputePipelineBase::Recreate()
{
	if (vk_pipeline)
	{
		device->GetDevice().destroyPipeline(vk_pipeline);
		vk_pipeline = VK_NULL_HANDLE;
	}
	Create();
}

void ComputePipelineBase::Dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
{
	Dispatch(group_count_x, group_count_y, group_count_z, vk_command_buffer);
}

void ComputePipelineBase::Dispatch(uint32_t			 group_count_x,
								   uint32_t			 group_count_y,
								   uint32_t			 group_count_z,
								   vk::CommandBuffer command_buffer)
{
	command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, vk_pipeline);
	command_buffer.dispatch(group_count_x, group_count_y, group_count_z);
}

//=============================================================================
// SWAPCHAIN GRAPHICS PIPELINE IMPLEMENTATIONS
//=============================================================================

GraphicsPipeline::GraphicsPipeline(Device* device, Scene* scene):
	GraphicsPipelineBase(device),
	scene(scene),
	clear_color(vk::ClearColorValue(std::array<float, 4> { 0.2f, 0.2f, 0.2f, 1.0f })),
	clear_depth(vk::ClearDepthStencilValue(1.0f, 0)),
	frame_set_layout(device),
	texture_set_layout(device),
	frame_descriptor_pool(device),
	texture_descriptor_pool(device)
{
}

void GraphicsPipeline::Init()
{
	// This will be called from Surface when extent and formats are known
	// Implementation moved to Surface::CreatePipeline()
}

void GraphicsPipeline::SetupDescriptorLayouts()
{
	// Set 0: Frame data (camera + object transforms)
	std::vector<DescriptorSetLayout::Binding> frame_bindings = {
		{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
		{ 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex }
	};
	frame_set_layout.Init(frame_bindings);

	//// Set 1: Texture array
	// std::vector<DescriptorSetLayout::Binding> texture_bindings = {
	//	{ 0, vk::DescriptorType::eCombinedImageSampler, 32, vk::ShaderStageFlagBits::eFragment }
	// };
	// texture_set_layout.Init(texture_bindings);
	//  Set 1: Bindless textures with variable count
	std::vector<DescriptorSetLayout::Binding> texture_bindings = {
		// Use a very large count or VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
		{ 0, vk::DescriptorType::eCombinedImageSampler, 65536, vk::ShaderStageFlagBits::eFragment }
	};

	// Enable descriptor indexing features
	vk::DescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info;
	std::vector<vk::DescriptorBindingFlags>		  binding_flags = { vk::DescriptorBindingFlagBits::eVariableDescriptorCount |
																	vk::DescriptorBindingFlagBits::ePartiallyBound |
																	vk::DescriptorBindingFlagBits::eUpdateAfterBind };
	binding_flags_info.setBindingCount(binding_flags.size()).setPBindingFlags(binding_flags.data());

	texture_set_layout.vk_descriptor_set_layout_info.setPNext(&binding_flags_info);
	texture_set_layout.Init(texture_bindings);

	vk_descriptor_set_layouts = { frame_set_layout.vk_descriptor_set_layout, texture_set_layout.vk_descriptor_set_layout };
}

void GraphicsPipeline::PrepareScene(vk::CommandBuffer command_buffer)
{
	if (!scene)
	{
		NFT_ERROR(VulkanFatal, "Scene is not set for rendering!");
		return;
	}

	const auto* geometry_batcher = scene->GetGeometryBatcher();
	if (!geometry_batcher)
	{
		NFT_ERROR(VulkanFatal, "Geometry batcher is null!");
		return;
	}

	vk::Buffer	 vertex_buffers[] = { geometry_batcher->GetVertexBuffer()->vk_buffer };
	VkDeviceSize offsets[]		  = { 0 };
	command_buffer.bindVertexBuffers(0, 1, vertex_buffers, offsets);

	// Check if we have index data using the new getter method
	if (!geometry_batcher->HasIndexData())
		return;

	command_buffer.bindIndexBuffer(geometry_batcher->GetIndexBuffer()->vk_buffer, 0, vk::IndexType::eUint32);
}

void GraphicsPipeline::RecordDrawCommands(Frame& frame, uint32_t image_index)
{
	auto					   command_buffer = frame.vk_command_buffer;
	vk::CommandBufferBeginInfo begin_info	  = vk::CommandBufferBeginInfo();
	command_buffer.begin(begin_info);

	std::vector<vk::ClearValue> clear_values = { clear_color, clear_depth };

	vk::RenderPassBeginInfo render_pass_begin_info =
		vk::RenderPassBeginInfo()
			.setRenderPass(render_pass.vk_render_pass)
			.setFramebuffer(frame.vk_frame_buffer)
			.setRenderArea(vk::Rect2D().setOffset({ 0, 0 }).setExtent(viewport_stage.scissor.extent))
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

	const auto& mesh_data = scene->GetGeometryBatcher()->GetMeshData();
	const auto& objects	  = scene->GetObjects();	  // Use getter method
	const auto& materials = scene->GetMaterials();	  // Use getter method

	for (const auto& mesh_entry : mesh_data)
	{
		const IMesh*					 mesh			 = mesh_entry.first;
		const GeometryBatcher::MeshData& mesh_data_entry = mesh_entry.second;

		uint32_t vertex_count = static_cast<uint32_t>(mesh_data_entry.size);
		uint32_t first_vertex = static_cast<uint32_t>(mesh_data_entry.offset);
		uint32_t index_count  = static_cast<uint32_t>(mesh_data_entry.index_size);
		uint32_t first_index  = static_cast<uint32_t>(mesh_data_entry.index_offset);

		// Draw each object separately with per-object material push constants
		for (uint32_t instance_id = 0; instance_id < objects.size(); ++instance_id)
		{
			if (objects[instance_id].mesh == mesh)
			{
				const auto& object = objects[instance_id];

				// Create material push constants for this object
				MaterialPushConstants material_push;
				if (object.material_index < materials.size())
				{
					const auto& material			  = materials[object.material_index];
					material_push.ambient			  = material.ambient;
					material_push.diffuse			  = material.diffuse;
					material_push.specular			  = material.specular;
					material_push.specular_highlights = material.specular_highlights;
					material_push.diffuse_texture_index =
						material.diffuse_texture_index != UINT32_MAX ? material.diffuse_texture_index : 33;
					material_push.ambient_texture_index =
						material.ambient_texture_index != UINT32_MAX ? material.ambient_texture_index : 33;
					material_push.specular_texture_index =
						material.specular_texture_index != UINT32_MAX ? material.specular_texture_index : 33;
					material_push.padding = 0;
				}
				else
				{
					// Default material
					material_push.ambient				 = glm::vec3(0.1f);
					material_push.diffuse				 = glm::vec3(0.8f);
					material_push.specular				 = glm::vec3(0.5f);
					material_push.specular_highlights	 = 32.0f;
					material_push.diffuse_texture_index	 = 0;
					material_push.ambient_texture_index	 = 31;
					material_push.specular_texture_index = 31;
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
					command_buffer.draw(vertex_count, 1, first_vertex, instance_id);
				}
				else
				{
					command_buffer.drawIndexed(index_count, 1, first_index, 0, instance_id);
				}
			}
		}
	}

	command_buffer.endRenderPass();
	command_buffer.end();
}

void GraphicsPipeline::CreateTextureDescriptorSet()
{
	const auto& textures = scene->GetTextures();
	if (!scene || textures.empty())
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
		texture_descriptor_set = device->GetDevice().allocateDescriptorSets(alloc_info)[0];
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Allocate Texture Array Descriptor Set:\n{}", err.what()));
	}

	// Create array of descriptor image infos
	std::vector<vk::DescriptorImageInfo> image_infos;
	image_infos.reserve(32);

	// Add all available textures using the new getter method
	for (size_t i = 0; i < textures.size() && i < 32; ++i)
	{
		const auto& texture = textures[i];
		image_infos.push_back(vk::DescriptorImageInfo()
								  .setSampler(texture.GetSampler())
								  .setImageView(texture.GetImageView())
								  .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));
	}

	// Fill remaining slots with the first texture to avoid validation errors
	for (size_t i = textures.size(); i < 32; ++i)
	{
		const auto& texture = textures[0];
		image_infos.push_back(vk::DescriptorImageInfo()
								  .setSampler(texture.GetSampler())
								  .setImageView(texture.GetImageView())
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

	device->GetDevice().updateDescriptorSets(1, &descriptor_write, 0, nullptr);
}

void GraphicsPipeline::CreatePipeline()
{
	// Set up descriptor pools
	std::vector<DescriptorSetLayout::Binding> frame_bindings = {
		{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
		{ 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex }
	};

	std::vector<DescriptorSetLayout::Binding> texture_bindings = {
		{ 0, vk::DescriptorType::eCombinedImageSampler, 32, vk::ShaderStageFlagBits::eFragment }
	};

	frame_descriptor_pool.Init(frame_bindings, 3);	  // Assume max 3 frames in flight
	texture_descriptor_pool.Init(texture_bindings, 1);

	// Create pipeline layout with push constants
	vk::PushConstantRange material_push_constant_range = vk::PushConstantRange()
															 .setOffset(0)
															 .setSize(sizeof(MaterialPushConstants))
															 .setStageFlags(vk::ShaderStageFlagBits::eFragment);

	pipeline_layout.Init(vk_descriptor_set_layouts, { material_push_constant_range });

	// Call base implementation to create the pipeline
	Create();
}

void GraphicsPipeline::RecreatePipeline()
{
	Recreate();
}

//=============================================================================
// OFFSCREEN GRAPHICS PIPELINE IMPLEMENTATIONS
//=============================================================================

OffscreenGraphicsPipeline::OffscreenGraphicsPipeline(Device*	  device,
													 vk::Extent2D extent,
													 vk::Format	  color_format,
													 vk::Format	  depth_format):
	GraphicsPipelineBase(device), extent(extent), color_format(color_format), depth_format(depth_format)
{
}

void OffscreenGraphicsPipeline::Init()
{
	CreateCommandPool();
	SetupPipelineStages(extent);
	SetupDescriptorLayouts();

	if (depth_format != vk::Format::eUndefined)
	{
		render_pass.Init(color_format, depth_format);
	}
	else
	{
		// Create a render pass with only color attachment
		std::vector<vk::AttachmentDescription> attachments = { vk::AttachmentDescription()
																   .setFormat(color_format)
																   .setSamples(vk::SampleCountFlagBits::e1)
																   .setLoadOp(vk::AttachmentLoadOp::eClear)
																   .setStoreOp(vk::AttachmentStoreOp::eStore)
																   .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
																   .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
																   .setInitialLayout(vk::ImageLayout::eUndefined)
																   .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal) };

		vk::AttachmentReference color_ref =
			vk::AttachmentReference().setAttachment(0).setLayout(vk::ImageLayout::eColorAttachmentOptimal);

		std::vector<vk::SubpassDescription> subpasses = { vk::SubpassDescription()
															  .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
															  .setColorAttachmentCount(1)
															  .setPColorAttachments(&color_ref) };

		render_pass.Init(attachments, subpasses);
	}
}

void OffscreenGraphicsPipeline::SetupDescriptorLayouts()
{
	// Default implementation - can be overridden by derived classes
	vk_descriptor_set_layouts.clear();
}

void OffscreenGraphicsPipeline::CreatePipeline()
{
	pipeline_layout.Init(vk_descriptor_set_layouts);
	Create();
}

void OffscreenGraphicsPipeline::RecreatePipeline()
{
	Recreate();
}

void OffscreenGraphicsPipeline::SetRenderTarget(Image* color_target, Image* depth_target)
{
	this->color_target = color_target;
	this->depth_target = depth_target;
}

void OffscreenGraphicsPipeline::CreateFramebuffer()
{
	if (!color_target)
	{
		NFT_ERROR(VulkanFatal, "Color target not set for offscreen pipeline!");
		return;
	}

	std::vector<vk::ImageView> attachments = { color_target->GetImageView() };
	if (depth_target)
	{
		attachments.push_back(depth_target->GetImageView());
	}

	vk::FramebufferCreateInfo framebuffer_info = vk::FramebufferCreateInfo()
													 .setRenderPass(render_pass.vk_render_pass)
													 .setAttachmentCount(attachments.size())
													 .setPAttachments(attachments.data())
													 .setWidth(extent.width)
													 .setHeight(extent.height)
													 .setLayers(1);

	try
	{
		framebuffer = device->GetDevice().createFramebuffer(framebuffer_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VulkanFatal, std::format("Failed To Create Offscreen Framebuffer:\n{}", err.what()));
	}
}

//=============================================================================
// EXAMPLE COMPUTE PIPELINE IMPLEMENTATIONS
//=============================================================================

ExampleComputePipeline::ExampleComputePipeline(Device* device):
	ComputePipelineBase(device), compute_set_layout(device), compute_descriptor_pool(device)
{
}

void ExampleComputePipeline::Init()
{
	CreateCommandPool();
	SetupDescriptorLayouts();
	SetupComputeStage();
}

void ExampleComputePipeline::SetupDescriptorLayouts()
{
	// Example descriptor layout for compute shader
	std::vector<DescriptorSetLayout::Binding> compute_bindings = {
		{ 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute },
		{ 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute }
	};
	compute_set_layout.Init(compute_bindings);

	vk_descriptor_set_layouts = { compute_set_layout.vk_descriptor_set_layout };

	// Create descriptor pool
	compute_descriptor_pool.Init(compute_bindings, 1);

	// Create pipeline layout
	pipeline_layout.Init(vk_descriptor_set_layouts);
}

void ExampleComputePipeline::SetupComputeStage()
{
	// This would be implemented by derived classes to add the actual compute shader
	// Example:
	// AddShaderStage(compute_shader_code, vk::ShaderStageFlagBits::eCompute);
	// Create();
}

//=============================================================================
// UTILITY FUNCTIONS
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

vk::VertexInputBindingDescription GetVertexInputBindingDescription()
{
	vk::VertexInputBindingDescription binding_description;
	binding_description.binding	  = 0;								 // Binding index
	binding_description.stride	  = 12 * sizeof(float);				 // Size of each vertex
	binding_description.inputRate = vk::VertexInputRate::eVertex;	 // Per-vertex data
	return binding_description;
}

std::vector<vk::VertexInputAttributeDescription> GetVertexInputAttributeDescriptions()
{
	std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;
	attribute_descriptions.reserve(4);
	// Position attribute
	attribute_descriptions.push_back(vk::VertexInputAttributeDescription()
										 .setBinding(0)
										 .setLocation(0)
										 .setFormat(vk::Format::eR32G32B32A32Sfloat)
										 .setOffset(0));
	// Texture Coordinate attribute
	attribute_descriptions.push_back(vk::VertexInputAttributeDescription()
										 .setBinding(0)
										 .setLocation(2)
										 .setFormat(vk::Format::eR32G32Sfloat)
										 .setOffset(7 * sizeof(float)));
	// Normal attribute
	attribute_descriptions.push_back(vk::VertexInputAttributeDescription()
										 .setBinding(0)
										 .setLocation(3)
										 .setFormat(vk::Format::eR32G32B32Sfloat)
										 .setOffset(9 * sizeof(float)));
	// Color attribute
	attribute_descriptions.push_back(vk::VertexInputAttributeDescription()
										 .setBinding(0)
										 .setLocation(1)
										 .setFormat(vk::Format::eR32G32B32A32Sfloat)
										 .setOffset(3 * sizeof(float)));
	return attribute_descriptions;
}

}	 // namespace nft::vulkan