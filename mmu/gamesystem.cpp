#include "mmu/gamesystem.h"
#include "mmu/log.h"
#include "mmu/sigscan.h"

#include <cstring>

namespace
{

	// Factory node layout, from the engine:
	// vtable pointer, then the intrusive list link, channel name, and instance.
	struct GameSystemFactory
	{
		void *vtable;            // 0x00
		GameSystemFactory *next; // 0x08
		const char *name;        // 0x10
		void *instance;          // 0x18
	};

	GameSystemFactory **g_ppFirst = nullptr;

} // namespace

namespace mmu
{
	namespace gamesystem
	{

		bool Resolve(const void *serverModuleAnchor, const uint8_t *signature, size_t sigLen)
		{
			if (g_ppFirst)
			{
				return true;
			}

			void *base = nullptr;
			size_t size = 0;
			if (!sig::GetModuleRange(serverModuleAnchor, base, size))
			{
				MMU_LOG_WARN("GameSystem: could not locate server module range.\n");
				return false;
			}

			bool multiple = false;
			void *insn = sig::FindSignatureUnique(base, size, signature, sigLen, multiple);
			if (!insn)
			{
				MMU_LOG_WARN("GameSystem: sm_pFirst signature not found.\n");
				return false;
			}
			if (multiple)
			{
				MMU_LOG_WARN("GameSystem: sm_pFirst signature matched multiple times, refusing.\n");
				return false;
			}

			g_ppFirst = static_cast<GameSystemFactory **>(sig::ResolveRipRelative(insn));
			return g_ppFirst != nullptr;
		}

		bool Ready()
		{
			return g_ppFirst != nullptr;
		}

		void *FindByName(const char *name)
		{
			if (!g_ppFirst || !name)
			{
				return nullptr;
			}

			for (GameSystemFactory *it = *g_ppFirst; it; it = it->next)
			{
				if (it->name && strcmp(it->name, name) == 0)
				{
					return it->instance;
				}
			}
			return nullptr;
		}

	} // namespace gamesystem
} // namespace mmu
