#ifndef _INCLUDE_MMU_WORKSHOP_H_
#define _INCLUDE_MMU_WORKSHOP_H_

#include <steam/steam_gameserver.h>

#include <string>

namespace mmu
{
	// If the workshop addon has no .vpk on disk, prune its stale ACF entry
	// (WorkshopItemsInstalled + WorkshopItemDetails in appworkshop_730.acf)
	// so Steam re-downloads it, then ask SteamUGC to re-read the file.
	// `steamAPI` is the plugin's game-server API context, used for the re-read.
	// Returns true if a stale entry was pruned.
	bool EnsureWorkshopMapReady(const std::string &workshopId, CSteamGameServerAPIContext &steamAPI);
} // namespace mmu

#endif // _INCLUDE_MMU_WORKSHOP_H_
