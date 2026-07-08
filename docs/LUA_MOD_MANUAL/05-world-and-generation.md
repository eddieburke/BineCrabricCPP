# Volume V — World and Generation

World queries, chunk mutation during generation, spawn search, and offline terrain sampling.

---

## `minecraft.world` (runtime)

Requires active client world (`Minecraft::INSTANCE->world` or chunk context).

| Function | Signature | Returns | Notes |
|----------|-----------|---------|-------|
| `block_id(name)` | string | int | Wire name → numeric id; 0 if unknown |
| `get_block(x,y,z)` | int × 3 | int | Block id at world coords |
| `get_top_y(x,z)` | int × 2 | int | Top solid Y; **-1** if no world |
| `random(bound?)` | int? | int | World RNG; default bound 1000 → 0..999 |
| `is_night()` | — | bool | From world time |
| `player()` | — | `{x,y,z}` \| nil | Local player position |
| `spawn_entity(type, pos)` | string, table or x,y,z | bool | Entity registry name |
| `count_entities(type)` | string | int | Count by name |
| `set_cursor(item_id, count)` | int × 2 | bool | Set player cursor stack |
| `set_time(ticks)` | 0..23999 | bool | `synchronizeTimeAndUpdates` |
| `sample_grid(seed, cx, cz, opts)` | see below | grid table | Offline terrain probe |
| `marker_px(grid, wx, wz)` | grid, coords | col, row | Map pixel index |

### Entity spawn names

Use vanilla entity id strings (e.g. `"pig"`, `"zombie"`). Invalid names return `false`.

---

## `minecraft.world.sample_grid`

Offline biome/terrain sampler. Does **not** require a loaded world.

```lua
local grid = minecraft.world.sample_grid(seed, center_x, center_z, {
  radius_chunks = 6,       -- 1..4096, default 6
  radius = 6,              -- alias for radius_chunks
  max_side = 48,           -- 8..256, default 48
  channel = "grass",       -- primary channel
  channels = { "grass", "height", "biome_id" },  -- up to 8
  mod_generation = false,  -- true → run mod chunk hooks in probe
})
```

### Return table

| Field | Type | Description |
|-------|------|-------------|
| `side` | int | Grid dimension (cells per side) |
| `step` | int | World blocks between samples |
| `origin_x`, `origin_z` | int | World coord of cell (0,0) |
| `center_x`, `center_z` | int | Requested center |
| `channel` | string | Primary channel name |
| `values` | int[] | Flat array, row-major, `side×side` cells |
| `channels` | table | Named arrays per channel |

### Channels

| Channel | Cell value | Notes |
|---------|------------|-------|
| `grass` | ARGB int | `0xFFRRGGBB` biome grass color |
| `biome_id` | int | Internal biome id |
| `height` | int | Surface height |
| `surface_block` | int | Block id at surface |
| `surface_block_below` | int | Block id two below surface top |

**Aliases** (normalized internally): `biome_grass`→`grass`, `surface_y`→`height`, `top_block`→`surface_block`, `block_below_surface`→`surface_block_below`.

Terrain channels (`height`, `surface_block`, `surface_block_below`) trigger `OverworldChunkGenerator` with optional `mod_generation`.

### `marker_px`

```lua
local col, row = minecraft.world.marker_px(grid, world_x, world_z)
```

Maps world coordinates to grid indices clamped to `[0, side-1]`.

---

## `minecraft.registry`

| Function | Args | Returns |
|----------|------|---------|
| `name(domain, id)` | `"biome"` or `"block"`, int | wire string; `"unknown"` if invalid |
| `list(domain)` | domain | sorted string array |

Biome domain always available. Block domain requires client build.

---

## `minecraft.chunk` (generation only)

Active only inside `chunk_generation` callbacks when `has_chunk == true`.

| Function | Args | Returns |
|----------|------|---------|
| `set_block(lx, y, lz, id)` | local 0..15, y 0..127, id | bool |
| `get_block(lx, y, lz)` | | int |
| `get_height(lx, lz)` | | int |
| `fill(x1,y1,z1, x2,y2,z2, id)` | inclusive local bounds | int changed |

**Coordinates:** local X/Z within chunk 0..15. World position = `chunk_x*16 + lx`.

### Write modes by stage

| Stage | Mode | Meaning |
|-------|------|---------|
| `terrain`, `surface`, `carver` | RawGeneration | Direct chunk buffer writes |
| `features` | ChunkApi | Full chunk API semantics |

---

## Generation events

Subscribe with:

```lua
minecraft.on(minecraft.events.chunk_generation, {
  stage = minecraft.generation.stages.features,
  moment = minecraft.generation.moments.after,
  when = minecraft.util.real_world,
  priority = 100,
}, function(event) ... end)
```

### Event fields

| Field | R/W | Description |
|-------|-----|-------------|
| `stage` | R | `terrain`, `surface`, `carver`, `features` |
| `moment` | R | `before`, `after` |
| `cancel_vanilla` | R/W | Before only — skip vanilla stage |
| `vanilla_stage_ran` | R | After — whether vanilla ran |
| `world_seed` | R | int64 seed |
| `chunk_x`, `chunk_z` | R | Chunk coords |
| `has_chunk` | R | Chunk context active |
| `mod_generation`, `is_overworld` | R | World flags |
| + world context | R | `has_world`, `world_name`, etc. |

**Skipped** on remote/client-only worlds.

---

## World lifecycle events

### `create_world`

Fires when player confirms Create World. Writable `options` map (string→string) persisted to `level.dat`.

```lua
minecraft.on(minecraft.events.create_world, {}, function(event)
  event.options["my_mod:mode"] = "hardcore"
end)
```

Use namespaced keys. Set `event.canceled = true` to abort.

### `world_open`

Before `World` construction. Read `event.options` restored from save.

### `world_start`

After world session begins. Read-only `save_name`, `new_world`.

### `world_spawn_search`

Adjust spawn point (server/local generation only).

| Field | R/W |
|-------|-----|
| `x`, `y`, `z` | R/W |
| `resolved` | R/W — set true when mod picks spawn |

---

## `minecraft.particles`

Client-side decorative particles:

```lua
minecraft.particles.spawn({
  x = 0, y = 64, z = 0,
  vx = 0, vy = 0.1, vz = 0,
  scale = 4.0,    -- default 4
  r = 1, g = 1, b = 1,
  max_age = 40,
  gravity = 0.04,
})  -- returns bool
```

---

## Pattern: ravine carver

```lua
minecraft.on(minecraft.events.chunk_generation, {
  stage = minecraft.generation.stages.carver,
  moment = minecraft.generation.moments.after,
  when = minecraft.util.real_world,
}, function(event)
  local seed = math.floor(event.world_seed or 0)
  if (seed + event.chunk_x * 7342871 + event.chunk_z * 912931) % 35 ~= 0 then
    return
  end
  -- carve with minecraft.chunk.set_block ...
end)
```

---

## Pattern: world profiles (create_world + chunk_generation)

`world_profiles` mod stores `world_profiles:type` in `create_world` options, restores on `world_open`, then filters `chunk_generation` by profile (cancel carver/features, adjust spawn via `world_spawn_search`).

See [Volume IX](09-mod-gallery.md#world_profiles).

---

*See also: [Volume II](02-events-reference.md), [Volume IV](04-registration.md)*
