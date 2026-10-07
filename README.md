# Metamod: Source Utils for CS2

Shared utility code for Metamod:Source plugins.

Add to a plugin as a git submodule at `mm-utils`.

There is no standalone build.
Consumers compile the `.cpp` files as part of their own binary and add this repo's root to their include path.

## Contents

Dependencies point one way: `utils` never includes `sdk` or `game`, `sdk` never includes `game`.

### `utils/`

Game-agnostic helpers.

| Path                        | What                                                                   |
| --------------------------- | ---------------------------------------------------------------------- |
| `utils/kv_parser.h`         | Minimal Valve KeyValues1 tokenizer/parser (`kv::LoadFile`)             |
| `utils/str.h`               | `str::ToLower`, `str::ToLowerInPlace`, `str::Trim`, `str::StripPort`   |
| `utils/json.h`              | `json::Escape`, `json::GetString`                                      |
| `utils/steamid.h`           | SteamID64 <-> STEAM_0:X:Y auth id conversion                           |
| `utils/maplist.h`           | maplist.txt line parser                                                |
| `utils/chat_command.h`      | Say-quote strip + prefix/command/arg parser                            |
| `utils/command_args.h`      | cs2kz-style `key=value` command args, `mmu::ParseArgs`, number parsing |
| `utils/chat_colors.h/.cpp`  | `CHAT_COLOR_*` macros, `mmu::ResolveColorTags`                         |
| `utils/translations.h/.cpp` | `mmu::Translations`, SourceMod-style phrase tables                     |
| `utils/http_client.h/.cpp`  | Async HTTP(S) GET/POST worker + main-thread queue                      |
| `utils/discord.h`           | Discord webhook send                                                   |
| `utils/log.h/.cpp`          | Engine logging channel + `MMU_LOG_*` macros + file mirroring           |
| `utils/sql.h/.cpp`          | `mmu::sql::Connection`, sql_mm connect/query/escape helpers            |
| `utils/config_blocks.h`     | Shared `[Database]` and log config blocks                              |

### `sdk/`

Engine types and memory access.

| Path                     | What                                                                     |
| ------------------------ | ------------------------------------------------------------------------ |
| `sdk/plugin_globals.h`   | Shared engine + Metamod interface globals + `MMU_GET_CORE_INTERFACES`    |
| `sdk/schema.h/.cpp`      | Schema offset resolver, `DECLARE_SCHEMA_CLASS`, `SCHEMA_FIELD`           |
| `sdk/sigscan.h/.cpp`     | `sig::` module range/section, KHook-backed sig scan, RTTI vtable/object  |
| `sdk/gamedata.h/.cpp`    | `mmu::GameData` KV1 offsets loader + shared `mmu::gamedata` offsets/sigs |
| `sdk/server_client.h`    | `CServerSideClient` vtable resolve, hook indices, slot offset            |
| `sdk/recipient_filter.h` | `CSingleRecipientFilter`, `CMultiRecipientFilter`                        |
| `sdk/entity/*.h`         | Entity wrappers, button masks, `mmu::EntitySystem`                       |

### `game/`

Player-facing behavior built on `sdk`.

| Path                      | What                                                                 |
| ------------------------- | -------------------------------------------------------------------- |
| `game/print.h/.cpp`       | Chat/console send primitives + `mmu::ChatPrinter` + `MMU_PRINT_*_FN` |
| `game/cvarquery.h/.cpp`   | Client convar queries + per client `cl_language` / OS                |
| `game/voice_block.h/.cpp` | Drop a client's voice packets on arrival, e.g. for mutes             |
| `game/target.h`           | `mmu::FindTargets`, @groups/#slot/SteamID/name player targeting      |
| `game/workshop.h/.cpp`    | Workshop registry checks, stale-ACF pruning, `PendingDownload`       |
| `game/player_table.h`     | `mmu::PlayerTable<T>`, bounds-checked per-slot storage               |

### `interfaces/`

| Path                                 | What                                                                    |
| ------------------------------------ | ----------------------------------------------------------------------- |
| `interfaces/interface_bridge.h`      | `mmu::InterfaceBridge<T>`, cached cross-plugin interface pointer        |
| `interfaces/<plugin>/*.h`            | Public plugin interfaces plus consumer helpers                          |
| `interfaces/cs2admin/admin_access.h` | `mmu::AdminAccess`, mm-cs2admin permission checks for consumers         |
| `interfaces/cs2kz/`                  | cs2kz's public interface, vendored as is under its own AGPL-3.0 LICENSE |
| `interfaces/sql_mm/`                 | sql_mm's public interface (GPL-3.0), vendored as is                     |

## Usage

Add the submodule:

```sh
git submodule add https://github.com/FemboyKZ/mm-utils mm-utils
```

AMBuildScript, in `additionalIncludes`:

```python
os.path.join(builder.sourcePath, "mm-utils"),
```

AMBuilder, in `binary.sources`:

```python
"mm-utils/utils/chat_colors.cpp",
"mm-utils/utils/log.cpp",
"mm-utils/sdk/schema.cpp",
"mm-utils/utils/sql.cpp",
"mm-utils/utils/translations.cpp",
```

Include as:

```cpp
#include "utils/kv_parser.h"
#include "sdk/entity/ccsplayercontroller.h"
```
