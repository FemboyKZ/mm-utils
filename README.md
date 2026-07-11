# Metamod: Source Utils for CS2

Shared utility code for Metamod:Source plugins.

Vendor into plugin as a git submodule at `vendor/mm-utils`.

There is no standalone build. Consumers compile the `.cpp` files as part of their own binary and add this repo's root to their include path.

## Contents

| Path                      | What                                                           |
| ------------------------- | -------------------------------------------------------------- |
| `mmu/kv_parser.h`         | Minimal Valve KeyValues1 tokenizer/parser (`kv::LoadFile`)     |
| `mmu/str_utils.h`         | `str::ToLower`, `str::ToLowerInPlace`                          |
| `mmu/chat_colors.h/.cpp`  | `CHAT_COLOR_*` macros, `mmu::ResolveColorTags`                 |
| `mmu/translations.h/.cpp` | `mmu::Translations`, SourceMod-style phrase tables             |
| `mmu/recipient_filter.h`  | `CSingleRecipientFilter`, `CMultiRecipientFilter`              |
| `mmu/schema.h/.cpp`       | Schema offset resolver, `DECLARE_SCHEMA_CLASS`, `SCHEMA_FIELD` |
| `mmu/log.h/.cpp`          | Engine logging channel + `MMU_LOG_*` macros + file mirroring   |
| `mmu/print.h/.cpp`        | Chat/console send primitives + `mmu::ChatPrinter`              |
| `mmu/chat_command.h`      | Say-quote strip + prefix/command/arg parser                    |
| `mmu/http_client.h/.cpp`  | Async HTTP(S) GET/POST worker + main-thread queue              |
| `mmu/steam_utils.h`       | SteamID64 <-> STEAM_0:X:Y auth id conversion                   |
| `mmu/gamedata.h/.cpp`     | `mmu::GameData`, per-platform KV1 offsets loader               |
| `mmu/sigscan.h/.cpp`      | `sig::` module range + unique signature scan + RIP resolve     |
| `mmu/gamesystem.h/.cpp`   | Engine game system factory list resolve + `FindByName`         |
| `mmu/workshop.h/.cpp`     | Engine workshop registry checks + stale-ACF pruning fallback   |
| `mmu/entity/*.h`          | Entity wrappers: CBaseEntity, controller, pawn, button masks   |

## Usage

Add the submodule:

```sh
git submodule add https://github.com/FemboyKZ/mm-utils vendor/mm-utils
```

AMBuildScript, in `additionalIncludes`:

```python
os.path.join(builder.sourcePath, "vendor", "mm-utils"),
```

AMBuilder, in `binary.sources`:

```python
"vendor/mm-utils/mmu/chat_colors.cpp",
"vendor/mm-utils/mmu/log.cpp",
"vendor/mm-utils/mmu/schema.cpp",
"vendor/mm-utils/mmu/translations.cpp",
```

Include as:

```cpp
#include "mmu/kv_parser.h"
#include "mmu/entity/ccsplayercontroller.h"
```

## Consumer contract

- Set `mmu::g_logTag = "MyPlugin";` at plugin load so schema log lines are attributed.
- `mmu/schema.cpp` needs `g_pSchemaSystem` acquired via
  `GET_V_IFACE_ANY(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION)`
  and links against the SDK's interfaces lib. It also references the plugin's `g_SMAPI`.
- The entity headers reference `extern CGameEntitySystem *g_pEntitySystem;`.
  The plugin defines and populates that global.
- `mmu::Translations::Load(baseDir, addonName)` reads `<baseDir>/addons/<addonName>/translations/config.txt` and `*.phrases.txt`.
  Call `SetResolveColorTags(false)` before `Load` for non-chat text (HTML menus).
- Logging: call `mmu::log::Init(setup)` from plugin Load (channel name, addon dir, file mirroring, retention)
  and `mmu::log::Shutdown()` from Unload so the engine doesn't keep a listener into an unloaded DLL
  Log with `MMU_LOG_INFO/DEBUG/WARN/ERROR`.
  `MMU_LOG_ERROR` is labeled ERROR but sent at LS_WARNING severity, real LS_ERROR makes the engine exit the server.
  `MMU_LOG_DEBUG` lines show with the `-debug` launch option or `Setup::debug`.
  File mirror writes to `addons/<addonName>/logs/<addonName>_YYYY-MM-DD.log`.
- `mmu/print.cpp` references `g_pEngine`, `g_pGameEventSystem` (plugin-defined) and
  `g_pNetworkMessages`, `g_pNetworkServerService` (interfaces.lib).
- Workshop: call `mmu::gamesystem::Resolve(g_pServerGameDLL, sig, sigLen)`
  at plugin Load with the plugin's `IGameSystem_InitAllSystems_pFirst` signature.
  `mmu::EnsureWorkshopMapReady` then consults the engine's `CDedicatedServerWorkshopManager` registry
  first and only falls back to the .vpk folder scan + ACF prune when the map isn't listed.
  `mmu::workshop::IsMapInstalled(fileId)` / `InstalledMapIds()` expose the registry directly.
  The manager struct layout is offset-pinned with static asserts and can drift on CS2 updates.
