#pragma once

#include <iomanip>
#include <lunasvg.h>
#include <set>
#include <sstream>
#include <string>
#include <NiftySVGElements.h>

namespace nft::SVG
{
//class BMPHandler
//{
//  public:
//	std::unique_ptr<lunasvg::Document> document;
//	lunasvg::Bitmap					   bitmap;
//	int								   width, height = 500;
//
//	void Update(const std::string& svg_data);
//
//	~BMPHandler();
//};

//class Image
//{
//  public:
//	Image();
//	void Update();
//	int	 GetTextureID() { return bmp_handler.texture_id; };
//	int	 GetWidth() { return bmp_handler.width; };
//	int	 GetHeight() { return bmp_handler.height; };
//	void PushClipPath(ClipPath clip_path);
//	void PopClipPath(ClipPath clip_path);
//
//	Path& Path();
//
//  private:
//	bool			   should_update = true;
//	std::set<ClipPath> clip_paths;
//	std::string		   svg_data;
//	BMPHandler		   bmp_handler;
//};
}	 // namespace nft::SVG