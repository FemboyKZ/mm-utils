#ifndef _INCLUDE_MMU_SAY_H_
#define _INCLUDE_MMU_SAY_H_

#include "sdk/plugin_globals.h"

#include <tier0/convar.h>

namespace mmu
{
	// -1 unless a player's say or say_team, matched by registered name since a typed "SAY" still dispatches to say.
	inline int SaySlot(ConCommandRef cmd, const CCommandContext &ctx, bool *isTeam = nullptr)
	{
		const char *name = cmd.IsValidRef() ? cmd.GetName() : nullptr;
		const int slot = ctx.GetPlayerSlot().Get();
		if (!name || !ValidSlot(slot))
		{
			return -1;
		}
		const bool team = V_stricmp(name, "say_team") == 0;
		if (!team && V_stricmp(name, "say") != 0)
		{
			return -1;
		}
		if (isTeam)
		{
			*isTeam = team;
		}
		return slot;
	}
} // namespace mmu

#endif // _INCLUDE_MMU_SAY_H_
