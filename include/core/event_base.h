#pragma once

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
	virtual ~IEventBase()				= default;
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
	KeyEvent(int key, int scancode, int action, int mods): IEvent(), key(key), scancode(scancode), action(action), mods(mods) {};

	int key;
	int scancode;
	int action;
	int mods;
};

}	 // namespace nft
