#pragma once

#include "core/error_base.h"
#include "core/event_base.h"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_set>
#include <vector>

#include "core/glfw_common.h"

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

	const std::unordered_map<KeyEvent::Key, KeyEvent::Action>* GetKeyStates() { return &key_states; }
	KeyEvent::Action										   GetKeyState(KeyEvent::Key key)
	{
		auto it = key_states.find(key);
		if (it == key_states.end())
			return KeyEvent::Action::Unknown;
		else
			return it->second;
	}
	void SetKeyState(KeyEvent::Key key, KeyEvent::Action action) { key_states[key] = action; }

	template<typename Event>
	void Attach(Observer* observer)
	{
		event_register.insert({ std::type_index(typeid(Event)), observer });
	}

	template<typename Event>
	void Detach(Observer* observer)
	{
		auto range = event_register.equal_range(std::type_index(typeid(Event)));

		for (auto it = range.first; it != range.second;)
		{
			if (it->second == observer)
				it = event_register.erase(it);	  // Erase and update iterator
			else
				++it;	 // Move to the next element
		}
	}

	template<typename Event, typename... Args>
	void Notify(Args&&... args)
	{
		auto event = std::make_shared<Event>(std::forward<Args>(args)...);

		auto range = event_register.equal_range(std::type_index(typeid(Event)));

		for (auto it = range.first; it != range.second; it++)
			it->second->Update(event.get());
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
	std::multimap<std::type_index, Observer*>			event_register;
	std::unordered_map<KeyEvent::Key, KeyEvent::Action> key_states;
	// static std::unordered_set<std::string> event_codes;
};

class Observer
{
  public:
	Observer(EventHandler* handler);
	virtual ~Observer() = default;

	virtual void Update(IEvent* source) = 0;

	template<typename E>
	void Subscribe()
	{
		event_handler->Attach<E>(this);
	}

	EventHandler* event_handler = nullptr;
};

}	 // namespace nft