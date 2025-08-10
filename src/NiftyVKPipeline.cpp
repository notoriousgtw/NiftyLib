#include "NiftyVKPipeline.h"

#include "NiftyApp.h"
#include "NiftyError.h"
#include "NiftyVK.h"

namespace nft::Vulkan
{
Pipeline::Pipeline(Device* device)
{
	if (!device)
		NFT_ERROR(VKInitFatal, "Device is null!");
	this->device = device;
	app			 = this->device->app;
	Init();
}

void Pipeline::Init() {}
}	 // namespace nft::Vulkan