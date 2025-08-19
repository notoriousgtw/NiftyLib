#include "core/string.h"

namespace nft::string
{
std::vector<std::string> split(std::string string, std::string delimiter)
{
	std::vector<std::string> split_str;

	size_t pos = 0;
	std::string token;
	while ((pos = string.find(delimiter)) != std::string::npos)
	{
		token = string.substr(0, pos);
		split_str.push_back(token);
		string.erase(0, pos + delimiter.length());
	}
	split_str.push_back(string);	// Add the last token after the last delimiter
	return split_str;
}
}	 // namespace nft::tring