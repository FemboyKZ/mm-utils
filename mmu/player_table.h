#ifndef _INCLUDE_MMU_PLAYER_TABLE_H_
#define _INCLUDE_MMU_PLAYER_TABLE_H_

#include "mmu/plugin_globals.h"

namespace mmu
{
	// Bounds-checked per-slot storage, slots 0..MAXPLAYERS inclusive.
	// Clear() assigns a fresh T, so T's member defaults are its cleared state.
	template<typename T>
	class PlayerTable
	{
	public:
		static bool ValidSlot(int slot)
		{
			return slot >= 0 && slot <= MAXPLAYERS;
		}

		T *Get(int slot)
		{
			return ValidSlot(slot) ? &m_players[slot] : nullptr;
		}

		const T *Get(int slot) const
		{
			return ValidSlot(slot) ? &m_players[slot] : nullptr;
		}

		void Clear(int slot)
		{
			if (ValidSlot(slot))
			{
				m_players[slot] = T();
			}
		}

		template<typename Pred>
		int Count(Pred pred) const
		{
			int n = 0;
			for (int i = 0; i <= MAXPLAYERS; i++)
			{
				if (pred(m_players[i]))
				{
					n++;
				}
			}
			return n;
		}

	private:
		T m_players[MAXPLAYERS + 1] {};
	};
} // namespace mmu

#endif // _INCLUDE_MMU_PLAYER_TABLE_H_
