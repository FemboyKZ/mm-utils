#ifndef _INCLUDE_MMU_ENTITY_CGAMERULES_H_
#define _INCLUDE_MMU_ENTITY_CGAMERULES_H_

#include "sdk/schema.h"
#include "sdk/entity/cbaseentity.h"
#include <entity2/entitysystem.h> // GameTime_t

// Layout/field names from CS2Fixes (src/cs2_sdk/entity/cgamerules.h).
class CGameRules
{
public:
	DECLARE_SCHEMA_CLASS(CGameRules)
};

class CCSGameRules : public CGameRules
{
public:
	DECLARE_SCHEMA_CLASS(CCSGameRules)

	SCHEMA_FIELD(GameTime_t, m_flGameStartTime)
	SCHEMA_FIELD(GameTime_t, m_fRoundStartTime)
	SCHEMA_FIELD(GameTime_t, m_flRestartRoundTime)
	SCHEMA_FIELD(int32, m_gamePhase)

	// Offset only. A reader would invite treating this write-only fake as truth.
	SCHEMA_FIELD_OFFSET_FN(m_bGameRestart)

	// GAMEPHASE_MATCH_ENDED
	bool IsMatchEnded()
	{
		return m_gamePhase() == 5;
	}

	// The getters report resolution separately,
	// because an unresolved field reads as 0 and cs2kz legitimately pins both start times to 0.

	// Game time mp_timelimit counts from. Excludes warmup, and does not care when this plugin loaded.
	bool GetGameStartTime(float &out)
	{
		if (m_flGameStartTime_Offset() <= 0)
		{
			return false;
		}
		out = m_flGameStartTime().GetTime();
		return true;
	}

	// Game time mp_roundtime counts from.
	bool GetRoundStartTime(float &out)
	{
		if (m_fRoundStartTime_Offset() <= 0)
		{
			return false;
		}
		out = m_fRoundStartTime().GetTime();
		return true;
	}

	bool GetRestartRoundTime(float &out)
	{
		if (m_flRestartRoundTime_Offset() <= 0)
		{
			return false;
		}
		out = m_flRestartRoundTime().GetTime();
		return true;
	}

	// Deliberately not SCHEMA_FIELD_NETWORKED: CCSGameRules is not a CEntityInstance,
	// so there is no NetworkStateChanged to call, and this fake is server-side only.
	bool SetGameRestart(bool value)
	{
		const int16_t offset = m_bGameRestart_Offset();
		if (offset <= 0)
		{
			return false;
		}
		*reinterpret_cast<bool *>(reinterpret_cast<uintptr_t>(this) + offset) = value;
		return true;
	}
};

// The networked entity (classname "cs_gamerules") that owns the CCSGameRules object.
class CCSGameRulesProxy : public CBaseEntity
{
public:
	DECLARE_SCHEMA_CLASS(CCSGameRulesProxy)

	SCHEMA_FIELD(CCSGameRules *, m_pGameRules)
};

// The plugin's, null while no level is loaded.
extern CGameEntitySystem *g_pEntitySystem;

namespace mmu
{
	inline CCSGameRules *&GameRulesCache()
	{
		static CCSGameRules *rules = nullptr;
		return rules;
	}

	// Call ResetGameRules at level init and shutdown, the proxy entity is recreated each map.
	inline CCSGameRules *FindGameRules()
	{
		CCSGameRules *&rules = GameRulesCache();
		if (rules || !g_pEntitySystem)
		{
			return rules;
		}
		EntityInstanceByClassIter_t iter("cs_gamerules");
		if (CEntityInstance *inst = iter.First())
		{
			rules = static_cast<CCSGameRulesProxy *>(inst)->m_pGameRules();
		}
		return rules;
	}

	inline void ResetGameRules()
	{
		GameRulesCache() = nullptr;
	}
} // namespace mmu

#endif // _INCLUDE_MMU_ENTITY_CGAMERULES_H_
