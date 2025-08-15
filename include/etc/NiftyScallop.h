#pragma once

#include <core/NiftyNum.h>
#include <etc/NiftySVG.h>
#include <etc/NiftyStik.h>
#include <etc/NiftyStikSVGTemplates.h>
#include <map>
#include <set>
#include <vector>

using namespace nft::Stik;

namespace nft
{
namespace SVG
{

//class Fretboard
//{
//  public:
//	class ClipPath
//	{
//	  public:
//		enum class ColorName
//		{
//			Scallop,
//			Route
//		};
//
//		ClipPath() = delete;
//		ClipPath(Scallop::FretboardData* fretboard_data);
//		ClipPath(Scallop::FretboardData* fretboard_data,
//				 Route::RouterBit		 router_bit,
//				 Scallop::RouteOffset	 route_offset);
//		bool		operator<(const ClipPath& other) const { return id < other.id; };
//		std::string Calc();
//
//	  private:
//		size_t					id;
//		ColorName					type;
//		Route::RouterBit*		router_bit	   = nullptr;
//		Scallop::RouteOffset*	route_offset   = nullptr;
//		Scallop::FretboardData* fretboard_data = nullptr;
//		void					GenId();
//	};
//
//	Fretboard();
//	void Update();
//	int	 GetTextureID() { return svg_handler.texture_id; };
//	int	 GetWidth() { return svg_handler.width; };
//	int	 GetHeight() { return svg_handler.height; };
//	void SetScaleLength(double scale_length);
//	void SetFretCount(size_t fret_count);
//	void SetFretWidth(double fret_width);
//	void SetFretHeight(double fret_height);
//	void SetScallopDepth(double scallop_depth);
//	void PushClipPath(ClipPath clip_path);
//	void PopClipPath(ClipPath clip_path);
//
//  private:
//	bool					should_update = true;
//	Scallop::FretboardData* fretboard_data;
//	std::set<ClipPath>		clip_paths;
//	std::string				svg_data;
//	BMPHandler					svg_handler;
//
//	std::string CalcFretData();
//	std::string CalcClipData();
//	std::string CalcScallopClipPath();
//	std::string CalcScallopClipHead();
//	std::string CalcScallopClipBody();
//	std::string CalcRouteClipPaths();
//	std::string CalcRouteClipHead();
//	std::string CalcRouteClipBody();
//};
//}	 // namespace SVG
//
//namespace Route
//{
//
//enum class RouterBit
//{
//	ThreeEighth,
//	FiveSixteenth,
//	ThreeSixteenth,
//	Eighth
//};
//
//const std::map<RouterBit, double> router_bits = { { RouterBit::ThreeEighth, 0.375 },
//												  { RouterBit::FiveSixteenth, 0.3132 },
//												  { RouterBit::ThreeSixteenth, 0.1875 },
//												  { RouterBit::Eighth, 0.125 } };
//
//}	 // namespace Route
//
//namespace Scallop
//{
//
//using namespace nft::Route;
//
//enum class RouteOffset
//{
//	Quarter,
//	Half,
//	ThreeQuarter
//};
//
//const std::set<RouteOffset> scallop_route_offsets = { RouteOffset::Quarter,
//													  RouteOffset::Half,
//													  RouteOffset::ThreeQuarter };
//
//struct ScallopRoute
//{
//	RouterBit	bit;
//	double		depth;
//	RouteOffset offset;
//	double		offset_dist;
//
//	ScallopRoute() = delete;
//	ScallopRoute(RouterBit bit, double depth, RouteOffset offset, double offset_dist):
//		bit(bit), depth(depth), offset(offset), offset_dist(offset_dist)
//	{
//	}
//};
//
//class FretboardData;
//
//class FretData
//{
//  public:
//	FretData() = delete;
//	FretData(FretboardData* fret_map, size_t fret_number);
//
//	inline size_t GetFretNumber() const { return fret_number; }
//	inline double GetFretLength() const { return fret_length; }
//	inline bool	  operator<(const FretData& other) const { return fret_number < other.fret_number; };
//
//  private:
//	FretboardData*			  fretboard_data;
//	size_t					  fret_number;
//	double					  fret_length = 0.0;
//	std::vector<ScallopRoute> routes;
//	void					  SetFretLength();
//	void					  CalcRoutes();
//};
//
//typedef std::vector<FretData> FretDataVector;
//
//class FretboardData
//{
//  public:
//	FretboardData() = delete;
//	FretboardData(double scale_length,
//				  int	 fret_count,
//				  int	 fret_start,
//				  int	 fret_end,
//				  double fret_width,
//				  double fret_height,
//				  double scallop_depth,
//				  double router_base_width);
//
//	inline double GetScaleLength() const { return scale_length; }
//	inline size_t GetFretCount() const { return fret_count; }
//	inline size_t GetFretStart() const { return fret_start; }
//	inline size_t GetFretEnd() const { return fret_end; }
//	inline double GetFretWidth() const { return fret_width; }
//	inline double GetFretHeight() const { return fret_height; }
//	inline double GetScallopDepth() const { return scallop_depth; }
//	inline double GetRouterBaseWidth() const { return router_base_width; }
//	// inline FretData*	  GetFretData() const { return fret_data_vec() }
//	inline FretDataVector GetFretDataVec() const { return fret_data_vec; }
//
//  private:
//	double		   scale_length;
//	size_t		   fret_count;
//	size_t		   fret_start;
//	size_t		   fret_end;
//	double		   fret_width;
//	double		   fret_height;
//	double		   scallop_depth;
//	double		   router_base_width;
//	FretDataVector fret_data_vec;
//	size_t		   fret_index = 1;
//};
}	 // namespace Scallop
}	 // namespace nft