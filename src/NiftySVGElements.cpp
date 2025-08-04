#include <NiftySVGElements.h>

namespace nft::SVG
{

//void StyledElement::Style(std::optional<Color>& fill, std::optional<Color>& stroke, std::optional<double>& stroke_width)
//{
//	if (fill.has_value())
//	{
//		this->fill = fill.value();
//		style	   = std::regex_replace(style, std::regex("(fill: )(.*)(?=;)"), "$1" + Colors.at(this->fill));
//	}
//
//	if (stroke.has_value())
//	{
//		this->stroke = stroke.value();
//		style		 = std::regex_replace(style, std::regex("(stroke: )(.*)(?=;)"), "$1" + Colors.at(this->stroke));
//	}
//
//	if (stroke_width.has_value())
//	{
//		this->stroke_width = stroke_width.value();
//		style			   = std::regex_replace(
//			 style, std::regex("(stroke-width: )(.*)(?=;)"), "$1" + std::to_string(this->stroke_width));
//	}
//}
}	 // namespace nft::SVG