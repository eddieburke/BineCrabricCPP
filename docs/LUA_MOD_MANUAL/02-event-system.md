# 02 - Event System & Engine Hooks

BineCrabricCPP exposes an event-driven hook model allowing Lua mods to intercept, modify, or extend engine operations across game ticks, rendering passes, player interactions, world generation, and GUI rendering.

---

## 1. Event Subscription Architecture

### `minecraft.on(event_name, options, callback)`

- **Signature**: `minecraft.on(event_name: string, options: table, callback: function) -> subscription_handle`
- **Parameters**:
  - `event_name`: String identifier from `minecraft.events`.
  - `options`: Table of filter criteria and execution settings:
    - `priority` (number): Execution order priority (default `0`, higher priorities run first).
    - `once` (boolean): Unsubscribes callback automatically after one execution.
    - `when` (function): Predicate `function(event) -> boolean`. Callback executes only if `when` returns `true`.
    - **Property Filtering**: Any key in `options` (other than `priority`, `once`, `when`) matches against `event[key]`:
      - Exact value match: `{ block_id = 155, right_click = true }`
      - Value array match: `{ stage = { "entities", "particles" } }`
      - Predicate function match: `{ damage = function(d) return d > 5 end }`
  - `callback`: Function signature `function(event) -> event | nil`.
    - Mutating fields on `event` passes modified data to subsequent priority listeners and back into the native C++ engine.

---

## 2. High-Level Event Registration API

### `minecraft.event.register(name, callback)`

Simplified event helper covering common lifecycle and rendering patterns:

| Name | Engine Event Mapped | Default Options / Priority | Parameters Passed to Callback |
| :--- | :--- | :--- | :--- |
| `"tick"` | `client_tick` (client) / `world_tick` (server) | `after_world = true` | `dt` (`0.05` sec) |
| `"render"` | `world_render` | `stage = "entities", moment = "after", priority = 120` | `camera` `{ x, y, z, yaw, pitch }` |
| `"render_clouds"` | `world_render` | `stage = "clouds", moment = "before"` | `camera`, `tick_delta` |
| `"fov"` | `fov` | `priority = 100` | `event` |
| `"player_move"` | `player_travel` | `is_local_player = true, priority = 100` | `event` |
| `"world_unload"` | `world_open` | `{}` | `nil` |

---

## 3. Complete Engine Event Catalog

Below is the complete specification of all engine event payload schemas (`minecraft.events`):

### World & Game Ticks

#### `client_tick`
- Payload: `{ paused: boolean, tick_delta: number }`
- Triggered: Every client render frame tick update.

#### `world_tick`
- Payload: `{ world_time: number, tick: number }`
- Triggered: Every 1/20th second game tick on active worlds.

#### `tick_rate`
- Payload: `{ tick_rate: number }`
- Triggered when server tick rate updates.

---

### Player & Entity Interaction

#### `block_interact`
- Payload: `{ x: integer, y: integer, z: integer, block_id: integer, meta: integer, side: integer, player: entity, item_id: integer, right_click: boolean, cancelled: boolean }`
- Triggered: When a player clicks or right-clicks a block. Set `event.cancelled = true` to prevent default block interaction.

#### `entity_interact`
- Payload: `{ entity_id: integer, player: entity, item_id: integer, right_click: boolean, cancelled: boolean }`

#### `attack_damage`
- Payload: `{ attacker: entity, target: entity, damage: integer, cancelled: boolean }`
- Set `event.damage = new_value` to mutate damage or `event.cancelled = true` to mitigate hit.

#### `player_travel`
- Payload: `{ is_local_player: boolean, speed: number, boost: number, jump: boolean, yaw: number, pitch: number }`
- Mutate `event.speed` or `event.boost` to modify movement physics dynamically.

#### `key_press` & `mouse_button`
- `key_press`: `{ key: integer, action: integer, mods: integer, key_name: string }`
- `mouse_button`: `{ button: integer, action: integer, x: number, y: number }`

---

### Entity Lifecycle Events

- **`entity_spawn`**: `{ entity: userdata, entity_id: integer, type_id: integer, x: number, y: number, z: number }`
- **`entity_remove`**: `{ entity_id: integer }`
- **`entity_teleport`**: `{ entity_id: integer, x: number, y: number, z: number }`
- **`entity_tick`**: `{ entity: userdata, entity_id: integer, delta: number }`

---

### Camera & Rendering Hooks

#### `world_render`
- Payload: `{ stage: string, moment: string, camera_x: number, camera_y: number, camera_z: number, camera_yaw: number, camera_pitch: number, tick_delta: number, shadow_pass: boolean, has_world: boolean }`
- **Stages** (`minecraft.render.stages`): `"sky"`, `"stars"`, `"terrain_opaque"`, `"entities"`, `"particles_lit"`, `"particles"`, `"terrain_translucent"`, `"weather"`, `"clouds"`, `"hand"`, `"framebuffer"`.
- **Moments** (`minecraft.render.moments`): `"before"`, `"after"`.

#### `pre_entity_render` & `entity_render`
- `pre_entity_render`: `{ entity: entity, entity_id: int, x: double, y: double, z: double, tick_delta: float, cancelled: bool }` (set `cancelled = true` to skip rendering entity).
- `entity_render`: `{ entity: entity, entity_id: int, x: double, y: double, z: double, tick_delta: float }`.

#### `pre_tile_entity_render` & `tile_entity_tick`
- Payload: `{ x: int, y: int, z: int, block_id: int, tile_entity: userdata, cancelled: bool }`.

#### `fog_settings` & `world_color`
- `fog_settings`: `{ density: float, red: float, green: float, blue: float, start_val: float, end_val: float }`.
- `world_color`: `{ color_type: string ("sky"|"fog"), time: double, red: float, green: float, blue: float }`.

#### `fov` & `camera_setup`
- `fov`: `{ fov: float, fov_multiplier: float }`.
- `camera_setup`: `{ x: double, y: double, z: double, yaw: float, pitch: float, fov: float }`.

---

### Screen & GUI Events

- **`screen_ui`**: `{ screen_id: string, region: string, ui: table }`
- **`screen_event`**: `{ screen_id: string, phase: string ("init"|"render"|"mouse"|"key"), width: int, height: int, mouse_x: int, mouse_y: int, key: int, button: int, released: bool, handled: bool }`
- **`screen_region`**: `{ screen_id: string, region: string, x: int, y: int, width: int, height: int }`

---

### Procedural World Generation Events

#### `chunk_generation`
- Payload: `{ chunk_x: int, chunk_z: int, stage: string, moment: string, mod_generation: bool }`
- **Stages** (`minecraft.generation.stages`): `"terrain"`, `"surface"`, `"carver"`, `"features"`.
- **Moments** (`minecraft.generation.moments`): `"before"`, `"after"`.

#### Lifecycle World Events
- **`create_world`**: `{ world_name: string, seed: integer }`
- **`world_open`**: `{ world_name: string }`
- **`world_start`**: `{ world_name: string }`
- **`world_spawn_search`**: `{ spawn_x: int, spawn_z: int }`
