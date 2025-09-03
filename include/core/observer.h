#pragma once

#include "core/error_base.h"
#include "core/event_base.h"
#include "core/util.h"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_set>
#include <vector>

// Forward declarations to break circular dependencies
namespace nft::vulkan
{
class Window;
}

namespace nft
{
class App;
}

namespace nft::Event
{

class Observer;

class EventHandler
{
  public:
	EventHandler() = default;
	EventHandler(nft::vulkan::Window* window);
	~EventHandler() = default;

	static void Init(nft::App* app);

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

	nft::vulkan::Window* GetWindow() { return window; };

  private:
	nft::vulkan::Window*							  window = nullptr;
	std::multimap<std::type_index, Observer*> event_register;
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

class WindowResizeHandler: public Observer
{
  public:
	typedef std::function<void(int width, int height)> ResizeCallback;
	WindowResizeHandler(EventHandler* handler): Observer(handler) { Subscribe<WindowResizeEvent>(); }
	~WindowResizeHandler() override = default;
	void Update(IEvent* source) override
	{
		if (auto event = dynamic_cast<WindowResizeEvent*>(source))
		{
			if (resize_callback)
				resize_callback(event->width, event->height);
		}
	}
	void SetResizeCallback(ResizeCallback callback) { resize_callback = callback; }
  private:
	ResizeCallback resize_callback = nullptr;
};

class KeyHandler: public Observer
{
  public:
	typedef std::function<void(Key key, Action action, ModifierFlags mods)> KeyCallback;
	typedef std::map<Key, Action>											KeyStateMap;

	KeyHandler(EventHandler* handler): Observer(handler)
	{
		Subscribe<KeyEvent>();
		InitKeyStates();
	}
	~KeyHandler() override = default;

	void Update(IEvent* source) override
	{
		if (auto event = dynamic_cast<KeyEvent*>(source))
		{
			key_states[event->key] = event->action;
			if (key_handler_callback)
				key_handler_callback(event->key, event->action, event->mods);
		}
	}

	void SetKeyHandlerCallback(KeyCallback callback) { key_handler_callback = callback; }

	const KeyStateMap GetKeyStates(std::vector<Key> keys)
	{
		KeyStateMap states;
		for (auto key : keys)
			states[key] = key_states[key];
		return states;
	}
	const Action GetKeyState(Key key) { return key_states[key]; }

  private:
	KeyStateMap key_states			 = {};
	KeyCallback key_handler_callback = nullptr;

	inline void InitKeyStates()
	{
		if (key_states.empty())
			for (auto key : GetEnumRange(Key::D0, Key::F25))
				key_states[key] = Action::Unknown;
	}
};

class MouseHandler: public Observer
{
  public:
	typedef std::function<void(MouseButton button, Action action, ModifierFlags mods)> MouseButtonCallback;
	typedef std::function<void(glm::vec2 pos)>										   MouseMoveCallback;
	typedef std::map<MouseButton, Action>											   MouseButtonStateMap;

	MouseHandler(EventHandler* handler);
	~MouseHandler() override = default;

	void Update(IEvent* source) override
	{
		if (auto event = dynamic_cast<MouseButtonEvent*>(source))
		{
			button_states[event->button] = event->action;
			if (button_handler_callback)
				button_handler_callback(event->button, event->action, event->mods);
		}
		else if (auto event = dynamic_cast<MouseMoveEvent*>(source))
		{
			last_mouse_pos = event->pos;
			if (move_handler_callback)
				move_handler_callback(event->pos);
		}
	}

	void SetMouseMoveHandlerCallback(MouseMoveCallback callback) { move_handler_callback = callback; }

	glm::vec2 GetLastMousePos() const { return last_mouse_pos; }

	void EnableRawMouseMotion();
	void DisableRawMouseMotion();
	void SetCursorDisabled();
	void SetCursorHidden();
	void SetCursorNormal();

	void SetButtonHandlerCallback(MouseButtonCallback callback) { button_handler_callback = callback; }

	MouseButtonStateMap GetButtonStates(std::vector<MouseButton> buttons)
	{
		MouseButtonStateMap states;
		for (auto button : buttons)
			states[button] = button_states[button];
		return states;
	}
	Action GetButtonState(MouseButton button) { return button_states[button]; }

  private:
	glm::dvec2			last_mouse_pos			= glm::dvec2(0.0f);
	MouseButtonStateMap button_states			= {};
	MouseButtonCallback button_handler_callback = nullptr;
	MouseMoveCallback	move_handler_callback	= nullptr;
	const bool			raw_mouse_supported;

	inline void InitButtonStates()
	{
		if (button_states.empty())
			for (auto button : GetEnumRange(MouseButton::Left, MouseButton::Button8))
				button_states[button] = Action::Unknown;
	}
};

}	 // namespace nft::Event