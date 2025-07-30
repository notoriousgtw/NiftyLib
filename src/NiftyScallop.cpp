#include <NiftyScallop.h>

namespace Nifty
{
namespace SVG
{

using namespace Nifty::SVG::Templates::Fretboard;

Fretboard::Fretboard() { Update(scale_length, fret_count, fret_width, fret_height, scallop_depth, show_scallop); }

void Fretboard::Update(double scale_length,
					   int	  fret_count,
					   double fret_width,
					   double fret_height,
					   double scallop_depth,
					   bool	  show_scallop)
{
	svg_data			= svg_template;
	this->scale_length	= scale_length;
	this->fret_count	= fret_count;
	this->fret_width	= fret_width;
	this->fret_height	= fret_height;
	this->scallop_depth = scallop_depth;
	this->show_scallop	= show_scallop;

	size_t pos;

	printf("%s", svg_data.c_str());

	// pos = svg_data.find("SCALLOP_DATA");
	// if (pos != std::string::npos)
	//	svg_data.replace(pos, std::string("SCALLOP_DATA").length(), CalcScallopData());

	pos = svg_data.find("\nCLIP_DATA");
	if (pos != std::string::npos) svg_data.replace(pos, std::string("\nCLIP_DATA").length(), CalcClipData());

	pos = svg_data.find("\nFRET_DATA");
	if (pos != std::string::npos) svg_data.replace(pos, std::string("\nFRET_DATA").length(), CalcFretData());

	pos = svg_data.find("\nSHOW_CLIP_DATA");
	if (pos != std::string::npos) svg_data.replace(pos, std::string("\nSHOW_CLIP_DATA").length(), CalcShowClips());

	printf("%s", svg_data.c_str());

	svg_handler.Update(svg_data);
}

std::string Fretboard::CalcFretData()
{
	std::string fret_data;

	for (int i = 1; i <= fret_count + 1; i++)
	{
		double fret_length = FretCalculator(scale_length, i);
		if (i == 1)
			fret_length -= fret_width / 2;
		else
			fret_length -= FretCalculator(scale_length, i - 1) - fret_width;

		std::ostringstream edit_stream;
		std::string		   new_fret_data;

		if (i != fret_count + 1)
			new_fret_data = fret_body_template;
		else
			new_fret_data = fret_body_final_template;
		// printf("%s\n\n", new_fret_data.c_str());

		edit_stream << std::fixed << std::setprecision(1) << fret_length * 1000;
		size_t pos = new_fret_data.find("FRET_LENGTH");
		if (pos != std::string::npos)
			new_fret_data.replace(pos, std::string("FRET_LENGTH").length(), edit_stream.str());
		edit_stream.str("");

		edit_stream << std::fixed << std::setprecision(1) << (fret_width / 2) * 1000;
		pos = new_fret_data.find("FRET_WIDTH_HALF");
		if (pos != std::string::npos)
			new_fret_data.replace(pos, std::string("FRET_WIDTH_HALF").length(), edit_stream.str());
		edit_stream.str("");

		edit_stream << std::fixed << std::setprecision(1) << fret_height * 1000;
		pos = new_fret_data.find("FRET_HEIGHT");
		if (pos != std::string::npos)
			new_fret_data.replace(pos, std::string("FRET_HEIGHT").length(), edit_stream.str());
		edit_stream.str("");

		edit_stream << std::fixed << std::setprecision(1) << fret_width * 1000;
		pos = new_fret_data.find("FRET_WIDTH");
		if (pos != std::string::npos) new_fret_data.replace(pos, std::string("FRET_WIDTH").length(), edit_stream.str());
		edit_stream.str("");

		new_fret_data.pop_back();
		// printf("%s\n", new_fret_data.c_str());
		fret_data.append(new_fret_data);
	}
	return fret_data;
}

std::string Fretboard::CalcClipData()
{
	std::string clip_data = clip_template;
	size_t		pos;
	// printf("%s", clip_data.c_str());

	pos = clip_data.find("CLIP_ID");
	if (pos != std::string::npos) clip_data.replace(pos, std::string("CLIP_ID").length(), scallop_clip_id);

	pos = clip_data.find("\nCLIP_HEAD");
	if (pos != std::string::npos) clip_data.replace(pos, std::string("\nCLIP_HEAD").length(), CalcScallopClipHead());

	pos = clip_data.find("\nCLIP_BODY");
	if (pos != std::string::npos) clip_data.replace(pos, std::string("\nCLIP_BODY").length(), CalcScallopClipBody());

	std::string scallop_clip_tail = scallop_clip_tail_template;
	scallop_clip_tail.pop_back();
	pos = clip_data.find("\nCLIP_TAIL");
	if (pos != std::string::npos) clip_data.replace(pos, std::string("\nCLIP_TAIL").length(), scallop_clip_tail);

	// printf("%s", clip_data.c_str());
	return (clip_data);
}

std::string Fretboard::CalcScallopClipHead()
{
	std::string scallop_clip_head = scallop_clip_head_template;
	size_t		pos;

	pos = scallop_clip_head.find("SCALLOP_OFFSET");
	if (pos != std::string::npos) scallop_clip_head.replace(pos, std::string("SCALLOP_OFFSET").length(), "500");

	scallop_clip_head.pop_back();
	return scallop_clip_head;
}

std::string Fretboard::CalcScallopClipBody()
{
	std::string scallop_clip_body;

	for (int i = 1; i <= fret_count; i++)
	{
		double fret_length = FretCalculator(scale_length, i);
		if (i == 1)
			fret_length -= fret_width / 2;
		else
			fret_length -= FretCalculator(scale_length, i - 1) - fret_width;

		std::ostringstream edit_stream;
		std::string		   new_scallop_data;

		if (i < fret_count)
			new_scallop_data = scallop_clip_body_template;
		else
			new_scallop_data = scallop_clip_body_final_template;
		// printf("%s\n\n", new_scallop_data.c_str());

		edit_stream << std::fixed << std::setprecision(1) << (fret_length / 2) * 1000;
		size_t pos = new_scallop_data.find("FRET_LENGTH_HALF");
		if (pos != std::string::npos)
			new_scallop_data.replace(pos, std::string("FRET_LENGTH_HALF").length(), edit_stream.str());
		edit_stream.str("");

		edit_stream << std::fixed << std::setprecision(1) << scallop_depth * 1000;
		pos = new_scallop_data.find("SCALLOP_DEPTH");
		if (pos != std::string::npos)
			new_scallop_data.replace(pos, std::string("SCALLOP_DEPTH").length(), edit_stream.str());
		edit_stream.str("");

		edit_stream << std::fixed << std::setprecision(1) << fret_length * 1000;
		pos = new_scallop_data.find("FRET_LENGTH");
		if (pos != std::string::npos)
			new_scallop_data.replace(pos, std::string("FRET_LENGTH").length(), edit_stream.str());
		edit_stream.str("");

		edit_stream << std::fixed << std::setprecision(1) << fret_width * 1000;
		pos = new_scallop_data.find("FRET_WIDTH");
		if (pos != std::string::npos)
			new_scallop_data.replace(pos, std::string("FRET_WIDTH").length(), edit_stream.str());
		edit_stream.str("");

		new_scallop_data.pop_back();
		// printf("%s\n", new_scallop_data.c_str());
		scallop_clip_body.append(new_scallop_data);
	}
	return scallop_clip_body;
}

std::string Fretboard::CalcScallopClipTail()
{
	std::string scallop_clip_tail = scallop_clip_body_template;
	size_t		pos;

	pos = scallop_clip_tail.find("SCALLOP_OFFSET");
	if (pos != std::string::npos) scallop_clip_tail.replace(pos, std::string("SCALLOP_OFFSET").length(), "500");

	scallop_clip_tail.pop_back();
	return scallop_clip_tail;
}

std::string Fretboard::CalcShowClips()
{
	std::string show_clip_data = show_clip_template;
	size_t		pos;

	pos = show_clip_data.find("CLIP_ID");
	if (pos != std::string::npos)
		if (show_scallop)
			show_clip_data.replace(pos, std::string("CLIP_ID").length(), scallop_clip_id);
		else
			return "";
	return show_clip_data;
}
}	 // namespace SVG
namespace Scallop
{

FretData::FretData(FretMap* fret_map, size_t fret_number): fret_map(fret_map), fret_number(fret_number)
{
	SetFretLength();
}

void FretData::SetFretLength()
{
	if (!this->fret_map) { throw std::runtime_error("fret_map is not initialized."); }

	double scale_length = this->fret_map->GetScaleLength();
	double fret_width	= this->fret_map->GetFretWidth();

	if (fret_number == 1)
		fret_length = FretCalculator(scale_length, fret_number) - (fret_width / 2);
	else
		fret_length =
			FretCalculator(scale_length, fret_number) - FretCalculator(scale_length, fret_number - 1) - fret_width;

	// double offset_quarter		= (fret_length * 0.25) + (router_base_width / 2);
	// double offset_half			= (fret_length * 0.5) + (router_base_width / 2);
	// double offset_three_quarter = (fret_length * 0.75) + (router_base_width / 2);
}
}	 // namespace Scallop
}	 // namespace Nifty
