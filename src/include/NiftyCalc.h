#pragma once  

#include <cmath>  
#include <set>  
#include <map>  
//#include "xlsxwriter.h"

namespace Nifty
{
	enum class ScallopOffsetType
	{
		Quarter,
		Half,
		ThreeQuarter
	};

    typedef std::map<ScallopOffsetType, double> ScallopOffsetMap;
	typedef std::map<int, ScallopOffsetMap> ScallopOffsetTable;

	inline ScallopOffsetTable ScallopCalculator(const double scale_length, const int fret_count, const double fret_width, const double router_base_width)
	{
		ScallopOffsetTable router_offsets;

		for (int i = 1; i < fret_count + 1; i++)
		{
			double fret_length;

			if (i == 1)
				fret_length = FretCalculator(scale_length, i) - (fret_width / 2);
			else
				fret_length = FretCalculator(scale_length, i) - FretCalculator(scale_length, i - 1) - fret_width;

			double offset_quarter = (fret_length * 0.25) + (router_base_width / 2);
			double offset_half = (fret_length * 0.5) + (router_base_width / 2);
			double offset_three_quarter = (fret_length * 0.75) + (router_base_width / 2);

			//Add offsets to the set
			ScallopOffsetMap fret_offsets;
			fret_offsets[ScallopOffsetType::Quarter] = offset_quarter;
			fret_offsets[ScallopOffsetType::Half] = offset_half;
			fret_offsets[ScallopOffsetType::ThreeQuarter] = offset_three_quarter;

			router_offsets[i] = fret_offsets;
		}
		return router_offsets;
	}

}
