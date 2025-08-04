#pragma once

namespace nft
{

template<typename TYPE>
class Enum
{
  private:
	TYPE value_;

  public:
	explicit constexpr Enum(TYPE value): value_(value) {}
	constexpr Enum()							 = default;
	~Enum()									 = default;
	constexpr explicit Enum(const Enum&)	 = default;
	constexpr Enum& operator=(const Enum&) = default;

	constexpr	   operator TYPE() const { return value_; }
	constexpr TYPE value() const { return value_; }
};
}	 // namespace nft