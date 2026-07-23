# 05 — World and generation

## `minecraft.world.*`

### `minecraft.world.block_id(name)`
Look up the numeric block ID by name. Supports both vanilla and mod-added blocks.

| Param | Type | Description |
|-------|------|-------------|
| `name` | string | Block identifier (e.g. `"stone"`, `"grass_block"`, `"mod:custom_block"`) |

**Returns:** integer — numeric block ID, or `0` if not found.

```lua
local stone = minecraft.world.block_id("stone")       -- 1
local grass = minecraft.world.block_id("grass_block")  -- 2
local custom = minecraft.world.block_id("mymod:foo")   -- 0 if not registered
local unknown = minecraft.world.block_id("nonexistent") -- 0
```

---

### `minecraft.world.get_block(x, y, z)`
Get the numeric block ID at the given world position in the active world.

| Param | Type | Description |
|-------|------|-------------|
| `x` | int | World X coordinate |
| `y` | int | World Y coordinate |
| `z` | int | World Z coordinate |

**Returns:** integer — block ID at the position, or `0` if out of bounds or no world.

```lua
local block = minecraft.world.get_block(100, 64, 200)
```

---

### `minecraft.world.random(bound?)`
Generate a world-scoped random integer. Uses the active world's random number generator for deterministic/reproducible sequences tied to the world seed.

| Param | Type | Default | Description |
|-------|------|---------|-------------|
| `bound` | int (optional) | `1000` | Upper bound (exclusive). Values `<= 0` return `0`. |

**Returns:** integer — random value in `[0, bound)`.

```lua
local r1 = minecraft.world.random()       -- random in [0, 1000)
local r2 = minecraft.world.random(10)     -- random in [0, 10)
```

---

### `minecraft.world.is_night()`
Check whether the active world is currently in night time.

**Returns:** boolean — `true` if world time is between 13000 and 23000 ticks (inclusive-exclusive), `false` otherwise.

```lua
if minecraft.world.is_night() then
  -- spawn mobs
end
```

---

### `minecraft.world.get_top_y(x, z)`
Get the Y coordinate immediately above the highest solid or fluid block at the given column.

| Param | Type | Description |
|-------|------|-------------|
| `x` | int | World X coordinate |
| `z` | int | World Z coordinate |

**Returns:** integer — top block Y + 1, or `-1` if no active world.

```lua
local top = minecraft.world.get_top_y(100, 200)
```

---

### `minecraft.world.get_heightmap(x, z, width, height)`
Get a packed ARGB heightmap for loaded columns. The red channel contains the highest translucent block height, the green channel contains its light opacity, and the blue channel contains the highest opaque block height. Unloaded columns are returned as zero heights.

| Param | Type | Description |
|-------|------|-------------|
| `x` | int | World X coordinate of the first column |
| `z` | int | World Z coordinate of the first column |
| `width` | int | Number of columns along X, clamped to 1–512 |
| `height` | int | Number of columns along Z, clamped to 1–512 |

**Returns:** integer array in row-major order, or `nil` if no world is active.

```lua
local pixels = minecraft.world.get_heightmap(0, 0, 128, 128)
```

---

### `minecraft.world.player()`
Get the position of the active player.

**Returns:** table `{x, y, z}` containing the player's world coordinates, or `nil` if no player is available.

```lua
local pos = minecraft.world.player()
if pos then
  print("Player at:", pos.x, pos.y, pos.z)
end
```

---

### `minecraft.world.spawn_entity(entity_type, {x,y,z} or x,y,z)`
Spawn an entity of the given type at the specified position. Works server-side only (returns `false` on the client).

| Param | Type | Description |
|-------|------|-------------|
| `entity_type` | string | Entity type identifier (e.g. `"Zombie"`, `"Creeper"`, `"Item"`) |
| position | table or vararg | Either `{x, y, z}` table or individual `x, y, z` arguments. Default Y is `64`. |

**Returns:** boolean — `true` if the entity was spawned successfully.

```lua
-- Using a table
minecraft.world.spawn_entity("Zombie", {x=100, y=64, z=200})

-- Using individual arguments
minecraft.world.spawn_entity("Creeper", 100, 64, 200)
```

---

### `minecraft.world.count_entities(entity_type)`
Count the number of entities of a given type in the active world.

| Param | Type | Description |
|-------|------|-------------|
| `entity_type` | string | Entity type identifier |

**Returns:** integer — entity count.

```lua
local zombieCount = minecraft.world.count_entities("Zombie")
```

---

### `minecraft.world.set_time(tick)`
Set the world time.

| Param | Type | Description |
|-------|------|-------------|
| `tick` | int | Time in ticks (0–24000). Must be a number. |

**Returns:** boolean — `true` if the time was set successfully (server-side only; `false` on client).

```lua
minecraft.world.set_time(0)    -- dawn
minecraft.world.set_time(6000) -- noon
minecraft.world.set_time(18000) -- midnight
```

---

### `minecraft.world.marker_px(grid, world_x, world_z)**
Convert world coordinates to grid pixel coordinates (for minimaps, chunk markers, etc.). This is a Lua helper defined in the runtime prelude.

| Param | Type | Description |
|-------|------|-------------|
| `grid` | table | Grid descriptor with fields: `side` (pixel dimension), `step` (blocks per pixel), `origin_x`, `origin_z` (world origin offset) |
| `world_x` | number | World X coordinate |
| `world_z` | number | World Z coordinate |

**Returns:** `col, row` — clamped pixel coordinates in `[0, side-1]`, or `0, 0` if grid is invalid.

```lua
-- Given a grid from world generation (sample or similar)
local col, row = minecraft.world.marker_px(grid, entity_x, entity_z)
```

---

## `ChunkHandle`

During chunk generation events, the local chunk being generated is exposed via the `event.chunk` object. The chunk handle provides the following methods (all of which require colon notation):

### `chunk:set_block(localX, y, localZ, blockId)`
Set a block in the chunk. Coordinates are local to the chunk (0–15 for X and Z, 0–255 for Y).

| Param | Type | Description |
|-------|------|-------------|
| `localX` | int | Chunk-local X coordinate (0–15) |
| `y` | int | World Y coordinate (0–255) |
| `localZ` | int | Chunk-local Z coordinate (0–15) |
| `blockId` | int | Numeric block ID to place |

**Returns:** boolean — `true` if the block was placed successfully.

```lua
event.chunk:set_block(7, 64, 7, minecraft.world.block_id("stone"))
```

---

### `chunk:fill(x1, y1, z1, x2, y2, z2, blockId)`
Fill a rectangular region in the chunk with the given block. Coordinates are automatically clamped to chunk bounds.

| Param | Type | Description |
|-------|------|-------------|
| `x1` | int | First corner X (clamped 0–15) |
| `y1` | int | First corner Y (clamped 0–255) |
| `z1` | int | First corner Z (clamped 0–15) |
| `x2` | int | Opposite corner X (clamped 0–15) |
| `y2` | int | Opposite corner Y (clamped 0–255) |
| `z2` | int | Opposite corner Z (clamped 0–15) |
| `blockId` | int | Numeric block ID to fill with |

**Returns:** integer — count of blocks successfully changed.

```lua
-- Fill a 5x5x5 area with stone
local count = event.chunk:fill(5, 60, 5, 10, 64, 10, minecraft.world.block_id("stone"))
```

---

### `chunk:get_block(localX, y, localZ)`
Get the block ID at a chunk-local position in the chunk.

| Param | Type | Description |
|-------|------|-------------|
| `localX` | int | Chunk-local X (0–15) |
| `y` | int | World Y (0–255) |
| `localZ` | int | Chunk-local Z (0–15) |

**Returns:** integer — block ID, or `0` if out of bounds.

```lua
local block = event.chunk:get_block(7, 64, 7)
```

---

### `chunk:get_height(localX, localZ)`
Get the height (top solid block Y + 1) at the given column in the chunk.

| Param | Type | Description |
|-------|------|-------------|
| `localX` | int | Chunk-local X (0–15) |
| `localZ` | int | Chunk-local Z (0–15) |

**Returns:** integer — height value, or `0` if out of bounds.

```lua
local h = event.chunk:get_height(7, 7)
```

---

## `minecraft.entities.*`

### `minecraft.entities.list(filter?)`
List all entities in the world with their full state.

| Param | Type | Description |
|-------|------|-------------|
| `filter` | string (optional) | If provided, only returns entities whose type or registry_id matches this string |

**Returns:** array of entity state tables, each containing:

| Field | Type | Description |
|-------|------|-------------|
| `id` | int | Entity network ID |
| `type` | string | Entity type (e.g. `"Zombie"`, `"Item"`) |
| `registry_id` | string | *(mod entities only)* Registry identifier (e.g. `"mymod:custom"`) |
| `data` | table | *(mod entities only)* Custom NBT data as a Lua table |
| `x`, `y`, `z` | double | Position |
| `vx`, `vy`, `vz` | double | Velocity |
| `yaw` | float | Yaw rotation |
| `pitch` | float | Pitch rotation |
| `on_ground` | boolean | Whether the entity is on the ground |
| `item_id` | int | *(Item entities only)* Item ID |
| `item_count` | int | *(Item entities only)* Stack count |
| `item_damage` | int | *(Item entities only)* Item damage |
| `item_max_damage` | int | *(Item entities only)* Maximum item damage |
| `texture_path` | string | *(Item entities only)* Texture path for the item |
| `mod_texture` | boolean | *(Item entities only)* Whether the texture is a mod texture |
| `atlas_index` | int | *(Item entities only)* Atlas texture index (-1 for mod textures) |

```lua
local entities = minecraft.entities.list()
for _, e in ipairs(entities) do
  print(e.id, e.type, e.x, e.y, e.z)
end

-- Filter by type
local zombies = minecraft.entities.list("Zombie")
```

---

### `minecraft.entities.apply_state(entity, state)`
Apply position, velocity, rotation, and/or custom data to one LuaModEntity. Only affects entities spawned via `minecraft.entities.spawn_mod`.

| Param | Type | Description |
|-------|------|-------------|
| `entity` | table | Entity handle table containing `id` |
| `state` | table | Optional fields: `x`, `y`, `z`, `vx`, `vy`, `vz`, `yaw`, `pitch`, `data` |

**Returns:** boolean — `true` on success.

```lua
local entity = minecraft.entities.get(42)
if entity then
  minecraft.entities.apply_state(entity, { x = 100, y = 64, z = 200, yaw = 90 })
end
```

---

### `minecraft.entities.teleport(id, {x,y,z,yaw,pitch} or x,y,z, yaw?, pitch?)`
Teleport **any** entity (not just mod-spawned) to a new position/rotation.

| Param | Type | Description |
|-------|------|-------------|
| `entity` | table | Entity handle table containing `id` |
| position | table or vararg | Either `{x, y, z, yaw?, pitch?}` table or individual `x, y, z, yaw?, pitch?` arguments |

**Returns:** boolean — `true` if the entity was found and teleported.

```lua
-- Using a table
minecraft.entities.teleport({id=42}, {x=100, y=64, z=200, yaw=90, pitch=0})

-- Using individual arguments
minecraft.entities.teleport({id=42}, 100, 64, 200, 90, 0)

-- Position only (no rotation change)
minecraft.entities.teleport({id=42}, 100, 64, 200)
```

---

### `minecraft.entities.remove(entity)`
Remove a LuaModEntity from the world.

| Param | Type | Description |
|-------|------|-------------|
| `entity` | table | Entity handle table containing the entity ID |

**Returns:** boolean — `true` if the entity was found and marked for removal. Only works on entities spawned via `spawn_mod`.

```lua
minecraft.entities.remove({id=42})
```

---

### `minecraft.entities.spawn_mod(registryId, {x, y, z, yaw?, pitch?, data?})`
Spawn a custom mod entity. The `registryId` must be in `"modid:name"` format and must match the current mod's ID.

| Param | Type | Description |
|-------|------|-------------|
| `registryId` | string | Registry identifier in `"modid:name"` format |
| spec | table | Table with `x`, `y`, `z` (doubles), optional `yaw`, `pitch` (floats), optional `data` (table) |

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

---

### `minecraft.entities.register_global_pose_hook(entityType, callback)`
Override the render pose for **all** entities of a given type. Affects every entity of that type, including vanilla ones. Requires mod context.

| Param | Type | Description |
|-------|------|-------------|
| `entityType` | string | Entity type string (e.g. `"Zombie"`) |
| `callback` | function | Function receiving event table with `entity_id`, `entity_type`, `tick_delta`, `pose` — modify `pose` to override |

**Returns:** boolean — `true` if registered.

```lua
minecraft.entities.register_global_pose_hook("Zombie", function(event)
  event.pose.head_yaw = event.pose.head_yaw + 180
  event.pose.parts.left_arm = { yaw = 90, pitch = 0, roll = 0 }
end)
```

---

### `minecraft.entities.register_local_pose_hook(entityId, callback)`
Override the render pose for a **specific** entity by ID.

| Param | Type | Description |
|-------|------|-------------|
| `entityId` | int | Entity network ID |
| `callback` | function | Same callback pattern as global pose hook |

**Returns:** boolean — `true` if registered.

```lua
minecraft.entities.register_local_pose_hook(42, function(event)
  event.pose.scale = 2.0
end)
```

---

### `minecraft.entities.unregister_local_pose_hook(entityId)`
Remove a previously registered local pose hook.

| Param | Type | Description |
|-------|------|-------------|
| `entityId` | int | Entity network ID whose local hook to remove |

**Returns:** boolean — `true` if a hook was removed.

```lua
minecraft.entities.unregister_local_pose_hook(42)
```

---

### EntityRenderPose fields

The `pose` table in pose hook callbacks has the following fields. All are read-write.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `body_yaw` | float | current | Body yaw rotation |
| `head_yaw` | float | current | Head yaw rotation |
| `head_pitch` | float | current | Head pitch rotation |
| `limb_swing` | float | current | Limb swing animation |
| `limb_distance` | float | current | Limb distance |
| `yaw` | float | current | Entity yaw |
| `pitch` | float | current | Entity pitch |
| `roll` | float | current | Entity roll |
| `scale` | float | current | Render scale (1.0 = normal) |
| `offset_x` | float | current | X render offset |
| `offset_y` | float | current | Y render offset |
| `offset_z` | float | current | Z render offset |
| `parts` | table | current | Table mapping part name to `{yaw, pitch, roll}` |

The event table also provides:
| Field | Type | Description |
|-------|------|-------------|
| `entity_id` | int | The entity being rendered |
| `entity_type` | string | Entity type string |
| `tick_delta` | float | Partial tick for interpolation |

```lua
-- Example: flip a creeper upside down
minecraft.entities.register_global_pose_hook("Creeper", function(event)
  event.pose.roll = 180
  event.pose.offset_y = 2.0
end)
```

---

## `minecraft.tile_entities.*`

### `minecraft.tile_entities.list(filter?)`
List all tile entities (block entities) in the world.

| Param | Type | Description |
|-------|------|-------------|
| `filter` | string (optional) | If provided, only returns tile entities whose type ID matches |

**Returns:** array of tile entity handle tables (see handle methods below).

```lua
local tes = minecraft.tile_entities.list()
for _, te in ipairs(tes) do
  print(te:get_id(), te.x, te.y, te.z)
end

-- Filter by type
local chests = minecraft.tile_entities.list("Chest")
```

---

### `minecraft.tile_entities.get(x, y, z)`
Get the tile entity handle at a specific world position.

| Param | Type | Description |
|-------|------|-------------|
| `x` | int | World X coordinate |
| `y` | int | World Y coordinate |
| `z` | int | World Z coordinate |

**Returns:** tile entity handle table, or `nil` if no tile entity at that position.

```lua
local te = minecraft.tile_entities.get(100, 64, 200)
```

---

### `minecraft.tile_entities.count(filter?)`
Count tile entities in the world.

| Param | Type | Description |
|-------|------|-------------|
| `filter` | string (optional) | Type filter string |

**Returns:** integer — number of tile entities.

```lua
local total = minecraft.tile_entities.count()
local chestCount = minecraft.tile_entities.count("Chest")
```

---

### Tile entity handle methods

Handles returned from `list()` and `get()` support the following methods:

| Method | Returns | Description |
|--------|---------|-------------|
| `:get_id()` | string or nil | Tile entity type string (e.g. `"Chest"`, `"Furnace"`) |
| `:get_block_id()` | int | Numeric block ID at this tile entity's position |
| `:get_block_meta()` | int | Block metadata value |
| `:is_removed()` | boolean | Whether the tile entity has been removed |
| `:mark_dirty()` | nothing | Mark the tile entity for saving to disk (server-side only) |
| `:distance_from(tx, ty, tz)` | double | Euclidean distance from the tile entity to the given point |
| `:get_world_time()` | double | Current world tick time |
| `:get_data()` | table or nil | Custom NBT data as a Lua table (only for LuaModBlockEntities) |
| `:set_data(table)` | nothing | Set custom NBT data (only for LuaModBlockEntities) |
| `:get_animation_frame()` | int | Current animation frame count (= tick × speed) |
| `:set_animation_speed(speed)` | nothing | Set animation speed multiplier |

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

---

## `minecraft.particles.*`

### `minecraft.particles.spawn({x?, y?, z?, vx?, vy?, vz?, scale?, r?, g?, b?, max_age?, gravity?})`
Spawn a custom client-side particle. Client-only (does nothing on server).

| Param | Type | Default | Description |
|-------|------|---------|-------------|
| `x` | float | `0.0` | Spawn position X |
| `y` | float | `64.0` | Spawn position Y |
| `z` | float | `0.0` | Spawn position Z |
| `vx` | float | `0.0` | Initial velocity X |
| `vy` | float | `0.0` | Initial velocity Y |
| `vz` | float | `0.0` | Initial velocity Z |
| `scale` | float | `4.0` | Particle scale (clamped 0.05–4.0) |
| `r` | float | `1.0` | Red color component; passed through without clamping |
| `g` | float | `1.0` | Green color component; passed through without clamping |
| `b` | float | `1.0` | Blue color component; passed through without clamping |
| `max_age` | int | `40` | Particle lifetime in ticks |
| `gravity` | float | `0.04` | Gravity strength (positive = downward) |

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

---

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

---

## `minecraft.raycast`

### `minecraft.raycast(options?)`
Perform a raycast from the camera, or from a custom origin with a custom direction. Client-only (returns `nil` on server).

| Param | Type | Description |
|-------|------|-------------|
| `options` | table (optional) | Configuration table (see below). If omitted, raycasts from center of screen with reach distance. |

**Options table fields:**

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `origin` | table | camera position | `{x, y, z}` or positional `{origin_x, origin_y, origin_z}` — ray origin |
| `direction` | table | camera look vector | `{x, y, z}` vector direction |
| `yaw`, `pitch` | float | camera look | Alternative to `direction` — converts degrees to direction vector |
| `max_distance` | float | reach / `5.0` | Maximum raycast distance |
| `reach` | float | reach / `5.0` | Alias for `max_distance` |
| `ignore_liquids` | boolean | `false` | Whether to ignore liquid blocks |
| `blocks` | boolean | `true` | Whether to test against blocks |
| `entities` | boolean | `true` | Whether to test against entities |

**Returns:** hit result table or `nil` if nothing was hit.

The result table has `type` — one of `"block"`, `"entity"`, or `"model"`.

**Block hit result:**

| Field | Type | Description |
|-------|------|-------------|
| `type` | string | `"block"` |
| `block_id` | int | Hit block's numeric ID |
| `block_name` | string | Hit block's wire name |
| `item_id` | int | Item ID (same as block_id for blocks) |
| `block_x`, `block_y`, `block_z` | int | Block position |
| `side` | int | Face side hit (0–5) |
| `hit_x`, `hit_y`, `hit_z` | double | Exact hit position |

**Entity hit result:**

| Field | Type | Description |
|-------|------|-------------|
| `type` | string | `"entity"` |
| `entity_id` | int | Entity network ID |
| `entity_raw_id` | int | Raw entity registry ID |
| `entity_type` | string | Entity type string |
| `entity_x`, `entity_y`, `entity_z` | double | Entity position |
| `hit_x`, `hit_y`, `hit_z` | double | Exact hit position |

**Model hit result:**

| Field | Type | Description |
|-------|------|-------------|
| `type` | string | `"model"` |
| `model_id` | int | Placed model instance ID |
| `model_tag` | string | Model tag string |
| `hit_x`, `hit_y`, `hit_z` | double | Exact hit position |
| `distance` | double | Distance from ray origin |

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

---

## World Generation Events

### `chunk_generation`

Fired during server-side chunk generation. `chunk_x` and `chunk_z` are absolute chunk coordinates; block edits through `event.chunk` use local X/Z coordinates (0–15).

**Event fields:**

| Field | Type | Description |
|-------|------|-------------|
| `stage` | string | One of: `"terrain"`, `"surface"`, `"carver"`, `"features"` |
| `moment` | string | `"before"` or `"after"` |
| `cancel_vanilla` | boolean | Set to `true` to cancel vanilla generation for this stage (read-write) |
| `vanilla_stage_ran` | boolean | Whether the vanilla stage already executed |
| `world_seed` | int | World seed |
| `has_world` | boolean | Whether a world context is available |
| `world_name` | string | World name |
| `is_overworld` | boolean | Whether this is the Overworld dimension |
| `mod_generation` | boolean | Whether mod generation is enabled for this world |
| `chunk_x`, `chunk_z` | int | Chunk coordinates |
| `has_chunk` | boolean | Whether a chunk context is active |
| `chunk` | ChunkHandle | The local chunk object handle for generation writes/reads |

**Stages:**

| Stage | Write Mode | Description |
|-------|-----------|-------------|
| `terrain` | RawGeneration | Base terrain shape (no biome decorations yet) |
| `surface` | RawGeneration | Surface layer placement (grass, sand, gravel, etc.) |
| `carver` | RawGeneration | Cave/canyon carving |
| `features` | ChunkApi | Ore veins, trees, flowers, dungeons, etc. |

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

---

### `world_spawn_search`

Fired when the game searches for a valid world spawn point. Can be used to override the spawn location.

**Event fields:**

| Field | Type | Description |
|-------|------|-------------|
| `x` | int | Candidate spawn X (read-write) |
| `y` | int | Candidate spawn Y (read-write, default `64`) |
| `z` | int | Candidate spawn Z (read-write) |
| `resolved` | boolean | Set to `true` to accept the current coordinates as the spawn point (read-write) |
| `has_world` | boolean | Whether a world context is available |
| `world_name` | string | World name |
| `is_overworld` | boolean | Whether this is the Overworld |

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

---

### `create_world`

Fired when a new world is being created. Cancellable.

**Event fields:**

| Field | Type | Description |
|-------|------|-------------|
| `save_name` | string | World save name |
| `seed` | int | World seed |
| `canceled` | boolean | Set to `true` to cancel world creation (read-write) |
| `options` | table | Map of string key-value pairs persisted in `level.dat` (read-write) |

```lua
minecraft.on(minecraft.events.create_world, {}, function(event)
  event.options = event.options or {}
  event.options["mymod:difficulty"] = "hard"
  event.options["mymod:starting_items"] = "yes"
end)
```

---

### `world_open`

Fired when a world is opened (loaded). Read-only.

**Event fields:**

| Field | Type | Description |
|-------|------|-------------|
| `save_name` | string | World save name |
| `new_world` | boolean | Whether this is a newly created world |
| `options` | table | Map of persisted options string values from `level.dat` |

---

### `world_start`

Fired when a world starts ticking. Read-only.

**Event fields:**

| Field | Type | Description |
|-------|------|-------------|
| `save_name` | string | World save name |
| `new_world` | boolean | Whether this is a newly created world |

---

### `world_tick`

Fired every world tick.

**Event fields:**

| Field | Type | Description |
|-------|------|-------------|
| `remote` | boolean | Whether this is the client world |
| `before` | boolean | `true` for pre-tick, `false` for post-tick |

```lua
minecraft.on(minecraft.events.world_tick, {}, function(event)
  if event.before then
    -- Pre-tick logic
  else
    -- Post-tick logic
  end
end)
```
