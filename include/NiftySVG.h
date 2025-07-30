#pragma once

#include <lunasvg.h> 
#include <glad/glad.h>
#include <string>
#include <sstream>
#include <iomanip>

namespace Nifty::SVG
{
	class Handler
	{
	public:
		std::unique_ptr<lunasvg::Document> document;
		lunasvg::Bitmap bitmap;
		GLuint texture_id = 0;
		int width, height = 500;

		void Update(const std::string & svg_data);

		~Handler();
	};
}