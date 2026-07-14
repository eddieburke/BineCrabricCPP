# Lua Mod API — Compact

Generated from `LUA_MOD_MANUAL`; code samples omitted unless `--examples`.

# Introduction and Packages
## What Is a Lua Mod
A Lua mod is a self-contained package placed under `%APPDATA%/.minecraft/mods/` (or the configured run directory) as either a **directory** or a **`.zip` archive**. Each mod requires:
``mod.json``: Manifest — declares `id`, `name`, `version`, `entry`, `description`, and `enabled`; ``scripts/` (or any path)`: Entry script and support modules
### mod.json Example
```lua
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
`id`/string/Unique mod identifier; `name`/string/Human-readable name; `version`/string/Semver string; `description`/string/Short description; `entry`/string/Relative path to the Lua entry script; `source`/enum/`Directory` or `Zip`; `enabledByDefault`/bool/Default enabled state; `configuredEnabled`/bool/Persisted user preference; `active`/bool/Currently loaded and running; `resourceOverlay`/bool/Has a `resources/` subdirectory; `runtimeScript`/bool/Entry is a `.lua` script (must be true); `downloadUrl`/string/Optional download URL; `error`/string/Error message if loading failed; `sourcePath`/path/Path to the archive or directory; `rootPath`/path/Extracted/canonical root
### Discovery & Loading
`ModHost::rescan()` iterates `mods/` directory. For each subdirectory containing `mod.json` it parses the manifest. For each `.zip` file it builds a zip index, reads and validates `mod.json`, then extracts the archive to a cache directory (`mods/.cache/<sanitized_name>_<size>_<timestamp>/`).
Security limits enforced:
Max archive size/`kMaxModArchiveBytes`/256 MiB; Max entry size/`kMaxModEntryBytes`/64 MiB; Max extracted size/`kMaxModExtractedBytes`/512 MiB; Max archive entries/`kMaxModArchiveEntries`/4096
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
```lua
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
`"init"`/`Init`/all content: blocks, items, biomes, entities, block entities, particles, renderers. Cross-references are NOT yet resolved; `"post_init"`/`PostInit`/Resolve cross-references and register recipes: crafting, smelting, fuel, block finalization, block items. All content from `init` is available; `"ready"`/`Ready`/All registration complete; world loading proceeds. Equivalent to the old `"frozen"`
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
The callback receives fields: `event.previous` — numeric previous phase enum (`0`–`3`) `event.current` — numeric current phase enum (`0`–`3`)
The C++ `LifecycleEvent` struct:
```lua
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
Parameters: `event_name` (string) — one of the event names from `minecraft.events` `options` (table) — filter criteria: - **Field equality**: any key-value pair where the key matches a field on the event table; the callback fires only if `event[key] == value` - **Set table**: if a value is a table, it acts as a set — the field must have a key matching the value, or the value must be found in array elements - **Predicate function**: if a value is a function, it's called with `expected(actual)` and must return `true` - `when` (function): a predicate gate on the entire event, `options.when(event)` must return `true` - `once` (boolean): auto-unsubscribes after the first match - `priority` (number): lower numbers execute first (default 0) `callback` (function): receives the event table, may mutate mutable fields
### Filtering Logic
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
### Examples
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
Builds a `present` set from cell coordinates for neighbor lookup For each cell, defines 6 faces with normals (`+Z, -Z, -X, +X, +Y, -Y`) Culls interior faces (faces shared with a present neighbor are skipped) Converts each remaining face to 4 vertices with `(r, g, b, a)` color Calls `minecraft.model.build({ quads = quads, key = opts.key })`
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
Gets the texture size via `texture.size()` Computes tile offset for atlas index: `tile_x = (index % 16) * 16`, `tile_y = floor(index / 16) * 16` Samples `grid × grid` pixels, creating a cell for each pixel whose alpha > cutoff Passes cells to `model.voxels()` with z origin at `0.5 - (1/grid)/2` Caches results to avoid rebuilding
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
All path operations go through `normalizeRelativePath()` which converts backslashes to forward slashes and removes leading `/` characters. It does not canonicalize `.`/empty segments and does not itself reject NUL bytes.
Then `isSafeRelativePath()` validates: No path traversal (`..` segments are rejected) No absolute paths No null characters are accepted by the public path validation layer
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
```lua
const std::lock_guard<std::recursive_mutex> lock(mod->stateMutex);
```
This ensures only one thread executes Lua callbacks for a given mod at a time.
### Mod Context Scope
`ModContextScope` sets a thread-local world + player context. Server entry points always establish a scope; client-side render/HUD/screen callbacks fall back to the client replica world.
Thread-local state: `previousWorld_`, `previousIsClient_`, `previousPlayer_` are saved/restored via RAII `contextOrClientWorld()` returns the active scoped world or the `client::Minecraft::INSTANCE->world` fallback `activeModPlayer()` returns the active scoped player `modContextIsClient()` answers whether the current scope is client-side
### GUI/LuaScreen Thread Safety
The screen and GUI APIs use thread-local singleton state: `activeScreenUi()` — per-thread `ActiveScreenUi` for `screen_ui` context `activeLuaScreen()` — per-thread active Lua screen state
This allows multiple mod screens on different threads without data races.

# Events Reference
All event subscriptions use `minecraft.on(event_name, options, callback)`. The filtering system is entirely implemented in Lua's prelude:
```lua
minecraft.on(event_name, {
  -- Field filters: any key matching an event field is checked:
  --   - primitive: exact equality
  --   - table: treated as a set (key = true lookup, or array iteration)
  --   - function: predicate called with (actual_value)
  -- Special keys:
  when = function(event) end,  -- gate predicate; must return true
  once = true,                 -- auto-unsubscribe after first fire
  priority = 0,                -- lower = earlier execution
}, function(event)
  -- callback may mutate read-write fields (see per-event notes)
end)
```
### Priority System
Lower priority numbers execute first. The C++ HookBus sorts entries via `std::stable_sort`:
```lua
listeners().push_back({owner, priority, listener});
std::stable_sort(listeners().begin(), listeners().end(),
  [](const Entry& a, const Entry& b) { return a.priority < b.priority; });
```
Default priority is `0`. Use negative values for early execution, positive for late.
### `event.handled` and `event.canceled` Conventions
**`canceled`**: Prevents default engine behavior. Many events support this (time change, weather, block tick, entity spawn, attack damage, teleport, etc.). **`handled`**: Used by interaction events (`block_interact`, `entity_interact`, `key_press`, `mouse_button`). Signifies that the mod consumed the event. For block interaction, setting `event.handled = true` causes the prelude to also set `event.canceled = true`:
```lua
function minecraft.register_block(spec)
  -- ...
  if spec.on_use then
    minecraft.on(minecraft.events.block_interact, { block_id = spec.id, ... }, function(event)
      local result = spec.on_use(event)
      if event.handled then event.canceled = true end
      return result
    end)
  end
end
```
### `event.before` Pattern
Events with tick/update cycles (`client_tick`, `world_tick`, `chunk_generation`, `world_render`) provide a `before` field or `moment` field. The `before` field is `true` in the pre-update phase and `false` in the post-update phase. Render events provide `stage` + `moment` for granular placement.
## World Events
### `world_tick`
Fires every world tick (both before and after the main tick processing).
`remote`/boolean/True if this is a client replica world/No; `before`/boolean/True in the pre-tick phase, false after/No; `side`/string/`"server"` or `"client"`/No
```lua
minecraft.on(minecraft.events.world_tick, { before = false }, function(event)
  -- post-tick logic
end)
```
### `world_time`
Fires when the world time changes. Available as a C++ struct but **NOT exposed to Lua** (not in the prelude event list).
`world`/userdata/The world/No; `old_time`/int64/Previous time value/No; `new_time`/int64/New time value/No; `canceled`/boolean/Cancel the time change/Yes
### `weather_cycle`
Fires when weather would change. **Not exposed to Lua.**
`world`/userdata/The world/No; `remote`/boolean/Client replica flag/No; `canceled`/boolean/Cancel weather change/Yes
### `lightning_strike`
Fires when lightning strikes. **Not exposed to Lua.**
`world`/userdata/The world/No; `x`/int/Strike X position/No; `y`/int/Strike Y position/No; `z`/int/Strike Z position/No; `canceled`/boolean/Cancel the lightning strike/Yes
### `snow_ice_placement`
Fires when snow/ice would be placed by weather. **Not exposed to Lua.**
`world`/userdata/The world/No; `x`/int/X position/No; `y`/int/Y position/No; `z`/int/Z position/No; `place_snow`/boolean/Whether snow would be placed/No; `place_ice`/boolean/Whether ice would be placed/No
### `random_block_tick`
Fires when a block receives a random tick. **Not exposed to Lua.**
`world`/userdata/The world/No; `block`/userdata/The block object/No; `x`/int/X position/No; `y`/int/Y position/No; `z`/int/Z position/No; `block_id`/int/Minecraft block ID/No; `canceled`/boolean/Cancel the random tick/Yes
### `scheduled_block_tick`
Fires when a scheduled block tick executes. **Not exposed to Lua.**
`world`/userdata/The world/No; `block`/userdata/The block object/No; `x`/int/X position/No; `y`/int/Y position/No; `z`/int/Z position/No; `block_id`/int/Minecraft block ID/No; `instant`/boolean/True if immediate execution/No; `canceled`/boolean/Cancel the scheduled tick/Yes
### `schedule_block_update`
Fires when a block update is scheduled. **Not exposed to Lua.**
`world`/userdata/The world/No; `x`/int/X position/No; `y`/int/Y position/No; `z`/int/Z position/No; `block_id`/int/Minecraft block ID/No; `tick_rate`/int/Scheduled delay in ticks/No; `canceled`/boolean/Cancel the scheduling/Yes
### `tick_rate`
Fires to query/adjust the server tick rate.
`target_tps`/float/Desired ticks per second (default 20.0)/Yes; `tps_scale`/float/Multiplier applied to TPS (default 1.0)/Yes
```lua
minecraft.on(minecraft.events.tick_rate, {}, function(event)
  event.target_tps = 10.0  -- slow down to 10 TPS
end)
```
### `chunk_generation`
Fires during chunk generation for each stage/moment.
`stage`/string/Generation stage: `"terrain"`, `"surface"`, `"carver"`, `"features"`/No; `moment`/string/`"before"` or `"after"`/No; `cancel_vanilla`/boolean/Skip vanilla generation for this stage/Yes; `vanilla_stage_ran`/boolean/Whether vanilla ran/No; `world_seed`/int64/World seed/No; `has_world`/boolean/World context present/No; `world_name`/string/World save name/No; `is_overworld`/boolean/Overworld dimension flag/No; `mod_generation`/boolean/Mod generation flag/No; `chunk_x`/int/Active chunk X/No; `chunk_z`/int/Active chunk Z/No; `has_chunk`/boolean/Chunk context present/No; `remote`/boolean/Client replica flag/No; `side`/string/`"server"` or `"client"`/No
```lua
minecraft.on(minecraft.events.chunk_generation, {
  stage = "features", moment = "after",
  when = minecraft.util.real_world,
}, function(event)
  -- Place custom features after vanilla
end)
```
The `cancel_vanilla` field, when set to `true`, skips the vanilla generation for that stage at `"before"` moment.
### `create_world`
Fires when a new world is being created (before world generation begins).
`save_name`/string/World save folder name/No; `seed`/int64/World seed/No; `canceled`/boolean/Cancel world creation/Yes; `options`/table/Map of string → string options/Yes
```lua
minecraft.on(minecraft.events.create_world, {}, function(event)
  event.options["gamemode"] = "creative"
  event.seed = 42  -- not mutable (but options is)
end)
```
Only `options` and `canceled` are read back.
### `world_open`
Fires when a world is opened/loaded (before it starts ticking).
`save_name`/string/World save folder name/No; `new_world`/boolean/True if this is a newly-created world/No; `options`/table/Read-only map of world options/No; `has_world`/boolean/World context present/No; `world_name`/string/World save name/No; `is_overworld`/boolean/Overworld dimension flag/No; `mod_generation`/boolean/Mod generation flag/No; `remote`/boolean/Client replica flag/No; `side`/string/`"server"` or `"client"`/No
```lua
minecraft.on(minecraft.events.world_open, { new_world = true }, function(event)
  print("New world created: " .. event.save_name)
end)
```
### `world_start`
Fires when the world starts ticking for the first time.
`save_name`/string/World save folder name/No; `new_world`/boolean/True if new world/No; `has_world`/boolean/World context present/No; `world_name`/string/World save name/No; `is_overworld`/boolean/Overworld dimension flag/No; `mod_generation`/boolean/Mod generation flag/No; `remote`/boolean/Client replica flag/No; `side`/string/`"server"` or `"client"`/No
```lua
minecraft.on(minecraft.events.world_start, { when = minecraft.util.real_world }, function(event)
  print("World started: " .. event.save_name)
end)
```
### `world_spawn_search`
Fires when the game searches for a valid spawn position.
`x`/int/Spawn X position/Yes; `y`/int/Spawn Y position (default 64)/Yes; `z`/int/Spawn Z position/Yes; `resolved`/boolean/Whether spawn was found/Yes; `has_world`/boolean/World context present/No; `world_name`/string/World save name/No; `is_overworld`/boolean/Overworld dimension flag/No; `mod_generation`/boolean/Mod generation flag/No; `remote`/boolean/Client replica flag/No; `side`/string/`"server"` or `"client"`/No
```lua
minecraft.on(minecraft.events.world_spawn_search, {}, function(event)
  event.x = 0
  event.z = 0
  event.y = 80
  event.resolved = true
end)
```
## Entity Events
### `block_interact`
Fires when a player interacts with a block (right-click or left-click).
`x`/int/Block X position/No; `y`/int/Block Y position/No; `z`/int/Block Z position/No; `block_id`/int/Minecraft block ID at position/No; `right_click`/boolean/True if right-click, false if left-click/No; `canceled`/boolean/Cancel the interaction/Yes; `handled`/boolean/Mark as handled (also sets canceled in register_block wrapper)/Yes; `remote`/boolean/Client replica flag/No; `has_player`/boolean/Player present/No; `local_player`/boolean/True if this is the local client player/No; `has_item`/boolean/Player is holding an item/No; `player_x`/double/Player X position/No; `player_y`/double/Player Y position/No; `player_z`/double/Player Z position/No; `player_yaw`/float/Player yaw/No; `player_pitch`/float/Player pitch/No; `item_id`/int/Held item ID/No; `item_count`/int/Held item count/Yes; `item_damage`/int/Held item damage/Yes; `item_max_damage`/int/Maximum item damage/No; `item_damageable`/boolean/Whether item can take damage/No; `side`/string/`"server"` or `"client"`/No
```lua
minecraft.on(minecraft.events.block_interact, { right_click = true }, function(event)
  print("Block at " .. event.x .. "," .. event.y .. "," .. event.z .. " was right-clicked")
  event.handled = true
end)
```
### `entity_interact`
Fires when a player interacts with an entity (attack or right-click).
`attack`/boolean/True if attacking, false if right-clicking/No; `canceled`/boolean/Cancel the interaction/Yes; `handled`/boolean/Mark as handled/Yes; `sneaking`/boolean/Player is sneaking/No; `has_player`/boolean/Player present/No; `local_player`/boolean/True if local client player/No; `has_target`/boolean/Target entity present/No; `entity_id`/int/Target entity ID/No; `entity_type`/string/Target entity type string/No; `target_id`/int/Target entity network ID/No; `has_item`/boolean/Player holding item/No; `item_id`/int/Held item ID/No; `item_count`/int/Held item count/Yes; `item_damage`/int/Held item damage/Yes; `player_yaw`/float/Player yaw/No; `player_pitch`/float/Player pitch/No; `remote`/boolean/Client replica flag/No; `side`/string/`"server"` or `"client"`/No
```lua
minecraft.on(minecraft.events.entity_interact, { attack = true }, function(event)
  if event.entity_type == "minecraft:cow" then
    event.canceled = true  -- prevent cow attacks
  end
end)
```
### `entity_teleport`
Fires when an entity teleports.
`entity_id`/int/Entity ID/No; `entity_type`/string/Entity type string/No; `from_x`/double/Source X position/No; `from_y`/double/Source Y position/No; `from_z`/double/Source Z position/No; `x`/double/Destination X/Yes; `y`/double/Destination Y/Yes; `z`/double/Destination Z/Yes; `yaw`/float/Destination yaw/Yes; `pitch`/float/Destination pitch/Yes; `canceled`/boolean/Cancel the teleport/Yes; `has_entity`/boolean/Entity present/No; `has_player`/boolean/Player present/No; `has_world`/boolean/World context present/No; `world_name`/string/World save name/No; `is_overworld`/boolean/Overworld dimension flag/No; `mod_generation`/boolean/Mod generation flag/No
Note: This event is SKIPPED on remote (client) worlds — only fires on server side.
```lua
minecraft.on(minecraft.events.entity_teleport, {}, function(event)
  -- Redirect all endermen to the sky
  if event.entity_type == "minecraft:enderman" then
    event.y = 200
  end
end)
```
### `attack_damage`
Fires when a player attacks an entity and damage is calculated.
`damage`/int/Calculated damage amount/Yes; `critical`/boolean/Whether this is a critical hit/Yes; `canceled`/boolean/Cancel the attack entirely/Yes; `fall_distance`/float/Player fall distance/No; `on_ground`/boolean/Player on ground/No; `target_x`/double/Target entity X/No; `target_y`/double/Target entity Y/No; `target_z`/double/Target entity Z/No; `has_player`/boolean/Player present/No; `has_target`/boolean/Target present/No; `remote`/boolean/Client replica flag/No; `side`/string/`"server"` or `"client"`/No
```lua
minecraft.on(minecraft.events.attack_damage, {}, function(event)
  event.damage = event.damage * 2  -- double all damage
  event.critical = true             -- always critical
end)
```
### `player_travel`
Fires when a player moves/travels, before movement processing.
`sideways`/float/Sideways input (strafe)/Yes; `forward`/float/Forward input/Yes; `speed_multiplier`/float/Speed multiplier (default 1.0)/Yes; `has_player`/boolean/Player present/No; `is_local_player`/boolean/True if local client player/No; `remote`/boolean/Client replica flag/No; `side`/string/`"server"` or `"client"`/No
```lua
minecraft.on(minecraft.events.player_travel, { is_local_player = true }, function(event)
  event.speed_multiplier = 2.0  -- double player speed
end)
```
### `crafting_take`
Fires when a player takes an item from a crafting output slot. **Not exposed to Lua.**
`player`/userdata/The player/No; `stack`/userdata/The item stack taken/No; `canceled`/boolean/Cancel the take action/Yes
### `furnace_output_take`
Fires when a player takes an item from a furnace output slot. **Not exposed to Lua.**
`player`/userdata/The player/No; `stack`/userdata/The item stack taken/No; `canceled`/boolean/Cancel the take action/Yes
### `entity_spawn`
Fires when an entity is spawned into the world (client-only).
`entity_id`/int/Entity ID/No; `entity_type`/string/Entity type string/No; `item_id`/int/Item ID (if item entity)/No; `item_count`/int/Stack count (if item entity)/No; `item_damage`/int/Item damage (if item entity)/No; `texture_path`/string/Item texture path (if item entity)/No; `mod_texture`/boolean/Whether item uses mod texture/No; `atlas_index`/int/Atlas tile index (if vanilla texture)/No
```lua
minecraft.on(minecraft.events.entity_spawn, {}, function(event)
  if event.entity_type == "minecraft:item" then
    print("Item spawned: " .. event.item_id)
  end
end)
```
### `entity_remove`
Fires when an entity is removed from the world (client-only).
`entity_id`/int/Entity ID/No; `entity_type`/string/Entity type string/No; `item_id`/int/Item ID (if item entity)/No; `item_count`/int/Stack count (if item entity)/No; `item_damage`/int/Item damage (if item entity)/No; `texture_path`/string/Item texture path (if item entity)/No; `mod_texture`/boolean/Whether item uses mod texture/No; `atlas_index`/int/Atlas tile index (if vanilla texture)/No
```lua
minecraft.on(minecraft.events.entity_remove, { entity_type = "minecraft:item" }, function(event)
  -- track item despawn
end)
```
### `entity_tick`
Fires every tick for each entity in the world.
`entity_id`/int/Entity ID/No; `entity_type`/string/Entity type string/No; `x`/double/Entity X position/No; `y`/double/Entity Y position/No; `z`/double/Entity Z position/No; `yaw`/float/Entity yaw/No; `pitch`/float/Entity pitch/No; `remote`/boolean/Client replica flag/No; `canceled`/boolean/Cancel the entity tick/Yes; `side`/string/`"server"` or `"client"`/No
```lua
minecraft.on(minecraft.events.entity_tick, { entity_type = "minecraft:zombie" }, function(event)
  -- zombies tick slower
  event.canceled = true  -- would freeze zombie AI
end)
```
## Client Events
### `client_tick`
Fires every client tick (both before and after world tick, and when paused).
`before`/boolean/True in pre-tick phase, false after/No; `after_world`/boolean/True in the post-world-tick sub-phase/No; `paused`/boolean/Game is paused (menu open)/No; `has_player`/boolean/Player present/No; `has_world`/boolean/World context present/No; `world_name`/string/World save name/No; `is_overworld`/boolean/Overworld dimension flag/No; `mod_generation`/boolean/Mod generation flag/No; `remote`/boolean/Client replica flag/No; `side`/string/`"server"` or `"client"`/No; `camera_y`/double/Camera entity Y position/No; `player_y`/double/Player Y position/No; `player_fall_distance`/float/Player fall distance/No; `player_on_ground`/boolean/Player on ground/No; `world_time`/double/World time mod 24000/No; `is_night`/boolean/Nighttime check/No
```lua
minecraft.on(minecraft.events.client_tick, { before = true, when = minecraft.util.real_world }, function(event)
  -- HUD update logic
  if event.is_night then
    -- render night vision overlay
  end
end)
```
### `raycast`
Fires when the client performs a raycast (crosshair targeting).
`has_hit`/boolean/Whether anything was hit/No; `type`/string/Hit type: `"block"`, `"entity"`, or `"none"`/No; `hit_x`/double/Hit position X/No; `hit_y`/double/Hit position Y/No; `hit_z`/double/Hit position Z/No; `block_x`/int/Block X (if block hit)/No; `block_y`/int/Block Y (if block hit)/No; `block_z`/int/Block Z (if block hit)/No; `side`/int/Block face hit (0-5)/No; `block_id`/int/Block ID at position/No; `block_name`/string/Block wire name/No; `item_id`/int/Same as block_id for block hits/No; `entity_id`/int/Entity ID (if entity hit)/No; `entity_type`/string/Entity type (if entity hit)/No; `entity_raw_id`/int/Entity raw registry ID/No; `entity_x`/double/Entity X (if entity hit)/No; `entity_y`/double/Entity Y (if entity hit)/No; `entity_z`/double/Entity Z (if entity hit)/No; `side`/string/`"server"` or `"client"`/No
```lua
minecraft.on(minecraft.events.raycast, {}, function(event)
  if event.has_hit and event.type == "block" then
    print("Looking at block " .. event.block_id .. " at " .. event.block_x .. "," .. event.block_y .. "," .. event.block_z)
  end
end)
```
### `key_press`
Fires on key press/release events.
`key`/int/Key code (see `minecraft.keys` for common values)/No; `pressed`/boolean/True if pressed, false if released/No; `repeat`/boolean/True if this is a repeat event (held key)/No; `handled`/boolean/Mark as handled (prevents further processing)/Yes
```lua
minecraft.on(minecraft.events.key_press, { key = minecraft.keys.space, pressed = true }, function(event)
  print("Space bar pressed!")
  event.handled = true
end)
```
### `mouse_button`
Fires on mouse button press/release events.
`button`/int/Mouse button (0 = left, 1 = right, 2 = middle)/No; `pressed`/boolean/True if pressed, false if released/No; `handled`/boolean/Mark as handled/Yes
```lua
minecraft.on(minecraft.events.mouse_button, { button = 0, pressed = true }, function(event)
  print("Left click!")
end)
```
### `screen_event`
Fires for custom Lua screens with phase-based dispatch.
`screen_id`/string/Screen identifier/No; `phase`/string/Phase: `"init"`, `"render"`, `"tick"`, `"key"`, `"mouse"`, `"scroll"`, `"close"`/No; `width`/int/Screen width/No; `height`/int/Screen height/No; `mouse_x`/int/Mouse X position/No; `mouse_y`/int/Mouse Y position/No; `x`/int/Alias for mouse_x/No; `y`/int/Alias for mouse_y/No; `tick_delta`/float/Render tick delta/No; `key`/int/Key code (key phase)/No; `char`/int/Character code (key phase, unsigned)/No; `button`/int/Mouse button (mouse phase)/No; `released`/boolean/True if button released (mouse phase)/No; `delta`/int/Scroll delta (scroll phase)/No; `handled`/boolean/Mark as handled/Yes
The `minecraft.screen.on_lua_screen()` wrapper dispatches by phase:
```lua
minecraft.screen.on_lua_screen("my_mod:screen", {
  init = function(event) end,
  render = function(event) end,
  mouse = function(event) end,
  key = function(event) end,
  close = function(event) end,
})
```
### `screen_ui`
Fires for screen UI injection into vanilla or mod screens.
`screen_id`/string/The target screen identifier/No; `region`/string/UI region name/No; `host_fields`/table/Host screen fields table/No; `ui`/table/UI helper table with `add_stacked_centered_button` etc/No
The `minecraft.screen.on_ui()` wrapper subscribes to this:
```lua
minecraft.screen.on_ui("options", "options", function(event)
  event.ui:add_stacked_centered_button("My Mod Settings", open_callback)
end)
```
### `screen_region`
Fires for screen rendering regions on Lua-hosted screens (render, mouse click, mouse scroll phases).
`phase_name`/string/`"render"`, `"mouse_click"`, or `"mouse_scroll"`/No; `screen_id`/string/Screen identifier/No; `region`/string/Region name/No; `mouse_x`/int/Mouse X/No; `mouse_y`/int/Mouse Y/No; `button`/int/Mouse button/No; `scroll_delta`/int/Scroll delta/No; `x`/int/Region X/No; `y`/int/Region Y/No; `width`/int/Region width/Yes; `height`/int/Region height/Yes; `handled`/boolean/Mark as handled/Yes
## Render Events
### `camera_setup`
Fires during camera setup, before rendering. All position/rotation fields are mutable.
`tick_delta`/float/Render tick delta/No; `x`/double/Camera X position/Yes; `y`/double/Camera Y position/Yes; `z`/double/Camera Z position/Yes; `yaw`/float/Camera yaw (degrees)/Yes; `pitch`/float/Camera pitch (degrees)/Yes; `roll`/float/Camera roll (degrees)/Yes; `custom_view`/boolean/Flag indicating custom view mode/Yes; `hide_first_person_hand`/boolean/Hide the first-person hand model/Yes
```lua
minecraft.on(minecraft.events.camera_setup, {}, function(event)
  event.roll = 15  -- tilt the camera
  event.custom_view = true
end)
```
### `render_frame`
Fires at the beginning of each rendered frame.
`tick_delta`/float/Render tick delta (partial ticks)/No
```lua
minecraft.on(minecraft.events.render_frame, {}, function(event)
  -- frame-level render logic
end)
```
### `render_targets`
Fires after framebuffer targets are set up.
`tick_delta`/float/Render tick delta/No
### `fov`
Fires to query/adjust the field of view.
`tick_delta`/float/Render tick delta/No; `fov`/float/Field of view in degrees (default 70.0)/Yes
```lua
minecraft.on(minecraft.events.fov, {}, function(event)
  event.fov = 120  -- wider FOV
end)
```
### `world_render`
Fires around each world rendering stage. Provides `minecraft.render.stages` and `minecraft.render.moments` constants for filtering.
`tick_delta`/float/Render tick delta/No; `stage`/string/Render stage: `"sky"`, `"stars"`, `"terrain_opaque"`, `"entities"`, `"particles_lit"`, `"particles"`, `"terrain_translucent"`, `"weather"`, `"clouds"`, `"hand"`, `"framebuffer"`/No; `moment`/string/`"before"` or `"after"`/No; `cancel_vanilla`/boolean/Skip vanilla rendering for this stage/Yes; `vanilla_stage_ran`/boolean/Whether vanilla stage executed/No; `shadow_pass`/boolean/`true` while an offscreen shadow-depth pass renders entities/No; `celestial_angle`/float/Celestial angle (only read/written at sky/before)/Yes; `sky_yaw_deg`/float/Sky yaw in degrees (sky/before)/Yes; `star_brightness`/float/Star brightness (stars/before only)/Yes; `rain_strength`/float/Current rain strength/No; `stars_enabled`/boolean/Whether stars are visible/No; `astronomy_enabled`/boolean/Enable astronomy mode/Yes; `astronomy_utc_millis`/double/Astronomy UTC time in ms/Yes; `observer_latitude_deg`/float/Observer latitude/Yes; `observer_longitude_deg`/float/Observer longitude/Yes; `world_time`/double/World time mod 24000/No; `celestial`/double/Normalized celestial angle/No; `is_night`/boolean/Nighttime/No; `camera_x`/double/Camera X world position/No; `camera_y`/double/Camera Y world position/No; `camera_z`/double/Camera Z world position/No; `camera_yaw`/double/Camera yaw/No; `camera_pitch`/double/Camera pitch/No; `camera_roll`/double/Camera roll/No; `custom_camera`/boolean/Custom camera active/No; `cloud_base_height`/float/Cloud height offset (clouds stage only)/No; `has_world`/boolean/World context present/No; `world_name`/string/World save name/No; `is_overworld`/boolean/Overworld flag/No; `mod_generation`/boolean/Mod generation flag/No
```lua
minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.sky,
  moment = minecraft.render.moments.before,
}, function(event)
  event.cancel_vanilla = true        -- custom sky
  event.celestial_angle = 0.25       -- set to noon
  event.astronomy_enabled = true
  event.observer_latitude_deg = 51.5
end)
```
### `first_person_hand`
Fires before rendering the first-person hand model.
`tick_delta`/float/Render tick delta/No; `width`/int/Framebuffer target width/No; `height`/int/Framebuffer target height/No; `eye`/int/Eye index (0 = main hand, 1 = offhand)/No; `canceled`/boolean/Skip rendering the hand/Yes; `entity_id`/int/Camera entity ID/No; `entity_type`/string/Camera entity type/No
```lua
minecraft.on(minecraft.events.first_person_hand, { eye = 0 }, function(event)
  event.canceled = true  -- hide main hand
end)
```
### `world_color`
Fires to query/adjust world colors (sky and fog).
`partial_ticks`/float/Render tick delta/No; `kind`/string/Color kind: `"sky"` or `"fog"`/No; `r`/double/Red component (0.0-1.0)/Yes; `g`/double/Green component (0.0-1.0)/Yes; `b`/double/Blue component (0.0-1.0)/Yes; `celestial`/double/Normalized celestial angle/No; `world_time`/double/World time mod 24000/No; `is_night`/boolean/Nighttime/No; `has_world`/boolean/World present/No; `world_name`/string/World save name/No; `is_overworld`/boolean/Overworld flag/No; `mod_generation`/boolean/Mod generation flag/No
```lua
minecraft.on(minecraft.events.world_color, { kind = minecraft.colors.sky }, function(event)
  event.r = 1.0  -- red sky
  event.g = 0.0
  event.b = 0.0
end)
```
### `pre_entity_render`
Fires before an entity is rendered.
`entity_id`/int/Entity ID/No; `entity_type`/string/Entity type string/No; `tick_delta`/float/Render tick delta/No; `canceled`/boolean/Skip rendering this entity/Yes; `item_id`/int/Item ID (if item entity)/No; `item_count`/int/Stack count (if item entity)/No; `item_damage`/int/Item damage (if item entity)/No; `texture_path`/string/Item texture path/No; `mod_texture`/boolean/Uses mod texture/No; `atlas_index`/int/Atlas tile index/No
```lua
minecraft.on(minecraft.events.pre_entity_render, { entity_type = "minecraft:item" }, function(event)
  if event.item_id == 10 then  -- skip rendering a specific item
    event.canceled = true
  end
end)
```
### `entity_render`
Fires during entity rendering to apply pose overrides (rotation, scale, offset, model part overrides).
`entity_id`/int/Entity ID/No; `entity_type`/string/Entity type string/No; `is_player`/boolean/Whether this is a player entity/No; `tick_delta`/float/Render tick delta/No; `pose.body_yaw`/float/Body yaw (degrees)/Yes; `pose.head_yaw`/float/Head yaw (degrees)/Yes; `pose.head_pitch`/float/Head pitch (degrees)/Yes; `pose.limb_swing`/float/Limb swing phase (radians)/Yes; `pose.limb_distance`/float/Limb distance (0.0-1.0)/Yes; `pose.yaw`/float/Entity yaw (degrees)/Yes; `pose.pitch`/float/Entity pitch (degrees)/Yes; `pose.roll`/float/Entity roll (degrees)/Yes; `pose.scale`/float/Entity scale multiplier/Yes; `pose.offset_x`/float/World-space X offset/Yes; `pose.offset_y`/float/World-space Y offset/Yes; `pose.offset_z`/float/World-space Z offset/Yes; `pose.parts`/table/Map of part name → `{ yaw, pitch, roll }` (NaN = leave as-is)/Yes
```lua
minecraft.on(minecraft.events.entity_render, { entity_type = "minecraft:zombie" }, function(event)
  event.pose.scale = 2.0           -- giant zombies
  event.pose.roll = 180            -- upside down
  event.pose.parts.head = { yaw = 45, pitch = 30, roll = 0 }
end)
```
## TileEntity Events
### `pre_tile_entity_render`
Fires before a block entity is rendered.
`x`/int/Block entity X/No; `y`/int/Block entity Y/No; `z`/int/Block entity Z/No; `id`/string/Block entity ID/No; `tick_delta`/float/Render tick delta/No; `canceled`/boolean/Skip rendering/Yes
```lua
minecraft.on(minecraft.events.pre_tile_entity_render, { id = "chest" }, function(event)
  event.canceled = true  -- hide all chests
end)
```
### `tile_entity_tick`
Fires every tick for each block entity. The engine also drives animation.
`x`/int/Block entity X/No; `y`/int/Block entity Y/No; `z`/int/Block entity Z/No; `id`/string/Block entity type ID/No; `remote`/boolean/Client replica flag/No; `removed`/boolean/Entity is marked removed/No; `canceled`/boolean/Cancel the tick/Yes; `world_time`/double/Current world time/No; `animation_frame`/int/Current animation frame/No; `animation_tick`/double/Current animation tick/No; `animation_speed`/float/Animation speed multiplier/Yes; `entity`/userdata/Block entity handle/No
```lua
minecraft.on(minecraft.events.tile_entity_tick, { id = "beehive" }, function(event)
  event.animation_speed = 2.0  -- speed up beehive animation
end)
```
## Lifecycle Event
The lifecycle phase transition uses `LifecycleEvent` internally, exposed via `minecraft.at_phase()`:
`previous`/string/Previous phase name (e.g., `"init"`)/No; `current`/string/Current phase name (e.g., `"post_init"`)/No
```lua
minecraft.at_phase("ready", 0, function(event)
  print("From phase " .. event.previous .. " to " .. event.current)
  -- All registration is complete. Register runtime callbacks here.
  minecraft.on(minecraft.events.world_tick, { when = minecraft.util.real_world }, function(tick)
    -- ...
  end)
end)
```
Notable details from `ModLifecycle::advanceTo()`:
```lua
static void advanceTo(LifecyclePhase phase) {
  LifecycleEvent event{phaseStorage(), phase};
  phaseStorage() = phase;
  hooks().publish(event);
}
```
The event fires AFTER the internal phase has been updated, so `event.previous` is the old numeric enum (`0`–`3`) and `event.current` is the new numeric enum.
## Unsupported Event Names
The following structs exist as C++ event types but are **not registered in the Lua subscriber map** and will produce `"unsupported Lua hook event"` warnings if subscribed:
`weather_cycle` `lightning_strike` `snow_ice_placement` `random_block_tick` `scheduled_block_tick` `schedule_block_update` `world_time` `crafting_take` `furnace_output_take`
These may be exposed in future versions.

# API Functions
All functions are accessed through the global `minecraft` table injected into every Lua mod's sandbox. Mods also have `os.clock`, `os.date`, `os.difftime`, `os.time`, `math`, `string`, `table`, and the sandboxed `require`/`minecraft.require`. `io`, `debug`, `dofile`, `loadfile`, `package.cpath`, `package.loadlib`, and `package` itself are removed.
## Logging
### `minecraft.log(level?, message)`
Writes a line to stdout with the prefix `[lua-mod:<modId>:<level>]`.
**`level`** — `"info"` (default), `"warn"`, or `"error"`. **`message`** — The text to log.
```lua
minecraft.log("info", "block placed")
minecraft.log("warn", "deprecated api used")
minecraft.log("error", "something broke")
minecraft.log("just a string")        -- level defaults to "info"
```
## Context
### `minecraft.is_client()`
Returns `true` if the current Lua execution context is on the client side (logical client, including integrated server). Server-side or dedicated-server execution returns `false`.
```lua
if minecraft.is_client() then
  print("running on client")
end
```
## Time
### `minecraft.time.utc_millis()`
Returns the current UTC epoch time in milliseconds as a number.
```lua
local now = minecraft.time.utc_millis()
```
## Input (Client Only)
### `minecraft.is_key_down(keyCode)`
Returns `true` if the keyboard key with the given scancode is currently held. Always returns `false` on the server. Use `minecraft.key_code` to convert a name to a scancode.
```lua
if minecraft.is_key_down(minecraft.key_code("space")) then
  -- player is holding space
end
```
### `minecraft.is_mouse_down(button)`
Returns `true` if the given mouse button is pressed (`0` = left, `1` = right, `2` = middle). Always returns `false` on the server.
### `minecraft.key_code(name)`
Converts a named key to a keyboard scancode integer. Accepts:
**Direct key names**: `"escape"`, `"1"`–`"0"`, `"q"`, `"w"`, `"e"`, `"r"`, `"t"`, `"y"`, `"u"`, `"i"`, `"o"`, `"p"`, `"enter"`, `"a"`, `"s"`, `"d"`, `"f"`, `"g"`, `"h"`, `"j"`, `"k"`, `"l"`, `"z"`, `"x"`, `"c"`, `"v"`, `"b"`, `"n"`, `"m"`, `"space"`, `"up"`, `"left_arrow"`, `"right_arrow"`, `"down"` **Binding names** (client only, reads the player's actual keybind): `"forward"` / `"move_forward"`, `"left"` / `"move_left"`, `"back"` / `"backward"` / `"move_back"`, `"right"` / `"move_right"`, `"jump"`, `"sneak"`, `"drop"`, `"inventory"`, `"chat"`, `"fog"`
If passed a number, it is returned as-is (useful pass-through).
```lua
local jumpKey = minecraft.key_code("jump")
local wKey = minecraft.key_code("w")
```
Convenience constants are also available on `minecraft.keys`:
```lua
minecraft.keys = { escape = 1, enter = 28, space = 57, up = 200, down = 208 }
```
## Options (Client Only)
### `minecraft.options.get(key)`
Reads a game option value. Supports any `OptionRegistry` persist key (e.g. `"gfx_brightness"`, `"view_distance"`, `"mouse_sensitivity"`) as well as keybind names (e.g. `"key_forward"`, `"key_jump"`, or just `"forward"`, `"jump"`). Also supports special names: `"skin"`, `"lastServer"`, `"fancyGraphics"`, `"thirdPerson"`, `"hideHud"`, `"renderClouds"` / `"clouds_enabled"`. Returns `nil` for unknown keys.
```lua
local brightness = minecraft.options.get("gfx_brightness")
```
### `minecraft.options.keys()`
Returns an array table of all option persist key strings.
```lua
for _, key in ipairs(minecraft.options.keys()) do
  print(key, minecraft.options.get(key))
end
```
## Session
### `minecraft.session.*`
Functions for reading and writing the runtime identity. All client-only.
``set_offline_username(name)``: the offline-mode username override; ``clear_offline_username()``: Clears the offline-mode username override; ``is_offline_mode()``: `true` if an offline override is active; ``get_offline_username()``: the current offline username string; ``get_username()``: the live session's username (Mojang or offline); ``is_authenticated()``: `true` if the session has valid Microsoft auth
```lua
minecraft.session.set_offline_username("MyModPlayer")
print(minecraft.session.get_username())
```
## Asset & File IO
### `minecraft.asset_path(relative)`
Returns the absolute filesystem path to a mod asset. The path is resolved relative to the mod's asset root directory. Returns `nil` if the mod has no asset root.
```lua
local path = minecraft.asset_path("textures/block/my_block.png")
```
### `minecraft.read_asset(relative)`
Reads a mod asset file and returns its content as a string. Returns `nil` if the file does not exist.
```lua
local data = minecraft.read_asset("data/table.json")
```
### `minecraft.read_asset_bytes(relative, options?)`
Reads a mod asset file as a binary string (byte data). The `options` table may contain `{gzip = true}` to decompress gzip-encoded data. Returns `nil` on failure.
```lua
local compressed = minecraft.read_asset_bytes("data/payload.bin")
local decompressed = minecraft.read_asset_bytes("data/payload.gz", {gzip = true})
```
### `minecraft.read_nbt_asset(relative)`
Reads a `.dat` or `.nbt` file as a Lua table. Automatically detects gzip compression (checks magic bytes `1f 8b`). Returns the parsed table plus a nil error, or `nil` plus an error string on failure. Files larger than 64 MiB are rejected.
```lua
local nbt, err = minecraft.read_nbt_asset("structures/my_building.nbt")
if nbt then
  -- use the table
end
```
### `minecraft.storage.read(path)`
Reads a file from the mod's persistent storage directory (`runDir/config/mods/<sanitizeName(modId)>/`). The mod ID is sanitized before it becomes a directory name. Returns the file content as a string, or `nil` if the file does not exist. Path traversal (`..`) is rejected.
```lua
local data = minecraft.storage.read("settings.txt")
```
### `minecraft.storage.write(path, content)`
Writes content to the mod's sanitized persistent-storage directory. Creates intermediate directories as needed. Returns `true` on success, `false` on failure. Path traversal (`..`) is rejected.
```lua
minecraft.storage.write("scores.dat", "player1: 100")
```
## Config File Parser
### `minecraft.config.load(path, defaults, options?)`
Loads a key-value config file from the mod's storage directory. Each line is parsed as `key = value` or `key: value`. Lines starting with `#` or `;` are ignored.
``path``: Relative path within the mod's storage directory; ``defaults``: Table of default values (sets expected types and fallback values); ``options.aliases``: Table mapping old key names to current key names; ``options.separator``: Custom separator (default `"="`)
Returns `values, loaded` — `values` is a table with the parsed (or default) values, `loaded` is `true` if the file existed and was read.
The type of each default value controls parsing: **boolean** — parsed via `util.parse_boolean` (accepts `true`/`false`, `1`/`0`, `yes`/`no`, `on`/`off`) **number** — parsed via `tonumber` **string** — raw value (empty string preserves default)
```lua
local defaults = {
  brightness = 1.0,
  enable_fog = true,
  name = "default"
}
local config, loaded = minecraft.config.load("graphics.cfg", defaults)
if not loaded then
  -- file didn't exist, using defaults
end
```
### `minecraft.config.save(path, values, options?)`
Writes a key-value config file to the mod's storage directory. Returns `true` on success.
``path``: Relative path within the mod's storage directory; ``values``: Table of key-value pairs to write; ``options.keys``: Ordered array of keys to write (default: sorted keys); ``options.names``: Table mapping output key names (for aliasing); ``options.separator``: Custom separator (default `"="`)
```lua
minecraft.config.save("graphics.cfg", {
  brightness = 0.8,
  enable_fog = false
}, {
  keys = {"brightness", "enable_fog"},
  separator = ":"
})
```
## Mod Lifecycle
### `minecraft.at_phase(phase_name, order, callback)`
Registers a callback to run during a specific lifecycle phase. The phase name is a string, but the callback receives numeric enum fields: `previous` and `current` (`NotStarted = 0`, `Init = 1`, `PostInit = 2`, `Ready = 3`). This callback is subscribed immediately while the mod script is loading.
**Available phase names** (in order):
``"init"``: all content: blocks, items, entities, etc; ``"post_init"``: Resolve cross-references and register recipes; ``"ready"``: All registration complete, game is live
The phase name is case-insensitive. The `order` parameter controls execution order within a phase (lower numbers run first). `minecraft.lifecycle` is a table with all phase names as keys for convenience.
```lua
minecraft.at_phase("init", 100, function(event)
  print("phase ordinal:", event.current)
end)
```
### `minecraft.on(event_name, options, callback)`
Subscribes to a game event. The `options` table supports:
``priority``: Integer priority (higher = runs later; default 0); ``once``: If `true`, unsubscribes after the first invocation; `Any event field`: Filter: the callback only fires if `event[field]` matches. Can be a literal value, a table of acceptable values, or a predicate function; ``when``: Function `(event) -> boolean` — fires only when it returns true
The callback receives the event table and should return the (possibly mutated) event table.
```lua
minecraft.on(minecraft.events.client_tick, {priority = 50}, function(event)
  -- called every client tick
  return event
end)

-- Filter by block_id and right_click
minecraft.on(minecraft.events.block_interact, {
  block_id = 42,
  right_click = true,
  priority = 10
}, function(event)
  print("right-clicked block 42 at", event.x, event.y, event.z)
  event.handled = true
  event.canceled = true
  return event
end)

-- Using a predicate
minecraft.on(minecraft.events.entity_tick, {
  when = function(e) return e.entity_type == "Zombie" end
}, function(event)
  return event
end)
```
See the [event reference](#event-reference) section below for supported event names and their fields.
## Lua Helpers
### `minecraft.require(name)`
Safe module loader. Only allows module names matching `[%w_.-]+` (alphanumeric, underscore, dot, hyphen). Path traversal is rejected. Modules are searched via the mod's asset path.
```lua
local mylib = minecraft.require("mylib")
local utils = minecraft.require("utils.helpers")
```
Inside `require`'d files, `require` is also sandboxed to `minecraft.require`.
### `minecraft.util.clamp(value, min, max)`
Clamps a number to the inclusive range `[min, max]`.
```lua
local x = minecraft.util.clamp(15, 0, 10)  -- 10
```
### `minecraft.util.trim(str)`
Strips leading and trailing whitespace from a string.
```lua
local cleaned = minecraft.util.trim("  hello  ")  -- "hello"
```
### `minecraft.util.in_rect(x, y, left, top, width, height)`
Returns `true` if point `(x, y)` lies within the rectangle. The right edge is exclusive (`x < left + width`), the bottom edge is inclusive of `top` and exclusive of `top + height`.
```lua
if minecraft.util.in_rect(mouse_x, mouse_y, 10, 20, 100, 30) then
  -- mouse is inside the rectangle
end
```
### `minecraft.util.real_world(event)`
Returns `true` if the event originates from the "real" world (not mod generation). Checks `event.mod_generation ~= false`.
```lua
if minecraft.util.real_world(event) then
  -- not during chunk generation
end
```
### `minecraft.util.parse_boolean(value, fallback)`
Parses a string as a boolean. Accepts `"true"`, `"1"`, `"yes"`, `"on"` → `true`; `"false"`, `"0"`, `"no"`, `"off"` → `false`. Returns `fallback` for `nil`, empty strings, or unrecognized values.
```lua
local ok = minecraft.util.parse_boolean("yes", false)   -- true
local ok = minecraft.util.parse_boolean("maybe", false)  -- false (fallback)
```
### `minecraft.util.copy(table)`
Performs a shallow copy of a table.
```lua
local t2 = minecraft.util.copy(t1)
```
## JSON API
### `minecraft.util.json_encode(value)`
Encodes a Lua table to a JSON string. Array-like tables with consecutive integer keys from 1 are encoded as arrays; other tables with string keys are encoded as objects. The top-level argument must be a table; scalar and `nil` values are rejected. Returns the JSON string on success, or `nil` + error on failure.
```lua
local json = minecraft.util.json_encode({name = "test", values = {1, 2, 3}})
-- '{"name":"test","values":[1,2,3]}'
```
### `minecraft.util.json_decode(string)`
Decodes a JSON string to a Lua value. Supports objects → tables with string keys, arrays → tables with integer keys, strings, numbers, booleans, and `null` → light userdata (use `minecraft.util.json_null` to compare). Returns the decoded value on success, or `nil` + error string on failure.
```lua
local data = minecraft.util.json_decode('{"x":1,"y":2}')
print(data.x, data.y)

-- Testing for null
if result == minecraft.util.json_null then
  -- JSON null
end
```
### `minecraft.util.json_null`
A sentinel value representing JSON `null` in decoded data. Compare with `==`:
```lua
if value == minecraft.util.json_null then
  -- was null in JSON
end
```
## NBT
NBT values read via `minecraft.read_nbt_asset` are automatically converted to Lua tables:
``Compound``: Table with string keys; ``List``: Array table (1-indexed); ``Byte`, `Short`, `Int`, `Long``: Integer; ``Float`, `Double``: Number; ``String``: String; ``ByteArray``: Binary string; ``IntArray``: Array table of integers; ``LongArray``: Array table of integers; ``End``: `nil`
Depth is limited to 256 levels.
To convert a Lua table back to NBT (e.g. for entity or tile entity data), the engine uses `luaValueToNbt` internally: booleans become bytes, integers become ints, floats become doubles, strings become strings, and tables with string keys become compounds. This conversion is used automatically for entity data, tile entity data, and similar write paths.
## Item Queries
### `minecraft.items.ids()`
Returns an array table of all registered item IDs.
```lua
for _, id in ipairs(minecraft.items.ids()) do
  print(id)
end
```
### `minecraft.items.describe(item_id)`
Returns a table describing an item, or `nil` if the ID is unknown. Fields:
`id`/int/Numeric item ID; `max_damage`/int/Maximum damage value; `damageable`/bool/Whether the item can take damage; `stackable`/bool/Whether the item stacks; `has_subtypes`/bool/Whether the item uses metadata/damage for variants; `max_count`/int/Maximum stack size
```lua
local info = minecraft.items.describe(256)
if info then
  print(info.id, info.max_count, info.damageable)
end
```
## World
### `minecraft.world.block_id(name)`
Resolves a block name to its numeric ID. Supports vanilla block names (e.g. `"stone"`, `"dirt"`, `"grass_block"`) and mod block wire names. Returns `0` for unknown names.
```lua
local stoneId = minecraft.world.block_id("stone")
```
### `minecraft.world.get_block(x, y, z)`
Returns the block ID at the given world position.
```lua
local id = minecraft.world.get_block(100, 64, 200)
```
### `minecraft.world.random(bound?)`
Returns a random integer from `[0, bound)` using the world's random source. `bound` defaults to 1000.
```lua
local roll = minecraft.world.random(100)  -- 0..99
```
### `minecraft.world.is_night()`
Returns `true` if the world time is between 13000 and 23000 ticks.
```lua
if minecraft.world.is_night() then
  -- spawn mobs
end
```
### `minecraft.world.get_time()`
Returns the current world time in ticks, or `0` when no world is available.
```lua
local time = minecraft.world.get_time()
```
### `minecraft.world.get_top_y(x, z)`
Returns one above the highest solid or fluid block at the given column, or `-1` if no world is available.
```lua
local y = minecraft.world.get_top_y(100, 200)
```
### `minecraft.world.player()`
Returns a table with the current player's position `{x, y, z}`, or `nil` if no player is available.
```lua
local pos = minecraft.world.player()
if pos then
  print("player at", pos.x, pos.y, pos.z)
end
```
### `minecraft.world.spawn_entity(entityId, position)`
Spawns an entity into the world. Accepts a string entity type ID and a position (table with `x, y, z`, or three individual numeric arguments). Only works on the server side (`!world.isRemote()`). Returns `true` on success.
```lua
-- Via table
minecraft.world.spawn_entity("Zombie", {x = 100, y = 64, z = 200})

-- Via individual coords
minecraft.world.spawn_entity("Creeper", 100, 64, 200)
```
### `minecraft.world.count_entities(entityId)`
Returns the count of entities with the given type ID in the world.
```lua
local count = minecraft.world.count_entities("Zombie")
```
### `minecraft.world.set_time(tick)`
Sets the world time (in ticks). Server-side only.
```lua
minecraft.world.set_time(6000)  -- set to noon
```
### `minecraft.world.marker_px(grid, world_x, world_z)`
Converts world coordinates to grid pixel coordinates, clamping to `[0, grid.side - 1]`. Returns `col, row`.
```lua
local col, row = minecraft.world.marker_px(grid, playerX, playerZ)
```
## Chunk Context (Generation)
The chunk helpers are exposed as `event.chunk` only while a `chunk_generation` callback is running. There is no global `minecraft.chunk` table.
### `event.chunk:set_block(localX, y, localZ, blockId)`
Sets a block within the currently generating chunk. Coordinates are local to the chunk (0–15 for X and Z, 0–127 for Y). Only usable during `chunk_generation` event callbacks. Returns `true` on success.
```lua
event.chunk:set_block(7, 40, 7, 1)  -- place stone
```
### `event.chunk:fill(x1, y1, z1, x2, y2, z2, blockId)`
Fills a cuboid within the chunk with the given block ID. Coordinates are clamped to chunk bounds. Returns the number of blocks changed. Only usable during `chunk_generation`.
```lua
local changed = event.chunk:fill(0, 0, 0, 15, 0, 15, 1)
```
### `event.chunk:get_block(localX, y, localZ)`
Gets the block ID at a local chunk position. Returns 0 if no chunk context is active.
### `event.chunk:get_height(localX, localZ)`
Gets the height value at a local chunk column. Returns 0 if no chunk context is active.
## Entities
### `minecraft.entities.list(filter?)`
Returns an array table of entity handle objects in the current world, optionally filtered by entity type string. Each entity handle object exposes properties and methods.
### Properties
``id``: Entity network ID; ``type``: Entity type string (e.g. `"Zombie"`); ``registry_id``: (mod entities only) Registry ID string; ``data``: (mod entities only) NBT data table; ``x`, `y`, `z``: Position; ``vx`, `vy`, `vz``: Velocity; ``yaw`, `pitch``: Rotation; ``on_ground``: Boolean ground state; ``item_id`, `item_count`, `item_damage`, `item_max_damage``: (Item entities only); ``texture_path`, `mod_texture`, `atlas_index``: (Item entities only)
### Methods
`entity:teleport(position | x, y, z, yaw?, pitch?)` — Teleports the entity. Accepts a table `{x, y, z, yaw?, pitch?}` or individual coordinate arguments. Returns `true` on success. Server-side only. `entity:apply_state(state)` — Applies state mutations. Accepts a state table with optional fields `x`, `y`, `z`, `vx`, `vy`, `vz`, `yaw`, `pitch`, and `data`. Returns `true` on success. Server-side only. `entity:remove()` — Marks the entity for removal. Returns `true` on success. Server-side only.
```lua
local zombies = minecraft.entities.list("Zombie")
for _, z in ipairs(zombies) do
  print(z.id, z.x, z.y, z.z)
  z:teleport(z.x, z.y + 10, z.z)
end
```
### `minecraft.entities.get(id)`
Returns the entity handle object for the given network ID, or `nil` if not found.
```lua
local ent = minecraft.entities.get(42)
if ent then
  ent:apply_state({vx = 0.0, vy = 0.5, vz = 0.0})
end
```
### `minecraft.entities.spawn_mod(registryId, spec)`
Spawns a custom Lua mod entity. The `registryId` must be `"<modId>:<your_type>"`. The `spec` table accepts:
``x`, `y`, `z``: Position; ``yaw`, `pitch``: Rotation; ``data``: Table to store as entity NBT data
Returns the spawned entity handle object on success, or `nil` on failure.
Mod entities cast a vanilla blob shadow sized from their width. Put a `shadow_radius` float in `data` to override it (`shadow_radius = 0` disables the shadow).
```lua
local ent = minecraft.entities.spawn_mod("mymod:custom_entity", {
  x = 100, y = 64, z = 200, data = {health = 50}
})
if ent then
  print("Spawned entity ID: " .. ent.id)
end
```
### `minecraft.entities.register_global_pose_hook(entityType, callback)`
Registers a pose hook for all entities of a given type. The callback receives an event with `entity_id`, `entity_type`, `tick_delta`, and `pose` (which can be read back and mutated).
### `minecraft.entities.register_local_pose_hook(entityId, callback)`
Like `register_global_pose_hook` but only for a specific entity by ID.
### `minecraft.entities.unregister_local_pose_hook(entityId)`
Removes a previously registered local pose hook.
## Particles
### `minecraft.particles.spawn(spec)`
Spawns a client-side particle. All fields are optional except the table itself.
`x`, `y`, `z`/`0, 64, 0`/Position; `vx`, `vy`, `vz`/`0, 0, 0`/Velocity; `scale`/`4.0`/Particle size (clamped `0.05`–`4.0`); `r`, `g`, `b`/`1.0, 1.0, 1.0`/Color; `max_age`/`40`/Lifetime in ticks; `gravity`/`0.04`/Gravity strength
```lua
minecraft.particles.spawn({
  x = 100, y = 70, z = 200,
  r = 1, g = 0, b = 0,
  max_age = 60,
  vx = 0, vy = 0.1, vz = 0
})
```
## Raycast
### `minecraft.raycast(spec?)`
Performs a raycast from the player's camera or a custom origin. If called without arguments, uses the player's camera position and look direction. The `spec` table accepts:
``origin` / `origin_x/y/z``: Ray origin (default: camera position); ``direction``: Ray direction vector `{x, y, z}`, or `yaw`/`pitch` in degrees; ``max_distance` / `reach``: Maximum ray distance (default: player reach or 5.0); ``ignore_liquids``: Whether to ignore liquid blocks (default `false`); ``blocks``: Whether to test blocks (default `true`); ``entities``: Whether to test entities (default `true`)
Returns a table with the hit result, or `nil` if nothing was hit. Result fields:
``type``: `"block"`, `"entity"`, or `"model"`; ``hit_x`, `hit_y`, `hit_z``: Hit position; ``block_x`, `block_y`, `block_z``: (block hits) Block position; ``side``: (block hits) Face index; ``block_id``: (block hits) Block ID; ``block_name``: (block hits) Wire block name; ``entity_id`, `entity_type``: (entity hits) Entity identity; ``entity_x`, `entity_y`, `entity_z``: (entity hits) Hit entity position; ``model_id`, `model_tag``: (model hits) Placed model instance info
```lua
local hit = minecraft.raycast({
  origin = {x = 0, y = 64, z = 0},
  yaw = 0, pitch = 0,
  max_distance = 10,
  entities = false
})
if hit and hit.type == "block" then
  print(hit.block_x, hit.block_y, hit.block_z, hit.block_id)
end

-- Default player raycast
local look = minecraft.raycast()
```
## Tile Entities
### `minecraft.tile_entities.list(filter?)`
Returns an array of tile entity handles in the current world, optionally filtered by ID string. Each handle is a table with methods (see below).
```lua
for _, te in ipairs(minecraft.tile_entities.list()) do
  print(te.x, te.y, te.z, te:get_id())
end
```
### `minecraft.tile_entities.get(x, y, z)`
Returns the tile entity handle at the given position, or `nil`.
```lua
local te = minecraft.tile_entities.get(100, 64, 200)
```
### `minecraft.tile_entities.count(filter?)`
Returns the count of tile entities, optionally filtered by ID.
```lua
local n = minecraft.tile_entities.count("mymod:my_tile")
```
### Tile Entity Handle Methods
Each handle returned by `list()` or `get()` has these methods:
`:get_id()`/string/Tile entity registry ID; `:get_block_id()`/int/Block ID at this position; `:get_block_meta()`/int/Block metadata at this position; `:is_removed()`/bool/Whether the entity was removed; `:mark_dirty()`/—/Marks the entity as needing saving (server only); `:distance_from(x, y, z)`/number/Euclidean distance to point; `:get_world_time()`/number/Current world time in ticks; `:get_data()`/table/(mod tile entities) NBT data table; `:set_data(table)`/—/(mod tile entities, server only) Sets NBT data from a table; `:get_animation_frame()`/int/Current animation frame; `:set_animation_speed(speed)`/—/animation speed multiplier
```lua
local te = minecraft.tile_entities.get(100, 64, 200)
if te then
  print(te:get_id())
  print(te:get_block_id())
  local data = te:get_data()
  te:set_animation_speed(2.0)
end
```
## Inventory
### `minecraft.inventory.*`
Functions for reading and writing the player's inventory. Client-only.
``slot_count()``: Total number of slots (main + armor); ``main_size()``: Number of main inventory slots; ``get(slot)``: item stack table for slot, or `nil`; ``set(slot, stack)``: slot to a stack, returns `true`/`false`; ``cursor_get()``: the cursor (held) item stack; ``cursor_set(stack)``: the cursor (held) item stack; ``give(stack)``: Adds stack to inventory, returns `true`/`false`; ``offer(stack)``: Like give but returns the remainder stack (what couldn't fit)
Stack tables have fields: `id`, `count`, `damage`, `max_damage`, `damageable`, `stackable`, `has_subtypes`, `max_count`.
```lua
-- Check slot 0
local stack = minecraft.inventory.get(0)
if stack then
  print(stack.id, stack.count)

  -- Set it
  minecraft.inventory.set(0, {id = 1, count = 64, damage = 0})
end

-- Give items
minecraft.inventory.give({id = 1, count = 10})

-- Offer with overflow
local remainder = minecraft.inventory.offer({id = 1, count = 200})
print("overflow:", remainder.count)
```
## Sound
### `minecraft.sound.*`
``register(id, path, kind?)``: a sound. `kind` is `"effect"` (default), `"streaming"`, or `"music"`. Returns `true`/`false`; ``play(id, volume?, pitch?)``: Plays a registered sound globally; ``play_at(id, x, y, z, volume?, pitch?)``: Plays a sound at a world position; ``play_loop_at(id, x, y, z, volume?, pitch?)``: Plays a looping sound at a position, returns a handle string; ``stop(handle)``: Stops a looping sound by handle
```lua
minecraft.sound.register("mymod:boom", "resources/mymod/sounds/boom.ogg", "effect")
minecraft.sound.play("mymod:boom", 1.0, 1.0)
local handle = minecraft.sound.play_loop_at("mymod:hum", 100, 64, 200)
minecraft.sound.stop(handle)
```
## Screen
### `minecraft.screen.*`
Functions for opening and managing custom Lua screens.
``open(id, options?)``: a Lua screen. `options` may contain `{title = "...", pause = true}`; ``close()``: the current screen; ``open_host(screenId, fields?)``: a vanilla host screen by ID, with optional field values; ``host_field(name)``: the value of a host screen field; ``host_set_field(name, value)``: a host screen field; ``add_field(name, x, y, w, h, options?)``: Adds a text field to the current Lua screen (init phase only). `options`: `{text, max_len, numeric, signed, decimal}`; ``field_text(name)``: a field's text; ``set_field_text(name, text)``: a field's text; ``add_button(x, y, w, h, text, callback?)``: Adds a button (init phase only); ``set_fields_visible(visible)``: Shows/hides all text fields
```lua
minecraft.screen.open("mymod:config", {title = "My Config"})
```
### Screen ID Constants (`minecraft.screen.ids`)
Pre-defined screen ID constants: `login`, `title`, `game_menu`, `multiplayer`, `connect`, `disconnected`, `downloading_terrain`, `death`, `chat`, `sleeping_chat`, `confirm`, `create_world`, `select_world`, `edit_world`, `world_settings`, `world_save_conflict`, `inventory`, `crafting`, `dispenser`, `double_chest`, `furnace`, `sign_edit`, `options`, `video_options`, `detail_settings`, `keybinds`, `mods`, `achievements`, `stats`, `lan`, `lan_info`, `server_mod_download`, `fatal_error`, `out_of_memory`.
### Screen Region Constants (`minecraft.screen.regions`)
`footer` `screen` `side_panel`
### Screen Convenience Functions
`minecraft.screen.on_ui(screen_id, region, callback, priority?)` — shorthand for subscribing to `screen_ui` with a specific screen/region filter.
`minecraft.screen.on_lua_screen(screen_id, handlers, priority?)` — shorthand for subscribing to `screen_event` by screen ID, dispatching `{init, render, tick, key, mouse, scroll, close}` to handler functions.
`minecraft.screen.settings(spec)` — Creates a mod settings screen. See the prelude for full details; supports sliders, toggles, and auto-layout.
## GUI Drawing
### `minecraft.gui.*`
GUI drawing functions — only usable inside `screen_event` render phase or `screen_region` render phase contexts.
``fill_rect(x, y, w, h, argb)``: Draws a filled rectangle; ``draw_text(x, y, text, argb)``: Draws text; ``draw_centered_text({x, y, width/w, text, color?})` or `(x, y, width, text, color?)``: Draws centered text; ``draw_item(x, y, itemId, count, damage?)``: Draws an item icon; ``text_width(text)``: the pixel width of rendered text; ``texture_id(path)``: the OpenGL texture ID for a resource path; ``draw_sprite(path/id, x, y, u, v, w, h)``: Draws a sprite from a texture atlas; ``draw_texture(textureId, x, y, w, h)``: Draws a full texture quad; ``draw_button({x, y, width, height, text, active?, mouse_x?, mouse_y?})``: Draws a vanilla-style button; ``draw_slider({x, y, width, height, value, text, mouse_x?, mouse_y?})``: Draws a vanilla-style slider; ``draw_toggle({x, y, width, height, label, value, mouse_x?, mouse_y?})``: Draws a vanilla-style toggle
```lua
-- Draw a button
minecraft.gui.draw_button({
  x = 10, y = 10, width = 100, height = 20,
  text = "Click", mouse_x = event.mouse_x, mouse_y = event.mouse_y
})
```
## Camera (Framebuffer Targets)
### `minecraft.camera.*`
Controls offscreen framebuffer objects for rendering the world to textures (viewfinder / render-to-texture).
``create(width, height, colorCount?, useDepthTex?)``: a camera target, returns handle or `-1`; ``create_display_size(colorCount?, useDepthTex?)``: a camera target matching the display size; ``destroy(handle)``: Destroys a target, returns `true`/`false`; ``resize(handle, width, height)``: Resizes a target; ``width(handle)``: pixel width; ``height(handle)``: pixel height; ``render(handle, x, y, z, yaw, pitch, roll, fov, tickDelta?)``: Renders the world into the target from the given camera position. Returns `true`/`false`; ``unbind()``: Unbinds the active framebuffer (returns to default); ``texture(handle, attachmentIndex?)``: the OpenGL texture ID for a color attachment; ``rendering()``: the handle of the currently-rendering target, or `-1`
```lua
local cam = minecraft.camera.create(256, 256)
if cam > 0 then
  minecraft.camera.render(cam, 0, 80, 0, 0, -30, 0, 70)
  local texId = minecraft.camera.texture(cam)
  -- use texId with gui.draw_texture or model/procedural rendering
  minecraft.camera.destroy(cam)
end
```
## FBO (Offscreen Framebuffers)
### `minecraft.fbo.*`
General-purpose offscreen framebuffer objects for custom render passes and shader work.
``create(width, height, colorCount?, useDepthTex?)``: an FBO, returns handle or `-1`; ``create_display_size(colorCount?, useDepthTex?)``: an FBO matching display size; ``destroy(handle)``: Destroys an FBO; ``resize(handle, width, height)``: Resizes an FBO; ``bind(handle)``: Binds an FBO for rendering, returns `true`/`false`; ``unbind()``: Unbinds the active FBO; ``texture(handle, attachmentIndex?)``: the OpenGL texture ID; ``width(handle)``: pixel width; ``height(handle)``: pixel height; ``bound()``: the currently bound FBO handle, or `-1`
## Render (World-Space Drawing)
### `minecraft.render.*`
Low-level world-space drawing functions, only usable during world render events (`world_render`) or chunk context callbacks.
``quads({texture?, texture_id?, blend?, cull?, depth_test?, depth_write?, r?, g?, b?, a?, x?, y?, z?, yaw?, pitch?, roll?, scale?, world_space?, vertices = {{x, y, z, u?, v?, r?, g?, b?, a?}, ...}})``: Draws textured/colored quads in world space. Returns number of quads emitted; ``billboards({brightness?, rotation_x_rad?, rotation_y_rad?, blend?, depth_test?, depth_write?, billboards/points = {{x, y, z, size, alpha}, ...}})``: Draws billboard sprites. Returns count; ``set_item_entity_override(enabled)``: When enabled, overrides item entity rendering with mod models. Call with `false` to disable
```lua
minecraft.render.quads({
  texture = "/textures/blocks/stone.png",
  vertices = {
    {x = -0.5, y = 0, z = -0.5, u = 0, v = 0},
    {x =  0.5, y = 0, z = -0.5, u = 1, v = 0},
    {x =  0.5, y = 0, z =  0.5, u = 1, v = 1},
    {x = -0.5, y = 0, z =  0.5, u = 0, v = 1}
  }
})
```
### `minecraft.tessellator.*`
``quad({texture?, texture_id?, r?, g?, b?, a?, vertices = {{x, y, z, u, v}, ...}})``: Emits a single quad into the manual block model buffer. Returns `true`/`false`
## Texture Queries
### `minecraft.texture.*`
``size(path)``: `{width = N, height = N}` for a texture resource; ``pixel(path, x, y)``: `{a, r, g, b}` (0–255) for the pixel at (x, y)
```lua
local size = minecraft.texture.size("textures/blocks/stone.png")
print(size.width, size.height)
local px = minecraft.texture.pixel("textures/blocks/stone.png", 0, 0)
```
## Model API
### `minecraft.model.*`
Functions for loading, building, placing, and drawing baked 3D models.
``load(path)``: Loads a baked model file from the mod's assets. Returns a handle integer, or `nil, error`; ``build({quads = {...}, key?})``: Builds a model from quad data at runtime. Returns a handle. Quad format: `{texture?, r, g, b, a?, shade?, vertices = {v1, v2, v3, v4}}`. Each vertex: `{x, y, z, u, v}`. The `key` string enables caching (subsequent calls with the same key return the existing handle); ``voxels({cells, resolution, origin_x/y/z, scale, key})``: Voxel model builder. Each cell: `{x, y, z, r?, g?, b?, a?}`. Interior faces shared by adjacent cells are automatically culled. Returns a model handle; ``voxel({texture, atlas_index?, mod_texture?, grid?, alpha_cutoff?})``: Samples a sprite texture and extrudes it into a one-voxel-thick model. `grid` controls sampling resolution (default 16). `alpha_cutoff` (default 30) controls the transparency threshold. Results are cached; ``place(handle, opts)``: Places a model instance in the world (hitbox-enabled, raycastable). `opts`: `{x, y, z, yaw?, pitch?, roll?, scale?, pivot_y?, tag?}`. Returns an instance ID; ``update(instanceId, opts)``: Updates a placed model instance's transform; ``remove(instanceId)``: Removes a placed model instance; ``clear()``: Removes all model instances owned by the current mod; ``draw(handle, opts)``: Draws a baked model in world space during a world render event. `opts`: `{x, y, z, yaw?, pitch?, roll?, scale?, pivot_y?, brightness?, a?, blend?, cull?, depth_test?, depth_write?}`. Returns `true`/`false`; ``draw_item(itemId, damage, opts)``: Draws a real 3D model for an item (mod or vanilla block). Returns `false` for flat sprite items; ``item_bounds(itemId, damage)``: the bounding box `{min_x, min_y, min_z, max_x, max_y, max_z}` of an item's 3D model, or `nil` if the item has no 3D shape; ``bounds(handle)``: the bounding box of a baked model
```lua
-- Build a colored quad model
local handle = minecraft.model.build({
  quads = {{
    texture = "textures/blocks/stone.png",
    r = 1, g = 0, b = 0,
    vertices = {
      {x = -0.5, y = -0.5, z = 0, u = 0, v = 0},
      {x =  0.5, y = -0.5, z = 0, u = 1, v = 0},
      {x =  0.5, y =  0.5, z = 0, u = 1, v = 1},
      {x = -0.5, y =  0.5, z = 0, u = 0, v = 1}
    }
  }},
  key = "my_quad"
})

-- Place it in the world
local instance = minecraft.model.place(handle, {x = 100, y = 65, z = 200, scale = 1.5})
```
## File Dialogs (Client Only)
### `minecraft.files.*`
``pick(options?)``: a file picker dialog. `options` can be a string extension filter (e.g. `"json"`, `".png"`) or a table `{extension = "..."}`. Returns the selected file path or `nil`; ``read(path)``: an external file by absolute path. Also resolves mod-bundled resources. Returns content string or `nil, error`
```lua
local path = minecraft.files.pick("json")
if path then
  local data = minecraft.files.read(path)
end
```
## Procedural Texture Creation (Client Only)
### `minecraft.render.create_texture(spec)`
Creates a texture from pixel data. Accepts a table `{width, height, values/colors = {argb...}}` or positional `(width, height, colors)`. Returns `{id, width, height}`.
```lua
local tex = minecraft.render.create_texture({
  width = 16, height = 16,
  values = {0xFFFF0000, 0xFF00FF00, ...}
})
```
### `minecraft.render.release_texture(id)`
Releases a previously created texture.
### `minecraft.render.get_texture_pixels(pathOrId)`
Returns `{width, height, pixels = {argb...}}` for a texture path or mod texture ID.
## Seed Resolution
### `minecraft.util.resolve_seed(text)`
Resolves a textual seed to its numeric value (supports numeric strings and named seeds).
```lua
local seed = minecraft.util.resolve_seed("myworld")
```
## Registry Queries
### `minecraft.registry.name(domain, id)`
Returns the wire name for a registry entry. Supports `"biome"` and `"block"` domains.
```lua
local name = minecraft.registry.name("block", 1)  -- "stone"
local name = minecraft.registry.name("biome", 0)  -- "ocean" (or similar)
```
### `minecraft.registry.list(domain)`
Returns an array of all wire names in a registry. Supports `"biome"` and `"block"`.
```lua
for _, name in ipairs(minecraft.registry.list("block")) do
  print(name)
end
```
## World Grid Sampling
### `minecraft.world.sample_grid(seed, centerX, centerZ, options?)`
Samples terrain/biome data into a grid array for minimap or visualization use. The `options` table accepts:
`radius_chunks` / `radius`/`6`/Radius in chunks (clamped 1–4096); `max_side`/`48`/Maximum samples per side (clamped 8–256); `channel`/`"grass"`/Primary data channel; `channels`/`{channel}`/Array of channel names to sample (max 8); `mod_generation`/`false`/Enable mod generation hooks during sampling
Supported channels: `"height"`, `"surface_block"`, `"surface_block_below"`, `"biome_id"`, `"grass"` (grass color as ARGB).
Returns a table with `side`, `step`, `origin_x`, `origin_z`, `center_x`, `center_z`, `channel`, `values` (primary channel array), and per-channel fields.
## Event Reference
All supported event names for `minecraft.on()`:
`client_tick`/`before`, `after_world`, `paused`, `has_player`, `has_world`, `world_name`, `is_overworld`, `camera_y`, `player_y`, `player_fall_distance`, `player_on_ground`, `world_time`, `is_night`, `mod_generation`/—; `render_frame`/`tick_delta`/—; `fog_settings`/`enabled`, `spherical`, `exponential`, `start`, `end`, `density`, `custom_color`, `red/green/blue`/fog fields; `render_targets`/`tick_delta`/—; `first_person_hand`/`tick_delta`, `eye`, `canceled`, `entity_id`, `entity_type`/`canceled`; `key_press`/`key`, `pressed`, `repeat`, `handled`/`handled`; `mouse_button`/`button`, `pressed`, `handled`/`handled`; `raycast`/`has_hit`, `type`, `hit_x/y/z`, `block_x/y/z`, `side`, `block_id`, `block_name`, `item_id`, `entity_id`, `entity_type`/—; `fov`/`tick_delta`, `fov`/`fov`; `camera_setup`/`tick_delta`, `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `custom_view`, `hide_first_person_hand`/`x`, `y`, `z`, `yaw`, `pitch`, `roll`, `custom_view`, `hide_first_person_hand`; `player_travel`/`sideways`, `forward`, `speed_multiplier`, `has_player`, `is_local_player`/`sideways`, `forward`, `speed_multiplier`; `tick_rate`/`target_tps`, `tps_scale`/`target_tps`, `tps_scale`; `world_start`/`save_name`, `new_world`/—; `world_open`/`save_name`, `new_world`, `options` (table)/—; `world_tick`/`remote`, `before`/—; `entity_tick`/`remote`, `canceled`, `entity_id`, `entity_type`, `x`, `y`, `z`, `yaw`, `pitch`/`canceled`; `tile_entity_tick`/`x`, `y`, `z`, `id`, `remote`, `removed`, `canceled`, `world_time`, `animation_frame`, `animation_tick`, `animation_speed`, `entity`/`canceled`, `animation_speed`; `create_world`/`save_name`, `seed`, `canceled`, `options` (table)/`canceled`, `options`; `block_interact`/`x`, `y`, `z`, `block_id`, `side`, `right_click`, `remote`, `canceled`, `handled`, `has_player`, `local_player`, `has_item`, `player_x/y/z`, `player_yaw/pitch`, `item_id/count/damage/max_damage/damageable`/`canceled`, `handled`, `item_count`, `item_damage`; `entity_interact`/`attack`, `remote`, `canceled`, `handled`, `sneaking`, `has_player`, `local_player`, `has_target`, `player_yaw/pitch`, `has_item`, `item_id/count/damage`, `entity_id`, `entity_type`, `target_id`/`canceled`, `handled`, `item_count`; `attack_damage`/`damage`, `critical`, `canceled`, `fall_distance`, `on_ground`, `target_x/y/z`, `has_player`, `has_target`/`damage`, `critical`, `canceled`; `entity_teleport`/`entity_id`, `entity_type`, `from_x/y/z`, `x`, `y`, `z`, `yaw`, `pitch`, `canceled`, `has_entity`, `has_player`/`x`, `y`, `z`, `yaw`, `pitch`, `canceled`; `world_color`/`partial_ticks`, `r`, `g`, `b`, `kind`, `celestial`, `world_time`, `is_night`/`r`, `g`, `b`; `entity_render`/`entity_id`, `entity_type`, `is_player`, `tick_delta`, `pose` (sub-table with `body_yaw`, `head_yaw/pitch`, `yaw`, `pitch`, `roll`, `scale`, `offset_x/y/z`, `parts`)/`pose` (full mutation); `world_render`/`tick_delta`, `stage`, `moment`, `cancel_vanilla`, `vanilla_stage_ran`, `shadow_pass`, `celestial_angle`, `sky_yaw_deg`, `star_brightness`, `rain_strength`, `stars_enabled`, `astronomy_enabled`, `astronomy_utc_millis`, `observer_lat/lon_deg`, `camera_x/y/z`, `camera_yaw/pitch/roll`, `custom_camera`, `world_time`, `celestial`, `is_night`, `cloud_base_height`/`cancel_vanilla`, celestial/sky/astronomy fields (sky stage only); `chunk_generation`/`stage`, `moment`, `cancel_vanilla`, `vanilla_stage_ran`, `world_seed`, `mod_generation`, `is_overworld`, `chunk_x`, `chunk_z`, `has_chunk`/`cancel_vanilla`; `screen_region`/`phase_name`, `screen_id`, `region`, `mouse_x`, `mouse_y`, `button`, `scroll_delta`, `x`, `y`, `width`, `height`, `handled`/`handled`, `width`, `height`; `screen_ui`/`screen_id`, `region`, `host_fields` (table), `ui` (table with `add_centered_button`, `add_button`, `add_stacked_centered_button`)/—; `screen_event`/`screen_id`, `phase`, `width`, `height`, `mouse_x`, `mouse_y`, `tick_delta`, `key`, `char`, `button`, `released`, `delta`, `handled`/`handled`; `world_spawn_search`/`x`, `y`, `z`, `resolved`/`x`, `y`, `z`, `resolved`; `pre_entity_render`/`entity_id`, `entity_type`, `tick_delta`, `canceled`, item fields/`canceled`; `pre_tile_entity_render`/`x`, `y`, `z`, `id`, `tick_delta`, `canceled`/`canceled`; `entity_spawn`/`entity_id`, `entity_type`, item fields/—; `entity_remove`/`entity_id`, `entity_type`, item fields/—
Events that carry execution context have `remote` (boolean) and `side` (`"client"` or `"server"`) set automatically. World-backed events commonly also expose `has_world`, `world_name`, `is_overworld`, and `mod_generation`; fields remain event-specific, so consult the table above.
### Lifecycle Phase Constants
`minecraft.lifecycle` is a convenience table with all phase names as keys:
```lua
-- Equivalent strings:
minecraft.lifecycle.init       -- "init"
minecraft.lifecycle.post_init  -- "post_init"
minecraft.lifecycle.ready      -- "ready"
```
### Generation Stage Constants
```lua
minecraft.generation.stages   -- { terrain = true, surface = true, carver = true, features = true }
minecraft.generation.moments  -- { before = true, after = true }
```
### Render Stage Constants
```lua
minecraft.render.stages  -- { sky, stars, terrain_opaque, entities, particles_lit, particles,
                         --   terrain_translucent, weather, clouds, hand, framebuffer }
minecraft.render.moments -- { before = true, after = true }
```
### World Color Kind Constants
```lua
minecraft.colors  -- { sky = true, fog = true }
```

# Registration
Registration functions create new blocks, items, and recipes. They must generally be called during mod initialization (before the game finishes bootstrapping). Content is registered via lifecycle phases (`at_phase`), which execute in order: `init` → `post_init` → `ready`. Register blocks and items in `init`, recipes and cross-references in `post_init`.
## Block Registration
### `minecraft.register_block(spec)`
Registers a new block. Returns `true` on success, or throws an assertion error on failure. The `spec` table accepts the following fields:
`id`/int/(required)/Numeric block ID (1–`BLOCK_COUNT - 1`). Must be unique and not already reserved; `texture`/string/(required unless `texture_id` given)/Texture path for the default face texture (e.g. `"textures/blocks/my_block.png"`); `texture_id`/int/`-1`/Vanilla terrain-atlas texture index override (0–255). Alternative to `texture`; `hardness`/float/`1.0`/Block hardness (mining time); `resistance`/float/`1.0`/Explosion resistance; `luminance`/float/`0.0`/Light emission level (clamped 0.0–1.0); `translation_key`/string/`"block<id>"`/i18n key token (without `"tile."` or `".name"` suffix); `name`/string/auto-generated/Display name (added to I18n table). If empty, derived from the translation key; `material`/string/`"stone"`/Material type. One of: `"stone"`, `"metal"`, `"wood"`, `"glass"`; `opaque`/bool/`true`/Whether the block is fully opaque (affects light and rendering); `full_cube`/bool/`true`/Whether the block occupies a full 1×1×1 cube for culling; `translucent`/bool/`not opaque`/Render layer. `false` = solid/cutout pass (block casts shader-mod shadows); `true` = alpha-blended pass (drawn after entities, casts no shader shadows). Solid-looking blocks that only set `opaque = false` for lighting should set `translucent = false`; `collision_height`/float/`1.0`/Custom collision height. Values > 1 create taller collision boxes; `stack_on_same`/bool/`false`/If `true`, the block can be placed on another block of the same type. If `false` (and default), placing on the same type is prevented; `requires_solid_below`/bool/`true`/Whether the block requires a solid block below to be placed; `coordinate_bounds`/bool/`false`/Randomizes the render/collision AABB per world position using coordinate-based hash. Produces natural-looking variation; `coordinate_color`/bool/`false`/When `true`, colors the block by world position using coordinate-based hashing; `bounds_padding`/float/`0.0625`/Padding from full cube when `coordinate_bounds` is active. Clamped 0.0–0.49; `bounds_offset`/float/`0.1`/Maximum random offset magnitude for coordinate-based bounds; `min_scale`/float/`0.9`/Minimum scale factor for coordinate-based bounds; `max_scale`/float/`1.1`/Maximum scale factor for coordinate-based bounds; `model`/int or function/—/Baked model handle (integer from `minecraft.model.load`/`minecraft.model.build`) or a model callback function. When set, overrides the default cube renderer with custom baked-model rendering; `item`/table/—/Sub-table for the block's corresponding item: `{texture = "...", texture_id = int}`. If omitted, the block item uses the block's terrain texture; `tile_entity`/string/—/Tile entity registry ID string. The full registry ID becomes `"<ownerModId>:<tile_entity>"`. Registers a block entity factory; `on_use`/function/—/Right-click handler function. Automatically wires a `block_interact` event listener filtered to this block ID with `right_click = true`; `behavior_priority`/int/`0`/Priority for the auto-wired interact handler when `on_use` is set
```lua
minecraft.register_block({
  id = 1000,
  texture = "textures/blocks/my_block.png",
  hardness = 2.0,
  resistance = 10.0,
  luminance = 0.5,
  name = "My Block",
  material = "stone",
  opaque = true,
  full_cube = true,
  on_use = function(event)
    print("Right-clicked my block at", event.x, event.y, event.z)
    event.handled = true
    return event
  end
})

-- With coordinate variation
minecraft.register_block({
  id = 1001,
  texture = "textures/blocks/pebbles.png",
  coordinate_bounds = true,
  min_scale = 0.3,
  max_scale = 0.6,
  bounds_offset = 0.2,
  name = "Pebbles"
})

-- With custom model
local modelHandle = minecraft.model.build({
  quads = {{...}},
  key = "custom_block"
})
minecraft.register_block({
  id = 1002,
  texture = "textures/blocks/custom.png",
  model = modelHandle,
  item = {texture = "textures/items/custom_item.png"},
  name = "Custom Model Block"
})

-- With tile entity
minecraft.register_block({
  id = 1003,
  texture = "textures/blocks/container.png",
  tile_entity = "my_container",
  name = "Container Block"
})
```
### Validation Rules
`id` must be between 1 and `Block::BLOCK_COUNT - 1` (typically 1–255). Either `texture` or `texture_id` (0–255) is required. `texture_id` must be 0–255 (vanilla terrain atlas index). Duplicate IDs are rejected. The block must be registered before the game finishes bootstrapping (`!Registry::isBootstrapped()`).
## Item Registration
### `minecraft.register_item(spec)`
Registers a new item. Returns `true` on success, or `false, error` on failure. The `spec` table accepts:
`id`/int/(required)/Numeric item ID (absolute, 256–`ITEM_COUNT - 1`). Must be unique; `texture`/string/(required unless `texture_id` given)/Texture path for the item sprite; `texture_id`/int/`-1`/Vanilla items-atlas texture index (0–255). Alternative to `texture`; `max_count`/int/`64`/Maximum stack size; `max_damage`/int/`0`/Maximum damage (0 = not damageable); `translation_key`/string/`"item<id>"`/i18n key token (without `"item."` or `".name"` suffix); `name`/string/auto-generated/Display name; `model`/int or function/—/Baked model handle or model callback. When set, the item renders as a 3D model. Requires `texture` to be set
The `ownerModId` field is set automatically from the mod context.
```lua
minecraft.register_item({
  id = 256,
  texture = "textures/items/my_item.png",
  max_count = 16,
  max_damage = 250,
  name = "My Tool"
})

-- With custom 3D model
local handle = minecraft.model.load("models/my_item.json")
minecraft.register_item({
  id = 257,
  texture = "textures/items/my_item.png",
  model = handle,
  name = "3D Item"
})
```
### Validation Rules
`id` must be ≥ 256 and < `ITEM_COUNT` (raw ID = `itemId - 256`). Either `texture` or `texture_id` (0–255) is required. If a `model` is provided, `texture` is also required. `texture_id` must be 0–255. Duplicate IDs are rejected. Must be registered before game bootstrapping completes.
### Block Items
When a block is registered via `register_block`, a corresponding item is automatically created (the block item). You can customize the block item's texture via the `item` sub-table in the block spec:
```lua
minecraft.register_block({
  id = 1000,
  texture = "textures/blocks/my_block.png",
  item = {
    texture = "textures/items/my_block_item.png",
    -- or: texture_id = 42
  }
})
```
## Recipe Registration
### `minecraft.register_shaped_recipe(spec)`
Registers a shaped (pattern-based) crafting recipe. The spec table accepts:
`output_block_id`/int/`0`/Output block ID (alternative to `output_item_id`); `output_item_id`/int/`0`/Output item ID (alternative to `output_block_id`); `output_count`/int/`1`/Output stack size (1–64); `pattern`/array of strings/(required)/Crafting pattern rows (1–3 rows, each 1–3 characters, all rows same width); `key`/string/`"#"`/The character in the pattern representing the ingredient. Uses only the first character; `item_id`/int/(required)/The ingredient item/block ID
Exactly one of `output_block_id` or `output_item_id` must be set.
```lua
minecraft.register_shaped_recipe({
  output_item_id = 256,
  output_count = 4,
  pattern = {"#", "#"},
  key = "#",
  item_id = 1  -- stone
})

-- 3×3 recipe
minecraft.register_shaped_recipe({
  output_block_id = 1000,
  output_count = 1,
  pattern = {"###", "# #", "###"},
  key = "#",
  item_id = 257
})
```
### `minecraft.register_shapeless_recipe(spec)` — *Not yet implemented*
Shapeless recipe registration is reserved for future use.
### `minecraft.register_furnace_recipe(spec)` — *Not yet implemented*
Furnace/smelting recipe registration is reserved for future use.
### `minecraft.recipes.remove(recipe_id)`
Removes a previously registered recipe by ID. *Implementation pending.*
### `minecraft.recipes.remove_all()`
Removes all recipes. *Implementation pending.*
## Mod Settings & Keybinds Registry
Register simple sliders, toggles, and keybinds here. The engine persists them and keeps them on the main **Mod Settings** page, opened from the permanent "Mod Settings..." button in Video Settings (`minecraft.screen.ids.video_options`).
For a richer page, use the [Settings DSL](07-gui-and-screens.md) (`minecraft.screen.settings`) with `parent_screen = minecraft.screen.ids.mod_settings`. Buttons injected into the `minecraft:mod_settings` footer are collected automatically on its scrollable **Mod Pages** page; no custom navigation glue is needed.
### `minecraft.settings.register(display_name, entries)`
`display_name` is the label shown for your mod's section in the shared screen (defaults to your mod id if omitted). `entries` is an array of tables:
`key`/string/all/Setting key, unique within your mod (required); `label`/string/all/Display label (defaults to `key`); `kind`/string/all/`"slider"` (default) or `"toggle"`; `default`/number/boolean/all/Initial value (number for slider, boolean for toggle); `min`, `max`/number/slider/Range (defaults `0`..`1`); `step`/number/slider/Increment per click (default: range / 20); `integer`/boolean/slider/Snap to whole numbers; `decimals`/number/slider/Display precision (default `2`)
```lua
minecraft.settings.register("My Mod", {
  { key = "particle_density", label = "Particle Density", kind = "slider", min = 0, max = 2, default = 1 },
  { key = "screen_shake", label = "Screen Shake", kind = "toggle", default = true },
})
```
### `minecraft.settings.get(key)`
Returns the current value for one of your own registered settings (number for `slider`, boolean for `toggle`), or `nil` if `key` isn't registered.
### `minecraft.keybinds.register(name, spec)`
Registers a rebindable keybind, stored and persisted under the id `"<your_mod_id>.<name>"`. `spec` is a table: `{ default = keycode, label = "Display Name" }`. The bind shows up in the shared Mod Settings screen for the player to rebind; changes are saved automatically.
### `minecraft.keybinds.get_code(id)`
Returns the current key code for a keybind, or `0` if unbound/not found. `id` must be the fully-qualified id, i.e. `"<your_mod_id>.<name>"` — the same id your mod registered with. Compare this against `event.key` inside a `key_press` subscription to react to the bind:
```lua
minecraft.keybinds.register("boost", { default = minecraft.key_code("b"), label = "Activate Boost" })

minecraft.on(minecraft.events.key_press, {}, function(event)
  if event.pressed and event.key == minecraft.keybinds.get_code("my_mod.boost") then
    -- activate boost
  end
end)
```
## Custom Block/Item Models
Models control the visual appearance of blocks and items in the world and inventory. The engine supports loading pre-baked models, building them from Lua quad data, or generating voxel-based geometry.
### `minecraft.model.load(path)`
Loads and bakes a JSON model file from the mod's assets (including its parent chain). The `path` is relative to the mod's asset root; `.json` is the supported model format and is appended when omitted. Returns a numeric handle on success, or `nil, error` on failure.
```lua
local handle = minecraft.model.load("models/my_model.json")
if handle then
  minecraft.register_block({
    id = 1000,
    texture = "textures/blocks/my_block.png",
    model = handle,
    name = "Model Block"
  })
end
```
### `minecraft.model.build(spec)`
Builds a baked model at runtime from quad data. The `spec` table accepts:
`quads`/array/Array of quad specification tables (see below). At least one quad required; `key`/string/Optional cache key. If provided, calling `model.build` again with the same key returns the existing handle
Each quad in `quads` is a table with:
`texture`/string/—/Texture path for this quad (batched by texture for rendering); `r`, `g`, `b`/float/`1.0`/Vertex color (clamped 0–1); `a`/float/`1.0`/Alpha (clamped 0–1); `shade`/float/`1.0`/Diffuse shading multiplier (clamped 0–1); `vertices`/array of 4 tables/(required)/Four vertex specification tables (see below)
Each vertex in `vertices`:
`x`, `y`, `z`/float/Vertex position in model-local space; `u`, `v`/float/Texture coordinates (0–1 range)
```lua
local handle = minecraft.model.build({
  quads = {{
    texture = "textures/blocks/stone.png",
    r = 1.0, g = 1.0, b = 1.0, a = 1.0,
    shade = 1.0,
    vertices = {
      {x = -0.5, y = -0.5, z = 0, u = 0, v = 0},
      {x =  0.5, y = -0.5, z = 0, u = 1, v = 0},
      {x =  0.5, y =  0.5, z = 0, u = 1, v = 1},
      {x = -0.5, y =  0.5, z = 0, u = 0, v = 1}
    }
  }},
  key = "my_generated_model"
})
```
### `minecraft.model.voxels(spec)`
Builds a model from a grid of voxel cells (integer lattice). Interior faces shared between adjacent cells are automatically culled.
`cells`/array/(required)/Array of cell specifications (see below); `resolution`/int/`16`/Grid resolution (voxel grid divisions per unit); `origin_x`, `origin_y`, `origin_z`/float/`0`/Origin offset for the model; `scale`/float/`1/resolution`/Size of each voxel in model units; `key`/string/—/Optional cache key for model reuse
Each cell in `cells`:
`x`, `y`, `z`/int/(required)/Lattice coordinates in the voxel grid; `r`, `g`, `b`/float/`1.0`/Per-cell color; `a`/float/`1.0`/Per-cell alpha
Returns a model handle, or `nil, error` if no cells result in visible faces.
```lua
local handle = minecraft.model.voxels({
  cells = {
    {x = 0, y = 0, z = 0, r = 1, g = 0, b = 0},
    {x = 1, y = 0, z = 0, r = 0, g = 1, b = 0},
    {x = 0, y = 1, z = 0, r = 0, g = 0, b = 1}
  },
  resolution = 8,
  key = "my_voxel_model"
})
```
### `minecraft.model.voxel(spec)`
Samples a sprite texture and extrudes its non-transparent pixels into a flat one-voxel-thick model, centered at `z = 0.5`. Results are cached.
`texture`/string/(required)/Path to the texture to sample; `atlas_index`/int/`-1`/If ≥ 0 and `mod_texture` is false, reads from the vanilla terrain atlas at this index; `mod_texture`/bool/`false`/If true, treats the texture as a mod texture (full image, not atlas); `grid`/int/`16`/Sampling grid size (e.g. 16 = 16×16 cells); `alpha_cutoff`/int/`30`/Alpha threshold (0–255). Pixels with alpha > this value become voxels
Returns a model handle, or `nil, error` if the texture is not found or has no opaque pixels.
```lua
local handle = minecraft.model.voxel({
  texture = "textures/blocks/stone.png",
  grid = 16,
  alpha_cutoff = 30
})

-- Use the handle in a block registration
minecraft.register_block({
  id = 1001,
  texture = "textures/blocks/stone.png",
  model = handle,
  name = "Voxel Block"
})
```
### Model Callbacks (for `model` field)
Instead of a static model handle, the `model` field in `register_block` and `register_item` can be a function. The function is called during rendering with an event table and a `tessellator` object already attached. It should issue draw calls; it does not return a model handle.
```lua
minecraft.register_block({
  id = 1002,
  texture = "textures/blocks/anim.png",
  model = function(event)
    -- event.type is "world" or "inventory"; event includes
    -- x/y/z, brightness, block_id, texture, and texture_id.
    minecraft.tessellator.quad({
      texture = event.texture,
      vertices = {
        {x=0, y=0, z=0, u=0, v=0}, {x=1, y=0, z=0, u=1, v=0},
        {x=1, y=1, z=0, u=1, v=1}, {x=0, y=1, z=0, u=0, v=1},
      },
    })
  end,
  name = "Dynamic Model Block"
})
```
## Complete Registration Example
```lua
minecraft.at_phase("init", 100, function()
  minecraft.register_block({
    id = 1000,
    texture = "textures/blocks/my_block.png",
    hardness = 3.0,
    resistance = 15.0,
    name = "Ruby Block",
    material = "stone",
    on_use = function(event)
      print("Ruby block used at", event.x, event.y, event.z)
      event.handled = true
      event.canceled = true
      return event
    end
  })
end)

minecraft.at_phase("init", 200, function()
  minecraft.register_item({
    id = 256,
    texture = "textures/items/ruby.png",
    max_count = 64,
    name = "Ruby"
  })
end)

minecraft.at_phase("post_init", 100, function()
  minecraft.register_shaped_recipe({
    output_block_id = 1000,
    output_count = 1,
    pattern = {"###", "###", "###"},
    key = "#",
    item_id = 256
  })
end)
```

# World and generation
## `minecraft.world.*`
### `minecraft.world.block_id(name)`
Look up the numeric block ID by name. Supports both vanilla and mod-added blocks.
`name`/string/Block identifier (e.g. `"stone"`, `"grass_block"`, `"mod:custom_block"`)
**Returns:** integer — numeric block ID, or `0` if not found.
```lua
local stone = minecraft.world.block_id("stone")       -- 1
local grass = minecraft.world.block_id("grass_block")  -- 2
local custom = minecraft.world.block_id("mymod:foo")   -- 0 if not registered
local unknown = minecraft.world.block_id("nonexistent") -- 0
```
### `minecraft.world.get_block(x, y, z)`
Get the numeric block ID at the given world position in the active world.
`x`/int/World X coordinate; `y`/int/World Y coordinate; `z`/int/World Z coordinate
**Returns:** integer — block ID at the position, or `0` if out of bounds or no world.
```lua
local block = minecraft.world.get_block(100, 64, 200)
```
### `minecraft.world.random(bound?)`
Generate a world-scoped random integer. Uses the active world's random number generator for deterministic/reproducible sequences tied to the world seed.
`bound`/int (optional)/`1000`/Upper bound (exclusive). Values `<= 0` return `0`
**Returns:** integer — random value in `[0, bound)`.
```lua
local r1 = minecraft.world.random()       -- random in [0, 1000)
local r2 = minecraft.world.random(10)     -- random in [0, 10)
```
### `minecraft.world.is_night()`
Check whether the active world is currently in night time.
**Returns:** boolean — `true` if world time is between 13000 and 23000 ticks (inclusive-exclusive), `false` otherwise.
```lua
if minecraft.world.is_night() then
  -- spawn mobs
end
```
### `minecraft.world.get_top_y(x, z)`
Get the Y coordinate immediately above the highest solid or fluid block at the given column.
`x`/int/World X coordinate; `z`/int/World Z coordinate
**Returns:** integer — top block Y + 1, or `-1` if no active world.
```lua
local top = minecraft.world.get_top_y(100, 200)
```
### `minecraft.world.player()`
Get the position of the active player.
**Returns:** table `{x, y, z}` containing the player's world coordinates, or `nil` if no player is available.
```lua
local pos = minecraft.world.player()
if pos then
  print("Player at:", pos.x, pos.y, pos.z)
end
```
### `minecraft.world.spawn_entity(entity_type, {x,y,z} or x,y,z)`
Spawn an entity of the given type at the specified position. Works server-side only (returns `false` on the client).
`entity_type`/string/Entity type identifier (e.g. `"Zombie"`, `"Creeper"`, `"Item"`); position/table or vararg/Either `{x, y, z}` table or individual `x, y, z` arguments. Default Y is `64`
**Returns:** boolean — `true` if the entity was spawned successfully.
```lua
-- Using a table
minecraft.world.spawn_entity("Zombie", {x=100, y=64, z=200})

-- Using individual arguments
minecraft.world.spawn_entity("Creeper", 100, 64, 200)
```
### `minecraft.world.count_entities(entity_type)`
Count the number of entities of a given type in the active world.
`entity_type`/string/Entity type identifier
**Returns:** integer — entity count.
```lua
local zombieCount = minecraft.world.count_entities("Zombie")
```
### `minecraft.world.set_time(tick)`
Set the world time.
`tick`/int/Time in ticks (0–24000). Must be a number
**Returns:** boolean — `true` if the time was set successfully (server-side only; `false` on client).
```lua
minecraft.world.set_time(0)    -- dawn
minecraft.world.set_time(6000) -- noon
minecraft.world.set_time(18000) -- midnight
```
### `minecraft.world.marker_px(grid, world_x, world_z)**
Convert world coordinates to grid pixel coordinates (for minimaps, chunk markers, etc.). This is a Lua helper defined in the runtime prelude.
`grid`/table/Grid descriptor with fields: `side` (pixel dimension), `step` (blocks per pixel), `origin_x`, `origin_z` (world origin offset); `world_x`/number/World X coordinate; `world_z`/number/World Z coordinate
**Returns:** `col, row` — clamped pixel coordinates in `[0, side-1]`, or `0, 0` if grid is invalid.
```lua
-- Given a grid from world generation (sample_grid or similar)
local col, row = minecraft.world.marker_px(grid, entity_x, entity_z)
```
## `ChunkHandle`
During chunk generation events, the local chunk being generated is exposed via the `event.chunk` object. The chunk handle provides the following methods (all of which require colon notation):
### `chunk:set_block(localX, y, localZ, blockId)`
Set a block in the chunk. Coordinates are local to the chunk (0–15 for X and Z, 0–255 for Y).
`localX`/int/Chunk-local X coordinate (0–15); `y`/int/World Y coordinate (0–255); `localZ`/int/Chunk-local Z coordinate (0–15); `blockId`/int/Numeric block ID to place
**Returns:** boolean — `true` if the block was placed successfully.
```lua
event.chunk:set_block(7, 64, 7, minecraft.world.block_id("stone"))
```
### `chunk:fill(x1, y1, z1, x2, y2, z2, blockId)`
Fill a rectangular region in the chunk with the given block. Coordinates are automatically clamped to chunk bounds.
`x1`/int/First corner X (clamped 0–15); `y1`/int/First corner Y (clamped 0–255); `z1`/int/First corner Z (clamped 0–15); `x2`/int/Opposite corner X (clamped 0–15); `y2`/int/Opposite corner Y (clamped 0–255); `z2`/int/Opposite corner Z (clamped 0–15); `blockId`/int/Numeric block ID to fill with
**Returns:** integer — count of blocks successfully changed.
```lua
-- Fill a 5x5x5 area with stone
local count = event.chunk:fill(5, 60, 5, 10, 64, 10, minecraft.world.block_id("stone"))
```
### `chunk:get_block(localX, y, localZ)`
Get the block ID at a chunk-local position in the chunk.
`localX`/int/Chunk-local X (0–15); `y`/int/World Y (0–255); `localZ`/int/Chunk-local Z (0–15)
**Returns:** integer — block ID, or `0` if out of bounds.
```lua
local block = event.chunk:get_block(7, 64, 7)
```
### `chunk:get_height(localX, localZ)`
Get the height (top solid block Y + 1) at the given column in the chunk.
`localX`/int/Chunk-local X (0–15); `localZ`/int/Chunk-local Z (0–15)
**Returns:** integer — height value, or `0` if out of bounds.
```lua
local h = event.chunk:get_height(7, 7)
```
## `minecraft.entities.*`
### `minecraft.entities.list(filter?)`
List all entities in the world with their full state.
`filter`/string (optional)/If provided, only returns entities whose type or registry_id matches this string
**Returns:** array of entity state tables, each containing:
`id`/int/Entity network ID; `type`/string/Entity type (e.g. `"Zombie"`, `"Item"`); `registry_id`/string/*(mod entities only)* Registry identifier (e.g. `"mymod:custom"`); `data`/table/*(mod entities only)* Custom NBT data as a Lua table; `x`, `y`, `z`/double/Position; `vx`, `vy`, `vz`/double/Velocity; `yaw`/float/Yaw rotation; `pitch`/float/Pitch rotation; `on_ground`/boolean/Whether the entity is on the ground; `item_id`/int/*(Item entities only)* Item ID; `item_count`/int/*(Item entities only)* Stack count; `item_damage`/int/*(Item entities only)* Item damage; `item_max_damage`/int/*(Item entities only)* Maximum item damage; `texture_path`/string/*(Item entities only)* Texture path for the item; `mod_texture`/boolean/*(Item entities only)* Whether the texture is a mod texture; `atlas_index`/int/*(Item entities only)* Atlas texture index (-1 for mod textures)
```lua
local entities = minecraft.entities.list()
for _, e in ipairs(entities) do
  print(e.id, e.type, e.x, e.y, e.z)
end

-- Filter by type
local zombies = minecraft.entities.list("Zombie")
```
### `minecraft.entities.apply_state(entity, state)`
Apply position, velocity, rotation, and/or custom data to one LuaModEntity. Only affects entities spawned via `minecraft.entities.spawn_mod`.
`entity`/table/Entity handle table containing `id`; `state`/table/Optional fields: `x`, `y`, `z`, `vx`, `vy`, `vz`, `yaw`, `pitch`, `data`
**Returns:** boolean — `true` on success.
```lua
local entity = minecraft.entities.get(42)
if entity then
  minecraft.entities.apply_state(entity, { x = 100, y = 64, z = 200, yaw = 90 })
end
```
### `minecraft.entities.teleport(id, {x,y,z,yaw,pitch} or x,y,z, yaw?, pitch?)`
Teleport **any** entity (not just mod-spawned) to a new position/rotation.
`entity`/table/Entity handle table containing `id`; position/table or vararg/Either `{x, y, z, yaw?, pitch?}` table or individual `x, y, z, yaw?, pitch?` arguments
**Returns:** boolean — `true` if the entity was found and teleported.
```lua
-- Using a table
minecraft.entities.teleport({id=42}, {x=100, y=64, z=200, yaw=90, pitch=0})

-- Using individual arguments
minecraft.entities.teleport({id=42}, 100, 64, 200, 90, 0)

-- Position only (no rotation change)
minecraft.entities.teleport({id=42}, 100, 64, 200)
```
### `minecraft.entities.remove(entity)`
Remove a LuaModEntity from the world.
`entity`/table/Entity handle table containing the entity ID
**Returns:** boolean — `true` if the entity was found and marked for removal. Only works on entities spawned via `spawn_mod`.
```lua
minecraft.entities.remove({id=42})
```
### `minecraft.entities.spawn_mod(registryId, {x, y, z, yaw?, pitch?, data?})`
Spawn a custom mod entity. The `registryId` must be in `"modid:name"` format and must match the current mod's ID.
`registryId`/string/Registry identifier in `"modid:name"` format; spec/table/Table with `x`, `y`, `z` (doubles), optional `yaw`, `pitch` (floats), optional `data` (table)
**Returns:** int — entity ID of the spawned entity, or `nil` if spawning failed.
```lua
local id = minecraft.entities.spawn_mod("mymod:custom_entity", {
  x = 100, y = 64, z = 200,
  yaw = 45, pitch = 10,
  data = { health = 100, color = "red" }
})
if id then
  print("Spawned entity with ID:", id)
end
```
### `minecraft.entities.register_global_pose_hook(entityType, callback)`
Override the render pose for **all** entities of a given type. Affects every entity of that type, including vanilla ones. Requires mod context.
`entityType`/string/Entity type string (e.g. `"Zombie"`); `callback`/function/Function receiving event table with `entity_id`, `entity_type`, `tick_delta`, `pose` — modify `pose` to override
**Returns:** boolean — `true` if registered.
```lua
minecraft.entities.register_global_pose_hook("Zombie", function(event)
  event.pose.head_yaw = event.pose.head_yaw + 180
  event.pose.parts.left_arm = { yaw = 90, pitch = 0, roll = 0 }
end)
```
### `minecraft.entities.register_local_pose_hook(entityId, callback)`
Override the render pose for a **specific** entity by ID.
`entityId`/int/Entity network ID; `callback`/function/Same callback pattern as global pose hook
**Returns:** boolean — `true` if registered.
```lua
minecraft.entities.register_local_pose_hook(42, function(event)
  event.pose.scale = 2.0
end)
```
### `minecraft.entities.unregister_local_pose_hook(entityId)`
Remove a previously registered local pose hook.
`entityId`/int/Entity network ID whose local hook to remove
**Returns:** boolean — `true` if a hook was removed.
```lua
minecraft.entities.unregister_local_pose_hook(42)
```
### EntityRenderPose fields
The `pose` table in pose hook callbacks has the following fields. All are read-write.
`body_yaw`/float/current/Body yaw rotation; `head_yaw`/float/current/Head yaw rotation; `head_pitch`/float/current/Head pitch rotation; `limb_swing`/float/current/Limb swing animation; `limb_distance`/float/current/Limb distance; `yaw`/float/current/Entity yaw; `pitch`/float/current/Entity pitch; `roll`/float/current/Entity roll; `scale`/float/current/Render scale (1.0 = normal); `offset_x`/float/current/X render offset; `offset_y`/float/current/Y render offset; `offset_z`/float/current/Z render offset; `parts`/table/current/Table mapping part name to `{yaw, pitch, roll}`
The event table also provides:
`entity_id`/int/The entity being rendered; `entity_type`/string/Entity type string; `tick_delta`/float/Partial tick for interpolation
```lua
-- Example: flip a creeper upside down
minecraft.entities.register_global_pose_hook("Creeper", function(event)
  event.pose.roll = 180
  event.pose.offset_y = 2.0
end)
```
## `minecraft.tile_entities.*`
### `minecraft.tile_entities.list(filter?)`
List all tile entities (block entities) in the world.
`filter`/string (optional)/If provided, only returns tile entities whose type ID matches
**Returns:** array of tile entity handle tables (see handle methods below).
```lua
local tes = minecraft.tile_entities.list()
for _, te in ipairs(tes) do
  print(te:get_id(), te.x, te.y, te.z)
end

-- Filter by type
local chests = minecraft.tile_entities.list("Chest")
```
### `minecraft.tile_entities.get(x, y, z)`
Get the tile entity handle at a specific world position.
`x`/int/World X coordinate; `y`/int/World Y coordinate; `z`/int/World Z coordinate
**Returns:** tile entity handle table, or `nil` if no tile entity at that position.
```lua
local te = minecraft.tile_entities.get(100, 64, 200)
```
### `minecraft.tile_entities.count(filter?)`
Count tile entities in the world.
`filter`/string (optional)/Type filter string
**Returns:** integer — number of tile entities.
```lua
local total = minecraft.tile_entities.count()
local chestCount = minecraft.tile_entities.count("Chest")
```
### Tile entity handle methods
Handles returned from `list()` and `get()` support the following methods:
`:get_id()`/string or nil/Tile entity type string (e.g. `"Chest"`, `"Furnace"`); `:get_block_id()`/int/Numeric block ID at this tile entity's position; `:get_block_meta()`/int/Block metadata value; `:is_removed()`/boolean/Whether the tile entity has been removed; `:mark_dirty()`/nothing/Mark the tile entity for saving to disk (server-side only); `:distance_from(tx, ty, tz)`/double/Euclidean distance from the tile entity to the given point; `:get_world_time()`/double/Current world tick time; `:get_data()`/table or nil/Custom NBT data as a Lua table (only for LuaModBlockEntities); `:set_data(table)`/nothing/custom NBT data (only for LuaModBlockEntities); `:get_animation_frame()`/int/Current animation frame count (= tick × speed); `:set_animation_speed(speed)`/nothing/animation speed multiplier
Handles also have read-only fields `x`, `y`, `z` for world position.
```lua
local te = minecraft.tile_entities.get(100, 64, 200)
if te then
  print("ID:", te:get_id())
  print("Block:", te:get_block_id())
  print("Meta:", te:get_block_meta())
  print("Dist from origin:", te:distance_from(0, 0, 0))
  print("World time:", te:get_world_time())
  te:mark_dirty()

  -- For mod block entities
  local data = te:get_data()
  if data then
    data.my_custom_field = 42
    te:set_data(data)
  end

  -- Animation control
  print("Frame:", te:get_animation_frame())
  te:set_animation_speed(2.0)  -- double speed
end
```
## `minecraft.particles.*`
### `minecraft.particles.spawn({x?, y?, z?, vx?, vy?, vz?, scale?, r?, g?, b?, max_age?, gravity?})`
Spawn a custom client-side particle. Client-only (does nothing on server).
`x`/float/`0.0`/Spawn position X; `y`/float/`64.0`/Spawn position Y; `z`/float/`0.0`/Spawn position Z; `vx`/float/`0.0`/Initial velocity X; `vy`/float/`0.0`/Initial velocity Y; `vz`/float/`0.0`/Initial velocity Z; `scale`/float/`4.0`/Particle scale (clamped 0.05–4.0); `r`/float/`1.0`/Red color component; passed through without clamping; `g`/float/`1.0`/Green color component; passed through without clamping; `b`/float/`1.0`/Blue color component; passed through without clamping; `max_age`/int/`40`/Particle lifetime in ticks; `gravity`/float/`0.04`/Gravity strength (positive = downward)
**Returns:** boolean — `true` if the particle was spawned (always `true` on client with valid world).
```lua
minecraft.particles.spawn({
  x = 100, y = 64, z = 200,
  vx = 0, vy = 1, vz = 0,
  scale = 2.0,
  r = 1.0, g = 0.2, b = 0.2,
  max_age = 60,
  gravity = 0.01
})
```
## `minecraft.items.*`
### `minecraft.items.ids()`
Returns a sequential array of all registered item numeric IDs.
**Returns:** array of integers — all registered item IDs.
```lua
local allIds = minecraft.items.ids()
for _, id in ipairs(allIds) do
  print("Item ID:", id)
end
```
## `minecraft.raycast`
### `minecraft.raycast(options?)`
Perform a raycast from the camera, or from a custom origin with a custom direction. Client-only (returns `nil` on server).
`options`/table (optional)/Configuration table (see below). If omitted, raycasts from center of screen with reach distance
**Options table fields:**
`origin`/table/camera position/`{x, y, z}` or positional `{origin_x, origin_y, origin_z}` — ray origin; `direction`/table/camera look vector/`{x, y, z}` vector direction; `yaw`, `pitch`/float/camera look/Alternative to `direction` — converts degrees to direction vector; `max_distance`/float/reach / `5.0`/Maximum raycast distance; `reach`/float/reach / `5.0`/Alias for `max_distance`; `ignore_liquids`/boolean/`false`/Whether to ignore liquid blocks; `blocks`/boolean/`true`/Whether to test against blocks; `entities`/boolean/`true`/Whether to test against entities
**Returns:** hit result table or `nil` if nothing was hit.
The result table has `type` — one of `"block"`, `"entity"`, or `"model"`.
**Block hit result:**
`type`/string/`"block"`; `block_id`/int/Hit block's numeric ID; `block_name`/string/Hit block's wire name; `item_id`/int/Item ID (same as block_id for blocks); `block_x`, `block_y`, `block_z`/int/Block position; `side`/int/Face side hit (0–5); `hit_x`, `hit_y`, `hit_z`/double/Exact hit position
**Entity hit result:**
`type`/string/`"entity"`; `entity_id`/int/Entity network ID; `entity_raw_id`/int/Raw entity registry ID; `entity_type`/string/Entity type string; `entity_x`, `entity_y`, `entity_z`/double/Entity position; `hit_x`, `hit_y`, `hit_z`/double/Exact hit position
**Model hit result:**
`type`/string/`"model"`; `model_id`/int/Placed model instance ID; `model_tag`/string/Model tag string; `hit_x`, `hit_y`, `hit_z`/double/Exact hit position; `distance`/double/Distance from ray origin
All hit results include `hit_x`, `hit_y`, `hit_z`.
```lua
-- Default raycast from center of screen
local hit = minecraft.raycast()
if hit then
  if hit.type == "block" then
    print("Hit block", hit.block_name, "at", hit.block_x, hit.block_y, hit.block_z, "side", hit.side)
  elseif hit.type == "entity" then
    print("Hit entity", hit.entity_type, "ID:", hit.entity_id)
  elseif hit.type == "model" then
    print("Hit model", hit.model_id, "tag:", hit.model_tag)
  end
end

-- Custom raycast with origin and direction
local hit = minecraft.raycast({
  origin = { x = 0, y = 64, z = 0 },
  direction = { x = 0, y = -1, z = 0 },
  max_distance = 100,
  ignore_liquids = true,
  blocks = true,
  entities = false,
})

-- Custom raycast using yaw/pitch (degrees)
local hit = minecraft.raycast({
  origin = { x = 0, y = 64, z = 0 },
  yaw = 0, pitch = -90,
  reach = 50,
})
```
## World Generation Events
### `chunk_generation`
Fired during server-side chunk generation. `chunk_x` and `chunk_z` are absolute chunk coordinates; block edits through `event.chunk` use local X/Z coordinates (0–15).
**Event fields:**
`stage`/string/One of: `"terrain"`, `"surface"`, `"carver"`, `"features"`; `moment`/string/`"before"` or `"after"`; `cancel_vanilla`/boolean/to `true` to cancel vanilla generation for this stage (read-write); `vanilla_stage_ran`/boolean/Whether the vanilla stage already executed; `world_seed`/int/World seed; `has_world`/boolean/Whether a world context is available; `world_name`/string/World name; `is_overworld`/boolean/Whether this is the Overworld dimension; `mod_generation`/boolean/Whether mod generation is enabled for this world; `chunk_x`, `chunk_z`/int/Chunk coordinates; `has_chunk`/boolean/Whether a chunk context is active; `chunk`/ChunkHandle/The local chunk object handle for generation writes/reads
**Stages:**
`terrain`/RawGeneration/Base terrain shape (no biome decorations yet); `surface`/RawGeneration/Surface layer placement (grass, sand, gravel, etc.); `carver`/RawGeneration/Cave/canyon carving; `features`/ChunkApi/Ore veins, trees, flowers, dungeons, etc
The `terrain`, `surface`, and `carver` stages use raw generation writes (bypass heightmap updates). The `features` stage uses the standard chunk API path.
```lua
-- Replace vanilla terrain with custom blocks
minecraft.on(minecraft.events.chunk_generation, {}, function(event)
  if event.stage == "terrain" and event.moment == "before" then
    event.cancel_vanilla = true

    local chunk = event.chunk
    -- Fill entire chunk with stone up to Y=64
    chunk:fill(0, 0, 0, 15, 64, 15, minecraft.world.block_id("stone"))
    -- Add a grass layer on top
    chunk:fill(0, 64, 0, 15, 64, 15, minecraft.world.block_id("grass_block"))

    -- Read during generation
    local block = chunk:get_block(7, 64, 7)
  end
end)

-- Decorate after vanilla features
minecraft.on(minecraft.events.chunk_generation, {}, function(event)
  if event.stage == "features" and event.moment == "after" then
    local chunk = event.chunk
    local h = chunk:get_height(7, 7)
    chunk:set_block(7, h, 7, minecraft.world.block_id("torch"))
  end
end)
```
### `world_spawn_search`
Fired when the game searches for a valid world spawn point. Can be used to override the spawn location.
**Event fields:**
`x`/int/Candidate spawn X (read-write); `y`/int/Candidate spawn Y (read-write, default `64`); `z`/int/Candidate spawn Z (read-write); `resolved`/boolean/to `true` to accept the current coordinates as the spawn point (read-write); `has_world`/boolean/Whether a world context is available; `world_name`/string/World name; `is_overworld`/boolean/Whether this is the Overworld
```lua
minecraft.on(minecraft.events.world_spawn_search, {}, function(event)
  if not event.resolved then
    event.x = 0
    event.y = 70
    event.z = 0
    event.resolved = true
  end
end)
```
If no mod resolves the spawn, the default overworld spawn logic still requires sand to be present at the candidate location.
### `create_world`
Fired when a new world is being created. Cancellable.
**Event fields:**
`save_name`/string/World save name; `seed`/int/World seed; `canceled`/boolean/to `true` to cancel world creation (read-write); `options`/table/Map of string key-value pairs persisted in `level.dat` (read-write)
```lua
minecraft.on(minecraft.events.create_world, {}, function(event)
  event.options = event.options or {}
  event.options["mymod:difficulty"] = "hard"
  event.options["mymod:starting_items"] = "yes"
end)
```
### `world_open`
Fired when a world is opened (loaded). Read-only.
**Event fields:**
`save_name`/string/World save name; `new_world`/boolean/Whether this is a newly created world; `options`/table/Map of persisted options string values from `level.dat`
### `world_start`
Fired when a world starts ticking. Read-only.
**Event fields:**
`save_name`/string/World save name; `new_world`/boolean/Whether this is a newly created world
### `world_tick`
Fired every world tick.
**Event fields:**
`remote`/boolean/Whether this is the client world; `before`/boolean/`true` for pre-tick, `false` for post-tick
```lua
minecraft.on(minecraft.events.world_tick, {}, function(event)
  if event.before then
    -- Pre-tick logic
  else
    -- Post-tick logic
  end
end)
```

# Rendering
## World Rendering Pipeline
The engine renders the world in a fixed sequence of stages. For each stage the `world_render` event fires twice: **before** the stage runs and **after** it completes. Mods can hook these moments to inject custom geometry or override vanilla rendering.
### `minecraft.render.stages`
Constant table with one entry per stage (value equals its own key):
``"sky"``: Sky background; ``"stars"``: Starfield; ``"terrain_opaque"``: Opaque terrain (chunk geometry); ``"entities"``: All living entities and items; ``"particles_lit"``: Lit particles (torches, glowstone, etc.); ``"particles"``: Regular particles; ``"terrain_translucent"``: Translucent terrain (water, glass, ice); ``"weather"``: Rain and snow; ``"clouds"``: Clouds; ``"hand"``: First-person hand; ``"framebuffer"``: Framebuffer blit / post-processing
Usage:
```lua
minecraft.on(minecraft.events.world_render, { stage = minecraft.render.stages.entities, moment = minecraft.render.moments.before },
  function(event)
    -- Draw custom geometry before entities render
  end)
```
### `minecraft.render.moments`
``"before"``: Fire just before vanilla renders the stage; ``"after"``: Fire just after vanilla renders the stage
The `world_render` event also exposes the following fields on the event table:
`world`/World/The current world; `camera`/Entity/The active camera entity; `tick_delta`/number/Partial tick for interpolation; `stage`/string/The current render stage; `moment`/string/`"before"` or `"after"`; `cancel_vanilla`/boolean/to `true` to skip vanilla rendering for this stage; `vanilla_stage_ran`/boolean/Whether vanilla already rendered this stage; `shadow_pass`/boolean/`true` while an offscreen shadow-depth pass renders the `entities` stage; `celestial_angle`/number/Current sun angle (0.0–1.0); `sky_yaw_deg`/number/Skybox rotation in degrees; `star_brightness`/number/Current star brightness (stars/before; writable); `rain_strength`/number/Rain intensity (0.0–1.0); `stars_enabled`/boolean/Whether stars are enabled in the sky; `astronomy_enabled`/boolean/Whether astronomy mode is active; `astronomy_utc_millis`/number/UTC epoch milliseconds for astronomy; `observer_latitude_deg`/number/Observer latitude in degrees; `observer_longitude_deg`/number/Observer longitude in degrees
## `minecraft.render.*`
### `minecraft.render.quads(spec)`
Draw textured or colored quads in world space. Only works when a world draw context is active (i.e. inside a `world_render` callback). The camera offset is handled automatically — position values are world-absolute unless `world_space` is specified.
**spec table fields:**
`texture`/string/`""` (untextured)/Path to a texture to bind; `texture_id`/int/`-1`/Raw GL texture ID (ignored if `texture` is set); `r`/number/`1.0`/Default red tint (0–1) for all vertices; `g`/number/`1.0`/Default green tint (0–1); `b`/number/`1.0`/Default blue tint (0–1); `a`/number/`1.0`/Default alpha (0–1); `x`/number/`0.0`/World-space X position (anchor); `y`/number/`0.0`/World-space Y position (anchor); `z`/number/`0.0`/World-space Z position (anchor); `yaw`/number/`0.0`/Yaw rotation in degrees (around Y axis); `pitch`/number/`0.0`/Pitch rotation in degrees (around X axis); `roll`/number/`0.0`/Roll rotation in degrees (around Z axis); `scale`/number/`1.0`/Uniform scale factor; `world_space`/boolean/`false`/If true, x/y/z are absolute world coords (camera is subtracted); `blend`/boolean/`true`/Enable alpha blending; `cull`/boolean/`false`/Enable back-face culling; `depth_test`/boolean/`true`/Enable depth testing; `depth_write`/boolean/`true`/Enable depth buffer writes; `vertices`/array/required/Array of vertex tables
**Vertex table fields** (each vertex can override the default tint):
`x`/number/Model-space X coordinate; `y`/number/Model-space Y coordinate; `z`/number/Model-space Z coordinate; `u`/number/Texture U coordinate (ignored when untextured); `v`/number/Texture V coordinate (ignored when untextured); `r`/number/Per-vertex red override (0–1); `g`/number/Per-vertex green override (0–1); `b`/number/Per-vertex blue override (0–1); `a`/number/Per-vertex alpha override (0–1)
If `texture` and `texture_id` are both absent/empty, quads are drawn without a bound texture (colored only).
The vertex count is rounded down to the nearest multiple of 4. Returns the number of quads drawn (integer), or `0` if no world draw context is active.
```lua
minecraft.on(minecraft.events.world_render, { stage = "entities", moment = "after" }, function(event)
  minecraft.render.quads({
    texture = "mymod:textures/blocks/test.png",
    x = event.camera.x, y = event.camera.y - 1, z = event.camera.z,
    yaw = event.camera.yaw,
    vertices = {
      { x = -0.5, y = 0, z = -0.5, u = 0, v = 0 },
      { x =  0.5, y = 0, z = -0.5, u = 1, v = 0 },
      { x =  0.5, y = 0, z =  0.5, u = 1, v = 1 },
      { x = -0.5, y = 0, z =  0.5, u = 0, v = 1 },
    }
  })
end)
```
### `minecraft.render.billboards(spec)`
Draw always-facing quads (billboards) using 3D direction vectors. The engine places them on a sphere about 100 world units from the camera; `x/y/z` are not absolute world positions. Only works during a world draw context.
**spec table fields:**
`brightness`/number/`1.0`/Brightness multiplier (0–1, ≤0 draws nothing); `rotation_x_rad`/number/`0.0`/Additional X-axis rotation in radians; `rotation_y_rad`/number/`0.0`/Additional Y-axis rotation in radians; `blend`/string/`"alpha"`/Blend mode: `"alpha"` or `"additive"`; `depth_test`/boolean/`false`/Enable depth testing; `depth_write`/boolean/`false`/Enable depth buffer writes; `billboards`/array/required/Array of billboard specs (also accepts `points` as alias)
**Billboard entry fields:**
`x`, `y`, `z`/number/`0.0`/Direction vector components in 3D world space. Zenith = `(0,1,0)`, north ≈ `(0,0,-1)`, east ≈ `(1,0,0)`; `size`/number/`0.2`/Billboard size (world units); `alpha`/number/`1.0`/Alpha transparency (0–1)
Returns the count of billboards emitted (integer).
```lua
minecraft.render.billboards({
  brightness = 1.0,
  blend = "additive",
  billboards = {
    { x = 0.0, y = 1.0, z = 0.0, size = 0.5, alpha = 1.0 },  -- zenith
    { x = 1.0, y = 0.0, z = 0.0, size = 0.3, alpha = 0.8 },  -- east
  }
})
```
### `minecraft.render.set_item_entity_override(enabled)`
Suppress the native dropped-item (ItemEntity) sprite renderer so a Lua mod can draw its own custom 3D model for ItemEntities instead. Pass `true` to override, `false` to restore vanilla rendering.
```lua
minecraft.render.set_item_entity_override(true)
```
## `minecraft.tessellator.*`
### `minecraft.tessellator.quad(spec)`
Emit a single textured/colored quad to the currently active world tessellator (block or item draw). Must be called during a block or item model callback (i.e. from a model function registered via `register_block` or `register_item`).
**spec table fields:**
`texture`/string/`""`/Texture path (overrides `texture_id`); `texture_id`/int/`-1`/Raw texture ID; `r`/number/`1.0`/Red tint (0–1); `g`/number/`1.0`/Green tint (0–1); `b`/number/`1.0`/Blue tint (0–1); `a`/number/`1.0`/Alpha (0–1); `vertices`/array/required/Exactly 4 vertex tables `{x, y, z, u, v}`
Returns `true` if the quad was emitted, `false` otherwise.
```lua
minecraft.tessellator.quad({
  texture = "minecraft:textures/blocks/diamond_block.png",
  vertices = {
    { x = 0, y = 0, z = 0, u = 0, v = 0 },
    { x = 1, y = 0, z = 0, u = 1, v = 0 },
    { x = 1, y = 1, z = 0, u = 1, v = 1 },
    { x = 0, y = 1, z = 0, u = 0, v = 1 },
  }
})
```
Positions are in the block/item's local model space. The tessellator handles atlas UV mapping automatically.
## `minecraft.camera.*` (Render Targets / Viewfinder Cameras)
Create and manage render targets (offscreen framebuffers) backed by the `GameRenderer` pipeline. Use these to render the world to a texture for viewfinder displays, CCTV feeds, etc.
`create`/`(width: int, height: int, colorCount?: int, useDepthTex?: bool)`/handle (int) or `-1`; `create_display_size`/`(colorCount?: int, useDepthTex?: bool)`/handle (int) or `-1`; `destroy`/`(handle: int)`/`bool`; `resize`/`(handle: int, width: int, height: int)`/`bool`; `render`/`(handle: int, x, y, z, yaw, pitch, roll, fov, tickDelta?: number)`/`bool`; `texture`/`(handle: int, attachmentIndex?: int)`/GL texture ID (int); `width`/`(handle: int)`/width (int); `height`/`(handle: int)`/height (int); `rendering`/`()`/currently bound handle (int), `-1` if none; `unbind`/`()`/`bool`
### `camera.create(width, height, colorCount?, useDepthTex?)`
Create a new render target. `colorCount` defaults to 1. `useDepthTex` (default `false`) attaches a depth texture instead of a renderbuffer, allowing the depth buffer to be sampled in shaders.
Returns a numeric handle, or `-1` if creation fails.
### `camera.create_display_size(colorCount?, useDepthTex?)`
Create a render target sized to the current display (window) dimensions.
### `camera.destroy(handle)`
Destroy a previously created render target. Returns `true` on success.
### `camera.resize(handle, width, height)`
Resize the target. The existing color attachment count is retained. Returns `true` on success.
### `camera.render(handle, x, y, z, yaw, pitch, roll, fov, tickDelta?)`
Render the world into the target from the given camera pose. `tickDelta` defaults to `1.0`. Returns `true` on success.
```lua
local cam = minecraft.camera.create(320, 240)
minecraft.camera.render(cam, player.x, player.y, player.z, player.yaw, player.pitch, 0, 70)
local texId = minecraft.camera.texture(cam)
```
### `camera.texture(handle, attachmentIndex?)`
Get the OpenGL texture ID for the given attachment index (default `0`). Returns `-1` if the handle is invalid.
### `camera.width(handle)` / `camera.height(handle)`
Get the render target dimensions. Returns `0` for invalid handles.
### `camera.rendering()`
Returns the handle of the currently bound render target, or `-1` if none.
### `camera.unbind()`
Unbind the currently bound render target. Returns `true`.
## `minecraft.fbo.*` (Offscreen Framebuffers)
Raw OpenGL framebuffer objects for custom render passes. Separate from `minecraft.camera` — these are not tied to `GameRenderer` and do not run the world render pipeline. Use them for post-processing, shadow maps, or custom shader operations.
`create`/`(width: int, height: int, colorCount?: int, useDepthTex?: bool)`/handle (int) or `-1`; `create_display_size`/`(colorCount?: int, useDepthTex?: bool)`/handle (int) or `-1`; `destroy`/`(handle: int)`/`bool`; `resize`/`(handle: int, width: int, height: int)`/`bool`; `bind`/`(handle: int)`/`bool`; `unbind`/`()`/`bool`; `texture`/`(handle: int, attachmentIndex?: int)`/GL texture ID (int); `width`/`(handle: int)`/width (int); `height`/`(handle: int)`/height (int); `bound`/`()`/currently bound handle (int), `-1` if none
Parameters follow the same conventions as `minecraft.camera.*` (colorCount defaults to 1, useDepthTex defaults to `false`).
```lua
local fbo = minecraft.fbo.create(512, 512)
minecraft.fbo.bind(fbo)
-- render custom geometry here
minecraft.fbo.unbind()
local fboTex = minecraft.fbo.texture(fbo)
```
## `minecraft.model.*`
Baked model management: load JSON models, build procedural models from quads, place instances with physics hitboxes, and draw them in world space.
### `minecraft.model.load(path)`
Load and bake a JSON model from a mod's assets. The path is relative to the mod's asset root (e.g. `"mymod:models/block/myblock.json"`). Parent chains are resolved automatically. Results are cached by `(modId, path)`.
Returns the model handle (integer ≥ 1) on success, or `nil, error` on failure.
```lua
local handle, err = minecraft.model.load("mymod:models/block/myblock.json")
if not handle then print("load failed:", err) end
```
### `minecraft.model.build(spec)`
Build a baked model programmatically from an array of quads. Supports caching via the `key` field.
**spec table fields:**
`quads`/array/required/Array of quad specifications; `key`/string/`""`/Optional cache key — reuse builds with the same key return the same handle
**Quad specification fields:**
`texture`/string/`""`/Texture path for this quad; `r`/number/`1.0`/Red tint (0–1); `g`/number/`1.0`/Green tint (0–1); `b`/number/`1.0`/Blue tint (0–1); `a`/number/`1.0`/Alpha (0–1); `shade`/number/`1.0`/Directional shading factor (0–1); `vertices`/array/required/Exactly 4 vertex tables `{x, y, z, u, v}`
Returns the model handle (integer) on success, or `nil, error` on failure.
```lua
local handle = minecraft.model.build({
  key = "my_custom_cube",
  quads = {
    { texture = "minecraft:textures/blocks/stone.png",
      vertices = {
        { x = 0, y = 0, z = 0, u = 0, v = 0 },
        { x = 1, y = 0, z = 0, u = 1, v = 0 },
        { x = 1, y = 1, z = 0, u = 1, v = 1 },
        { x = 0, y = 1, z = 0, u = 0, v = 1 },
      }
    },
  }
})
```
### `minecraft.model.place(handle, opts)`
Place an instance of a baked model in the world. The instance creates a hitbox that the engine's raycast system honors (via `raycast` event if the mod implements it). The bounding box is derived from the model's baked bounds and the transform's scale.
**opts fields:**
`x`/number/`0.0`/World X position; `y`/number/`0.0`/World Y position; `z`/number/`0.0`/World Z position; `yaw`/number/`0.0`/Yaw rotation in degrees; `pitch`/number/`0.0`/Pitch rotation in degrees; `roll`/number/`0.0`/Roll rotation in degrees; `scale`/number/`1.0`/Uniform scale factor; `pivot_y`/number/`0.0`/Y offset of the rotation pivot in model space; `tag`/string/`""`/Arbitrary string tag (accessible in raycast results)
Returns the instance ID (integer ≥ 1) on success, or `nil, error` on failure.
```lua
local instanceId = minecraft.model.place(handle, { x = 100, y = 64, z = 100, scale = 2, tag = "landmark" })
```
### `minecraft.model.update(instanceId, opts)`
Update an existing placed instance's transform. Same `opts` fields as `place` (except `tag`). Omitted transform fields reset to defaults (`x/y/z = 0`, angles = `0`, `scale = 1`, `pivot_y = 0`); pass every field that must be kept. Returns `true` on success.
```lua
minecraft.model.update(instanceId, { x = 100, y = 70, z = 100, yaw = 45,
  pitch = 0, roll = 0, scale = 1, pivot_y = 0 })
```
### `minecraft.model.remove(instanceId)`
Remove a placed instance. Returns `true` on success.
### `minecraft.model.clear()`
Remove all placed model instances belonging to the current mod.
### `minecraft.model.bounds(handle)`
Get the model-space bounding box of a baked model. Returns a table with `min_x`, `min_y`, `min_z`, `max_x`, `max_y`, `max_z` (all floats), or `nil` if the model has no bounds.
### `minecraft.model.draw(handle, opts)`
Draw a baked model in world space immediately. Only works during a world draw context (`world_render` event). The camera offset is handled automatically. Returns `true` if the model was drawn, `false` if no client renderer is active or the handle is invalid.
**opts fields:**
`x`/number/`0.0`/World X position; `y`/number/`0.0`/World Y position; `z`/number/`0.0`/World Z position; `yaw`/number/`0.0`/Yaw rotation in degrees; `pitch`/number/`0.0`/Pitch rotation in degrees; `roll`/number/`0.0`/Roll rotation in degrees; `scale`/number/`1.0`/Uniform scale factor; `pivot_y`/number/`0.0`/Y offset of the rotation pivot in model space; `brightness`/number/world light/Brightness multiplier (0–1); omitted samples light at `x`, `y`, `z`; `a`/number/`1.0`/Alpha override (0–1, multiplied into each quad's alpha); `blend`/boolean/`true`/Enable alpha blending; `cull`/boolean/`false`/Enable back-face culling; `depth_test`/boolean/`true`/Enable depth testing; `depth_write`/boolean/`true`/Enable depth buffer writes
```lua
minecraft.on(minecraft.events.world_render, { stage = "entities", moment = "after" }, function()
  minecraft.model.draw(handle, { x = 100, y = 64, z = 100, yaw = 45, scale = 1.5 })
end)
```
### `minecraft.model.draw_item(item_id, damage, opts)`
Draw an item or block's 3D model in world space. Uses the same 3D model the game would use for dropped item entities or inventory icons. For plain sprite items (tools, food, etc.) that have no 3D shape, returns `false` — callers should fall back to their own flat-icon representation. Omitted `brightness` samples world light at the draw position. Same remaining `opts` fields as `model.draw`.
```lua
local drew = minecraft.model.draw_item(1, 0, { x = player.x, y = player.y + 2, z = player.z, scale = 3 })
if not drew then
  -- draw a flat sprite icon instead
end
```
### `minecraft.model.item_bounds(item_id, damage)`
Get the model-space bounding box of an item's 3D model. Returns a table with `min_x`, `min_y`, `min_z`, `max_x`, `max_y`, `max_z`, or `nil` if the item has no real 3D shape (plain sprite items). Vanilla full-block items that lack a custom baked model return an approximate unit-cube `{0,0,0,1,1,1}`.
```lua
local bounds = minecraft.model.item_bounds(1, 0)
if bounds then
  print("item bounds:", bounds.min_x, bounds.min_y, bounds.min_z, bounds.max_x, bounds.max_y, bounds.max_z)
end
```
## `minecraft.model.voxels(opts)`
Voxel model builder — constructs a baked model from integer-lattice cells. Implemented in Lua on top of `minecraft.model.build`. Interior faces (shared with a present neighbor) are automatically culled.
**opts fields:**
`cells`/array/required/Array of cell specifications; `resolution`/int/`16`/Voxel grid resolution; `scale`/number/`1/resolution`/Size of one voxel in model units; `origin_x`/number/`0`/Model origin X offset; `origin_y`/number/`0`/Model origin Y offset; `origin_z`/number/`0`/Model origin Z offset; `key`/string/`nil`/Cache key for reuse
**Cell specification:**
`x`/int/required/Cell X coordinate on the integer lattice; `y`/int/required/Cell Y coordinate on the integer lattice; `z`/int/required/Cell Z coordinate on the integer lattice; `r`/number/`1.0`/Red tint (0–1); `g`/number/`1.0`/Green tint (0–1); `b`/number/`1.0`/Blue tint (0–1); `a`/number/`1.0`/Alpha (0–1)
Returns a model handle (integer) on success, or `nil, error` if no cells were provided or all faces were culled.
```lua
local handle = minecraft.model.voxels({
  cells = {
    { x = 1, y = 1, z = 1, r = 1, g = 0, b = 0 },
    { x = 1, y = 2, z = 1 },
    { x = 2, y = 1, z = 1, r = 0, g = 0, b = 1 },
  },
  resolution = 8,
  key = "my_voxel_shape",
})
```
## `minecraft.model.voxel(spec)`
Sprite extrude to voxel — samples a texture and extrudes opaque pixels into a one-voxel-thick model centered on `z = 0.5`. Automatically cached by `texture + atlas_index + mod_texture + grid`; `alpha_cutoff` is not part of the cache key. Built in Lua on top of `model.voxels`.
**spec fields:**
`texture`/string/required/Texture path to sample; `atlas_index`/int/`-1`/Atlas tile index (for non-mod textures); `-1` uses full texture; `mod_texture`/boolean/`false`/If true, the texture is a mod texture (different UV mapping); `grid`/int/`16`/Sample grid resolution (e.g. 16 = 16×16 grid); `alpha_cutoff`/int/`30`/Alpha threshold (0–255); pixels above this are kept
Returns a model handle (integer) on success, or `nil, error` if the texture is not found or has no opaque pixels.
```lua
local handle = minecraft.model.voxel({
  texture = "minecraft:textures/items/diamond.png",
  grid = 16,
  alpha_cutoff = 30,
})
```
## `minecraft.texture.*`
Read texture metadata and pixels. Textures are cached (up to 64 entries) for performance.
### `minecraft.texture.size(path)`
Get the dimensions of a texture. Returns `{width: int, height: int}`. If the texture cannot be loaded, both values are `0`.
```lua
local size = minecraft.texture.size("minecraft:textures/blocks/stone.png")
print(size.width, size.height)
```
### `minecraft.texture.pixel(path, x, y)`
Get the color of a single pixel. Returns `{r: int, g: int, b: int, a: int}` with 0–255 values. Out-of-bounds coordinates return `{r=0, g=0, b=0, a=0}`.
```lua
local p = minecraft.texture.pixel("minecraft:textures/blocks/stone.png", 8, 8)
print(p.r, p.g, p.b, p.a)
```
## GUI 3D Viewport
Embed a 3D perspective viewport inside a GUI screen. The viewport uses its own projection and model-view matrices, clear color, and camera pose.
### `gui.begin_3d(params)`
Begin a 3D viewport region. Must be called during a `screen_ui` render callback (when `minecraft.gui` draw is active).
**params fields:**
`x`/int/`0`/Viewport X position in GUI coordinates; `y`/int/`0`/Viewport Y position in GUI coordinates; `size`/int/`0`/Shortcut — sets both `width` and `height`; `width`/int/`0`/Viewport width in GUI coordinates (overrides `size`); `height`/int/`0`/Viewport height in GUI coordinates (overrides `size`); `gui_width`/int/display width/Total GUI width (used for scaling); `gui_height`/int/display height/Total GUI height (used for scaling); `yaw_deg`/number/`0.0`/Camera yaw in degrees; `pitch_deg`/number/`0.0`/Camera pitch in degrees; `distance` / `cam_dist`/number/`2.05`/Camera distance from origin (clamped 1.5–6.0); `fov_deg`/number/`40.0`/Field of view in degrees (clamped 10–120); `clear_color`/int/—/ARGB hex color for clear (e.g. `0xFF112233`); overrides individual clear fields; `clear_r`/number/`0.11`/Clear color red (0–1); `clear_g`/number/`0.13`/Clear color green (0–1); `clear_b`/number/`0.17`/Clear color blue (0–1); `clear_a`/number/`1.0`/Clear color alpha (0–1)
```lua
-- In a screen_ui render callback:
gui.begin_3d({
  x = 10, y = 10, size = 100,
  yaw_deg = 45, pitch_deg = -30, distance = 3,
  fov_deg = 60,
  clear_color = 0xFF334466,
})
```
### `gui.end_3d()`
End the current 3D viewport, restoring the previous projection and viewport.
### `gui.draw_3d(spec)`
Draw immediate-mode geometry inside an active 3D viewport (between `begin_3d` and `end_3d`).
**spec fields:**
`mode`/string/required/Draw mode: `"lines"`, `"line_strip"`, `"line_loop"`, `"quads"`, `"quad_strip"`, `"points"`, `"triangles"`; `color`/int/—/ARGB hex color (overrides r/g/b/a); `r`/number/`1.0`/Red (0–1); `g`/number/`1.0`/Green (0–1); `b`/number/`1.0`/Blue (0–1); `a`/number/`1.0`/Alpha (0–1); `line_width`/number/`1.0`/Line width (clamped 0.5–8.0); `point_size`/number/`1.0`/Point size (clamped 1.0–16.0); `vertices`/array/required/Array of `{x, y, z}` vertex tables
```lua
gui.draw_3d({
  mode = "lines",
  color = 0xFFFF4444,
  line_width = 2,
  vertices = {
    { x = 0, y = 0, z = 0 },
    { x = 1, y = 0, z = 0 },
    { x = 0, y = 1, z = 0 },
    { x = 1, y = 1, z = 0 },
  }
})
```
### `gui.unproject(params)`
Unproject a mouse position in GUI coordinates to a 3D ray (origin + direction) in the viewport's world space. This is useful for mouse-picking inside a 3D viewport.
Takes the same position/size/orientation params as `begin_3d`, plus `mouse_x` and `mouse_y`. Returns `{ origin = {x, y, z}, direction = {x, y, z} }` or `nil` if the mouse is outside the viewport.
```lua
local ray = gui.unproject({
  x = 10, y = 10, size = 100,
  yaw_deg = 45, pitch_deg = -30,
  fov_deg = 60,
  mouse_x = event.mouse_x, mouse_y = event.mouse_y,
})
if ray then
  -- ray.origin, ray.direction are {x, y, z} vectors
end
```
## Render Events (Reference)
The following events modify rendering behavior. See `05-events.md` for detailed documentation.
``world_color``: Modify sky/fog color. `event.kind` is `"sky"` or `"fog"` (from `minecraft.colors`). Set `event.color` (Vec3d); ``camera_setup``: Override camera position, rotation, and roll. Fields: `x, y, z, yaw, pitch, roll`. Set `customView = true`; ``fov``: Override field of view. Set `event.fov` (float, default 70); ``first_person_hand``: Cancel or control first-person hand rendering. Set `canceled = true` to hide the hand; ``render_frame``: Start-of-frame hook. Fires once per frame before any world rendering; ``render_targets``: Post-render-targets hook. Fires after all render targets have been populated; ``world_render``: Per-stage render hooks. Fields: `stage`, `moment`, `cancel_vanilla`, etc; ``pre_entity_render``: Pre-entity-render hook. Set `canceled = true` to skip an entity; ``entity_render``: Entity render hook with pose control. Modify `event.pose` (bodyYaw, headYaw, headPitch, yaw, pitch, roll, scale, offsetX/Y/Z, parts) to override entity rendering

# GUI and screens
## GUI draw scope
All `minecraft.gui.*` draw calls (fill_rect, draw_text, draw_item, draw_slider, draw_toggle, draw_button, draw_centered_text, draw_sprite, draw_texture) **ONLY** function during a Lua GUI draw scope. These scopes are entered automatically during:
The `render` phase of an `on_lua_screen` lifecycle handler The render callback of a `screen_ui` / `screen_region` subscription
Internally the engine tracks a depth counter (`g_luaGuiDepth`). `ScopedLuaGuiDraw` increments it on construction and decrements on destruction. Calling any draw function outside this scope silently produces no output (functions return early if `luaGuiDrawActive()` is false).
## `minecraft.gui.*`
All drawing here uses pixel coordinates relative to the Minecraft GUI scale (typically 1920×1080 nominal for fullscreen, scaled by the game's GUI scale factor).
### `minecraft.gui.fill_rect(x, y, w, h, argb_color)`
Draws a filled rectangle. `argb_color` is a 32-bit integer in 0xAARRGGBB format. If the alpha byte is 0, it defaults to 255 (fully opaque).
```lua
minecraft.gui.fill_rect(10, 10, 100, 20, 0x80FF0000) -- semi-transparent red rect
```
### `minecraft.gui.draw_text(x, y, text, argb_color)`
Draws left-aligned text at the given position. `text` must be a string. Color in ARGB format.
```lua
minecraft.gui.draw_text(50, 50, "Hello", 0xFFFFFFFF) -- white text
```
### `minecraft.gui.draw_item(x, y, itemId, count, damage?)`
Draws an item stack icon (sprite + optional count/decorations). `itemId` is the numeric item ID, `count` is the stack size, `damage` is optional (default 0). Both the item icon and its damage bar/stack count overlay are rendered.
```lua
minecraft.gui.draw_item(100, 100, 264, 1)     -- diamond
minecraft.gui.draw_item(100, 130, 268, 1, 50) -- damaged diamond sword
```
### `minecraft.gui.text_width(text)`
Returns the pixel width of `text` as rendered by the current font renderer. Can be called outside the draw scope (no guard). Returns 0 if the text renderer is unavailable.
```lua
local w = minecraft.gui.text_width("Hello") -- e.g. 30
```
### `minecraft.gui.texture_id(path)`
Returns the OpenGL texture ID for a resource path (e.g. `"/gui/gui.png"`). Returns 0 if the path is unknown or the client is not available. Can be called outside the draw scope.
```lua
local tex = minecraft.gui.texture_id("/gui/gui.png")
```
### `minecraft.gui.draw_sprite(path_or_textureId, x, y, u, v, w, h)`
Draws a sprite (sub-rectangle) from a texture atlas or full texture. The first argument can be either: A **string** path (e.g. `"/gui/gui.png"`) — the engine resolves it to a texture ID internally An **integer** texture ID (from `texture_id()`)
The remaining arguments are: `x, y` screen position, `u, v` UV offset (in pixels) within the texture, `w, h` size of the sprite region.
```lua
-- Using string path:
minecraft.gui.draw_sprite("/gui/gui.png", 10, 10, 0, 46, 100, 20)

-- Using pre-resolved texture ID:
local tex = minecraft.gui.texture_id("/gui/gui.png")
minecraft.gui.draw_sprite(tex, 10, 40, 0, 66, 100, 20)
```
### `minecraft.gui.draw_texture(textureId, x, y, w, h)`
Draws a full-texture quad covering the entire texture (UV 0-1) at the given screen rectangle. `textureId` must be a valid integer texture ID (use `texture_id()` or `draw_sprite` for atlas lookups).
```lua
local tex = minecraft.gui.texture_id("/textures/gui/title/minecraft.png")
minecraft.gui.draw_texture(tex, 50, 50, 256, 64)
```
### `minecraft.gui.draw_button({x, y, width, height, text, active?})`
Draws a vanilla-style button widget. Accepts a single table argument:
`x`/number/required/Left edge; `y`/number/required/Top edge; `width`/number/required/Width (> 0); `height`/number/required/Height (> 0); `text`/string/required/Button label; `active`/boolean/`true`/If false, drawn greyed out; `mouse_x`/number/absent/Mouse X used for hover detection when supplied; `mouse_y`/number/absent/Mouse Y used for hover detection when supplied; `hovered`/boolean/`false`/Explicit hover override; without `hovered` and both mouse coordinates, the widget is not hovered
The button is drawn with the vanilla gui.png texture (9-slice), with text centered. Active + hovered = gold text (0xFFFFA0), active = light grey (0xFFE0E0E0), inactive = dark grey (0xFFA0A0A0).
```lua
minecraft.gui.draw_button({x=100, y=200, width=150, height=20, text="Click Me", active=true,
  mouse_x = event.mouse_x, mouse_y = event.mouse_y})
```
### `minecraft.gui.draw_slider({x, y, width, height, value, text})`
Draws a vanilla-style slider widget (e.g. options screen volume slider).
`x`/number/required/Left edge; `y`/number/required/Top edge; `width`/number/required/Width (> 0); `height`/number/required/Height (> 0); `value`/number/`0.0`/Normalized position (0.0 — 1.0), clamped internally; `text`/string/required/Label rendered over the slider; `mouse_x`/number/auto/For hover detection; `mouse_y`/number/auto/For hover detection
```lua
minecraft.gui.draw_slider({x=100, y=100, width=200, height=20, value=0.5, text="Volume: 50%",
  mouse_x = event.mouse_x, mouse_y = event.mouse_y})
```
### `minecraft.gui.draw_toggle({x, y, width, height, label, value})`
Draws a vanilla-style toggle/on-off button. Rendered as a button with the label followed by ": ON" or ": OFF". Uses the translated keys `"options.on"` / `"options.off"` for the state text.
`x`/number/required/Left edge; `y`/number/required/Top edge; `width`/number/required/Width (> 0); `height`/number/required/Height (> 0); `label`/string/required/Label prefix; `value`/boolean/`false`/Toggle state; `mouse_x`/number/auto/For hover detection; `mouse_y`/number/auto/For hover detection
```lua
minecraft.gui.draw_toggle({x=100, y=50, width=150, height=20, label="Fullscreen", value=true,
  mouse_x = event.mouse_x, mouse_y = event.mouse_y})
```
### `minecraft.gui.draw_centered_text(x, y, width, text, color?)`
Draws text horizontally centered within a region. Accepts either positional arguments or a table:
**Positional form:**
```lua
minecraft.gui.draw_centered_text(x, y, width, text, color?)
```
**Table form:**
```lua
minecraft.gui.draw_centered_text({x=..., y=..., width=..., text=..., color=...})
minecraft.gui.draw_centered_text({x=..., y=..., w=..., text=..., color=...}) -- "w" alias for width
```
`x`/number/required/Left edge of centering region; `y`/number/required/Top edge; `width` / `w`/number/required/Width of centering region; `text`/string/required/Text to draw; `color`/number/`0xFFFFFFFF`/ARGB color
```lua
minecraft.gui.draw_centered_text(200, 300, 400, "Centered!", 0xFFFFFFFF)
```
## `minecraft.screen.*`
### `minecraft.screen.open(screen_id, {title?, pause?})`
Opens a Lua-defined screen. `screen_id` is a string identifier unique to your mod. The optional options table:
`title`/string/`""`/Title text drawn at the top of the screen; `pause`/boolean/`true`/Whether the screen pauses the game
The screen is registered implicitly on first open. Lifecycle is handled via `on_lua_screen`.
```lua
minecraft.screen.open("my_mod:config", {title="Configuration", pause=false})
```
### `minecraft.screen.close()`
Closes the currently open screen (returns to the previous screen or game).
```lua
minecraft.screen.close()
```
### `minecraft.screen.host_field(name)`
Reads the current text value of a field on the **host screen** (the underlying engine screen, e.g. the options screen). Returns the field text as a string, or empty string if the field or screen is not available.
```lua
local name = minecraft.screen.host_field("name")
```
### `minecraft.screen.host_set_field(name, value)`
Sets a field's text on the host screen.
```lua
minecraft.screen.host_set_field("name", "New Name")
```
### `minecraft.screen.open_host(screen_id, fields?)`
Opens a host-defined (engine/native) screen by its screen ID string. `fields` is an optional table of string key → string value pairs passed to the screen opener.
```lua
minecraft.screen.open_host(minecraft.screen.ids.options)
minecraft.screen.open_host(minecraft.screen.ids.create_world, {seed="my_seed"})
```
The screen must have been registered with the host's `HostScreenRegistry` by the engine. This function has no return value; an unknown ID is ignored by the host opener.
### `minecraft.screen.add_field(name, x, y, width, height, {text?, max_len?, numeric?, signed?, decimal?}))
Adds a text input widget to the Lua screen. **Only valid during the `init` phase** of an `on_lua_screen` lifecycle — outside init it is silently ignored.
`name`/string/required/Field identifier (used with `field_text`/`set_field_text`); `x`/number/required/Left edge; `y`/number/required/Top edge; `width`/number/required/Width; `height`/number/20/Height; `text`/string/`""`/Initial text; `max_len`/number/0/Maximum character length (0 = unlimited); `numeric`/boolean/`false`/Only allow numeric characters; `signed`/boolean/`false`/Allow leading `-` (only if numeric); `decimal`/boolean/`false`/Allow `.` decimal separator (only if numeric)
```lua
-- In init phase:
minecraft.screen.add_field("player_name", 100, 100, 200, 20, {text="Steve", max_len=16})
minecraft.screen.add_field("age", 100, 130, 100, 20, {numeric=true, signed=false, max_len=3})
```
### `minecraft.screen.field_text(name)`
Returns the current text of a Lua screen field added with `add_field`. Returns empty string if the field doesn't exist.
```lua
local name = minecraft.screen.field_text("player_name")
```
### `minecraft.screen.set_field_text(name, text)`
Sets the text of a Lua screen field programmatically.
```lua
minecraft.screen.set_field_text("player_name", "Alex")
```
### `minecraft.screen.add_button(x, y, width, height, text, callback?)`
Adds a clickable button widget to the Lua screen. **Only valid during the `init` phase**. The `callback` is a function invoked with no arguments when the button is clicked. If no callback is provided, the button is non-functional but still rendered.
```lua
-- In init phase:
minecraft.screen.add_button(100, 200, 200, 20, "Save", function()
  print("Saved!")
end)
```
### `minecraft.screen.set_fields_visible(bool)`
Toggles visibility of all text field widgets on the Lua screen. Default is visible.
```lua
minecraft.screen.set_fields_visible(false) -- hide all fields
minecraft.screen.set_fields_visible(true)  -- show all fields
```
## `minecraft.screen.on_ui(screen_id, region, callback, priority?)`
Attaches a callback to a **host screen's region** (e.g. the footer of the options screen). This is a convenience wrapper around subscribing to the `screen_ui` event.
`screen_id`/string/One of `minecraft.screen.ids.*`, or any string; `region`/string/One of `minecraft.screen.regions.*`; `callback`/function/Receives an event with an `event.ui` context object; `priority`/number/Optional (default 0)
The callback receives an event table with an `event.ui` object. These are methods: call them with colon syntax (`event.ui:method(...)`) so Lua passes the receiver.
`event.ui:add_centered_button(y, text, callback?)` — Add a centered button at pixel y `event.ui:add_stacked_centered_button(text, callback?)` — Add a button centered and stacked below the last one (auto-y) `event.ui:add_button(x, y, w, h, text, callback?)` — Add a button at exact position `event.host_fields` — Table of host-screen field values available to the callback
`event.ui` does not expose a `screen` property. Use the host-field helpers when you need to read or update fields on the underlying screen.
```lua
minecraft.screen.on_ui(minecraft.screen.ids.title, minecraft.screen.regions.screen, function(event)
  if event.ui == nil then return event end
  event.ui:add_centered_button(200, "My Mod", function()
    minecraft.screen.open("my_mod:main")
  end)
  return event
end)
```
### Regions
Available through `minecraft.screen.regions.*`:
`regions.footer`/`"footer"`/Bottom region of host screens; `regions.screen`/`"screen"`/Main region, published by every screen's `init()` after building widgets; `regions.side_panel`/`"side_panel"`/Inventory side panel region
The `"screen"` region is the most commonly used — every engine screen publishes to this region during its `init()` phase, allowing mods to add buttons/behavior to any GUI.
`minecraft:mod_settings` treats its footer specially. It collects every injected button and shows the buttons on a separate, scrollable **Mod Pages** page. Registered settings and keybinds remain on the main **Mod Settings** page.
## `minecraft.screen.on_lua_screen(screen_id, handlers, priority?)`
Handles the lifecycle of a Lua-defined screen (opened via `minecraft.screen.open`). The `handlers` table can have these phases:
Handlers run synchronously on the event-publishing thread under the mod's Lua-state mutex; do not assume they are asynchronous or yieldable.
`init`/`{screen_id, phase, width, height}`/Screen is initializing — add_field, add_button, and set title here; `render`/`{screen_id, phase, width, height, mouse_x, mouse_y, tick_delta}`/Every frame — GUI draw calls work here; `mouse`/`{screen_id, phase, x, y, button, released?}`/Mouse click/release events. `released` is true for release, false/absent for press; `key`/`{screen_id, phase, key, char, handled?}`/Keyboard events. Set `handled = true` to prevent default close-on-escape; `tick`/`{screen}`/Called every game tick (20 Hz); `scroll`/`{screen_id, phase, x, y, delta}`/Mouse scroll events; `close`/`{screen_id, phase}`/Screen is being removed
```lua
minecraft.screen.on_lua_screen("my_mod:config", {
  init = function(event)
    minecraft.screen.add_field("name", 10, 50, 200, 20, {text="default"})
    minecraft.screen.add_button(10, 80, 200, 20, "Save", function()
      print("Saved:", minecraft.screen.field_text("name"))
      minecraft.screen.close()
    end)
    minecraft.screen.add_button(10, 105, 200, 20, "Cancel", minecraft.screen.close)
  end,
  render = function(event)
    minecraft.gui.draw_text(10, 30, "Name:", 0xFFFFFFFF)
  end,
  mouse = function(event)
    if event.released then
      -- handle mouse release
    end
  end,
  key = function(event)
    if event.key == minecraft.keys.escape then
      event.handled = true -- prevent closing
    end
  end,
})
```
## Screen constants
### `minecraft.screen.ids.*`
All screen ID string constants for the engine's built-in screens:
``ids.login``: `"minecraft:login"`; ``ids.title``: `"minecraft:title"`; ``ids.game_menu``: `"minecraft:game_menu"`; ``ids.multiplayer``: `"minecraft:multiplayer"`; ``ids.connect``: `"minecraft:connect"`; ``ids.disconnected``: `"minecraft:disconnected"`; ``ids.downloading_terrain``: `"minecraft:downloading_terrain"`; ``ids.death``: `"minecraft:death"`; ``ids.chat``: `"minecraft:chat"`; ``ids.sleeping_chat``: `"minecraft:sleeping_chat"`; ``ids.confirm``: `"minecraft:confirm"`; ``ids.create_world``: `"minecraft:create_world"`; ``ids.select_world``: `"minecraft:select_world"`; ``ids.edit_world``: `"minecraft:edit_world"`; ``ids.world_settings``: `"minecraft:world_settings"`; ``ids.world_save_conflict``: `"minecraft:world_save_conflict"`; ``ids.inventory``: `"minecraft:inventory"`; ``ids.crafting``: `"minecraft:crafting"`; ``ids.dispenser``: `"minecraft:dispenser"`; ``ids.double_chest``: `"minecraft:double_chest"`; ``ids.furnace``: `"minecraft:furnace"`; ``ids.sign_edit``: `"minecraft:sign_edit"`; ``ids.options``: `"minecraft:options"`; ``ids.video_options``: `"minecraft:video_options"`; ``ids.detail_settings``: `"minecraft:detail_settings"`; ``ids.keybinds``: `"minecraft:keybinds"`; ``ids.mods``: `"minecraft:mods"`; ``ids.mod_settings``: `"minecraft:mod_settings"`; ``ids.achievements``: `"minecraft:achievements"`; ``ids.stats``: `"minecraft:stats"`; ``ids.lan``: `"minecraft:lan"`; ``ids.lan_info``: `"minecraft:lan_info"`; ``ids.server_mod_download``: `"minecraft:server_mod_download"`; ``ids.fatal_error``: `"minecraft:fatal_error"`; ``ids.out_of_memory``: `"minecraft:out_of_memory"`
### `minecraft.screen.regions.*`
`regions.footer`/`"footer"`/Screen footer area; `regions.screen`/`"screen"`/Main screen content region (published by every screen after init); `regions.side_panel`/`"side_panel"`/Inventory side panel region
## Settings DSL
### `minecraft.screen.settings({id, title, parent_screen?, parent_region?, button_label?, values?, sliders?, toggles?, on_change?, on_save?, on_reset?, priority?})`
Creates a complete settings screen with auto-generated UI. Returns the `open` function.
The function: Attaches a button to `parent_screen`'s `parent_region` (default `regions.footer`) using `add_stacked_centered_button` Creates a Lua screen with id `id` and auto-laid-out sliders/toggles Fits controls to the current screen height and splits overflow across pages, with **Previous** and **Next** buttons when needed Handles slider dragging, toggle clicks, saving, and closing
Set `parent_screen = minecraft.screen.ids.mod_settings` to place the generated page under **Mod Pages** automatically. This is the normal path for categorized or multi-page mod settings; no hand-built screen-navigation glue is required.
`id`/string/Screen ID (required); `title`/string/Screen title; `parent_screen`/string/Screen ID to attach the settings button to; `parent_region`/string/Region on parent screen (default `"footer"`); `button_label`/string/Label for the parent button; `values`/table or function/Mutable table of current values (key→value), or a function returning it; `sliders`/array of tables/Each: `{key, label?, min, max, integer?, format?}`; `toggles`/array of tables/Each: `{key, label?}`; `on_change`/function/Called when any value changes; `on_save`/function/Called when the generated screen closes (Done or Escape); `on_reset`/function/If set, shows a "Reset to Defaults" button; `priority`/number/Event priority (default 100)
Each slider entry:
`key`/string/Key in `values()` table; `label`/string/Display label (defaults to key); `min`/number/Minimum value; `max`/number/Maximum value; `integer`/boolean/Snap to integer values; `format`/function/Custom formatting: `format(value)` → string
Each toggle entry:
`key`/string/Key in `values()` table; `label`/string/Display label (defaults to key)
```lua
local config = {
  volume = 0.8,
  fullscreen = false,
  fov = 70,
}

minecraft.screen.settings({
  id = "my_mod:settings",
  title = "My Mod Settings",
  parent_screen = minecraft.screen.ids.mod_settings,
  button_label = "My Mod",
  values = config,
  sliders = {
    {key="volume", label="Volume", min=0, max=1},
    {key="fov", label="FOV", min=30, max=120, integer=true, format=function(v) return "FOV: "..v end},
  },
  toggles = {
    {key="fullscreen", label="Fullscreen"},
  },
  on_save = function()
    minecraft.storage.write("my_mod_config.json", minecraft.util.json_encode(config))
  end,
  on_reset = function()
    config.volume = 0.8
    config.fullscreen = false
    config.fov = 70
  end,
})
```
## Session API
### `minecraft.session.set_offline_username(name)`
Sets the offline-mode username override. This changes the name the client presents to unauthenticated servers without modifying engine internals.
```lua
minecraft.session.set_offline_username("MyModPlayer")
```
### `minecraft.session.clear_offline_username()`
Clears the offline username override, reverting to the default behavior.
```lua
minecraft.session.clear_offline_username()
```
### `minecraft.session.is_offline_mode()`
Returns `true` if an offline username override is currently active.
```lua
if minecraft.session.is_offline_mode() then
  print("Playing offline as:", minecraft.session.get_offline_username())
end
```
### `minecraft.session.get_offline_username()`
Returns the current offline username override string (may be empty if not set).
### `minecraft.session.get_username()`
Returns the **live session username** from the Minecraft client (the authenticated username or last-used name), regardless of offline override.
```lua
print("Logged in as:", minecraft.session.get_username())
```
### `minecraft.session.is_authenticated()`
Returns `true` if the client has a valid Microsoft authentication session.
```lua
if not minecraft.session.is_authenticated() then
  print("Not authenticated with Microsoft")
end
```

# Inventory, audio, utilities
## `minecraft.inventory.*`
All inventory functions operate on the **local player's inventory** (single-player client included). Functions that mutate inventory require a valid local player; if the player is unavailable (e.g. not in-world), mutation returns `false` and reads return `nil`.
### Slot layout
The inventory has 40 slots total: **Slots 0–35**: Main inventory (36 slots) **Slots 36–39**: Armor slots (4 slots: boots, leggings, chestplate, helmet)
Accessing a slot outside this range returns `nil` / `false`.
### `minecraft.inventory.slot_count()`
Returns the total number of inventory slots: `40` (36 main + 4 armor).
```lua
local count = minecraft.inventory.slot_count() -- 40
```
### `minecraft.inventory.main_size()`
Returns the size of the main inventory: `36`.
```lua
local main = minecraft.inventory.main_size() -- 36
```
### `minecraft.inventory.get(slot)`
Returns a table describing the item stack at `slot`. An empty slot is returned as `{id=0, count=0, damage=0}`; an invalid/out-of-range slot returns `nil`.
The returned table:
`id`/number/Numeric item ID; `count`/number/Stack size; `damage`/number/Item damage/durability value; `max_damage`/number/Maximum durability (only present if non-empty); `damageable`/boolean/Whether the item can take damage (only present if non-empty); `stackable`/boolean/Whether the item stacks (only present if non-empty); `has_subtypes`/boolean/Whether the item has subtypes (only present if non-empty); `max_count`/number/Maximum stack size (only present if non-empty)
```lua
local stack = minecraft.inventory.get(0)
if stack and stack.id ~= 0 then
  print(stack.id, stack.count, stack.damage)
end
```
### `minecraft.inventory.set(slot, {id, count?, damage?})`
Sets the item stack at `slot`. Returns `true` on success, `false` if the slot is invalid or the player is unavailable.
The item spec table accepts `id` (required), optional `count` (default 1), and optional `damage` (default 0).
```lua
local ok = minecraft.inventory.set(0, {id=264, count=1})     -- place diamond in first slot
local ok = minecraft.inventory.set(36, {id=310, count=1, damage=0}) -- diamond helmet in armor slot
```
### `minecraft.inventory.cursor_get()`
Returns the item stack currently held on the cursor (being dragged from a container slot). An empty cursor is returned as an empty stack table with `id=0`, `count=0`, and `damage=0`; unavailable inventory returns `nil`.
```lua
local cursor = minecraft.inventory.cursor_get()
if cursor then
  print("Dragging:", cursor.id)
end
```
### `minecraft.inventory.cursor_set({id, count?, damage?})`
Sets the cursor stack. Returns `true` on success, `false` if the player is unavailable.
```lua
minecraft.inventory.cursor_set({id=264, count=1})
```
### `minecraft.inventory.give({id, count?, damage?})`
Gives an item stack to the player. The item is placed into any available slot (fitting into existing stacks first, then empty slots); any overflow is dropped into the world. For an available player this usually returns `true`, not a signal that every item fit. It returns `false` if the player is unavailable.
```lua
local ok = minecraft.inventory.give({id=264, count=5}) -- give 5 diamonds
```
### `minecraft.inventory.offer({id, count?, damage?})`
Offers an item stack to the player. Returns the **remainder** stack (items that could not be placed) as a table in the same format as `get()`. If the entire stack was accepted, the remainder is empty (count will be 0, but the table is non-nil). If the player is unavailable, the original input stack is returned as the remainder.
```lua
local remainder = minecraft.inventory.offer({id=264, count=100})
if remainder.count > 0 then
  print("Could not fit", remainder.count, "diamonds")
end
```
## `minecraft.items.*`
### `minecraft.items.ids()`
Returns an array of all registered item numeric IDs. Items that are `nil` in the engine's ITEMS array are skipped.
```lua
local ids = minecraft.items.ids()
for _, id in ipairs(ids) do
  print("Item ID:", id)
end
```
### `minecraft.items.describe(item_id)`
Returns metadata about an item ID, or `nil` if the ID is invalid or the item is empty.
`id`/number/The item ID; `max_damage`/number/Maximum durability; `damageable`/boolean/Whether the item can take damage; `stackable`/boolean/Whether the item can stack; `has_subtypes`/boolean/Whether the item uses damage for subtypes; `max_count`/number/Maximum items per stack
```lua
local info = minecraft.items.describe(264)
if info then
  print("Diamond - max stack:", info.max_count, "damageable:", info.damageable)
end
```
## `minecraft.sound.*`
### `minecraft.sound.register(id, filepath, kind?)`
Registers a new sound effect with the audio engine. `id` is a string identifier, `filepath` is a resource path (resolved relative to the mod's asset directory). The optional `kind` determines how the sound is loaded. On failure it returns `false, error`.
``"effect"``: Short sound effect, loaded entirely into memory (default); ``"streaming"``: Longer sound, streamed from disk; ``"music"``: Background music track
Returns `false, error_message` on failure (missing file, unknown kind), or `true` on success.
```lua
local ok, err = minecraft.sound.register("my_mod:explode", "sounds/explode.ogg", "effect")
if not ok then print(err) end
```
### `minecraft.sound.play(id, volume?, pitch?)`
Plays a registered sound as a 2D (non-positional) effect. Default volume is 1.0, default pitch is 1.0. Returns `true` if the sound was started, `false` if the audio engine is unavailable.
```lua
minecraft.sound.play("my_mod:explode", 1.0, 1.0)
```
### `minecraft.sound.play_at(id, x, y, z, volume?, pitch?)`
Plays a registered sound at a 3D world position. Positional audio applies (distance attenuation, stereo panning). Default volume 1.0, default pitch 1.0. Returns `true` on success.
```lua
minecraft.sound.play_at("my_mod:explode", player_x, player_y, player_z, 1.0, 1.0)
```
### `minecraft.sound.play_loop_at(id, x, y, z, volume?, pitch?)`
Plays a registered sound as a looping 3D positional sound. Returns a **handle string** that can be passed to `stop()` to end the loop, or `nil` if the sound could not be started.
```lua
local handle = minecraft.sound.play_loop_at("my_mod:machine_hum", 100, 64, 100, 0.5, 1.0)
if handle then
  -- save handle for later
end
```
### `minecraft.sound.stop(handle)`
Stops a looping sound by its handle (returned from `play_loop_at`). The handle must be a non-empty string. Returns `true`.
```lua
if handle then
  minecraft.sound.stop(handle)
end
```
## JSON utilities (`minecraft.util.*`)
### `minecraft.util.json_encode(value)`
Encodes a Lua table to a JSON string. It accepts array-like tables, string-keyed tables, and the `minecraft.util.json_null` sentinel inside tables. The top-level argument must be a table; scalar values and top-level `nil` are rejected.
Returns `(json_string)` on success, or `(nil, error_message)` if the value is not JSON-serializable.
```lua
local json = minecraft.util.json_encode({name="Steve", health=20, items={1, 2, 3}})
-- {"name":"Steve","health":20,"items":[1,2,3]}
```
### `minecraft.util.json_decode(string)`
Decodes a JSON string to a Lua value. Supports: Objects → string-keyed tables Arrays → integer-indexed tables Strings, numbers (integers use `pushinteger`, floats use `pushnumber`) Booleans `null` → `minecraft.util.json_null` lightuserdata sentinel
Returns `(value)` on success, or `(nil, error_message)` on failure (empty input, invalid JSON, trailing characters).
```lua
local data, err = minecraft.util.json_decode('{"name":"Steve","health":20}')
if data then
  print(data.name, data.health)
end
```
### `minecraft.util.json_null`
A special sentinel value representing JSON `null`. Use this when you need to explicitly represent null in JSON output.
```lua
local encoded = minecraft.util.json_encode({value = minecraft.util.json_null})
-- {"value":null}

local decoded = minecraft.util.json_decode('{"value":null}')
-- decoded.value == minecraft.util.json_null
```
### `minecraft.util.resolve_seed(text)`
Resolves a seed string (numeric or textual) into a 64-bit integer, matching Minecraft's seed resolution logic. Returns the resolved integer.
```lua
local seed = minecraft.util.resolve_seed("12345")   -- 12345
local seed = minecraft.util.resolve_seed("hello")    -- numeric hash
```
