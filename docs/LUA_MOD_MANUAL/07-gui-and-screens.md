# Volume VII — GUI and Screens

Drawing, vanilla screen injection, custom Lua screens, and 3D viewports.

---

## GUI draw context

`minecraft.gui.*` draw functions require **active GUI scope**:

- `screen_event` phase `render`
- `screen_region` phase `render`
- Inside `minecraft.screen.on_lua_screen` render handler

Calls outside scope return immediately (no error).

---

## `minecraft.gui` reference

### Primitives

| Function | Args | Notes |
|----------|------|-------|
| `fill_rect(x,y,w,h,color)` | ARGB int | Alpha 0 → treated as 255 |
| `draw_text(x,y,text,color)` | | Uses TextRenderer |
| `draw_centered_text(opts)` | `{x,y,width,text,color}` or `(x,y,width,text,color?)` | |
| `draw_item(x,y,item_id,count,damage?)` | | Vanilla item icon + count |
| `text_width(text)` | | Pixel width |
| `texture_id(path)` | | Returns GL texture id; 0 if missing |
| `draw_sprite(path,x,y,u,v,w,h)` | path or tex id first | Atlas sub-rectangle |
| `draw_texture(tex_id,x,y,w,h)` | | Full texture stretched |

### Vanilla-styled widgets

Support **table** or **positional** args. Table form accepts `mouse_x`/`mouse_y` for hover detection.

| Function | Table keys |
|----------|------------|
| `draw_button` | `x,y,width,height,text`, `active` (default true), `hovered` or mouse coords |
| `draw_slider` | `x,y,width,height`, `value` or `normalized`, `text`, hover |
| `draw_toggle` | `x,y,width,height`, `label`/`text`, `value`/`enabled`, hover |

Positional: `draw_button(x,y,w,h,text,active?,hover?)` etc.

---

## 3D viewport (`begin_3d`)

Embedded 3D preview on Lua screens (globe, model viewer):

```lua
minecraft.gui.begin_3d({
  x = 40, y = 30,
  width = 200, height = 200,   -- or size = N for square
  gui_width = display_w,       -- default: current display size
  gui_height = display_h,
  yaw_deg = 0,
  pitch_deg = 20,
  distance = 2.05,             -- alias cam_dist; clamped 1.5..6
  fov_deg = 40,                  -- clamped 10..120
  clear_color = 0xFF203040,      -- ARGB
  -- or clear_r, clear_g, clear_b, clear_a
})

minecraft.gui.draw_3d({
  mode = "line_strip",   -- lines | line_loop | quads | quad_strip | points | triangles
  color = 0xFFFFFFFF,    -- or r,g,b,a fields
  line_width = 1.0,      -- 0.5..8
  point_size = 1.0,      -- 1..16 for points
  vertices = {
    { x=0, y=0, z=0 },
    { x=1, y=0, z=0 },
    -- or array indices {x,y,z}
  },
})

minecraft.gui.end_3d()
```

### `unproject`

Mouse → ray in viewport model space:

```lua
local ray = minecraft.gui.unproject({
  mouse_x = event.mouse_x,
  mouse_y = event.mouse_y,
  -- same opts as begin_3d (x, y, width, height, yaw, pitch, distance, fov, gui sizes)
})
-- ray.origin = {x,y,z}
-- ray.direction = {x,y,z}
```

Returns `nil` if mouse outside viewport or GL unavailable.

---

## `minecraft.screen` — vanilla injection

### Constants

```lua
minecraft.screen.ids.create_world
minecraft.screen.ids.inventory
minecraft.screen.ids.detail_settings
minecraft.screen.ids.world_settings

minecraft.screen.regions.footer
minecraft.screen.regions.screen
minecraft.screen.regions.side_panel
```

### `screen_ui` event

```lua
minecraft.screen.on_ui(screen_id, region, function(event)
  event.ui.add_stacked_centered_button("My Settings", open_fn)
  event.ui.add_centered_button(y, text, w, h, callback)  -- y first in native; prelude wraps
  event.ui.add_button(x, y, w, h, text, callback)
  return event
end, priority)
```

**`event.host_fields`** — string map of host screen fields (seed text, world name, etc.).

### `screen_region` event

Draw/interact in a sub-rectangle of a vanilla screen:

```lua
minecraft.on(minecraft.events.screen_region, {
  screen_id = minecraft.screen.ids.inventory,
  region = minecraft.screen.regions.side_panel,
  phase_name = "render",  -- render | mouse_click | mouse_scroll
}, function(event)
  -- event.x, y, width, height — region bounds (width/height writable)
  minecraft.gui.fill_rect(event.x, event.y, event.width, event.height, 0x80000000)
end)
```

Set `event.handled = true` to consume clicks/scroll.

---

## Custom Lua screens

### Open/close

```lua
minecraft.screen.open("my_mod:screen", { title = "Title" })
minecraft.screen.close()
```

### Phase handlers

```lua
minecraft.screen.on_lua_screen("my_mod:screen", {
  init = function(event) end,      -- minecraft.screen.add_button/add_field valid here only
  render = function(event) end,
  tick = function(event) end,
  key = function(event) end,       -- event.key, event.char
  mouse = function(event) end,     -- event.button, event.released
  scroll = function(event) end,    -- event.delta
  close = function(event) end,
}, priority)
```

**Init-only APIs** (called as `minecraft.screen.<fn>(...)`, not `event.<fn>`; no-op outside `init`):

| Function | Description |
|----------|-------------|
| `minecraft.screen.add_field(name,x,y,w,h,text?,maxLen?,numeric?)` | Text field widget |
| `minecraft.screen.add_button(x,y,w,h,text,callback)` | Click button |

**Any phase** (also on `minecraft.screen`, not `event`):

| Function | Description |
|----------|-------------|
| `minecraft.screen.field_text(name)` | Read field |
| `minecraft.screen.set_field_text(name, text)` | Write field |
| `minecraft.screen.set_fields_visible(bool)` | Show/hide field widgets |

### Host screens

```lua
minecraft.screen.open_host("create_world", { seed = "12345" })
local seed = minecraft.screen.host_field("seed")
minecraft.screen.host_set_field("seed", "999")
```

---

## Prelude helpers

### `minecraft.screen.slots(spec)`

Declarative container UI with cursor and slot clicking:

```lua
local ui = minecraft.screen.slots({
  id = "my_mod:workbench",
  title = "Workbench",
  slots = 3,                    -- or positions = {{x,y}, ...}
  panel_width = 176,
  panel_height = 72,
  slot_y = 24,
  gap = 26,
  background = "mods/foo/panel.png",
  background_uv = { 0, 0, 176, 72 },
  label = "Repair",
  on_open = function(ctx) end,
  on_slot_change = function(ctx, index) end,
  on_tick = function(ctx) end,
  on_close = function(ctx) end,
})
ui.open()
-- ui.ctx.get(i), ctx.set(i, stack), ctx.slots(), ctx.count()
```

Uses `minecraft.stack.click` for vanilla slot rules. On close, returns items via `inventory.give`.

### `minecraft.screen.settings(spec)`

Two-column slider/toggle screen with parent button injection:

```lua
local open = minecraft.screen.settings({
  id = "my_mod:settings",
  title = "My Mod Settings",
  parent_screen = minecraft.screen.ids.detail_settings,
  parent_region = minecraft.screen.regions.footer,
  button_label = "My Mod...",
  values = function() return state end,  -- or static table
  sliders = {
    { key = "opacity", label = "Opacity", min = 0, max = 1, format = function(v) return string.format("%.2f", v) end },
  },
  toggles = {
    { key = "enabled", label = "Enabled" },
  },
  on_change = function() end,
  on_save = function() end,
  on_reset = function() end,  -- optional Reset button
  priority = 100,
})
```

---

## `camera_setup` vs GUI

| API | Use for |
|-----|---------|
| `minecraft.events.camera_setup` | Per-frame **world** camera (cinematics, roll) |
| `minecraft.gui.begin_3d` | **Screen** widget 3D preview |

Do not use `camera_setup` for inventory widgets.

---

## Patterns

### too_many_items

- Toggle with `key_press` on `O`
- `screen_region` on inventory `side_panel`: render item grid, handle scroll/click
- `minecraft.inventory.cursor_set` / `give` on click

### seedfinder

- Full custom `on_lua_screen` with manual layout, `sample_grid` preview texture via `create_texture`
- Keyboard navigation in `key` phase

### realtime_sky

- `globe_ui.lua`: `begin_3d`, coastline meshes, `unproject` for lat/lon picking
- `screen.settings` + legacy `realtime_sky.txt` config

---

*Event field tables: [Volume II](02-events-reference.md)*
