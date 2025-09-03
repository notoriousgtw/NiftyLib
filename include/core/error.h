#pragma once

#include "core/app.h"
#include "core/error_base.h"
#include "core/log.h"

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace nft
{

#define NFT_ERROR(Err, Msg) ErrorHandler::Error<Err>(Msg, __func__)
#define NFT_REGISTER_ERROR(Err) ErrorHandler::Register<Err>()

// Compile-time assertions
#define NFT_STATIC_ASSERT(condition, message) static_assert(condition, message)

// Type-checking compile-time assertions
#define NFT_STATIC_ASSERT_IS_SAME(T1, T2, message) static_assert(std::is_same_v<T1, T2>, message)

#define NFT_STATIC_ASSERT_IS_BASE_OF(Base, Derived, message) static_assert(std::is_base_of_v<Base, Derived>, message)

#define NFT_STATIC_ASSERT_IS_ARITHMETIC(T, message) static_assert(std::is_arithmetic_v<T>, message)

#define NFT_STATIC_ASSERT_IS_POINTER(T, message) static_assert(std::is_pointer_v<T>, message)

#define NFT_STATIC_ASSERT_IS_ENUM(T, message) static_assert(std::is_enum_v<T>, message)

#define NFT_STATIC_ASSERT_IS_TRIVIAL(T, message) static_assert(std::is_trivial_v<T>, message)

#define NFT_STATIC_ASSERT_IS_STANDARD_LAYOUT(T, message) static_assert(std::is_standard_layout_v<T>, message)

// Size and alignment assertions
#define NFT_STATIC_ASSERT_SIZE(T, expected_size, message) static_assert(sizeof(T) == expected_size, message)

#define NFT_STATIC_ASSERT_SIZE_MULTIPLE(T, multiple, message) static_assert(sizeof(T) % multiple == 0, message)

#define NFT_STATIC_ASSERT_ALIGNMENT(T, expected_alignment, message) static_assert(alignof(T) == expected_alignment, message)

// Concept-like assertions (C++20 and later)
// #if __cplusplus >= 202002L
#define NFT_STATIC_ASSERT_CONSTRUCTIBLE(T, Args, message) static_assert(std::is_constructible_v<T, Args>, message)

#define NFT_STATIC_ASSERT_COPY_CONSTRUCTIBLE(T, message) static_assert(std::is_copy_constructible_v<T>, message)

#define NFT_STATIC_ASSERT_MOVE_CONSTRUCTIBLE(T, message) static_assert(std::is_move_constructible_v<T>, message)

#define NFT_STATIC_ASSERT_DESTRUCTIBLE(T, message) static_assert(std::is_destructible_v<T>, message)
// #endif

// Value-based compile-time assertions
#define NFT_STATIC_ASSERT_GREATER(val1, val2, message) static_assert((val1) > (val2), message)

#define NFT_STATIC_ASSERT_LESS(val1, val2, message) static_assert((val1) < (val2), message)

#define NFT_STATIC_ASSERT_EQUAL(val1, val2, message) static_assert((val1) == (val2), message)

#define NFT_STATIC_ASSERT_NOT_EQUAL(val1, val2, message) static_assert((val1) != (val2), message)

#ifdef _DEBUG
#define NFT_ASSERT(condition, message)                                                            \
	do                                                                                            \
	{                                                                                             \
		if (!(condition))                                                                         \
		{                                                                                         \
			ErrorHandler::Error<AssertionFatal>(                                                  \
				std::string("Assertion Failed: ") + #condition +                                  \
					(message && strlen(message) > 0 ? std::string("\nMessage: ") + message : ""), \
				__func__);                                                                        \
		}                                                                                         \
	} while (0)

#define NFT_ASSERT_MSG(condition, message) NFT_ASSERT(condition, message)

#define NFT_ASSERT_EQ(expected, actual, message) \
	NFT_ASSERT((expected) == (actual),           \
			   std::string(message) + "\nExpected: " + std::to_string(expected) + ", Actual: " + std::to_string(actual))

#define NFT_ASSERT_NE(val1, val2, message) \
	NFT_ASSERT((val1) != (val2), std::string(message) + "\nValues should not be equal: " + std::to_string(val1))

#define NFT_ASSERT_LT(val1, val2, message) \
	NFT_ASSERT((val1) < (val2),            \
			   std::string(message) + "\n" + std::to_string(val1) + " should be less than " + std::to_string(val2))

#define NFT_ASSERT_GT(val1, val2, message) \
	NFT_ASSERT((val1) > (val2),            \
			   std::string(message) + "\n" + std::to_string(val1) + " should be greater than " + std::to_string(val2))

#define NFT_ASSERT_NULL(ptr, message) NFT_ASSERT((ptr) == nullptr, std::string(message) + "\nPointer should be null")

#define NFT_ASSERT_NOT_NULL(ptr, message) NFT_ASSERT((ptr) != nullptr, std::string(message) + "\nPointer should not be null")
#else
#define NFT_ASSERT(condition, message) ((void)0)
#define NFT_ASSERT_MSG(condition, message) ((void)0)
#define NFT_ASSERT_EQ(expected, actual, message) ((void)0)
#define NFT_ASSERT_NE(val1, val2, message) ((void)0)
#define NFT_ASSERT_LT(val1, val2, message) ((void)0)
#define NFT_ASSERT_GT(val1, val2, message) ((void)0)
#define NFT_ASSERT_NULL(ptr, message) ((void)0)
#define NFT_ASSERT_NOT_NULL(ptr, message) ((void)0)
#endif

class ErrorHandler
{
  public:
	ErrorHandler()	= delete;
	~ErrorHandler() = delete;

	static void Init(App* app);

	template<typename T, typename... Args>
	static void Error(Args&&... args)
	{
		auto error = std::make_unique<T>(std::forward<Args>(args)...);

		std::string extra;
		if (!error->function_name.empty())
			extra = error->GetCode() + "->" + error->function_name;
		else
			extra = error->GetCode();

		switch (error->type)
		{
		case ErrorType::Warning: app->GetLogger()->Warn(error->message, extra); break;
		case ErrorType::Error: app->GetLogger()->Error(error->message, extra); break;
		case ErrorType::Fatal:
			app->GetLogger()->Fatal(error->message, extra);
			std::exit(EXIT_FAILURE);	// Immediately terminate for fatal errors
		}
	}

	template<typename E>
	static void Register()
	{
		// static_assert(std::is_base_of<Warning, E>::value || std::is_base_of<nft::Error, E>::value ||
		// std::is_base_of<FatalError, E>::value, 			  "E must derive from Error");
		const std::string code = E::GetCode();
		if (!error_codes.insert(code).second)
		{
			NFT_ERROR(DuplicateErrorCodeFatal, "Duplicate error code registered: " + code);
		}
		app->GetLogger()->Debug("Registered Error: \"" + code + "\"", "ErrorHandler");
	}

  private:
	static App*							   app;
	static std::unordered_set<std::string> error_codes;
};
}	 // namespace nft