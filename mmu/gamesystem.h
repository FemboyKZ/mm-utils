#ifndef _INCLUDE_MMU_GAMESYSTEM_H_
#define _INCLUDE_MMU_GAMESYSTEM_H_

namespace mmu
{
	namespace gamesystem
	{
		// Resolve the engine's game system factory list head (CBaseGameSystemFactory::sm_pFirst),
		// via mmu::gamedata::kGameSystemFactorySig. Returns false and logs on failure.
		// `serverModuleAnchor` is any pointer inside the server module (e.g. g_pServerGameDLL).
		bool Resolve(const void *serverModuleAnchor);

		// True once Resolve succeeded.
		bool Ready();

		// Walk the factory list and return the IGameSystem instance registered under `name`,
		// or null when unknown, not yet constructed, or Resolve hasn't run.
		void *FindByName(const char *name);
	} // namespace gamesystem
} // namespace mmu

#endif // _INCLUDE_MMU_GAMESYSTEM_H_
