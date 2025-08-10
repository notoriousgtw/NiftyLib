#pragma once

#include "NiftyVKCommon.h"

#include "NiftyVKShader.h"

namespace nft::Vulkan
{
class Instance;
class Device;
class Swapchain;

class Pipeline
{
  public:
	Pipeline(Device* device);

	void Init();

	App*	  app	   = nullptr;
	Instance* instance = nullptr;
	Device*	  device   = nullptr;

	std::unique_ptr<Shader> vertex_shader;
	std::unique_ptr<Shader> geometry_shader;
	std::unique_ptr<Shader> fragment_shader;
};
}	 // namespace nft::Vulkan