#include "core/error.h"

#include "core/error_base.h"

#include <thread>

namespace nft
{
std::unordered_set<std::string> ErrorHandler::error_codes;

App*										 ErrorHandler::app = nullptr;

void ErrorHandler::Init(App* app)
{
	app->GetLogger()->Debug("Registering System Error Codes...", "ErrorHandler");
	ErrorHandler::app = app;
	NFT_REGISTER_ERROR(DuplicateErrorCodeFatal);
	NFT_REGISTER_ERROR(GLFWWarning);
	NFT_REGISTER_ERROR(GLFWError);
	NFT_REGISTER_ERROR(GLFWFatal);
	NFT_REGISTER_ERROR(VulkanWarning);
	NFT_REGISTER_ERROR(VulkanError);
	NFT_REGISTER_ERROR(VulkanFatal);
	NFT_REGISTER_ERROR(FileError);
	NFT_REGISTER_ERROR(ColorEncodingError);
	app->GetLogger()->Debug("System Error Codes Successfully Registered!", "ErrorHandler");
}
}	 // namespace nft