# world_profiles (Lua)

Data-driven world profile definitions using generic generation-stage and spawn-selection events. Each profile declares its generator, canceled vanilla stages, and optional spawn-height policy.

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
