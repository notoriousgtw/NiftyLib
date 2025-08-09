#pragma once

#include <memory>
#include <string>
#include <unordered_set>
#include <typeinfo>

#if defined(__GNUG__)
#include <cxxabi.h>
#include <cstdlib>
#endif

// #include "NiftyApp.h"

namespace nft
{

class ErrorHandler;

class App;

enum class ErrorType : uint8_t
{
	Warning,
	Error,
	Fatal
};

template<typename Derived>
struct ErrorBase
{
  public:
	ErrorBase(std::string message, std::string function_name, ErrorType type):
		message(message), function_name(function_name), type(type) {};
	static std::string GetCode()
	{
		std::string name;
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
		return name;
	}

	std::string function_name;
	std::string message;
	ErrorType	type;
};

//#define DEFINE_ERROR(CLASS, BASE, ...)                                             \
//	struct CLASS: public BASE                                                      \
//	{                                                                              \
//	  public:                                                                      \
//		CLASS(std::string message, std::string function_name = "", ##__VA_ARGS__): \
//			BASE(std::move(message), std::move(function_name))                     \
//		{                                                                          \
//			Init();                                                                \
//		}                                                                          \
//		static const std::string& StaticCode();                                    \
//		std::string				  GetCode() const override                         \
//		{                                                                          \
//			return StaticCode();                                                   \
//		}                                                                          \
//	};
//
// #define DEFINE_ERROR_CODE()                            \
//	static const std::string& StaticCode();            \
//	std::string				  GetCode() const override \
//	{                                                  \
//		return StaticCode();                           \
//	}
//
// #define DECLARE_ERROR_CODE(CLASS)               \
//	const std::string& CLASS::StaticCode()      \
//	{                                           \
//		static const std::string code = #CLASS; \
//		return code;                            \
//	}
//
// #define REGISTER_ERROR(CLASS) ErrorHandler::Register<CLASS>()

template<typename Derived>
struct Warning: public ErrorBase<Derived>
{
	Warning(std::string message, std::string function_name = ""):
		ErrorBase<Derived>(std::move(message), std::move(function_name), ErrorType::Warning) {};
};

template<typename Derived>
struct Error: public ErrorBase<Derived>
{
	Error(std::string message, std::string function_name = ""):
		ErrorBase<Derived>(std::move(message), std::move(function_name), ErrorType::Error) {};
};

template<typename Derived>
struct FatalError: public ErrorBase<Derived>
{
	FatalError(std::string message, std::string function_name = ""):
		ErrorBase<Derived>(std::move(message), std::move(function_name), ErrorType::Fatal) {};
};

struct DuplicateErrorCodeError: public FatalError<DuplicateErrorCodeError>
{
	DuplicateErrorCodeError(std::string message, std::string function_name = ""):
		FatalError(std::move(message), std::move(function_name)) {};
};

struct GLFWFatal: public FatalError<GLFWFatal>
{
	GLFWFatal(std::string message, std::string function_name = ""): FatalError(std::move(message), std::move(function_name)) {};
};
}	 // namespace nft
