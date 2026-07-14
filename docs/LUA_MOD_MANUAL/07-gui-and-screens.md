# 07 — GUI and screens

## GUI draw scope

All `minecraft.gui.*` draw calls (fill_rect, draw_text, draw_item, draw_slider, draw_toggle, draw_button, draw_centered_text, draw_sprite, draw_texture, begin_3d, draw_3d, end_3d) **ONLY** function during a Lua GUI draw scope. These scopes are entered automatically during:

- The `render` phase of an `on_lua_screen` lifecycle handler
- The render callback of a `screen_ui` / `screen_region` subscription

Internally the engine tracks a depth counter (`g_luaGuiDepth`). `ScopedLuaGuiDraw` increments it on construction and decrements on destruction. Calling any draw function outside this scope silently produces no output (functions return early if `luaGuiDrawActive()` is false).

---

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

Draws a sprite (sub-rectangle) from a texture atlas or full texture. The first argument can be either:
- A **string** path (e.g. `"/gui/gui.png"`) — the engine resolves it to a texture ID internally
- An **integer** texture ID (from `texture_id()`)

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

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `x` | number | required | Left edge |
| `y` | number | required | Top edge |
| `width` | number | required | Width (> 0) |
| `height` | number | required | Height (> 0) |
| `text` | string | required | Button label |
| `active` | boolean | `true` | If false, drawn greyed out |
| `mouse_x` | number | auto | For hover detection (inferred from `hovered` or rect test) |
| `mouse_y` | number | auto | For hover detection (inferred from `hovered` or rect test) |
| `hovered` | boolean | auto | Override hover state; if absent, computed from mouse position |

The button is drawn with the vanilla gui.png texture (9-slice), with text centered. Active + hovered = gold text (0xFFFFA0), active = light grey (0xFFE0E0E0), inactive = dark grey (0xFFA0A0A0).

```lua
minecraft.gui.draw_button({x=100, y=200, width=150, height=20, text="Click Me", active=true,
  mouse_x = event.mouse_x, mouse_y = event.mouse_y})
```

### `minecraft.gui.draw_slider({x, y, width, height, value, text})`

Draws a vanilla-style slider widget (e.g. options screen volume slider).

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `x` | number | required | Left edge |
| `y` | number | required | Top edge |
| `width` | number | required | Width (> 0) |
| `height` | number | required | Height (> 0) |
| `value` | number | `0.0` | Normalized position (0.0 — 1.0), clamped internally |
| `text` | string | required | Label rendered over the slider |
| `mouse_x` | number | auto | For hover detection |
| `mouse_y` | number | auto | For hover detection |

```lua
minecraft.gui.draw_slider({x=100, y=100, width=200, height=20, value=0.5, text="Volume: 50%",
  mouse_x = event.mouse_x, mouse_y = event.mouse_y})
```

### `minecraft.gui.draw_toggle({x, y, width, height, label, value})`

Draws a vanilla-style toggle/on-off button. Rendered as a button with the label followed by ": ON" or ": OFF". Uses the translated keys `"options.on"` / `"options.off"` for the state text.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `x` | number | required | Left edge |
| `y` | number | required | Top edge |
| `width` | number | required | Width (> 0) |
| `height` | number | required | Height (> 0) |
| `label` | string | required | Label prefix |
| `value` | boolean | `false` | Toggle state |
| `mouse_x` | number | auto | For hover detection |
| `mouse_y` | number | auto | For hover detection |

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

| Field/Arg | Type | Default | Description |
|-----------|------|---------|-------------|
| `x` | number | required | Left edge of centering region |
| `y` | number | required | Top edge |
| `width` / `w` | number | required | Width of centering region |
| `text` | string | required | Text to draw |
| `color` | number | `0xFFFFFFFF` | ARGB color |

```lua
minecraft.gui.draw_centered_text(200, 300, 400, "Centered!", 0xFFFFFFFF)
```

---

## 3D viewport (`gui.draw_3d`)

The 3D viewport system lets mods embed a perspective-rendered 3D scene (model viewer, minimap, etc.) inside a GUI.

### `minecraft.gui.begin_3d({params...})`

Begins a 3D viewport. Must be called within the GUI draw scope. Sets up a perspective camera, clears the sub-region, and pushes OpenGL matrices. Every `begin_3d` must be paired with `end_3d`. Viewports cannot be nested (global depth tracking).

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `x` | number | 0 | Left edge (GUI coordinates) |
| `y` | number | 0 | Top edge (GUI coordinates) |
| `width` | number | 0 | Viewport width |
| `height` | number | 0 | Viewport height |
| `size` | number | 0 | Shortcut — sets both width and height to this value |
| `gui_width` | number | display width | GUI coordinate system width (for scaling) |
| `gui_height` | number | display height | GUI coordinate system height (for scaling) |
| `yaw_deg` | number | 0 | Camera yaw rotation (degrees) |
| `pitch_deg` | number | 0 | Camera pitch rotation (degrees) |
| `distance` / `cam_dist` | number | 2.05 | Camera distance from origin (clamped 1.5–6.0) |
| `fov_deg` | number | 40 | Field of view in degrees (clamped 10–120) |
| `clear_color` | number | — | ARGB background clear color (overrides individual clear channels) |
| `clear_r` | number | 0.11 | Clear color red (0–1) |
| `clear_g` | number | 0.13 | Clear color green (0–1) |
| `clear_b` | number | 0.17 | Clear color blue (0–1) |
| `clear_a` | number | 1.0 | Clear color alpha (0–1) |

```lua
minecraft.gui.begin_3d({x=10, y=10, width=200, height=200, yaw_deg=45, pitch_deg=30, distance=3})
```

### `minecraft.gui.draw_3d({mode, color?, r?, g?, b?, a?, vertices, line_width?, point_size?}))

Draws a 3D primitive inside the current viewport. Can only be called between `begin_3d` / `end_3d`.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `mode` | string | required | Draw mode: `"lines"`, `"line_strip"`, `"line_loop"`, `"quads"`, `"quad_strip"`, `"points"`, `"triangles"` |
| `color` | number | — | ARGB color for all vertices (overrides r/g/b/a) |
| `r` | number | 1.0 | Red (0–1) |
| `g` | number | 1.0 | Green (0–1) |
| `b` | number | 1.0 | Blue (0–1) |
| `a` | number | 1.0 | Alpha (0–1) |
| `line_width` | number | 1.0 | Line width for line modes (clamped 0.5–8.0) |
| `point_size` | number | 1.0 | Point size for point mode (clamped 1.0–16.0) |
| `vertices` | table | required | Array of vertex tables, each with `{x, y, z}` (named or positional indices 1,2,3) |

```lua
minecraft.gui.draw_3d({
  mode = "line_loop",
  color = 0xFFFF0000,
  line_width = 2,
  vertices = {
    {x=0, y=0, z=0},
    {x=1, y=0, z=0},
    {x=1, y=1, z=0},
    {x=0, y=1, z=0},
  }
})
```

### `minecraft.gui.end_3d()`

Ends the current 3D viewport, restoring the previous OpenGL matrices and viewport.

```lua
minecraft.gui.end_3d()
```

### `minecraft.gui.unproject({viewport_params, mouse_x, mouse_y}))

Computes a 3D ray from the camera through a mouse position, without requiring an active viewport. Returns a table with `{origin={x,y,z}, direction={x,y,z}}`, or `nil` on failure.

Takes the same viewport parameter table as `begin_3d`, plus `mouse_x` and `mouse_y` in GUI coordinates.

```lua
local ray = minecraft.gui.unproject({
  x=10, y=10, width=200, height=200, yaw_deg=45, pitch_deg=30,
  mouse_x = event.mouse_x, mouse_y = event.mouse_y
})
if ray then
  -- ray.origin.x, ray.origin.y, ray.origin.z
  -- ray.direction.x, ray.direction.y, ray.direction.z
end
```

---

## `minecraft.screen.*`

### `minecraft.screen.open(screen_id, {title?, pause?})`

Opens a Lua-defined screen. `screen_id` is a string identifier unique to your mod. The optional options table:

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `title` | string | `""` | Title text drawn at the top of the screen |
| `pause` | boolean | `true` | Whether the screen pauses the game |

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

The screen must have been registered with the host's `HostScreenRegistry` by the engine; returns false if the ID is unknown.

### `minecraft.screen.add_field(name, x, y, width, height, {text?, max_len?, numeric?, signed?, decimal?}))

Adds a text input widget to the Lua screen. **Only valid during the `init` phase** of an `on_lua_screen` lifecycle — outside init it is silently ignored.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | string | required | Field identifier (used with `field_text`/`set_field_text`) |
| `x` | number | required | Left edge |
| `y` | number | required | Top edge |
| `width` | number | required | Width |
| `height` | number | 20 | Height |
| `text` | string | `""` | Initial text |
| `max_len` | number | 0 | Maximum character length (0 = unlimited) |
| `numeric` | boolean | `false` | Only allow numeric characters |
| `signed` | boolean | `false` | Allow leading `-` (only if numeric) |
| `decimal` | boolean | `false` | Allow `.` decimal separator (only if numeric) |

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

---

## `minecraft.screen.on_ui(screen_id, region, callback, priority?)`

Attaches a callback to a **host screen's region** (e.g. the footer of the options screen). This is a convenience wrapper around subscribing to the `screen_ui` event.

| Argument | Type | Description |
|----------|------|-------------|
| `screen_id` | string | One of `minecraft.screen.ids.*`, or any string |
| `region` | string | One of `minecraft.screen.regions.*` |
| `callback` | function | Receives an event with an `event.ui` context object |
| `priority` | number | Optional (default 0) |

The callback receives an event table with an `event.ui` object exposing the following methods (which require colon notation):

- `event.ui:add_centered_button(y, text, callback?)` — Add a centered button at pixel y
- `event.ui:add_stacked_centered_button(text, callback?)` — Add a button centered and stacked below the last one (auto-y)
- `event.ui:add_button(x, y, w, h, text, callback?)` — Add a button at exact position
- `event.ui.screen` — The underlying host screen object (read-only property)

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

| Constant | Value | Description |
|----------|-------|-------------|
| `regions.footer` | `"footer"` | Bottom region of host screens |
| `regions.screen` | `"screen"` | Main region, published by every screen's `init()` after building widgets |
| `regions.side_panel` | `"side_panel"` | Inventory side panel region |

The `"screen"` region is the most commonly used — every engine screen publishes to this region during its `init()` phase, allowing mods to add buttons/behavior to any GUI.

---

## `minecraft.screen.on_lua_screen(screen_id, handlers, priority?)`

Handles the lifecycle of a Lua-defined screen (opened via `minecraft.screen.open`). The `handlers` table can have these phases:

| Phase | Event fields | Description |
|-------|-------------|-------------|
| `init` | `{screen, width, height}` | Screen is initializing — add_field, add_button, and set title here |
| `render` | `{screen, mouse_x, mouse_y, tick_delta}` | Every frame — GUI draw calls work here |
| `mouse` | `{screen, x, y, button, released?}` | Mouse click/release events. `released` is true for release, false/absent for press |
| `key` | `{screen, character, key, handled?}` | Keyboard events. Set `handled = true` to prevent default close-on-escape |
| `tick` | `{screen}` | Called every game tick (20 Hz) |
| `scroll` | `{screen, x, y, scroll_delta}` | Mouse scroll events |
| `close` | `{screen}` | Screen is being removed |

```lua
minecraft.screen.on_lua_screen("my_mod:config", {
  init = function(event)
    event.screen.setTitle("Configure My Mod")
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

---

## Screen constants

### `minecraft.screen.ids.*`

All screen ID string constants for the engine's built-in screens:

| Constant | Value |
|----------|-------|
| `ids.login` | `"minecraft:login"` |
| `ids.title` | `"minecraft:title"` |
| `ids.game_menu` | `"minecraft:game_menu"` |
| `ids.multiplayer` | `"minecraft:multiplayer"` |
| `ids.connect` | `"minecraft:connect"` |
| `ids.disconnected` | `"minecraft:disconnected"` |
| `ids.downloading_terrain` | `"minecraft:downloading_terrain"` |
| `ids.death` | `"minecraft:death"` |
| `ids.chat` | `"minecraft:chat"` |
| `ids.sleeping_chat` | `"minecraft:sleeping_chat"` |
| `ids.confirm` | `"minecraft:confirm"` |
| `ids.create_world` | `"minecraft:create_world"` |
| `ids.select_world` | `"minecraft:select_world"` |
| `ids.edit_world` | `"minecraft:edit_world"` |
| `ids.world_settings` | `"minecraft:world_settings"` |
| `ids.world_save_conflict` | `"minecraft:world_save_conflict"` |
| `ids.inventory` | `"minecraft:inventory"` |
| `ids.crafting` | `"minecraft:crafting"` |
| `ids.dispenser` | `"minecraft:dispenser"` |
| `ids.double_chest` | `"minecraft:double_chest"` |
| `ids.furnace` | `"minecraft:furnace"` |
| `ids.sign_edit` | `"minecraft:sign_edit"` |
| `ids.options` | `"minecraft:options"` |
| `ids.video_options` | `"minecraft:video_options"` |
| `ids.detail_settings` | `"minecraft:detail_settings"` |
| `ids.keybinds` | `"minecraft:keybinds"` |
| `ids.mods` | `"minecraft:mods"` |
| `ids.achievements` | `"minecraft:achievements"` |
| `ids.stats` | `"minecraft:stats"` |
| `ids.lan` | `"minecraft:lan"` |
| `ids.lan_info` | `"minecraft:lan_info"` |
| `ids.server_mod_download` | `"minecraft:server_mod_download"` |
| `ids.fatal_error` | `"minecraft:fatal_error"` |
| `ids.out_of_memory` | `"minecraft:out_of_memory"` |

### `minecraft.screen.regions.*`

| Constant | Value | Description |
|----------|-------|-------------|
| `regions.footer` | `"footer"` | Screen footer area |
| `regions.screen` | `"screen"` | Main screen content region (published by every screen after init) |
| `regions.side_panel` | `"side_panel"` | Inventory side panel region |

---

## Settings DSL

### `minecraft.screen.settings({id, title, parent_screen?, parent_region?, button_label?, values?, sliders?, toggles?, on_change?, on_save?, on_reset?, priority?})`

Creates a complete settings screen with auto-generated UI. Returns the `open` function.

The function:
1. Attaches a button to `parent_screen`'s `parent_region` (default `regions.footer`) using `add_stacked_centered_button`
2. Creates a Lua screen with id `id` and auto-laid-out sliders/toggles
3. Renders sliders using `draw_slider` and toggles using `draw_toggle`
4. Handles mouse drag for sliders, clicks for toggles
5. Calls `on_save()` on "Done" click or ESC

| Option | Type | Description |
|--------|------|-------------|
| `id` | string | Screen ID (required) |
| `title` | string | Screen title |
| `parent_screen` | string | Screen ID to attach the settings button to |
| `parent_region` | string | Region on parent screen (default `"footer"`) |
| `button_label` | string | Label for the parent button |
| `values` | table or function | Mutable table of current values (key→value), or a function returning it |
| `sliders` | array of tables | Each: `{key, label?, min, max, integer?, format?}` |
| `toggles` | array of tables | Each: `{key, label?}` |
| `on_change` | function | Called when any value changes |
| `on_save` | function | Called on save/close |
| `on_reset` | function | If set, shows a "Reset to Defaults" button |
| `priority` | number | Event priority (default 100) |

Each slider entry:

| Field | Type | Description |
|-------|------|-------------|
| `key` | string | Key in `values()` table |
| `label` | string | Display label (defaults to key) |
| `min` | number | Minimum value |
| `max` | number | Maximum value |
| `integer` | boolean | Snap to integer values |
| `format` | function | Custom formatting: `format(value)` → string |

Each toggle entry:

| Field | Type | Description |
|-------|------|-------------|
| `key` | string | Key in `values()` table |
| `label` | string | Display label (defaults to key) |

```lua
local config = {
  volume = 0.8,
  fullscreen = false,
  fov = 70,
}

minecraft.screen.settings({
  id = "my_mod:settings",
  title = "My Mod Settings",
  parent_screen = minecraft.screen.ids.mods,
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

---

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
