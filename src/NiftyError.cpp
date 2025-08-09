#include "NiftyError.h"
#include "NiftyErrorBase.h"
#include "NiftyColor.h"
#include "NiftyVK.h"
#include "NiftyEvent.h"

#include <thread>

namespace nft
{
std::unordered_set<std::string> ErrorHandler::error_codes;

App*										 ErrorHandler::app = nullptr;

void ErrorHandler::Init(App* app)
{
	app->GetLogger()->Debug("Registering System Error Codes...", "ErrorHandler");
	ErrorHandler::app = app;
	Register<DuplicateErrorCodeError>();
	Register<GLFWFatal>();
	//Register<DuplicateEventCodeError>();
	Register<ColorEncodingError>();
	Register<Vulkan::VKInitFatal>();
	app->GetLogger()->Debug("System Error Codes Successfully Registered!", "ErrorHandler");
}
}	 // namespace nft