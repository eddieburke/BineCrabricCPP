# 04 - World, Generation & Entity API

BineCrabricCPP exposes world query/mutation interfaces, procedural generation pipeline hooks, entity state mutators, and a raycasting engine to Lua scripts.

---

## 1. World Manipulation API (`minecraft.world`)

Functions under `minecraft.world` allow reading and mutating blocks, lighting, biomes, and entities in the active world.

### Block Queries & Mutations

#### `minecraft.world.get_block(x, y, z)`
- **Signature**: `minecraft.world.get_block(x: integer, y: integer, z: integer) -> block_id: integer`
- **Returns**: Numerical block ID at world coordinates `(x, y, z)` (0 if empty/air or out of bounds).

#### `minecraft.world.set_block(x, y, z, block_id, meta)`
- **Signature**: `minecraft.world.set_block(x: integer, y: integer, z: integer, block_id: integer, meta?: integer) -> success: boolean`
- **Behavior**: Replaces block at `(x, y, z)` with `block_id` and optional metadata value. Triggers block update and lighting recalculation.

#### `minecraft.world.fill_block(x1, y1, z1, x2, y2, z2, block_id)`
- **Signature**: `minecraft.world.fill_block(x1: int, y1: int, z1: int, x2: int, y2: int, z2: int, block_id: int) -> count: int`
- **Behavior**: Fills axis-aligned bounding box from `(x1, y1, z1)` to `(x2, y2, z2)` with `block_id`. Returns number of blocks changed.

#### `minecraft.world.get_height(x, z)`
- **Signature**: `minecraft.world.get_height(x: integer, z: integer) -> y: integer`
- **Returns**: Highest non-air block Y-coordinate at `(x, z)`.

#### `minecraft.world.get_light(x, y, z)`
- **Signature**: `minecraft.world.get_light(x: integer, y: integer, z: integer) -> light_level: integer`
- **Returns**: Combined block and sky light level (0..15).

#### `minecraft.world.get_biome(x, z)`
- **Signature**: `minecraft.world.get_biome(x: integer, z: integer) -> biome_id: string`
- **Returns**: Biome identifier (e.g. `"rainforest"`, `"taiga"`, `"desert"`).

---

## 2. World Generation Pipeline (`minecraft.generation`)

World generation events occur sequentially per chunk (`chunk_generation` event). Mods intercept or override pipeline stages:

- **Stages** (`minecraft.generation.stages`):
  - `"terrain"`: Initial heightmap and base stone shape generation.
  - `"surface"`: Grass, dirt, sand, and bedrock top layer placement.
  - `"carver"`: Cave systems and ravine carver pass.
  - `"features"`: Ores, trees, plants, structures, and custom mod features (e.g., coral reefs, meteor deposits).
- **Moments** (`minecraft.generation.moments`): `"before"`, `"after"`.

### Chunk Context API

During generation passes, `LuaChunkContext` provides fast in-chunk relative block placement:

- `minecraft.world.set_block(local_x, y, local_z, block_id)` (where `local_x`, `local_z` are 0..15 inside chunk bounds).

---

## 3. Entity Manipulation API (`minecraft.entity`)

Interact with entities in the world using entity handles:

### `minecraft.entity.get_pos(entity)`
- **Returns**: `x: double, y: double, z: double`

### `minecraft.entity.set_pos(entity, x, y, z)`
- Sets entity position in world coordinates.

### `minecraft.entity.get_velocity(entity)`
- **Returns**: `vx: double, vy: double, vz: double`

### `minecraft.entity.set_velocity(entity, vx, vy, vz)`
- Sets entity motion vectors.

### `minecraft.entity.get_health(entity)` / `set_health(entity, health)`
- Reads or mutates LivingEntity health value.

### `minecraft.entity.is_player(entity)`
- Returns `true` if entity handle references a `PlayerEntity`.

### `minecraft.entity.get_inventory(player)`
- Returns array of item stack tables `{ id = item_id, count = stack_size, damage = durability_used }` currently held in player inventory slots.

---

## 4. Raycasting Engine (`minecraft.raycast`)

Performs line-of-sight raycasts against world geometry and entity hitboxes.

### `minecraft.raycast(spec)`
- **Signature**: `minecraft.raycast(spec: table) -> hit_result: table`
- **Spec Parameters**:
  - `origin`: `{ x, y, z }` (camera or entity eye position).
  - `direction`: `{ x, y, z }` (normalized look vector).
  - `max_distance`: Maximum distance in blocks (default `5.0`).
- **Returns Table**:
  ```lua
  {
    hit_type = "block" | "entity" | "none",
    distance = 3.42,
    block_x = 120, block_y = 64, block_z = -45,
    side = 4, -- Block face index
    entity_id = 42 -- Present if hit_type == "entity"
  }
  ```
