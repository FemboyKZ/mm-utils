#ifndef _INCLUDE_MMU_SERVER_CLIENT_H_
#define _INCLUDE_MMU_SERVER_CLIENT_H_

#include "mmu/sigscan.h"

#include <cstdint>

// Only ever handled as an opaque vtable owner.
class CServerSideClient;

namespace mmu
{
	// Indices follow the CServerSideClientBase declaration order in both the CS2Fixes and cs2kz SDK headers,
	// so a game update that reorders it moves every one of them together.
	namespace serverclient
	{
#ifdef _WIN32
		constexpr uint32_t kProcessVoiceDataIndex = 37;
		constexpr uint32_t kProcessRespondCvarValueIndex = 38;
#else
		constexpr uint32_t kProcessVoiceDataIndex = 39;
		constexpr uint32_t kProcessRespondCvarValueIndex = 40;
#endif
		constexpr int kSlotOffset = 72;

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
		inline CServerSideClient *HookTarget()
		{
			void *&vtable = VtableStorage();
			return vtable ? reinterpret_cast<CServerSideClient *>(&vtable) : nullptr;
		}

		inline int Slot(const CServerSideClient *client)
		{
			return *reinterpret_cast<const int *>(reinterpret_cast<const uint8_t *>(client) + kSlotOffset);
		}
	} // namespace serverclient
} // namespace mmu

#endif // _INCLUDE_MMU_SERVER_CLIENT_H_
