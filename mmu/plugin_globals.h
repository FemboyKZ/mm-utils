#ifndef _INCLUDE_MMU_PLUGIN_GLOBALS_H_
#define _INCLUDE_MMU_PLUGIN_GLOBALS_H_

// Engine and Metamod:Source interface globals every plugin declares identically.
// This header only declares them. Each plugin still DEFINES them.

#include <ISmmPlugin.h>
#include <eiface.h>

extern IVEngineServer *g_pEngine;
extern IServerGameDLL *g_pServerGameDLL;
extern IServerGameClients *g_pGameClients;
extern ICvar *g_pICvar;

extern ISmmAPI *g_SMAPI;
extern ISmmPlugin *g_PLAPI;
extern PluginId g_PLID;

// Highest client slot index. Player arrays are sized [MAXPLAYERS + 1].
#define MAXPLAYERS 64

// Resolve the engine interfaces every plugin needs, in one line.
//
// Expands to GET_V_IFACE_* calls, which read `ismm`, `error` and `maxlen` from the enclosing scope
// and `return false` on the first interface that does not resolve.
// Goes inside ISmmPlugin::Load, after PLUGIN_SAVEVARS()
//
// The globals are declared per plugin, not here:
// some come from this header, some from the plugin's own common.h, and some from the SDK's interfaces.lib.
#define MMU_GET_CORE_INTERFACES() \
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pEngine, IVEngineServer, INTERFACEVERSION_VENGINESERVER); \
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pICvar, ICvar, CVAR_INTERFACE_VERSION); \
	GET_V_IFACE_ANY(GetServerFactory, g_pServerGameDLL, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL); \
	GET_V_IFACE_ANY(GetServerFactory, g_pGameClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS); \
	GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkMessages, INetworkMessages, NETWORKMESSAGES_INTERFACE_VERSION); \
	GET_V_IFACE_ANY(GetEngineFactory, g_pGameEventSystem, IGameEventSystem, GAMEEVENTSYSTEM_INTERFACE_VERSION)

#endif // _INCLUDE_MMU_PLUGIN_GLOBALS_H_
