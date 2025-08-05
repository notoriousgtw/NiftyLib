#pragma once

// #include "NiftyUtil.h"
#include "NiftyApp.h"
#include "NiftyErrorBase.h"
#include "NiftyLog.h"

#include <memory>
#include <string>
#include <vector>

namespace nft
{

class ErrorHandler
{
  public:
	ErrorHandler()	= delete;
	~ErrorHandler() = delete;

	static void Init(App* app);

	// Method to add an Error object to the list
	template<typename T, typename... Args>
	static void Error(Args&&... args)
	{
		auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
		errors.push_back(std::move(ptr));
	}
	static void Warn(std::string message) { Error<Warning>(message); };
	static void Register(const std::string& code)
	{
		static std::unordered_set<std::string> codes;
		static std::mutex					   mtx;
		std::lock_guard<std::mutex>			   lock(mtx);
		if (!codes.insert(code).second)
		{
			//app->GetLogger()->Fatal("Code: \"" + code + "\"",
			//						DuplicateErrorCodeError::StaticCode());
			//std::exit(EXIT_FAILURE);
			Error<DuplicateErrorCodeError>("Code: \"" + code + "\"");
		}
		app->GetLogger()->Debug("Registered code: \"" + code + "\"", "ErrorHandler");
	}

	// Method to retrieve all stored errors
	// const std::vector<std::unique_ptr<Error>>& GetErrors() const;
  private:
	// List to store objects derived from Error
	static std::vector<std::unique_ptr<ErrorBase>> errors;
	static App*											app;
};
}	 // namespace nft