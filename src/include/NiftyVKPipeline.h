#pragma once

#include <../generated/simple_shader.frag.spv.h>
#include <../generated/simple_shader.vert.spv.h>
#include <print>

namespace nft::Vulkan
{
class Pipeline
{
  public:
	Pipeline();

  private:
	void Create();
};
}	 // namespace nft::VK