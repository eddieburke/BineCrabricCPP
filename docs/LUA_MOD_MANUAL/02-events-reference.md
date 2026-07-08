# Volume II — Event Reference (Complete)

Every event name in `minecraft.events` maps to a C++ hook in `LuaEventSubscribers.cpp`. Unknown event names log a warning and are **not** subscribed.

## Subscription recap

```lua
minecraft.on(event_name, { filter_key = value, priority = 0, once = false, when = fn }, callback)
```

- **Priority:** lower integer runs **earlier** among callbacks on the same hook.
- **Native fast filters** (passed to C++ before Lua): `screen_id`, `region`, `stage`, `moment` (string equality only).
- **All other filter keys** are evaluated in Lua by `event_matches()` in the prelude.
- **Return value:** For mutable events, return the event table (or a replacement). Immutable events ignore return.
- **`when`:** `function(event) return bool end` — additional gate.

---

## Shared world context fields

Present on most world-linked events (`client_tick`, `world_tick`, `world_color`, `world_render`, `chunk_generation`, `world_spawn_search`, etc.):

| Field | Type | Description |
|-------|------|-------------|
| `has_world` | bool | Active world exists |
| `world_name` | string | Save folder name |
| `is_overworld` | bool | Not nether; no ceiling dimension |
| `mod_generation` | bool | World executes mod chunk hooks |

Filter with `when = minecraft.util.real_world` to skip engine probe worlds where `mod_generation` is false.

---

## Mutability summary

| Event | Writable fields |
|-------|-----------------|
| `key_press`, `mouse_button` | `handled` |
| `fov` | `fov` |
| `camera_setup` | `x,y,z`, `yaw,pitch,roll`, `custom_view`, `hide_first_person_hand` |
| `player_travel` | `sideways`, `forward`, `speed_multiplier` |
| `tick_rate` | `target_tps`, `tps_scale` |
| `create_world` | `canceled`, `options` |
| `block_interact` | `canceled`, `handled`, `item_damage` |
| `entity_interact` | `canceled`, `handled` |
| `attack_damage` | `damage`, `critical`, `canceled` |
| `world_color` | `r`, `g`, `b` |
| `world_render` | `cancel_vanilla`, sky-before astronomy fields |
| `chunk_generation` | `cancel_vanilla` |
| `world_spawn_search` | `x,y,z`, `resolved` |
| `screen_region`, `screen_event` | `handled`; region `width`, `height` |

Immutable (read-only): `world_start`, `world_open`, `world_tick`, `screen_ui`.

---

## `client_tick`

**Fires:** Every client tick (before/after world simulation phases).

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `before` | bool | R | `true` during pre-tick phase |
| `after_world` | bool | R | `true` during post-world-tick phase |
| `paused` | bool | R | Game paused |
| `has_player` | bool | R | Local player exists |
| `has_world` | bool | R | World loaded |
| `world_name` | string | R | Active world name |
| `is_overworld` | bool | R | Overworld dimension |
| `mod_generation` | bool | R | World has mod generation enabled |
| `camera_y` | number | R | Interpolated camera Y |
| `player_y` | number | R | Player Y |
| `player_fall_distance` | number | R | Player fall distance |
| `player_on_ground` | bool | R | Player on ground |
| `world_time` | number | R | `world.getTime() % 24000` |
| `is_night` | bool | R | Night by world time |

**Typical filters:** `before = false`, `paused = false`, `has_world = true`, `is_overworld = true`.

**Example — void_fog tracks camera height:**

```lua
minecraft.on(minecraft.events.client_tick, {
  before = false, paused = false, has_world = true, is_overworld = true,
}, function(event)
  last_camera_y = event.camera_y or 64.0
end)
```

---

## `world_tick`

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `client_world` | bool | R | Client world tick path |
| `before` | bool | R | Phase flag |
| + world context | | R | `has_world`, `world_name`, `is_overworld`, `mod_generation` |

---

## `key_press`

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `key` | int | R | LWJGL scancode |
| `pressed` | bool | R | Key down vs release |
| `repeat` | bool | R | OS key repeat |
| `handled` | bool | R/W | Set `true` to consume |

**Filter example:** `{ key = minecraft.key_code("o"), pressed = true }`

**Named key codes** (complete `defaultKeyCodeForName` table):

| Name | Code | Name | Code |
|------|------|------|------|
| escape | 1 | q | 16 |
| 1 | 2 | w | 17 |
| 2 | 3 | e | 18 |
| 3 | 4 | r | 19 |
| 4 | 5 | t | 20 |
| 5 | 6 | y | 21 |
| 6 | 7 | u | 22 |
| 7 | 8 | i | 23 |
| 8 | 9 | o | 24 |
| 9 | 10 | p | 25 |
| 0 | 11 | a | 30 |
| enter | 28 | s | 31 |
| | | d | 32 |
| | | f | 33 |
| | | g | 34 |
| | | h | 35 |
| | | j | 36 |
| | | k | 37 |
| | | l | 38 |
| space | 57 | z | 44 |
| up | 200 | x | 45 |
| left_arrow | 203 | c | 46 |
| right_arrow | 205 | v | 47 |
| down | 208 | b | 48 |
| | | n | 49 |
| | | m | 50 |

**Bound keys** (when client active): `forward`, `left`, `back`, `right`, `jump`, `sneak`, `drop`, `inventory`, `chat`, `fog` — resolved from `GameOptions`.

---

## `mouse_button`

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `button` | int | R | 0 = left, 1 = right |
| `pressed` | bool | R | Down vs up |
| `handled` | bool | R/W | Consume click |

---

## `player_travel`

Called when movement input is applied. **Only meaningful for local player** when filtered.

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `sideways` | number | R/W | Strafe input |
| `forward` | number | R/W | Forward input |
| `speed_multiplier` | number | R/W | Speed scale (sprint mods) |
| `has_player` | bool | R | |
| `is_local_player` | bool | R | Filter on this for client mods |

**Example — sprint mod:**

```lua
minecraft.on(minecraft.events.player_travel, { is_local_player = true }, function(event)
  if sprinting and event.forward > 0.0 then
    event.speed_multiplier = (event.speed_multiplier or 1.0) * 1.45
  end
end)
```

---

## `fov`

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `tick_delta` | number | R | Partial tick |
| `fov` | number | R/W | Field of view multiplier |

---

## `tick_rate`

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `target_tps` | number | R/W | Target ticks per second |
| `tps_scale` | number | R/W | TPS scale factor |

---

## `camera_setup`

Per-frame world camera. Use for cinematics, **not** GUI.

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `tick_delta` | number | R | |
| `x`, `y`, `z` | number | R/W | Camera position |
| `yaw`, `pitch`, `roll` | number | R/W | Rotation degrees |
| `custom_view` | bool | R/W | Custom view active |
| `hide_first_person_hand` | bool | R/W | Hide hand model |

---

## `attack_damage`

Server-side entity damage (client receives for local predictions where applicable).

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `damage` | int | R/W | Damage amount |
| `critical` | bool | R/W | Critical hit |
| `canceled` | bool | R/W | Cancel attack |
| `fall_distance` | number | R | Attacker fall distance |
| `on_ground` | bool | R | Attacker on ground |
| `target_x`, `target_y`, `target_z` | number | R | Target position |
| `has_player`, `has_target` | bool | R | |

---

## `block_interact`

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `x`, `y`, `z` | int | R | Block position |
| `block_id` | int | R | Block at position |
| `side` | int | R | Face clicked |
| `right_click` | bool | R | Right click vs left |
| `canceled` | bool | R/W | Cancel interaction |
| `handled` | bool | R/W | Mark handled |
| `has_player` | bool | R | |
| `has_item` | bool | R | Player holding item |
| `item_id`, `item_count`, `item_damage` | int | R/W | Held stack (damage writable if damageable) |
| `item_max_damage`, `item_damageable` | | R | Item metadata |

`register_block` with `on_use` auto-subscribes here filtered by `block_id` + `right_click`.

---

## `entity_interact`

Skipped on remote client worlds for player-initiated attacks.

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `attack` | bool | R | Attack vs use |
| `canceled`, `handled` | bool | R/W | |
| `has_player`, `has_target` | bool | R | |

---

## `world_color`

Composable sky/fog tint. Callbacks chain — each sees prior RGB.

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `kind` | string | R | `"sky"` or `"fog"` — filter with `minecraft.colors.*` |
| `r`, `g`, `b` | number | R/W | 0..1 components |
| `partial_ticks` | number | R | |
| `celestial` | number | R | Normalized celestial angle 0..1 |
| `world_time` | number | R | |
| `is_night` | bool | R | |
| + world context | | R | |

---

## `world_render`

**Managed drawing only inside callback:** `minecraft.render.quads`, `minecraft.render.billboards`.

Filter: `stage` = `sky`|`stars`|`clouds`, `moment` = `before`|`after`.

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `tick_delta` | number | R | |
| `stage`, `moment` | string | R | |
| `cancel_vanilla` | bool | R/W | Skip vanilla pass (before only) |
| `vanilla_stage_ran` | bool | R | Result of vanilla (after) |
| `celestial_angle` | number | R/W | Sky before only |
| `sky_yaw_deg` | number | R/W | Sky before only |
| `star_brightness` | number | R | |
| `rain_strength` | number | R | |
| `stars_enabled` | bool | R | |
| `astronomy_enabled` | bool | R/W | Sky before |
| `astronomy_utc_millis` | number | R/W | Sky before |
| `observer_latitude_deg` | number | R/W | Sky before |
| `observer_longitude_deg` | number | R/W | Sky before |
| `cloud_base_height` | number | R | Clouds stage only |
| `camera_x/y/z` | number | R | |
| `camera_yaw/pitch/roll` | number | R | |
| `custom_camera` | bool | R | |
| + world context | | R | |

---

## `render_targets`

**Fires:** Once per real screen frame, before the main view renders.

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `tick_delta` | number | R | |

Not tied to `world_render`'s stage system — this is the place to call `minecraft.camera.render(...)`
for any offscreen render target your mod wants refreshed this frame. Native does not decide when
targets render; this hook is the only per-frame signal, so all "should I re-render / how often"
policy belongs in the callback. See `minecraft.camera` in Volume VI.

---

## `chunk_generation`

**Only when `has_chunk` is true.** Use `minecraft.chunk.*` APIs.

Skipped on remote worlds. `mod_generation` false for probe worlds unless enabled.

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `stage` | string | R | `terrain`, `surface`, `carver`, `features` |
| `moment` | string | R | `before`, `after` |
| `cancel_vanilla` | bool | R/W | Before moment only |
| `vanilla_stage_ran` | bool | R | |
| `world_seed` | int | R | Signed 64-bit seed as Lua integer |
| `chunk_x`, `chunk_z` | int | R | Chunk coordinates |
| `has_chunk` | bool | R | |
| `mod_generation`, `is_overworld` | bool | R | |
| + world context | | R | |

**Write modes by stage:**

| Stage | Chunk API mode |
|-------|----------------|
| terrain, surface, carver | Raw generation |
| features | Full chunk API |

---

## `world_spawn_search`

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `x`, `y`, `z` | int | R/W | Spawn candidate |
| `resolved` | bool | R/W | Set true when mod picks spawn |
| + world context | | R | |

Skipped on remote worlds.

---

## `create_world`

Create World screen confirm.

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `save_name` | string | R | |
| `seed` | int | R | |
| `canceled` | bool | R/W | Cancel world creation |
| `options` | table | R/W | String→string map persisted to level.dat |

Use namespaced keys: `"my_mod:setting" = "value"`.

---

## `world_open`

Fires before `World` is constructed.

| Field | Type | Description |
|-------|------|-------------|
| `save_name` | string | |
| `new_world` | bool | |
| `options` | table | Restored from level.dat |

---

## `world_start`

| Field | Type | Description |
|-------|------|-------------|
| `save_name` | string | |
| `new_world` | bool | |

---

## `screen_region`

Draw/interact in a rectangular region of a vanilla screen. **GUI draw valid in render phase.**

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `phase_name` | string | R | `render`, `mouse_click`, `mouse_scroll` |
| `screen_id` | string | R | e.g. `inventory` |
| `region` | string | R | `footer`, `screen`, `side_panel` |
| `x`, `y`, `width`, `height` | int | R/W | Region bounds (`width`/`height` writable) |
| `mouse_x`, `mouse_y` | int | R | |
| `button` | int | R | Mouse button (click phase) |
| `scroll_delta` | int | R | Scroll wheel delta |
| `handled` | bool | R/W | |

**too_many_items** uses `side_panel` on inventory with `when = function() return visible end`.

---

## `screen_ui`

Inject buttons into vanilla screen regions during UI build.

| Field | Type | Description |
|-------|------|-------------|
| `screen_id`, `region` | string | |
| `ui` | table | Builder API |
| `host_fields` | table | String fields on host screen |

**`event.ui` methods:**

| Method | Arguments | Description |
|--------|-----------|-------------|
| `add_button` | text, x, y, w, h, callback | Absolute button |
| `add_centered_button` | text, y, w, h, callback | Horizontally centered |
| `add_stacked_centered_button` | text, callback | Stacks in region footer |

---

## `screen_event` (Lua custom screens)

Phases: `init`, `render`, `tick`, `key`, `mouse`, `scroll`, `close`.

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `screen_id` | string | R | |
| `phase` | string | R | |
| `width`, `height` | int | R | |
| `mouse_x`, `mouse_y` | int | R | Alias `x`, `y` |
| `tick_delta` | number | R | |
| `key` | int | R | Key phase |
| `char` | int | R | Character code |
| `button` | int | R | Mouse phase |
| `released` | bool | R | Mouse up |
| `delta` | int | R | Scroll delta |
| `handled` | bool | R/W | |

**GUI drawing** (`minecraft.gui.*`) is valid during `render` phase and `screen_region` render phase.

---

## Lifecycle events (`minecraft.at_phase`)

Not subscribed via `minecraft.on`. Use:

```lua
minecraft.at_phase(minecraft.lifecycle.crafting_recipe_registration, 50000, function(event)
  -- event.previous, event.current (numeric enum values)
end)
```

Phase name strings match `minecraft.lifecycle.*` constants.

---

## `pre_entity_render`

Fired before any entity is rendered. Set `canceled = true` to hide/cancel rendering.

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `entity_id` | int | R | Entity instance ID |
| `entity_type` | string | R | Entity type name (e.g. `"Item"`, `"Pig"`) |
| `tick_delta` | number | R | Partial tick |
| `canceled` | bool | R/W | Set `true` to cancel vanilla rendering |
| `item_id` | int | R | Present only if `entity_type == "Item"` |
| `item_count` | int | R | Present only if `entity_type == "Item"` |
| `item_damage` | int | R | Present only if `entity_type == "Item"` |
| `texture_path` | string | R | Present only if `entity_type == "Item"` |
| `atlas_index` | int | R | Present only if `entity_type == "Item"` |
| `mod_texture` | bool | R | Present only if `entity_type == "Item"` |

---

## `pre_tile_entity_render`

Fired before any block entity (tile entity) is rendered. Set `canceled = true` to hide/cancel rendering.

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `x`, `y`, `z` | int | R | Block position |
| `id` | string | R | Tile entity ID |
| `tick_delta` | number | R | Partial tick |
| `canceled` | bool | R/W | Set `true` to cancel vanilla rendering |

---

## `entity_spawn`

Fired when an entity is spawned in the world.

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `entity_id` | int | R | Entity instance ID |
| `entity_type` | string | R | Entity type name |
| `item_id`, `item_count`, `item_damage` | int | R | Present only if `entity_type == "Item"` |
| `texture_path`, `atlas_index`, `mod_texture` | | R | Present only if `entity_type == "Item"` |

---

## `entity_remove`

Fired when an entity is removed from the world.

| Field | Type | R/W | Description |
|-------|------|-----|-------------|
| `entity_id` | int | R | Entity instance ID |
| `entity_type` | string | R | Entity type name |
| `item_id`, `item_count`, `item_damage` | int | R | Present only if `entity_type == "Item"` |
| `texture_path`, `atlas_index`, `mod_texture` | | R | Present only if `entity_type == "Item"` |

---

## Event constant list

```lua
minecraft.events.attack_damage
minecraft.events.block_interact
minecraft.events.chunk_generation
minecraft.events.client_tick
minecraft.events.create_world
minecraft.events.entity_interact
minecraft.events.fov
minecraft.events.camera_setup
minecraft.events.key_press
minecraft.events.mouse_button
minecraft.events.player_travel
minecraft.events.screen_event
minecraft.events.screen_region
minecraft.events.screen_ui
minecraft.events.tick_rate
minecraft.events.world_color
minecraft.events.world_render
minecraft.events.render_targets
minecraft.events.world_open
minecraft.events.world_spawn_search
minecraft.events.world_start
minecraft.events.world_tick
minecraft.events.pre_entity_render
minecraft.events.pre_tile_entity_render
minecraft.events.entity_spawn
minecraft.events.entity_remove
```
