#ifndef _INCLUDE_MMU_INTERFACE_BRIDGE_H_
#define _INCLUDE_MMU_INTERFACE_BRIDGE_H_

#include "mmu/plugin_globals.h"

namespace mmu
{
	// What a Refresh() did to the cached pointer, so the caller can log its own wording and only on an actual transition.
	enum class BridgeChange
	{
		Unchanged, // still there, or still missing
		Loaded,    // was missing, resolved now
		Unloaded,  // was there, gone now
	};

	// Cached pointer to another Metamod plugin's exported interface.
	//
	// Refresh() must run from IMetamodListener::OnPluginLoad and OnPluginUnload:
	// the pointer belongs to the other plugin's code, so an unload leaves it dangling.
	// It re-resolves unconditionally rather than trusting the cached value, since a plugin can be reloaded in place.
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
				// MetaFactory searches every loaded plugin's factory by interface name.
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

		// Drop the pointer without re-resolving. Call from plugin Unload().
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

		// Null when the other plugin isn't loaded, so guard with Available() first.
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
