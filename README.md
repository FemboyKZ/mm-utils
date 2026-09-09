# Metamod: Source Utils for CS2

Shared utility code for Metamod:Source plugins.

Vendor into plugin as a git submodule at `vendor/mm-utils`.

There is no standalone build. Consumers compile the `.cpp` files as part of their own binary and add this repo's root to their include path.

## Contents

| Path                      | What                                                           |
| ------------------------- | -------------------------------------------------------------- |
| `mmu/kv_parser.h`         | Minimal Valve KeyValues1 tokenizer/parser (`kv::LoadFile`)     |
| `mmu/str_utils.h`         | `str::ToLower`, `str::ToLowerInPlace`                          |
| `mmu/plugin_globals.h`    | Shared engine + Metamod interface global declarations          |
| `mmu/sql.h/.cpp`          | `mmu::sql::Connection`, sql_mm connect/query/escape helpers    |
| `mmu/chat_colors.h/.cpp`  | `CHAT_COLOR_*` macros, `mmu::ResolveColorTags`                 |
| `mmu/translations.h/.cpp` | `mmu::Translations`, SourceMod-style phrase tables             |
| `mmu/recipient_filter.h`  | `CSingleRecipientFilter`, `CMultiRecipientFilter`              |
| `mmu/schema.h/.cpp`       | Schema offset resolver, `DECLARE_SCHEMA_CLASS`, `SCHEMA_FIELD` |
| `mmu/log.h/.cpp`          | Engine logging channel + `MMU_LOG_*` macros + file mirroring   |
| `mmu/print.h/.cpp`        | Chat/console send primitives + `mmu::ChatPrinter`              |
| `mmu/chat_command.h`      | Say-quote strip + prefix/command/arg parser                    |
| `mmu/cvarquery.h/.cpp`    | Client convar queries + per client `cl_language` / OS          |
| `mmu/http_client.h/.cpp`  | Async HTTP(S) GET/POST worker + main-thread queue              |
| `mmu/steam_utils.h`       | SteamID64 <-> STEAM_0:X:Y auth id conversion                   |
| `mmu/gamedata.h/.cpp`     | `mmu::GameData`, per-platform KV1 offsets loader               |
| `mmu/sigscan.h/.cpp`      | `sig::` module range/section + sig scan + vtable by RTTI name  |
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
"vendor/mm-utils/mmu/sql.cpp",
"vendor/mm-utils/mmu/translations.cpp",
```

Include as:

```cpp
#include "mmu/kv_parser.h"
#include "mmu/entity/ccsplayercontroller.h"
```
