# 06 — Rendering

## World Rendering Pipeline

The engine renders the world in a fixed sequence of stages. For each stage the
`world_render` event fires twice: **before** the stage runs and **after** it
completes. Mods can hook these moments to inject custom geometry or override
vanilla rendering.

### `minecraft.render.stages`

Constant table with one entry per stage (value equals its own key):

| Constant | Stage |
|---|---|
| `"sky"` | Sky background |
| `"stars"` | Starfield |
| `"terrain_opaque"` | Opaque terrain (chunk geometry) |
| `"entities"` | All living entities and items |
| `"particles_lit"` | Lit particles (torches, glowstone, etc.) |
| `"particles"` | Regular particles |
| `"terrain_translucent"` | Translucent terrain (water, glass, ice) |
| `"weather"` | Rain and snow |
| `"clouds"` | Clouds |
| `"hand"` | First-person hand |
| `"framebuffer"` | Framebuffer blit / post-processing |

Usage:

```lua
minecraft.on(minecraft.events.world_render, { stage = minecraft.render.stages.entities, moment = minecraft.render.moments.before },
  function(event)
    -- Draw custom geometry before entities render
  end)
```

### `minecraft.render.moments`

| Constant | Meaning |
|---|---|
| `"before"` | Fire just before vanilla renders the stage |
| `"after"` | Fire just after vanilla renders the stage |

The `world_render` event also exposes the following fields on the event table:

| Field | Type | Description |
|---|---|---|
| `world` | World | The current world |
| `camera` | Entity | The active camera entity |
| `tick_delta` | number | Partial tick for interpolation |
| `stage` | string | The current render stage |
| `moment` | string | `"before"` or `"after"` |
| `cancel_vanilla` | boolean | Set to `true` to skip vanilla rendering for this stage |
| `vanilla_stage_ran` | boolean | Whether vanilla already rendered this stage |
| `shadow_pass` | boolean | `true` while an offscreen shadow-depth pass renders the `entities` stage |
| `celestial_angle` | number | Current sun angle (0.0–1.0) |
| `sky_yaw_deg` | number | Skybox rotation in degrees |
| `star_brightness` | number | Current star brightness (stars/before; writable) |
| `rain_strength` | number | Rain intensity (0.0–1.0) |
| `stars_enabled` | boolean | Whether stars are enabled in the sky |
| `astronomy_enabled` | boolean | Whether astronomy mode is active |
| `astronomy_utc_millis` | number | UTC epoch milliseconds for astronomy |
| `observer_latitude_deg` | number | Observer latitude in degrees |
| `observer_longitude_deg` | number | Observer longitude in degrees |

---

## `minecraft.render.*`

### `minecraft.render.quads(spec)`

Draw textured or colored quads in world space. Only works when a world draw
context is active (i.e. inside a `world_render` callback). The camera offset
is handled automatically — position values are world-absolute unless
`world_space` is specified.

**spec table fields:**

| Field | Type | Default | Description |
|---|---|---|---|
| `texture` | string | `""` (untextured) | Path to a texture to bind |
| `texture_id` | int | `-1` | Raw GL texture ID (ignored if `texture` is set) |
| `r` | number | `1.0` | Default red tint (0–1) for all vertices |
| `g` | number | `1.0` | Default green tint (0–1) |
| `b` | number | `1.0` | Default blue tint (0–1) |
| `a` | number | `1.0` | Default alpha (0–1) |
| `x` | number | `0.0` | World-space X position (anchor) |
| `y` | number | `0.0` | World-space Y position (anchor) |
| `z` | number | `0.0` | World-space Z position (anchor) |
| `yaw` | number | `0.0` | Yaw rotation in degrees (around Y axis) |
| `pitch` | number | `0.0` | Pitch rotation in degrees (around X axis) |
| `roll` | number | `0.0` | Roll rotation in degrees (around Z axis) |
| `scale` | number | `1.0` | Uniform scale factor |
| `world_space` | boolean | `false` | If true, x/y/z are absolute world coords (camera is subtracted) |
| `blend` | boolean | `true` | Enable alpha blending |
| `cull` | boolean | `false` | Enable back-face culling |
| `depth_test` | boolean | `true` | Enable depth testing |
| `depth_write` | boolean | `true` | Enable depth buffer writes |
| `vertices` | array | required | Array of vertex tables |

**Vertex table fields** (each vertex can override the default tint):

| Field | Type | Description |
|---|---|---|
| `x` | number | Model-space X coordinate |
| `y` | number | Model-space Y coordinate |
| `z` | number | Model-space Z coordinate |
| `u` | number | Texture U coordinate (ignored when untextured) |
| `v` | number | Texture V coordinate (ignored when untextured) |
| `r` | number | Per-vertex red override (0–1) |
| `g` | number | Per-vertex green override (0–1) |
| `b` | number | Per-vertex blue override (0–1) |
| `a` | number | Per-vertex alpha override (0–1) |

If `texture` and `texture_id` are both absent/empty, quads are drawn without
a bound texture (colored only).

The vertex count is rounded down to the nearest multiple of 4. Returns the
number of quads drawn (integer), or `0` if no world draw context is active.

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

Draw always-facing quads (billboards) using 3D direction vectors. The engine
places them on a sphere about 100 world units from the camera; `x/y/z` are not
absolute world positions. Only works during a world draw context.

**spec table fields:**

| Field | Type | Default | Description |
|---|---|---|---|
| `brightness` | number | `1.0` | Brightness multiplier (0–1, ≤0 draws nothing) |
| `rotation_x_rad` | number | `0.0` | Additional X-axis rotation in radians |
| `rotation_y_rad` | number | `0.0` | Additional Y-axis rotation in radians |
| `blend` | string | `"alpha"` | Blend mode: `"alpha"` or `"additive"` |
| `depth_test` | boolean | `false` | Enable depth testing |
| `depth_write` | boolean | `false` | Enable depth buffer writes |
| `billboards` | array | required | Array of billboard specs (also accepts `points` as alias) |

**Billboard entry fields:**

| Field | Type | Default | Description |
|---|---|---|---|
| `x`, `y`, `z` | number | `0.0` | Direction vector components in 3D world space. Zenith = `(0,1,0)`, north ≈ `(0,0,-1)`, east ≈ `(1,0,0)`. |
| `size` | number | `0.2` | Billboard size (world units) |
| `alpha` | number | `1.0` | Alpha transparency (0–1) |

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

Suppress the native dropped-item (ItemEntity) sprite renderer so a Lua mod
can draw its own custom 3D model for ItemEntities instead. Pass `true` to
override, `false` to restore vanilla rendering.

```lua
minecraft.render.set_item_entity_override(true)
```

---

## `minecraft.tessellator.*`

### `minecraft.tessellator.quad(spec)`

Emit a single textured/colored quad to the currently active world tessellator
(block or item draw). Must be called during a block or item model callback
(i.e. from a model function registered via `register_block` or `register_item`).

**spec table fields:**

| Field | Type | Default | Description |
|---|---|---|---|
| `texture` | string | `""` | Texture path (overrides `texture_id`) |
| `texture_id` | int | `-1` | Raw texture ID |
| `r` | number | `1.0` | Red tint (0–1) |
| `g` | number | `1.0` | Green tint (0–1) |
| `b` | number | `1.0` | Blue tint (0–1) |
| `a` | number | `1.0` | Alpha (0–1) |
| `vertices` | array | required | Exactly 4 vertex tables `{x, y, z, u, v}` |

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

Positions are in the block/item's local model space. The tessellator handles
atlas UV mapping automatically.

---

## `minecraft.camera.*` (Render Targets / Viewfinder Cameras)

Create and manage render targets (offscreen framebuffers) backed by the
`GameRenderer` pipeline. Use these to render the world to a texture for
viewfinder displays, CCTV feeds, etc.

| Function | Signature | Returns |
|---|---|---|
| `create` | `(width: int, height: int, colorCount?: int, useDepthTex?: bool)` | handle (int) or `-1` |
| `create_display_size` | `(colorCount?: int, useDepthTex?: bool)` | handle (int) or `-1` |
| `destroy` | `(handle: int)` | `bool` |
| `resize` | `(handle: int, width: int, height: int)` | `bool` |
| `render` | `(handle: int, x, y, z, yaw, pitch, roll, fov, tickDelta?: number)` | `bool` |
| `texture` | `(handle: int, attachmentIndex?: int)` | GL texture ID (int) |
| `width` | `(handle: int)` | width (int) |
| `height` | `(handle: int)` | height (int) |
| `rendering` | `()` | currently bound handle (int), `-1` if none |
| `unbind` | `()` | `bool` |

### `camera.create(width, height, colorCount?, useDepthTex?)`

Create a new render target. `colorCount` defaults to 1.
`useDepthTex` (default `false`) attaches a depth texture instead of a
renderbuffer, allowing the depth buffer to be sampled in shaders.

Returns a numeric handle, or `-1` if creation fails.

### `camera.create_display_size(colorCount?, useDepthTex?)`

Create a render target sized to the current display (window) dimensions.

### `camera.destroy(handle)`

Destroy a previously created render target. Returns `true` on success.

### `camera.resize(handle, width, height)`

Resize the target. The existing color attachment count is retained.
Returns `true` on success.

### `camera.render(handle, x, y, z, yaw, pitch, roll, fov, tickDelta?)`

Render the world into the target from the given camera pose. `tickDelta`
defaults to `1.0`. Returns `true` on success.

```lua
local cam = minecraft.camera.create(320, 240)
minecraft.camera.render(cam, player.x, player.y, player.z, player.yaw, player.pitch, 0, 70)
local texId = minecraft.camera.texture(cam)
```

### `camera.texture(handle, attachmentIndex?)`

Get the OpenGL texture ID for the given attachment index (default `0`).
Returns `-1` if the handle is invalid.

### `camera.width(handle)` / `camera.height(handle)`

Get the render target dimensions. Returns `0` for invalid handles.

### `camera.rendering()`

Returns the handle of the currently bound render target, or `-1` if none.

### `camera.unbind()`

Unbind the currently bound render target. Returns `true`.

---

## `minecraft.fbo.*` (Offscreen Framebuffers)

Raw OpenGL framebuffer objects for custom render passes. Separate from
`minecraft.camera` — these are not tied to `GameRenderer` and do not run
the world render pipeline. Use them for post-processing, shadow maps, or
custom shader operations.

| Function | Signature | Returns |
|---|---|---|
| `create` | `(width: int, height: int, colorCount?: int, useDepthTex?: bool)` | handle (int) or `-1` |
| `create_display_size` | `(colorCount?: int, useDepthTex?: bool)` | handle (int) or `-1` |
| `destroy` | `(handle: int)` | `bool` |
| `resize` | `(handle: int, width: int, height: int)` | `bool` |
| `bind` | `(handle: int)` | `bool` |
| `unbind` | `()` | `bool` |
| `texture` | `(handle: int, attachmentIndex?: int)` | GL texture ID (int) |
| `width` | `(handle: int)` | width (int) |
| `height` | `(handle: int)` | height (int) |
| `bound` | `()` | currently bound handle (int), `-1` if none |

Parameters follow the same conventions as `minecraft.camera.*` (colorCount
defaults to 1, useDepthTex defaults to `false`).

```lua
local fbo = minecraft.fbo.create(512, 512)
minecraft.fbo.bind(fbo)
-- render custom geometry here
minecraft.fbo.unbind()
local fboTex = minecraft.fbo.texture(fbo)
```

---

## `minecraft.model.*`

Baked model management: load JSON models, build procedural models from quads,
place instances with physics hitboxes, and draw them in world space.

### `minecraft.model.load(path)`

Load and bake a JSON model from a mod's assets. The path is relative to the
mod's asset root (e.g. `"mymod:models/block/myblock.json"`). Parent chains
are resolved automatically. Results are cached by `(modId, path)`.

Returns the model handle (integer ≥ 1) on success, or `nil, error` on failure.

```lua
local handle, err = minecraft.model.load("mymod:models/block/myblock.json")
if not handle then print("load failed:", err) end
```

### `minecraft.model.build(spec)`

Build a baked model programmatically from an array of quads. Supports caching
via the `key` field.

**spec table fields:**

| Field | Type | Default | Description |
|---|---|---|---|
| `quads` | array | required | Array of quad specifications |
| `key` | string | `""` | Optional cache key — reuse builds with the same key return the same handle |

**Quad specification fields:**

| Field | Type | Default | Description |
|---|---|---|---|
| `texture` | string | `""` | Texture path for this quad |
| `r` | number | `1.0` | Red tint (0–1) |
| `g` | number | `1.0` | Green tint (0–1) |
| `b` | number | `1.0` | Blue tint (0–1) |
| `a` | number | `1.0` | Alpha (0–1) |
| `shade` | number | `1.0` | Directional shading factor (0–1) |
| `vertices` | array | required | Exactly 4 vertex tables `{x, y, z, u, v}` |

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

Place an instance of a baked model in the world. The instance creates a
hitbox that the engine's raycast system honors (via `raycast` event if the
mod implements it). The bounding box is derived from the model's baked bounds
and the transform's scale.

**opts fields:**

| Field | Type | Default | Description |
|---|---|---|---|
| `x` | number | `0.0` | World X position |
| `y` | number | `0.0` | World Y position |
| `z` | number | `0.0` | World Z position |
| `yaw` | number | `0.0` | Yaw rotation in degrees |
| `pitch` | number | `0.0` | Pitch rotation in degrees |
| `roll` | number | `0.0` | Roll rotation in degrees |
| `scale` | number | `1.0` | Uniform scale factor |
| `pivot_y` | number | `0.0` | Y offset of the rotation pivot in model space |
| `tag` | string | `""` | Arbitrary string tag (accessible in raycast results) |

Returns the instance ID (integer ≥ 1) on success, or `nil, error` on failure.

```lua
local instanceId = minecraft.model.place(handle, { x = 100, y = 64, z = 100, scale = 2, tag = "landmark" })
```

### `minecraft.model.update(instanceId, opts)`

Update an existing placed instance's transform. Same `opts` fields as `place`
(except `tag`). Omitted transform fields reset to defaults (`x/y/z = 0`, angles
= `0`, `scale = 1`, `pivot_y = 0`); pass every field that must be kept.
Returns `true` on success.

```lua
minecraft.model.update(instanceId, { x = 100, y = 70, z = 100, yaw = 45,
  pitch = 0, roll = 0, scale = 1, pivot_y = 0 })
```

### `minecraft.model.remove(instanceId)`

Remove a placed instance. Returns `true` on success.

### `minecraft.model.clear()`

Remove all placed model instances belonging to the current mod.

### `minecraft.model.bounds(handle)`

Get the model-space bounding box of a baked model. Returns a table with
`min_x`, `min_y`, `min_z`, `max_x`, `max_y`, `max_z` (all floats), or `nil`
if the model has no bounds.

### `minecraft.model.draw(handle, opts)`

Draw a baked model in world space immediately. Only works during a world
draw context (`world_render` event). The camera offset is handled
automatically. Returns `true` if the model was drawn, `false` if no client
renderer is active or the handle is invalid.

**opts fields:**

| Field | Type | Default | Description |
|---|---|---|---|
| `x` | number | `0.0` | World X position |
| `y` | number | `0.0` | World Y position |
| `z` | number | `0.0` | World Z position |
| `yaw` | number | `0.0` | Yaw rotation in degrees |
| `pitch` | number | `0.0` | Pitch rotation in degrees |
| `roll` | number | `0.0` | Roll rotation in degrees |
| `scale` | number | `1.0` | Uniform scale factor |
| `pivot_y` | number | `0.0` | Y offset of the rotation pivot in model space |
| `brightness` | number | world light | Brightness multiplier (0–1); omitted samples light at `x`, `y`, `z` |
| `a` | number | `1.0` | Alpha override (0–1, multiplied into each quad's alpha) |
| `blend` | boolean | `true` | Enable alpha blending |
| `cull` | boolean | `false` | Enable back-face culling |
| `depth_test` | boolean | `true` | Enable depth testing |
| `depth_write` | boolean | `true` | Enable depth buffer writes |

```lua
minecraft.on(minecraft.events.world_render, { stage = "entities", moment = "after" }, function()
  minecraft.model.draw(handle, { x = 100, y = 64, z = 100, yaw = 45, scale = 1.5 })
end)
```

### `minecraft.model.draw_item(item_id, damage, opts)`

Draw an item or block's 3D model in world space. Uses the same 3D model the
game would use for dropped item entities or inventory icons. For plain sprite
items (tools, food, etc.) that have no 3D shape, returns `false` — callers
should fall back to their own flat-icon representation. Omitted `brightness`
samples world light at the draw position. Same remaining `opts` fields as
`model.draw`.

```lua
local drew = minecraft.model.draw_item(1, 0, { x = player.x, y = player.y + 2, z = player.z, scale = 3 })
if not drew then
  -- draw a flat sprite icon instead
end
```

### `minecraft.model.item_bounds(item_id, damage)`

Get the model-space bounding box of an item's 3D model. Returns a table with
`min_x`, `min_y`, `min_z`, `max_x`, `max_y`, `max_z`, or `nil` if the item
has no real 3D shape (plain sprite items). Vanilla full-block items that lack
a custom baked model return an approximate unit-cube `{0,0,0,1,1,1}`.

```lua
local bounds = minecraft.model.item_bounds(1, 0)
if bounds then
  print("item bounds:", bounds.min_x, bounds.min_y, bounds.min_z, bounds.max_x, bounds.max_y, bounds.max_z)
end
```

---

## `minecraft.model.voxels(opts)`

Voxel model builder — constructs a baked model from integer-lattice cells.
Implemented in Lua on top of `minecraft.model.build`. Interior faces (shared
with a present neighbor) are automatically culled.

**opts fields:**

| Field | Type | Default | Description |
|---|---|---|---|
| `cells` | array | required | Array of cell specifications |
| `resolution` | int | `16` | Voxel grid resolution |
| `scale` | number | `1/resolution` | Size of one voxel in model units |
| `origin_x` | number | `0` | Model origin X offset |
| `origin_y` | number | `0` | Model origin Y offset |
| `origin_z` | number | `0` | Model origin Z offset |
| `key` | string | `nil` | Cache key for reuse |

**Cell specification:**

| Field | Type | Default | Description |
|---|---|---|---|
| `x` | int | required | Cell X coordinate on the integer lattice |
| `y` | int | required | Cell Y coordinate on the integer lattice |
| `z` | int | required | Cell Z coordinate on the integer lattice |
| `r` | number | `1.0` | Red tint (0–1) |
| `g` | number | `1.0` | Green tint (0–1) |
| `b` | number | `1.0` | Blue tint (0–1) |
| `a` | number | `1.0` | Alpha (0–1) |

Returns a model handle (integer) on success, or `nil, error` if no cells
were provided or all faces were culled.

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

---

## `minecraft.model.voxel(spec)`

Sprite extrude to voxel — samples a texture and extrudes opaque pixels into
a one-voxel-thick model centered on `z = 0.5`. Automatically cached by
`texture + atlas_index + mod_texture + grid`; `alpha_cutoff` is not part of the
cache key. Built in Lua on top of `model.voxels`.

**spec fields:**

| Field | Type | Default | Description |
|---|---|---|---|
| `texture` | string | required | Texture path to sample |
| `atlas_index` | int | `-1` | Atlas tile index (for non-mod textures); `-1` uses full texture |
| `mod_texture` | boolean | `false` | If true, the texture is a mod texture (different UV mapping) |
| `grid` | int | `16` | Sample grid resolution (e.g. 16 = 16×16 grid) |
| `alpha_cutoff` | int | `30` | Alpha threshold (0–255); pixels above this are kept |

Returns a model handle (integer) on success, or `nil, error` if the texture
is not found or has no opaque pixels.

```lua
local handle = minecraft.model.voxel({
  texture = "minecraft:textures/items/diamond.png",
  grid = 16,
  alpha_cutoff = 30,
})
```

---

## `minecraft.texture.*`

Read texture metadata and pixels. Textures are cached (up to 64 entries) for
performance.

### `minecraft.texture.size(path)`

Get the dimensions of a texture. Returns `{width: int, height: int}`. If the
texture cannot be loaded, both values are `0`.

```lua
local size = minecraft.texture.size("minecraft:textures/blocks/stone.png")
print(size.width, size.height)
```

### `minecraft.texture.pixel(path, x, y)`

Get the color of a single pixel. Returns `{r: int, g: int, b: int, a: int}`
with 0–255 values. Out-of-bounds coordinates return `{r=0, g=0, b=0, a=0}`.

```lua
local p = minecraft.texture.pixel("minecraft:textures/blocks/stone.png", 8, 8)
print(p.r, p.g, p.b, p.a)
```

---

## GUI 3D Viewport

Embed a 3D perspective viewport inside a GUI screen. The viewport uses its
own projection and model-view matrices, clear color, and camera pose.

### `gui.begin_3d(params)`

Begin a 3D viewport region. Must be called during a `screen_ui` render
callback (when `minecraft.gui` draw is active).

**params fields:**

| Field | Type | Default | Description |
|---|---|---|---|
| `x` | int | `0` | Viewport X position in GUI coordinates |
| `y` | int | `0` | Viewport Y position in GUI coordinates |
| `size` | int | `0` | Shortcut — sets both `width` and `height` |
| `width` | int | `0` | Viewport width in GUI coordinates (overrides `size`) |
| `height` | int | `0` | Viewport height in GUI coordinates (overrides `size`) |
| `gui_width` | int | display width | Total GUI width (used for scaling) |
| `gui_height` | int | display height | Total GUI height (used for scaling) |
| `yaw_deg` | number | `0.0` | Camera yaw in degrees |
| `pitch_deg` | number | `0.0` | Camera pitch in degrees |
| `distance` / `cam_dist` | number | `2.05` | Camera distance from origin (clamped 1.5–6.0) |
| `fov_deg` | number | `40.0` | Field of view in degrees (clamped 10–120) |
| `clear_color` | int | — | ARGB hex color for clear (e.g. `0xFF112233`); overrides individual clear fields |
| `clear_r` | number | `0.11` | Clear color red (0–1) |
| `clear_g` | number | `0.13` | Clear color green (0–1) |
| `clear_b` | number | `0.17` | Clear color blue (0–1) |
| `clear_a` | number | `1.0` | Clear color alpha (0–1) |

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

Draw immediate-mode geometry inside an active 3D viewport (between
`begin_3d` and `end_3d`).

**spec fields:**

| Field | Type | Default | Description |
|---|---|---|---|
| `mode` | string | required | Draw mode: `"lines"`, `"line_strip"`, `"line_loop"`, `"quads"`, `"quad_strip"`, `"points"`, `"triangles"` |
| `color` | int | — | ARGB hex color (overrides r/g/b/a) |
| `r` | number | `1.0` | Red (0–1) |
| `g` | number | `1.0` | Green (0–1) |
| `b` | number | `1.0` | Blue (0–1) |
| `a` | number | `1.0` | Alpha (0–1) |
| `line_width` | number | `1.0` | Line width (clamped 0.5–8.0) |
| `point_size` | number | `1.0` | Point size (clamped 1.0–16.0) |
| `vertices` | array | required | Array of `{x, y, z}` vertex tables |

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

Unproject a mouse position in GUI coordinates to a 3D ray (origin +
direction) in the viewport's world space. This is useful for mouse-picking
inside a 3D viewport.

Takes the same position/size/orientation params as `begin_3d`, plus `mouse_x`
and `mouse_y`. Returns `{ origin = {x, y, z}, direction = {x, y, z} }` or
`nil` if the mouse is outside the viewport.

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

---

## Render Events (Reference)

The following events modify rendering behavior. See `05-events.md` for
detailed documentation.

| Event | Purpose |
|---|---|
| `world_color` | Modify sky/fog color. `event.kind` is `"sky"` or `"fog"` (from `minecraft.colors`). Set `event.color` (Vec3d). |
| `camera_setup` | Override camera position, rotation, and roll. Fields: `x, y, z, yaw, pitch, roll`. Set `customView = true`. |
| `fov` | Override field of view. Set `event.fov` (float, default 70). |
| `first_person_hand` | Cancel or control first-person hand rendering. Set `canceled = true` to hide the hand. |
| `render_frame` | Start-of-frame hook. Fires once per frame before any world rendering. |
| `render_targets` | Post-render-targets hook. Fires after all render targets have been populated. |
| `world_render` | Per-stage render hooks. Fields: `stage`, `moment`, `cancel_vanilla`, etc. |
| `pre_entity_render` | Pre-entity-render hook. Set `canceled = true` to skip an entity. |
| `entity_render` | Entity render hook with pose control. Modify `event.pose` (bodyYaw, headYaw, headPitch, yaw, pitch, roll, scale, offsetX/Y/Z, parts) to override entity rendering. |
