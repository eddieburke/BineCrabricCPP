# Volume IX — Shipped Mod Gallery

Walkthrough of every mod in `native/mods/`. Read sources alongside this chapter.

| Mod | Lines | Primary APIs |
|-----|-------|--------------|
| stone_bricks | 21 | register_block, register_shaped_recipe |
| iron_bars | 48 | connected_bars model |
| simple_lantern | 45 | manual model, tessellator |
| repair_table | 39+ | box_list, on_use, screen.slots |
| coral | 100 | coordinate bounds/color, chunk_generation |
| ravine_backport | 39 | chunk_generation carver |
| world_profiles | 237 | create_world, world_open, chunk_generation |
| void_fog | 34 | client_tick, world_color |
| colorful_skies | 48 | world_color sky |
| layered_clouds | 261 | world_render clouds, screen.settings |
| northern_stars | 137 | read_nbt_asset, billboards, astronomy |
| realtime_sky | 441+ | astronomy, screen.settings, begin_3d, globe_ui |
| meteors | 58 | particles, key_press |
| critical_hit | 31 | attack_damage, particles |
| sprint | 109 | client_tick, player_travel, fov, key_code |
| too_many_items | 119 | screen_region, items.ids, inventory |
| item_audio_demo | 96 | register_block box_list, sound |
| camera | 470+ | camera channels, render.quads, manual model, on_lua_screen |
| seedfinder | 1912 | sample_grid, create_texture, on_lua_screen |
| item_drop_physics | 148 | pre_entity_render, world_render, get_texture_pixels, render.quads |

---

## stone_bricks

**Minimal content mod.** Registers block id 98 with vanilla terrain atlas tile 7.

```lua
minecraft.register_block({
  id = 98,
  texture_id = 7,
  hardness = 2.0,
  resistance = 10.0,
  translation_key = "stoneBrick",
  material = "stone",
  model = { type = "full_cube" },
})
```

Recipe: 2×2 stone (`item_id = 1`) → 4 stone bricks. Demonstrates `texture_id` instead of custom PNG.

**Lessons:** numeric ids must match reserved vanilla slots; recipes use `register_shaped_recipe` at startup.

---

## iron_bars

**Connected bars model** — arms extend to neighbors matching connect rules.

```lua
model = {
  type = "connected_bars",
  core = { min = {0.4375,0,0.4375}, max = {0.5625,1,0.5625} },
  north = { ... }, south = { ... }, east = { ... }, west = { ... },
  connect = { "same", "opaque", "glass", "fence" },
}
```

Custom texture at `mods/iron_bars/iron_bars.png`. Opaque false, full_cube false.

**Lessons:** box coords in 0..1 block space; default connect rules if `connect` omitted.

---

## simple_lantern

**Manual tessellated block** — no JSON model file.

Helper `quad()` wraps `minecraft.tessellator.quad` with 4 corners and UVs. `draw_lantern()` emits ~10 quads for lantern shape.

```lua
model = {
  type = "manual",
  opaque = false,
  full_cube = false,
  draw = draw_lantern,
}
```

**Lessons:** UVs index into block texture atlas; manual draw runs in world + inventory contexts.

---

## repair_table

**Block + GUI composition.**

1. `register_block` id 150, `box_list` table top, `on_use` opens repair UI on right-click.
2. `minecraft.require("repair_screen")` — separate module implements `screen.slots` with 2 slots (input + material).
3. `minecraft.stack.combine_damage` in slot change handler for repair logic.
4. Shaped recipe: planks pattern → repair table block.

**Lessons:** `behavior_priority` on block; `event.handled = true` in `on_use`; modular scripts via `require`.

---

## coral

**Block + worldgen feature.**

Block id 180: non-opaque, `varied_bounds`, `coordinate_color`, custom texture.

```lua
minecraft.on(minecraft.events.chunk_generation, {
  stage = minecraft.generation.stages.features,
  moment = minecraft.generation.moments.after,
  when = minecraft.util.real_world,
}, function(event)
  -- scan chunk surface, place CORAL_ID on sand/dirt underwater patches
end)
```

Uses `minecraft.chunk.get_height`, `get_block`, `set_block`. Resolves vanilla ids via `minecraft.world.block_id`.

**Lessons:** feature stage for decorations; `real_world` filter excludes probes.

---

## ravine_backport

**Carver stage column carving.**

Deterministic per-chunk from `world_seed`, `chunk_x`, `chunk_z`. ~1/35 chunks get a ravine. `carve_column` clears blocks vertically with `minecraft.chunk.set_block`.

**Lessons:** carver `after` moment mutates terrain post-vanilla; local coords via `value % 16`.

---

## world_profiles

**Create World profile system** using only generic hooks.

1. **`create_world`:** writes `world_profiles:type` into `event.options`.
2. **`world_open`:** reads option, selects profile (`default`, `flatlands`, `highlands`, `caves`).
3. **`screen_ui` on create_world:** adds profile cycle button via `host_fields`.
4. **`chunk_generation`:** profiles cancel carver/features or no-op; flatlands skips terrain features.
5. **`world_spawn_search`:** highlands/caves adjust Y range.

**Lessons:** namespaced `level.dat` options; no engine special cases; profile table drives behavior.

---

## void_fog

**Height-based fog darkening.**

1. `client_tick` (after world, overworld): cache `event.camera_y`.
2. `world_color` kind `fog`: multiply RGB by `1 - darkness` where darkness ramps below Y=16.

**Lessons:** composable fog tinting; no `cancel_vanilla`; separate tick cache from color event.

---

## colorful_skies

**Procedural sky tint from celestial angle.**

`sky_color_for_celestial(celestial)` — sin/cos palette. `world_color` kind `sky` blends toward tint preserving luminance.

Filters: overworld, uses `event.celestial` from engine.

**Lessons:** chain-friendly RGB edit; strength parameter for subtle vs vivid.

---

## layered_clouds

**Replace vanilla clouds** with stacked quad layers.

1. Legacy cfg: `mod_LayeredClouds.cfg` via `config.load`/`save`.
2. `screen.settings` parented on detail settings footer; hotkey opens settings.
3. `world_render` clouds **before:** `cancel_vanilla = true`.
4. **after:** draw N layers of `render.quads` with procedural offsets, wind animation from `client_tick` time.

**Lessons:** cancel + custom draw; settings helper; cfg migration path.

---

## northern_stars

**Real star catalog rendering.**

1. `read_nbt_asset("assets/star_catalog.nbt")` — versioned arrays RA/Dec/magnitude.
2. Compile billboards when UTC/observer lat/lon changes (cache key).
3. `astronomy.horizontal_from_equatorial` per star.
4. `world_render` stars **after:** `render.billboards` with magnitude→size/alpha curve.

**Lessons:** NBT asset pipeline; expensive work cached; brightness tied to `event.star_brightness`.

---

## realtime_sky

**Largest sky mod** — solar time, DST, globe UI.

Modules (`minecraft.require`):

| Module | Role |
|--------|------|
| `earth_time_solar` | Time zone, DST, solar position |
| `places` | Preset locations |
| `globe_ui` | 3D globe, coastlines, picking |

Hooks:

- `world_render` sky before: astronomy fields, celestial override
- `world_color` sky/fog: optional tint
- `screen.settings` + `on_lua_screen` for globe
- `gui.begin_3d` / `unproject` for click→lat/lon
- Config: `realtime_sky.txt` legacy + in-memory settings table

**Lessons:** split large mods across Lua files; globe picking is mod code not engine; provider priority ordering.

---

## meteors

**Particle meteor shower.**

- `client_tick`: random chance spawns shower near player
- `key_press` M: manual trigger
- Computes trajectory; spawns 8 `particles.spawn` along trail with velocity

**Lessons:** `world.player()` for position; particles for effects without entities.

---

## critical_hit

**Fall critical feedback.**

`attack_damage` when `critical` and airborne (`fall_distance > 0.5`): spawn 10 gold particles at target position.

**Lessons:** combat hook is server-authoritative path; client particles for juice.

---

## sprint

**Movement modifier** — double-tap forward or hold sprint key.

1. `client_tick`: track key state, double-tap window (7 ticks), sprint key binding via `key_code("sprint")` fallback 29.
2. `player_travel` `is_local_player`: multiply `speed_multiplier` by 1.45 (+ start boost).
3. `fov`: ×1.08 while sprinting.

**Lessons:** normalize via `speed_multiplier` not raw forward; bind-aware keys; FOV feedback.

---

## too_many_items

**Creative sidebar** on inventory.

1. `key_press` O toggles `visible`.
2. `screen_region` inventory `side_panel`:
   - `render`: draw item grid from `items.ids()`, scroll offset
   - `mouse_scroll`: paginate
   - `mouse_click`: left/right → cursor or give stack
3. `when = function() return visible end` gate.

**Lessons:** region bounds from event; no custom screen needed; uses full item id list.

---

## item_audio_demo

**Custom item + sounds.**

- `register_item` id 30777, `box_list` crystal model, custom texture
- `sound.register` effect + streaming kinds (same wav)
- `block_interact` or tick hook plays sounds on use (see full source)

**Lessons:** high item ids (256+); streaming for loop; package paths under `resources/mods/item_audio_demo/`.

---

## seedfinder

**Full application mod** (~1900 lines) on generic APIs only.

Features:

- Rule DSL for biome/height/block conditions
- Cooperative async seed search (state machine in `tick`/`client_tick`)
- Biome preview map via `world.sample_grid` + `render.create_texture`
- Custom `on_lua_screen` UI: text fields, buttons, keyboard nav
- `files.pick` / `files.read` for rule import
- `util.json_encode` / `json_decode` for persistence

**Lessons:** no native seedfinder engine; `sample_grid` with `mod_generation` for accurate previews; large UIs without engine widgets beyond gui primitives.

---

## item_drop_physics

**3D Voxel Extrusion Mod** (~150 lines) rendering dropped item entities as extruded 3D voxel models instead of flat billboard quads.

Features:

- Cancels the engine's flat rendering of dropped item entities via `pre_entity_render`.
- Queries the item entity lists via `minecraft.entities.list("Item")` during `world_render`.
- Fetches and caches the item texture pixels via `minecraft.render.get_texture_pixels`.
- Builds a 3D voxel grid representation of the item icon, with full neighbor-face culling optimization to maximize rendering performance.
- Extrudes, scales, rotates (spins), and translates the 3D voxel vertices dynamically to follow the item entity's bobbing and movement.
- Submits the transformed 3D quads via `minecraft.render.quads`.

**Lessons:** Cancel default rendering via cancelable events, fetch raw texture colors to generate dynamic 3D geometry in Lua, optimize voxel vertex counts with neighbor face culling.

---

## Tutorial: your first mod

1. Copy `stone_bricks` layout.
2. Change `mod.json` id.
3. Pick unused block id (check registry).
4. Add `assets` texture or `texture_id`.
5. Package and drop in `.minecraft/mods/`.

## Tutorial: runtime recipe

After world load:

```lua
minecraft.crafting.add_shaped_recipe({
  output_block_id = 98,
  output_count = 1,
  pattern = { "A" },
  key = "A",
  item_id = 263,  -- coal
})
```

Use from `block_interact`, command hook, or unlock screen — not during `main.lua` top-level if ingredients are not registered yet.

---

*API details: Volumes III–VIII. Event fields: Volume II.*
