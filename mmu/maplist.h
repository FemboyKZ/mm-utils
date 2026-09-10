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
