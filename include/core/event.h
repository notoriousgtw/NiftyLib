#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

#include "core/app.h"
#include "core/error.h"

namespace nft
{

class Event;

class Observer
{
  public:
	virtual ~Observer()	  = default;
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

}	 // namespace nft