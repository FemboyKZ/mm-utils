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
| `mmu/recipient_filter.h`  | `CSingleRecipientFilter` (single-slot reliable filter)         |
| `mmu/schema.h/.cpp`       | Schema offset resolver, `DECLARE_SCHEMA_CLASS`, `SCHEMA_FIELD` |
| `mmu/log.h/.cpp`          | `mmu::g_logTag` used in console log lines                      |
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
- `mmu::Translations::Load(baseDir, addonName)` reads
  `<baseDir>/addons/<addonName>/translations/config.txt` and `*.phrases.txt`.
  Call `SetResolveColorTags(false)` before `Load` for non-chat text (HTML menus).
