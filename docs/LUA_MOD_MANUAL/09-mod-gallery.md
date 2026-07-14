# 09 — Mod Gallery

This chapter walks through every example mod shipped with the Lua runtime, showing real production patterns for blocks, items, entities, GUIs, world generation, rendering, and more. Each section highlights the key APIs and design decisions used.

---

## 1. `simple_lantern` — Custom Block with Light

**Files:** `scripts/main.lua`, `mod.json`

Registers a single custom block with luminance, a custom model, and material properties.

```lua
local model = assert(minecraft.model.load("models/lantern.json"))

minecraft.register_block({
  id = 151,
  texture = "mods/simple_lantern/lantern.png",
  hardness = 0.5,
  resistance = 1.0,
  luminance = 0.9375,          -- light level (0.0–1.0)
  translation_key = "lantern",
  material = "metal",
  opaque = false,               -- transparent rendering
  full_cube = false,            -- non-full block shape
  model = model,                -- custom model handle
})
```

**mod.json:**
```json
{
  "id": "simple_lantern",
  "name": "Simple Lantern",
  "version": "1.0.0",
  "enabled": true,
  "entry": "scripts/main.lua"
}
```

**Key patterns:**
- `minecraft.model.load(path)` loads a JSON model; `assert()` ensures it succeeds.
- `luminance` is a 0–1 float; 0.9375 ≈ level 15.
- `opaque = false` and `full_cube = false` enable transparent/non-full-block rendering.
- `material` affects sound and tool effectiveness.

---

## 2. `stone_bricks` — Multiple Block Variants

**Files:** `scripts/main.lua`, `mod.json`

Registers a block with a crafting recipe.

```lua
minecraft.register_block({
  id = 98,
  texture_id = 7,            -- uses vanilla texture index 7 (stone)
  hardness = 2.0,
  resistance = 10.0,
  translation_key = "stoneBrick",
  material = "stone",
})

minecraft.register_shaped_recipe({
  output_block_id = 98,
  output_count = 4,
  pattern = { "##", "##" },
  key = "#",
  item_id = 1,               -- stone item ID
})

minecraft.log("info", "stone_bricks registered from Lua")
```

**Key patterns:**
- `texture_id` references a vanilla texture atlas index instead of a custom texture path.
- `register_shaped_recipe` supports 2×2 and 3×3 patterns with up to 3 keys (`key`, `key2`, `key3`).
- Use `minecraft.log("info", ...)` for debug output.

---

## 3. `coral` — Coral Simulation

**Files:** `scripts/main.lua`, `mod.json`

Full coral generation via `chunk_generation` with custom block registration and reef placement logic.

```lua
local CORAL_ID = 180
local WATER_ID = minecraft.world.block_id("water") or 0
local SAND_ID = minecraft.world.block_id("sand") or 0

minecraft.register_block({
  id = CORAL_ID,
  texture = "mods/coral/coral.png",
  hardness = 1.5,
  resistance = 10.0,
  translation_key = "coral",
  material = "stone",
  opaque = false,
  full_cube = false,
  requires_solid_below = false,
  coordinate_bounds = true,          -- random scale/offset
  coordinate_color = true,       -- tint from position
  bounds_padding = 0.0625,
  bounds_offset = 0.1,
  min_scale = 0.9,
  max_scale = 1.1,
  model = coral_model,
})

-- Chunk-level helpers
local function get_block(chunk, x, y, z)
  if not in_chunk(x, z) or y < 0 or y >= 128 then return 0 end
  return chunk:get_block(x, y, z)
end

local function set_block(chunk, x, y, z, id)
  if in_chunk(x, z) and y >= 0 and y < 128 then
    chunk:set_block(x, y, z, id)
  end
end

minecraft.on(minecraft.events.chunk_generation, {
  stage = minecraft.generation.stages.features,
  moment = minecraft.generation.moments.after,
  vanilla_stage_ran = true,
  when = minecraft.util.real_world,
  priority = 100,
}, function(event)
  if WATER_ID <= 0 or SAND_ID <= 0 or DIRT_ID <= 0 then return end
  if minecraft.world.random(16) ~= 0 then return end
  -- find water body and generate reef...
end)
```

**Key patterns:**
- `minecraft.world.block_id(name)` resolves vanilla block names to numeric IDs.
- `event.chunk:get_block` / `set_block` operate on the current chunk being generated (coordinates 0–15).
- `vanilla_stage_ran = true` ensures vanilla features ran first.
- `minecraft.util.real_world` guards against superflat/void worlds.
- `priority` controls execution order among multiple listeners.

---

## 4. `iron_bars` — Connected Textures Block

**Files:** `scripts/main.lua`, `mod.json`

Custom block with collision height and a crafting recipe.

```lua
local iron_bars_model = assert(minecraft.model.load("models/iron_bars.json"))

minecraft.register_block({
  id = 101,
  texture = "mods/iron_bars/iron_bars.png",
  hardness = 5.0,
  resistance = 10.0,
  translation_key = "fenceIron",
  material = "metal",
  collision_height = 1.5,       -- taller collision box
  opaque = false,
  full_cube = false,
  stack_on_same = true,         -- stacking behavior like fences
  model = iron_bars_model,
  item = {
    texture = "mods/iron_bars/iron_bars.png",
  },
})

minecraft.register_shaped_recipe({
  output_block_id = 101,
  output_count = 16,
  pattern = { "###", "###" },
  key = "#",
  item_id = 265,               -- iron ingot
})
```

**Key patterns:**
- `collision_height` sets a non-full-block collision box.
- The nested `item` table defines a separate texture for the inventory item.
- `stack_on_same` enables visual stacking when placed on the same block type.

---

## 5. `critical_hit` — Attack Damage Events

**Files:** `scripts/main.lua`, `mod.json`

Hooks the `attack_damage` event to modify critical-hit logic and spawn particles.

```lua
local function spawn_crit_particles(x, y, z)
  for _ = 1, 10 do
    minecraft.particles.spawn({
      x = x, y = y, z = z,
      vx = (math.random() - 0.5) * 0.4,
      vy = math.random() * 0.25 + 0.05,
      vz = (math.random() - 0.5) * 0.4,
      scale = 0.35,
      r = 1.0, g = 0.95, b = 0.4,
      max_age = 12,
      gravity = 0.04,
    })
  end
end

minecraft.on(minecraft.events.attack_damage, {
  has_player = true,
  has_target = true,
  priority = 100,
  when = function(event)
    return not event.on_ground and (event.fall_distance or 0.0) > 0.5 and event.critical
  end,
}, function(event)
  event.damage = math.max(event.damage + 1, math.floor(event.damage * 1.5 + 0.5))
  event.critical = true
  spawn_crit_particles(event.target_x, event.target_y, event.target_z)
end)
```

**Key patterns:**
- The `when` filter checks conditions before the handler runs.
- Mutating `event.damage` modifies the actual damage dealt.
- `minecraft.particles.spawn` takes position, velocity, color, scale, age, and gravity.
- `event.target_x/y/z` give the hit entity's position.

---

## 6. `sprint` — Keybinding & Player Travel

**Files:** `scripts/main.lua`, `mod.json`

Implements a double-tap-to-sprint mod using `client_tick`, `player_travel`, and `fov` events.

```lua
local SPRINT_MULTIPLIER = 1.45

minecraft.on(minecraft.events.client_tick, {
  before = false,
  after_world = false,
  priority = 100,
}, function()
  update_sprint_state()
end)

minecraft.on(minecraft.events.player_travel, {
  is_local_player = true,
  priority = 100,
}, function(event)
  if sprinting and event.forward > 0.0 then
    local multiplier = SPRINT_MULTIPLIER
    if sprint_boost_timer > 0 then
      multiplier = multiplier * START_BOOST_MULTIPLIER
      sprint_boost_timer = sprint_boost_timer - 1
    end
    event.speed_multiplier = (event.speed_multiplier or 1.0) * multiplier
  end
end)

minecraft.on(minecraft.events.fov, { priority = 100 }, function(event)
  if sprinting then
    event.fov = event.fov * 1.08
  end
end)
```

**Key patterns:**
- `minecraft.is_key_down(keyCode)` checks held keys during `client_tick`.
- `minecraft.key_code(name)` maps a key name to its scancode.
- `player_travel` with `is_local_player = true` fires only for the local player.
- Mutating `event.speed_multiplier` scales movement speed.
- The `fov` event lets you change the field-of-view for visual feedback.

---

## 7. `meteors` — Particle Effects & Key Press

**Files:** `scripts/main.lua`, `mod.json`

Spawns meteor particle showers randomly and on key press.

```lua
local CHANCE_TO_SPAWN = 4000
local METEOR_KEY = minecraft.key_code("m")

local function spawn_meteor_shower()
  local player = minecraft.world.player()
  if player == nil then return end
  local dist = 200.0 + math.random() * 100.0
  local angle = math.random() * PI * 2.0
  local start_x = player.x + math.cos(angle) * dist
  local start_z = player.z + math.sin(angle) * dist
  local start_y = 200.0 + math.random() * 100.0
  local speed = 3.0 + math.random() * 3.0
  for i = 1, 8 do
    local t = i / 8.0
    minecraft.particles.spawn({
      x = start_x - vel_x * t * 6.0,
      y = start_y - vel_y * t * 6.0,
      z = start_z - vel_z * t * 6.0,
      vx = vel_x, vy = vel_y, vz = vel_z,
      scale = 0.8 + math.random() * 0.6,
      r = 1.0, g = 0.95, b = 0.8,
      max_age = 80,
      gravity = 0.0,
    })
  end
end

minecraft.on(minecraft.events.client_tick, {
  before = false, paused = false, has_world = true, priority = 100,
}, function(event)
  if event.is_night and minecraft.world.random(CHANCE_TO_SPAWN) == 0 then
    spawn_meteor_shower()
  end
end)

minecraft.on(minecraft.events.key_press, {
  key = METEOR_KEY,
  pressed = true,
  handled = false,
  priority = 100,
}, function(event)
  spawn_meteor_shower()
  event.handled = true
end)
```

**Key patterns:**
- `client_tick` with `has_world = true` only fires when a world is loaded.
- `event.is_night` from `client_tick` tells the time of day.
- `key_press` with `handled = false` allows other mods to also handle the key.
- Setting `event.handled = true` prevents default processing.

---

## 8. `item_drop_physics` — Custom Physics Simulation

**Files:** `scripts/main.lua`, `scripts/box3d.lua`, `assets/item_physics.json`, `mod.json`

Full physics engine for dropped items. Hides vanilla entity rendering, runs a rigid-body sim, and draws tumbling 3D models.

### Main loop

```lua
minecraft.on(minecraft.events.pre_entity_render, { entity_type = "Item" }, function(event)
  event.canceled = true
  return event
end)

minecraft.on(minecraft.events.client_tick, {
  before = false, after_world = true, paused = false,
}, function(event)
  if not event.has_world then return end
  local list = minecraft.entities.list("Item")
  simulate_items(list)
  resolve_item_collisions(list)
  queue_server_sync(list)
end)

local function render_items(event)
  local d = event.tick_delta or 1.0
  for _, item in ipairs(minecraft.entities.list("Item") or {}) do
    local s = sims[item.id]
    if s then
      local q = box3d.quat_slerp(s.prev_orientation, s.body.orientation, d)
      local yaw, pitch, roll = box3d.quat_to_euler_degrees(q)
      local transform = {
        x = s.px + (s.x - s.px) * d,
        y = s.py + (s.y - s.py) * d,
        z = s.pz + (s.z - s.pz) * d,
        yaw = yaw, pitch = pitch, roll = roll,
        pivot_y = 0.5, scale = DRAW_SCALE,
      }
      if not minecraft.model.draw_item(item.item_id, item.item_damage or 0, transform) then
        local handle = voxel_handle(item)
        if handle then minecraft.model.draw(handle, transform) end
      end
    end
  end
end

minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.terrain_opaque,
  moment = minecraft.render.moments.after,
}, function(event)
  if not event.shadow_pass then render_items(event) end
end)

minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.entities,
  moment = minecraft.render.moments.after,
}, function(event)
  if not event.shadow_pass then return end
  render_items(event)
end)
```

`box3d.lua` supplies swept block collision, contact impulses, quaternion rotation, and AABB depenetration. `main.lua` adds iterative item-item contact resolution, including restitution, friction, resting support, and swept collision for fast throws.

**Entity fields used on Item entities:**
- `item_id`, `item_count`, `item_damage`, `texture_path`, `atlas_index`, `mod_texture`
- `x`, `y`, `z`, `vx`, `vy`, `vz`, `id`

**Key patterns:**
- `pre_entity_render` with `entity_type = "Item"` and `canceled = true` hides vanilla rendering.
- Server-side synchronization applies simulated item positions; client callbacks only queue updates.
- Render in `terrain_opaque/after` for normal shader lighting, then in `entities/after` only when `event.shadow_pass` is true to write the shadow map.
- `minecraft.render.set_item_entity_override(true)` can be used to take full control.
- `minecraft.model.draw_item(id, damage, transform)` draws the item's real 3D model.
- `minecraft.model.voxel({...})` creates a voxel model from a texture for fallback rendering.
- `minecraft.model.item_bounds(id, damage)` returns the model's AABB for physics.
- `minecraft.require("scripts.box3d")` provides collision and rigid-body math.
- `minecraft.world.get_block_collisions(query)` retrieves block collision AABBs.

---

## 9. `camera` — Viewfinder & FBO Screens

**Files:** `scripts/main.lua`, `mod.json`

Custom items (tripod, camera), a block (TV), model rendering, camera channels (render-to-texture), and a full-screen viewfinder.

### Registering custom items and blocks

```lua
minecraft.register_item({
  id = TRIPOD_ITEM,
  name = "Tripod",
  translation_key = "camera.tripod",
  max_count = 16,
  texture = "mods/camera/tripod.png",
})

minecraft.register_block({
  id = TV_BLOCK,
  texture = "mods/camera/tv_body.png",
  hardness = 1.5,
  resistance = 4.0,
  translation_key = "camera.tv",
  material = "metal",
  opaque = false,
  full_cube = false,
  model = tv_model,
  tile_entity = "tv",          -- tile entity type
  item = { texture = "mods/camera/tv_icon.png" },
})
```

### Placing mod entities

```lua
minecraft.entities.spawn_mod("camera:tripod", {
  x = x + 0.5, y = y, z = z + 0.5,
  yaw = event.player_yaw or 0,
  data = { mounted = 0 },
})
```

### Camera channels (render-to-texture)

```lua
local channel = minecraft.camera.create_display_size()

-- Render camera view to channel
minecraft.on(minecraft.events.render_targets, {}, function(event)
  minecraft.camera.render(channel, x, y + 0.4, z, camera.yaw, camera.pitch or 0, 0, 70, event.tick_delta)
end)
```

### Full-screen viewfinder with `screen_event`

```lua
minecraft.on(minecraft.events.block_interact, {
  right_click = true, block_id = TV_BLOCK
}, function(event)
  minecraft.screen.open(SCREEN_ID, { title = "Camera Viewfinder", pause = false })
end)

minecraft.on(minecraft.events.screen_event, { screen_id = SCREEN_ID }, function(event)
  if event.phase == "init" then
    -- Add channel selector buttons
    minecraft.screen.add_button(bx, by, 50, 20, "CH " .. i, function()
      client.active_camera_id = camera.id
    end)
  elseif event.phase == "render" then
    -- Draw camera feed texture
    local texture = minecraft.camera.texture(client.channels[id])
    minecraft.gui.draw_texture(texture, x, y, 256, 192)
  elseif event.phase == "close" then
    client.open = false
  end
end)
```

### Custom entity rendering

```lua
minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.entities,
  moment = minecraft.render.moments.after,
}, function(event)
  for _, entity in ipairs(client_entities("camera:tripod")) do
    minecraft.model.draw(tripod_bottom_model, {
      x = entity.x, y = entity.y, z = entity.z,
      yaw = entity.yaw, scale = MODEL_SCALE, pivot_y = 0
    })
  end
end)
```

**Key patterns:**
- `minecraft.camera.create_display_size()` creates a render target channel returning an ID.
- `minecraft.camera.render(channel, x, y, z, yaw, pitch, roll, fov, tickDelta)` renders the world to a texture.
- `minecraft.camera.texture(channel)` returns the OpenGL texture ID for GUI drawing.
- `minecraft.camera.destroy(channel)` cleans up.
- `render_targets` event is the correct place to call `camera.render`.
- `screen_event` phases include `init`, `render`, `tick`, `key`, `mouse`, `scroll`, and `close`; callbacks are event-driven.
- Tile entities accessed via `minecraft.tile_entities.get(x, y, z)`.

---

## 10. `too_many_items` — GUI & Inventory

**Files:** `scripts/main.lua`, `mod.json`

Creative item browser that adds a side panel to the inventory screen with scrollable grid, item rendering, and give functionality.

### Side panel region hook

```lua
minecraft.on(minecraft.events.screen_region, {
  screen_id = minecraft.screen.ids.inventory,
  region = minecraft.screen.regions.side_panel,
  priority = 100,
  when = function() return visible end,
}, function(event)
  -- phase_name is "render", "mouse_click", "mouse_scroll"
end)
```

### Rendering the item grid

```lua
minecraft.on(minecraft.events.screen_region, {
  screen_id = minecraft.screen.ids.inventory,
  region = minecraft.screen.regions.side_panel,
  phase_name = "render",
  priority = 100,
  when = function() return visible end,
}, function(event)
  local items = minecraft.items.ids()
  for row = 0, rows - 1 do
    for col = 0, cols - 1 do
      local index = scroll * cols + row * cols + col + 1
      local item_id = items[index]
      if item_id ~= nil then
        local sx = x + PADDING + col * SLOT
        local sy = y + GRID_Y_OFFSET + row * SLOT
        minecraft.gui.fill_rect(sx, sy, SLOT, SLOT, 0x80202020)
        minecraft.gui.draw_item(sx + 1, sy + 1, item_id, 1)
      end
    end
  end
end)
```

### Click handling and giving items

```lua
minecraft.on(minecraft.events.screen_region, {
  phase_name = "mouse_click",
}, function(event)
  local item_id = item_at(items, event, event.mouse_x, event.mouse_y)
  if item_id ~= nil then
    local count = event.button == 1 and 1 or 64
    minecraft.inventory.give({id = item_id, count = count})
    event.handled = true
  end
end)
```

### Toggle with keybind

```lua
minecraft.on(minecraft.events.key_press, {
  key = minecraft.key_code("o"),
  pressed = true,
  priority = 100,
}, function(event)
  visible = not visible
  event.handled = true
end)
```

**Key patterns:**
- `minecraft.screen.regions.side_panel` is a screen region on the inventory screen.
- `minecraft.items.ids()` returns all registered item IDs.
- `minecraft.gui.draw_item(x, y, id, count)` renders an item stack in GUI space.
- `minecraft.inventory.give({id, count})` adds items to the player inventory.
- `screen_region` events receive `mouse_x`, `mouse_y`, `width`, `height`, `scroll_delta`.

---

## 11. `world_profiles` — World Creation

**Files:** `scripts/main.lua`, `mod.json`

Adds world type profiles (Default, Flatlands, Highlands, Caves) with custom generation and `create_world` options injection.

### Hooking world creation

```lua
minecraft.on(minecraft.events.create_world, {}, function(event)
  event.options = event.options or {}
  event.options[PROFILE_OPTION] = selected_id()
  event.options[FLAT_HEIGHT_OPTION] = tostring(options.flat_height)
  active_profile = selected_profile()
end)
```

### Adding UI to the create-world screen

```lua
minecraft.screen.on_ui(minecraft.screen.ids.create_world,
  minecraft.screen.regions.footer, function(event)
  if event.ui ~= nil then
    event.ui:add_stacked_centered_button(profile_label(), cycle_profile)
  end
  return event
end, 100)
```

### Settings sliders via `minecraft.screen.settings`

```lua
minecraft.screen.settings({
  id = "world_profiles:options",
  title = "World Profile Options",
  parent_screen = minecraft.screen.ids.create_world,
  parent_region = minecraft.screen.regions.footer,
  button_label = "Profile Options...",
  values = function() return options end,
  sliders = {
    { key = "flat_height", label = "Flat Height", min = 1, max = 32, integer = true },
    { key = "highland_boost", label = "Highland Boost", min = 4, max = 40, integer = true },
    { key = "cave_min_y", label = "Cave Spawn Min Y", min = 8, max = 80, integer = true },
    { key = "cave_max_y", label = "Cave Spawn Max Y", min = 16, max = 100, integer = true },
  },
  priority = 90,
})
```

### Custom chunk generation

```lua
minecraft.on(minecraft.events.chunk_generation, {
  stage = minecraft.generation.stages.surface,
  moment = minecraft.generation.moments.after,
  when = minecraft.util.real_world,
  is_overworld = true,
  priority = 100,
}, function(event)
  local generate = profile_for_event(event).generate
  if generate ~= nil then generate(event) end
end)

-- Cancel vanilla for flatlands
minecraft.on(minecraft.events.chunk_generation, {
  stage = { minecraft.generation.stages.carver, minecraft.generation.stages.features },
  moment = minecraft.generation.moments.before,
  priority = 100,
}, function(event)
  event.cancel_vanilla = true
end)
```

### Spawn search customization

```lua
minecraft.on(minecraft.events.world_spawn_search, {
  resolved = false, is_overworld = true,
  when = minecraft.util.real_world, priority = 100,
}, function(event)
  local profile = profile_for_event(event)
  -- Override event.y based on profile
  event.y = y
  event.resolved = true
end)
```

**Key patterns:**
- `create_world` event lets you inject options that persist in world save.
- `world_open` with `event.new_world` tells if this is a freshly created world.
- `chunk_generation` with `cancel_vanilla = true` disables vanilla generation for that stage.
- `world_spawn_search` customizes where players first spawn.
- `minecraft.screen.settings({...})` creates a slider-based settings screen.
- `event.chunk:fill(x1, y1, z1, x2, y2, z2, block_id)` fills a volume in the generating chunk.

---

## 12. `void_fog` — Fog Color via `world_color`

**Files:** `scripts/main.lua`, `mod.json`

Darkens fog based on the player's Y-level to simulate void darkness.

```lua
minecraft.on(minecraft.events.client_tick, {
  before = false, paused = false, has_world = true, is_overworld = true, priority = 100,
}, function(event)
  last_camera_y = event.camera_y or 64.0
end)

minecraft.on(minecraft.events.world_color, {
  kind = minecraft.colors.fog,
  is_overworld = true,
  priority = 100,
}, function(event)
  local darkness = fog_darkness(last_camera_y)
  if darkness <= 0.0 then return end
  event.r = event.r * (1.0 - darkness)
  event.g = event.g * (1.0 - darkness)
  event.b = event.b * (1.0 - darkness)
end)
```

**Key patterns:**
- `world_color` with `kind = minecraft.colors.fog` (or `minecraft.colors.sky`) lets you modify ambient colors.
- Mutate `event.r`, `event.g`, `event.b` in 0–1 range.
- Camera Y is available from `client_tick` via `event.camera_y`.

---

## 13. `northern_stars` — Custom Star Rendering

**Files:** `scripts/main.lua`, `mod.json`

Renders a starfield using billboards from an NBT star catalog with astronomy-aware positions.

### Loading star catalog

```lua
local root, error_message = minecraft.read_nbt_asset("assets/star_catalog.nbt")
```

### Rendering stars as billboards

```lua
minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.stars,
  moment = minecraft.render.moments.before,
  is_overworld = true,
  priority = 150,
}, function(event)
  event.cancel_vanilla = true
  minecraft.render.billboards({
    brightness = event.star_brightness or 0.0,
    rotation_x_rad = rotation_x,
    rotation_y_rad = rotation_y,
    blend = "additive",
    depth_test = false,
    depth_write = false,
    billboards = compiled_billboards,
  })
end)
```

**Key patterns:**
- `world_render` with `stage = minecraft.render.stages.stars` replaces the vanilla star renderer.
- `event.cancel_vanilla = true` disables the built-in star rendering.
- `minecraft.render.billboards({...})` draws billboard quads with `x`, `y`, `z`, `size`, `alpha` per star.
- `blend = "additive"` gives glowing star appearance.
- Astronomy data comes from `event.astronomy_enabled`, `event.astronomy_utc_millis`, `event.observer_latitude_deg`, `event.observer_longitude_deg`.

---

## 14. `colorful_skies` — Sky & Fog Coloring

**Files:** `scripts/main.lua`, `mod.json`

Applies a dynamic color tint to sky and fog based on the celestial angle.

```lua
local function sky_color_for_celestial(celestial)
  local t = (celestial or 0.0) * 6.2831853
  local r = 0.45 + 0.35 * math.max(0.0, math.cos(t))
  local g = 0.55 + 0.30 * math.max(0.0, math.sin(t + 1.0))
  local b = 0.85 + 0.10 * math.max(0.0, math.cos(t + 2.0))
  return r, g, b
end

minecraft.on(minecraft.events.world_color, {
  kind = minecraft.colors.sky,
  is_overworld = true, priority = 100,
}, function(event)
  return apply_colorful_tint(event, 0.75)
end)

minecraft.on(minecraft.events.world_color, {
  kind = minecraft.colors.fog,
  is_overworld = true, priority = 100,
}, function(event)
  return apply_colorful_tint(event, 0.5)
end)
```

**Key patterns:**
- Multiple `world_color` handlers can target `minecraft.colors.sky` and `minecraft.colors.fog` independently.
- `event.celestial` provides the current sun angle (0–1 normalized).

---

## 15. `seedfinder` — Utility Mod with Full GUI

**Files:** `scripts/main.lua`, `mod.json`

A complete seed-finding tool with custom GUI, text fields, search/filter, biome map rendering, and a rule specification editor.

### Opening a custom screen

```lua
minecraft.on(minecraft.events.screen_ui, {
  screen_id = minecraft.screen.ids.create_world,
  region = minecraft.screen.regions.footer,
  priority = 90,
}, function(event)
  event.ui:add_stacked_centered_button("Seed tools...", function()
    open_finder()
  end)
end)

-- The finder screen
minecraft.on(minecraft.events.screen_event, { screen_id = SCREEN_ID }, function(event)
  if event.phase == "init" then
    for _, f in ipairs(L.fields) do
      minecraft.screen.add_field(f.name, f.x, f.y, f.w, f.h, {
        text = S.saved[f.name] or "",
        numeric = f.numeric or false,
        signed = f.signed or false,
        decimal = f.decimal or false,
        max_len = 20,
      })
    end
  elseif event.phase == "render" then
    -- Full custom GUI drawing
  elseif event.phase == "tick" then
    search_tick()
  elseif event.phase == "mouse" then
    -- Click handling
  elseif event.phase == "scroll" then
    S.scroll = S.scroll + step
  elseif event.phase == "key" then
    if event.key == KEY_ESCAPE then go_back() end
  elseif event.phase == "close" then
    S.searching = false
  end
end)
```

### Slime chunk detection via `minecraft.world.sample_grid`

```lua
local ok, grid = pcall(minecraft.world.sample_grid, seed, 0, 0, {
  radius_chunks = radius,
  max_side = side,
  channels = { "biome_id", "height", "surface_block", "surface_block_below" },
  mod_generation = true,
})
```

### Biome map rendering from grid

```lua
local tex = minecraft.render.create_texture(grid.side, grid.side, grid.values)
-- Then draw with:
minecraft.gui.draw_texture(tex.id, img_x, img_y, img, img)
```

**Key patterns:**
- `minecraft.world.sample_grid` generates a grid of biome/height/block data for any seed without loading the world.
- `minecraft.render.create_texture(width, height, pixelData)` creates a texture from raw pixel data.
- `minecraft.render.release_texture(id)` frees a created texture.
- `minecraft.screen.add_field(name, x, y, w, h, opts)` creates native text input fields.
- `minecraft.screen.field_text(name)` reads text from a field.
- `minecraft.screen.set_field_text(name, value)` writes to a field.
- `minecraft.screen.host_field(name)` reads fields from the parent screen (create world).
- `minecraft.files.pick({extension})` opens a native file picker.
- `minecraft.files.read(path)` reads a file's contents.
- `minecraft.screen.open_host(screenId, params)` opens a screen with initial values.
- `minecraft.util.json_decode` / `minecraft.util.json_encode` for JSON serialization.

---

## 16. `offline_mode` — Session API

**Files:** `scripts/main.lua`, `mod.json`

Sets the offline-mode username using `minecraft.session.*` API with persistent config.

```lua
local function apply()
  if state.enabled and state.username ~= "" then
    minecraft.session.set_offline_username(state.username)
  else
    minecraft.session.clear_offline_username()
  end
  save_config()
end

-- Persist to disk
local function save_config()
  minecraft.storage.write(CONFIG_PATH, minecraft.util.json_encode(state))
end

-- Load on client ready
minecraft.on(minecraft.events.client_tick, {
  has_player = true, once = true, priority = 0,
}, function()
  load_config()
  apply()
end)
```

### Custom settings screen

```lua
minecraft.on(minecraft.events.screen_event, {
  screen_id = "offline_mode:settings", priority = 100,
}, function(event)
  if event.phase == "init" then
    minecraft.screen.add_field("username", 60, 64, 200, 20, {text=state.username, max_len=16})
    minecraft.screen.add_button(60, 92, 200, 20, toggle_label(), function()
      state.enabled = not state.enabled
      apply()
    end)
  elseif event.phase == "render" then
    minecraft.gui.draw_centered_text({ x = 0, y = 24, width = w, text = "Offline Mode", color = 0xFFFFFF })
  end
end)
```

### Inject button into login screen

```lua
minecraft.on(minecraft.events.screen_ui, {
  screen_id = minecraft.screen.ids.login,
  region = minecraft.screen.regions.screen,
  priority = 100,
}, function(event)
  event.ui:add_centered_button(186, "Offline Mode: OFF", function()
    minecraft.screen.open("offline_mode:settings")
  end)
end)
```

**Key patterns:**
- `minecraft.session.set_offline_username(name)` / `clear_offline_username()` controls session identity.
- `minecraft.storage.read(path)` / `minecraft.storage.write(path, data)` for file persistence.
- `minecraft.time.utc_millis()` gets real-world time without requiring a world.
- `client_tick` with `once = true` fires exactly once when the condition is met.

---

## 17. `ravine_backport` — Terrain Carving

**Files:** `scripts/main.lua`, `mod.json`

Simple procedural ravine generation via `chunk_generation`.

```lua
minecraft.on(minecraft.events.chunk_generation, {
  stage = minecraft.generation.stages.carver,
  moment = minecraft.generation.moments.after,
  when = minecraft.util.real_world,
}, function(event)
  local seed = math.floor(event.world_seed or 0)
  if (seed + event.chunk_x * 7342871 + event.chunk_z * 912931) % 35 ~= 0 then return end
  for step = 0, 12 do
    local world_x = base_x + step * 2
    local world_z = base_z + math.floor(math.sin(step * 0.7) * 3.0)
    local x = chunk_coord(world_x)
    local z = chunk_coord(world_z)
    local top = event.chunk:get_height(x, z)
    if top > 8 then
      carve_column(x, z, top - 2, 10 + step)
    end
  end
end)
```

**Key patterns:**
- `moment = minecraft.generation.moments.after` runs after vanilla carving.
- `event.world_seed`, `event.chunk_x`, `event.chunk_z` from chunk generation event.
- `event.chunk:get_height(x, z)` returns the surface height at the given column.
- Deterministic seeded randomness using arithmetic on chunk coordinates.

---

## 18. `layered_clouds` — Custom Cloud Rendering

**Files:** `scripts/main.lua`, `mod.json`

Replaces vanilla clouds with multiple layers of procedurally-animated cloud quads.

### Render-time cloud quads

```lua
minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.clouds,
  moment = minecraft.render.moments.before,
  priority = 100,
}, function(event)
  if not mod_active() or not event.is_overworld then return event end
  event.cancel_vanilla = true
  rebuild_layers(event.world_name)
  for layer = 0, config.layer_count - 1 do
    local height = base_height + layer * config.layer_height_spacing
    draw_cloud_layer(event, layer, height, color_r, color_g, color_b)
  end
  return event
end)
```

### Drawing with `minecraft.render.quads`

```lua
local function draw_cloud_layer(event, layer_index, height, color_r, color_g, color_b)
  local vertices = {}
  -- build vertex list...
  minecraft.render.quads({
    texture = "/environment/clouds.png",
    vertices = vertices,
    r = color_r, g = color_g, b = color_b,
    a = 0.8 * layer.opacity,
    blend = true,
    cull = false,
    depth_test = true,
    depth_write = false,
  })
end
```

### Config with persistence

```lua
local config = minecraft.util.copy(CONFIG_DEFAULTS)

local function save_config()
  minecraft.config.save(CONFIG_FILE, config, {
    keys = CONFIG_KEYS,
    names = CONFIG_NAMES,
  })
end

local function load_config()
  config, found = minecraft.config.load(CONFIG_FILE, CONFIG_DEFAULTS, {
    aliases = CONFIG_ALIASES,
  })
end
```

### Settings screen

```lua
local open_settings = minecraft.screen.settings({
  id = SCREEN_ID,
  title = "Cloud Settings",
  parent_screen = minecraft.screen.ids.world_settings,
  parent_region = minecraft.screen.regions.footer,
  button_label = "Cloud Settings...",
  values = function() return config end,
  sliders = {
    { key = "layer_count", label = "Layers", min = 1, max = 12, integer = true },
    { key = "base_opacity", min = 0, max = 1,
      format = function(v) return "Opacity: " .. math.floor(v * 100) .. "%" end },
    { key = "cloud_scale", min = 0.5, max = 2,
      format = function(v) return string.format("Scale: %.2fx", v) end },
    { key = "layer_height_spacing", min = 4, max = 32,
      format = function(v) return "Spacing: " .. math.floor(v + 0.5) .. " blocks" end },
    { key = "wind_speed", min = 0, max = 5,
      format = function(v) return string.format("Wind Speed: %.1fx", v) end },
  },
  on_change = clamp_config,
  on_reset = reset_config,
  on_save = save_config,
})
```

**Key patterns:**
- `world_render` with `stage = minecraft.render.stages.clouds` replaces clouds.
- `minecraft.render.quads({...})` renders custom geometry with texture, color, alpha, and render state.
- `minecraft.config.save` / `minecraft.config.load` provides automatic config file I/O with key aliasing.
- `minecraft.screen.settings({...})` creates a slider settings screen.
- `event.cancel_vanilla = true` disables vanilla cloud rendering.
- `event.cloud_base_height`, `event.celestial`, `event.camera_x/z`, `event.tick_delta` from cloud render event.

---

## 19. `realtime_sky` — Real-World Solar Position

**Files:** `scripts/main.lua`, `scripts/places.lua`, `scripts/globe_ui.lua`, `scripts/earth_time_solar.lua`, `scripts/config_helper.lua`, `mod.json`

Full real-time astronomy: computes the sun's position from UTC time and GPS coordinates, renders a 3D globe, and provides a place-picker UI.

### Sky event hooks

```lua
minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.sky,
  moment = minecraft.render.moments.before,
  is_overworld = true,
  priority = SKY_PROVIDER_PRIORITY,
  when = function() return realtime_active() end,
}, function(event)
  local frame = current_solar_frame(event.tick_delta)
  event.celestial_angle = frame.sun_angle
  event.sky_yaw_deg = frame.skydome_yaw_deg
  event.astronomy_enabled = true
  event.astronomy_utc_millis = frame.utc_millis
  event.observer_latitude_deg = settings.latitude
  event.observer_longitude_deg = settings.longitude
end)
```

### Solar position calculation

```lua
function earth_time_solar.build_frame(settings, partial_ticks, utc_millis)
  local observer_millis = earth_time_solar.resolve_observer_millis(settings, utc_millis)
  local right_ascension, declination = solar_ra_dec(observer_millis)
  local sun_azimuth, sun_altitude, hour_angle = horizontal_from_ra_dec(
    right_ascension, declination, observer_millis, settings.latitude, settings.longitude)
  return {
    day_tick = normalize_tick(6000.0 + hour_angle / TWO_PI * 24000.0),
    sun_angle = (90.0 - sun_altitude) * DEG,
    skydome_yaw_deg = normalize_signed_deg(180.0 - sun_azimuth),
    sun_altitude_deg = sun_altitude,
    is_daylight = sun_altitude > -0.833,
  }
end
```

### 3D globe rendering with `gui.begin_3d`

```lua
function globe_ui.draw(ui, width, height, pin_lat, pin_lon)
  local opts = globe_ui.viewport_opts(ui, width, height)
  minecraft.gui.begin_3d(opts)
  for _, primitive in ipairs(graticule_primitives) do
    minecraft.gui.draw_3d(primitive)
  end
  for _, vertices in ipairs(coast_vertices) do
    minecraft.gui.draw_3d({ mode = "line_strip", color = 0xFFB8C2D6, vertices = vertices })
  end
  -- Draw pin
  minecraft.gui.draw_3d({ mode = "points", color = 0xFFFA9E1F, point_size = 6.0, vertices = { { x = px, y = py, z = pz } } })
  minecraft.gui.end_3d()
end
```

### Picking lat/lon from mouse click (ray-sphere intersection)

```lua
function globe_ui.pick_lat_lon(ui, width, height, mouse_x, mouse_y)
  local opts = globe_ui.viewport_opts(ui, width, height)
  opts.mouse_x = mouse_x
  opts.mouse_y = mouse_y
  local ray = minecraft.gui.unproject(opts)
  local hx, hy, hz = ray_hit_sphere(ray.origin.x, ray.origin.y, ray.origin.z,
    ray.direction.x, ray.direction.y, ray.direction.z, SPHERE_RADIUS)
  local lat, lon = xyz_to_lat_lon(hx, hy, hz)
  return { lat = lat, lon = lon }
end
```

### Toggle and DST buttons

```lua
minecraft.gui.draw_toggle({
  x = x, y = y, width = w, height = h,
  label = "Sky", value = settings.enabled,
  mouse_x = event.mouse_x, mouse_y = event.mouse_y
})
```

**Key patterns:**
- `world_render` with `stage = sky` and setting `event.celestial_angle`, `event.sky_yaw_deg`, and `event.astronomy_enabled = true` provides astronomy data to other mods (like northern_stars).
- `minecraft.time.utc_millis()` gets real UTC time.
- `minecraft.gui.begin_3d(opts)` / `end_3d()` renders 3D content in a GUI viewport.
- `minecraft.gui.draw_3d(primitive)` renders line loops, line strips, points, etc.
- `minecraft.gui.unproject(opts)` converts screen coordinates to a 3D ray.
- `minecraft.gui.draw_toggle({...})` renders a GUI toggle button.
- `minecraft.read_asset(path)` reads a text asset bundled with the mod.
- `minecraft.require("scripts.earth_time_solar")` loads a helper module.

---

## 20. `repair_table` — Custom Block Screen with Inventory

**Files:** `scripts/main.lua`, `scripts/repair_screen.lua`, `scripts/inventory_helper.lua`, `mod.json`

A repair table block that opens a custom GUI with inventory slot management.

```lua
-- Block registration with on_use handler
minecraft.register_block({
  id = 150,
  texture = "mods/repair_table/repair_table.png",
  hardness = 2.5,
  resistance = 10.0,
  translation_key = "repairTable",
  material = "wood",
  opaque = false,
  full_cube = false,
  model = repair_table_model,
  behavior_priority = 100,
  on_use = function(event)
    if not event.right_click then return end
    repair_screen.open()
    event.handled = true
  end,
})
```

### Screen with slot inventory

```lua
local screen = inventory_helper.slots({
  id = SCREEN_ID,
  title = "Repair Table",
  slots = 3,
  panel_width = 176,
  panel_height = 166,
  player_inventory = true,
  background = "mods/repair_table/repair_table_gui.png",
  positions = {
    { x = 44, y = 35 },
    { x = 62, y = 35 },
    { x = 120, y = 35 },
  },
  on_tick = function(ctx)
    if not inventory_helper.stack.is_empty(ctx.get(3)) then return end
    local left = ctx.get(1)
    local right = ctx.get(2)
    local repaired = inventory_helper.stack.combine_damage(
      inventory_helper.stack.copy(left),
      inventory_helper.stack.copy(right))
    ctx.set(3, repaired)
    ctx.set(1, inventory_helper.stack.empty())
    ctx.set(2, inventory_helper.stack.empty())
  end,
})
```

### Inventory slot interaction

```lua
function M.stack.click(slot_stack, cursor, button)
  -- Pick up half/double-click, merge, swap logic
  if M.stack.is_empty(slot_stack) then
    -- Place from cursor
  elseif M.stack.is_empty(cursor) then
    -- Pick up from slot
  elseif M.stack.mergeable(slot_stack, cursor) then
    -- Merge stacks
  else
    -- Swap stacks
  end
  return slot_stack, cursor
end
```

### Screen lifecycle events

```lua
minecraft.on(minecraft.events.screen_event, { screen_id = spec.id, priority = priority }, function(event)
  if event.phase == "init" then
    panel_origin(event.width, event.height)
  elseif event.phase == "render" then
    render_panel(event.mouse_x, event.mouse_y)
  elseif event.phase == "tick" then
    if spec.on_tick then spec.on_tick(ctx) end
  elseif event.phase == "mouse" then
    handle_click(event.x, event.y, event.button)
  elseif event.phase == "key" then
    if event.key == minecraft.keys.escape or event.key == minecraft.key_code("inventory") then
      minecraft.screen.close()
    end
  elseif event.phase == "close" then
    return_items()
  end
end)
```

### Item description API for damageables

```lua
local info = minecraft.items.describe(item_id)
if info and info.damageable then
  local max_damage = info.max_damage or 0
  local remaining = (max_damage - damage_left) + (max_damage - damage_right)
  remaining = math.min(remaining, max_damage)
end
```

**Key patterns:**
- `on_use` in `register_block` fires when the block is right-clicked.
- The screen lifecycle phases are `init`, `render`, `tick`, `key`, `mouse`, `scroll`, and `close`; callbacks are event-driven, not a guaranteed fixed linear sequence.
- `minecraft.gui.draw_item(x, y, id, count, damage)` renders an item with damage.
- `minecraft.inventory.get(slot)` / `minecraft.inventory.set(slot, stack)` for player inventory.
- `minecraft.inventory.cursor_get()` / `minecraft.inventory.cursor_set(stack)` for cursor stack.
- `minecraft.inventory.give(stack)` drops items if inventory is full.
- `minecraft.items.describe(item_id)` returns `{damageable, max_damage, has_subtypes, stackable, max_count}`.
- `minecraft.gui.draw_sprite(texture, x, y, u, v, w, h)` draws a sprite from a GUI texture.

---

## 21. Config Helper Pattern (shared by layered_clouds, realtime_sky)

**Files:** `scripts/config_helper.lua` (both mods)

Parses key=value config files with type-aware loading.

```lua
function M.load(path, defaults, options)
  local values = {}
  for k, v in pairs(defaults) do values[k] = v end
  local text = minecraft.storage.read(path)
  for line in text:gmatch("[^\r\n]+") do
    local raw_key, raw_value = line:match("^%s*([^#;][^:=]-)%s*[:=]%s*(.-)%s*$")
    if raw_key then
      local key = (options.aliases and options.aliases[raw_key]) or minecraft.util.trim(raw_key)
      if defaults[key] ~= nil then
        if type(defaults[key]) == "boolean" then
          values[key] = minecraft.util.parse_boolean(raw_value, values[key])
        elseif type(defaults[key]) == "number" then
          values[key] = tonumber(raw_value) or values[key]
        else
          values[key] = raw_value ~= "" and raw_value or values[key]
        end
      end
    end
  end
  return values, found
end

function M.save(path, values, options)
  local lines = {}
  for _, key in ipairs(options.keys or {}) do
    local output_key = (options.names and options.names[key]) or key
    lines[#lines + 1] = output_key .. "=" .. tostring(values[key])
  end
  return minecraft.storage.write(path, table.concat(lines, "\n") .. "\n")
end
```

**Key patterns:**
- `minecraft.util.trim(s)` strips whitespace.
- `minecraft.util.parse_boolean(s, default)` parses true/false/yes/no/1/0 strings.
- Config files use `: ` or `=` as separators.
- `options.aliases` maps config-file names to internal names.
- `options.keys` controls save order; `options.names` maps internal names to file names.

---

## Best Practices

### Guard `world_tick` with `minecraft.util.real_world`

```lua
when = minecraft.util.real_world,
```

Always check `event.has_world` or use the `when` filter to avoid running in menus or superflat/void worlds.

### Use `priority` for ordering

Higher priority values run first. Common conventions:
- `100` — normal behavior
- `90` — UI additions
- `150` — replacement rendering (stars, clouds)
- `50` — low-level overrides
- `0` — late cleanup

### Clean up models on world unload

```lua
minecraft.on(minecraft.events.world_start, {}, function(event)
  minecraft.model.clear()
  client.channels = {}
end)
```

### `apply_state` vs `teleport` for mod entities

`apply_state` only works on entities spawned via `spawn_mod` — it silently
no-ops on any other entity ID. `teleport` works on any entity, mod-spawned or
vanilla.

Both move the entity server-side and let the ordinary entity tracker sync the
new position to clients, so remote observers see the same smooth interpolated
motion either way — neither one is a hard client-side snap over the network.
The real differences are local to the server object: `teleport` immediately
resets the entity's `prev*` fields (so it doesn't lag one tick server-side)
and fires a cancelable teleport event; `apply_state` does neither, and can
also set custom `data` in the same call. Prefer `apply_state` when you're
already updating a mod entity's data alongside its position; use `teleport`
for one-off repositioning (portals, `/tp`-style commands) or when you want the
teleport event to be cancelable.

### Check `ModWorldDrawContext` for rendering

Wrap custom rendering in appropriate event phases. Never call rendering APIs outside of `world_render`, `screen_event`, or `screen_region` events.

### Screen lifecycle phases

The standard screen event lifecycle:

| Phase | Purpose |
|-------|---------|
| `init` | Create buttons, fields, layout |
| `tick` | Per-tick logic (search, crafting) |
| `render` | Draw GUI elements |
| `mouse` | Handle clicks (button 0 = left, 1 = right) |
| `scroll` | Handle scroll wheel |
| `key` | Handle keyboard input |
| `close` | Clean up, return items, save state |

### Config persistence

Use `minecraft.config.save()` / `minecraft.config.load()` for settings or `minecraft.storage.read()` / `minecraft.storage.write()` for raw files. Prefer JSON via `minecraft.util.json_encode` / `json_decode` for structured data.

### Module loading

Split large mods into separate files and load them:

```lua
local physics = minecraft.require("scripts.physics")
```

The path is relative to the mod's `scripts/` directory. Omit the `.lua` extension.

### Use `minecraft.util.copy(table)` for deep copies

```lua
local config = minecraft.util.copy(CONFIG_DEFAULTS)
```

### Entity fields reference

When iterating `minecraft.entities.list("Item")`, each entry contains:

| Field | Type | Description |
|-------|------|-------------|
| `id` | number | Unique entity ID |
| `x, y, z` | number | Position |
| `vx, vy, vz` | number | Velocity (client-side) |
| `yaw, pitch` | number | Rotation |
| `item_id` | number | Item/block numeric ID |
| `item_count` | number | Stack size |
| `item_damage` | number | Damage value |
| `texture_path` | string | Custom texture path |
| `atlas_index` | number | Texture atlas index |
| `mod_texture` | boolean | Whether it's a mod texture |

For mod entities (`spawn_mod`), `registry_id` holds the mod-specific identifier string.

### Block registration fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | number | required | Unique block ID |
| `texture` | string | — | Custom texture path |
| `texture_id` | number | — | Vanilla texture atlas index |
| `hardness` | number | 0 | Mining time base |
| `resistance` | number | 0 | Explosion resistance |
| `luminance` | number | 0 | Light emission (0–1) |
| `material` | string | "stone" | Sound/mining material |
| `translation_key` | string | — | Localization key |
| `opaque` | boolean | true | Transparency |
| `full_cube` | boolean | true | Full-block shape |
| `model` | model | — | Custom model handle |
| `collision_height` | number | 1 | Collision box height |
| `item` | table | — | Item override (e.g., different texture) |
| `tile_entity` | string | — | Tile entity type |
| `behavior_priority` | number | 0 | Block behavior ordering |
| `on_use` | function | — | Right-click handler |
| `requires_solid_below` | boolean | true | Plant-like placement |
| `coordinate_bounds` | boolean | false | Random scale/offset |
| `coordinate_color` | boolean | false | Position-based tint |
| `bounds_padding` | number | 0 | Bounds shrink/grow |
| `bounds_offset` | number | 0 | Vertical offset |
| `min_scale` / `max_scale` | number | 1 | Random scale range |
| `stack_on_same` | boolean | false | Fence-like stacking |
