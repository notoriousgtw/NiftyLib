#include "core/event.h"

#include "core/error.h"

#include <thread>

namespace nft
{
//std::unordered_set<std::string> EventHandler::event_codes;
Observer::Observer(EventHandler* event_handler)
{
	if (event_handler)
		this->event_handler = event_handler;
	else
		NFT_ERROR(EventFatal, "EventHandler is null!");
}
}	 // namespace nft