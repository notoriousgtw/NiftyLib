#pragma once

#include <memory>
#include <string>

namespace nft
{

class ErrorHandler;

struct ErrorBase
{
  public:
	enum class Type : uint8_t
	{
		Warning,
		Error,
		Fatal
	};

	//template<typename T, typename... Args>
	//static T* Create(Args&&... args)
	//{
	//	auto ptr	= std::make_unique<T>(std::forward<Args>(args)...);
	//	T*	 rawPtr = ptr.get();
	//	ErrorHandler::Error(std::move(ptr));
	//	return rawPtr;
	//}

	ErrorBase(std::string message, Type type): message(message), type(type) {};
	virtual std::string GetCode() const = 0;

	std::string message;
	Type		type;
};

struct Warning: public ErrorBase
{
	Warning(std::string message): ErrorBase(std::move(message), Type::Warning) {};
	std::string GetCode() const override { return ""; }
};

struct Error: public ErrorBase
{
	Error(std::string message): ErrorBase(std::move(message), Type::Error) {};
	std::string GetCode() const override { return ""; }
};


struct FatalError: public ErrorBase
{
	FatalError(std::string message): ErrorBase(std::move(message), Type::Fatal) {};
	std::string GetCode() const override { return ""; }
};

}	 // namespace nft