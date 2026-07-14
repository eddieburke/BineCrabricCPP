# 01 — Introduction and Packages

## What Is a Lua Mod

A Lua mod is a self-contained package placed under `%APPDATA%/.minecraft/mods/` (or the configured run directory) as either a **directory** or a **`.zip` archive**. Each mod requires:

| File | Purpose |
|------|---------|
| `mod.json` | Manifest — declares `id`, `name`, `version`, `entry`, `description`, and `enabled` |
| `scripts/` (or any path) | Entry script and support modules |

### mod.json Example

```json
{
  "id": "my_first_mod",
  "name": "My First Mod",
  "version": "1.0.0",
  "entry": "scripts/main.lua",
  "description": "A simple example mod",
  "enabled": true
}
```

The `entry` field specifies the Lua file loaded at startup. If the path is unsafe or non-existent the mod fails to load.

### ModPackage Fields

Internally every discovered mod is represented as a `ModPackage` struct:

| Field | Type | Description |
|-------|------|-------------|
| `id` | string | Unique mod identifier |
| `name` | string | Human-readable name |
| `version` | string | Semver string |
| `description` | string | Short description |
| `entry` | string | Relative path to the Lua entry script |
| `source` | enum | `Directory` or `Zip` |
| `enabledByDefault` | bool | Default enabled state |
| `configuredEnabled` | bool | Persisted user preference |
| `active` | bool | Currently loaded and running |
| `resourceOverlay` | bool | Has a `resources/` subdirectory |
| `runtimeScript` | bool | Entry is a `.lua` script (must be true) |
| `downloadUrl` | string | Optional download URL |
| `error` | string | Error message if loading failed |
| `sourcePath` | path | Path to the archive or directory |
| `rootPath` | path | Extracted/canonical root |

### Discovery & Loading

`ModHost::rescan()` iterates `mods/` directory. For each subdirectory containing `mod.json` it parses the manifest. For each `.zip` file it builds a zip index, reads and validates `mod.json`, then extracts the archive to a cache directory (`mods/.cache/<sanitized_name>_<size>_<timestamp>/`).

Security limits enforced:

| Limit | Constant | Value |
|-------|----------|-------|
| Max archive size | `kMaxModArchiveBytes` | 256 MiB |
| Max entry size | `kMaxModEntryBytes` | 64 MiB |
| Max extracted size | `kMaxModExtractedBytes` | 512 MiB |
| Max archive entries | `kMaxModArchiveEntries` | 4096 |

Duplicate IDs are detected and renamed with a `__duplicate__N` suffix and disabled.

## Sandbox

The runtime prelude aggressively restricts Lua's standard library to prevent unsafe operations:

```lua
-- Removed entirely
io = nil
debug = nil
dofile = nil
loadfile = nil

-- os limited to time querying only
os = { clock = os.clock, date = os.date, difftime = os.difftime, time = os.time }

-- package restricted
package.cpath = ""
package.loadlib = nil
package.searchers[3] = nil  -- C library searcher
package.searchers[4] = nil  -- C root searcher
-- After require is replaced, package itself is set to nil
```

Only the `minecraft.*` API table is available for file I/O, asset access, storage, and configuration. All native Lua module-loading paths are severed — use `minecraft.require()` instead.

## Load Order

The loading sequence for each mod is:

```
1. C++ bindings     → installMinecraftTable() populates the `minecraft` global
                       with CoreApi, WorldApi, EntityApi, BlockApi, ItemApi,
                       RecipeApi, TileEntityApi, and (on client) CameraApi,
                       FboApi, TextureApi, ModelApi, SoundApi, InventoryApi,
                       ScreenApi, RenderApi, RaycastApi, GenericModApi.

2. Runtime Prelude  → kRuntimePrelude is loaded and executed. This establishes
                       the full DSL: minecraft.on, minecraft.register_block,
                       minecraft.register_item, minecraft.register_shaped_recipe,
                       minecraft.config, minecraft.screen, model.voxels,
                       minecraft.world.marker_px, minecraft.storage,
                       minecraft.util.*, minecraft.require, etc.

3. Entry Script     → The user's entry script is loaded and executed. During
                       top-level execution, the script typically calls
                       minecraft.on() and minecraft.at_phase() to register
                       callbacks. The mod is marked active BEFORE the script
                       runs so resource lookups work during init.

4. HookBus Subscribe→ `minecraft.on()` subscriptions are retained during
                       entry-script execution and attached after the script
                       completes. `minecraft.at_phase()` is different: it
                       subscribes its lifecycle callback immediately.
```

## Lifecycle Phases

Lifecycle phases run once at startup during `Registry::bootstrap()`. Lua mods register phase handlers with `minecraft.at_phase()`, which subscribes to the internal `LifecycleEvent`. The phase name argument is a string, but the callback receives numeric enum values: `NotStarted = 0`, `Init = 1`, `PostInit = 2`, and `Ready = 3`.

### All 3 Phases

| Constant String | C++ Enum | Purpose |
|----------------|----------|---------|
| `"init"` | `Init` | Register all content: blocks, items, biomes, entities, block entities, particles, renderers. Cross-references are NOT yet resolved. |
| `"post_init"` | `PostInit` | Resolve cross-references and register recipes: crafting, smelting, fuel, block finalization, block items. All content from `init` is available. |
| `"ready"` | `Ready` | All registration complete; world loading proceeds. Equivalent to the old `"frozen"`. |

### Using Lifecycle Phases

```lua
minecraft.at_phase("init", 0, function(event)
  -- event.previous  → numeric phase enum
  -- event.current   → numeric phase enum
  minecraft.register_block({
    id = 1000,
    -- ...
  })
end)
```

The callback receives fields:
- `event.previous` — numeric previous phase enum (`0`–`3`)
- `event.current` — numeric current phase enum (`0`–`3`)

The C++ `LifecycleEvent` struct:
```cpp
struct LifecycleEvent {
  LifecyclePhase previous;  // defaults to NotStarted
  LifecyclePhase current;   // defaults to NotStarted
};
```

## The Prelude DSL

The entire `kRuntimePrelude` is injected as a Lua string before the user's entry script runs. It establishes the complete developer-facing API.

### Event Name Constants

```lua
minecraft.events = {
  attack_damage          = "attack_damage",
  block_interact         = "block_interact",
  chunk_generation       = "chunk_generation",
  client_tick            = "client_tick",
  create_world           = "create_world",
  entity_interact        = "entity_interact",
  entity_remove          = "entity_remove",
  entity_render          = "entity_render",
  entity_spawn           = "entity_spawn",
  entity_teleport        = "entity_teleport",
  entity_tick            = "entity_tick",
  fog_settings           = "fog_settings",
  first_person_hand      = "first_person_hand",
  fov                    = "fov",
  camera_setup           = "camera_setup",
  key_press              = "key_press",
  mouse_button           = "mouse_button",
  player_travel          = "player_travel",
  pre_entity_render      = "pre_entity_render",
  pre_tile_entity_render = "pre_tile_entity_render",
  raycast                = "raycast",
  render_frame           = "render_frame",
  render_targets         = "render_targets",
  screen_event           = "screen_event",
  screen_region          = "screen_region",
  screen_ui              = "screen_ui",
  tick_rate              = "tick_rate",
  tile_entity_tick       = "tile_entity_tick",
  world_color            = "world_color",
  world_open             = "world_open",
  world_render           = "world_render",
  world_spawn_search     = "world_spawn_search",
  world_start            = "world_start",
  world_tick             = "world_tick",
}
```

### Lifecycle Phase Constants

```lua
minecraft.lifecycle = {
  init      = "init",
  post_init = "post_init",
  ready     = "ready",
}
```

### Generation Stage/Moment Constants

```lua
minecraft.generation = {
  stages = { terrain = "terrain", surface = "surface", carver = "carver", features = "features" },
  moments = { before = "before", after = "after" },
}
```

### Render Stage/Moment Constants

```lua
minecraft.render = {
  stages = {
    sky                 = "sky",
    stars               = "stars",
    terrain_opaque      = "terrain_opaque",
    entities            = "entities",
    particles_lit       = "particles_lit",
    particles           = "particles",
    terrain_translucent = "terrain_translucent",
    weather             = "weather",
    clouds              = "clouds",
    hand                = "hand",
    framebuffer         = "framebuffer",
  },
  moments = { before = "before", after = "after" },
}
```

### Color Constants

```lua
minecraft.colors = { sky = "sky", fog = "fog" }
```

### Key Code Constants

```lua
minecraft.keys = { escape = 1, enter = 28, space = 57, up = 200, down = 208 }
```

### Core Subscription Function: `minecraft.on()`

```lua
minecraft.on(event_name, options, callback)
```

Parameters:
- `event_name` (string) — one of the event names from `minecraft.events`
- `options` (table) — filter criteria:
  - **Field equality**: any key-value pair where the key matches a field on the event table; the callback fires only if `event[key] == value`
  - **Set table**: if a value is a table, it acts as a set — the field must have a key matching the value, or the value must be found in array elements
  - **Predicate function**: if a value is a function, it's called with `expected(actual)` and must return `true`
  - `when` (function): a predicate gate on the entire event, `options.when(event)` must return `true`
  - `once` (boolean): auto-unsubscribes after the first match
  - `priority` (number): lower numbers execute first (default 0)
- `callback` (function): receives the event table, may mutate mutable fields

#### Filtering Logic

The `matches()` function in the prelude:

```lua
local function matches(event, options)
  for key, expected in pairs(options) do
    if key ~= "once" and key ~= "priority" and key ~= "when" then
      local actual = event[key]
      if type(expected) == "function" then
        if not expected(actual) then return false end
      elseif type(expected) == "table" then
        local found = expected[actual] == true
        if not found then
          for _, value in ipairs(expected) do
            if value == actual then found = true break end
          end
        end
        if not found then return false end
      elseif actual ~= expected then
        return false
      end
    end
  end
  return options.when == nil or options.when(event) == true
end
```

#### Examples

```lua
-- Run once when any block is right-clicked
minecraft.on(minecraft.events.block_interact, { right_click = true, once = true }, function(event)
  print("Block right-clicked at " .. event.x .. "," .. event.y .. "," .. event.z)
end)

-- Filter by block_id with a set
minecraft.on(minecraft.events.block_interact, { block_id = { [1] = true, [2] = true } }, function(event)
  -- matches block_id 1 or 2
end)

-- Filter by predicate
minecraft.on(minecraft.events.world_tick, { when = minecraft.util.real_world }, function(event)
  -- only in real worlds (not mod generation)
end)
```

### Registration Wrappers

**`minecraft.register_block(spec)`** — registers a block and optionally subscribes `block_interact`:

```lua
minecraft.register_block({
  id = 1000,
  on_use = function(event)
    -- event fields: player, world, x, y, z, side, right_click, etc.
    if event.handled then event.canceled = true end
  end,
  behavior_priority = 0,  -- passed to the block_interact priority
})
```

**`minecraft.register_item(spec)`** — registers an item (wrapper around the native `register_item`):

```lua
minecraft.register_item({
  id = "my_mod:custom_item",
  -- other fields depend on item type
})
```

**`minecraft.register_shaped_recipe(spec)`** — registers a shaped crafting recipe:

```lua
minecraft.register_shaped_recipe({
  output_item_id = 264,
  output_count = 1,
  item_id = 265,
  key = "X",
  pattern = { "XXX", "X X", "XXX" },
})
```

### Utility Functions (`minecraft.util`)

```lua
-- Clamp a value to [minimum, maximum]
util.clamp(value, minimum, maximum)

-- Trim whitespace from a string
util.trim(value)

-- Test if (x, y) is inside a rectangle
util.in_rect(x, y, left, top, width, height)

-- Predicate: true if the event is from a real world (not mod generation)
util.real_world(event)

-- Parse a boolean string ("true"/"false"/"1"/"0"/"yes"/"no"/"on"/"off")
util.parse_boolean(value, fallback)

-- Shallow-copy a table
util.copy(values)
```

### Config Persistence

**`minecraft.config.load(path, defaults, options)`** — reads a key=value config file from persistent storage:

```lua
local values, ok = minecraft.config.load("my_mod/config.txt", {
  enabled = true,
  volume = 0.5,
}, {
  aliases = { enable = "enabled" },
})
```

Format per line: `key = value` or `key: value`. Comments start with `#` or `;`. Type coercion is driven by the defaults table (boolean, number, string).

**`minecraft.config.save(path, values, options)`** — writes a key=value config file:

```lua
minecraft.config.save("my_mod/config.txt", {
  enabled = true,
  volume = 0.5,
}, {
  keys = { "volume", "enabled" },  -- explicit order
  separator = "=",
  names = { enabled = "enable" },  -- output key aliases
})
```

### Persistent Key-Value Storage

```lua
minecraft.storage.read(path)   → string or nil
minecraft.storage.write(path, text) → boolean
```

These are thin wrappers around `minecraft._read_storage` and `minecraft._write_storage` (native C functions that read/write files under the mod's safe storage area).

### World Marker Coordinate Helper

```lua
minecraft.world.marker_px(grid, world_x, world_z) → col, row

-- grid = { side = N, step = S, origin_x = X, origin_z = Z }
-- Maps a world coordinate to a grid column/row, clamped to [0, side-1].
```

### Screen DSL

**`minecraft.screen.on_ui(screen_id, region, callback, priority)`** — subscribes to `screen_ui` for a specific screen ID + region:

```lua
minecraft.screen.on_ui("my_mod:config", "options", function(event)
  event.ui:add_stacked_centered_button("Open Settings", open_fn)
end, 100)
```

**`minecraft.screen.on_lua_screen(screen_id, handlers, priority)`** — subscribes to `screen_event` and dispatches by phase:

```lua
minecraft.screen.on_lua_screen("my_mod:config", {
  init = function(event)
    -- event.width, event.height
    minecraft.gui.add_button(x, y, w, h, "Done", close_fn)
  end,
  render = function(event)
    -- event.mouse_x, event.mouse_y
  end,
  mouse = function(event)
    -- event.button, event.x, event.y, event.released
  end,
  key = function(event)
    -- event.key, event.char
  end,
  close = function(event)
    -- cleanup
  end,
}, 100)
```

**`minecraft.screen.settings(spec)`** — full settings screen builder:

```lua
minecraft.screen.settings({
  id = "my_mod:settings",
  title = "My Mod Settings",
  parent_screen = "options",
  parent_region = "options",
  button_label = "My Mod",
  values = { volume = 0.5, fullscreen = false },
  sliders = {
    { key = "volume", min = 0, max = 1, integer = false, label = "Volume", format = function(v) return v end },
  },
  toggles = {
    { key = "fullscreen", label = "Fullscreen" },
  },
  on_change = function() end,
  on_save = function() end,
  on_reset = function() end,
  priority = 100,
})
```

### Safe Module Loading: `minecraft.require`

```lua
-- Functions identically to Lua's require but restricted:
-- - Name must match ^[%w_.-]+$ (alphanumeric, underscore, dot, hyphen)
-- - Must NOT contain ".." (no directory traversal)
-- - Only searches minecraft.asset_path(".") + "/?.lua" and "/?/init.lua"
-- - After this replaces require, package is set to nil

minecraft.require("my_mod.helpers")   -- loads my_mod/helpers.lua
minecraft.require("my_mod.utils")     -- loads my_mod/utils.lua or my_mod/utils/init.lua
```

### Voxel Model Builder: `model.voxels(opts)`

Builds a voxel model from a list of unit cubes. Entirely implemented in Lua on top of `minecraft.model.build()`.

```lua
model.voxels({
  cells = {
    { x = 0, y = 0, z = 0, r = 1, g = 0, b = 0, a = 1 },  -- red cube
    { x = 1, y = 0, z = 0, r = 0, g = 1, b = 0, a = 1 },  -- green cube
  },
  resolution = 16,    -- number of voxels per unit (default 16)
  scale = 1/16,       -- size of each voxel (derived from resolution)
  origin_x = 0,       -- model origin offset
  origin_y = 0,
  origin_z = 0,
  key = "my_model",   -- cache key for model.build()
})
```

The function:

1. Builds a `present` set from cell coordinates for neighbor lookup
2. For each cell, defines 6 faces with normals (`+Z, -Z, -X, +X, +Y, -Y`)
3. Culls interior faces (faces shared with a present neighbor are skipped)
4. Converts each remaining face to 4 vertices with `(r, g, b, a)` color
5. Calls `minecraft.model.build({ quads = quads, key = opts.key })`

### Voxel Texture Extruder: `model.voxel(spec)`

Samples a sprite texture and extrudes it one voxel thick (centered on z=0.5).

```lua
local handle = model.voxel({
  texture = "my_mod:textures/blocks/custom.png",
  atlas_index = -1,     -- vanilla atlas tile index (default -1 = no atlas)
  mod_texture = false,  -- true = direct pixel sampling from mod texture
  grid = 16,            -- sampling grid resolution (default 16)
  alpha_cutoff = 30,    -- alpha threshold 0-255 (default 30)
})
```

The function:

1. Gets the texture size via `texture.size()`
2. Computes tile offset for atlas index: `tile_x = (index % 16) * 16`, `tile_y = floor(index / 16) * 16`
3. Samples `grid × grid` pixels, creating a cell for each pixel whose alpha > cutoff
4. Passes cells to `model.voxels()` with z origin at `0.5 - (1/grid)/2`
5. Caches results to avoid rebuilding

### Asset Path & Reading

```lua
-- Resolve a file path relative to the mod's root
minecraft.asset_path("scripts/helpers.lua") → "/abs/path/to/mod/scripts/helpers.lua"

-- Read an asset file as text
minecraft.read_asset("config.json") → string

-- Read an asset file as bytes (table of byte values)
minecraft.read_asset_bytes("image.png") → { 137, 80, 78, 71, ... }
```

### Safe Path Handling

All path operations go through `normalizeRelativePath()` which converts
backslashes to forward slashes and removes leading `/` characters. It does
not canonicalize `.`/empty segments and does not itself reject NUL bytes.

Then `isSafeRelativePath()` validates:
- No path traversal (`..` segments are rejected)
- No absolute paths
- No null characters are accepted by the public path validation layer

`sanitizeName()` strips unsafe characters for cache directory names.

## Mod Identity

### `minecraft.log`

```lua
minecraft.log("Hello from my mod!")   -- prints [lua-mod:my_mod_id:info] Hello from my mod!
```

Backed by `runtimeLog()` which writes `[lua-mod:<modId>:<level>] <message>` to stdout. Levels: `info`, `warn`, `error`.

### `minecraft.is_client`

Returns `true` if the current Lua execution has a client-side `Minecraft::INSTANCE`. Set by `setLuaExecutionFields()` via the `side` field on event tables.

### `minecraft.mod_id`

Set during `installMinecraftTable()` — the unique ID of the currently-loaded mod, matching the value from `mod.json`.

### `minecraft.asset_path`

Returns absolute filesystem paths relative to the mod's root directory.

## Thread Safety

### State Mutex

Each `ModHost::LoadedLuaMod` carries a `std::recursive_mutex stateMutex`. The `callLuaEvent()` template locks this mutex before invoking the Lua function:

```cpp
const std::lock_guard<std::recursive_mutex> lock(mod->stateMutex);
```

This ensures only one thread executes Lua callbacks for a given mod at a time.

### Mod Context Scope

`ModContextScope` sets a thread-local world + player context. Server entry points always establish a scope; client-side render/HUD/screen callbacks fall back to the client replica world.

Thread-local state:
- `previousWorld_`, `previousIsClient_`, `previousPlayer_` are saved/restored via RAII
- `contextOrClientWorld()` returns the active scoped world or the `client::Minecraft::INSTANCE->world` fallback
- `activeModPlayer()` returns the active scoped player
- `modContextIsClient()` answers whether the current scope is client-side

### GUI/LuaScreen Thread Safety

The screen and GUI APIs use thread-local singleton state:
- `activeScreenUi()` — per-thread `ActiveScreenUi` for `screen_ui` context
- `activeLuaScreen()` — per-thread active Lua screen state

This allows multiple mod screens on different threads without data races.
