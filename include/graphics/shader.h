#pragma once

#include "vk/pipeline.h"
#include "vk/shader.h"

namespace nft::graphics
{
class ShaderComponentBase
{
  public:
	ShaderComponentBase(vulkan::Shader::ShaderCode code, vulkan::Shader::ShaderType type): shader_code(code), shader_type(type) {}

	virtual vulkan::DescriptorSetLayout GetDescriptorSet() = 0;

  private:
	vulkan::Shader::ShaderCode shader_code;
	vulkan::Shader::ShaderType shader_type;
};

class SimpleVertexShaderComponent : public ShaderComponentBase
{
  public:
	SimpleVertexShaderComponent(vulkan::Shader::ShaderCode code):
		ShaderComponentBase(code, vulkan::ShaderType::Vertex) {}
	~SimpleVertexShaderComponent() override = default;
	vulkan::DescriptorSetLayout GetDescriptorSet() override
	{
		vulkan::DescriptorSetLayout layout;
		layout.Init(device, {});
		return vulkan::DescriptorSetLayout();
	}

}	 // namespace nft::graphics