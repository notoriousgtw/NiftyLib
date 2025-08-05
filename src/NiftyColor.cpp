#include "NiftyColor.h"

#include "NiftyError.h"

namespace nft
{
DECLARE_ERROR_CODE(ColorEncodingError, "ColorEncodingError")
//  Define static members outside the class
// std::expected<Color, Error> Color::RGBA(float r, float g, float b, float a)
//{
//	if ((a >= 0 || a < 1) || (r >= 0 || r < 1) || (g >= 0 || g < 1) || (b >= 0 || b < 1))
//	{
//		return std::unexpected(
//			nft::Error(ErrorCode::ColorEncodingError, "Invalid RGBA values. Values must be between 0 and 1."));
//	}
//	else
//		return Color(Encoding::RGBA, r, g, b, a);
// }
//
// std::expected<Color, Error> Color::ARGB(float a, float r, float g, float b)
//{
//	if ((a >= 0 || a < 1) || (r >= 0 || r < 1) || (g >= 0 || g < 1) || (b >= 0 || b < 1))
//	{
//		return std::unexpected(
//			nft::Error(ErrorCode::ColorEncodingError, "Invalid ARGB values. Values must be between 0 and 1."));
//	}
//	else
//		return Color(Encoding::ARGB, a, r, g, b);
// }
//
// std::expected<Color, Error> Color::RGB(float r, float g, float b)
//{
//	if ((r >= 0 || r < 1) || (g >= 0 || g < 1) || (b >= 0 || b < 1))
//	{
//		return std::unexpected(
//			nft::Error(ErrorCode::ColorEncodingError, "Invalid RGB values. Values must be between 0 and 1."));
//	}
//	else
//		return Color(Encoding::RGB, r, g, b);
// }
}	 // namespace nft