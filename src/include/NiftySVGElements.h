#pragma once

#include <any>
#include <functional>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>

namespace Nifty::SVG
{

//enum class Color
//{
//	none,
//	red,
//	green,
//	blue,
//	white,
//	black
//};
//
//const std::map<Color, std::string> Colors = { { Color::none, "none" },	 { Color::red, "red" },
//											  { Color::green, "green" }, { Color::blue, "blue" },
//											  { Color::white, "white" }, { Color::black, "black" } };
//
//class Element
//{
//  public:
//	Image*		 image;
//	virtual void Calc() = 0;
//
//  private:
//	std::string element_data;
//	bool		editing = false;
//	size_t		indent;
//};
//
//class StyledElement: Element
//{
//  public:
//	virtual void Calc() = 0;
//
//  private:
//	Color		stroke		 = Color::black;
//	double		stroke_width = 1;
//	std::string style		 = R"SVG(
//	style="
//		fill: none;
//		stroke: black;
//		stroke-width: 1;
//	"
//)SVG";
//};
//
//class Path: StyledElement
//{
//  public:
//	Path() {};
//	void Calc() override {};
//
//  private:
//	std::string open = "<path";
//	std::string body;
//	std::string path_data = R"SVG(
//	d=\"
//DATA
//	"
//)SVG";
//};
//
//class ClipPath
//{
//  public:
//	ClipPath() { GenId(); };
//	bool		operator<(const ClipPath& other) const { return id < other.id; };
//	std::string Calc();
//
//  private:
//	std::string	 id;
//	virtual void GenId() = 0;
//};
//
}	 // namespace Nifty::SVG
