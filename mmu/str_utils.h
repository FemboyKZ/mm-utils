#ifndef _INCLUDE_MMU_STR_UTILS_H_
#define _INCLUDE_MMU_STR_UTILS_H_

#include <algorithm>
#include <cctype>
#include <string>

namespace str
{
	// ASCII-lowercase a copy of `s`.
	inline std::string ToLower(const std::string &s)
	{
		std::string r = s;
		std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return r;
	}

	// ASCII-lowercase `s` in place.
	inline void ToLowerInPlace(std::string &s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	}

	// Strip a trailing :port suffix from an address ("1.2.3.4:27015" -> "1.2.3.4").
	// Splits on the last colon, so plain IPv4 and host names are handled. Not IPv6 aware.
	inline std::string StripPort(const char *addr)
	{
		if (!addr || !addr[0])
		{
			return {};
		}
		std::string s(addr);
		auto colon = s.rfind(':');
		if (colon != std::string::npos)
		{
			return s.substr(0, colon);
		}
		return s;
	}
} // namespace str

#endif // _INCLUDE_MMU_STR_UTILS_H_
