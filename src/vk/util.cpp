#include "vk/util.h"

#include "vk/handler.h"
#include "vk/geometry.h"  // For MaterialPushConstants

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

void InputAssemblyStage::Init()
{
	vk_input_assembly_info = vk::PipelineInputAssemblyStateCreateInfo()
								 .setFlags(vk::PipelineInputAssemblyStateCreateFlags())
								 .setTopology(vk::PrimitiveTopology::eTriangleList);
}

void ViewportStage::Init(vk::Extent2D extent)
{
	viewport.setX(0.0f)
		.setY(0.0f)
		.setWidth(static_cast<float>(extent.width))
		.setHeight(static_cast<float>(extent.height))
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f);
	scissor.setOffset({ 0, 0 }).setExtent(extent);
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

DescriptorSetLayout::DescriptorSetLayout(Surface* surface):
	surface(surface), device(surface->device)
{
	if (!surface)
		NFT_ERROR(VKFatal, "Surface Is Null!");
	if (!device)
		NFT_ERROR(VKFatal, "Device Is Null!");
}

DescriptorSetLayout::DescriptorSetLayout(Device* device):
	surface(nullptr), device(device)
{
	if (!device)
		NFT_ERROR(VKFatal, "Device Is Null!");
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
														.setPImmutableSamplers(nullptr);	// No immutable samplers for now
		vk_bindings.push_back(vk_binding);
	}

	vk_descriptor_set_layout_info = vk::DescriptorSetLayoutCreateInfo()
										.setFlags(vk::DescriptorSetLayoutCreateFlags())
										.setBindingCount(vk_bindings.size())
										.setPBindings(vk_bindings.data());
	try
	{
		vk_descriptor_set_layout =
			device->vk_device.createDescriptorSetLayout(vk_descriptor_set_layout_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Create Descriptor Set Layout:\n{}", err.what()));
	}

	if (surface)
		surface->vk_descriptor_set_layouts.push_back(vk_descriptor_set_layout);
}

void DescriptorSetLayout::Cleanup()
{
	if (vk_descriptor_set_layout && device && device->vk_device)
	{
		device->vk_device.destroyDescriptorSetLayout(vk_descriptor_set_layout);
		vk_descriptor_set_layout = VK_NULL_HANDLE;
	}
}

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
		vk_descriptor_pool = device->vk_device.createDescriptorPool(vk_descriptor_pool_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Create Descriptor Pool:\n{}", err.what()));
	}
}

void DescriptorPool::Cleanup()
{
	if (vk_descriptor_pool && device && device->vk_device)
	{
		device->vk_device.destroyDescriptorPool(vk_descriptor_pool);
		vk_descriptor_pool = VK_NULL_HANDLE;
	}
}

void PipelineLayout::Init(std::vector<vk::DescriptorSetLayout> descriptor_set_layouts)
{
	// Add push constant range for material data
	vk::PushConstantRange material_push_constant_range = vk::PushConstantRange()
														 .setOffset(0)
														 .setSize(sizeof(MaterialPushConstants))
														 .setStageFlags(vk::ShaderStageFlagBits::eFragment);

	vk_pipeline_layout_info = vk::PipelineLayoutCreateInfo()
								  .setFlags(vk::PipelineLayoutCreateFlags())
								  .setSetLayoutCount(descriptor_set_layouts.size())
								  .setPSetLayouts(descriptor_set_layouts.data())
								  .setPushConstantRangeCount(1)
								  .setPPushConstantRanges(&material_push_constant_range);

	try
	{
		vk_pipeline_layout = device->vk_device.createPipelineLayout(vk_pipeline_layout_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Create Pipeline Layout:\n{}", err.what()));
	}
}

void PipelineLayout::Cleanup()
{
	if (vk_pipeline_layout && device && device->vk_device)
	{
		device->vk_device.destroyPipelineLayout(vk_pipeline_layout);
		vk_pipeline_layout = VK_NULL_HANDLE;
	}
}

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

	depth_attachment_ref = vk::AttachmentReference()
									.setAttachment(1).setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

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

	try
	{
		vk_render_pass = device->vk_device.createRenderPass(vk_render_pass_info);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Create Render Pass:\n{}", err.what()));
	}
}

void RenderPass::Cleanup()
{
	if (vk_render_pass && device && device->vk_device)
	{
		device->vk_device.destroyRenderPass(vk_render_pass);
		vk_render_pass = VK_NULL_HANDLE;
	}
}

vk::DescriptorSet GetDescriptorSet(Device* device, DescriptorPool* pool, DescriptorSetLayout* layout)
{
	return VK_NULL_HANDLE;
}

}	 // namespace nft::vulkan
