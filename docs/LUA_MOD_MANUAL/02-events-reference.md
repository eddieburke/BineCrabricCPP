# 02 — Events Reference

All event subscriptions use `minecraft.on(event_name, options, callback)`. The filtering system is entirely implemented in Lua's prelude:

```lua
minecraft.on(event_name, {
  -- Field filters: any key matching an event field is checked:
  --   - primitive: exact equality
  --   - table: treated as a set (key = true lookup, or array iteration)
  --   - function: predicate called with (actual_value)
  -- Special keys:
  when = function(event) end,  -- gate predicate; must return true
  once = true,                 -- auto-unsubscribe after first fire
  priority = 0,                -- lower = earlier execution
}, function(event)
  -- callback may mutate read-write fields (see per-event notes)
end)
```

### Priority System

Lower priority numbers execute first. The C++ HookBus sorts entries via `std::stable_sort`:

```cpp
listeners().push_back({owner, priority, listener});
std::stable_sort(listeners().begin(), listeners().end(),
  [](const Entry& a, const Entry& b) { return a.priority < b.priority; });
```

Default priority is `0`. Use negative values for early execution, positive for late.

### `event.handled` and `event.canceled` Conventions

- **`canceled`**: Prevents default engine behavior. Many events support this (time change, weather, block tick, entity spawn, attack damage, teleport, etc.).
- **`handled`**: Used by interaction events (`block_interact`, `entity_interact`, `key_press`, `mouse_button`). Signifies that the mod consumed the event. For block interaction, setting `event.handled = true` causes the prelude to also set `event.canceled = true`:

```lua
function minecraft.register_block(spec)
  -- ...
  if spec.on_use then
    minecraft.on(minecraft.events.block_interact, { block_id = spec.id, ... }, function(event)
      local result = spec.on_use(event)
      if event.handled then event.canceled = true end
      return result
    end)
  end
end
```

### `event.before` Pattern

Events with tick/update cycles (`client_tick`, `world_tick`, `chunk_generation`, `world_render`) provide a `before` field or `moment` field. The `before` field is `true` in the pre-update phase and `false` in the post-update phase. Render events provide `stage` + `moment` for granular placement.

---

## World Events

### `world_tick`

Fires every world tick (both before and after the main tick processing).

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `remote` | boolean | True if this is a client replica world | No |
| `before` | boolean | True in the pre-tick phase, false after | No |
| `side` | string | `"server"` or `"client"` | No |

```lua
minecraft.on(minecraft.events.world_tick, { before = false }, function(event)
  -- post-tick logic
end)
```

### `world_time`

Fires when the world time changes. Available as a C++ struct but **NOT exposed to Lua** (not in the prelude event list).

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `world` | userdata | The world | No |
| `old_time` | int64 | Previous time value | No |
| `new_time` | int64 | New time value | No |
| `canceled` | boolean | Cancel the time change | Yes |

### `weather_cycle`

Fires when weather would change. **Not exposed to Lua.**

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `world` | userdata | The world | No |
| `remote` | boolean | Client replica flag | No |
| `canceled` | boolean | Cancel weather change | Yes |

### `lightning_strike`

Fires when lightning strikes. **Not exposed to Lua.**

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `world` | userdata | The world | No |
| `x` | int | Strike X position | No |
| `y` | int | Strike Y position | No |
| `z` | int | Strike Z position | No |
| `canceled` | boolean | Cancel the lightning strike | Yes |

### `snow_ice_placement`

Fires when snow/ice would be placed by weather. **Not exposed to Lua.**

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `world` | userdata | The world | No |
| `x` | int | X position | No |
| `y` | int | Y position | No |
| `z` | int | Z position | No |
| `place_snow` | boolean | Whether snow would be placed | No |
| `place_ice` | boolean | Whether ice would be placed | No |

### `random_block_tick`

Fires when a block receives a random tick. **Not exposed to Lua.**

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `world` | userdata | The world | No |
| `block` | userdata | The block object | No |
| `x` | int | X position | No |
| `y` | int | Y position | No |
| `z` | int | Z position | No |
| `block_id` | int | Minecraft block ID | No |
| `canceled` | boolean | Cancel the random tick | Yes |

### `scheduled_block_tick`

Fires when a scheduled block tick executes. **Not exposed to Lua.**

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `world` | userdata | The world | No |
| `block` | userdata | The block object | No |
| `x` | int | X position | No |
| `y` | int | Y position | No |
| `z` | int | Z position | No |
| `block_id` | int | Minecraft block ID | No |
| `instant` | boolean | True if immediate execution | No |
| `canceled` | boolean | Cancel the scheduled tick | Yes |

### `schedule_block_update`

Fires when a block update is scheduled. **Not exposed to Lua.**

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `world` | userdata | The world | No |
| `x` | int | X position | No |
| `y` | int | Y position | No |
| `z` | int | Z position | No |
| `block_id` | int | Minecraft block ID | No |
| `tick_rate` | int | Scheduled delay in ticks | No |
| `canceled` | boolean | Cancel the scheduling | Yes |

### `tick_rate`

Fires to query/adjust the server tick rate.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `target_tps` | float | Desired ticks per second (default 20.0) | Yes |
| `tps_scale` | float | Multiplier applied to TPS (default 1.0) | Yes |

```lua
minecraft.on(minecraft.events.tick_rate, {}, function(event)
  event.target_tps = 10.0  -- slow down to 10 TPS
end)
```

### `chunk_generation`

Fires during chunk generation for each stage/moment.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `stage` | string | Generation stage: `"terrain"`, `"surface"`, `"carver"`, `"features"` | No |
| `moment` | string | `"before"` or `"after"` | No |
| `cancel_vanilla` | boolean | Skip vanilla generation for this stage | Yes |
| `vanilla_stage_ran` | boolean | Whether vanilla ran | No |
| `world_seed` | int64 | World seed | No |
| `has_world` | boolean | World context present | No |
| `world_name` | string | World save name | No |
| `is_overworld` | boolean | Overworld dimension flag | No |
| `mod_generation` | boolean | Mod generation flag | No |
| `chunk_x` | int | Active chunk X | No |
| `chunk_z` | int | Active chunk Z | No |
| `has_chunk` | boolean | Chunk context present | No |
| `remote` | boolean | Client replica flag | No |
| `side` | string | `"server"` or `"client"` | No |

```lua
minecraft.on(minecraft.events.chunk_generation, {
  stage = "features", moment = "after",
  when = minecraft.util.real_world,
}, function(event)
  -- Place custom features after vanilla
end)
```

The `cancel_vanilla` field, when set to `true`, skips the vanilla generation for that stage at `"before"` moment.

### `create_world`

Fires when a new world is being created (before world generation begins).

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `save_name` | string | World save folder name | No |
| `seed` | int64 | World seed | No |
| `canceled` | boolean | Cancel world creation | Yes |
| `options` | table | Map of string → string options | Yes |

```lua
minecraft.on(minecraft.events.create_world, {}, function(event)
  event.options["gamemode"] = "creative"
  event.seed = 42  -- not mutable (but options is)
end)
```

Only `options` and `canceled` are read back.

### `world_open`

Fires when a world is opened/loaded (before it starts ticking).

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `save_name` | string | World save folder name | No |
| `new_world` | boolean | True if this is a newly-created world | No |
| `options` | table | Read-only map of world options | No |
| `has_world` | boolean | World context present | No |
| `world_name` | string | World save name | No |
| `is_overworld` | boolean | Overworld dimension flag | No |
| `mod_generation` | boolean | Mod generation flag | No |
| `remote` | boolean | Client replica flag | No |
| `side` | string | `"server"` or `"client"` | No |

```lua
minecraft.on(minecraft.events.world_open, { new_world = true }, function(event)
  print("New world created: " .. event.save_name)
end)
```

### `world_start`

Fires when the world starts ticking for the first time.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `save_name` | string | World save folder name | No |
| `new_world` | boolean | True if new world | No |
| `has_world` | boolean | World context present | No |
| `world_name` | string | World save name | No |
| `is_overworld` | boolean | Overworld dimension flag | No |
| `mod_generation` | boolean | Mod generation flag | No |
| `remote` | boolean | Client replica flag | No |
| `side` | string | `"server"` or `"client"` | No |

```lua
minecraft.on(minecraft.events.world_start, { when = minecraft.util.real_world }, function(event)
  print("World started: " .. event.save_name)
end)
```

### `world_spawn_search`

Fires when the game searches for a valid spawn position.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `x` | int | Spawn X position | Yes |
| `y` | int | Spawn Y position (default 64) | Yes |
| `z` | int | Spawn Z position | Yes |
| `resolved` | boolean | Whether spawn was found | Yes |
| `has_world` | boolean | World context present | No |
| `world_name` | string | World save name | No |
| `is_overworld` | boolean | Overworld dimension flag | No |
| `mod_generation` | boolean | Mod generation flag | No |
| `remote` | boolean | Client replica flag | No |
| `side` | string | `"server"` or `"client"` | No |

```lua
minecraft.on(minecraft.events.world_spawn_search, {}, function(event)
  event.x = 0
  event.z = 0
  event.y = 80
  event.resolved = true
end)
```

---

## Entity Events

### `block_interact`

Fires when a player interacts with a block (right-click or left-click).

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `x` | int | Block X position | No |
| `y` | int | Block Y position | No |
| `z` | int | Block Z position | No |
| `block_id` | int | Minecraft block ID at position | No |
| `right_click` | boolean | True if right-click, false if left-click | No |
| `canceled` | boolean | Cancel the interaction | Yes |
| `handled` | boolean | Mark as handled (also sets canceled in register_block wrapper) | Yes |
| `remote` | boolean | Client replica flag | No |
| `has_player` | boolean | Player present | No |
| `local_player` | boolean | True if this is the local client player | No |
| `has_item` | boolean | Player is holding an item | No |
| `player_x` | double | Player X position | No |
| `player_y` | double | Player Y position | No |
| `player_z` | double | Player Z position | No |
| `player_yaw` | float | Player yaw | No |
| `player_pitch` | float | Player pitch | No |
| `item_id` | int | Held item ID | No |
| `item_count` | int | Held item count | Yes |
| `item_damage` | int | Held item damage | Yes |
| `item_max_damage` | int | Maximum item damage | No |
| `item_damageable` | boolean | Whether item can take damage | No |
| `side` | string | `"server"` or `"client"` | No |

```lua
minecraft.on(minecraft.events.block_interact, { right_click = true }, function(event)
  print("Block at " .. event.x .. "," .. event.y .. "," .. event.z .. " was right-clicked")
  event.handled = true
end)
```

### `entity_interact`

Fires when a player interacts with an entity (attack or right-click).

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `attack` | boolean | True if attacking, false if right-clicking | No |
| `canceled` | boolean | Cancel the interaction | Yes |
| `handled` | boolean | Mark as handled | Yes |
| `sneaking` | boolean | Player is sneaking | No |
| `has_player` | boolean | Player present | No |
| `local_player` | boolean | True if local client player | No |
| `has_target` | boolean | Target entity present | No |
| `entity_id` | int | Target entity ID | No |
| `entity_type` | string | Target entity type string | No |
| `target_id` | int | Target entity network ID | No |
| `has_item` | boolean | Player holding item | No |
| `item_id` | int | Held item ID | No |
| `item_count` | int | Held item count | Yes |
| `item_damage` | int | Held item damage | Yes |
| `player_yaw` | float | Player yaw | No |
| `player_pitch` | float | Player pitch | No |
| `remote` | boolean | Client replica flag | No |
| `side` | string | `"server"` or `"client"` | No |

```lua
minecraft.on(minecraft.events.entity_interact, { attack = true }, function(event)
  if event.entity_type == "minecraft:cow" then
    event.canceled = true  -- prevent cow attacks
  end
end)
```

### `entity_teleport`

Fires when an entity teleports.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `entity_id` | int | Entity ID | No |
| `entity_type` | string | Entity type string | No |
| `from_x` | double | Source X position | No |
| `from_y` | double | Source Y position | No |
| `from_z` | double | Source Z position | No |
| `x` | double | Destination X | Yes |
| `y` | double | Destination Y | Yes |
| `z` | double | Destination Z | Yes |
| `yaw` | float | Destination yaw | Yes |
| `pitch` | float | Destination pitch | Yes |
| `canceled` | boolean | Cancel the teleport | Yes |
| `has_entity` | boolean | Entity present | No |
| `has_player` | boolean | Player present | No |
| `has_world` | boolean | World context present | No |
| `world_name` | string | World save name | No |
| `is_overworld` | boolean | Overworld dimension flag | No |
| `mod_generation` | boolean | Mod generation flag | No |

Note: This event is SKIPPED on remote (client) worlds — only fires on server side.

```lua
minecraft.on(minecraft.events.entity_teleport, {}, function(event)
  -- Redirect all endermen to the sky
  if event.entity_type == "minecraft:enderman" then
    event.y = 200
  end
end)
```

### `attack_damage`

Fires when a player attacks an entity and damage is calculated.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `damage` | int | Calculated damage amount | Yes |
| `critical` | boolean | Whether this is a critical hit | Yes |
| `canceled` | boolean | Cancel the attack entirely | Yes |
| `fall_distance` | float | Player fall distance | No |
| `on_ground` | boolean | Player on ground | No |
| `target_x` | double | Target entity X | No |
| `target_y` | double | Target entity Y | No |
| `target_z` | double | Target entity Z | No |
| `has_player` | boolean | Player present | No |
| `has_target` | boolean | Target present | No |
| `remote` | boolean | Client replica flag | No |
| `side` | string | `"server"` or `"client"` | No |

```lua
minecraft.on(minecraft.events.attack_damage, {}, function(event)
  event.damage = event.damage * 2  -- double all damage
  event.critical = true             -- always critical
end)
```

### `player_travel`

Fires when a player moves/travels, before movement processing.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `sideways` | float | Sideways input (strafe) | Yes |
| `forward` | float | Forward input | Yes |
| `speed_multiplier` | float | Speed multiplier (default 1.0) | Yes |
| `has_player` | boolean | Player present | No |
| `is_local_player` | boolean | True if local client player | No |
| `remote` | boolean | Client replica flag | No |
| `side` | string | `"server"` or `"client"` | No |

```lua
minecraft.on(minecraft.events.player_travel, { is_local_player = true }, function(event)
  event.speed_multiplier = 2.0  -- double player speed
end)
```

### `crafting_take`

Fires when a player takes an item from a crafting output slot. **Not exposed to Lua.**

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `player` | userdata | The player | No |
| `stack` | userdata | The item stack taken | No |
| `canceled` | boolean | Cancel the take action | Yes |

### `furnace_output_take`

Fires when a player takes an item from a furnace output slot. **Not exposed to Lua.**

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `player` | userdata | The player | No |
| `stack` | userdata | The item stack taken | No |
| `canceled` | boolean | Cancel the take action | Yes |

### `entity_spawn`

Fires when an entity is spawned into the world (client-only).

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `entity_id` | int | Entity ID | No |
| `entity_type` | string | Entity type string | No |
| `item_id` | int | Item ID (if item entity) | No |
| `item_count` | int | Stack count (if item entity) | No |
| `item_damage` | int | Item damage (if item entity) | No |
| `texture_path` | string | Item texture path (if item entity) | No |
| `mod_texture` | boolean | Whether item uses mod texture | No |
| `atlas_index` | int | Atlas tile index (if vanilla texture) | No |

```lua
minecraft.on(minecraft.events.entity_spawn, {}, function(event)
  if event.entity_type == "minecraft:item" then
    print("Item spawned: " .. event.item_id)
  end
end)
```

### `entity_remove`

Fires when an entity is removed from the world (client-only).

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `entity_id` | int | Entity ID | No |
| `entity_type` | string | Entity type string | No |
| `item_id` | int | Item ID (if item entity) | No |
| `item_count` | int | Stack count (if item entity) | No |
| `item_damage` | int | Item damage (if item entity) | No |
| `texture_path` | string | Item texture path (if item entity) | No |
| `mod_texture` | boolean | Whether item uses mod texture | No |
| `atlas_index` | int | Atlas tile index (if vanilla texture) | No |

```lua
minecraft.on(minecraft.events.entity_remove, { entity_type = "minecraft:item" }, function(event)
  -- track item despawn
end)
```

### `entity_tick`

Fires every tick for each entity in the world.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `entity_id` | int | Entity ID | No |
| `entity_type` | string | Entity type string | No |
| `x` | double | Entity X position | No |
| `y` | double | Entity Y position | No |
| `z` | double | Entity Z position | No |
| `yaw` | float | Entity yaw | No |
| `pitch` | float | Entity pitch | No |
| `remote` | boolean | Client replica flag | No |
| `canceled` | boolean | Cancel the entity tick | Yes |
| `side` | string | `"server"` or `"client"` | No |

```lua
minecraft.on(minecraft.events.entity_tick, { entity_type = "minecraft:zombie" }, function(event)
  -- zombies tick slower
  event.canceled = true  -- would freeze zombie AI
end)
```

---

## Client Events

### `client_tick`

Fires every client tick (both before and after world tick, and when paused).

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `before` | boolean | True in pre-tick phase, false after | No |
| `after_world` | boolean | True in the post-world-tick sub-phase | No |
| `paused` | boolean | Game is paused (menu open) | No |
| `has_player` | boolean | Player present | No |
| `has_world` | boolean | World context present | No |
| `world_name` | string | World save name | No |
| `is_overworld` | boolean | Overworld dimension flag | No |
| `mod_generation` | boolean | Mod generation flag | No |
| `remote` | boolean | Client replica flag | No |
| `side` | string | `"server"` or `"client"` | No |
| `camera_y` | double | Camera entity Y position | No |
| `player_y` | double | Player Y position | No |
| `player_fall_distance` | float | Player fall distance | No |
| `player_on_ground` | boolean | Player on ground | No |
| `world_time` | double | World time mod 24000 | No |
| `is_night` | boolean | Nighttime check | No |

```lua
minecraft.on(minecraft.events.client_tick, { before = true, when = minecraft.util.real_world }, function(event)
  -- HUD update logic
  if event.is_night then
    -- render night vision overlay
  end
end)
```

### `raycast`

Fires when the client performs a raycast (crosshair targeting).

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `has_hit` | boolean | Whether anything was hit | No |
| `type` | string | Hit type: `"block"`, `"entity"`, or `"none"` | No |
| `hit_x` | double | Hit position X | No |
| `hit_y` | double | Hit position Y | No |
| `hit_z` | double | Hit position Z | No |
| `block_x` | int | Block X (if block hit) | No |
| `block_y` | int | Block Y (if block hit) | No |
| `block_z` | int | Block Z (if block hit) | No |
| `side` | int | Block face hit (0-5) | No |
| `block_id` | int | Block ID at position | No |
| `block_name` | string | Block wire name | No |
| `item_id` | int | Same as block_id for block hits | No |
| `entity_id` | int | Entity ID (if entity hit) | No |
| `entity_type` | string | Entity type (if entity hit) | No |
| `entity_raw_id` | int | Entity raw registry ID | No |
| `entity_x` | double | Entity X (if entity hit) | No |
| `entity_y` | double | Entity Y (if entity hit) | No |
| `entity_z` | double | Entity Z (if entity hit) | No |
| `side` | string | `"server"` or `"client"` | No |

```lua
minecraft.on(minecraft.events.raycast, {}, function(event)
  if event.has_hit and event.type == "block" then
    print("Looking at block " .. event.block_id .. " at " .. event.block_x .. "," .. event.block_y .. "," .. event.block_z)
  end
end)
```

### `key_press`

Fires on key press/release events.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `key` | int | Key code (see `minecraft.keys` for common values) | No |
| `pressed` | boolean | True if pressed, false if released | No |
| `repeat` | boolean | True if this is a repeat event (held key) | No |
| `handled` | boolean | Mark as handled (prevents further processing) | Yes |

```lua
minecraft.on(minecraft.events.key_press, { key = minecraft.keys.space, pressed = true }, function(event)
  print("Space bar pressed!")
  event.handled = true
end)
```

### `mouse_button`

Fires on mouse button press/release events.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `button` | int | Mouse button (0 = left, 1 = right, 2 = middle) | No |
| `pressed` | boolean | True if pressed, false if released | No |
| `handled` | boolean | Mark as handled | Yes |

```lua
minecraft.on(minecraft.events.mouse_button, { button = 0, pressed = true }, function(event)
  print("Left click!")
end)
```

### `screen_event`

Fires for custom Lua screens with phase-based dispatch.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `screen_id` | string | Screen identifier | No |
| `phase` | string | Phase: `"init"`, `"render"`, `"tick"`, `"key"`, `"mouse"`, `"scroll"`, `"close"` | No |
| `width` | int | Screen width | No |
| `height` | int | Screen height | No |
| `mouse_x` | int | Mouse X position | No |
| `mouse_y` | int | Mouse Y position | No |
| `x` | int | Alias for mouse_x | No |
| `y` | int | Alias for mouse_y | No |
| `tick_delta` | float | Render tick delta | No |
| `key` | int | Key code (key phase) | No |
| `char` | int | Character code (key phase, unsigned) | No |
| `button` | int | Mouse button (mouse phase) | No |
| `released` | boolean | True if button released (mouse phase) | No |
| `delta` | int | Scroll delta (scroll phase) | No |
| `handled` | boolean | Mark as handled | Yes |

The `minecraft.screen.on_lua_screen()` wrapper dispatches by phase:

```lua
minecraft.screen.on_lua_screen("my_mod:screen", {
  init = function(event) end,
  render = function(event) end,
  mouse = function(event) end,
  key = function(event) end,
  close = function(event) end,
})
```

### `screen_ui`

Fires for screen UI injection into vanilla or mod screens.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `screen_id` | string | The target screen identifier | No |
| `region` | string | UI region name | No |
| `host_fields` | table | Host screen fields table | No |
| `ui` | table | UI helper table with `add_stacked_centered_button` etc. | No |

The `minecraft.screen.on_ui()` wrapper subscribes to this:

```lua
minecraft.screen.on_ui("options", "options", function(event)
  event.ui:add_stacked_centered_button("My Mod Settings", open_callback)
end)
```

### `screen_region`

Fires for screen rendering regions on Lua-hosted screens (render, mouse click, mouse scroll phases).

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `phase_name` | string | `"render"`, `"mouse_click"`, or `"mouse_scroll"` | No |
| `screen_id` | string | Screen identifier | No |
| `region` | string | Region name | No |
| `mouse_x` | int | Mouse X | No |
| `mouse_y` | int | Mouse Y | No |
| `button` | int | Mouse button | No |
| `scroll_delta` | int | Scroll delta | No |
| `x` | int | Region X | No |
| `y` | int | Region Y | No |
| `width` | int | Region width | Yes |
| `height` | int | Region height | Yes |
| `handled` | boolean | Mark as handled | Yes |

---

## Render Events

### `camera_setup`

Fires during camera setup, before rendering. All position/rotation fields are mutable.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `tick_delta` | float | Render tick delta | No |
| `x` | double | Camera X position | Yes |
| `y` | double | Camera Y position | Yes |
| `z` | double | Camera Z position | Yes |
| `yaw` | float | Camera yaw (degrees) | Yes |
| `pitch` | float | Camera pitch (degrees) | Yes |
| `roll` | float | Camera roll (degrees) | Yes |
| `custom_view` | boolean | Flag indicating custom view mode | Yes |
| `hide_first_person_hand` | boolean | Hide the first-person hand model | Yes |

```lua
minecraft.on(minecraft.events.camera_setup, {}, function(event)
  event.roll = 15  -- tilt the camera
  event.custom_view = true
end)
```

### `render_frame`

Fires at the beginning of each rendered frame.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `tick_delta` | float | Render tick delta (partial ticks) | No |

```lua
minecraft.on(minecraft.events.render_frame, {}, function(event)
  -- frame-level render logic
end)
```

### `fov`

Fires to query/adjust the field of view.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `tick_delta` | float | Render tick delta | No |
| `fov` | float | Field of view in degrees (default 70.0) | Yes |

```lua
minecraft.on(minecraft.events.fov, {}, function(event)
  event.fov = 120  -- wider FOV
end)
```

### `world_render`

Fires around each world rendering stage. Provides `minecraft.render.stages` and `minecraft.render.moments` constants for filtering.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `tick_delta` | float | Render tick delta | No |
| `stage` | string | Render stage: `"sky"`, `"stars"`, `"terrain_opaque"`, `"entities"`, `"particles_lit"`, `"particles"`, `"terrain_translucent"`, `"weather"`, `"clouds"`, `"hand"`, `"framebuffer"` | No |
| `moment` | string | `"before"` or `"after"` | No |
| `cancel_vanilla` | boolean | Skip vanilla rendering for this stage | Yes |
| `vanilla_stage_ran` | boolean | Whether vanilla stage executed | No |
| `shadow_pass` | boolean | `true` while an offscreen shadow-depth pass renders entities | No |
| `celestial_angle` | float | Celestial angle (only read/written at sky/before) | Yes |
| `sky_yaw_deg` | float | Sky yaw in degrees (sky/before) | Yes |
| `star_brightness` | float | Star brightness (stars/before only) | Yes |
| `rain_strength` | float | Current rain strength | No |
| `stars_enabled` | boolean | Whether stars are visible | No |
| `astronomy_enabled` | boolean | Enable astronomy mode | Yes |
| `astronomy_utc_millis` | double | Astronomy UTC time in ms | Yes |
| `observer_latitude_deg` | float | Observer latitude | Yes |
| `observer_longitude_deg` | float | Observer longitude | Yes |
| `world_time` | double | World time mod 24000 | No |
| `celestial` | double | Normalized celestial angle | No |
| `is_night` | boolean | Nighttime | No |
| `camera_x` | double | Camera X world position | No |
| `camera_y` | double | Camera Y world position | No |
| `camera_z` | double | Camera Z world position | No |
| `camera_yaw` | double | Camera yaw | No |
| `camera_pitch` | double | Camera pitch | No |
| `camera_roll` | double | Camera roll | No |
| `custom_camera` | boolean | Custom camera active | No |
| `cloud_base_height` | float | Cloud height offset (clouds stage only) | No |
| `has_world` | boolean | World context present | No |
| `world_name` | string | World save name | No |
| `is_overworld` | boolean | Overworld flag | No |
| `mod_generation` | boolean | Mod generation flag | No |

```lua
minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.sky,
  moment = minecraft.render.moments.before,
}, function(event)
  event.cancel_vanilla = true        -- custom sky
  event.celestial_angle = 0.25       -- set to noon
  event.astronomy_enabled = true
  event.observer_latitude_deg = 51.5
end)
```

### `first_person_hand`

Fires before rendering the first-person hand model.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `tick_delta` | float | Render tick delta | No |
| `width` | int | Framebuffer target width | No |
| `height` | int | Framebuffer target height | No |
| `eye` | int | Eye index (0 = main hand, 1 = offhand) | No |
| `canceled` | boolean | Skip rendering the hand | Yes |
| `entity_id` | int | Camera entity ID | No |
| `entity_type` | string | Camera entity type | No |

```lua
minecraft.on(minecraft.events.first_person_hand, { eye = 0 }, function(event)
  event.canceled = true  -- hide main hand
end)
```

### `world_color`

Fires to query/adjust world colors (sky and fog).

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `partial_ticks` | float | Render tick delta | No |
| `kind` | string | Color kind: `"sky"` or `"fog"` | No |
| `r` | double | Red component (0.0-1.0) | Yes |
| `g` | double | Green component (0.0-1.0) | Yes |
| `b` | double | Blue component (0.0-1.0) | Yes |
| `celestial` | double | Normalized celestial angle | No |
| `world_time` | double | World time mod 24000 | No |
| `is_night` | boolean | Nighttime | No |
| `has_world` | boolean | World present | No |
| `world_name` | string | World save name | No |
| `is_overworld` | boolean | Overworld flag | No |
| `mod_generation` | boolean | Mod generation flag | No |

```lua
minecraft.on(minecraft.events.world_color, { kind = minecraft.colors.sky }, function(event)
  event.r = 1.0  -- red sky
  event.g = 0.0
  event.b = 0.0
end)
```

### `pre_entity_render`

Fires before an entity is rendered.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `entity_id` | int | Entity ID | No |
| `entity_type` | string | Entity type string | No |
| `tick_delta` | float | Render tick delta | No |
| `canceled` | boolean | Skip rendering this entity | Yes |
| `item_id` | int | Item ID (if item entity) | No |
| `item_count` | int | Stack count (if item entity) | No |
| `item_damage` | int | Item damage (if item entity) | No |
| `texture_path` | string | Item texture path | No |
| `mod_texture` | boolean | Uses mod texture | No |
| `atlas_index` | int | Atlas tile index | No |

```lua
minecraft.on(minecraft.events.pre_entity_render, { entity_type = "minecraft:item" }, function(event)
  if event.item_id == 10 then  -- skip rendering a specific item
    event.canceled = true
  end
end)
```

### `entity_render`

Fires during entity rendering to apply pose overrides (rotation, scale, offset, model part overrides).

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `entity_id` | int | Entity ID | No |
| `entity_type` | string | Entity type string | No |
| `is_player` | boolean | Whether this is a player entity | No |
| `tick_delta` | float | Render tick delta | No |
| `pose.body_yaw` | float | Body yaw (degrees) | Yes |
| `pose.head_yaw` | float | Head yaw (degrees) | Yes |
| `pose.head_pitch` | float | Head pitch (degrees) | Yes |
| `pose.limb_swing` | float | Limb swing phase (radians) | Yes |
| `pose.limb_distance` | float | Limb distance (0.0-1.0) | Yes |
| `pose.yaw` | float | Entity yaw (degrees) | Yes |
| `pose.pitch` | float | Entity pitch (degrees) | Yes |
| `pose.roll` | float | Entity roll (degrees) | Yes |
| `pose.scale` | float | Entity scale multiplier | Yes |
| `pose.offset_x` | float | World-space X offset | Yes |
| `pose.offset_y` | float | World-space Y offset | Yes |
| `pose.offset_z` | float | World-space Z offset | Yes |
| `pose.parts` | table | Map of part name → `{ yaw, pitch, roll }` (NaN = leave as-is) | Yes |

```lua
minecraft.on(minecraft.events.entity_render, { entity_type = "minecraft:zombie" }, function(event)
  event.pose.scale = 2.0           -- giant zombies
  event.pose.roll = 180            -- upside down
  event.pose.parts.head = { yaw = 45, pitch = 30, roll = 0 }
end)
```

---

## TileEntity Events

### `pre_tile_entity_render`

Fires before a block entity is rendered.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `x` | int | Block entity X | No |
| `y` | int | Block entity Y | No |
| `z` | int | Block entity Z | No |
| `id` | string | Block entity ID | No |
| `tick_delta` | float | Render tick delta | No |
| `canceled` | boolean | Skip rendering | Yes |

```lua
minecraft.on(minecraft.events.pre_tile_entity_render, { id = "chest" }, function(event)
  event.canceled = true  -- hide all chests
end)
```

### `tile_entity_tick`

Fires every tick for each block entity. The engine also drives animation.

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `x` | int | Block entity X | No |
| `y` | int | Block entity Y | No |
| `z` | int | Block entity Z | No |
| `id` | string | Block entity type ID | No |
| `remote` | boolean | Client replica flag | No |
| `removed` | boolean | Entity is marked removed | No |
| `canceled` | boolean | Cancel the tick | Yes |
| `world_time` | double | Current world time | No |
| `animation_frame` | int | Current animation frame | No |
| `animation_tick` | double | Current animation tick | No |
| `animation_speed` | float | Animation speed multiplier | Yes |
| `entity` | userdata | Block entity handle | No |

```lua
minecraft.on(minecraft.events.tile_entity_tick, { id = "beehive" }, function(event)
  event.animation_speed = 2.0  -- speed up beehive animation
end)
```

---

## Lifecycle Event

The lifecycle phase transition uses `LifecycleEvent` internally, exposed via `minecraft.at_phase()`:

| Field | Type | Description | Mutable |
|-------|------|-------------|---------|
| `previous` | string | Previous phase name (e.g., `"init"`) | No |
| `current` | string | Current phase name (e.g., `"post_init"`) | No |

```lua
minecraft.at_phase("ready", 0, function(event)
  print("From phase " .. event.previous .. " to " .. event.current)
  -- All registration is complete. Register runtime callbacks here.
  minecraft.on(minecraft.events.world_tick, { when = minecraft.util.real_world }, function(tick)
    -- ...
  end)
end)
```

Notable details from `ModLifecycle::advanceTo()`:

```cpp
static void advanceTo(LifecyclePhase phase) {
  LifecycleEvent event{phaseStorage(), phase};
  phaseStorage() = phase;
  hooks().publish(event);
}
```

The event fires AFTER the internal phase has been updated, so `event.previous`
is the old numeric enum (`0`–`3`) and `event.current` is the new numeric enum.

---

## Unsupported Event Names

The following structs exist as C++ event types but are **not registered in the Lua subscriber map** and will produce `"unsupported Lua hook event"` warnings if subscribed:

- `weather_cycle`
- `lightning_strike`
- `snow_ice_placement`
- `random_block_tick`
- `scheduled_block_tick`
- `schedule_block_update`
- `world_time`
- `crafting_take`
- `furnace_output_take`

These may be exposed in future versions.
