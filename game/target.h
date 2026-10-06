#ifndef _INCLUDE_MMU_TARGET_H_
#define _INCLUDE_MMU_TARGET_H_

#include "sdk/entity/ccsplayercontroller.h"
#include "sdk/plugin_globals.h"
#include "utils/steamid.h"
#include "utils/str.h"

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <random>
#include <string>
#include <vector>

namespace mmu
{
	struct TargetCandidate
	{
		uint64_t steamid64 = 0;
		bool bot = false;
	};

	// The plugin's own view of who is connected. False for a slot with nobody in it.
	using TargetLookup = std::function<bool(int slot, TargetCandidate &out)>;

	enum class TargetMode
	{
		One,
		// A SteamID nobody online has still resolves, for a player's saved data.
		OneOrOffline,
		// @groups too.
		Many,
	};

	struct TargetResult
	{
		std::vector<int> slots;
		// Set, with no slots, for an OneOrOffline SteamID with nobody online.
		uint64_t offlineSteamid64 = 0;
		// Came from an @group.
		bool group = false;
		// An English phrase to translate, empty on success.
		std::string error;
	};

	// Who `pattern` names, from `caller`'s view, -1 for the console:
	//   @me, and for Many: @all, @humans, @bots, @t, @ct, @spec, @alive, @dead (humans only) and @random (one human)
	//   #slot, $steamid64, a bare slot number, STEAM_X:Y:Z, [U:1:X] or a SteamID64
	//   &name for an exact name, else a name or a unique part of one, an exact name winning over parts
	inline TargetResult FindTargets(int caller, const std::string &pattern, const TargetLookup &lookup, TargetMode mode = TargetMode::One)
	{
		TargetResult result;
		auto fail = [&result](const char *error)
		{
			result.slots.clear();
			result.error = error;
			return result;
		};
		auto connected = [&lookup](int slot, TargetCandidate &who) { return slot >= 0 && slot <= MAXPLAYERS && lookup(slot, who); };
		auto bySteamID = [&](uint64_t steamid64)
		{
			TargetCandidate who;
			for (int i = 0; i <= MAXPLAYERS; i++)
			{
				if (connected(i, who) && !who.bot && who.steamid64 == steamid64)
				{
					result.slots.push_back(i);
					return result;
				}
			}
			if (mode == TargetMode::OneOrOffline)
			{
				result.offlineSteamid64 = steamid64;
				return result;
			}
			return fail("No player with that SteamID is online.");
		};
		auto bySlot = [&](const char *digits)
		{
			char *end = nullptr;
			const long slot = std::strtol(digits, &end, 10);
			TargetCandidate who;
			if (end == digits || *end != '\0' || !connected(static_cast<int>(slot), who))
			{
				return false;
			}
			result.slots.push_back(static_cast<int>(slot));
			return true;
		};

		const std::string pat = str::Trim(pattern);
		if (pat.empty())
		{
			return fail("No player found matching that name.");
		}

		if (pat[0] == '@')
		{
			const std::string group = str::ToLower(pat.substr(1));
			TargetCandidate who;
			if (group == "me")
			{
				if (!connected(caller, who))
				{
					return fail("You are not in-game.");
				}
				result.slots.push_back(caller);
				return result;
			}
			if (mode != TargetMode::Many)
			{
				return fail("Multiple players matched. Use a more specific target.");
			}
			result.group = group != "random";
			for (int i = 0; i <= MAXPLAYERS; i++)
			{
				if (!connected(i, who))
				{
					continue;
				}
				CCSPlayerController *controller = CCSPlayerController::FromSlot(i);
				const int team = controller ? controller->m_iTeamNum() : CS_TEAM_NONE;
				const bool alive = controller && controller->m_bPawnIsAlive();
				bool match = false;
				if (group == "bot" || group == "bots")
				{
					match = who.bot;
				}
				else if (!who.bot)
				{
					match = group == "all" || group == "human" || group == "humans" || group == "random" || (group == "t" && team == CS_TEAM_T)
							|| (group == "ct" && team == CS_TEAM_CT) || (group == "spec" && team == CS_TEAM_SPECTATOR) || (group == "alive" && alive)
							|| (group == "dead" && !alive);
				}
				if (match)
				{
					result.slots.push_back(i);
				}
			}
			if (result.slots.empty())
			{
				return fail("No matching players found.");
			}
			if (group == "random")
			{
				static std::mt19937 rng(std::random_device {}());
				const int picked = result.slots[std::uniform_int_distribution<size_t>(0, result.slots.size() - 1)(rng)];
				result.slots = {picked};
			}
			return result;
		}

		if (pat[0] == '$')
		{
			char *end = nullptr;
			const uint64_t steamid64 = std::strtoull(pat.c_str() + 1, &end, 10);
			if (end == pat.c_str() + 1 || *end != '\0' || steamid64 == 0)
			{
				return fail("Invalid SteamID64.");
			}
			return bySteamID(steamid64);
		}

		if (pat[0] == '#')
		{
			return bySlot(pat.c_str() + 1) ? result : fail("Player not found with that slot/userid.");
		}

		if (const uint64_t steamid64 = ParseSteamID64(pat))
		{
			return bySteamID(steamid64);
		}

		if (bySlot(pat.c_str()))
		{
			return result;
		}

		const bool exactOnly = pat[0] == '&';
		const std::string search = str::ToLower(exactOnly ? pat.substr(1) : pat);
		std::vector<int> parts;
		TargetCandidate who;
		for (int i = 0; i <= MAXPLAYERS; i++)
		{
			CCSPlayerController *controller = connected(i, who) ? CCSPlayerController::FromSlot(i) : nullptr;
			if (!controller)
			{
				continue;
			}
			const std::string name = str::ToLower(controller->GetPlayerName());
			if (name == search)
			{
				result.slots.push_back(i);
				return result;
			}
			if (!exactOnly && name.find(search) != std::string::npos)
			{
				parts.push_back(i);
			}
		}
		if (exactOnly)
		{
			return fail("No player found with exact name.");
		}
		if (parts.size() > 1)
		{
			return fail("Multiple players match that name. Be more specific.");
		}
		if (parts.empty())
		{
			return fail("No player found matching that name.");
		}
		result.slots = parts;
		return result;
	}
} // namespace mmu

#endif // _INCLUDE_MMU_TARGET_H_
