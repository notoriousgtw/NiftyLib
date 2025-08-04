#include "NiftyError.h"

#include <thread>

namespace nft
{
std::vector<std::unique_ptr<nft::ErrorBase>> ErrorHandler::errors;
Logger										 ErrorHandler::logger;

void ErrorHandler::Init(std::string app_name)
{
	std::thread(
		[]()
		{
			while (true)
			{
				// Loop logic here
				for (auto it = errors.begin(); it != errors.end();)
				{
					// Process each error
					if (*it)
					{
						switch ((*it)->type)
						{
						case ErrorBase::Type::Warning: logger.Warn((*it)->message, (*it)->GetCode()); break;
						case ErrorBase::Type::Error: logger.Error((*it)->message, (*it)->GetCode()); break;
						case ErrorBase::Type::Fatal: logger.Fatal((*it)->message, (*it)->GetCode()); std::exit(EXIT_FAILURE);
						}
						it = errors.erase(it);	  // Remove the error from the list after handling
					}
					else { ++it; }
				}

				// Check for errors once a second
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		})
		.detach();
}
}	 // namespace nft