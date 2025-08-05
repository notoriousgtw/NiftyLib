#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

#include "NiftyError.h"

namespace nft
{
DEFINE_ERROR(DuplicateEventCodeError, FatalError)

struct EventBase
{
  public:
	EventBase(std::string message): message(message) {};

	virtual std::string GetCode() const = 0;
	virtual bool		Watch()			= 0;

	std::string message;

  private:
};

#define DEFINE_EVENT(CLASS, BASE, ...)                                                  \
	struct CLASS: public BASE                                                           \
	{                                                                                   \
	  public:                                                                           \
		CLASS(std::string message, ##__VA_ARGS__): BASE(std::move(message)) { Init(); } \
		static const std::string& StaticCode();                                         \
		std::string				  GetCode() const override { return StaticCode(); }     \
	};

#define DEFINE_EVENT_CODE()                 \
	static const std::string& StaticCode(); \
	std::string				  GetCode() const override { return StaticCode(); }

#define DECLARE_EVENT_CODE(CLASS, CODE)             \
	const std::string& CLASS::StaticCode()          \
	{                                               \
		static const std::string code		= CODE; \
		static const bool		 registered = []()  \
		{                                           \
			ErrorHandler::Register(code);           \
			return true;                            \
		}();                                        \
		(void)registered;                           \
		return code;                                \
	}

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

class WindowEvent: public EventBase
{
	WindowEvent(std::string message): EventBase(std::move(message)) {}
	virtual bool Watch() = 0;
	DEFINE_EVENT_CODE()
};

class KeyPressEvent: public EventBase
{
	KeyPressEvent(std::string message): EventBase(std::move(message)) {}
	virtual bool Watch() = 0;
	DEFINE_EVENT_CODE()
};
}	 // namespace nft