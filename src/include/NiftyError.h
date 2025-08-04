#pragma once

#include "NiftyUtil.h"
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

	static void Init(std::string name);

	// Method to add an Error object to the list
	template<typename T, typename... Args>
	static void Error(Args&&... args)
	{
		auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
		errors.push_back(std::move(ptr));
	}
	static void Warn(std::string message) { Error<Warning>(message); };

	// Method to retrieve all stored errors
	// const std::vector<std::unique_ptr<Error>>& GetErrors() const;
  private:
	// List to store objects derived from Error
	static std::vector<std::unique_ptr<nft::ErrorBase>> errors;
	static Logger									logger;
};
}	 // namespace nft