# world_profiles (Lua)

Data-driven world profile definitions using generic generation-stage and spawn-selection events. Each profile declares its generator, canceled vanilla stages, and optional spawn-height policy.

Profiles apply per dimension. Each profile declares `dimensions` (`"all"`, a list of dimension ids, or
`function(dimension_id, info)`; default is the overworld only). The create-world screen exposes
`World Type` for the overworld and `Dimension Profiles...` for every dimension returned by
`minecraft.world.dimensions()`, including custom registered dimensions. Selections persist as
`world_profiles:dim:<id>` and chunk hooks resolve the profile from the event's `dimension_id`.

A profile with `terrain = "alpha"` or `"modern"` selects a native generator style and writes
`terrain_style:<id>` into the world options before the world, biome source and chunk generator are
built. It applies to any dimension whose type sets `usesTerrainStyle` (the overworld and custom
dimensions that reuse `OverworldChunkGenerator`). `alpha` removes the climate term from terrain
height and uses a single fixed climate; `modern` shapes continentalness, erosion, peaks/valleys
and rivers inside the fixed 128-block column.

The selected profile is stored per world in the namespaced `world_profiles:type`
creation option. The generic `create_world`/`world_open` lifecycle restores it
before spawn selection or chunk generation begins. Offline generation probes
use the currently selected Create World profile when they request
`mod_generation`.

The `Infdev 20100415` profile ports its decompiled 16/16/8-octave density
generator, 64-block sea, original surface pass, ore population, and Paul
Spooner Forester-derived giant trees. The `Infdev 20100227` profile ports its
earlier two-noise height field, oceans, brick pyramids, and coordinate-axis
obsidian walls. Both adaptations use the saved world seed so regenerated
chunks remain deterministic.
