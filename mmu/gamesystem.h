#ifndef _INCLUDE_MMU_GAMESYSTEM_H_
#define _INCLUDE_MMU_GAMESYSTEM_H_

#include <cstddef>
#include <cstdint>

namespace mmu
{
	namespace gamesystem
	{
		// Resolve the engine's game system factory list head (CBaseGameSystemFactory::sm_pFirst).
		// `serverModuleAnchor` is any pointer inside the server module (e.g. g_pServerGameDLL).
		// `signature` must match a single RIP-relative mov whose displacement starts 3 bytes in,
		// e.g. `48 8B 1D <disp32>` loading sm_pFirst. Returns false and logs on failure.
		bool Resolve(const void *serverModuleAnchor, const uint8_t *signature, size_t sigLen);

		// True once Resolve succeeded.
		bool Ready();

		// Walk the factory list and return the IGameSystem instance registered under `name`,
		// or null when unknown, not yet constructed, or Resolve hasn't run.
		void *FindByName(const char *name);
	} // namespace gamesystem
} // namespace mmu

#endif // _INCLUDE_MMU_GAMESYSTEM_H_
