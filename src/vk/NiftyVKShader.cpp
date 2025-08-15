//=============================================================================
// VULKAN SHADER IMPLEMENTATION
//=============================================================================
// This file implements the Vulkan shader module management, including
// shader module creation and cleanup.

#include "vk/NiftyVKShader.h"

#include "core/NiftyApp.h"
#include "core/NiftyError.h"
#include "vk/NiftyVK.h"

namespace nft::vulkan
{

//=============================================================================
// CONSTRUCTOR & DESTRUCTOR
//=============================================================================

Shader::Shader(Device* device, ShaderCode code) : device(device), code(code)
{
	// Validate input parameters
	if (!device)
		NFT_ERROR(VKFatal, "Device Is Null!");
	if (!code.data || code.size == 0)
		NFT_ERROR(VKFatal, "Shader Code Is Null Or Empty!");

	// Initialize references
	app		 = device->GetApp();
	instance = device->GetInstance();

	// Create shader module info structure
	vk_shader_module_info = vk::ShaderModuleCreateInfo()
								.setFlags(vk::ShaderModuleCreateFlags())
								.setCodeSize(code.size)
								.setPCode(code.data);

	// Create the shader module
	try
	{
		vk_shader_module = device->GetDevice().createShaderModule(
			vk_shader_module_info, nullptr, instance->GetDispatchLoader());
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKFatal, std::format("Failed To Create Shader Module:\n{}", err.what()));
	}
}

Shader::~Shader()
{
	// Clean up the shader module
	device->GetDevice().destroyShaderModule(vk_shader_module, nullptr, instance->GetDispatchLoader());
}

}	 // namespace nft::vulkan