#ifndef _INCLUDE_MMU_ADMIN_ACCESS_H_
#define _INCLUDE_MMU_ADMIN_ACCESS_H_

#include "interfaces/cs2admin/ics2admin.h"
#include "mmu/interface_bridge.h"

#include <cstdint>

namespace mmu
{
	// Consumer-side gate for mm-cs2admin's permission checks.
	//
	// Fallbacks when mm-cs2admin is absent are deliberately restrictive:
	// a command that named a required flag stays blocked rather than becoming open to everyone.
	// Only commands that asked for no flag at all (defaultFlag 0) stay usable.
	//
	// `group` is the override group name mm-cs2admin matches admin_overrides.cfg against, e.g. "cs2rtv".
	// It is fixed per consuming plugin.
	class AdminAccess
	{
	public:
		explicit AdminAccess(const char *group) : m_bridge(CS2ADMIN_INTERFACE), m_group(group) {}

		// Re-resolve mm-cs2admin. Call from OnPluginLoad and OnPluginUnload.
		BridgeChange Refresh()
		{
			return m_bridge.Refresh();
		}

		void Shutdown()
		{
			m_bridge.Shutdown();
		}

		bool Available() const
		{
			return m_bridge.Available();
		}

		ICS2Admin *Get() const
		{
			return m_bridge.Get();
		}

		// Override chain (group overrides, admin_overrides.cfg, then defaultFlag).
		// Server console (slot < 0) always passes.
		bool CanUseCommand(int slot, const char *commandName, uint32_t defaultFlag) const
		{
			if (slot < 0)
			{
				return true;
			}
			if (!m_bridge.Available())
			{
				return defaultFlag == 0;
			}
			return m_bridge->CanUseCommand(slot, commandName, m_group, defaultFlag);
		}

		// Raw flag test, bypassing the override chain.
		// Console always passes, and with mm-cs2admin absent nobody does.
		bool HasFlag(int slot, uint32_t flag) const
		{
			if (slot < 0)
			{
				return true;
			}
			if (!m_bridge.Available())
			{
				return false;
			}
			return m_bridge->HasFlag(slot, flag);
		}

		bool IsAdmin(int slot) const
		{
			return m_bridge.Available() && m_bridge->IsAdmin(slot);
		}

	private:
		InterfaceBridge<ICS2Admin> m_bridge;
		const char *m_group;
	};
} // namespace mmu

#endif // _INCLUDE_MMU_ADMIN_ACCESS_H_
