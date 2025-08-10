#include "NiftyVKShader.h"

#include "NiftyApp.h"
#include "NiftyError.h"
#include "NiftyVK.h"

namespace nft::Vulkan
{
Shader::Shader(Device* device, ShaderCode code): device(device), code(code)
{
	if (!device)
		NFT_ERROR(VKInitFatal, "Device Is Null!");
	if (!code.data || code.size == 0)
		NFT_ERROR(VKInitFatal, "Shader Code Is Null Or Empty!");
	app		 = this->device->app;
	instance = this->device->instance;

	create_info = vk::ShaderModuleCreateInfo().setFlags(vk::ShaderModuleCreateFlags()).setCodeSize(code.size).setPCode(code.data);

	try
	{
		vk_shader_module = device->vk_device.createShaderModule(create_info, nullptr, instance->dispatch_loader_dynamic);
	}
	catch (const vk::SystemError& err)
	{
		NFT_ERROR(VKInitFatal, std::format("Failed To Create Shader Module: \n{}", err.what()));
	}
}
Shader::~Shader()
{
	device->vk_device.destroyShaderModule(vk_shader_module, nullptr, instance->dispatch_loader_dynamic);
}
}	 // namespace nft::Vulkan