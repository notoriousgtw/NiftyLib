#pragma once

#include <string>

#if defined(__GNUG__)
#include <cstdlib>
#include <cxxabi.h>
#endif

namespace nft
{

//template<typename T>
//class Enum
//{
//  private:
//	T value;
//
//  public:
//	explicit constexpr Enum(T value): value(value) {}
//	constexpr Enum()					   = default;
//	~Enum()								   = default;
//	constexpr explicit Enum(const Enum&)   = default;
//	constexpr Enum& operator=(const Enum&) = default;
//
//	constexpr	operator T() const { return value; }
//	constexpr T value() const { return value; }
//};



template<typename Derived>
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
	return name;
}

template<typename E>
constexpr auto ToUnderlying(E e) noexcept
{
	return static_cast<std::underlying_type_t<E>>(e);
}

template<typename E>
class EnumIterator
{
	int value;

  public:
	explicit EnumIterator(int v): value(v) {}
	E			  operator*() const { return static_cast<E>(value); }
	EnumIterator& operator++()
	{
		++value;
		return *this;
	}
	bool operator!=(const EnumIterator& other) const { return value != other.value; }
};

template<typename E>
class EnumRange
{
	int begin_value, end_value;

  public:
	EnumRange(E begin, E end): begin_value(ToUnderlying(begin)), end_value(ToUnderlying(end)) {}
	EnumIterator<E> begin() const { return EnumIterator<E>(begin_value); }
	EnumIterator<E> end() const { return EnumIterator<E>(end_value + 1); }
};

template<typename E>
EnumRange<E> GetEnumRange(E begin, E end)
{
	return EnumRange<E>(begin, end);
}

}	 // namespace nft