#pragma once

// #include <../generated/simple_shader.frag.spv.h>
// #include <../generated/simple_shader.vert.spv.h>
#include "NiftyVKCommon.h"

namespace nft
{
class App;
}

namespace nft::Vulkan
{
class Instance;
class Device;

class Shader
{
  public:
	struct ShaderCode
	{
		uint32_t*	 data;
		const size_t size;
	};
	Shader(Device* device, ShaderCode code);
	~Shader();

	App*	  app	   = nullptr;
	Instance* instance = nullptr;
	Device*	  device   = nullptr;

	ShaderCode				   code;
	vk::ShaderModule		   vk_shader_module;
	vk::ShaderModuleCreateInfo create_info;
};

}	 // namespace nft::Vulkan
