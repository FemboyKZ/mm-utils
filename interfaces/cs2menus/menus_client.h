#ifndef _INCLUDE_CS2MENUS_MENUS_CLIENT_H_
#define _INCLUDE_CS2MENUS_MENUS_CLIENT_H_

#include "interfaces/cs2menus/ics2menus.h"

#include "mmu/interface_bridge.h"

#include <algorithm>
#include <vector>

// A consumer plugin's link to mm-cs2menus for one-shot menus.
// Tracks what it shows per slot, so every menu is freed when its display ends and cancelled when this plugin unloads.
class CS2MenusClient
{
public:
	// How Present puts a menu on screen, see ICS2Menus.
	enum class Show
	{
		Display, // DisplayMenu, a fresh display
		Push,    // PushMenu, on top of the current menu with Back to it
		Replace, // ReplaceMenu, in place of the current menu
	};

	CS2MenusClient() : m_menus(CS2MENUS_INTERFACE) {}

	// Call from AllPluginsLoaded, OnPluginLoad and OnPluginUnload.
	mmu::BridgeChange Refresh()
	{
		mmu::BridgeChange change = m_menus.Refresh();
		if (change == mmu::BridgeChange::Unloaded)
		{
			// Handles belonged to the unloaded instance.
			ClearHandles();
		}
		return change;
	}

	// Call from Unload(). mm-cs2menus would otherwise keep lambdas that point into this plugin's unloaded code.
	void Shutdown()
	{
		// meta clear unloads plugins without firing OnPluginUnload, so the cached pointer can already be a freed library.
		if (m_menus.Revalidate())
		{
			for (int i = 0; i <= MAXPLAYERS; i++)
			{
				// CancelMenu ends the display and its whole history, whose end callbacks forget and destroy each menu.
				if (!m_live[i].empty())
				{
					m_menus->CancelMenu(i);
				}
				// Anything left was never on screen.
				for (MenuHandle menu : std::vector<MenuHandle>(m_live[i]))
				{
					m_menus->DestroyMenu(menu);
				}
			}
		}
		m_menus.Shutdown();
		ClearHandles();
	}

	bool Available() const
	{
		return m_menus.Available();
	}

	// Null when unloaded, check Available() first.
	ICS2Menus *operator->() const
	{
		return m_menus.Get();
	}

	ICS2Menus *Get() const
	{
		return m_menus.Get();
	}

	explicit operator bool() const
	{
		return m_menus.Available();
	}

	// Shows a menu built on Get() and destroys it once its display ends, after `onEnd` runs.
	// A menu in a history ends when the whole display does, or when it drops out of the history.
	// A refused display destroys it at once and returns false. `slot` must be in 0..MAXPLAYERS.
	bool Present(int slot, MenuHandle menu, float duration, MenuEndFn onEnd = nullptr, Show how = Show::Display)
	{
		m_menus->SetMenuEndCallback(menu,
									[this, onEnd](MenuHandle ended, int s, MenuEndReason reason)
									{
										Forget(s, ended);
										if (m_menus)
										{
											m_menus->DestroyMenu(ended);
										}
										if (onEnd)
										{
											onEnd(ended, s, reason);
										}
									});

		// Recorded before showing, since that can end other menus of this slot whose end callbacks run meanwhile.
		m_live[slot].push_back(menu);
		const bool shown = how == Show::Push      ? m_menus->PushMenu(menu, slot, duration)
						   : how == Show::Replace ? m_menus->ReplaceMenu(menu, slot, duration)
												  : m_menus->DisplayMenu(menu, slot, duration);
		if (!shown)
		{
			// Refused (a host menu owns the slot), so no end callback will ever free it.
			Forget(slot, menu);
			m_menus->DestroyMenu(menu);
			return false;
		}
		return true;
	}

private:
	void Forget(int slot, MenuHandle menu)
	{
		if (slot >= 0 && slot <= MAXPLAYERS)
		{
			std::vector<MenuHandle> &live = m_live[slot];
			live.erase(std::remove(live.begin(), live.end(), menu), live.end());
		}
	}

	void ClearHandles()
	{
		for (std::vector<MenuHandle> &live : m_live)
		{
			live.clear();
		}
	}

	mmu::InterfaceBridge<ICS2Menus> m_menus;
	// Menus of ours that are on screen or in a slot's history.
	std::vector<MenuHandle> m_live[MAXPLAYERS + 1];
};

#endif // _INCLUDE_CS2MENUS_MENUS_CLIENT_H_
