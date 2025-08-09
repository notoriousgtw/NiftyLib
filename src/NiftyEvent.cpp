#include "NiftyEvent.h"
#include "NiftyEventBase.h"

#include <thread>

namespace nft
{
//DECLARE_EVENT_CODE(WindowEvent, "WindowEvent")
//DECLARE_EVENT_CODE(KeyPressEvent, "KeyPressEvent")

//std::unordered_set<std::shared_ptr<Observer>>  EventHandler::observers;
//std::mutex									   EventHandler::observers_mutex;
//std::unordered_set<std::unique_ptr<EventBase>> EventHandler::events;
//App*										   EventHandler::app = nullptr;

//void EventHandler::Init(App* app)
//{
//	EventHandler::app = app;
//	std::thread(
//		[app]()
//		{
//			while (true)
//			{
//				// Loop logic here
//				for (auto& event : events)
//				{
//					// Process each event
//					if (event->Watch())
//					{
//						for (auto& observer : observers)
//						{
//							if (observer->CheckEvent(event.get()))
//								observer->TriggerCallback();
//						}
//					}
//				}
//
//				// Check for errors once a second
//				std::this_thread::sleep_for(std::chrono::seconds(1));
//			}
//		})
//		.detach();
//}
}	 // namespace nft