#include "core/log.h"

#include "core/error.h"

#include <regex>

namespace nft
{
void Logger::Print(const std::string& message,
				   const std::string& extra,
				   LogType			  type,
				   DisplayFlags		  display_flags,
				   uint8_t			  indent,
				   uint8_t			  message_indent,
				   bool				  debug_only,
				   nft::Color		  color)
{
	std::string color_str;
	if (color.encoding != Color::Encoding::Console)
	{
		NFT_ERROR(ColorEncodingError, "Color encoding is not set to Console! Defaulting to no color.");
		color_str = "0";
	}
	else
		color_str = std::to_string(color.data);

	if (display_flags == Log::Flags::Null) { display_flags = this->display_flags; }
	if ((debug_only && DEBUG) || !debug_only)
	{
		std::string tag_final	  = "";
		std::string name_final	  = "";
		std::string extra_final	  = "";
		std::string tail_final	  = "";
		std::string header_final  = "";
		std::string message_final = message;

		switch (type)
		{
		case LogType::Default: tag_final = default_tag; break;
		case LogType::Info: tag_final = info_tag; break;
		case LogType::Debug:
			if (verbose)
			{
				tag_final = debug_tag;
				break;
			}
			else
				return;
		case LogType::Warning:
			if (verbose)
			{
				tag_final = warning_tag;
				break;
			}
			else
				return;
		case LogType::Error: tag_final = error_tag; break;
		case LogType::Fatal: tag_final = fatal_tag; break;
		}

		if (display_flags & Log::Flags::ShowHeader)
		{
			if (!name.empty() && (display_flags & Log::Flags::ShowName))
				name_final = std::regex_replace(name_format, std::regex("NAME"), name);
			if (!tag_final.empty() && (display_flags & Log::Flags::ShowTag))
				tag_final = std::regex_replace(tag_format, std::regex("TAG"), tag_final);
			if (!extra.empty() && (display_flags & Log::Flags::ShowExtra))
				extra_final = std::regex_replace(extra_format, std::regex("EXTRA"), extra);
			if (display_flags & Log::Flags::ShowTail) tail_final = header_tail;

			header_final = std::format("{}{}{}{}", name_final, tag_final, extra_final, tail_final);

			if (display_flags & Log::Flags::Indent)
				for (uint16_t i = 0; i < indent; i++) { header_final.insert(0, " "); }
		}
		if (display_flags & Log::Flags::IndentMessage)
			for (uint16_t i = 0; i < message_indent; i++) { message_final.insert(0, " "); }
		if (!(display_flags & Log::Flags::MessageOnNewLine))
			std::print("\x1b[{}m{}{}\x1b[0m", color_str, header_final, message_final);
		else
			std::print("\x1b[{}m{}\n{}\x1b[0m", color_str, header_final, message_final);
		if (display_flags & Log::Flags::NewLine) std::print("\n");
	}
};

}	 // namespace nft