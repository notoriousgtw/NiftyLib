#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

#include <GLFW/glfw3.h>

//#include "core/app.h"
//#include "core/error.h"

namespace nft
{

class Event;

class Observer
{
  public:
	virtual ~Observer()				   = default;
	virtual void Update(Event* source) = 0;
};

class Event
{
  public:
	void Attach(std::shared_ptr<Observer> observer) { observers.push_back(observer); }
	void Detach(std::shared_ptr<Observer> observer)
	{
		observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
	}
	void Notify()
	{
		for (auto& observer : observers)
			if (observer)
				observer->Update(this);
	}

  private:
	std::vector<std::shared_ptr<Observer>> observers;
};

class KeyPressEvent: public Event
{
  public:
	bool key_states[GLFW_KEY_LAST + 1];
};

}	 // namespace nft