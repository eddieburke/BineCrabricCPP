# Registration and settings

## Content registration

The prelude exposes `minecraft.register_block`, `minecraft.register_item`, and
`minecraft.register_shaped_recipe`. Small content mods call these during module
load. For example, Stone Bricks registers block 98 and a 2×2 stone recipe in
`mods/stone_bricks/scripts/main.lua:6-25`; Iron Bars registers block 101 and an
iron-ingot recipe at `iron_bars/scripts/main.lua:6-36`.

## `lib.settings`

```lua
local config = require("lib.settings").define("mod_id", {
  name = "Visible name",
  native_ui = true,
  fields = {
    enabled = { type = "toggle", default = true },
    density = { type = "slider", min = 0, max = 1, default = 0.5 },
  },
})
```

The returned proxy reads and writes `minecraft.settings` when `native_ui` is
enabled, otherwise it holds local values. Validation clamps numeric values and
normalizes booleans. See `mods/lib/settings/init.lua:16-140`.

Important behavior:

- Definitions are cached: the first schema for a mod id wins.
- Only `bool`/`toggle` become a native toggle; other field types are registered
  as sliders.
- A declared field is not automatically a runtime feature. Many small mods
  declare `enabled` but never read it.
- Values are persisted immediately when a declared setting is assigned; no
  separate save call is needed.

## Registration versus use

The configuration files for Void Fog, Too Many Items, Stone Bricks, Critical
Hit, Iron Bars, Ravine Backport, Simple Lantern, Coral, Offline Mode, Northern
Stars, and Repair Table declare settings but their main scripts do not consume
the returned proxy. Treat the fields as available UI schema, not as active
feature gates.

## Persisted mod data

Use `minecraft.config.load/save` for host-managed config data. Realtime Sky
stores `realtime_sky.txt` and preserves its settings-table identity while loading
camelCase aliases. Offline Mode instead saves an `offline_mode.cfg` state record.
World Profiles saves selection/options into Create World and World Open option
maps rather than into its module config.
