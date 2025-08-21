#pragma once

#include "core/error_base.h"
#include "core/event_base.h"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

#include <GLFW/glfw3.h>

// #include "core/app.h"
// #include "core/error.h"

namespace nft
{

// #define NFT_ERROR(Err, Msg) ErrorHandler::Error<Err>()
// #define NFT_REGISTER_ERROR(Err) ErrorHandler::Register<Err>()
class Observer;

class EventHandler
{
  public:
	EventHandler()	= default;
	~EventHandler() = default;

	static void Init(App* app);

	template<typename E>
	void Attach(Observer* observer)
	{
		event_register.insert({ E::GetCode(), observer });
	}

	template<typename E>
	void Detach(Observer* observer)
	{
		auto range = event_register.equal_range(E::GetCode());

		for (auto it = range.first; it != range.second;)
		{
			if (it->second == observer)
				it = event_register.erase(it);	  // Erase and update iterator
			else
				++it;	 // Move to the next element
		}
	}

	template<typename T, typename... Args>
	void Notify(Args&&... args)
	{
		auto event = std::make_shared<T>(std::forward<Args>(args)...);

		auto range = event_register.equal_range(event->GetCode());

		for (auto it = range.first; it != range.second; it++)
		{
			auto observer = it->second;
			observer->Update(event.get());
		}
	}

	// template<typename E>
	// static void Register()
	//{
	//	// static_assert(std::is_base_of<Warning, E>::value || std::is_base_of<nft::Error, E>::value ||
	//	// std::is_base_of<FatalError, E>::value, 			  "E must derive from Error");
	//	const std::string code = E::GetCode();
	//	if (!event_codes.insert(code).second)
	//	{
	//		NFT_ERROR(DuplicateEventCodeFatal, "Duplicate event code registered: " + code);
	//	}
	//	app->GetLogger()->Debug("Registered Event: \"" + code + "\"", "");
	// }

  private:
	// static App*									 app;
	std::multimap<std::string, Observer*> event_register;
	// static std::unordered_set<std::string> event_codes;
};

class Observer
{
  public:
	virtual ~Observer() = default;

	virtual void Update(IEventBase* source) = 0;

	template<typename E>
	void Subscribe(EventHandler* handler)
	{
		handler->Attach<E>(this);
	}
};

}	 // namespace nft