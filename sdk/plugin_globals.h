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

// Interfaces every plugin needs. Use inside Load() after PLUGIN_SAVEVARS().
// Like the GET_V_IFACE_* calls it expands to, returns false from Load on failure.
#define MMU_GET_CORE_INTERFACES() \
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pEngine, IVEngineServer, INTERFACEVERSION_VENGINESERVER); \
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pICvar, ICvar, CVAR_INTERFACE_VERSION); \
	GET_V_IFACE_ANY(GetServerFactory, g_pServerGameDLL, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL); \
	GET_V_IFACE_ANY(GetServerFactory, g_pGameClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS); \
	GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkMessages, INetworkMessages, NETWORKMESSAGES_INTERFACE_VERSION); \
	GET_V_IFACE_ANY(GetEngineFactory, g_pGameEventSystem, IGameEventSystem, GAMEEVENTSYSTEM_INTERFACE_VERSION)

#endif // _INCLUDE_MMU_PLUGIN_GLOBALS_H_
