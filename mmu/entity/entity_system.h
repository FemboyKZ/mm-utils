#ifndef _INCLUDE_MMU_ENTITY_ENTITY_SYSTEM_H_
#define _INCLUDE_MMU_ENTITY_ENTITY_SYSTEM_H_

#include "mmu/gamedata.h"

#include <entity2/entitysystem.h>
#include <interfaces/interfaces.h>

namespace mmu
{
	// The SDK's entity2 code calls a free GameEntitySystem() that each plugin must define.
	// Forward it here:  CGameEntitySystem *GameEntitySystem() { return mmu::EntitySystem(); }
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
