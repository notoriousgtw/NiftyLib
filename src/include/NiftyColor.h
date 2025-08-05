#pragma once

#include <algorithm>
#include <expected>
#include <glm/vec4.hpp>
#include <vector>
#include <string>
#include <memory>

#include "NiftyErrorBase.h"

namespace nft
{
//struct ColorEncodingError: public Error
//{
//  public:
//	ColorEncodingError(std::string message): Error(std::move(message)) { Init(); }
//	DEFINE_ERROR_CODE()
//};
DEFINE_ERROR(ColorEncodingError, Error)

// struct ColorData
//{
// enum class Encoding
//{
//	Console,
//	RGBA
// };
//
// constexpr ColorData(uint8_t r, uint8_t g, uint8_t b, uint8_t a): encoding(Encoding::RGBA)
//{
//	color = (static_cast<unsigned int>(r) << 24) | (static_cast<unsigned int>(g) << 16) |
//			(static_cast<unsigned int>(b) << 8) | static_cast<unsigned int>(a);
// }
// constexpr ColorData(uint32_t color, Encoding encoding = Encoding::RGBA): color(color), encoding(encoding) {};
// Encoding encoding;
// uint32_t color;
// };

class Color
{
  public:
	enum class Encoding
	{
		Console,
		RGBA
	};
	explicit constexpr Color(uint32_t data, Encoding encoding = Encoding::RGBA): data(data), encoding(encoding) {}
	explicit constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255): encoding(Encoding::RGBA)
	{
		data = (static_cast<unsigned int>(r) << 24) | (static_cast<unsigned int>(g) << 16) |
				(static_cast<unsigned int>(b) << 8) | static_cast<unsigned int>(a);
	}
	constexpr Color(): Color(0, 0, 0, 255) {}

	uint32_t data;
	Encoding encoding;
};

namespace Pallete
{
namespace Default
{
static const Color Red	   = Color(255, 0, 0);
static const Color Green   = Color(0, 255, 0);
static const Color Blue	   = Color(0, 0, 255);
static const Color Yellow  = Color(255, 255, 0);
static const Color Cyan	   = Color(0, 255, 255);
static const Color Magenta = Color(255, 0, 255);
static const Color Black   = Color(0, 0, 0);
static const Color White   = Color(255, 255, 255);
static const Color Gray	   = Color(127, 127, 127);

static const std::vector<Color> AllDefaultColors = { Red, Green, Blue, Yellow, Cyan, Magenta, Black, White, Gray };
}	 // namespace Default
namespace Console
{
static const Color BlackFG	 = Color(30, Color::Encoding::Console);
static const Color BlackBG	 = Color(40, Color::Encoding::Console);
static const Color RedFG	 = Color(31, Color::Encoding::Console);
static const Color RedBG	 = Color(41, Color::Encoding::Console);
static const Color GreenFG	 = Color(32, Color::Encoding::Console);
static const Color GreenBG	 = Color(42, Color::Encoding::Console);
static const Color YellowFG	 = Color(33, Color::Encoding::Console);
static const Color YellowBG	 = Color(43, Color::Encoding::Console);
static const Color BlueFG	 = Color(34, Color::Encoding::Console);
static const Color BlueBG	 = Color(44, Color::Encoding::Console);
static const Color MagentaFG = Color(35, Color::Encoding::Console);
static const Color MagentaBG = Color(45, Color::Encoding::Console);
static const Color CyanFG	 = Color(36, Color::Encoding::Console);
static const Color CyanBG	 = Color(46, Color::Encoding::Console);
static const Color WhiteFG	 = Color(37, Color::Encoding::Console);
static const Color WhiteBG	 = Color(47, Color::Encoding::Console);
static const Color DefaultFG = Color(39, Color::Encoding::Console);
static const Color DefaultBG = Color(49, Color::Encoding::Console);

static const std::vector<Color> AllConsoleColors = { BlackFG,  BlackBG,	 RedFG,	  RedBG,   GreenFG,	  GreenBG,
													 YellowFG, YellowBG, BlueFG,  BlueBG,  MagentaFG, MagentaBG,
													 CyanFG,   CyanBG,	 WhiteFG, WhiteBG, DefaultFG, DefaultBG };
}	 // namespace Console
namespace Dracula
{
static const Color Background  = Color(40, 42, 54);
static const Color CurrentLine = Color(68, 71, 90);
static const Color Selection   = Color(68, 71, 90);
static const Color Foreground  = Color(248, 248, 242);
static const Color Comment	   = Color(98, 114, 164);
static const Color Cyan		   = Color(139, 233, 253);
static const Color Green	   = Color(80, 250, 123);
static const Color Orange	   = Color(255, 184, 108);
static const Color Pink		   = Color(255, 121, 198);
static const Color Purple	   = Color(189, 147, 249);
static const Color Red		   = Color(255, 85, 85);
static const Color Yellow	   = Color(241, 250, 140);

static const std::vector<Color> AllColors = { Background, CurrentLine, Selection, Foreground, Comment, Cyan,
											  Green,	  Orange,	   Pink,	  Purple,	  Red,	   Yellow };
}	 // namespace Dracula
}	 // namespace Pallete

}	 // namespace nft