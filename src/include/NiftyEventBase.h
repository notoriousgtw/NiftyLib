#pragma once

//#include <functional>
//#include <memory>
//#include <mutex>
//#include <string>
//#include <unordered_set>
//
//#include "NiftyError.h"
//
//namespace nft
//{
//
//template <typename Derived>
//struct EventBase
//{
//  public:
//	static std::string GetCode()
//	{
//		std::string name;
//#if defined(__GNUG__)
//		int	  status	= 0;
//		char* demangled = abi::__cxa_demangle(typeid(Derived).name(), nullptr, nullptr, &status);
//		if (status == 0 && demangled)
//		{
//			name = demangled;
//			std::free(demangled);
//		}
//		else
//		{
//			name = typeid(Derived).name();
//		}
//#else
//		name = typeid(Derived).name();
//#endif
//		// Remove namespaces
//		 size_t pos = name.rfind("::");
//		 if (pos != std::string::npos)
//		{
//			name = name.substr(pos + 2);
//		 }
//		// Remove "struct " or "class " prefix if present
//		 const std::string struct_prefix = "struct ";
//		 const std::string class_prefix	= "class ";
//		 if (name.compare(0, struct_prefix.size(), struct_prefix) == 0)
//		{
//			name = name.substr(struct_prefix.size());
//		 }
//		 else if (name.compare(0, class_prefix.size(), class_prefix) == 0)
//		{
//			name = name.substr(class_prefix.size());
//		 }
//		return name;
//	}
//	virtual bool		Watch()			= 0;
//};

//#define DEFINE_EVENT(CLASS, BASE, ...)                                                  \
//	struct CLASS: public BASE                                                           \
//	{                                                                                   \
//	  public:                                                                           \
//		CLASS(std::string message, ##__VA_ARGS__): BASE(std::move(message)) { Init(); } \
//		static const std::string& StaticCode();                                         \
//		std::string				  GetCode() const override { return StaticCode(); }     \
//	};
//
//#define DEFINE_EVENT_CODE()                 \
//	static const std::string& StaticCode(); \
//	std::string				  GetCode() const override { return StaticCode(); }
//
//#define DECLARE_EVENT_CODE(CLASS, CODE)             \
//	const std::string& CLASS::StaticCode()          \
//	{                                               \
//		static const std::string code		= CODE; \
//		return code;                                \
//	}

// Macro to define event code and register it
// #define DEFINE_EVENT_CODE(CLASS, CODE) \
//    static const std::string& StaticCode() { \
//        static const std::string code = CODE; \
//        static const bool registered = [](){ \
//            EventCodeRegistry::Register(code); \
//            return true; \
//        }(); \
//        (void)registered; \
//        return code; \
//    } \
//    std::string GetCode() const override { return StaticCode(); }

//class WindowEvent: public EventBase<WindowEvent>
//{
//	virtual bool Watch() = 0;
//};
//
//class KeyPressEvent: public EventBase<KeyPressEvent>
//{
//	virtual bool Watch() = 0;
//};
//}	 // namespace nft