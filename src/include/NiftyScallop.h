#pragma once

#include <NiftyNum.h>
#include <NiftySVG.h>
#include <NiftyStik.h>
#include <NiftyStikSVGTemplates.h>
#include <map>
#include <set>
#include <vector>

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
	std::string CalcScallopClipPath();
	std::string CalcScallopClipHead();
	std::string CalcScallopClipBody();
	std::string CalcScallopClipTail();
};
}	 // namespace SVG

namespace Route
{

enum class RouterBit
{
	ThreeEighth,
	FiveSixteenth,
	ThreeSixteenth,
	Eighth
};

const std::map<RouterBit, double> router_bits = { { RouterBit::ThreeEighth, 0.375 },
												  { RouterBit::FiveSixteenth, 0.3132 },
												  { RouterBit::ThreeSixteenth, 0.1875 },
												  { RouterBit::Eighth, 0.125 } };

}	 // namespace Route

namespace Scallop
{

using namespace Nifty::Route;

enum class ScallopRouteOffset
{
	Quarter,
	Half,
	ThreeQuarter
};

const std::set<ScallopRouteOffset> scallop_route_offsets = { ScallopRouteOffset::Quarter,
															 ScallopRouteOffset::Half,
															 ScallopRouteOffset::ThreeQuarter };

struct ScallopRoute
{
	RouterBit		   bit;
	double			   depth;
	ScallopRouteOffset offset;
	double			   offset_dist;

	ScallopRoute() = delete;
	ScallopRoute(RouterBit bit, double depth, ScallopRouteOffset offset, double offset_dist):
		bit(bit), depth(depth), offset(offset), offset_dist(offset_dist)
	{
	}
};

class FretboardData;

class FretData
{
  public:
	FretData() = delete;
	FretData(FretboardData* fret_map, size_t fret_number);

	inline size_t GetFretNumber() const { return fret_number; }
	inline double GetFretLength() const { return fret_length; }

  private:
	FretboardData*			  fretboard_data;
	size_t					  fret_number;
	double					  fret_length = 0.0;
	std::vector<ScallopRoute> routes;
	void					  SetFretLength();
	void					  CalcRoutes();
};

typedef std::vector<FretData> FretDataVector;

class FretboardData
{
  public:
	FretboardData() = delete;
	FretboardData(double scale_length,
				  int	 fret_count,
				  int	 fret_start,
				  int	 fret_end,
				  double fret_width,
				  double fret_height,
				  double scallop_depth);

	inline double		  GetScaleLength() const { return scale_length; }
	inline size_t		  GetFretCount() const { return fret_count; }
	inline size_t		  GetFretStart() const { return fret_start; }
	inline size_t		  GetFretEnd() const { return fret_end; }
	inline double		  GetFretWidth() const { return fret_width; }
	inline double		  GetFretHeight() const { return fret_height; }
	inline double		  GetScallopDepth() const { return scallop_depth; }
	inline FretDataVector GetFretDataVec() const { return fret_data_vec; }

  private:
	double		   scale_length;
	size_t		   fret_count;
	size_t		   fret_start;
	size_t		   fret_end;
	double		   fret_width;
	double		   fret_height;
	double		   scallop_depth;
	FretDataVector fret_data_vec;
	size_t		   fret_index = 1;
};
}	 // namespace Scallop
}	 // namespace Nifty