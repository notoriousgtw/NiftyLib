#pragma once

#include "core/glfw_common.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_set>

namespace nft::Event
{

// class App;
class EventHandler;

enum class Key : int32_t
{
	D0,
	D1,
	D2,
	D3,
	D4,
	D5,
	D6,
	D7,
	D8,
	D9,
	Num0,
	Num1,
	Num2,
	Num3,
	Num4,
	Num5,
	Num6,
	Num7,
	Num8,
	Num9,
	NumAdd,
	NumMinus,
	NumMult,
	NumDivide,
	NumDecimal,
	NumEnter,
	Space,
	Tab,
	Grave,
	Minus,
	Equal,
	RightBracket,
	LeftBracket,
	Backslash,
	Semicolon,
	Apostrophe,
	Comma,
	Period,
	Slash,
	A,
	B,
	C,
	D,
	E,
	F,
	G,
	H,
	I,
	J,
	K,
	L,
	M,
	N,
	O,
	P,
	Q,
	R,
	S,
	T,
	U,
	V,
	W,
	X,
	Y,
	Z,
	Escape,
	Enter,
	Backspace,
	Insert,
	Delete,
	LeftAlt,
	RightAlt,
	LeftControl,
	RightControl,
	LeftShift,
	RightShift,
	LeftSuper,
	RightSuper,
	Right,
	Left,
	Up,
	Down,
	PageUp,
	PageDown,
	CapsLock,
	NumLock,
	ScrollLock,
	F1,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	F10,
	F11,
	F12,
	F13,
	F14,
	F15,
	F16,
	F17,
	F18,
	F19,
	F20,
	F21,
	F22,
	F23,
	F24,
	F25,
	Unknown = -1
};

enum class MouseButton : int32_t
{
	Left = 1,
	Right,
	Middle,
	Button4,
	Button5,
	Button6,
	Button7,
	Button8,
	Unknown = -1
};

enum class Action : int32_t
{
	Press	= 1,
	Release = 0,
	Repeat	= 2,
	Unknown = -1
};

enum class Modifier : uint32_t
{
	Null	 = 0x0000,
	Shift	 = 0x0001,
	Control	 = 0x0002,
	Alt		 = 0x0004,
	Super	 = 0x0008,
	CapsLock = 0x0010,
	NumLock	 = 0x0020
};

struct ModifierFlags
{
	ModifierFlags() = default;
	ModifierFlags(Modifier flags): flags(static_cast<uint32_t>(flags)) {}
	ModifierFlags(uint32_t flags): flags(flags) {}

	inline ModifierFlags& operator=(Modifier rhs)
	{
		flags = static_cast<uint32_t>(rhs);
		return *this;
	}

	inline Modifier		 operator&(Modifier rhs) const { return static_cast<Modifier>(flags & static_cast<uint32_t>(rhs)); }
	inline ModifierFlags operator|(Modifier rhs) const { return ModifierFlags { flags | static_cast<uint32_t>(rhs) }; }

  private:
	uint32_t flags = 0;
};

// inline uint32_t operator&(ModifierFlags lhs, Modifier rhs)
//{
//	return static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs);
// }
//
// inline ModifierFlags operator|(ModifierFlags lhs, Modifier rhs)
//{
//	return static_cast<ModifierFlags>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
// }

Key			  GetKey(int32_t key);
int32_t		  GetGLFWKey(Key key);
MouseButton	  GetMouseButton(int32_t button);
int32_t		  GetGLFWMouseButton(MouseButton button);
Action		  GetAction(int32_t action);
int32_t		  GetGLFWAction(Action action);
ModifierFlags GetModifiers(uint32_t mods);
uint32_t	  GetGLFWModifiers(ModifierFlags mods);

class IEvent
{
  public:
	IEvent(EventHandler* event_handler): event_handler(event_handler) {};
	virtual ~IEvent() = default;
	// virtual void		Notify()		= 0;
	EventHandler* event_handler = nullptr;
};

struct KeyEvent: public IEvent
{
	KeyEvent(EventHandler* event_handler, int32_t GLFW_key, int32_t GLFW_action, uint32_t GLFW_mods);

	Key			  key	 = Key::Unknown;
	Action		  action = Action::Unknown;
	ModifierFlags mods	 = Modifier::Null;

  private:
};

struct MouseButtonEvent: public IEvent
{
	MouseButtonEvent(EventHandler* event_handler, int32_t GLFW_button, int32_t GLFW_action, uint32_t GLFW_mods);

	MouseButton	  button = MouseButton::Unknown;
	Action		  action = Action::Unknown;
	ModifierFlags mods	 = Modifier::Null;
};

struct MouseMoveEvent: public IEvent
{
	MouseMoveEvent(EventHandler* event_handler, double GLFW_x, double GLFW_y);
	glm::dvec2 pos;
};
}	 // namespace nft::Event
