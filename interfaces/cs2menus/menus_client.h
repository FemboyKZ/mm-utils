#ifndef _INCLUDE_CS2MENUS_MENUS_CLIENT_H_
#define _INCLUDE_CS2MENUS_MENUS_CLIENT_H_

#include "interfaces/cs2menus/ics2menus.h"

#include "mmu/interface_bridge.h"

// A consumer plugin's link to mm-cs2menus for one-shot menus.
// Tracks what it shows per slot, so every menu is freed when its display ends and cancelled when this plugin unloads.
class CS2MenusClient
{
public:
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
			// CancelMenu fires the end callback, which clears the handle and destroys the menu.
			for (int i = 0; i <= MAXPLAYERS; i++)
			{
				if (m_handle[i] != kInvalidMenuHandle)
				{
					m_menus->CancelMenu(i);
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
	// A refused display destroys it at once and returns false. `slot` must be in 0..MAXPLAYERS.
	bool Present(int slot, MenuHandle menu, float duration, MenuEndFn onEnd = nullptr)
	{
		m_menus->SetMenuEndCallback(menu,
									[this, onEnd](MenuHandle ended, int s, MenuEndReason reason)
									{
										if (s >= 0 && s <= MAXPLAYERS && m_handle[s] == ended)
										{
											m_handle[s] = kInvalidMenuHandle;
										}
										if (m_menus)
										{
											m_menus->DestroyMenu(ended);
										}
										if (onEnd)
										{
											onEnd(ended, s, reason);
										}
									});

		// Recorded before DisplayMenu, since displaying replaces the slot's current menu
		// and that menu's end callback must not clear the handle just set.
		m_handle[slot] = menu;
		if (!m_menus->DisplayMenu(menu, slot, duration))
		{
			// Refused (a host menu owns the slot), so no end callback will ever free it.
			m_handle[slot] = kInvalidMenuHandle;
			m_menus->DestroyMenu(menu);
			return false;
		}
		return true;
	}

private:
	void ClearHandles()
	{
		for (MenuHandle &h : m_handle)
		{
			h = kInvalidMenuHandle;
		}
	}

	mmu::InterfaceBridge<ICS2Menus> m_menus;
	MenuHandle m_handle[MAXPLAYERS + 1] = {};
};

#endif // _INCLUDE_CS2MENUS_MENUS_CLIENT_H_
