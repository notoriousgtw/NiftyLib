#include "NiftyError.h"
#include "NiftyErrorBase.h"

#include <thread>

namespace nft
{
DECLARE_ERROR_CODE(Warning, "Warning")
DECLARE_ERROR_CODE(Error, "Error")
DECLARE_ERROR_CODE(FatalError, "FatalError")
DECLARE_ERROR_CODE(DuplicateErrorCodeError, "DuplicateErrorCodeError")

std::vector<std::unique_ptr<nft::ErrorBase>> ErrorHandler::errors;
App*										 ErrorHandler::app = nullptr;

void ErrorHandler::Init(App* app)
{
	ErrorHandler::app = app;
	std::thread(
		[app]()
		{
			while (true)
			{
				// Loop logic here
				for (auto it = errors.begin(); it != errors.end();)
				{
					// Process each error
					if (*it)
					{
						std::string extra;
						if (!(*it)->function_name.empty())
							extra =
								(*it)->GetCode() + "->" + (*it)->function_name;
						else
							extra = (*it)->GetCode();
						switch ((*it)->type)
						{
						case ErrorBase::Type::Warning:
							app->GetLogger()->Warn((*it)->message, extra);
							break;
						case ErrorBase::Type::Error:
							app->GetLogger()->Error((*it)->message, extra);
							break;
						case ErrorBase::Type::Fatal:
							app->GetLogger()->Fatal((*it)->message, extra);
							std::exit(EXIT_FAILURE);
						}
						it = errors.erase(it);	  // Remove the error from the list after handling
					}
					else { ++it; }
				}

				// Check for errors once a second
				//std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		})
		.detach();
}
}	 // namespace nft