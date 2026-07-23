# Rendering, sky, and simulation

## Realtime Sky

The active entry is `realtime_sky/scripts/main.lua`. It imports the solar,
cities, globe, and settings modules, persists `realtime_sky.txt`, and owns the
settings/UI state. Its execution trace is:

1. A World Settings toggle enables realtime mode.
2. Sky-before builds or reuses a 50 ms cached solar frame.
3. With `drive_sun`, it writes astronomy/celestial fields, cancels vanilla sky,
   and draws a custom sky dome.
4. World-colour and fog hooks derive colours from solar altitude.
5. Stars-before sets brightness; terrain-before draws procedural Sun/Moon.
6. Non-remote world ticks align game phase to apparent solar time.

The globe screen supports filtered cities, coast picking, drag/zoom, DST, and
immediate persistence of a picked location. `earth_time_solar.lua` is the active
physical model: curated time zones/fixed offsets, simplified DST, low-order NOAA
sun position, lunar perturbations/parallax, and a renderer frame. Unknown IANA
zone names fall back to a longitude-derived offset.

`astronomy/calculator.lua`, `rendering/skybox.lua`, and `rendering/celestial.lua`
are documented standalone helpers but are not imported by the active entry.
They provide simplified solar/lunar math, a cached inward 12×32 sky dome, and
celestial geometry/pixel shading respectively.

## Shared rendering utilities

`lib/rendering/init.lua` exports:

- `BatchRenderer`: begin, append quad/cube records, flush through LÖVE graphics.
  Flush is 2D and ignores z/UV data.
- `DebugRenderer`: retained lines/points, box/arrow expansion, lifetime update,
  and orthographic render.
- `ParticleSystem`: pooled particles with emit, motion/lifetime update, render,
  and stats.

These are manual utilities: callers must update and render them. Particle update
currently reads undefined `options.initial_alpha` for a live particle, and its
stats method returns before counting. See `mods/lib/rendering/init.lua:12-449`.

## Meteors

`meteors/scripts/main.lua` orchestrates settings/keybinds, a meteor list,
trajectory updates, impact effects, and render submission. `physics/trajectory`
creates records, applies gravity/drag, fragments under stress, leaves trails, and
reports ground impact. `geology/generator` creates/caches cube-sphere models
and supplies an impact descriptor; the impact itself is log-only.
`rendering/effects` appends bounded camera-facing glow/ribbon geometry for
meteors, explosions, and comets. Main converts four vertices to each render quad.

## Item Drop Physics

`item_drop_physics/config/init.lua` declares defaults, normalizes numeric values,
and chooses storage/asset data with cached per-item/block properties. The entry
intends to initialize that database, advance simulations on tick, and create a
renderer simulation when an item spawns. `item_renderer.lua` caches voxel/shape
data, derives a body, and caches quaternion render state. `physics/world.lua`
intends gravity, fluid effects, drag, substeps, sleep, and per-simulation update.

This path is presently not operational without fixes: it subscribes to nonexistent
`minecraft.events.tick` and `item_spawn`; world physics references undefined
constants and a missing water function; water/config use Minetest APIs; and
`water.fluid_cell` needs an argument callers omit. Treat it as an implementation
sketch, not a usable item-physics service.

## Box3D

`lib/physics/box3d.lua` implements pooled axis-aligned bodies, AABB contact
detection, impulse/friction resolution, and a spatial-hash World step. The
intended world loop is hash query → pair detection → resolve → integrate → rebuild
hash. It has confirmed blockers: collision uses nonexistent `core.vec3_lerp`,
pair ordering compares tables with `<`, and query bounds leak pooled vectors.
Do not enable collision simulation before correcting those defects.
