#ifndef _INCLUDE_MMU_PLUGIN_GLOBALS_H_
#define _INCLUDE_MMU_PLUGIN_GLOBALS_H_

// Engine and Metamod:Source interface globals every plugin declares identically.
// This header only declares them. Each plugin still DEFINES them.

#include <ISmmPlugin.h>

class IVEngineServer;
class IServerGameDLL;
class IServerGameClients;
class ICvar;

extern IVEngineServer *g_pEngine;
extern IServerGameDLL *g_pServerGameDLL;
extern IServerGameClients *g_pGameClients;
extern ICvar *g_pICvar;

extern ISmmAPI *g_SMAPI;
extern ISmmPlugin *g_PLAPI;
extern PluginId g_PLID;
extern SourceHook::ISourceHook *g_SHPtr;

// Highest client slot index. Player arrays are sized [MAXPLAYERS + 1].
#define MAXPLAYERS 64

#endif // _INCLUDE_MMU_PLUGIN_GLOBALS_H_
