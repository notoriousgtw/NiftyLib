#include "NiftySVG.h"

namespace Nifty::SVG
{
void BMPHandler::Update(const std::string & svg_data)
{
	document = lunasvg::Document::loadFromData(svg_data);
	if (!document)
		return;
	bitmap = document->renderToBitmap();
	bitmap.convertToRGBA();
	width = bitmap.width();
	height = bitmap.height();
	const unsigned char * pixels = bitmap.data(); // Assumes RGBA format, check LunaSVG docs

	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Upload the pixel data to the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// Unbind the texture
	glBindTexture(GL_TEXTURE_2D, 0);
}
BMPHandler::~BMPHandler()
{
			// Cleanup GLTexture
	if (texture_id)
	{
		glDeleteTextures(1, &texture_id);
		texture_id = 0;
	}
}
}