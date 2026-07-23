# Events and execution phases

Register a callback with `minecraft.on(event, options, callback)`. `options`
supports `priority`, `once`, `when`, and scalar, list, or predicate filters.
The call returns false when validation or subscription fails. The compatibility
form `minecraft.event.register` accepts a small legacy set (`tick`, `render`,
`render_clouds`, `fov`, `player_move`, `world_unload`, `unload`) plus canonical
names. Implementation: `LuaRuntimePrelude.hpp:50-109,491-554`.

## Event map

| Event | Side | Important mutable fields | Primary purpose |
| --- | --- | --- | --- |
| `client_tick` | client | none | Per-frame/tick player and camera state. |
| `render_frame` | client | none | Render timing. |
| `first_person_hand` | client | `canceled` | Hand rendering. |
| `key_press`, `mouse_button` | client | `handled` | Raw input routing. |
| `raycast` | client | none | Hit result notification. |
| `fov` | client | `fov` | Multiply or replace field of view. |
| `camera_setup` | client | pose, `custom_view`, `hide_first_person_hand` | Camera transform. |
| `player_travel` | client | `sideways`, `forward`, `speed_multiplier` | Movement adjustment. |
| `tick_rate` | client | `target_tps`, `tps_scale` | Client tick timing. |
| `world_start`, `world_open` | both | none | World lifecycle and saved options. |
| `world_tick` | server | none | Server-world tick. |
| `entity_tick`, `tile_entity_tick` | both | `canceled`; tile animation speed | Entity lifecycle. |
| `create_world` | server | `canceled`, `options` | Capture or change new-world options. |
| `block_interact`, `entity_interact` | both | `canceled`, `handled`, stack count/damage | Interaction override. |
| `attack_damage` | server | `damage`, `critical`, `canceled` | Damage calculation. |
| `entity_teleport` | server | target pose, `canceled` | Teleport interception. |
| `world_color`, `fog_settings` | client | colour/fog fields | Atmospheric rendering. |
| `entity_render`, `pre_entity_render`, `pre_tile_entity_render` | client | pose/parts or `canceled` | Rendering override. |
| `world_render` | client | `cancel_vanilla`, sky/stars fields | World rendering phases. |
| `chunk_generation` | server | `cancel_vanilla` | Terrain, surface, carver, or feature stage. |
| `world_spawn_search` | server | `x`, `y`, `z`, `resolved` | Custom spawn location. |
| `screen_region`, `screen_ui`, `screen_event` | client | `handled`; UI dimensions where supplied | Host and Lua screen input. |
| `entity_spawn`, `entity_remove` | both | none | Entity notification. |

The canonical names and field-level implementation are in
`LuaEventId.hpp:43-77` and `LuaDirectHooks.cpp:299-1355`. Most event tables also
include `remote` and `side` when dispatched through the common hook path.

## Rendering and generation phases

`world_render` carries a `stage` and `moment`. Realtime Sky uses sky-before to
set celestial state, cancel vanilla sky work, and draw its dome; it uses
terrain-before for Sun and Moon work and stars-before to set brightness.

`chunk_generation` carries the generation stage and moment. World Profiles runs
with priority 100 before `terrain`, `surface`, `carver`, and `features`, cancels
only a profile's named stages, and invokes its generator. It decorates after the
surface stage.

## Important current mismatch

The native hook supports `attack_damage`, but the prelude omits
`minecraft.events.attack_damage`; `critical_hit` consequently subscribes with a
nil event reference. There is no canonical `tick` or `item_spawn` event:
`item_drop_physics` tries both and cannot register through `minecraft.on`.
Use `client_tick`, `world_tick`, or `entity_spawn` as appropriate, or correct the
prelude/entry before relying on those mods.
