#pragma once

#include <NiftySVG.h>
#include <NiftyStik.h>
#include <NiftyStikSVGTemplates.h>
#include <map>

using namespace Nifty::Stik;

namespace Nifty
{
namespace SVG
{
class Fretboard
{
  public:
	Fretboard();
	void Update(double scale_length,
				int	   fret_count,
				double fret_width,
				double fret_height,
				double scallop_depth,
				bool   show_scallop);
	int	 GetTextureID() { return svg_handler.texture_id; };
	int	 GetWidth() { return svg_handler.width; };
	int	 GetHeight() { return svg_handler.height; };

  private:
	double		scale_length  = 25.5;
	int			fret_count	  = 24;
	double		fret_width	  = 0.115;
	double		fret_height	  = 0.053;
	double		scallop_depth = 0.0625;
	bool		show_scallop  = false;
	std::string svg_data;
	Handler		svg_handler;

	std::string CalcFretData();
	std::string CalcClipData();
	std::string CalcScallopClipHead();
	std::string CalcScallopClipBody();
	std::string CalcScallopClipTail();
	std::string CalcShowClips();
};
}	 // namespace SVG
namespace Scallop
{
class FretMap;

class FretData
{
  public:
	FretData() = delete;
	FretData(FretMap* fret_map, size_t fret_number);

	inline size_t GetFretNumber() const { return fret_number; }
	inline double GetFretLength() const { return fret_length; }

  private:
	FretMap* fret_map;
	size_t	 fret_number;
	double	 fret_length = 0.0;
	void	 SetFretLength();
};

typedef std::map<int, FretData> FretDataMap;

class FretMap
{
  public:
	inline size_t	   GetFretCount() const { return fret_count; }
	inline size_t	   GetFretStart() const { return fret_start; }
	inline size_t	   GetFretEnd() const { return fret_end; }
	inline double	   GetScaleLength() const { return scale_length; }
	inline double	   GetFretWidth() const { return fret_width; }
	inline double	   GetFretHeight() const { return fret_height; }
	inline double	   GetScallopDepth() const { return scallop_depth; }
	inline FretDataMap GetFretDataMap() const { return fret_data_map; }

  private:
	size_t		fret_count;
	size_t		fret_start;
	size_t		fret_end;
	double		scale_length;
	double		fret_width;
	double		fret_height;
	double		scallop_depth;
	FretDataMap fret_data_map;
};
}	 // namespace Scallop
}	 // namespace Nifty