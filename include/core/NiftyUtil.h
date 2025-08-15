#pragma once

namespace nft
{

template<typename T>
class Enum
{
  private:
	T value;

  public:
	explicit constexpr Enum(T value): value(value) {}
	constexpr Enum()							 = default;
	~Enum()									 = default;
	constexpr explicit Enum(const Enum&)	 = default;
	constexpr Enum& operator=(const Enum&) = default;

	constexpr	   operator T() const { return value; }
	constexpr T value() const { return value; }
};
}	 // namespace nft