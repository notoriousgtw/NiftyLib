#pragma once

#include <set>
#include <sstream>
#include <string>

namespace Nifty::SVG
{

class Element
{
  public:
	Image*		image;
	std::string tag;
	size_t		indent;
	virtual void Calc() = 0;
};

class Group: Element
{
};

class Path: Element
{
  public:
	Path();

};

class ClipPath
{
  public:
	ClipPath() { GenId(); };
	bool		operator<(const ClipPath& other) const { return id < other.id; };
	std::string Calc();

  private:
	std::string	 id;
	virtual void GenId() = 0;
};

}	 // namespace Nifty::SVG
