#pragma once

#include <optional>
#include <print>

#include "core/color.h"

namespace nft
{
#ifdef _DEBUG
constexpr bool DEBUG = true;
#else
constexpr bool DEBUG = false;
#endif
namespace Log::Flags
{
using DisplayFlags = uint16_t;

static const DisplayFlags Null			   = 0b1000000000000000;
static const DisplayFlags None			   = 0b0000000000000000;
static const DisplayFlags NewLine		   = 0b0000000000000001;
static const DisplayFlags ShowName		   = 0b0000000000000010;
static const DisplayFlags ShowTag		   = 0b0000000000000100;
static const DisplayFlags ShowExtra		   = 0b0000000000001000;
static const DisplayFlags ShowTail		   = 0b0000000000010000;
static const DisplayFlags ShowHeader	   = 0b0000000000100000;
static const DisplayFlags Indent		   = 0b0000000001000000;
static const DisplayFlags IndentMessage	   = 0b0000000010000000;
static const DisplayFlags MessageOnNewLine = 0b0000000100000000;

static const DisplayFlags Default = 0b0000000010111111;
}	 // namespace Log::Flags

class Logger
{
  public:
	using DisplayFlags = uint16_t;
	enum class LogType
	{
		Default,
		Info,
		Debug,
		Warning,
		Error,
		Fatal
	};

	class DisplayFlag
	{
	  public:
	};

	Logger(std::string name = "NiftyLib"): name(name) {}

	void Log(const std::string&		message,
			 std::string			extra		   = "",
			 DisplayFlags			display_flags  = Log::Flags::Null,
			 std::optional<uint8_t> indent		   = std::optional<uint8_t>(),
			 std::optional<uint8_t> message_indent = std::optional<uint8_t>())
	{
		Print(message,
			  extra,
			  LogType::Default,
			  display_flags,
			  indent.has_value() ? *indent : this->indent,
			  message_indent.has_value() ? *message_indent : this->message_indent,
			  false);
	};

	void Info(const std::string&	 message,
			  std::string			 extra			= "",
			  DisplayFlags			 display_flags	= Log::Flags::Null,
			  std::optional<uint8_t> indent			= std::optional<uint8_t>(),
			  std::optional<uint8_t> message_indent = std::optional<uint8_t>())
	{
		Print(message,
			  extra,
			  LogType::Debug,
			  display_flags,
			  indent.has_value() ? *indent : this->indent,
			  message_indent.has_value() ? *message_indent : this->message_indent,
			  true);
	};

	void Debug(const std::string&	  message,
			   std::string			  extra			 = "",
			   DisplayFlags			  display_flags	 = Log::Flags::Null,
			   std::optional<uint8_t> indent		 = std::optional<uint8_t>(),
			   std::optional<uint8_t> message_indent = std::optional<uint8_t>())
	{
		Print(message,
			  extra,
			  LogType::Debug,
			  display_flags,
			  indent.has_value() ? *indent : this->indent,
			  message_indent.has_value() ? *message_indent : this->message_indent,
			  true);
	};

	void Warn(const std::string&	 message,
			  std::string			 extra			= "",
			  DisplayFlags			 display_flags	= Log::Flags::Null,
			  std::optional<uint8_t> indent			= std::optional<uint8_t>(),
			  std::optional<uint8_t> message_indent = std::optional<uint8_t>())
	{
		Print(message,
			  extra,
			  LogType::Warning,
			  display_flags,
			  indent.has_value() ? *indent : this->indent,
			  message_indent.has_value() ? *message_indent : this->message_indent,
			  true,
			  Pallete::Console::YellowFG);
	};

	void Error(const std::string&	  message,
			   std::string			  extra			 = "",
			   DisplayFlags			  display_flags	 = Log::Flags::Null,
			   std::optional<uint8_t> indent		 = std::optional<uint8_t>(),
			   std::optional<uint8_t> message_indent = std::optional<uint8_t>())
	{
		Print(message,
			  extra,
			  LogType::Error,
			  display_flags,
			  indent.has_value() ? *indent : this->indent,
			  message_indent.has_value() ? *message_indent : this->message_indent,
			  false,
			  Pallete::Console::RedFG);
	};

	void Fatal(const std::string&	  message,
			   std::string			  extra			 = "",
			   DisplayFlags			  display_flags	 = Log::Flags::Null,
			   std::optional<uint8_t> indent		 = std::optional<uint8_t>(),
			   std::optional<uint8_t> message_indent = std::optional<uint8_t>())
	{
		Print(message,
			  extra,
			  LogType::Fatal,
			  display_flags,
			  indent.has_value() ? *indent : this->indent,
			  message_indent.has_value() ? *message_indent : this->message_indent,
			  false,
			  Pallete::Console::RedFG);
	};

	void SetName(std::string name) { this->name = name; }
	void SetDefaultTag(std::string default_tag) { this->default_tag = default_tag; }
	void SetInfoTag(std::string info_tag) { this->info_tag = info_tag; }
	void SetDebugTag(std::string debug_tag) { this->debug_tag = debug_tag; }
	void SetWarningTag(std::string warning_tag) { this->warning_tag = warning_tag; }
	void SetErrorTag(std::string error_tag) { this->error_tag = error_tag; }
	void SetFatalTag(std::string fatal_tag) { this->fatal_tag = fatal_tag; }

	void SetNameFormat(const std::string& prefix, const std::string& suffix)
	{
		name_format = prefix + "NAME" + suffix;
	};

	void SetTagFormat(const std::string& prefix, const std::string& suffix)
	{
		tag_format = prefix + "TAG" + suffix;
	};

	void SetExtraFormat(const std::string& prefix, const std::string& suffix)
	{
		extra_format = prefix + "EXTRA" + suffix;
	};

	void SetHeaderTail(std::string header_tail) { this->header_tail = header_tail; }
	void SetIndent(uint8_t indent) { this->indent = indent; }
	void SetMessageIndent(uint8_t message_indent) { this->message_indent = message_indent; }
	void SetDisplayFlags(DisplayFlags display_flags) { this->display_flags = display_flags; }
	void SetVerbose(bool verbose) { this->verbose = verbose; }

  private:
	std::string	 default_tag	= "";
	std::string	 info_tag		= "Info";
	std::string	 debug_tag		= "Debug";
	std::string	 warning_tag	= "Warn";
	std::string	 error_tag		= "Error";
	std::string	 fatal_tag		= "Fatal";
	std::string	 name_format	= "[NAME]";
	std::string	 tag_format		= "[TAG]";
	std::string	 extra_format	= "{EXTRA}";
	std::string	 header_tail	= ":";
	uint8_t		 indent			= 0;
	uint8_t		 message_indent = 1;
	std::string	 name			= "";
	DisplayFlags display_flags	= Log::Flags::Default;
	bool		 verbose		= false;

	void Print(const std::string& message,
			   const std::string& extra,
			   LogType			  type,
			   DisplayFlags		  display_flags,
			   uint8_t			  indent,
			   uint8_t			  message_indent,
			   bool				  debug_only,
			   Color			  color = Pallete::Console::DefaultFG);
};
}	 // namespace nft