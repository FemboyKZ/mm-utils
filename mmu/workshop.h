#ifndef _INCLUDE_MMU_WORKSHOP_H_
#define _INCLUDE_MMU_WORKSHOP_H_

#include <steam/steam_gameserver.h>

#include <cstdint>
#include <string>
#include <vector>

namespace mmu
{
	namespace workshop
	{
		// Engine-native workshop registry, the CDedicatedServerWorkshopManager game system.
		// Requires mmu::gamesystem::Resolve to have succeeded.
		// Resolution is lazy and cached, every query below attempts it first.
		bool ResolveManager();
		bool ManagerReady();

		// True when the engine's own registry lists the workshop map as loaded/mounted.
		bool IsMapInstalled(uint64_t fileId);

		// All workshop file ids the engine currently lists as loaded.
		std::vector<uint64_t> InstalledMapIds();
	} // namespace workshop

	// Ensure a workshop map can be downloaded cleanly at map change.
	// If the engine's registry already lists the map as loaded, nothing to do.
	// Otherwise, if the addon has no .vpk on disk, prune its stale ACF entry
	// (WorkshopItemsInstalled + WorkshopItemDetails in appworkshop_730.acf)
	// so Steam re-downloads it, then ask SteamUGC to re-read the file.
	// `steamAPI` is the plugin's game-server API context, used for the re-read.
	// Returns true if a stale entry was pruned.
	bool EnsureWorkshopMapReady(const std::string &workshopId, CSteamGameServerAPIContext &steamAPI);
} // namespace mmu

#endif // _INCLUDE_MMU_WORKSHOP_H_
