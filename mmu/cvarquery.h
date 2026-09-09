#ifndef _INCLUDE_MMU_CVARQUERY_H_
#define _INCLUDE_MMU_CVARQUERY_H_

#include <functional>

namespace mmu
{
	// Reads convar values off connected clients: CSVCMsg_GetCvarValue out, CCLCMsg_RespondCvarValue back.
	namespace cvarquery
	{
		enum class Status
		{
			ValueIntact = 0, // It got the value fine.
			CvarNotFound = 1,
			NotACvar = 2,     // There's a ConCommand, but it's not a ConVar.
			CvarProtected = 3 // The cvar was marked FCVAR_SERVER_CAN_NOT_QUERY, so the server may not read it.
		};

		using Callback = std::function<void(int slot, Status status, const char *name, const char *value)>;

		// Call from plugin Load once the engine interfaces are up. `engineModuleAnchor` is any pointer inside engine2, e.g. g_pEngine.
		// On failure Query is a no-op and the getters stay null.
		bool Init(const void *engineModuleAnchor);

		// Call from plugin Unload.
		void Shutdown();

		// The callback runs once, when the client answers. A client that never answers never calls back.
		bool Query(int slot, const char *cvarName, Callback callback);

		// Call from the plugin's own IServerGameClients hooks.
		// OnClientConnected starts the cl_language and engine_ostype queries behind the getters below.
		void OnClientConnected(int slot, bool fakePlayer);
		void OnClientDisconnect(int slot);

		// Null until the client answers, which lands shortly after connect.
		const char *GetClientLanguage(int slot);
		const char *GetClientOS(int slot);
	} // namespace cvarquery
} // namespace mmu

#endif // _INCLUDE_MMU_CVARQUERY_H_
