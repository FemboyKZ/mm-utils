#ifndef _INCLUDE_MMU_PLAYER_TABLE_H_
#define _INCLUDE_MMU_PLAYER_TABLE_H_

#include "sdk/plugin_globals.h"

namespace mmu
{
	// Bounds-checked per-slot storage.
	// Clear() assigns a fresh T, so T's member defaults are its cleared state.
	template<typename T>
	class PlayerTable
	{
	public:
		T *Get(int slot)
		{
			return mmu::ValidSlot(slot) ? &m_players[slot] : nullptr;
		}

		const T *Get(int slot) const
		{
			return mmu::ValidSlot(slot) ? &m_players[slot] : nullptr;
		}

		void Clear(int slot)
		{
			if (mmu::ValidSlot(slot))
			{
				m_players[slot] = T();
			}
		}

		template<typename Pred>
		int Count(Pred pred) const
		{
			int n = 0;
			for (int i = 0; i < MAXPLAYERS; i++)
			{
				if (pred(m_players[i]))
				{
					n++;
				}
			}
			return n;
		}

	private:
		T m_players[MAXPLAYERS] {};
	};
} // namespace mmu

#endif // _INCLUDE_MMU_PLAYER_TABLE_H_
