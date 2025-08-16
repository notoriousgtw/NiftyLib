#include "core/num.h"

namespace nft::Num
{

double ChordLength(double x_radius, double y_radius, double distance, Axis2D axis)
{
	if (axis == Axis2D::x) { return ((2 * y_radius) / x_radius) * sqrt(pow(x_radius, 2) - pow(distance, 2)); }
	else { return ((2 * x_radius) / y_radius) * sqrt(pow(y_radius, 2) - pow(distance, 2)); }
}

glm::dvec2 GetChord(double x_radius, double y_radius, double distance, Axis2D axis)
{
	glm::dvec2 chord; 
	if (axis == Axis2D::x)
	{
		chord.x = -((y_radius / x_radius) * sqrt(pow(x_radius, 2) - pow(distance, 2)));
		chord.y = abs(chord.x);
	}
	else
	{
		chord.x = -((x_radius / y_radius) * sqrt(pow(y_radius, 2) - pow(distance, 2)));
		chord.y = abs(chord.x);
	}
	return chord;
}
}	 // namespace nft::Num