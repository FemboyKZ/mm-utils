#include "game/players.h"
#include "game/player_table.h"
#include "sdk/plugin_globals.h"
#include "utils/str.h"

#include <inetchannelinfo.h>

namespace
{
	mmu::PlayerTable<mmu::Player> s_players;
} // namespace

namespace mmu
{
	namespace players
	{
		void OnClientConnected(int slot, const char *name, uint64_t xuid, const char *address, bool fakePlayer)
		{
			// Whoever had the slot before is gone, and what Steam said of them with it.
			s_players.Clear(slot);
			Player *p = s_players.Get(slot);
			if (!p)
			{
				return;
			}
			p->connected = true;
			p->fakePlayer = fakePlayer;
			p->steamid64 = xuid;
			p->name = name ? name : "";
			p->ip = str::StripPort(address);
		}

		void OnClientPutInServer(int slot)
		{
			if (Player *p = Get(slot))
			{
				p->inGame = true;
			}
		}

		void OnClientDisconnect(int slot)
		{
			s_players.Clear(slot);
		}

		void OnLateLoad()
		{
			for (int slot = 0; slot < MAXPLAYERS; slot++)
			{
				CPlayerSlot playerSlot(slot);
				const uint64_t xuid = g_pEngine->GetClientXUID(playerSlot);
				INetChannelInfo *netInfo = g_pEngine->GetPlayerNetInfo(playerSlot);
				// A bot has no SteamID and is skipped.
				if (xuid == 0 || !netInfo)
				{
					continue;
				}
				Player *p = s_players.Get(slot);
				*p = Player {};
				p->connected = true;
				// Anyone with a SteamID is taken to be past ClientPutInServer, which fired before the plugin was there to see it.
				p->inGame = true;
				p->steamid64 = xuid;
				p->ip = str::StripPort(netInfo->GetAddress());
			}
		}

		void RunFrame(const std::function<void(int slot)> &onAuthenticated)
		{
			for (int slot = 0; slot < MAXPLAYERS; slot++)
			{
				Player *p = Get(slot);
				if (!p || p->fakePlayer || p->authenticated || !g_pEngine->IsClientFullyAuthenticated(CPlayerSlot(slot)))
				{
					continue;
				}
				p->authenticated = true;
				// May kick the player, `p` is not read after it.
				if (onAuthenticated)
				{
					onAuthenticated(slot);
				}
			}
		}

		Player *Get(int slot)
		{
			Player *p = s_players.Get(slot);
			return p && p->connected ? p : nullptr;
		}
	} // namespace players
} // namespace mmu
