# Integrated mod catalogue

## Movement and camera

- **Sprint** — Uses forward double-tap or sprint key state, client tick, player
  travel, and FOV hooks. The config is live: speed, multiplier, and FOV ease are
  read by the main. `start_boost_ticks` is declared but unused; boost duration
  mistakenly starts from the double-tap window.
- **Camera** — Declares auto-rotate/zoom sensitivity, but its main reads missing
  `fov`, `smoothness`, and `rotate_speed`; it computes unused view state and has
  no visible camera mutation in current source.

## Atmosphere and world render

- **Colorful Skies** — Live config drives static or deterministic daily HSB
  palettes, celestial interpolation, and two `world_color` handlers for sky/fog.
- **Fog Settings** — Live config maps reserved `end` through `end_val`, then
  copies every field to `fog_settings`; this is the direct host adapter.
- **Void Fog** — Records camera Y and darkens fog colour below Y=16. Its enabled
  and density fields are declared only; it does not alter fog distance/density.
- **Layered Clouds** — Cancels vanilla clouds, creates moving packed quad layers.
  Vertex buffers are reused without clearing and only count/spacing trigger
  rebuilds.
- **Northern Stars** — Loads/validates an NBT star catalogue, transforms RA/Dec
  with astronomy when present, caches per-minute billboards, cancels vanilla
  stars, and renders additive billboards. Its package config is unused; visual
  setting changes are absent from the cache key.

## Content and generation

- **Stone Bricks**, **Iron Bars**, and **Simple Lantern** register their block
  content at load. Stone Bricks and Iron Bars also register recipes; Lantern has
  none. All declare but ignore `enabled`.
- **Coral** performs a bounded, 1/16 after-features reef pass in deep water on
  sand/dirt. Its cross-edge random placement is clipped to the current chunk.
- **Ravine Backport** performs a 1/35 carver pass. Its intended multi-chunk path
  wraps coordinates into the current chunk, so it repeatedly carves locally.

## Meteors and item physics

Meteors is an operational client effect chain: main → trajectory → geology and
effects. Its ground impact is y<0, impact geology only logs metadata, and periodic
comet scheduling uses an accidental global.

Item Drop Physics has a complete file layout but not a working integration. Its
entry cannot subscribe to the events it names, and later modules have missing
functions/constants and a Minetest/minecraft API mismatch. See Rendering for the
failure sequence rather than enabling it as a feature.
