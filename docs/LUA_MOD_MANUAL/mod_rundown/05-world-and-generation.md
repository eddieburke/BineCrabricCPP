# World profiles and generation

`world_profiles/scripts/main.lua` loads, validates, sorts, and selects profile
tables. A profile has an `id`, label/order, optional option schema, optional
`spawn(options)`, a `cancel` stage map, `generate(event, options)`, and
`decorate(event, options)`. Selection is written at `create_world`, restored at
`world_open`, displayed in the Create World footer, and applied to spawn and
generation hooks. Trace: `mods/world_profiles/scripts/main.lua:15-210`.

## Controller flow

1. Load profiles from `scripts/worldtypes`, reject duplicate/invalid ids, and
   normalize option storage keys.
2. On Create World, write the chosen id and numeric options to `event.options`.
3. On World Open, restore that id/options and make it current for new worlds.
4. On spawn search, use the profile result; for an accepted min/max range probe
   as many as 49 top-Y positions before resolving a coordinate.
5. Before terrain/surface/carver/features, cancel only named profile stages and
   call `generate`; after surface call `decorate` when supplied.

`config/init.lua` declares `enabled`, but the controller loads and never reads
that proxy. `common.lua` owns AIR id lookup, safe block-id resolution, and the
deterministic coordinate hash.

## Profile reference

| File | Options and flow |
| --- | --- |
| `worldtypes/default.lua` | Metadata only; leaves vanilla generation and spawn unchanged. |
| `worldtypes/flatlands.lua` | `flat_height` 1–32. Cancels carver/features, then post-surface fills a dirt slab, grass cap, optional bedrock, and clears above it. |
| `worldtypes/highlands.lua` | `highland_boost` 4–40. Retains vanilla generation, requires spawn top-Y ≥81, then adds deterministic stone uplift plus dirt/grass cap. |
| `worldtypes/caves.lua` | `cave_min_y` and `cave_max_y`. Retains vanilla stages, trims overburden and deterministically carves small clipped air pockets after surface. |
| `worldtypes/infdev_20100227.lua` | Cancels all stages. Java-style RNG/Perlin/octaves generate terrain only; no custom surface, carver, or features run. State is cached per seed without eviction. |
| `worldtypes/infdev_20100415.lua` | Cancels all stages. Builds a 5×5×33 density grid, then surface layers, ore veins, and trees. Its chunk-height cache evicts FIFO after 512 chunks; carver is canceled without a replacement. |

## Infdev traces

The 20100227 profile builds a 48-bit Java LCG, Perlin, and octave state, computes
column land height around 64, then writes strata, water, rare dandelions, an
obsidian axis, and occasional brick-pyramid replacement. Key locations are
`infdev_20100227.lua:1-268`.

The 20100415 profile interpolates density to voxels; it scans exposed stone for
grass/dirt/sand/gravel/water surface layers. Feature sources deliberately span a
NW 2×2 chunk set so edge writes are intentional. It places coal, iron, gold,
diamond, and forest-noise-controlled trees subject to target-chunk/Y limits.
See `infdev_20100415.lua:189-564`.

## Other world modifiers

- `ravine_backport/scripts/main.lua` runs after a real-world carver stage; a
  seed/chunk hash gives a 1/35 chance of carving a 13-column sine path.
- `coral/scripts/main.lua` runs after features; with a 1/16 chance it checks
  deep water and a sand/mud base before placing a 30–59-block coral cluster.
- Seed Finder calls `minecraft.world.sample` through a cooperative UI search;
  it samples around `(0,0)`, not a located world spawn. See its dedicated
  coverage in the mod catalogue.
