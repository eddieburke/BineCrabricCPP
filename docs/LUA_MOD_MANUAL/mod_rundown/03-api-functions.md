# Runtime API

This is the installed `minecraft` surface, traced from the binding installers.
Use it as an availability map; client namespaces are conditional.

## Core and utility

- Root: `log`, `notify`, `is_client`, `asset_path`, `list_assets`, `read_asset`,
  `read_asset_bytes`, `read_nbt_asset`, input queries, `time.utc_millis`, and
  client options.
- `minecraft.util`: `clamp`, `trim`, `in_rect`, `real_world`, `parse_boolean`,
  `copy`, `resolve_seed`, JSON encode/decode/null, and registry name/list helpers.
- `minecraft.config`: load and save Lua data.
- `minecraft.storage`: scoped read/write and subscription helpers.
- Module helpers: `minecraft.require` and `minecraft.require_dir`.

`read_nbt_asset` returns `(value, error)` and applies a safe path and entry-size
check. JSON encode accepts tables only and rejects non-finite or unsupported
values; decode returns `nil, error` for empty or invalid data. See
`LuaCoreBindings.cpp:457-673` and `LuaJsonApi.cpp:502-533`.

## World, blocks, and entities

`minecraft.world` provides block lookup/ids, random, time/night helpers,
height/top-Y lookup, player lookup, entity spawn/count, and time setting.
Generic additions provide `sample`, `sample_grid`, `sample_channels`, and
`get_block_collisions`. `minecraft.particles.spawn` submits particles.

`minecraft.entities` lists, retrieves, moves, removes, and applies state to
entities; mod-specific APIs spawn mod entities and register pose hooks.
`minecraft.tile_entities` lists, retrieves, and counts tile entities. `items.ids`
is broadly available; item descriptions and inventory functions are client-only.

## Client-only API

| Namespace | Capability |
| --- | --- |
| `camera` | camera pose, FOV, and view state. |
| `texture` | size, pixel access, and bind. |
| `render`, `render.tessellator` | quads, billboards, item override, tessellated quads, dynamic textures. |
| `sound` | register/play positional or looping sound and stop it. |
| `inventory` | slots, cursor, give/offer, selection. |
| `gui`, `screen`, `screen.session` | GUI primitives, screen lifecycle, regions, and sessions. |
| `raycast` | ray query. |
| `files` | user file selection and reading. |
| `model` | voxel/model helpers created by the prelude. |

The exact installers are in `LuaCameraBindings.cpp:213-231`,
`LuaRenderBindings.cpp:442-453`, `LuaSoundBindings.cpp:151-160`,
`LuaInventoryBindings.cpp:219-238`, `LuaScreenBindings.cpp:904-982`, and
`LuaRaycastBindings.cpp:288-290`.

## Screen helpers

The prelude supplies `minecraft.screen.on_ui`, `on_lua_screen`, and settings
helpers. Host-screen handlers receive UI handles through the `screen_ui` event;
Lua-screen handlers receive lifecycle and input through `screen_event`. The
bundled Seed Finder and Realtime Sky show both patterns.

## Phase callbacks

`minecraft.at_phase(phase, order, fn)` accepts only `init`, `post_init`, and
`ready`. It is distinct from runtime events. Use it for staged setup rather than
for frame, world, or render work.
