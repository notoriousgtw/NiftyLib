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

class ErrorHandler;

class App;

enum class ErrorType : uint8_t
{
	Warning,
	Error,
	Fatal
};

template<typename Derived>
struct IError
{
  public:
	IError(std::string message, std::string function_name, ErrorType type):
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
struct Warning: public IError<Derived>
{
	Warning(std::string message, std::string function_name = ""):
		IError<Derived>(std::move(message), std::move(function_name), ErrorType::Warning) {};
};

template<typename Derived>
struct Error: public IError<Derived>
{
	Error(std::string message, std::string function_name = ""):
		IError<Derived>(std::move(message), std::move(function_name), ErrorType::Error) {};
};

template<typename Derived>
struct FatalError: public IError<Derived>
{
	FatalError(std::string message, std::string function_name = ""):
		IError<Derived>(std::move(message), std::move(function_name), ErrorType::Fatal) {};
};

struct DuplicateErrorCodeFatal: public FatalError<DuplicateErrorCodeFatal>
{
	DuplicateErrorCodeFatal(std::string message, std::string function_name = ""):
		FatalError(std::move(message), std::move(function_name)) {};
};

struct DuplicateEventCodeFatal: public FatalError<DuplicateEventCodeFatal>
{
	DuplicateEventCodeFatal(std::string message, std::string function_name = ""):
		FatalError(std::move(message), std::move(function_name)) {};
};

struct GLFWWarning: public Warning<GLFWWarning>
{
	GLFWWarning(std::string message, std::string function_name = ""): Warning(std::move(message), std::move(function_name)) {};
};

struct GLFWError: public Error<GLFWError>
{
	GLFWError(std::string message, std::string function_name = ""): Error(std::move(message), std::move(function_name)) {};
};

struct GLFWFatal: public FatalError<GLFWFatal>
{
	GLFWFatal(std::string message, std::string function_name = ""): FatalError(std::move(message), std::move(function_name)) {};
};

struct VKWarning: public Warning<VKWarning>
{
	VKWarning(std::string message, std::string function_name = ""): Warning(std::move(message), std::move(function_name)) {};
};

struct VKError: public Error<VKError>
{
	VKError(std::string message, std::string function_name = ""): Error(std::move(message), std::move(function_name)) {};
};

struct VKFatal: public FatalError<VKFatal>
{
	VKFatal(std::string message, std::string function_name = ""): FatalError(std::move(message), std::move(function_name)) {};
};

struct FileError: public Error<FileError>
{
	FileError(std::string message, std::string function_name = ""): Error(std::move(message), std::move(function_name)) {};
};

struct ColorEncodingError: public Error<ColorEncodingError>
{
	ColorEncodingError(std::string message, std::string function_name = ""):
		Error(std::move(message), std::move(function_name)) {};
};
}	 // namespace nft
