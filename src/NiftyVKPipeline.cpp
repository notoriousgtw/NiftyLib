#include "NiftyVKPipeline.h"

namespace nft::Vulkan
{
Pipeline::Pipeline()
{
	// Initialize the pipeline
	Create();
}

void Pipeline::Create()
{
	std::println("Creating vulkan pipeline...");
	std::println("Vertex shader code size: {}", simple_shader_vert_len);
	std::println("Fragment shader code size: {}", simple_shader_frag_len);
}
}