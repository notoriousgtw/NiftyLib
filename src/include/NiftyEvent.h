#pragma once

namespace Nifty::Observe
{
class Event
{
  public:
	virtual ~Event();

	using EventType = std::string;

	virtual EventType GetType() const = 0;
};

class MouseClickEvent: public Event
{
  public:
	MouseClickEvent();
	virtual ~MouseClickEvent();

	EventType type = "MouseClick";
	EventType GetType() const override { return type; }
}
}	 // namespace Nifty::Observe