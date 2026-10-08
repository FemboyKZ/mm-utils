#ifndef _INCLUDE_MMU_PLAYERS_H_
#define _INCLUDE_MMU_PLAYERS_H_

#include <cstdint>
#include <functional>
#include <string>

namespace mmu
{
	struct Player
	{
		bool connected = false;
		// Past ClientPutInServer. A client before it can't legitimately chat or run commands.
		bool inGame = false;
		bool fakePlayer = false;
		// Steam has confirmed steamid64. Until then it is only what the client claimed, so nothing may be granted on it.
		// Stays false for a bot.
		bool authenticated = false;
		uint64_t steamid64 = 0;
		// As of connecting. Empty after a late load, until the plugin fills it in.
		std::string name;
		// Without the port. Empty for a bot.
		std::string ip;
	};

	namespace players
	{
		// Call from the plugin's own IServerGameClients hooks.
		void OnClientConnected(int slot, const char *name, uint64_t xuid, const char *address, bool fakePlayer);
		void OnClientPutInServer(int slot);
		void OnClientDisconnect(int slot);

		// The players already on at a late load never pass through the hooks.
		void OnLateLoad();

		// Call every frame. Steam confirms a SteamID some time after the client connects, before or after ClientPutInServer.
		// `onAuthenticated` runs once for each player when it does, and for the confirmed ones a late load found.
		void RunFrame(const std::function<void(int slot)> &onAuthenticated);

		// Null for a free slot. Writable, so a plugin can keep `name` up with renames.
		Player *Get(int slot);
	} // namespace players
} // namespace mmu

#endif // _INCLUDE_MMU_PLAYERS_H_
