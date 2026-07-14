# 03 — API Functions

All functions are accessed through the global `minecraft` table injected into every Lua mod's sandbox. Mods also have `os.clock`, `os.date`, `os.difftime`, `os.time`, `math`, `string`, `table`, and the sandboxed `require`/`minecraft.require`. `io`, `debug`, `dofile`, `loadfile`, `package.cpath`, `package.loadlib`, and `package` itself are removed.

---

## Logging

### `minecraft.log(level?, message)`

Writes a line to stdout with the prefix `[lua-mod:<modId>:<level>]`.

- **`level`** — `"info"` (default), `"warn"`, or `"error"`.
- **`message`** — The text to log.

```lua
minecraft.log("info", "block placed")
minecraft.log("warn", "deprecated api used")
minecraft.log("error", "something broke")
minecraft.log("just a string")        -- level defaults to "info"
```

---

## Context

### `minecraft.is_client()`

Returns `true` if the current Lua execution context is on the client side (logical client, including integrated server). Server-side or dedicated-server execution returns `false`.

```lua
if minecraft.is_client() then
  print("running on client")
end
```

---

## Time

### `minecraft.time.utc_millis()`

Returns the current UTC epoch time in milliseconds as a number.

```lua
local now = minecraft.time.utc_millis()
```

---

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

- **Direct key names**: `"escape"`, `"1"`–`"0"`, `"q"`, `"w"`, `"e"`, `"r"`, `"t"`, `"y"`, `"u"`, `"i"`, `"o"`, `"p"`, `"enter"`, `"a"`, `"s"`, `"d"`, `"f"`, `"g"`, `"h"`, `"j"`, `"k"`, `"l"`, `"z"`, `"x"`, `"c"`, `"v"`, `"b"`, `"n"`, `"m"`, `"space"`, `"up"`, `"left_arrow"`, `"right_arrow"`, `"down"`
- **Binding names** (client only, reads the player's actual keybind): `"forward"` / `"move_forward"`, `"left"` / `"move_left"`, `"back"` / `"backward"` / `"move_back"`, `"right"` / `"move_right"`, `"jump"`, `"sneak"`, `"drop"`, `"inventory"`, `"chat"`, `"fog"`

If passed a number, it is returned as-is (useful pass-through).

```lua
local jumpKey = minecraft.key_code("jump")
local wKey = minecraft.key_code("w")
```

Convenience constants are also available on `minecraft.keys`:

```lua
minecraft.keys = { escape = 1, enter = 28, space = 57, up = 200, down = 208 }
```

---

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

---

## Session

### `minecraft.session.*`

Functions for reading and writing the runtime identity. All client-only.

| Function | Description |
|---|---|
| `set_offline_username(name)` | Sets the offline-mode username override |
| `clear_offline_username()` | Clears the offline-mode username override |
| `is_offline_mode()` | Returns `true` if an offline override is active |
| `get_offline_username()` | Returns the current offline username string |
| `get_username()` | Returns the live session's username (Mojang or offline) |
| `is_authenticated()` | Returns `true` if the session has valid Microsoft auth |

```lua
minecraft.session.set_offline_username("MyModPlayer")
print(minecraft.session.get_username())
```

---

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

Reads a file from the mod's persistent storage directory (`runDir/config/mods/<modId>/`). Returns the file content as a string, or `nil` if the file does not exist. Path traversal (`..`) is rejected.

```lua
local data = minecraft.storage.read("settings.txt")
```

### `minecraft.storage.write(path, content)`

Writes content to the mod's persistent storage directory. Creates intermediate directories as needed. Returns `true` on success, `false` on failure. Path traversal (`..`) is rejected.

```lua
minecraft.storage.write("scores.dat", "player1: 100")
```

---

## Config File Parser

### `minecraft.config.load(path, defaults, options?)`

Loads a key-value config file from the mod's storage directory. Each line is parsed as `key = value` or `key: value`. Lines starting with `#` or `;` are ignored.

| Parameter | Description |
|---|---|
| `path` | Relative path within the mod's storage directory |
| `defaults` | Table of default values (sets expected types and fallback values) |
| `options.aliases` | Table mapping old key names to current key names |
| `options.separator` | Custom separator (default `"="`) |

Returns `values, loaded` — `values` is a table with the parsed (or default) values, `loaded` is `true` if the file existed and was read.

The type of each default value controls parsing:
- **boolean** — parsed via `util.parse_boolean` (accepts `true`/`false`, `1`/`0`, `yes`/`no`, `on`/`off`)
- **number** — parsed via `tonumber`
- **string** — raw value (empty string preserves default)

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

| Parameter | Description |
|---|---|
| `path` | Relative path within the mod's storage directory |
| `values` | Table of key-value pairs to write |
| `options.keys` | Ordered array of keys to write (default: sorted keys) |
| `options.names` | Table mapping output key names (for aliasing) |
| `options.separator` | Custom separator (default `"="`) |

```lua
minecraft.config.save("graphics.cfg", {
  brightness = 0.8,
  enable_fog = false
}, {
  keys = {"brightness", "enable_fog"},
  separator = ":"
})
```

---

## Mod Lifecycle

### `minecraft.at_phase(phase_name, order, callback)`

Registers a callback to run during a specific lifecycle phase. The callback receives an event table with `previous` (previous phase string) and `current` (current phase string).

**Available phase names** (in order):

| Phase | Description |
|---|---|
| `"init"` | Register all content: blocks, items, entities, etc. |
| `"post_init"` | Resolve cross-references and register recipes |
| `"ready"` | All registration complete, game is live |

The phase name is case-insensitive. The `order` parameter controls execution order within a phase (lower numbers run first). `minecraft.lifecycle` is a table with all phase names as keys for convenience.

```lua
minecraft.at_phase("init", 100, function(event)
  print("phase ordinal:", event.current)
end)
```

### `minecraft.on(event_name, options, callback)`

Subscribes to a game event. The `options` table supports:

| Option | Description |
|---|---|
| `priority` | Integer priority (higher = runs later; default 0) |
| `once` | If `true`, unsubscribes after the first invocation |
| Any event field | Filter: the callback only fires if `event[field]` matches. Can be a literal value, a table of acceptable values, or a predicate function. |
| `when` | Function `(event) -> boolean` — fires only when it returns true |

The callback receives the event table and should return the (possibly mutated) event table.

```lua
minecraft.on("client_tick", {priority = 50}, function(event)
  -- called every client tick
  return event
end)

-- Filter by block_id and right_click
minecraft.on("block_interact", {
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
minecraft.on("entity_tick", {
  when = function(e) return e.entity_type == "Zombie" end
}, function(event)
  return event
end)
```

See the [event reference](#event-reference) section below for supported event names and their fields.

---

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

---

## JSON API

### `minecraft.util.json_encode(value)`

Encodes a Lua value to a JSON string. Supports `nil` → `null`, booleans, finite numbers, strings, and tables. Tables with consecutive integer keys from 1 are encoded as arrays; other tables with string keys are encoded as objects. Returns the JSON string on success, or `nil` + error on failure.

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

---

## NBT

NBT values read via `minecraft.read_nbt_asset` are automatically converted to Lua tables:

| NBT Type | Lua Representation |
|---|---|
| `Compound` | Table with string keys |
| `List` | Array table (1-indexed) |
| `Byte`, `Short`, `Int`, `Long` | Integer |
| `Float`, `Double` | Number |
| `String` | String |
| `ByteArray` | Binary string |
| `IntArray` | Array table of integers |
| `LongArray` | Array table of integers |
| `End` | `nil` |

Depth is limited to 256 levels.

To convert a Lua table back to NBT (e.g. for entity or tile entity data), the engine uses `luaValueToNbt` internally: booleans become bytes, integers become ints, floats become doubles, strings become strings, and tables with string keys become compounds. This conversion is used automatically for entity data, tile entity data, and similar write paths.

---

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

| Field | Type | Description |
|---|---|---|
| `id` | int | Numeric item ID |
| `max_damage` | int | Maximum damage value |
| `damageable` | bool | Whether the item can take damage |
| `stackable` | bool | Whether the item stacks |
| `has_subtypes` | bool | Whether the item uses metadata/damage for variants |
| `max_count` | int | Maximum stack size |

```lua
local info = minecraft.items.describe(256)
if info then
  print(info.id, info.max_count, info.damageable)
end
```

---

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

### `minecraft.world.get_top_y(x, z)`

Returns the Y coordinate of the top solid block at the given column, or `-1` if no world is available.

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

---

## Chunk (Generation)

### `minecraft.chunk.set_block(localX, y, localZ, blockId)`

Sets a block within the currently generating chunk. Coordinates are local to the chunk (0–15 for X and Z, 0–127 for Y). Only usable during `chunk_generation` event callbacks. Returns `true` on success.

```lua
minecraft.chunk.set_block(7, 40, 7, 1)  -- place stone
```

### `minecraft.chunk.fill(x1, y1, z1, x2, y2, z2, blockId)`

Fills a cuboid within the chunk with the given block ID. Coordinates are clamped to chunk bounds. Returns the number of blocks changed. Only usable during `chunk_generation`.

```lua
local changed = minecraft.chunk.fill(0, 0, 0, 15, 0, 15, 1)
```

### `minecraft.chunk.get_block(localX, y, localZ)`

Gets the block ID at a local chunk position. Returns 0 if no chunk context is active.

### `minecraft.chunk.get_height(localX, localZ)`

Gets the height value at a local chunk column. Returns 0 if no chunk context is active.

---

## Entities

### `minecraft.entities.list(filter?)`

Returns an array table of entity handle objects in the current world, optionally filtered by entity type string. Each entity handle object exposes properties and methods.

#### Properties

| Field | Description |
|---|---|
| `id` | Entity network ID |
| `type` | Entity type string (e.g. `"Zombie"`) |
| `registry_id` | (mod entities only) Registry ID string |
| `data` | (mod entities only) NBT data table |
| `x`, `y`, `z` | Position |
| `vx`, `vy`, `vz` | Velocity |
| `yaw`, `pitch` | Rotation |
| `on_ground` | Boolean ground state |
| `item_id`, `item_count`, `item_damage`, `item_max_damage` | (Item entities only) |
| `texture_path`, `mod_texture`, `atlas_index` | (Item entities only) |

#### Methods

- `entity:teleport(position | x, y, z, yaw?, pitch?)` — Teleports the entity. Accepts a table `{x, y, z, yaw?, pitch?}` or individual coordinate arguments. Returns `true` on success. Server-side only.
- `entity:apply_state(state)` — Applies state mutations. Accepts a state table with optional fields `x`, `y`, `z`, `vx`, `vy`, `vz`, `yaw`, `pitch`, and `data`. Returns `true` on success. Server-side only.
- `entity:remove()` — Marks the entity for removal. Returns `true` on success. Server-side only.

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

| Field | Description |
|---|---|
| `x`, `y`, `z` | Position |
| `yaw`, `pitch` | Rotation |
| `data` | Table to store as entity NBT data |

Returns the spawned entity handle object on success, or `nil` on failure.

Mod entities cast a vanilla blob shadow sized from their width. Put a
`shadow_radius` float in `data` to override it (`shadow_radius = 0` disables
the shadow).

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

---

## Particles

### `minecraft.particles.spawn(spec)`

Spawns a client-side particle. All fields are optional except the table itself.

| Field | Default | Description |
|---|---|---|
| `x`, `y`, `z` | `0, 64, 0` | Position |
| `vx`, `vy`, `vz` | `0, 0, 0` | Velocity |
| `scale` | `4.0` | Particle size (clamped `0.05`–`4.0`) |
| `r`, `g`, `b` | `1.0, 1.0, 1.0` | Color |
| `max_age` | `40` | Lifetime in ticks |
| `gravity` | `0.04` | Gravity strength |

```lua
minecraft.particles.spawn({
  x = 100, y = 70, z = 200,
  r = 1, g = 0, b = 0,
  max_age = 60,
  vx = 0, vy = 0.1, vz = 0
})
```

---

## Raycast

### `minecraft.raycast(spec?)`

Performs a raycast from the player's camera or a custom origin. If called without arguments, uses the player's camera position and look direction. The `spec` table accepts:

| Field | Description |
|---|---|
| `origin` / `origin_x/y/z` | Ray origin (default: camera position) |
| `direction` | Ray direction vector `{x, y, z}`, or `yaw`/`pitch` in degrees |
| `max_distance` / `reach` | Maximum ray distance (default: player reach or 5.0) |
| `ignore_liquids` | Whether to ignore liquid blocks (default `false`) |
| `blocks` | Whether to test blocks (default `true`) |
| `entities` | Whether to test entities (default `true`) |

Returns a table with the hit result, or `nil` if nothing was hit. Result fields:

| Field | Description |
|---|---|
| `type` | `"block"`, `"entity"`, or `"model"` |
| `hit_x`, `hit_y`, `hit_z` | Hit position |
| `block_x`, `block_y`, `block_z` | (block hits) Block position |
| `side` | (block hits) Face index |
| `block_id` | (block hits) Block ID |
| `block_name` | (block hits) Wire block name |
| `entity_id`, `entity_type` | (entity hits) Entity identity |
| `entity_x`, `entity_y`, `entity_z` | (entity hits) Hit entity position |
| `model_id`, `model_tag` | (model hits) Placed model instance info |

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

---

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

| Method | Returns | Description |
|---|---|---|
| `:get_id()` | string | Tile entity registry ID |
| `:get_block_id()` | int | Block ID at this position |
| `:get_block_meta()` | int | Block metadata at this position |
| `:is_removed()` | bool | Whether the entity was removed |
| `:mark_dirty()` | — | Marks the entity as needing saving (server only) |
| `:distance_from(x, y, z)` | number | Euclidean distance to point |
| `:get_world_time()` | number | Current world time in ticks |
| `:get_data()` | table | (mod tile entities) NBT data table |
| `:set_data(table)` | — | (mod tile entities, server only) Sets NBT data from a table |
| `:get_animation_frame()` | int | Current animation frame |
| `:set_animation_speed(speed)` | — | Sets animation speed multiplier |

```lua
local te = minecraft.tile_entities.get(100, 64, 200)
if te then
  print(te:get_id())
  print(te:get_block_id())
  local data = te:get_data()
  te:set_animation_speed(2.0)
end
```

---

## Inventory

### `minecraft.inventory.*`

Functions for reading and writing the player's inventory. Client-only.

| Function | Description |
|---|---|
| `slot_count()` | Total number of slots (main + armor) |
| `main_size()` | Number of main inventory slots |
| `get(slot)` | Returns item stack table for slot, or `nil` |
| `set(slot, stack)` | Sets slot to a stack, returns `true`/`false` |
| `cursor_get()` | Returns the cursor (held) item stack |
| `cursor_set(stack)` | Sets the cursor (held) item stack |
| `give(stack)` | Adds stack to inventory, returns `true`/`false` |
| `offer(stack)` | Like give but returns the remainder stack (what couldn't fit) |

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

---

## Sound

### `minecraft.sound.*`

| Function | Description |
|---|---|
| `register(id, path, kind?)` | Registers a sound. `kind` is `"effect"` (default), `"streaming"`, or `"music"`. Returns `true`/`false`. |
| `play(id, volume?, pitch?)` | Plays a registered sound globally |
| `play_at(id, x, y, z, volume?, pitch?)` | Plays a sound at a world position |
| `play_loop_at(id, x, y, z, volume?, pitch?)` | Plays a looping sound at a position, returns a handle string |
| `stop(handle)` | Stops a looping sound by handle |

```lua
minecraft.sound.register("mymod:boom", "resources/mymod/sounds/boom.ogg", "effect")
minecraft.sound.play("mymod:boom", 1.0, 1.0)
local handle = minecraft.sound.play_loop_at("mymod:hum", 100, 64, 200)
minecraft.sound.stop(handle)
```

---

## Screen

### `minecraft.screen.*`

Functions for opening and managing custom Lua screens.

| Function | Description |
|---|---|
| `open(id, options?)` | Opens a Lua screen. `options` may contain `{title = "...", pause = true}`. |
| `close()` | Closes the current screen |
| `open_host(screenId, fields?)` | Opens a vanilla host screen by ID, with optional field values |
| `host_field(name)` | Returns the value of a host screen field |
| `host_set_field(name, value)` | Sets a host screen field |
| `add_field(name, x, y, w, h, options?)` | Adds a text field to the current Lua screen (init phase only). `options`: `{text, max_len, numeric, signed, decimal}` |
| `field_text(name)` | Gets a field's text |
| `set_field_text(name, text)` | Sets a field's text |
| `add_button(x, y, w, h, text, callback?)` | Adds a button (init phase only) |
| `set_fields_visible(visible)` | Shows/hides all text fields |

```lua
minecraft.screen.open("mymod:config", {title = "My Config"})
```

### Screen ID Constants (`minecraft.screen.ids`)

Pre-defined screen ID constants: `login`, `title`, `game_menu`, `multiplayer`, `connect`, `disconnected`, `downloading_terrain`, `death`, `chat`, `sleeping_chat`, `confirm`, `create_world`, `select_world`, `edit_world`, `world_settings`, `world_save_conflict`, `inventory`, `crafting`, `dispenser`, `double_chest`, `furnace`, `sign_edit`, `options`, `video_options`, `detail_settings`, `keybinds`, `mods`, `achievements`, `stats`, `lan`, `lan_info`, `server_mod_download`, `fatal_error`, `out_of_memory`.

### Screen Region Constants (`minecraft.screen.regions`)

- `footer`
- `screen`
- `side_panel`

### Screen Convenience Functions

`minecraft.screen.on_ui(screen_id, region, callback, priority?)` — shorthand for subscribing to `screen_ui` with a specific screen/region filter.

`minecraft.screen.on_lua_screen(screen_id, handlers, priority?)` — shorthand for subscribing to `screen_event` by screen ID, dispatching `{init, render, tick, key, mouse, scroll, close}` to handler functions.

`minecraft.screen.settings(spec)` — Creates a mod settings screen. See the prelude for full details; supports sliders, toggles, and auto-layout.

---

## GUI Drawing

### `minecraft.gui.*`

GUI drawing functions — only usable inside `screen_event` render phase or `screen_region` render phase contexts.

| Function | Description |
|---|---|
| `fill_rect(x, y, w, h, argb)` | Draws a filled rectangle |
| `draw_text(x, y, text, argb)` | Draws text |
| `draw_centered_text({x, y, width/w, text, color?})` or `(x, y, width, text, color?)` | Draws centered text |
| `draw_item(x, y, itemId, count, damage?)` | Draws an item icon |
| `text_width(text)` | Returns the pixel width of rendered text |
| `texture_id(path)` | Returns the OpenGL texture ID for a resource path |
| `draw_sprite(path/id, x, y, u, v, w, h)` | Draws a sprite from a texture atlas |
| `draw_texture(textureId, x, y, w, h)` | Draws a full texture quad |
| `draw_button({x, y, width, height, text, active?, mouse_x?, mouse_y?})` | Draws a vanilla-style button |
| `draw_slider({x, y, width, height, value, text, mouse_x?, mouse_y?})` | Draws a vanilla-style slider |
| `draw_toggle({x, y, width, height, label, value, mouse_x?, mouse_y?})` | Draws a vanilla-style toggle |
| `begin_3d({x, y, width, height, size?, gui_width?, gui_height?, yaw_deg?, pitch_deg?, distance?, fov_deg?, clear_color?, clear_r/g/b/a?})` | Begins a 3D viewport for GUI rendering |
| `end_3d()` | Ends a 3D viewport |
| `draw_3d({mode = "lines"|"quads"|..., color?, line_width?, point_size?, vertices = {{x,y,z}, ...}})` | Draws 3D geometry inside a viewport |
| `unproject({x, y, width, height, ..., mouse_x, mouse_y, ...})` | Computes a 3D ray from mouse position in a viewport. Returns `{origin = {x, y, z}, direction = {x, y, z}}` |

```lua
-- Draw a button
minecraft.gui.draw_button({
  x = 10, y = 10, width = 100, height = 20,
  text = "Click", mouse_x = event.mouse_x, mouse_y = event.mouse_y
})

-- 3D viewport
minecraft.gui.begin_3d({
  x = 0, y = 0, width = 200, height = 200,
  yaw_deg = 45, pitch_deg = 30, distance = 3
})
minecraft.gui.draw_3d({
  mode = "quads",
  r = 1, g = 0, b = 0,
  vertices = {
    {x = -0.5, y = -0.5, z = 0},
    {x =  0.5, y = -0.5, z = 0},
    {x =  0.5, y =  0.5, z = 0},
    {x = -0.5, y =  0.5, z = 0}
  }
})
minecraft.gui.end_3d()
```

---

## Camera (Framebuffer Targets)

### `minecraft.camera.*`

Controls offscreen framebuffer objects for rendering the world to textures (viewfinder / render-to-texture).

| Function | Description |
|---|---|
| `create(width, height, colorCount?, useDepthTex?)` | Creates a camera target, returns handle or `-1` |
| `create_display_size(colorCount?, useDepthTex?)` | Creates a camera target matching the display size |
| `destroy(handle)` | Destroys a target, returns `true`/`false` |
| `resize(handle, width, height, colorCount?)` | Resizes a target |
| `width(handle)` | Returns pixel width |
| `height(handle)` | Returns pixel height |
| `render(handle, x, y, z, yaw, pitch, roll, fov, tickDelta?)` | Renders the world into the target from the given camera position. Returns `true`/`false` |
| `unbind()` | Unbinds the active framebuffer (returns to default) |
| `texture(handle, attachmentIndex?)` | Returns the OpenGL texture ID for a color attachment |
| `rendering()` | Returns the handle of the currently-rendering target, or `-1` |

```lua
local cam = minecraft.camera.create(256, 256)
if cam > 0 then
  minecraft.camera.render(cam, 0, 80, 0, 0, -30, 0, 70)
  local texId = minecraft.camera.texture(cam)
  -- use texId with gui.draw_texture or model/procedural rendering
  minecraft.camera.destroy(cam)
end
```

---

## FBO (Offscreen Framebuffers)

### `minecraft.fbo.*`

General-purpose offscreen framebuffer objects for custom render passes and shader work.

| Function | Description |
|---|---|
| `create(width, height, colorCount?, useDepthTex?)` | Creates an FBO, returns handle or `-1` |
| `create_display_size(colorCount?, useDepthTex?)` | Creates an FBO matching display size |
| `destroy(handle)` | Destroys an FBO |
| `resize(handle, width, height, colorCount?)` | Resizes an FBO |
| `bind(handle)` | Binds an FBO for rendering, returns `true`/`false` |
| `unbind()` | Unbinds the active FBO |
| `texture(handle, attachmentIndex?)` | Returns the OpenGL texture ID |
| `width(handle)` | Returns pixel width |
| `height(handle)` | Returns pixel height |
| `bound()` | Returns the currently bound FBO handle, or `-1` |

---

## Render (World-Space Drawing)

### `minecraft.render.*`

Low-level world-space drawing functions, only usable during world render events (`world_render`) or chunk context callbacks.

| Function | Description |
|---|---|
| `quads({texture?, texture_id?, blend?, cull?, depth_test?, depth_write?, r?, g?, b?, a?, x?, y?, z?, yaw?, pitch?, roll?, scale?, world_space?, vertices = {{x, y, z, u?, v?, r?, g?, b?, a?}, ...}})` | Draws textured/colored quads in world space. Returns number of quads emitted. |
| `billboards({brightness?, rotation_x_rad?, rotation_y_rad?, blend?, depth_test?, depth_write?, billboards/points = {{x, y, z, size, alpha}, ...}})` | Draws billboard sprites. Returns count. |
| `set_item_entity_override(enabled)` | When enabled, overrides item entity rendering with mod models. Call with `false` to disable. |

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

| Function | Description |
|---|---|
| `quad({texture?, texture_id?, r?, g?, b?, a?, vertices = {{x, y, z, u, v}, ...}})` | Emits a single quad into the manual block model buffer. Returns `true`/`false`. |

---

## Texture Queries

### `minecraft.texture.*`

| Function | Description |
|---|---|
| `size(path)` | Returns `{width = N, height = N}` for a texture resource |
| `pixel(path, x, y)` | Returns `{a, r, g, b}` (0–255) for the pixel at (x, y) |

```lua
local size = minecraft.texture.size("textures/blocks/stone.png")
print(size.width, size.height)
local px = minecraft.texture.pixel("textures/blocks/stone.png", 0, 0)
```

---

## Model API

### `minecraft.model.*`

Functions for loading, building, placing, and drawing baked 3D models.

| Function | Description |
|---|---|
| `load(path)` | Loads a baked model file from the mod's assets. Returns a handle integer, or `nil, error`. |
| `build({quads = {...}, key?})` | Builds a model from quad data at runtime. Returns a handle. Quad format: `{texture?, r, g, b, a?, shade?, vertices = {v1, v2, v3, v4}}`. Each vertex: `{x, y, z, u, v}`. The `key` string enables caching (subsequent calls with the same key return the existing handle). |
| `voxels({cells, resolution, origin_x/y/z, scale, key})` | Voxel model builder. Each cell: `{x, y, z, r?, g?, b?, a?}`. Interior faces shared by adjacent cells are automatically culled. Returns a model handle. |
| `voxel({texture, atlas_index?, mod_texture?, grid?, alpha_cutoff?})` | Samples a sprite texture and extrudes it into a one-voxel-thick model. `grid` controls sampling resolution (default 16). `alpha_cutoff` (default 30) controls the transparency threshold. Results are cached. |
| `place(handle, opts)` | Places a model instance in the world (hitbox-enabled, raycastable). `opts`: `{x, y, z, yaw?, pitch?, roll?, scale?, pivot_y?, tag?}`. Returns an instance ID. |
| `update(instanceId, opts)` | Updates a placed model instance's transform. |
| `remove(instanceId)` | Removes a placed model instance. |
| `clear()` | Removes all model instances owned by the current mod. |
| `draw(handle, opts)` | Draws a baked model in world space during a world render event. `opts`: `{x, y, z, yaw?, pitch?, roll?, scale?, pivot_y?, brightness?, a?, blend?, cull?, depth_test?, depth_write?}`. Returns `true`/`false`. |
| `draw_item(itemId, damage, opts)` | Draws a real 3D model for an item (mod or vanilla block). Returns `false` for flat sprite items. |
| `item_bounds(itemId, damage)` | Returns the bounding box `{min_x, min_y, min_z, max_x, max_y, max_z}` of an item's 3D model, or `nil` if the item has no 3D shape. |
| `bounds(handle)` | Returns the bounding box of a baked model. |

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

---

## File Dialogs (Client Only)

### `minecraft.files.*`

| Function | Description |
|---|---|
| `pick(options?)` | Opens a file picker dialog. `options` can be a string extension filter (e.g. `"json"`, `".png"`) or a table `{extension = "..."}`. Returns the selected file path or `nil`. |
| `read(path)` | Reads an external file by absolute path. Also resolves mod-bundled resources. Returns content string or `nil, error`. |

```lua
local path = minecraft.files.pick("json")
if path then
  local data = minecraft.files.read(path)
end
```

---

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

---

## Seed Resolution

### `minecraft.util.resolve_seed(text)`

Resolves a textual seed to its numeric value (supports numeric strings and named seeds).

```lua
local seed = minecraft.util.resolve_seed("myworld")
```

---

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

---

## World Grid Sampling

### `minecraft.world.sample_grid(seed, centerX, centerZ, options?)`

Samples terrain/biome data into a grid array for minimap or visualization use. The `options` table accepts:

| Field | Default | Description |
|---|---|---|
| `radius_chunks` / `radius` | `6` | Radius in chunks (clamped 1–4096) |
| `max_side` | `48` | Maximum samples per side (clamped 8–256) |
| `channel` | `"grass"` | Primary data channel |
| `channels` | `{channel}` | Array of channel names to sample (max 8) |
| `mod_generation` | `false` | Enable mod generation hooks during sampling |

Supported channels: `"height"`, `"surface_block"`, `"surface_block_below"`, `"biome_id"`, `"grass"` (grass color as ARGB).

Returns a table with `side`, `step`, `origin_x`, `origin_z`, `center_x`, `center_z`, `channel`, `values` (primary channel array), and per-channel fields.

---

## Event Reference

All supported event names for `minecraft.on()`:

| Event | Key Fields | Read/Write Fields |
|---|---|---|
| `client_tick` | `before`, `after_world`, `paused`, `has_player`, `has_world`, `world_name`, `is_overworld`, `camera_y`, `player_y`, `player_fall_distance`, `player_on_ground`, `world_time`, `is_night`, `mod_generation` | — |
| `render_frame` | `tick_delta` | — |
| `render_targets` | `tick_delta` | — |
| `first_person_hand` | `tick_delta`, `eye`, `canceled`, `entity_id`, `entity_type` | `canceled` |
| `key_press` | `key`, `pressed`, `repeat`, `handled` | `handled` |
| `mouse_button` | `button`, `pressed`, `handled` | `handled` |
| `raycast` | `has_hit`, `type`, `hit_x/y/z`, `block_x/y/z`, `side`, `block_id`, `block_name`, `item_id`, `entity_id`, `entity_type` | — |
| `fov` | `tick_delta`, `fov` | `fov` |
| `camera_setup` | `tick_delta`, `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `custom_view`, `hide_first_person_hand` | `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `custom_view`, `hide_first_person_hand` |
| `player_travel` | `sideways`, `forward`, `speed_multiplier`, `has_player`, `is_local_player` | `sideways`, `forward`, `speed_multiplier` |
| `tick_rate` | `target_tps`, `tps_scale` | `target_tps`, `tps_scale` |
| `world_start` | `save_name`, `new_world` | — |
| `world_open` | `save_name`, `new_world`, `options` (table) | — |
| `world_tick` | `remote`, `before` | — |
| `entity_tick` | `remote`, `canceled`, `entity_id`, `entity_type`, `x`, `y`, `z`, `yaw`, `pitch` | `canceled` |
| `tile_entity_tick` | `x`, `y`, `z`, `id`, `remote`, `removed`, `canceled`, `world_time`, `animation_frame`, `animation_tick`, `animation_speed`, `entity` | `canceled`, `animation_speed` |
| `create_world` | `save_name`, `seed`, `canceled`, `options` (table) | `canceled`, `options` |
| `block_interact` | `x`, `y`, `z`, `block_id`, `side`, `right_click`, `remote`, `canceled`, `handled`, `has_player`, `local_player`, `has_item`, `player_x/y/z`, `player_yaw/pitch`, `item_id/count/damage/max_damage/damageable` | `canceled`, `handled`, `item_count`, `item_damage` |
| `entity_interact` | `attack`, `remote`, `canceled`, `handled`, `sneaking`, `has_player`, `local_player`, `has_target`, `player_yaw/pitch`, `has_item`, `item_id/count/damage`, `entity_id`, `entity_type`, `target_id` | `canceled`, `handled`, `item_count` |
| `attack_damage` | `damage`, `critical`, `canceled`, `fall_distance`, `on_ground`, `target_x/y/z`, `has_player`, `has_target` | `damage`, `critical`, `canceled` |
| `entity_teleport` | `entity_id`, `entity_type`, `from_x/y/z`, `x`, `y`, `z`, `yaw`, `pitch`, `canceled`, `has_entity`, `has_player` | `x`, `y`, `z`, `yaw`, `pitch`, `canceled` |
| `world_color` | `partial_ticks`, `r`, `g`, `b`, `kind`, `celestial`, `world_time`, `is_night` | `r`, `g`, `b` |
| `entity_render` | `entity_id`, `entity_type`, `is_player`, `tick_delta`, `pose` (sub-table with `body_yaw`, `head_yaw/pitch`, `yaw`, `pitch`, `roll`, `scale`, `offset_x/y/z`, `parts`) | `pose` (full mutation) |
| `world_render` | `tick_delta`, `stage`, `moment`, `cancel_vanilla`, `vanilla_stage_ran`, `celestial_angle`, `sky_yaw_deg`, `star_brightness`, `rain_strength`, `stars_enabled`, `astronomy_enabled`, `astronomy_utc_millis`, `observer_lat/lon_deg`, `camera_x/y/z`, `camera_yaw/pitch/roll`, `custom_camera`, `world_time`, `celestial`, `is_night`, `cloud_base_height` | `cancel_vanilla`, celestial/sky/astronomy fields (sky stage only) |
| `chunk_generation` | `stage`, `moment`, `cancel_vanilla`, `vanilla_stage_ran`, `world_seed`, `mod_generation`, `is_overworld`, `chunk_x`, `chunk_z`, `has_chunk` | `cancel_vanilla` |
| `screen_region` | `phase_name`, `screen_id`, `region`, `mouse_x`, `mouse_y`, `button`, `scroll_delta`, `x`, `y`, `width`, `height`, `handled` | `handled`, `width`, `height` |
| `screen_ui` | `screen_id`, `region`, `host_fields` (table), `ui` (table with `add_centered_button`, `add_button`, `add_stacked_centered_button`) | — |
| `screen_event` | `screen_id`, `phase`, `width`, `height`, `mouse_x`, `mouse_y`, `tick_delta`, `key`, `char`, `button`, `released`, `delta`, `handled` | `handled` |
| `world_spawn_search` | `x`, `y`, `z`, `resolved` | `x`, `y`, `z`, `resolved` |
| `pre_entity_render` | `entity_id`, `entity_type`, `tick_delta`, `canceled`, item fields | `canceled` |
| `pre_tile_entity_render` | `x`, `y`, `z`, `id`, `tick_delta`, `canceled` | `canceled` |
| `entity_spawn` | `entity_id`, `entity_type`, item fields | — |
| `entity_remove` | `entity_id`, `entity_type`, item fields | — |

All events have `remote` (boolean) and `side` (`"client"` or `"server"`) set automatically. World events also have `has_world`, `world_name`, `is_overworld`, and `mod_generation` fields.

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
