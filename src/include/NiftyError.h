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
		static_assert(std::is_base_of<ErrorBase<E>, E>::value, "E must derive from ErrorBase");
		const std::string code = E::GetCode();
		if (!error_codes.insert(code).second)
		{
			NFT_ERROR(DuplicateErrorCodeError, "Duplicate error code registered: " + code);
		}
		app->GetLogger()->Debug("Registered Error: \"" + code + "\"", "ErrorHandler");
	}

  private:
	static App*							   app;
	static std::unordered_set<std::string> error_codes;
};
}	 // namespace nft