#ifndef _INCLUDE_MMU_ENTITY_CCSPLAYERPAWN_H_
#define _INCLUDE_MMU_ENTITY_CCSPLAYERPAWN_H_

#include "mmu/schema.h"
#include "mmu/entity/cbaseentity.h"
#include "mmu/entity/in_buttons.h"

#include <cstdint>

// Matches CPlayerPawnComponent vtable layout from the game.
// We define all virtual methods to ensure correct vtable indices for derived classes.
class CPlayerPawnComponent
{
	virtual void unk_00() = 0;
	virtual void unk_01() = 0;
	virtual void unk_02() = 0;
	virtual void unk_03() = 0;
	virtual void unk_04() = 0;
	virtual void unk_05() = 0;
	virtual ~CPlayerPawnComponent() = 0;
	virtual void unk_07() = 0;
	virtual void unk_08() = 0;
	virtual void unk_09() = 0;
	virtual void unk_10() = 0;
	virtual void unk_11() = 0;
	virtual void unk_12() = 0;
	virtual void unk_13() = 0;
	virtual void unk_14() = 0;
	virtual void unk_15() = 0;
	virtual void unk_16() = 0;
	virtual void unk_17() = 0;
	virtual void unk_18() = 0;
	virtual void unk_19() = 0;
	virtual void unk_20() = 0;
	virtual void unk_21() = 0;
	virtual void unk_22() = 0;
};

class CPlayer_ItemServices : public CPlayerPawnComponent
{
	virtual ~CPlayer_ItemServices() = 0;
};

// CCSPlayer_ItemServices virtual method layout (matches CS2Fixes).
class CCSPlayer_ItemServices : public CPlayer_ItemServices
{
	virtual ~CCSPlayer_ItemServices() = 0;

private:
	virtual void *_GiveNamedItem(const char *name) = 0;

public:
	virtual bool GiveNamedItemBool(const char *name) = 0;
	virtual void *GiveNamedItem(const char *name) = 0;
	virtual void DropActiveWeapon(void *weapon) = 0;
	virtual void StripPlayerWeapons(bool removeSuit) = 0;
};

// CBasePlayerPawn : CBaseEntity
// Schema class name must match game's class for field resolution.
class CBasePlayerPawn : public CBaseEntity
{
public:
	DECLARE_SCHEMA_CLASS(CBasePlayerPawn)

	SCHEMA_FIELD(CCSPlayer_ItemServices *, m_pItemServices)

	// The pawn's CInButtonState::m_pButtonStates array, or nullptr if unavailable.
	const uint64_t *GetButtonStates()
	{
		// Cache only once all three resolve. A 0 (schema not ready yet) is not cached,
		// so an early call can't poison the offsets for the life of the process.
		static int16_t offMovement = 0;
		static int16_t offButtons = 0;
		static int16_t offStates = 0;
		if (offMovement <= 0 || offButtons <= 0 || offStates <= 0)
		{
			offMovement = schema::GetOffset("CBasePlayerPawn", FNV1a("CBasePlayerPawn"), "m_pMovementServices", FNV1a("m_pMovementServices"));
			offButtons = schema::GetOffset("CPlayer_MovementServices", FNV1a("CPlayer_MovementServices"), "m_nButtons", FNV1a("m_nButtons"));
			offStates = schema::GetOffset("CInButtonState", FNV1a("CInButtonState"), "m_pButtonStates", FNV1a("m_pButtonStates"));
		}

		if (offMovement <= 0 || offButtons <= 0 || offStates <= 0)
		{
			return nullptr;
		}

		void *services = *reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(this) + offMovement);
		if (!services)
		{
			return nullptr;
		}

		uintptr_t buttonState = reinterpret_cast<uintptr_t>(services) + offButtons;
		return reinterpret_cast<const uint64_t *>(buttonState + offStates);
	}

	// Buttons still held at the end of the tick, or 0 if unavailable.
	uint64_t GetHeldButtons()
	{
		const uint64_t *states = GetButtonStates();
		return states ? states[0] : 0;
	}

	// Buttons that saw an up->down transition during the tick, or 0 if unavailable.
	// This catches taps pressed and released inside a single tick, which never show up in the held mask.
	uint64_t GetPressedButtons()
	{
		const uint64_t *states = GetButtonStates();
		return states ? (states[2] | (states[0] & states[1])) : 0;
	}
};

// CCSPlayerPawn : CBasePlayerPawn (via CCSPlayerPawnBase in game hierarchy)
class CCSPlayerPawn : public CBasePlayerPawn
{
public:
	DECLARE_SCHEMA_CLASS(CCSPlayerPawn)
};

#endif // _INCLUDE_MMU_ENTITY_CCSPLAYERPAWN_H_
