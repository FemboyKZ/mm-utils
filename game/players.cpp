#include "game/players.h"
#include "sdk/plugin_globals.h"
#include "utils/str.h"

#include <inetchannelinfo.h>

namespace mmu
{
	namespace players
	{
		bool ReadLateLoad(int slot, Player &out)
		{
			CPlayerSlot playerSlot(slot);
			const uint64_t xuid = g_pEngine->GetClientXUID(playerSlot);
			INetChannelInfo *netInfo = g_pEngine->GetPlayerNetInfo(playerSlot);
			if (xuid == 0 || !netInfo)
			{
				return false;
			}
			out = Player {};
			out.connected = true;
			// ClientPutInServer fired before the plugin was there to see it.
			out.inGame = true;
			out.steamid64 = xuid;
			out.ip = str::StripPort(netInfo->GetAddress());
			return true;
		}

		bool SteamConfirmed(int slot)
		{
			return g_pEngine->IsClientFullyAuthenticated(CPlayerSlot(slot));
		}
	} // namespace players
} // namespace mmu
