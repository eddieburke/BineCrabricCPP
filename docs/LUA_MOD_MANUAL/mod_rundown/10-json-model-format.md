# Data formats and known limitations

## Formats in use

| Area | Format and owner |
| --- | --- |
| Realtime Sky | `realtime_sky.txt` through `minecraft.config`; camelCase keys are accepted as load aliases. |
| Offline Mode | `offline_mode.cfg` JSON record containing enable state and username. |
| World Profiles | Create World/World Open option map: selected profile id and numeric option strings. |
| Seed Finder | Imported JSON rule/objective arrays; the encoder is hand-built and only safely targets controlled data. |
| Northern Stars | NBT catalogue asset, validated before cached billboard build. |
| Item Drop Physics | A file named `.json`, but loaded with `minetest.deserialize`; it is not a normal JSON contract. |
| Realtime coastlines/cities | Coastline geometry and `cities.json`, with bundled place fallback. |

## Verified runtime limitations

### Event/API mismatches

- The engine defines the `attack_damage` hook but does not publish it in
  `minecraft.events`; Critical Hit cannot register its callback through the
  public table.
- `minecraft.events.tick` and `minecraft.events.item_spawn` do not exist.
  Item Drop Physics uses both and fails before useful simulation begins.

### Simulation defects

- Box3D collision calls missing `core.vec3_lerp`, compares plain tables with
  `<` when forming a pair key, and leaks query-bound vectors.
- The Item Drop Physics world calls missing `water.sample_water` and reads
  undefined speed/sleep constants. Its water module uses Minetest names and
  requires a block callback its own callers omit. No item entity render/writeback
  is wired from its simulation records.
- The shared particle update reads undefined `options.initial_alpha`; its stats
  count is unreachable.

### Configuration disconnects

Many package `enabled` values are declarations only. This applies to Void Fog,
Too Many Items, Stone Bricks, Critical Hit, Iron Bars, Ravine Backport, Simple
Lantern, Coral, Offline Mode, Northern Stars, Repair Table, Seed Finder, and
World Profiles. Camera also reads several fields it never declares. In contrast,
Sprint, Colorful Skies, Fog Settings, Layered Clouds, and the Meteor local config
are read by their main flows.

### Behavioural caveats

- Repair Table consumes both input slots even when its combine helper returns
  unchanged items for an invalid repair.
- Seed Finder labels samples at `(0,0)` as spawn; it does not locate generated
  spawn and depends on globals from its main file.
- Realtime Sky's named-zone and DST handling is deliberately curated/simplified;
  unknown zones are estimated from longitude.
- Template `standard_mod` has no host-event bridge, registers undefined
  `on_player_chat`, and lets entity and physics positions diverge.

## Using this manual safely

When writing a new mod, first verify the event/name is in the event chapter,
keep client-only API access out of server load paths, and treat any `Declared`
setting as inert until the relevant entry script reads it. The source locations
in the chapter text are the authority when a behavior is subtle or performance-
sensitive.
