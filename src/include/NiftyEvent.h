#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_set>

#include "NiftyApp.h"
#include "NiftyError.h"
#include "NiftyEventBase.h"

namespace nft
{

class Observer
{
  public:
	using Callback = std::function<void(void)>;
	Observer(EventBase* event, Callback callback): event(event) {};

	void SetCallback(Callback callback)
	{
		std::lock_guard<std::mutex> lock(callback_mutex);
		this->callback = callback;
	}

	void TriggerCallback()
	{
		std::lock_guard<std::mutex> lock(callback_mutex);
		if (callback) { callback(); }
	}

	bool CheckEvent(EventBase* event) { return this->event == event; }

  private:
	EventBase* event;
	Callback   callback;
	std::mutex callback_mutex;
};

class EventHandler
{
  public:
	EventHandler()	= delete;
	~EventHandler() = delete;

	static void Init(App* app);

	template<typename T, typename... Args>
	static void Observe(Args... args)
	{
		auto ptr		= std::make_unique<T>(std::forward<Args>(args)...);
		auto event_pair = events.emplace(std::move(ptr));

		if (!event_pair.second)
		{
			ptr.reset();
			return;
		}

		T*							event	 = event_pair.first->get();
		auto						observer = std::make_shared<Observer>(event);
		std::lock_guard<std::mutex> lock(observers_mutex);
		observers.insert(observer);
	}

	static void Register(const std::string& code)
	{
		static std::unordered_set<std::string> codes;
		static std::mutex					   mtx;
		std::lock_guard<std::mutex>			   lock(mtx);
		if (!codes.insert(code).second)
		{
			ErrorHandler::Error<DuplicateEventCodeError>("Code: \"" + code + "\"");
		}
		app->GetLogger()->Debug("Registered code: \"" + code + "\"", "EventHandler");
	}

  private:
	static std::unordered_set<std::shared_ptr<Observer>>  observers;
	static std::mutex									  observers_mutex;
	static std::unordered_set<std::unique_ptr<EventBase>> events;
	static App*											  app;
};

// Static member definitions

}	 // namespace nft