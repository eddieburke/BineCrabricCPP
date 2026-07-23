# File-by-file source coverage

Every Lua file under `native/mods` is listed here. Descriptions are execution
traces, not filename guesses. Detailed contracts are linked from the chapter
named after each area.

The canonical source paths are included in the manifest at the end of this page;
the shorter paths in the catalogue are relative to `native/mods`.

## Core libraries and template

- `lib/config.lua` — config load/save forwarding wrapper; ignores `mod_name`.
- `lib/math_util.lua` — scalar/vector helpers and deterministic hashes.
- `lib/screen_ui.lua` — Mod Settings footer button helper.
- `lib/audio/init.lua` — LÖVE source pool, cache, sound/music/playlist lifecycle.
- `lib/core/init.lua` — loader, internal priority bus, pools, vectors, AABB,
  spatial hash, scalar math, timers.
- `lib/physics/box3d.lua` — pooled AABB bodies/contact solver/world loop; blocked
  by missing vector helper and invalid pair comparison.
- `lib/rendering/init.lua` — LÖVE batch, debug, and particle render utilities.
- `lib/settings/init.lua` — cached native/local settings proxy and validation.
- `lib/ui/init.lua` — retained LÖVE element/button/label/slider/panel widgets.
- `templates/standard_mod/main.lua` — reference state/entity/physics loop; no
  host bridge and registers an undefined chat handler.

## Small content, input, and atmosphere mods

- `camera/config/init.lua` — auto-rotate and zoom schema.
- `camera/scripts/main.lua` — incomplete tick/FOV/render experiment.
- `colorful_skies/config/init.lua` — active palette, twilight, and fog schema.
- `colorful_skies/scripts/main.lua` — time-colour interpolation and sky/fog hooks.
- `coral/config/init.lua` — inert enabled schema.
- `coral/scripts/main.lua` — block registration and bounded reef feature pass.
- `critical_hit/config/init.lua` — inert enabled schema.
- `critical_hit/scripts/main.lua` — airborne critical-damage enhancement/particles;
  event reference is absent from public prelude table.
- `fog_settings/config/init.lua` — active fog schema with `end_val` name bridge.
- `fog_settings/scripts/main.lua` — config-to-fog event adapter.
- `iron_bars/config/init.lua` — inert enabled schema.
- `iron_bars/scripts/main.lua` — iron-bars block/model/recipe registration.
- `layered_clouds/config/init.lua` — layer geometry/wind schema.
- `layered_clouds/scripts/main.lua` — custom cloud quads and vanilla cancellation.
- `northern_stars/config/init.lua` — inert package schema.
- `northern_stars/scripts/main.lua` — NBT catalogue, astronomy transform, stars pass.
- `simple_lantern/config/init.lua` — inert enabled schema.
- `simple_lantern/scripts/main.lua` — luminant lantern block registration.
- `sprint/config/init.lua` — active speed, boost, FOV, and key schema.
- `sprint/scripts/main.lua` — double-tap/held sprint state and travel/FOV mutation.
- `stone_bricks/config/init.lua` — inert enabled schema.
- `stone_bricks/scripts/main.lua` — stone-brick block and 2×2 recipe.
- `void_fog/config/init.lua` — inert enabled/density schema.
- `void_fog/scripts/main.lua` — low-altitude fog-colour darkener.

## Item Drop Physics and Meteors

- `item_drop_physics/config/init.lua` — physics defaults, storage/asset load,
  property normalizer/cache; uses Minetest deserialization.
- `item_drop_physics/scripts/main.lua` — intended init/tick/item-spawn coordinator;
  has invalid events and undefined sync interval.
- `item_drop_physics/rendering/item_renderer.lua` — bounds/voxel cache, body/sim
  creation, quaternion render cache.
- `item_drop_physics/physics/world.lua` — intended gravity/fluid/drag/sleep loop;
  missing water API/constants and no collision phase.
- `item_drop_physics/physics/water.lua` — Minetest-oriented fluid cells/buoyancy/
  flow helpers; disconnected and called with missing callback.
- `meteors/config/init.lua` — meteor settings, key bindings, default getter.
- `meteors/scripts/main.lua` — input/tick/render orchestrator and list ownership.
- `meteors/physics/trajectory.lua` — motion, thermal fragmentation, trail, impact.
- `meteors/geology/generator.lua` — deterministic cube-sphere mesh/cache and
  log-only impact descriptor.
- `meteors/rendering/effects.lua` — bounded glow, ribbon, explosion, comet quads.

## Screens and inventory

- `offline_mode/config/init.lua` — inert enabled schema.
- `offline_mode/scripts/main.lua` — independent offline username persistence/
  session bridge/login settings screen.
- `repair_table/config/init.lua` — inert enable/cost schema.
- `repair_table/scripts/main.lua` — table block, recipe, right-click entry.
- `repair_table/scripts/repair_screen.lua` — three-slot combine-and-consume flow.
- `repair_table/scripts/inventory_helper.lua` — reusable stacks and custom slot UI.
- `seedfinder/config/init.lua` — inert enabled schema.
- `seedfinder/scripts/main.lua` — Create World injection, global state, finder router.
- `seedfinder/scripts/search.lua` — incremental search, import, results/maps, host nav.
- `seedfinder/scripts/rules.lua` — rule JSON, grid analysis, hard filter/scoring.
- `seedfinder/scripts/ui_spec.lua` — transactional rule-card editor.
- `too_many_items/config/init.lua` — inert enable/page schema.
- `too_many_items/scripts/main.lua` — O-toggle item panel, grid, grant actions.

## Realtime Sky

- `realtime_sky/config/init.lua` — defaults, serialized aliases, renderer constants.
- `realtime_sky/scripts/main.lua` — active persistence, settings/globe screens,
  solar frame cache, sky/fog/stars/celestial/tick hooks.
- `realtime_sky/scripts/earth_time_solar.lua` — zone/DST parse, observer time,
  solar/lunar horizontal positions, render-frame API.
- `realtime_sky/scripts/globe_ui.lua` — coastline geometry, clipped projection,
  map picking, zoom/drag and cache API.
- `realtime_sky/scripts/cities.lua` — city asset parser/sorter/fallback/filter.
- `realtime_sky/scripts/places.lua` — 20-place fallback/filter.
- `realtime_sky/scripts/settings_screen.lua` — standalone settings lifecycle.
- `realtime_sky/astronomy/calculator.lua` — unintegrated simplified astronomy API.
- `realtime_sky/rendering/skybox.lua` — unintegrated sky colour/dome renderer.
- `realtime_sky/rendering/celestial.lua` — unintegrated celestial geometry/shading.

## World Profiles

- `world_profiles/config/init.lua` — enabled schema, currently unread by main.
- `world_profiles/scripts/common.lua` — AIR/block-id/hash helpers.
- `world_profiles/scripts/main.lua` — profile registry, persistence, UI, spawn,
  generation cancellation and decoration dispatch.
- `world_profiles/scripts/worldtypes/default.lua` — vanilla metadata profile.
- `world_profiles/scripts/worldtypes/flatlands.lua` — flat slab post-surface profile.
- `world_profiles/scripts/worldtypes/highlands.lua` — deterministic uplift decorator.
- `world_profiles/scripts/worldtypes/caves.lua` — spawn bounds/overburden/pockets.
- `world_profiles/scripts/worldtypes/infdev_20100227.lua` — cached Java RNG/noise
  terrain-only Infdev implementation.
- `world_profiles/scripts/worldtypes/infdev_20100415.lua` — density/surface/ore/
  tree Infdev implementation with bounded height cache.
- `ravine_backport/config/init.lua` — inert enabled schema.
- `ravine_backport/scripts/main.lua` — deterministic after-carver local ravine pass.

## Canonical source manifest

```text
native/mods/camera/config/init.lua
native/mods/camera/scripts/main.lua
native/mods/colorful_skies/config/init.lua
native/mods/colorful_skies/scripts/main.lua
native/mods/coral/config/init.lua
native/mods/coral/scripts/main.lua
native/mods/critical_hit/config/init.lua
native/mods/critical_hit/scripts/main.lua
native/mods/fog_settings/config/init.lua
native/mods/fog_settings/scripts/main.lua
native/mods/iron_bars/config/init.lua
native/mods/iron_bars/scripts/main.lua
native/mods/item_drop_physics/config/init.lua
native/mods/item_drop_physics/physics/water.lua
native/mods/item_drop_physics/physics/world.lua
native/mods/item_drop_physics/rendering/item_renderer.lua
native/mods/item_drop_physics/scripts/main.lua
native/mods/layered_clouds/config/init.lua
native/mods/layered_clouds/scripts/main.lua
native/mods/lib/audio/init.lua
native/mods/lib/config.lua
native/mods/lib/core/init.lua
native/mods/lib/math_util.lua
native/mods/lib/physics/box3d.lua
native/mods/lib/rendering/init.lua
native/mods/lib/screen_ui.lua
native/mods/lib/settings/init.lua
native/mods/lib/ui/init.lua
native/mods/meteors/config/init.lua
native/mods/meteors/geology/generator.lua
native/mods/meteors/physics/trajectory.lua
native/mods/meteors/rendering/effects.lua
native/mods/meteors/scripts/main.lua
native/mods/northern_stars/config/init.lua
native/mods/northern_stars/scripts/main.lua
native/mods/offline_mode/config/init.lua
native/mods/offline_mode/scripts/main.lua
native/mods/ravine_backport/config/init.lua
native/mods/ravine_backport/scripts/main.lua
native/mods/realtime_sky/astronomy/calculator.lua
native/mods/realtime_sky/config/init.lua
native/mods/realtime_sky/rendering/celestial.lua
native/mods/realtime_sky/rendering/skybox.lua
native/mods/realtime_sky/scripts/cities.lua
native/mods/realtime_sky/scripts/earth_time_solar.lua
native/mods/realtime_sky/scripts/globe_ui.lua
native/mods/realtime_sky/scripts/main.lua
native/mods/realtime_sky/scripts/places.lua
native/mods/realtime_sky/scripts/settings_screen.lua
native/mods/repair_table/config/init.lua
native/mods/repair_table/scripts/inventory_helper.lua
native/mods/repair_table/scripts/main.lua
native/mods/repair_table/scripts/repair_screen.lua
native/mods/seedfinder/config/init.lua
native/mods/seedfinder/scripts/main.lua
native/mods/seedfinder/scripts/rules.lua
native/mods/seedfinder/scripts/search.lua
native/mods/seedfinder/scripts/ui_spec.lua
native/mods/simple_lantern/config/init.lua
native/mods/simple_lantern/scripts/main.lua
native/mods/sprint/config/init.lua
native/mods/sprint/scripts/main.lua
native/mods/stone_bricks/config/init.lua
native/mods/stone_bricks/scripts/main.lua
native/mods/templates/standard_mod/main.lua
native/mods/too_many_items/config/init.lua
native/mods/too_many_items/scripts/main.lua
native/mods/void_fog/config/init.lua
native/mods/void_fog/scripts/main.lua
native/mods/world_profiles/config/init.lua
native/mods/world_profiles/scripts/common.lua
native/mods/world_profiles/scripts/main.lua
native/mods/world_profiles/scripts/worldtypes/caves.lua
native/mods/world_profiles/scripts/worldtypes/default.lua
native/mods/world_profiles/scripts/worldtypes/flatlands.lua
native/mods/world_profiles/scripts/worldtypes/highlands.lua
native/mods/world_profiles/scripts/worldtypes/infdev_20100227.lua
native/mods/world_profiles/scripts/worldtypes/infdev_20100415.lua
```
