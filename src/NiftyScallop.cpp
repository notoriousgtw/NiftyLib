#include "NiftyScallop.h"
#include <NiftyScallop.h>

namespace nft
{
//namespace SVG
//{
//
//using namespace nft::SVG::Templates::Fretboard;

//Fretboard::ClipPath::ClipPath(Scallop::FretboardData* fretboard_data)
//{
//	this->type			 = ColorName::Scallop;
//	this->router_bit	 = nullptr;
//	this->route_offset	 = nullptr;
//	this->fretboard_data = fretboard_data;
//	GenId();
//}
//
//Fretboard::ClipPath::ClipPath(Scallop::FretboardData* fretboard_data,
//							  Route::RouterBit		  router_bit,
//							  Scallop::RouteOffset	  route_offset)
//{
//	this->type			 = ColorName::Route;
//	this->router_bit	 = &router_bit;
//	this->route_offset	 = &route_offset;
//	this->fretboard_data = fretboard_data;
//	GenId();
//}
//
//std::string Fretboard::ClipPath::Calc() {}
//
//void Fretboard::ClipPath::GenId()
//{
//	id = 0b00000000;
//	if (type == ColorName::Route)
//		id = id | 0b00000001;
//
//	if (*router_bit == Route::RouterBit::ThreeEighth)
//		id = id | 0b00000010;
//	else if (*router_bit == Route::RouterBit::FiveSixteenth)
//		id = id | 0b00000100;
//	else if (*router_bit == Route::RouterBit::ThreeSixteenth)
//		id = id | 0b00001000;
//	else if (*router_bit == Route::RouterBit::Eighth)
//		id = id | 0b00010000;
//
//	if (*route_offset == Scallop::RouteOffset::Quarter)
//		id = id | 0b00100000;
//	else if (*route_offset == Scallop::RouteOffset::Half)
//		id = id | 0b01000000;
//	else if (*route_offset == Scallop::RouteOffset::ThreeQuarter)
//		id = id | 0b10000000;
//}
//
//Fretboard::Fretboard() { Update(); }
//
//void Fretboard::Update()
//{
//	if (should_update)
//	{
//		size_t pos;
//
//		printf("%s", svg_data.c_str());
//
//		// pos = svg_data.find("SCALLOP_DATA");
//		// if (pos != std::string::npos)
//		//	svg_data.replace(pos, std::string("SCALLOP_DATA").length(), CalcScallopData());
//
//		pos = svg_data.find("\nCLIP_DATA");
//		if (pos != std::string::npos) svg_data.replace(pos, std::string("\nCLIP_DATA").length(), CalcClipData());
//
//		pos = svg_data.find("\nFRET_DATA");
//		if (pos != std::string::npos) svg_data.replace(pos, std::string("\nFRET_DATA").length(), CalcFretData());
//
//		printf("%s", svg_data.c_str());
//
//		svg_handler.Update(svg_data);
//	}
//	should_update = false;
//}
//
//void Fretboard::SetScaleLength(double scale_length)
//{
//	//this->fretboard_data->scale_length = scale_length;
//	should_update	   = true;
//}
//
//void Fretboard::SetFretCount(size_t fret_count)
//{
//	//this->fretboard_data->fret_count = fret_count;
//	should_update	 = true;
//}
//
//void Fretboard::SetFretWidth(double fret_width)
//{
//	//this->fretboard_data->fret_width = fret_width;
//	should_update	 = true;
//}
//
//void Fretboard::SetFretHeight(double fret_height)
//{
//	//this->fretboard_data->fret_height = fret_height;
//	should_update	  = true;
//}
//
//void Fretboard::SetScallopDepth(double scallop_depth)
//{
//	//this->fretboard_data->scallop_depth = scallop_depth;
//	should_update		= true;
//}
//
//void Fretboard::PushClipPath(ClipPath clip_path)
//{
//	clip_paths.insert(clip_path);
//	should_update = true;
//}
//
//void Fretboard::PopClipPath(ClipPath clip_path)
//{
//	clip_paths.erase(clip_path);
//	should_update = true;
//}
//
//std::string Fretboard::CalcFretData()
//{
//	std::string fret_data;
//
//	//for (int i = 1; i <= fret_count + 1; i++)
//	//{
//	//	double fret_length = FretCalculator(scale_length, i);
//	//	if (i == 1)
//	//		fret_length -= fret_width / 2;
//	//	else
//	//		fret_length -= FretCalculator(scale_length, i - 1) - fret_width;
//
//	//	std::ostringstream edit_stream;
//	//	std::string		   new_fret_data;
//
//	//	if (i != fret_count + 1)
//	//		new_fret_data = fret_body_template;
//	//	else
//	//		new_fret_data = fret_body_final_template;
//	//	// printf("%s\n\n", new_fret_data.c_str());
//
//	//	edit_stream << std::fixed << std::setprecision(1) << fret_length * 1000;
//	//	size_t pos = new_fret_data.find("FRET_LENGTH");
//	//	if (pos != std::string::npos)
//	//		new_fret_data.replace(pos, std::string("FRET_LENGTH").length(), edit_stream.str());
//	//	edit_stream.str("");
//
//	//	edit_stream << std::fixed << std::setprecision(1) << (fret_width / 2) * 1000;
//	//	pos = new_fret_data.find("FRET_WIDTH_HALF");
//	//	if (pos != std::string::npos)
//	//		new_fret_data.replace(pos, std::string("FRET_WIDTH_HALF").length(), edit_stream.str());
//	//	edit_stream.str("");
//
//	//	edit_stream << std::fixed << std::setprecision(1) << fret_height * 1000;
//	//	pos = new_fret_data.find("FRET_HEIGHT");
//	//	if (pos != std::string::npos)
//	//		new_fret_data.replace(pos, std::string("FRET_HEIGHT").length(), edit_stream.str());
//	//	edit_stream.str("");
//
//	//	edit_stream << std::fixed << std::setprecision(1) << fret_width * 1000;
//	//	pos = new_fret_data.find("FRET_WIDTH");
//	//	if (pos != std::string::npos) new_fret_data.replace(pos, std::string("FRET_WIDTH").length(), edit_stream.str());
//	//	edit_stream.str("");
//
//	//	new_fret_data.pop_back();
//	//	// printf("%s\n", new_fret_data.c_str());
//	//	fret_data.append(new_fret_data);
//	//}
//	return fret_data;
//}
//
//std::string Fretboard::CalcClipData()
//{
//	std::string clip_data = clip_template;
//	size_t		pos;
//	// printf("%s", clip_data.c_str());
//
//	pos = clip_data.find("\nCLIP_PATHS");
//	if (pos != std::string::npos) clip_data.replace(pos, std::string("\nCLIP_PATHS").length(), CalcScallopClipPath());
//
//	// printf("%s", clip_data.c_str());
//	return (clip_data);
//}
//
//std::string Fretboard::CalcScallopClipPath()
//{
//	std::string clip_path = clip_path_template;
//	size_t		pos;
//	// printf("%s", clip_data.c_str());
//
//	pos = clip_path.find("\nCLIP_HEAD");
//	if (pos != std::string::npos) clip_path.replace(pos, std::string("\nCLIP_HEAD").length(), CalcScallopClipHead());
//
//	pos = clip_path.find("\nCLIP_BODY");
//	if (pos != std::string::npos) clip_path.replace(pos, std::string("\nCLIP_BODY").length(), CalcScallopClipBody());
//
//	std::string scallop_clip_tail = scallop_clip_tail_template;
//	scallop_clip_tail.pop_back();
//	pos = clip_path.find("\nCLIP_TAIL");
//	if (pos != std::string::npos) clip_path.replace(pos, std::string("\nCLIP_TAIL").length(), scallop_clip_tail);
//
//	// printf("%s", clip_data.c_str());
//	return (clip_path);
//}
//
//std::string Fretboard::CalcScallopClipHead()
//{
//	std::string scallop_clip_head = scallop_clip_head_template;
//	size_t		pos;
//
//	pos = scallop_clip_head.find("SCALLOP_OFFSET");
//	if (pos != std::string::npos) scallop_clip_head.replace(pos, std::string("SCALLOP_OFFSET").length(), "500");
//
//	scallop_clip_head.pop_back();
//	return scallop_clip_head;
//}
//
//std::string Fretboard::CalcScallopClipBody()
//{
//	std::string scallop_clip_body;
//
//	//for (int i = 1; i <= fret_count; i++)
//	//{
//	//	double fret_length = FretCalculator(scale_length, i);
//	//	if (i == 1)
//	//		fret_length -= fret_width / 2;
//	//	else
//	//		fret_length -= FretCalculator(scale_length, i - 1) - fret_width;
//
//	//	std::ostringstream edit_stream;
//	//	std::string		   new_scallop_data;
//
//	//	if (i < fret_count)
//	//		new_scallop_data = scallop_clip_body_template;
//	//	else
//	//		new_scallop_data = scallop_clip_body_final_template;
//	//	// printf("%s\n\n", new_scallop_data.c_str());
//
//	//	edit_stream << std::fixed << std::setprecision(1) << (fret_length / 2) * 1000;
//	//	size_t pos = new_scallop_data.find("FRET_LENGTH_HALF");
//	//	if (pos != std::string::npos)
//	//		new_scallop_data.replace(pos, std::string("FRET_LENGTH_HALF").length(), edit_stream.str());
//	//	edit_stream.str("");
//
//	//	edit_stream << std::fixed << std::setprecision(1) << scallop_depth * 1000;
//	//	pos = new_scallop_data.find("SCALLOP_DEPTH");
//	//	if (pos != std::string::npos)
//	//		new_scallop_data.replace(pos, std::string("SCALLOP_DEPTH").length(), edit_stream.str());
//	//	edit_stream.str("");
//
//	//	edit_stream << std::fixed << std::setprecision(1) << fret_length * 1000;
//	//	pos = new_scallop_data.find("FRET_LENGTH");
//	//	if (pos != std::string::npos)
//	//		new_scallop_data.replace(pos, std::string("FRET_LENGTH").length(), edit_stream.str());
//	//	edit_stream.str("");
//
//	//	edit_stream << std::fixed << std::setprecision(1) << fret_width * 1000;
//	//	pos = new_scallop_data.find("FRET_WIDTH");
//	//	if (pos != std::string::npos)
//	//		new_scallop_data.replace(pos, std::string("FRET_WIDTH").length(), edit_stream.str());
//	//	edit_stream.str("");
//
//	//	new_scallop_data.pop_back();
//	//	// printf("%s\n", new_scallop_data.c_str());
//	//	scallop_clip_body.append(new_scallop_data);
//	//}
//	return scallop_clip_body;
//}
//
//std::string Fretboard::CalcRouteClipPaths()
//{
//	std::string clip_path = clip_path_template;
//	size_t		pos;
//	// printf("%s", clip_data.c_str());
//
//	pos = clip_path.find("\nCLIP_HEAD");
//	if (pos != std::string::npos) clip_path.replace(pos, std::string("\nCLIP_HEAD").length(), CalcScallopClipHead());
//
//	pos = clip_path.find("\nCLIP_BODY");
//	if (pos != std::string::npos) clip_path.replace(pos, std::string("\nCLIP_BODY").length(), CalcScallopClipBody());
//
//	std::string scallop_clip_tail = scallop_clip_tail_template;
//	scallop_clip_tail.pop_back();
//	pos = clip_path.find("\nCLIP_TAIL");
//	if (pos != std::string::npos) clip_path.replace(pos, std::string("\nCLIP_TAIL").length(), scallop_clip_tail);
//
//	// printf("%s", clip_data.c_str());
//	return (clip_path);
//}
//
//std::string Fretboard::CalcRouteClipHead()
//{
//	std::string scallop_clip_head = scallop_clip_head_template;
//	size_t		pos;
//
//	pos = scallop_clip_head.find("SCALLOP_OFFSET");
//	if (pos != std::string::npos) scallop_clip_head.replace(pos, std::string("SCALLOP_OFFSET").length(), "500");
//
//	scallop_clip_head.pop_back();
//	return scallop_clip_head;
//}
//
//std::string Fretboard::CalcRouteClipBody()
//{
//	std::string scallop_clip_body;
//
//	//for (int i = 1; i <= fret_count; i++)
//	//{
//	//	double fret_length = FretCalculator(scale_length, i);
//	//	if (i == 1)
//	//		fret_length -= fret_width / 2;
//	//	else
//	//		fret_length -= FretCalculator(scale_length, i - 1) - fret_width;
//
//	//	std::ostringstream edit_stream;
//	//	std::string		   new_scallop_data;
//
//	//	if (i < fret_count)
//	//		new_scallop_data = scallop_clip_body_template;
//	//	else
//	//		new_scallop_data = scallop_clip_body_final_template;
//	//	// printf("%s\n\n", new_scallop_data.c_str());
//
//	//	edit_stream << std::fixed << std::setprecision(1) << (fret_length / 2) * 1000;
//	//	size_t pos = new_scallop_data.find("FRET_LENGTH_HALF");
//	//	if (pos != std::string::npos)
//	//		new_scallop_data.replace(pos, std::string("FRET_LENGTH_HALF").length(), edit_stream.str());
//	//	edit_stream.str("");
//
//	//	edit_stream << std::fixed << std::setprecision(1) << scallop_depth * 1000;
//	//	pos = new_scallop_data.find("SCALLOP_DEPTH");
//	//	if (pos != std::string::npos)
//	//		new_scallop_data.replace(pos, std::string("SCALLOP_DEPTH").length(), edit_stream.str());
//	//	edit_stream.str("");
//
//	//	edit_stream << std::fixed << std::setprecision(1) << fret_length * 1000;
//	//	pos = new_scallop_data.find("FRET_LENGTH");
//	//	if (pos != std::string::npos)
//	//		new_scallop_data.replace(pos, std::string("FRET_LENGTH").length(), edit_stream.str());
//	//	edit_stream.str("");
//
//	//	edit_stream << std::fixed << std::setprecision(1) << fret_width * 1000;
//	//	pos = new_scallop_data.find("FRET_WIDTH");
//	//	if (pos != std::string::npos)
//	//		new_scallop_data.replace(pos, std::string("FRET_WIDTH").length(), edit_stream.str());
//	//	edit_stream.str("");
//
//	//	new_scallop_data.pop_back();
//	//	// printf("%s\n", new_scallop_data.c_str());
//	//	scallop_clip_body.append(new_scallop_data);
//	//}
//	return scallop_clip_body;
//}
//}	 // namespace SVG
//namespace Scallop
//{
//
//FretData::FretData(FretboardData* fretboard_data, size_t fret_number):
//	fretboard_data(fretboard_data), fret_number(fret_number)
//{
//	SetFretLength();
//	CalcRoutes();
//}
//
//void FretData::CalcRoutes()
//{
//	double scallop_depth = fretboard_data->GetScallopDepth();
//
//	for (const auto& pair : Route::router_bits)
//	{
//		for (const auto& it : scallop_route_offsets)
//		{
//			double	   offset = 0;
//			glm::dvec2 bit_chord;
//			glm::dvec2 scallop_chord;
//			double	   depth;
//
//			if (it == RouteOffset::Half)
//				depth = scallop_depth;
//			else
//				depth = scallop_depth / 2;
//
//			if (it == RouteOffset::Quarter)
//				offset = -(fret_length / 4);
//			else if (it == RouteOffset::ThreeQuarter)
//				offset = fret_length / 4;
//
//			size_t slices = 1;
//			double test_depth;
//			bool   pass = true;
//			for (size_t i = 1; i <= slices; i++)
//			{
//				test_depth	  = (depth / i);
//				bit_chord	  = Num::GetChord(pair.second, depth - test_depth);
//				scallop_chord = Num::GetChord(fret_length / 2, scallop_depth, depth - test_depth, Num::Axis2D::y);
//				bit_chord.x += offset;
//				bit_chord.y += offset;
//
//				if (bit_chord.x < scallop_chord.x || bit_chord.y > scallop_chord.y) pass = false;
//			}
//
//			bool can_insert = true;
//			for (auto& route : routes)
//				if (route.offset == it) can_insert = false;
//
//			if (pass && can_insert) routes.push_back(ScallopRoute(pair.first, depth, it, offset + (fret_length / 2)));
//		}
//	}
//	return;
//}
//
//void FretData::SetFretLength()
//{
//	if (!this->fretboard_data) { throw std::runtime_error("fret_map is not initialized."); }
//
//	double scale_length = this->fretboard_data->GetScaleLength();
//	double fret_width	= this->fretboard_data->GetFretWidth();
//
//	if (fret_number == 1)
//		fret_length = FretCalculator(scale_length, fret_number) - (fret_width / 2);
//	else
//		fret_length =
//			FretCalculator(scale_length, fret_number) - FretCalculator(scale_length, fret_number - 1) - fret_width;
//
//	// double offset_quarter		= (fret_length * 0.25) + (router_base_width / 2);
//	// double offset_half			= (fret_length * 0.5) + (router_base_width / 2);
//	// double offset_three_quarter = (fret_length * 0.75) + (router_base_width / 2);
//}
//
//FretboardData::FretboardData(double scale_length,
//							 int	fret_count,
//							 int	fret_start,
//							 int	fret_end,
//							 double fret_width,
//							 double fret_height,
//							 double scallop_depth,
//							 double router_base_width):
//	scale_length(scale_length),
//	fret_count(fret_count),
//	fret_start(fret_start),
//	fret_end(fret_end),
//	fret_width(fret_width),
//	fret_height(fret_height),
//	scallop_depth(scallop_depth),
//	router_base_width(router_base_width)
//{
//	for (int i = 1; i <= fret_count; i++) { fret_data_vec.push_back(FretData(this, i)); }
//};

//}	 // namespace Scallop
}	 // namespace nft
