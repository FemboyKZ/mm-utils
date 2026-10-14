#ifndef _INCLUDE_MMU_SERVER_CLIENT_H_
#define _INCLUDE_MMU_SERVER_CLIENT_H_

#include "sdk/sigscan.h"

#include <engine2/serversideclientbase.h>

namespace mmu
{
	namespace serverclient
	{
		// Shared by everything in one module that hooks the vtable.
		// It never gets cleared, the vtable outlives any plugin, and a hook added through it has to be removable after another user shuts down.
		inline void *&VtableStorage()
		{
			static void *vtable = nullptr;
			return vtable;
		}

		// `engineModuleAnchor` is any pointer inside engine2, e.g. g_pEngine.
		inline bool Resolve(const void *engineModuleAnchor)
		{
			void *&vtable = VtableStorage();
			if (!vtable && engineModuleAnchor)
			{
				vtable = sig::FindVirtualTable(engineModuleAnchor, "CServerSideClient");
			}
			return vtable != nullptr;
		}

		// What KHook's AddGlobal/RemoveGlobal want, since they read the vtable pointer out of the object they are handed.
		// Null until Resolve succeeds.
		inline CServerSideClientBase *HookTarget()
		{
			void *&vtable = VtableStorage();
			return vtable ? reinterpret_cast<CServerSideClientBase *>(&vtable) : nullptr;
		}
	} // namespace serverclient
} // namespace mmu

#endif // _INCLUDE_MMU_SERVER_CLIENT_H_
