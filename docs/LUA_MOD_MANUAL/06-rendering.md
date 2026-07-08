# Volume VI — Rendering

Sky, stars, clouds, fog tinting, world-space batches, and block/item tessellation.

---

## Draw contexts

| Context | Valid APIs | Active during |
|---------|------------|---------------|
| World render | `minecraft.render.*` | `world_render` callback |
| Block/item model | `minecraft.tessellator.quad` | manual `model.draw` / `inventory` |
| GUI | `minecraft.gui.*` | `screen_event` render, `screen_region` render |
| GUI 3D viewport | `minecraft.gui.begin_3d` … `draw_3d` | inside begin/end block |

**No raw GL.** Attempting draw outside these scopes is silently ignored or returns zero.

---

## `world_render` event

Filter:

```lua
minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.sky,   -- sky | stars | clouds
  moment = minecraft.render.moments.before,  -- before | after
  is_overworld = true,
}, fn)
```

### Mutable fields (summary)

| Field | When writable |
|-------|---------------|
| `cancel_vanilla` | `before` only |
| `celestial_angle`, `sky_yaw_deg` | sky + before |
| `astronomy_enabled`, `astronomy_utc_millis` | sky + before |
| `observer_latitude_deg`, `observer_longitude_deg` | sky + before |

### Read-only camera fields

`camera_x/y/z`, `camera_yaw/pitch/roll`, `custom_camera`, `tick_delta`, `celestial`, `world_time`, `is_night`, `star_brightness`, `rain_strength`, `stars_enabled`, `cloud_base_height` (clouds stage).

---

## `world_color` event

Composable RGB chain for sky and fog:

```lua
minecraft.on(minecraft.events.world_color, {
  kind = minecraft.colors.fog,  -- or .sky
  is_overworld = true,
}, function(event)
  event.r = event.r * 0.5
  event.g = event.g * 0.5
  event.b = event.b * 0.5
end)
```

Each callback receives RGB already modified by lower-priority (earlier) callbacks. Components clamped 0..1 on write.

Extra reads: `partial_ticks`, `celestial`, `world_time`, `is_night`, world context.

---

## `minecraft.render.quads`

**Only inside `world_render`.** Returns quad count emitted.

```lua
local count = minecraft.render.quads({
  texture = "/terrain.png",  -- optional; omit for untextured
  texture_id = 0,              -- optional raw GL texture id (e.g. camera.get_texture); texture path wins if both set
  blend = true,                -- default true
  cull = false,                -- default false
  depth_test = true,
  depth_write = true,
  r = 1, g = 1, b = 1, a = 1,  -- default vertex color
  vertices = {
    { x=0, y=64, z=0, u=0, v=0 },
    { x=1, y=64, z=0, u=1, v=0 },
    { x=1, y=65, z=0, u=1, v=1 },
    { x=0, y=65, z=0, u=0, v=1 },
    -- multiples of 4 vertices = multiple quads
  },
})
```

- Max **65536** vertices per batch (truncated to multiple of 4).
- Per-vertex `r,g,b,a` override defaults.
- Untextured vertices omit `u,v`.

---

## `minecraft.render.billboards`

Spherical sky billboards (stars, meteors). Returns billboard count.

```lua
minecraft.render.billboards({
  brightness = 1.0,           -- skip draw if <= 0
  rotation_x_rad = 0,
  rotation_y_rad = 0,
  blend = "alpha",              -- or "additive"
  depth_test = false,
  depth_write = false,
  billboards = {
    { yaw_deg = 45, pitch_deg = 30, size = 0.2, alpha = 1.0 },
    { az = 90, el = 10, size = 0.15 },  -- aliases
  },
})
```

`points` is alias for `billboards` array key.

Billboards render at radius 100 in camera-centered spherical coordinates.

---

## `minecraft.camera` — render-to-texture targets

A generic offscreen-render primitive: allocate an FBO, render a world view from an arbitrary
transform into it on demand, and sample the result as a texture (the camera entity renders with
skin). Native has no concept of "channels", "active", or update intervals — it only renders when
told to, once, synchronously. All policy (which target to show, how often to refresh it, whether
anything is currently displaying it) belongs in the calling mod's own Lua.

| Function | Returns | Notes |
|----------|---------|-------|
| `create(w, h)` | handle or -1 | Allocates an FBO render target, returns its handle |
| `destroy(handle)` | bool | |
| `render(handle, x,y,z, yaw,pitch,roll, fov, tick_delta?)` | bool | Renders the world from this transform into the target **right now**; call once per desired refresh, not every tick |
| `texture(handle)` | GL id or -1 | Feed to `render.quads{texture_id=}` / `gui.draw_texture` |
| `rendering()` | handle or -1 | Handle currently being rendered into, if any; never sample a target's texture while it is the one currently rendering (GPU read/write hazard) |

`render()` does a full scene pass — call it only for targets something is actually displaying,
and only as often as needed (e.g. every 2nd-3rd frame for a background TV feed, every frame only
while a mod's own close-up viewfinder UI is open). Use `minecraft.events.render_targets` (fires
once per real frame, before the main view renders) as the natural place to decide whether to call
`render()` this frame.

---

## Dynamic textures

```lua
local tex = minecraft.render.create_texture(width, height, argb_array)
-- or create_texture({ width, height, values = { 0xFFRRGGBB, ... } })

-- tex = { id = int, width = int, height = int }

minecraft.render.release_texture(tex.id)  -- bool; only mod-owned ids
```

- `values` length must be `width * height`.
- Mod-owned textures auto-released on mod unload.
- Use `gui.texture_id(path)` for atlas textures; `create_texture` for procedural pixels.

---

## Get texture pixels

```lua
local image = minecraft.render.get_texture_pixels(path_or_id)
-- returns { width = int, height = int, pixels = { 0xFFRRGGBB, ... } } or nil
```

- Accepts a texture resource path (e.g. `"gui/items.png"`, `"terrain.png"`, `"mods/my_mod/texture.png"`) or an OpenGL texture ID.
- Returns a table containing `width`, `height`, and a flat `pixels` array of 32-bit ARGB values.
- Returns `nil` if the texture cannot be loaded or found.

---

## `minecraft.tessellator.quad`

For **manual block/item models** (`model.type = "manual"`):

```lua
minecraft.tessellator.quad({
  texture = "mods/foo/block.png",  -- or texture_id = N
  r = 1, g = 1, b = 1, a = 1,
  vertices = {
    { x, y, z, u, v },
    { x, y, z, u, v },
    { x, y, z, u, v },
    { x, y, z, u, v },
  },
})  -- returns bool
```

Coordinates in **block space** 0..1 for blocks; item space for items. Emits to active block or item tessellator context.

---

## Astronomy helper

```lua
local az, alt = minecraft.astronomy.horizontal_from_equatorial(
  ra_hours, dec_degrees,
  minecraft.time.utc_millis(),
  latitude_deg, longitude_deg)
```

- Azimuth: degrees east from north (0..360).
- Altitude: degrees above horizon.
- Used by `northern_stars`, `realtime_sky`.

---

## Realtime clock on sky

On `world_render` sky **before**, set:

```lua
event.astronomy_enabled = true
event.astronomy_utc_millis = utc_ms
event.observer_latitude_deg = lat
event.observer_longitude_deg = lon
```

Engine uses these for sun/moon placement when astronomy mode is on.

---

## Layered clouds pattern

`layered_clouds` cancels vanilla clouds (`cancel_vanilla = true`) and draws multiple `render.quads` layers at different Y offsets in `world_render` clouds stage. Settings via `screen.settings` + legacy cfg file.

---

## Northern stars pattern

1. Load `read_nbt_asset("assets/star_catalog.nbt")` — arrays `right_ascension_hours`, `declination_degrees`, `magnitude`.
2. Each frame (or when observer moves), convert RA/Dec → `yaw_deg`/`pitch_deg` via `astronomy.horizontal_from_equatorial`.
3. Submit `render.billboards` on `stars` stage `after` with magnitude→size/alpha mapping.

---

## Void fog pattern

Track `camera_y` on `client_tick`; multiply fog RGB on `world_color` kind `fog` by darkness factor below Y=16. No custom geometry.

---

*See [Volume II](02-events-reference.md) for full `world_render` / `world_color` field tables.*
