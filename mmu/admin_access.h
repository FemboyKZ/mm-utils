#ifndef _INCLUDE_MMU_ADMIN_ACCESS_H_
#define _INCLUDE_MMU_ADMIN_ACCESS_H_

#include "interfaces/cs2admin/ics2admin.h"
#include "mmu/interface_bridge.h"

#include <cstdint>

namespace mmu
{
	// mm-cs2admin permission checks for consumer plugins.
	// Without mm-cs2admin, only commands with defaultFlag 0 stay usable.
	// `group` is the admin_overrides.cfg group, e.g. "cs2rtv".
	class AdminAccess
	{
	public:
		explicit AdminAccess(const char *group) : m_bridge(CS2ADMIN_INTERFACE), m_group(group) {}

		// Call from OnPluginLoad and OnPluginUnload.
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

		// Group overrides, then admin_overrides.cfg, then defaultFlag. Console always passes.
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

		// Skips overrides. Console always passes, nobody else does without mm-cs2admin.
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
