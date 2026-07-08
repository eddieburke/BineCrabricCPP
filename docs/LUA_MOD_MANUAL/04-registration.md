# Volume IV — Registration (Blocks, Items, Recipes)

All content registration runs during **mod script load**, before `Registry::bootstrap()`. Calling `register_*` after bootstrap returns an error (except `crafting.add_shaped_recipe` at runtime).

---

## Block registration

### `minecraft.register_block(spec)`

Internally calls `registerBlockSpec` in `LuaBlockModel.cpp`.

#### Top-level spec fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | int | **yes** | 1 .. BLOCK_COUNT-1, unique, reserved via Registry |
| `texture` | string | one of | Package path e.g. `mods/foo/block.png` |
| `texture_id` | int | one of | Terrain atlas index 0..255 |
| `hardness` | float | no | Default 1.0 |
| `resistance` | float | no | Default 1.0 |
| `luminance` | float | no | 0..1 light emission |
| `translation_key` | string | no | Lang key; aliases registered |
| `name` | string | no | Display name override |
| `material` | string | no | `stone`, `wood`, `metal`, etc. |
| `behavior_priority` | int | no | `block_interact` priority for `on_use` |
| `model` | table | no | Default `full_cube` |
| `on_use` | function | no | Right-click handler |

#### Error messages

- `register_block id must be between 1 and N`
- `register_block requires texture or texture_id`
- `register_block texture_id must be a vanilla terrain-atlas index from 0 to 255`
- `register_block must run while Lua mod scripts load at startup`
- `register_block duplicate id: X`
- `register_block id is already reserved: X`

#### Model types

**`full_cube` / `simple` (default)**

Optional model fields:

| Field | Default | Description |
|-------|---------|-------------|
| `opaque` | true | Blocks light |
| `full_cube` | true | Full cube collision |
| `collision_height` | 1.0 | Hitbox height |
| `stack_on_same` | false | Stack placement |
| `requires_solid_below` | false | Needs support |
| `varied_bounds` / `coordinate_bounds` | false | Per-position size variation |
| `coordinate_color` / `coord_color` | false | Position-based tint |
| `color` = `"coordinate"` | — | Enables coordinate color |
| `bounds_padding`, `bounds_offset` | | Variation params |
| `min_scale`, `max_scale` | | Scale range |

**`box_list`**

```lua
model = {
  type = "box_list",
  opaque = false,
  full_cube = false,
  boxes = {
    { min = { x, y, z }, max = { x, y, z }, always = true },
  },
}
```

Coordinates in **0..1** block space. Up to 32 boxes.

**`connected_bars`**

```lua
model = {
  type = "connected_bars",
  core = { min = {...}, max = {...} },
  north = { min, max },  -- optional arms
  south = { ... },
  east = { ... },
  west = { ... },
  connect = { "same", "opaque", "glass", "fence" },
}
```

**`manual` / `custom` / `tessellated`**

```lua
model = {
  type = "manual",
  opaque = false,
  full_cube = false,
  draw = function() ... end,           -- world/block tessellator
  inventory = function() ... end,      -- optional GUI icon draw
}
```

Use `minecraft.tessellator.quad({...})` inside draw functions.

#### `on_use` event fields

See Vol II `block_interact`. Set `event.handled = true` to mark consumed; prelude sets `event.canceled` when handled.

---

## Item registration

### `minecraft.register_item(spec)`

Item ids: **256 .. ITEM_COUNT-1** (raw id = itemId - 256).

| Field | Required | Description |
|-------|----------|-------------|
| `id` | yes | Numeric item id |
| `texture` or `texture_id` | yes | items.png atlas or custom path |
| `name` | no | Display name |
| `translation_key` | no | Lang key |
| `max_count` | no | Default 64 |
| `max_damage` | no | Default 0 (undamageable) |
| `model` | no | Default `flat` |

#### Model types

| type | Description |
|------|-------------|
| `flat` / `simple` | Icon in atlas |
| `box_list` / `boxes` | 3D boxes in item GUI |
| `manual` / `custom` | `draw = function(info)` — info has `item_id`, `texture`, `brightness` |

**Errors (from `LuaItemModel.cpp`):**

- `register_item id must be between 256 and N`
- `register_item requires texture or texture_id`
- `register_item requires texture for a box_list or manual model`
- `register_item texture_id must be a vanilla items-atlas index from 0 to 255`
- `register_item must run while Lua mod scripts load at startup`
- `register_item duplicate id: X`
- `register_item id is already reserved: X`

**Model errors:** `box_list item model requires boxes array`, `manual item model requires a draw function`, `unknown item model type`.

Item box coords: `min`/`max` tables or `min_x`..`max_z` fields, 0..1 space.

---

## Recipe registration

### Startup: `minecraft.register_shaped_recipe(spec)`

Queued at crafting phase order **50000** (after vanilla block/item recipes by id). Sorted when phase completes.

### Runtime: `minecraft.crafting.add_shaped_recipe(spec)`

Immediate registration; recipes re-sorted. Use for player-created recipes, unlock systems, etc.

**Validation (from `LuaBlockRegistry.cpp`):**

- Exactly one of `output_block_id`, `output_item_id`
- `shaped recipe accepts only one of output_block_id or output_item_id`
- `output_count` 1..64
- Pattern 1..3 rows, 1..3 columns, uniform width
- `shaped recipe pattern does not use the ingredient key`
- `item_id` required; ingredient must exist when registered
- At runtime after bootstrap: `shaped recipe output or ingredient is unknown`

---

## Registration order (engine)

```
loadEnabledPackageMods()     -- all main.lua, queue registrations
Registry::bootstrap():
  BlockRegistration          -- vanilla by id, Lua blocks @ 50000
  BlockRegistryFinalize
  BiomeRegistration
  ItemRegistration           -- Lua items @ 50000
  BlockItemRegistration
  SmeltingRecipeRegistration
  CraftingRecipeRegistration -- vanilla recipes, Lua @ 50000, then sort
  EntityRegistration
  BlockEntityRegistration
  FuelRegistration
  ClientRendererRegistration
  ParticleRegistration
Frozen
```

Lua mod blocks/items/recipes use order **50000** so vanilla content registers first by numeric id.

---

## Wire names

After registration, blocks/items are addressable:

```lua
minecraft.world.block_id("stone_brick")  -- mod block by translation alias
minecraft.registry.name("block", 98)
```

Mod blocks register aliases from `translation_key` (snake_case wire name).
