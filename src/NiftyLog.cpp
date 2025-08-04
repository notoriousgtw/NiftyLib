
#include "NiftyLog.h"

#include "NiftyError.h"
#include <regex>

namespace nft
{
void Logger::Print(const std::string& message,
				   const std::string& extra,
				   LogType			  type,
				   DisplayType		  display_type,
				   bool				  new_line,
				   bool				  debug_only,
				   nft::Color		  color)
{
	std::string color_str	= std::to_string(color.data);
	std::string tag			= "";
	std::string name_final = "";
	std::string extra_final = "";

	switch (type)
	{
	case LogType::Default: tag = default_tag; break;
	case LogType::Debug: tag = debug_tag; break;
	case LogType::Warning: tag = warning_tag; break;
	case LogType::Error: tag = error_tag; break;
	case LogType::Fatal: tag = fatal_tag; break;
	}

	if (!name.empty())
		name_final = std::regex_replace(name_format, std::regex("NAME"), name);
	else
		name_final = "";
	if (!tag.empty())
		tag = std::regex_replace(tag_format, std::regex("TAG"), tag);
	else
		tag = "";
	if (!extra.empty())
		extra_final = std::regex_replace(extra_format, std::regex("EXTRA"), extra);
	else
		extra_final = "";
	// header = std::regex_replace(header, std::regex("NAME"), name);
	// header = std::regex_replace(header, std::regex("TAG"), tag);
	// if (!extra.empty()) header = std::regex_replace(header, std::regex("EXTRA"), extra);
	std::string header = std::format("{}{}{}:", name_final, tag, extra_final);

	if (color.encoding != Color::Encoding::Console)
	{
		ErrorHandler::Error<ColorEncodingError>("Color encoding is not set to Console! Defaulting to no color.");
		color_str = "0";
	}

	if ((debug_only && DEBUG) || !debug_only)
	{
		if (display_type == DisplayType::Null) display_type = this->display_type;
		switch (display_type)
		{
		case DisplayType::Default: std::print("\x1b[{}m{} {}\x1b[0m", color_str, header, message); break;
		case DisplayType::Headerless: std::print("\x1b[{}m{}\x1b[0m", color_str, message); break;
		case DisplayType::MessageOnNewLine:
			std::println("\x1b[{}m{}", color_str, header);
			std::print("{}\x1b[0m", message);
			break;
		case DisplayType::MessageOnNewLineIndented:
			std::println("\x1b[{}m{}", color_str, header);
			std::print("\t{}\x1b[0m", message);
			break;
		}
		if (new_line) std::print("\n");
	}
};

}	 // namespace nft