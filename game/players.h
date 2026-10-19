#ifndef _INCLUDE_MMU_PLAYERS_H_
#define _INCLUDE_MMU_PLAYERS_H_

#include "game/player_table.h"
#include "utils/str.h"

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>

namespace mmu
{
	struct Player
	{
		bool connected = false;
		// Past ClientPutInServer. A client before it can't legitimately chat or run commands.
		bool inGame = false;
		bool fakePlayer = false;
		// Steam has confirmed steamid64. Until then it is only what the client claimed, so nothing may be granted on it.
		// Stays false for a bot, and without RunFrame.
		bool authenticated = false;
		uint64_t steamid64 = 0;
		// As of connecting. Empty after a late load, until the plugin fills it in.
		std::string name;
		// Without the port. Empty for a bot.
		std::string ip;
	};

	namespace players
	{
		// False for a free slot and for a bot, which has no SteamID.
		bool ReadLateLoad(int slot, Player &out);
		bool SteamConfirmed(int slot);
	} // namespace players

	// T adds the plugin's own fields, a new connect clears them with the rest.
	template<typename T = Player>
	class Players
	{
		static_assert(std::is_base_of_v<Player, T>);

	public:
		// Call from the plugin's own IServerGameClients hooks.
		void OnClientConnected(int slot, const char *name, uint64_t xuid, const char *address, bool fakePlayer)
		{
			// Whoever had the slot before is gone, and what Steam said of them with it.
			m_players.Clear(slot);
			if (T *p = m_players.Get(slot))
			{
				p->connected = true;
				p->fakePlayer = fakePlayer;
				p->steamid64 = xuid;
				p->name = name ? name : "";
				p->ip = str::StripPort(address);
			}
		}

		void OnClientPutInServer(int slot)
		{
			if (T *p = Get(slot))
			{
				p->inGame = true;
			}
		}

		void OnClientDisconnect(int slot)
		{
			m_players.Clear(slot);
		}

		// The players already on at a late load never pass through the hooks.
		void OnLateLoad()
		{
			for (int slot = 0; slot < MAXPLAYERS; slot++)
			{
				Player found;
				if (players::ReadLateLoad(slot, found))
				{
					T *p = m_players.Get(slot);
					*p = T();
					static_cast<Player &>(*p) = found;
				}
			}
		}

		// Call every frame. Steam confirms a SteamID some time after the client connects, before or after ClientPutInServer.
		// `onAuthenticated` runs once for each player when it does, and for the confirmed ones a late load found.
		void RunFrame(const std::function<void(int slot)> &onAuthenticated)
		{
			for (int slot = 0; slot < MAXPLAYERS; slot++)
			{
				T *p = Get(slot);
				if (!p || p->fakePlayer || p->authenticated || !players::SteamConfirmed(slot))
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

		// Null for a free slot.
		T *Get(int slot)
		{
			T *p = m_players.Get(slot);
			return p && p->connected ? p : nullptr;
		}

		const T *Get(int slot) const
		{
			const T *p = m_players.Get(slot);
			return p && p->connected ? p : nullptr;
		}

		// `p` must have come from Get.
		int SlotOf(const T &p) const
		{
			return static_cast<int>(&p - m_players.Get(0));
		}

		template<typename Pred>
		int Count(Pred pred) const
		{
			return m_players.Count([&pred](const T &p) { return p.connected && pred(p); });
		}

	private:
		PlayerTable<T> m_players;
	};
} // namespace mmu

#endif // _INCLUDE_MMU_PLAYERS_H_
