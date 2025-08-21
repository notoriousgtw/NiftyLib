#pragma once

#include <GLFW/glfw3.h>
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_set>

#if defined(__GNUG__)
#include <cstdlib>
#include <cxxabi.h>
#endif

namespace nft
{

// class App;

class IEventBase
{
  public:
	virtual ~IEventBase() = default;
	virtual std::string GetCode() const { return code; }
	// virtual void		Notify()		= 0;
	static std::string code;
};

template<typename Derived>
struct IEvent: public IEventBase
{
  public:
	static std::string GetCode()
	{
		std::string name;
		if (!name.empty())
			return name;	// Return cached name if already set
#if defined(__GNUG__)
		int	  status	= 0;
		char* demangled = abi::__cxa_demangle(typeid(Derived).name(), nullptr, nullptr, &status);
		if (status == 0 && demangled)
		{
			name = demangled;
			std::free(demangled);
		}
		else
		{
			name = typeid(Derived).name();
		}
#else
		name = typeid(Derived).name();
#endif
		// Remove namespaces
		size_t pos = name.rfind("::");
		if (pos != std::string::npos)
		{
			name = name.substr(pos + 2);
		}
		// Remove "struct " or "class " prefix if present
		const std::string struct_prefix = "struct ";
		const std::string class_prefix	= "class ";
		if (name.compare(0, struct_prefix.size(), struct_prefix) == 0)
		{
			name = name.substr(struct_prefix.size());
		}
		else if (name.compare(0, class_prefix.size(), class_prefix) == 0)
		{
			name = name.substr(class_prefix.size());
		}
		code = name;
		return code;
	}
};

struct KeyEvent: public IEvent<KeyEvent>
{
	enum class Key
	{
		Unknown		 = GLFW_KEY_UNKNOWN,
		D0			 = GLFW_KEY_0,
		D1			 = GLFW_KEY_1,
		D2			 = GLFW_KEY_2,
		D3			 = GLFW_KEY_3,
		D4			 = GLFW_KEY_4,
		D5			 = GLFW_KEY_5,
		D6			 = GLFW_KEY_6,
		D7			 = GLFW_KEY_7,
		D8			 = GLFW_KEY_8,
		D9			 = GLFW_KEY_9,
		Num0		 = GLFW_KEY_KP_0,
		Num1		 = GLFW_KEY_KP_1,
		Num2		 = GLFW_KEY_KP_2,
		Num3		 = GLFW_KEY_KP_3,
		Num4		 = GLFW_KEY_KP_4,
		Num5		 = GLFW_KEY_KP_5,
		Num6		 = GLFW_KEY_KP_6,
		Num7		 = GLFW_KEY_KP_7,
		Num8		 = GLFW_KEY_KP_8,
		Num9		 = GLFW_KEY_KP_9,
		NumAdd		 = GLFW_KEY_KP_ADD,
		NumMinus	 = GLFW_KEY_KP_SUBTRACT,
		NumMult		 = GLFW_KEY_KP_MULTIPLY,
		NumDivide	 = GLFW_KEY_KP_DIVIDE,
		NumDecimal	 = GLFW_KEY_KP_DECIMAL,
		NumEnter	 = GLFW_KEY_KP_ENTER,
		Space		 = GLFW_KEY_SPACE,
		Tab			 = GLFW_KEY_TAB,
		Grave		 = GLFW_KEY_GRAVE_ACCENT,
		Minus		 = GLFW_KEY_MINUS,
		Equal		 = GLFW_KEY_EQUAL,
		Period		 = GLFW_KEY_PERIOD,
		RightBracket = GLFW_KEY_RIGHT_BRACKET,
		LeftBracket	 = GLFW_KEY_LEFT_BRACKET,
		Backslash	 = GLFW_KEY_BACKSLASH,
		Semicolon	 = GLFW_KEY_SEMICOLON,
		Apostrophe	 = GLFW_KEY_APOSTROPHE,
		Comma		 = GLFW_KEY_COMMA,
		Period		 = GLFW_KEY_PERIOD,
		Slash		 = GLFW_KEY_SLASH,
		A			 = GLFW_KEY_A,
		B			 = GLFW_KEY_B,
		C			 = GLFW_KEY_C,
		D			 = GLFW_KEY_D,
		E			 = GLFW_KEY_E,
		F			 = GLFW_KEY_F,
		G			 = GLFW_KEY_G,
		H			 = GLFW_KEY_H,
		I			 = GLFW_KEY_I,
		J			 = GLFW_KEY_J,
		K			 = GLFW_KEY_K,
		L			 = GLFW_KEY_L,
		M			 = GLFW_KEY_M,
		N			 = GLFW_KEY_N,
		O			 = GLFW_KEY_O,
		P			 = GLFW_KEY_P,
		Q			 = GLFW_KEY_Q,
		R			 = GLFW_KEY_R,
		S			 = GLFW_KEY_S,
		T			 = GLFW_KEY_T,
		U			 = GLFW_KEY_U,
		V			 = GLFW_KEY_V,
		W			 = GLFW_KEY_W,
		X			 = GLFW_KEY_X,
		Y			 = GLFW_KEY_Y,
		Z			 = GLFW_KEY_Z,
		Escape		 = GLFW_KEY_ESCAPE,
		Enter		 = GLFW_KEY_ENTER,
		Backspace	 = GLFW_KEY_BACKSPACE,
		Insert		 = GLFW_KEY_INSERT,
		Delete		 = GLFW_KEY_DELETE,
		LeftAlt		 = GLFW_KEY_LEFT_ALT,
		RightAlt	 = GLFW_KEY_RIGHT_ALT,
		LeftControl	 = GLFW_KEY_LEFT_CONTROL,
		RightControl = GLFW_KEY_RIGHT_CONTROL,
		LeftShift	 = GLFW_KEY_LEFT_SHIFT,
		RightShift	 = GLFW_KEY_RIGHT_SHIFT,
		LeftSuper	 = GLFW_KEY_LEFT_SUPER,
		RightSuper	 = GLFW_KEY_RIGHT_SUPER,
		Right		 = GLFW_KEY_RIGHT,
		Left		 = GLFW_KEY_LEFT,
		Up			 = GLFW_KEY_UP,
		Down		 = GLFW_KEY_DOWN,
		PageUp		 = GLFW_KEY_PAGE_UP,
		PageDown	 = GLFW_KEY_PAGE_DOWN,
		CapsLock	 = GLFW_KEY_CAPS_LOCK,
		NumLock		 = GLFW_KEY_NUM_LOCK,
		ScrollLock	 = GLFW_KEY_SCROLL_LOCK,
		F1			 = GLFW_KEY_F1,
		F2			 = GLFW_KEY_F2,
		F3			 = GLFW_KEY_F3,
		F4			 = GLFW_KEY_F4,
		F5			 = GLFW_KEY_F5,
		F6			 = GLFW_KEY_F6,
		F7			 = GLFW_KEY_F7,
		F8			 = GLFW_KEY_F8,
		F9			 = GLFW_KEY_F9,
		F10			 = GLFW_KEY_F10,
		F11			 = GLFW_KEY_F11,
		F12			 = GLFW_KEY_F12,
		F13			 = GLFW_KEY_F13,
		F14			 = GLFW_KEY_F14,
		F15			 = GLFW_KEY_F15,
		F16			 = GLFW_KEY_F16,
		F17			 = GLFW_KEY_F17,
		F18			 = GLFW_KEY_F18,
		F19			 = GLFW_KEY_F19,
		F20			 = GLFW_KEY_F20,
		F21			 = GLFW_KEY_F21,
		F22			 = GLFW_KEY_F22,
		F23			 = GLFW_KEY_F23,
		F24			 = GLFW_KEY_F24,
		F25			 = GLFW_KEY_F25
	};

	enum class Action
	{
		Press	= 0,
		Release = 1,
		Repeat	= 2
	};

	enum class Modifier
	{
		Shift	= 0x0001,
		Control = 0x0002,
		Alt		= 0x0004,
		Super	= 0x0008
	};

	KeyEvent(int key, int scancode, int action, int mods): IEvent(), key(key), scancode(scancode), action(action), mods(mods) {};

	int key;
	int scancode;
	int action;
	int mods;
};

struct MouseButtonEvent: public IEvent<MouseButtonEvent>
{
	MouseButtonEvent(int button, int action, int mods): IEvent(), button(button), action(action), mods(mods) {};
	int button;
	int action;
	int mods;
};

struct MouseMoveEvent: public IEvent<MouseMoveEvent>
{
	MouseMoveEvent(double x, double y): IEvent(), x(x), y(y) {};
	double x;
	double y;
};

}	 // namespace nft
