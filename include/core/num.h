#pragma once
#include <cmath>
#include <sstream>
#include <string>
#include <glm/glm.hpp>

namespace nft::Num
{

enum class Axis2D
{
	x,
	y
};

inline double ChordLength(double radius, double distance) { return 2 * sqrt((2 * radius * distance) - pow(distance, 2)); }
double ChordLength(double x_radius, double y_radius, double distance, Axis2D axis);
glm::dvec2	  GetChord(double x_radius, double y_radius, double distance, Axis2D axis);
inline glm::dvec2 GetChord(double radius, double distance) { return GetChord(radius, radius, distance, Axis2D::x); };

struct Precision
{
	enum class Power2
	{
		P2	 = 1,
		P4	 = 2,
		P8	 = 3,
		P16	 = 4,
		P32	 = 5,
		P64	 = 6,
		P128 = 7
	};

	const Power2 precision_power;

	Precision(int precision_power): precision_power(static_cast<Power2>(precision_power)) {}
	constexpr Precision(Power2 precision_power = Power2::P64): precision_power(precision_power) {}
	constexpr operator Power2() const { return precision_power; }
	constexpr operator int() const { return static_cast<int>(precision_power); }

	operator std::string() const { return ToString(); }

	operator const char*() const noexcept
	{
		static thread_local std::string cachedString;
		cachedString = ToString();
		return cachedString.c_str();
	}

	std::string ToString() const
	{
		static const std::string str[] { "2", "4", "8", "16", "32", "64", "128" };

		return str[static_cast<int>(precision_power) - 1];
	}
};

class MixedNumber
{
  public:
	int whole;
	int numerator;
	int denominator;

	MixedNumber(double num, Precision precision_power = Precision::Power2::P64)
	{
		if (num == 0)
		{
			whole		= 0;
			numerator	= 0;
			denominator = 1;
			return;
		}

		bool   is_neg  = num < 0;
		double abs_num = abs(num);

		int	   best_numerator	= 0;
		int	   best_denominator = 1;
		double min_error		= 100;

		whole			 = std::floor(num);
		int	   abs_whole = abs(whole);
		double remainder = abs_num - abs_whole;

		if (remainder == 0)
		{
			numerator	= 0;
			denominator = 1;
			return;
		}

		for (int power = 1; power <= static_cast<int>(precision_power); power++)
		{
			int	   denominator_candidate	 = pow(2, power);
			double numerator_candidate_float = remainder * denominator_candidate;

			int	   rounded_numerator = static_cast<int>(round(numerator_candidate_float));
			double current_error	 = abs(numerator_candidate_float - rounded_numerator) / denominator_candidate;

			if (current_error < min_error)
			{
				min_error		 = current_error;
				best_numerator	 = rounded_numerator;
				best_denominator = denominator_candidate;
			}

			if (current_error == 0) break;
		}

		if (is_neg) best_numerator *= -1;

		numerator	= best_numerator;
		denominator = best_denominator;
	}

	MixedNumber(int whole): whole(whole), numerator(0), denominator(1) {}
	MixedNumber(int numerator, int denominator): whole(0), numerator(numerator), denominator(denominator) {}
	MixedNumber(int whole, int numerator, int denominator): whole(whole), numerator(numerator), denominator(denominator)
	{
	}

	operator std::string() const { return ToString(); }

	operator const char*() const noexcept
	{
		static thread_local std::string cachedString;
		cachedString = ToString();
		return cachedString.c_str();
	}

	std::string ToString() const
	{
		std::ostringstream oss;
		oss << whole << " " << numerator << "/" << denominator;
		return oss.str();
	}
};
}	 // namespace nft::Num