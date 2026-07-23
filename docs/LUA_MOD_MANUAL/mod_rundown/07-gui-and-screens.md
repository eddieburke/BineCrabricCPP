# Screens, inventory, audio, and utilities

## Seed Finder

Seed Finder injects **Seed tools…** into Create World's footer. Its entry defines
global `S` and helper globals used implicitly by `search`, `rules`, and `ui_spec`;
load those modules through `scripts/main.lua`, not independently.

The finder screen owns inclusive seed bounds, radius, retained hits, minimum
score, spawn-biome picker, imported rule state, selection, and map preview.
Every screen tick advances a bounded cooperative search; terrain rules lower the
budget to one seed. Results are de-duplicated, score-sorted, and trimmed to the
requested top-K. Choosing a hit returns to Create World with its seed.

`rules.lua` defines block, block-pair, biome, and objective schemas. It encodes
hard rules separately from weighted objectives, analyzes biome grid coverage and
connected components, then returns score metadata from `score_seed`. `ui_spec`
opens a transactional card editor: Cancel restores a deep backup; Done keeps the
mutated rules. Source trace: `mods/seedfinder/scripts/main.lua:36-593`,
`search.lua:83-296`, `rules.lua:153-608`, and `ui_spec.lua:113-423`.

The search samples world data around `(0,0)` and reports that position as spawn;
it does not perform a true spawn search. Its `enabled` config is declared but
not read. Its JSON builder has limited escaping, so treat imported data as a
controlled format.

## Repair Table

The entry registers block 150 and a wood recipe; right-click calls
`repair_screen.open`. `inventory_helper.lua` supplies reusable stack functions
and a slot-screen lifecycle: it builds player/custom slot state, draws it,
routes mouse/key/tick events, and returns items/cursor to inventory on close.

The repair screen owns three slots. On tick it calls `combine_damage` for the two
inputs, writes the output, and clears both. Incompatible, undamaged, non-single,
or otherwise unrepairable inputs return the original stacks; the screen still
clears both, so it can delete the right input. The declared `enabled` and
`max_repair_cost` settings are not used and repair is free.

## Too Many Items and Offline Mode

Too Many Items toggles an inventory side panel with `O`, builds a capped 9×8
grid, scrolls it, and grants maximum stack on left-click or one item on
right-click. Its declared enabled/page-size fields are ignored; the hard cap is
72 slots.

Offline Mode has independent JSON state in `offline_mode.cfg`, generates a
deterministic adjective-noun-four-digit name, applies a session offline username
after a ready client tick, and injects a login-screen settings button. Its
`lib.settings` `enabled` field is unused; state may load too late for title UI.

## Audio and UI libraries

`lib/audio/init.lua` manages a 32-source LÖVE pool, category/master volume,
static/stream caching, sound cloning, attenuation/pan, current music, playlists,
and a manual `update`. Its callers must drive update. The 3D effect is only
attenuation/pan; a zero maximum distance divides by zero. Stats hard-code loaded
counts, fades are TODO, and ordinary looping music prevents playlist advance.

`lib/ui/init.lua` exports `Element`, `Button`, `Label`, `Slider`, and `Panel`.
They are LÖVE-rendered retained widgets with no host dispatcher. Panel input
captures its entire bounds before children, so a contained control can miss a
complete mouse sequence. `lib/screen_ui.lua` supplies one small bridge that adds
a stacked footer button to Mod Settings.

`lib/math_util.lua` contains clamp, interpolation, normalization, and
deterministic coordinate/day/string/float hash helpers. `lib/config.lua` is a
thin `minecraft.config` forwarding wrapper; its `mod_name` argument is unused.
