# 06 - GUI Screens, Custom UI & Audio Engine API

BineCrabricCPP includes a GUI screen injection system, dynamic mod settings screen builder, 2D drawing primitives, offline session identity manager, and audio engine.

---

## 1. Screen System & Injection (`minecraft.screen`)

Inject custom UI elements into vanilla screens or register entirely custom Lua screens.

### Region Specifications & Subscriptions

Standard screen region hooks allow adding buttons to existing screens (e.g. `TitleScreen`, `OptionsMenu`):

```lua
minecraft.screen.on_ui("options", minecraft.screen.regions.footer, function(event)
  if event.ui ~= nil then
    event.ui:add_stacked_centered_button("Mod Settings", function()
      minecraft.screen.open("my_mod_settings")
    end)
  end
end)
```

### Custom Lua Screens

Register a custom screen using `screen.on_lua_screen(screen_id, handlers, priority)`:

```lua
minecraft.screen.on_lua_screen("my_custom_screen", {
  init = function(event)
    -- Screen initialized (event.width, event.height)
    minecraft.screen.add_button(event.width / 2 - 100, event.height - 30, 200, 20, "Close", function()
      minecraft.screen.close()
    end)
  end,
  render = function(event)
    -- Render loop (event.mouse_x, event.mouse_y)
    minecraft.gui.draw_centered_text(event.width / 2, 40, "Custom Screen Title", 0xFFFFFFFF)
  end,
  mouse = function(event)
    -- Mouse click events (event.x, event.y, event.button, event.released)
  end,
  key = function(event)
    -- Keyboard input events (event.key, event.character)
  end
}, 100)
```

---

## 2. Mod Settings Screen Builder (`minecraft.screen.settings`)

High-level declarative API for creating paginated mod settings menus.

### `minecraft.screen.settings(spec)`

- **Spec Fields**:
  - `id`: Screen ID string.
  - `title`: Header title string.
  - `parent_screen`: Screen ID string of parent menu (e.g. `"options"`).
  - `button_label`: Label for injection button in parent menu (e.g. `"Sprint Settings"`).
  - `values`: Function or table providing mutable config key-value pairs.
  - `sliders`: Array of slider specs `{ key, label, min, max, integer, format }`.
  - `toggles`: Array of toggle specs `{ key, label }`.
  - `on_change`: Callback invoked whenever any control is modified.
  - `on_save`: Callback invoked when user clicks "Done".
  - `on_reset`: Optional callback for resetting values to defaults.

#### Example

```lua
minecraft.screen.settings({
  id = "sprint_settings",
  title = "Sprint Mod Options",
  parent_screen = "options",
  button_label = "Sprint Options...",
  values = function() return sprint_config end,
  sliders = {
    { key = "speed_boost", label = "Speed Boost", min = 1.0, max = 2.5, format = function(v) return string.format("Speed: %.2fx", v) end }
  },
  toggles = {
    { key = "toggle_sprint", label = "Toggle Sprint" },
    { key = "particles", label = "Sprint Particles" }
  },
  on_change = function()
    minecraft.config.save("sprint.cfg", sprint_config)
  end
})
```

---

## 3. GUI Drawing Primitives (`minecraft.gui`)

Low-level 2D rendering primitives available inside screen render callbacks:

- **`draw_text(text, x, y, color)`**: Draws text at `(x, y)` using ARGB hex color (e.g. `0xFFFFFFFF`).
- **`draw_centered_text(x, y, text, color)`**: Horizontally centered text drawing.
- **`draw_rect(x, y, width, height, color)`**: Draws solid 2D rectangle.
- **`draw_textured_rect(spec)`**: Draws textured 2D quad `{ x, y, width, height, u, v, u_width, v_height, texture }`.
- **`draw_slider(spec)`**: Draws native slider widget `{ x, y, width, height, value, text, mouse_x, mouse_y }`.
- **`draw_toggle(spec)`**: Draws native toggle widget `{ x, y, width, height, label, value, mouse_x, mouse_y }`.

---

## 4. Session Identity API (`minecraft.session`)

Allows mods to query or override offline session identity for LAN/offline play:

- **`minecraft.session.set_offline_username(name)`**: Sets custom offline username.
- **`minecraft.session.clear_offline_username()`**: Resets offline username override.
- **`minecraft.session.is_offline_mode()`**: Returns `true` if client is running in offline mode.
- **`minecraft.session.get_offline_username()`**: Returns active offline username override.
- **`minecraft.session.get_username()`**: Returns current active session player name.
- **`minecraft.session.is_authenticated()`**: Returns `true` if Microsoft authentication is active.

---

## 5. Audio Engine API (`minecraft.sound` / `minecraft.audio`)

Sound effect registration and 3D positional audio playback:

### `minecraft.sound.register(id, path, kind)`
- **Signature**: `minecraft.sound.register(id: string, path: string, kind?: string) -> boolean, err_msg`
- **Parameters**:
  - `id`: Unique sound identifier string (e.g. `"my_mod:meteor_impact"`).
  - `path`: Audio file asset path (e.g. `"sounds/meteor.ogg"`).
  - `kind`: `"effect"` (default), `"streaming"`, or `"music"`.

### `minecraft.sound.play(id, volume, pitch)`
- **Signature**: `minecraft.sound.play(id: string, volume?: number, pitch?: number) -> boolean`
- Plays global non-positional audio effect (`volume` default `1.0`, `pitch` default `1.0`).

### `minecraft.sound.play_at(id, x, y, z, volume, pitch)`
- **Signature**: `minecraft.sound.play_at(id: string, x: number, y: number, z: number, volume?: number, pitch?: number) -> boolean`
- Plays 3D positional audio sample centered at world coordinates `(x, y, z)`.
