#pragma once

#include <optional>
#include <print>

#include "NiftyColor.h"

namespace nft
{
#ifdef _DEBUG
constexpr bool DEBUG = true;
#else
constexpr bool DEBUG = false;
#endif

class Logger
{
  public:
	enum class LogType
	{
		Default,
		Debug,
		Warning,
		Error,
		Fatal
	};

	enum class DisplayType
	{
		Null,
		Default,
		Headerless,
		MessageOnNewLine,
		MessageOnNewLineIndented
	};

	Logger(std::string name = "NiftyLib", DisplayType display_type = DisplayType::Default, bool new_line = true):
		name(name), display_type(display_type), new_line(new_line)
	{
	}

	void Log(const std::string& message,
			 std::string		extra		 = "",
			 DisplayType		display_type = DisplayType::Null,
			 bool				new_line	 = true)
	{
		Print(message, extra, LogType::Default, display_type, new_line, false);
	};

	void Debug(const std::string& message,
			   std::string		  extra		   = "",
			   DisplayType		  display_type = DisplayType::Null,
			   bool				  new_line	   = true)
	{
		Print(message, extra, LogType::Debug, display_type, new_line, true);
	};

	void Warn(const std::string& message,
			  std::string		 extra		  = "",
			  DisplayType		 display_type = DisplayType::Null,
			  bool				 new_line	  = true)
	{
		Print(message, extra, LogType::Warning, display_type, new_line, true, Pallete::Console::YellowFG);
	};

	void Error(const std::string& message,
			   std::string		  extra		   = "",
			   DisplayType		  display_type = DisplayType::Null,
			   bool				  new_line	   = true)
	{
		Print(message, extra, LogType::Error, display_type, new_line, false, Pallete::Console::RedFG);
	};

	void Fatal(const std::string& message,
			   std::string		  extra		   = "",
			   DisplayType		  display_type = DisplayType::Null,
			   bool				  new_line	   = true)
	{
		Print(message, extra, LogType::Fatal, display_type, new_line, false, Pallete::Console::RedFG);
	};

	void SetName(std::string name) { this->name = name; }
	void SetDefaultTag(std::string default_tag) { this->default_tag = default_tag; }
	void SetDebugTag(std::string debug_tag) { this->debug_tag = debug_tag; }
	void SetWarningTag(std::string warning_tag) { this->warning_tag = warning_tag; }
	void SetErrorTag(std::string error_tag) { this->error_tag = error_tag; }
	void SetFatalTag(std::string fatal_tag) { this->fatal_tag = fatal_tag; }
	void SetDisplayType(DisplayType display_type) { this->display_type = display_type; }

  private:
	std::string default_tag	= "";
	std::string debug_tag	= "Debug";
	std::string warning_tag = "Warn";
	std::string error_tag	= "Error";
	std::string fatal_tag	= "Fatal";
	std::string name_format	   = "[NAME]";
	std::string tag_format = "[TAG]";
	std::string extra_format = "{EXTRA}";
	std::string name;
	DisplayType display_type;
	bool		new_line;

	void Print(const std::string& message,
			   const std::string& extra,
			   LogType			  type,
			   DisplayType		  display_type,
			   bool				  new_line,
			   bool				  debug_only,
			   Color			  color = Pallete::Console::DefaultFG);
};
// namespace Log
//{
//
// }
}	 // namespace nft