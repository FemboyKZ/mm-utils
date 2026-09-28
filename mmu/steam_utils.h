#ifndef _INCLUDE_MMU_STEAM_UTILS_H_
#define _INCLUDE_MMU_STEAM_UTILS_H_

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

// Convert SteamID64 to "X:Y" suffix (the part after STEAM_)
// SBPP stores authids as STEAM_0:X:Y, but queries with REGEXP '^STEAM_[0-9]:X:Y$'
// Steam universe digit can vary, so we store as STEAM_0: but query with regex.
inline std::string SteamID64ToSuffix(uint64_t steamid64)
{
	// SteamID64 format: upper 32 bits = metadata, lower 32 bits = account info
	// Account ID = steamid64 & 0xFFFFFFFF
	// Y = AccountID & 1
	// Z = AccountID >> 1
	uint32_t accountId = static_cast<uint32_t>(steamid64 & 0xFFFFFFFF);
	uint32_t y = accountId & 1;
	uint32_t z = accountId >> 1;
	return std::to_string(y) + ":" + std::to_string(z);
}

inline std::string SteamID64ToAuthId(uint64_t steamid64)
{
	std::string suffix = SteamID64ToSuffix(steamid64);
	return "STEAM_0:" + suffix;
}

// STEAM_X:Y:Z or [U:1:account], in any case, as STEAM_0:Y:Z.
// Empty for anything else, a SteamID64 included, so callers can pick how to treat bare numbers.
inline std::string SteamID2Or3ToAuthId(const std::string &input)
{
	std::string s = input;
	for (char &c : s)
	{
		c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
	}

	unsigned int x = 0;
	unsigned int y = 0;
	unsigned int z = 0;
	int end = 0;
	if (sscanf(s.c_str(), "STEAM_%u:%u:%u%n", &x, &y, &z, &end) == 3 && end == static_cast<int>(s.size()))
	{
		return "STEAM_0:" + std::to_string(y) + ":" + std::to_string(z);
	}

	unsigned int universe = 0;
	unsigned int account = 0;
	end = 0;
	if (sscanf(s.c_str(), "[U:%u:%u]%n", &universe, &account, &end) == 2 && end == static_cast<int>(s.size()) && account != 0)
	{
		return "STEAM_0:" + std::to_string(account & 1) + ":" + std::to_string(account >> 1);
	}

	return "";
}

// STEAM_X:Y:Z, [U:1:account] or a SteamID64 as a SteamID64. 0 for anything else.
inline uint64_t ParseSteamID64(const std::string &input)
{
	const std::string authid = SteamID2Or3ToAuthId(input);
	unsigned int y = 0;
	unsigned int z = 0;
	if (!authid.empty() && sscanf(authid.c_str(), "STEAM_0:%u:%u", &y, &z) == 2)
	{
		return 76561197960265728ull + static_cast<uint64_t>(z) * 2 + y;
	}
	if (input.size() != 17 || input.compare(0, 4, "7656") != 0)
	{
		return 0;
	}
	for (char c : input)
	{
		if (!std::isdigit(static_cast<unsigned char>(c)))
		{
			return 0;
		}
	}
	return strtoull(input.c_str(), nullptr, 10);
}

// Extract the "X:Y" suffix from a "STEAM_0:X:Y" auth ID safely.
// Returns the input as-is if the format is unexpected.
inline std::string ExtractAuthSuffix(const std::string &authid)
{
	// Find second colon in "STEAM_0:X:Y" => skip past "STEAM_0:" (8 chars)
	if (authid.size() > 10 && authid[7] == ':' && authid[9] == ':')
	{
		return authid.substr(8);
	}

	// Fallback: try to find first colon manually, return everything after it
	size_t first = authid.find(':');
	if (first != std::string::npos && first + 1 < authid.size())
	{
		return authid.substr(first + 1);
	}

	return authid;
}

#endif // _INCLUDE_MMU_STEAM_UTILS_H_
