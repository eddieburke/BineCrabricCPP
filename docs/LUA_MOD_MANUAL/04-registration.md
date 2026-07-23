# 04 — Registration

Registration functions create new blocks, items, and recipes. They must generally be called during mod initialization (before the game finishes bootstrapping). Content is registered via lifecycle phases (`at_phase`), which execute in order: `init` → `post_init` → `ready`. Register blocks and items in `init`, recipes and cross-references in `post_init`.

---

## Block Registration

### `minecraft.register_block(spec)`

Registers a new block. Returns `true` on success, or throws an assertion error on failure. The `spec` table accepts the following fields:

| Field | Type | Default | Description |
|---|---|---|---|
| `id` | int | (required) | Numeric block ID (1–`BLOCK_COUNT - 1`). Must be unique and not already reserved. |
| `texture` | string | (required unless `texture_id` given) | Texture path for the default face texture (e.g. `"textures/blocks/my_block.png"`). |
| `texture_id` | int | `-1` | Vanilla terrain-atlas texture index override (0–255). Alternative to `texture`. |
| `hardness` | float | `1.0` | Block hardness (mining time). |
| `resistance` | float | `1.0` | Explosion resistance. |
| `luminance` | float | `0.0` | Light emission level (clamped 0.0–1.0). |
| `translation_key` | string | `"block<id>"` | i18n key token (without `"tile."` or `".name"` suffix). |
| `name` | string | auto-generated | Display name (added to I18n table). If empty, derived from the translation key. |
| `material` | string | `"stone"` | Material type. One of: `"stone"`, `"metal"`, `"wood"`, `"glass"`. |
| `opaque` | bool | `true` | Whether the block is fully opaque (affects light and rendering). |
| `full_cube` | bool | `true` | Whether the block occupies a full 1×1×1 cube for culling. |
| `translucent` | bool | `not opaque` | Render layer. `false` = solid/cutout pass (block casts shader-mod shadows); `true` = alpha-blended pass (drawn after entities, casts no shader shadows). Solid-looking blocks that only set `opaque = false` for lighting should set `translucent = false`. |
| `collision_height` | float | `1.0` | Custom collision height. Values > 1 create taller collision boxes. |
| `stack_on_same` | bool | `false` | If `true`, the block can be placed on another block of the same type. If `false` (and default), placing on the same type is prevented. |
| `requires_solid_below` | bool | `true` | Whether the block requires a solid block below to be placed. |
| `coordinate_bounds` | bool | `false` | Randomizes the render/collision AABB per world position using coordinate-based hash. Produces natural-looking variation. |
| `coordinate_color` | bool | `false` | When `true`, colors the block by world position using coordinate-based hashing. |
| `bounds_padding` | float | `0.0625` | Padding from full cube when `coordinate_bounds` is active. Clamped 0.0–0.49. |
| `bounds_offset` | float | `0.1` | Maximum random offset magnitude for coordinate-based bounds. |
| `min_scale` | float | `0.9` | Minimum scale factor for coordinate-based bounds. |
| `max_scale` | float | `1.1` | Maximum scale factor for coordinate-based bounds. |
| `model` | int or function | — | Baked model handle (integer from `minecraft.model.load`/`minecraft.model.build`) or a model callback function. When set, overrides the default cube renderer with custom baked-model rendering. |
| `item` | table | — | Sub-table for the block's corresponding item: `{texture = "...", texture_id = int}`. If omitted, the block item uses the block's terrain texture. |
| `tile_entity` | string | — | Tile entity registry ID string. The full registry ID becomes `"<ownerModId>:<tile_entity>"`. Registers a block entity factory. |
| `on_use` | function | — | Right-click handler function. Automatically wires a `block_interact` event listener filtered to this block ID with `right_click = true`. |
| `behavior_priority` | int | `0` | Priority for the auto-wired interact handler when `on_use` is set. |

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

- `id` must be between 1 and `Block::BLOCK_COUNT - 1` (typically 1–255).
- Either `texture` or `texture_id` (0–255) is required.
- `texture_id` must be 0–255 (vanilla terrain atlas index).
- Duplicate IDs are rejected.
- The block must be registered before the game finishes bootstrapping (`!Registry::isBootstrapped()`).

---

## Item Registration

### `minecraft.register_item(spec)`

Registers a new item. Returns `true` on success, or `false, error` on failure. The `spec` table accepts:

| Field | Type | Default | Description |
|---|---|---|---|
| `id` | int | (required) | Numeric item ID (absolute, 256–`ITEM_COUNT - 1`). Must be unique. |
| `texture` | string | (required unless `texture_id` given) | Texture path for the item sprite. |
| `texture_id` | int | `-1` | Vanilla items-atlas texture index (0–255). Alternative to `texture`. |
| `max_count` | int | `64` | Maximum stack size. |
| `max_damage` | int | `0` | Maximum damage (0 = not damageable). |
| `translation_key` | string | `"item<id>"` | i18n key token (without `"item."` or `".name"` suffix). |
| `name` | string | auto-generated | Display name. |
| `model` | int or function | — | Baked model handle or model callback. When set, the item renders as a 3D model. Requires `texture` to be set. |

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

- `id` must be ≥ 256 and < `ITEM_COUNT` (raw ID = `itemId - 256`).
- Either `texture` or `texture_id` (0–255) is required.
- If a `model` is provided, `texture` is also required.
- `texture_id` must be 0–255.
- Duplicate IDs are rejected.
- Must be registered before game bootstrapping completes.

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

---

## Recipe Registration

### `minecraft.register_shaped_recipe(spec)`

Registers a shaped (pattern-based) crafting recipe. The spec table accepts:

| Field | Type | Default | Description |
|---|---|---|---|
| `output_block_id` | int | `0` | Output block ID (alternative to `output_item_id`). |
| `output_item_id` | int | `0` | Output item ID (alternative to `output_block_id`). |
| `output_count` | int | `1` | Output stack size (1–64). |
| `pattern` | array of strings | (required) | Crafting pattern rows (1–3 rows, each 1–3 characters, all rows same width). |
| `key` | string | `"#"` | The character in the pattern representing the ingredient. Uses only the first character. |
| `item_id` | int | (required) | The ingredient item/block ID. |

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

---

## Mod Settings & Keybinds Registry

Register simple sliders, toggles, and keybinds here. The engine persists them and keeps them on the main **Mod Settings** page, opened from the permanent "Mod Settings..." button in Video Settings (`minecraft.screen.ids.video_options`).

For a richer page, use the [Settings DSL](07-gui-and-screens.md) (`minecraft.screen.settings`) with `parent_screen = minecraft.screen.ids.mod_settings`. Buttons injected into the `minecraft:mod_settings` footer are collected automatically on its scrollable **Mod Pages** page; no custom navigation glue is needed.

### `minecraft.settings.register(display_name, entries)`

`display_name` is the label shown for your mod's section in the shared screen (defaults to your mod id if omitted). `entries` is an array of tables:

| Field | Type | Applies to | Description |
|-------|------|------------|--------------|
| `key` | string | all | Setting key, unique within your mod (required) |
| `label` | string | all | Display label (defaults to `key`) |
| `kind` | string | all | `"slider"` (default) or `"toggle"` |
| `default` | number/boolean | all | Initial value (number for slider, boolean for toggle) |
| `min`, `max` | number | slider | Range (defaults `0`..`1`) |
| `step` | number | slider | Increment per click (default: range / 20) |
| `integer` | boolean | slider | Snap to whole numbers |
| `decimals` | number | slider | Display precision (default `2`) |

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

---

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

| Field | Type | Description |
|---|---|---|
| `quads` | array | Array of quad specification tables (see below). At least one quad required. |
| `key` | string | Optional cache key. If provided, calling `model.build` again with the same key returns the existing handle. |

Each quad in `quads` is a table with:

| Field | Type | Default | Description |
|---|---|---|---|
| `texture` | string | — | Texture path for this quad (batched by texture for rendering). |
| `r`, `g`, `b` | float | `1.0` | Vertex color (clamped 0–1). |
| `a` | float | `1.0` | Alpha (clamped 0–1). |
| `shade` | float | `1.0` | Diffuse shading multiplier (clamped 0–1). |
| `vertices` | array of 4 tables | (required) | Four vertex specification tables (see below). |

Each vertex in `vertices`:

| Field | Type | Description |
|---|---|---|
| `x`, `y`, `z` | float | Vertex position in model-local space. |
| `u`, `v` | float | Texture coordinates (0–1 range). |

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

| Field | Type | Default | Description |
|---|---|---|---|
| `cells` | array | (required) | Array of cell specifications (see below). |
| `resolution` | int | `16` | Grid resolution (voxel grid divisions per unit). |
| `origin_x`, `origin_y`, `origin_z` | float | `0` | Origin offset for the model. |
| `scale` | float | `1/resolution` | Size of each voxel in model units. |
| `key` | string | — | Optional cache key for model reuse. |

Each cell in `cells`:

| Field | Type | Default | Description |
|---|---|---|---|
| `x`, `y`, `z` | int | (required) | Lattice coordinates in the voxel grid. |
| `r`, `g`, `b` | float | `1.0` | Per-cell color. |
| `a` | float | `1.0` | Per-cell alpha. |

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

| Field | Type | Default | Description |
|---|---|---|---|
| `texture` | string | (required) | Path to the texture to sample. |
| `atlas_index` | int | `-1` | If ≥ 0 and `mod_texture` is false, reads from the vanilla terrain atlas at this index. |
| `mod_texture` | bool | `false` | If true, treats the texture as a mod texture (full image, not atlas). |
| `grid` | int | `16` | Sampling grid size (e.g. 16 = 16×16 cells). |
| `alpha_cutoff` | int | `30` | Alpha threshold (0–255). Pixels with alpha > this value become voxels. |

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

---

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
