# World Profiles 3

World Profiles uses explicit generation stages and a strict saved-world schema. Version 1 profile definitions and version 1/2 saved profile options are rejected; there is no migration or compatibility layer. Worlds with no World Profiles metadata use the explicit Default profile.

Each definition must declare its id, label, order and supported dimensions. Native terrain styles additionally require the dimension's terrain_styles capability. Duplicate ids, duplicate option keys, invalid stage names, unavailable blocks and malformed saved values are errors.

| Definition field | Behavior |
| --- | --- |
| `stages.<stage>` omitted | Run the native stage |
| `stages.<stage> = false` | Skip the native stage |
| `stages.<stage> = function(event, options)` | Replace the native stage |
| `after.<stage> = function(event, options)` | Modify the completed stage |
| `terrain = "alpha"` | Select the native terrain style before world construction |
| `spawn = function(options)` | Return minimum and optional maximum spawn height |

Stages are terrain, surface, carver and features. Flatlands replaces terrain directly and skips the other native stages. Highlands and Caves modify the surface stage. Infdev profiles declare each replacement separately.

The create-world selection and active world's selection have separate state. Opening a world never changes the create-world selection. Options belong to individual dimensions; Profile Options displays only fields for currently selected profiles. Values must be finite, in range and integral when declared as integers.

Saved options use world_profiles:schema=3, world_profiles:dim:<id>, and world_profiles:option:<id>:<key>. Native styles are written explicitly as terrain_style:<id>. Unknown profiles, unsupported dimensions, absent required values, unexpected keys and inconsistent native styles stop loading before generation. Back up existing worlds before changing versions.

Custom spawn policies search up to 256 columns in a deterministic spiral and require two empty blocks above the surface. Exhaustion is an error. Flatlands selects its grass surface directly instead of entering the native sand search.

Generation and spawn hooks execute with their own world and chunk context. Nested generation restores the enclosing context. Offline chunk events preserve the full signed 64-bit seed. Offline sampling uses the current create-world selection for Lua stages; its native base style is selected explicitly through minecraft.world.sample's terrain_style option.

Hook failures during creation, opening, generation or spawn selection abort the operation. Cancellation of a generation stage remains set once claimed; after callbacks receive the actual cancellation and native-stage result. No failed Lua generator silently resumes native generation.

Flatlands defaults to the verified Java 1.1 layer stack: bedrock at Y=0, dirt at Y=1..2 and grass at Y=3. Grass Height controls the grass block coordinate; the spawn stands one block above it. Villages are not part of this profile.

Historical verification and the local 122-label reference corpus are described in [HISTORICAL_AUDIT.md](HISTORICAL_AUDIT.md). Alpha now uses its original density scales, full 3D noise-grid path and surface rules instead of Beta arithmetic with the climate factor removed. Its population and carver stages are outside the verified boundary. February Infdev is explicitly a seeded adaptation because the archived original initializes independent unseeded noise streams and uses Math.random for flowers. April Infdev terrain/surface generation is compared separately from its adapted population.

Modern Inspired remains a custom 128-block generator. It is not a port of the 384-block overworld generation in the downloaded modern releases. Highlands and Caves are custom profiles. No full-game or rendered-world historical parity is claimed.
