#pragma once

#include <cmath>

namespace Nifty::Stik
{
inline double FretCalculator(double scale_length, int fret_number)
{
	return scale_length - (scale_length / pow(2.0, fret_number / 12.0));
}
}
