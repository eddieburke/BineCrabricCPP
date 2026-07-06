# Runtime mods (Lua only)

Zip-package sources for `%APPDATA%/.minecraft/mods/`. **No native code here.**
Shader files are rejected by the packager. Runtime mods use managed fixed-function
draw services; they do not receive shader/program or raw GL state APIs.

```text
<mod_id>/
  mod.json
  scripts/main.lua
  assets/        optional (mod-owned files read via read_asset)
  resources/     optional (vanilla resource overlay: textures, lang, …)
  lang/          optional
```

Package: `native/package-runtime-mods.ps1`

## Engine contract

Mods talk to the game through **generic** hooks and the `minecraft.*` API in `native/src/net/minecraft/mod/`. The engine does not contain per-mod gameplay logic or mod-specific bootstrap paths.

## Events

`minecraft.on` supports the original callback form and a declarative filtered form:

```lua
minecraft.on(minecraft.events.client_tick, callback, 100)

minecraft.on(minecraft.events.chunk_generation, {
  stage = minecraft.generation.stages.features,
  moment = minecraft.generation.moments.after,
  when = minecraft.util.real_world,
  priority = 100,
}, callback)
```

Every option except `priority`, `once`, and `when` filters the field with the same name on the event. World events expose `mod_generation` (false for engine probe/synthetic worlds). `minecraft.util.real_world(event)` is shorthand for `event.mod_generation ~= false`.

Names and phases come from `minecraft.events`, `minecraft.generation`, `minecraft.render`, and `minecraft.lifecycle`. Shared helpers live in `minecraft.util`; simple persisted key/value settings use `minecraft.config.load` and `minecraft.config.save`. Raw mod-scoped persistence is available as `minecraft.storage.read` and `minecraft.storage.write`; paths resolve below `.minecraft/config/mods/<mod_id>/`. Modules inside the mod can be loaded with `minecraft.require`.

World creation has a generic per-save handoff. A `create_world` callback may add
string values to `event.options`; use namespaced keys such as
`my_mod:generator_type`. The options are saved in `level.dat`. `world_open`
fires before the `World` is constructed and exposes the restored
`event.options` snapshot, `save_name`, and `new_world`, so generation mods can restore
their active state before spawn selection and the first chunk.

Lua states do not expose `io`, `debug`, process execution, native-module loading, or
the host's default module search path. A package can read its own files through
`read_asset`/`read_asset_bytes`, load its own Lua modules with `minecraft.require`,
and write only through its scoped storage service.

World drawing uses one staged event rather than renderer-specific hook types. Filter
`minecraft.events.world_render` with `minecraft.render.stages` (`sky`, `stars`,
`clouds`) and `minecraft.render.moments` (`before`, `after`). A before callback can
set `event.cancel_vanilla`; `event.vanilla_stage_ran` reports the result to after
callbacks. The sky-before stage also exposes `celestial_angle` and `sky_yaw_deg`.
Managed drawing is valid only inside that callback: `render.quads` submits a
`vertices = {{x,y,z,u,v,...}, ...}` batch with optional `texture`, color, blend,
cull, depth-test, and depth-write fields; `render.billboards` submits a spherical
billboard batch. GL objects and mutable raw GL state are never exposed to Lua.

Sky and fog tinting share the composable `world_color` event; filter its `kind`
with `minecraft.colors.sky` or `.fog` and edit `r`, `g`, and `b`. Every callback
receives the result of earlier callbacks, so color mods layer instead of claiming
an exclusive override flag.

Block declarations may use a packaged resource path in `texture` or an existing
vanilla terrain-atlas tile in `texture_id`; registration rejects missing texture
declarations and duplicate/reserved block IDs instead of asserting later in bootstrap.

## GUI composition

The screen helpers are Lua compositions over the same filtered event API:

| Helper | Purpose |
|--------|---------|
| `minecraft.screen.on_ui(screen_id, region, fn, priority?)` | Add buttons/widgets to a vanilla screen region (`event.ui`) |
| `minecraft.screen.on_lua_screen(screen_id, handlers, priority?)` | Own a custom Lua screen (`init`, `render`, `tick`, `key`, `mouse`, `scroll`, `close`) |
| `minecraft.screen.settings(spec)` | Build a shared two-column slider/toggle settings screen from data |

Constants: `minecraft.screen.ids.*` (`create_world`, `inventory`, `detail_settings`) and `minecraft.screen.regions.*` (`footer`, `screen`, `side_panel`).

The underlying `screen_ui`, phased `screen_event`, and reusable `screen_region` events remain available through `minecraft.on` for direct composition. A region event provides `screen_id`, `region`, `phase_name`, bounds, mouse fields, and `handled`; inventory's right-side space is simply the `side_panel` region rather than a dedicated hook type.

## General data services

Composable primitives every mod can use:

- `minecraft.util.resolve_seed(text)` — numeric or Java-string-hash seed
- `minecraft.util.json_encode(table)` / `json_decode(text)` — JSON for configs and storage
- `minecraft.registry.name(domain, id)`, `.list(domain)`
- `minecraft.world.sample_grid(seed, x, z, opts)` — channels: `grass`, `biome_id`, `height`,
  `surface_block`, and `surface_block_below`. Pass `channels = {...}` to sample several
  channels in one terrain-generation pass; results are available in `grid.channels`.
  Seeds remain exact signed 64-bit Lua integers. Set `mod_generation = true` to
  run generic generation-stage callbacks for an offline probe.
- `minecraft.world.marker_px(grid, x, z)` — map pixel from world coords
- `minecraft.screen.host_field(name)`, `.host_set_field(name, val)`, `.open_host(id, fields)` on vanilla screens
- `minecraft.screen.active_id()` — removed; use `screen_ui` event `host_fields` table instead
- `minecraft.files.pick({extension = "json"})`, `.read(path)` — native file picker and text read
- `minecraft.render.create_texture(w, h, values)`, `.release_texture(id)`
- `minecraft.render.quads`, `billboards`
- `minecraft.items.ids`
Seed search, rules, and preview are implemented in `native/mods/seedfinder/` (Lua on generic APIs like `sample_grid`).

## Seedfinder ownership

There is no separate native seedfinder engine or CLI. Only the Lua zip under `native/mods/seedfinder/`.
