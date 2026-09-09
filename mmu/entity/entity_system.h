#ifndef _INCLUDE_MMU_ENTITY_ENTITY_SYSTEM_H_
#define _INCLUDE_MMU_ENTITY_ENTITY_SYSTEM_H_

#include "mmu/gamedata.h"

#include <entity2/entitysystem.h>
#include <interfaces/interfaces.h>

namespace mmu
{
	// Resolve CGameEntitySystem through IGameResourceService.
	// g_pGameResourceServiceServer comes from the SDK's interfaces.lib.
	//
	// The SDK declares a free `GameEntitySystem()` (entity2/entitysystem.h) and its own entity2 sources call it,
	// so each plugin must still define that symbol itself. Define it as a forwarder to this:
	//
	//   CGameEntitySystem *GameEntitySystem() { return mmu::EntitySystem(); }
	//
	// Null until the server module hands out the resource service,
	// and the pointer behind the offset is republished per level, so re-read it on map start rather than caching it across maps.
	inline CGameEntitySystem *EntitySystem()
	{
		if (!g_pGameResourceServiceServer)
		{
			return nullptr;
		}

		return *reinterpret_cast<CGameEntitySystem **>(reinterpret_cast<uintptr_t>(g_pGameResourceServiceServer) + gamedata::kGameEntitySystemOffset);
	}
} // namespace mmu

#endif // _INCLUDE_MMU_ENTITY_ENTITY_SYSTEM_H_
