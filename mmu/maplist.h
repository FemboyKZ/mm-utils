#ifndef _INCLUDE_MMU_MAPLIST_H_
#define _INCLUDE_MMU_MAPLIST_H_

#include "mmu/str_utils.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace mmu
{
	// Fields parsed from one maplist.txt line.
	struct MapListEntry
	{
		std::string displayName; // e.g. "kz_grotto (T3, Linear)"
		std::string mapName;     // e.g. "kz_grotto"
		std::string workshopId;  // empty for a stock map
		bool isWorkshop = false;
	};

	// "kz_grotto (T3, Linear)" -> "kz_grotto".
	inline std::string StripMapAnnotation(const std::string &displayName)
	{
		size_t pos = displayName.find(" (");
		if (pos != std::string::npos)
		{
			return displayName.substr(0, pos);
		}
		return displayName;
	}

	// Length of a leading map prefix like "kz_", "bkz_" or "surf_", 0 without one.
	// Same rule as mm-cs2menus' MenuStyle::PagePrefixDelimiter "_".
	inline size_t MapPrefixLength(const std::string &name)
	{
		for (size_t i = 0; i < name.size() && i <= 5; i++)
		{
			if (name[i] == '_')
			{
				return i > 0 ? i + 1 : 0;
			}
			if (!std::isalnum(static_cast<unsigned char>(name[i])))
			{
				return 0;
			}
		}
		return 0;
	}

	// Case-insensitive map order ignoring the prefix, for menus since maplists are in file order.
	// The same name under different prefixes, like kz_grotto and bkz_grotto, falls back to the full name.
	inline bool MapNameLess(const std::string &a, const std::string &b)
	{
		auto less = [](const char *x, const char *xEnd, const char *y, const char *yEnd)
		{ return std::lexicographical_compare(x, xEnd, y, yEnd, [](char c, char d) { return std::tolower((unsigned char)c) < std::tolower((unsigned char)d); }); };
		const char *aEnd = a.c_str() + a.size();
		const char *bEnd = b.c_str() + b.size();
		const char *aKey = a.c_str() + MapPrefixLength(a);
		const char *bKey = b.c_str() + MapPrefixLength(b);
		if (less(aKey, aEnd, bKey, bEnd))
		{
			return true;
		}
		if (less(bKey, bEnd, aKey, aEnd))
		{
			return false;
		}
		return less(a.c_str(), aEnd, b.c_str(), bEnd);
	}

	// Line is "mapname" or "displayname:workshopid". Returns false for blanks and comments.
	// Splits on the last colon, and only an all-digit tail counts as a workshop id.
	inline bool ParseMapListLine(const std::string &rawLine, MapListEntry &out)
	{
		std::string line = str::Trim(rawLine);

		if (line.empty() || line[0] == '#' || line[0] == '/' || line[0] == ';')
		{
			return false;
		}

		size_t colonPos = line.rfind(':');
		if (colonPos != std::string::npos)
		{
			std::string tail = str::Trim(line.substr(colonPos + 1));
			bool allDigits = !tail.empty() && std::all_of(tail.begin(), tail.end(), [](unsigned char c) { return std::isdigit(c) != 0; });

			if (allDigits)
			{
				out.displayName = str::Trim(line.substr(0, colonPos));
				out.workshopId = tail;
				out.mapName = StripMapAnnotation(out.displayName);
				out.isWorkshop = true;
				return true;
			}
		}

		out.displayName = line;
		out.workshopId.clear();
		out.mapName = StripMapAnnotation(line);
		out.isWorkshop = false;
		return true;
	}
} // namespace mmu

#endif // _INCLUDE_MMU_MAPLIST_H_
