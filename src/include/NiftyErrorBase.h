#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

// #include "NiftyApp.h"

namespace nft
{

class ErrorHandler;

class App;

struct ErrorBase
{
  public:
	enum class Type : uint8_t
	{
		Warning,
		Error,
		Fatal
	};

	// template<typename T, typename... Args>
	// static T* Create(Args&&... args)
	//{
	//	auto ptr	= std::make_unique<T>(std::forward<Args>(args)...);
	//	T*	 rawPtr = ptr.get();
	//	ErrorHandler::Error(std::move(ptr));
	//	return rawPtr;
	// }

	ErrorBase(std::string message, std::string function_name, Type type):
		message(message), function_name(function_name), type(type) {};
	void				Init() { GetCode(); }
	virtual std::string GetCode() const = 0;

	std::string function_name;
	std::string message;
	Type		type;
};

//#define DEFINE_ERROR(CLASS, BASE)                                                   \
//	struct CLASS: public BASE                                                       \
//	{                                                                               \
//	  public:                                                                       \
//		CLASS(std::string message): Error(std::move(message)) { Init(); }           \
//		static const std::string& StaticCode();                                     \
//		std::string				  GetCode() const override { return StaticCode(); } \
//	};

//#define DEFINE_ERROR(CLASS, BASE, ...)                                             \
//	struct CLASS: public BASE                                                       \
//	{                                                                               \
//	  public:                                                                       \
//		CLASS(std::string message, ): Error(std::move(message)) { Init(); }     \
	//	static const std::string& StaticCode();                                     \
	//	std::string				  GetCode() const override { return StaticCode(); } \
	//};

//#define DEFINE_ERROR(CLASS, ARGS)                                                   \
//	CLASS(std::string message, ARGS): Error(std::move(message)) { Init(); } \
//	static const std::string& StaticCode();                                              \
//	std::string				  GetCode() const override { return StaticCode(); }

#define DEFINE_ERROR(CLASS, BASE, ...)                                              \
	struct CLASS: public BASE                                                       \
	{                                                                               \
	  public:                                                                       \
		CLASS(std::string message, std::string function_name = "", ##__VA_ARGS__):  \
			BASE(std::move(message), std::move(function_name))                      \
		{                                                                           \
			Init();                                                                 \
		}                                                                           \
		static const std::string& StaticCode();                                     \
		std::string				  GetCode() const override { return StaticCode(); } \
	};

#define DEFINE_ERROR_CODE()                 \
	static const std::string& StaticCode(); \
	std::string				  GetCode() const override { return StaticCode(); }

#define DECLARE_ERROR_CODE(CLASS, CODE)             \
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

struct Warning: public ErrorBase
{
	Warning(std::string message, std::string function_name = ""):
		ErrorBase(std::move(message), std::move(function_name), Type::Warning)
	{
		Init();
	};
	DEFINE_ERROR_CODE()
};

struct Error: public ErrorBase
{
	Error(std::string message, std::string function_name = ""):
		ErrorBase(std::move(message), std::move(function_name), Type::Error)
	{
		Init();
	};
	DEFINE_ERROR_CODE()
};

struct FatalError: public ErrorBase
{
	FatalError(std::string message, std::string function_name = ""):
		ErrorBase(std::move(message), std::move(function_name), Type::Fatal)
	{
		Init();
	};
	DEFINE_ERROR_CODE()
};

DEFINE_ERROR(DuplicateErrorCodeError, FatalError)
}	 // namespace nft
