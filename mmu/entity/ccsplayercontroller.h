#ifndef _INCLUDE_MMU_ENTITY_CCSPLAYERCONTROLLER_H_
#define _INCLUDE_MMU_ENTITY_CCSPLAYERCONTROLLER_H_

#include "mmu/schema.h"
#include "mmu/entity/cbaseentity.h"
#include "mmu/entity/ccsplayerpawn.h"
#include <entity2/entitysystem.h>
#include <entity2/entityidentity.h>
#include <entityhandle.h>
#include <ehandle.h>
#include <tier1/utlsymbollarge.h>

// CBasePlayerController : CBaseEntity
class CBasePlayerController : public CBaseEntity
{
public:
	DECLARE_SCHEMA_CLASS(CBasePlayerController)

	SCHEMA_FIELD(CHandle<CBasePlayerPawn>, m_hPawn)

	// The pawn currently being controlled.
	// May be the player's own pawn, an observer pawn while dead or spectating, or a bot pawn.
	// Use GetPlayerPawn for the real one.
	CBasePlayerPawn *GetPawn()
	{
		return m_hPawn().Get();
	}
};

// CCSPlayerController : CBasePlayerController
class CCSPlayerController : public CBasePlayerController
{
public:
	DECLARE_SCHEMA_CLASS(CCSPlayerController)

	SCHEMA_FIELD(bool, m_bPawnIsAlive)
	SCHEMA_FIELD(uint32_t, m_iPawnHealth)
	SCHEMA_FIELD(CHandle<CCSPlayerPawn>, m_hPlayerPawn)
	SCHEMA_FIELD(CHandle<CBasePlayerPawn>, m_hObserverPawn)

	// Scoreboard clan tag. Prefer SetClan over Setm_szClan, see the lifetime note there.
	SCHEMA_FIELD_NETWORKED(CUtlSymbolLarge, m_szClan)

	// Set the scoreboard clan tag.
	//
	// CUtlSymbolLarge stores the bare pointer it is handed, it does not copy the string.
	// The caller therefore owns `clan` and must keep it alive for as long as the tag is displayed,
	// so a std::string temporary or a local buffer will dangle.
	// Hold the backing storage somewhere with the player's lifetime.
	void SetClan(const char *clan)
	{
		if (!clan)
		{
			return;
		}
		Setm_szClan(CUtlSymbolLarge(clan));
	}

	// The player's own pawn, regardless of what they're currently controlling.
	CCSPlayerPawn *GetPlayerPawn()
	{
		return m_hPlayerPawn().Get();
	}

	// Pawn to read button input from: the engine repoints m_hPawn to the controlled pawn
	// (player pawn when alive, observer pawn when dead/spectating), so prefer it.
	// Fall back to the observer pawn handle if m_hPawn is momentarily unset.
	CBasePlayerPawn *GetInputPawn()
	{
		if (CBasePlayerPawn *current = m_hPawn().Get())
		{
			return current;
		}
		return m_hObserverPawn().Get();
	}

	// Get controller from player slot (slot 0 -> entity index 1).
	// Returns null when the slot holds no live entity.
	static CCSPlayerController *FromSlot(int slot)
	{
		extern CGameEntitySystem *g_pEntitySystem;
		if (!g_pEntitySystem)
		{
			return nullptr;
		}

		return static_cast<CCSPlayerController *>(g_pEntitySystem->GetEntityInstance(CEntityIndex(slot + 1)));
	}
};

// CS2 team constants
constexpr int CS_TEAM_NONE = 0;
constexpr int CS_TEAM_SPECTATOR = 1;
constexpr int CS_TEAM_T = 2;
constexpr int CS_TEAM_CT = 3;

#endif // _INCLUDE_MMU_ENTITY_CCSPLAYERCONTROLLER_H_
