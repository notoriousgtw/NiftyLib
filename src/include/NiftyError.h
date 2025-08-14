#pragma once

#include "NiftyApp.h"
#include "NiftyErrorBase.h"
#include "NiftyLog.h"

#include <memory>
#include <string>
#include <vector>

namespace nft
{

#define NFT_ERROR(Err, Msg) ErrorHandler::Error<Err>(Msg, __func__)
#define NFT_REGISTER_ERROR(Err) ErrorHandler::Register<Err>()
// #define NFT_WARN(Msg) ErrorHandler::Error<Warning>(Msg, __func__)
// #define NFT_ERROR(Msg) ErrorHandler::Error<Error>(Msg, __func__)
// #define NFT_FATAL(Msg) ErrorHandler::Error<FatalError>(Msg, __func__)

class ErrorHandler
{
  public:
	ErrorHandler()	= delete;
	~ErrorHandler() = delete;

	static void Init(App* app);

	template<typename T, typename... Args>
	static void Error(Args&&... args)
	{
		auto error = std::make_unique<T>(std::forward<Args>(args)...);

		std::string extra;
		if (!error->function_name.empty())
			extra = error->GetCode() + "->" + error->function_name;
		else
			extra = error->GetCode();

		switch (error->type)
		{
		case ErrorType::Warning: app->GetLogger()->Warn(error->message, extra); break;
		case ErrorType::Error: app->GetLogger()->Error(error->message, extra); break;
		case ErrorType::Fatal:
			app->GetLogger()->Fatal(error->message, extra);
			std::exit(EXIT_FAILURE);	// Immediately terminate for fatal errors
		}
	}

	template<typename E>
	static void Register()
	{
		//static_assert(std::is_base_of<Warning, E>::value || std::is_base_of<nft::Error, E>::value || std::is_base_of<FatalError, E>::value,
		//			  "E must derive from Error");
		const std::string code = E::GetCode();
		if (!error_codes.insert(code).second)
		{
			NFT_ERROR(DuplicateErrorCodeFatal, "Duplicate error code registered: " + code);
		}
		app->GetLogger()->Debug("Registered Error: \"" + code + "\"", "ErrorHandler");
	}

  private:
	static App*							   app;
	static std::unordered_set<std::string> error_codes;
};
}	 // namespace nft