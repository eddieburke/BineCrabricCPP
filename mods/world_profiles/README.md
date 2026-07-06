# world_profiles (Lua)

Data-driven world profile definitions using generic generation-stage and spawn-selection events. Each profile declares its generator, canceled vanilla stages, and optional spawn-height policy.

The selected profile is stored per world in the namespaced `world_profiles:type`
creation option. The generic `create_world`/`world_open` lifecycle restores it
before spawn selection or chunk generation begins. Offline generation probes
use the currently selected Create World profile when they request
`mod_generation`.
