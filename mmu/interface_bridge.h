#ifndef _INCLUDE_MMU_INTERFACE_BRIDGE_H_
#define _INCLUDE_MMU_INTERFACE_BRIDGE_H_

#include "mmu/plugin_globals.h"

namespace mmu
{
	// Refresh() result, so callers log only on a transition.
	enum class BridgeChange
	{
		Unchanged, // still present, or still absent
		Loaded,
		Unloaded,
	};

	// Cached pointer to another plugin's exported interface.
	// Call Refresh() from OnPluginLoad and OnPluginUnload, an unload leaves the pointer dangling.
	template<typename T>
	class InterfaceBridge
	{
	public:
		explicit InterfaceBridge(const char *interfaceName) : m_name(interfaceName) {}

		BridgeChange Refresh()
		{
			T *prev = m_iface;
			m_iface = nullptr;

			if (g_SMAPI)
			{
				m_iface = static_cast<T *>(g_SMAPI->MetaFactory(m_name, nullptr, nullptr));
			}

			if (m_iface && !prev)
			{
				return BridgeChange::Loaded;
			}
			if (!m_iface && prev)
			{
				return BridgeChange::Unloaded;
			}
			return BridgeChange::Unchanged;
		}

		// Call from plugin Unload().
		void Shutdown()
		{
			m_iface = nullptr;
		}

		bool Available() const
		{
			return m_iface != nullptr;
		}

		T *Get() const
		{
			return m_iface;
		}

		// Null when unloaded, check Available() first.
		T *operator->() const
		{
			return m_iface;
		}

		explicit operator bool() const
		{
			return m_iface != nullptr;
		}

	private:
		const char *m_name;
		T *m_iface = nullptr;
	};
} // namespace mmu

#endif // _INCLUDE_MMU_INTERFACE_BRIDGE_H_
