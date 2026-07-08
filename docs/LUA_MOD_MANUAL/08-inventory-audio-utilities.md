# Volume VIII — Inventory, Audio, and Utilities

Stacks, player inventory, items, sound, config, JSON, seeds, and files.

---

## Stack tables

Stacks are plain Lua tables:

```lua
{ id = 256, item_id = 256, count = 1, damage = 0 }
```

`id` and `item_id` are interchangeable. Empty = `id == 0` or `count <= 0`.

### `minecraft.stack`

| Function | Description |
|----------|-------------|
| `empty()` | Zero stack |
| `is_empty(s)` | Test |
| `item_id(s)` | Numeric id |
| `copy(s)` | Clone |
| `describe(s)` | Delegates to `items.describe` |
| `mergeable(a,b)` | Same item, compatible damage, stackable |
| `max_count(s)` | From item metadata; default 64 |
| `click(slot, cursor, button)` | Vanilla click rules; button 0=left 1=right |
| `combine_damage(a,b)` | Anvil-style merge; returns two stacks |

### `combine_damage` rules

- Both stacks: same damageable item, count 1 each
- Sums remaining durability, caps at max damage
- Returns repaired stack + empty stack

---

## `minecraft.inventory`

Requires local player.

| Function | Returns | Description |
|----------|---------|-------------|
| `slot_count()` | int | Main + armor slots |
| `main_size()` | int | Main inventory only |
| `get(slot)` | stack \| nil | |
| `set(slot, stack)` | bool | |
| `cursor_get()` | stack | |
| `cursor_set(stack)` | bool | |
| `give(stack)` | bool | Add to player inventory |
| `offer(stack)` | stack | Add or return remainder |

**Slot layout:** `0 .. main_size-1` = main inventory; armor follows main slots.

**`get`/`cursor_get`** return enriched stacks when non-empty:

| Field | Description |
|-------|-------------|
| `max_damage` | |
| `damageable` | bool |
| `stackable` | bool |
| `has_subtypes` | bool |
| `max_count` | int |

---

## `minecraft.items`

| Function | Returns |
|----------|---------|
| `ids()` | array of all registered item ids |
| `describe(item_id)` | metadata table or nil |

### `describe` table

| Field | Type |
|-------|------|
| `id` | int |
| `max_damage` | int |
| `damageable` | bool |
| `stackable` | bool |
| `has_subtypes` | bool |
| `max_count` | int |

---

## `minecraft.sound`

Paths are **package-relative** resources (`resources/mods/<mod_id>/...` or resolve via host).

### Registration

```lua
local ok, err = minecraft.sound.register(id, path, kind)
```

| kind | Aliases | Engine slot |
|------|---------|-------------|
| `effect` | `effects`, `one_shot`, `oneshot` | one-shot SFX |
| `streaming` | `stream`, `record` | positional streaming |
| `music` | `bgm` | background music |

**Errors:** missing file, unknown kind.

### Playback

| Function | Returns |
|----------|---------|
| `play(id, volume?, pitch?)` | bool |
| `play_at(id, x,y,z, vol?, pitch?)` | bool |
| `play_loop_at(id, x,y,z, vol?, pitch?)` | handle string \| nil |
| `stop(handle)` | bool |

Default volume/pitch = 1.0. Loop handles are opaque strings; `stop` ends that instance.

---

## `minecraft.storage` and `minecraft.config`

### Raw storage

```lua
local text = minecraft.storage.read("settings.json")
minecraft.storage.write("settings.json", content)  -- bool
```

### Config helper

```lua
local defaults = { enabled = true, count = 5, name = "foo" }
local values, found = minecraft.config.load("mod.cfg", defaults, {
  keys = { "enabled", "count", "name" },
  aliases = { fileName = "internalKey" },
  names = { internalKey = "fileName" },
  separator = "=",  -- default "="
})
minecraft.config.save("mod.cfg", values, { keys = {...}, names = {...} })
```

- Lines: `key=value` or `key: value`
- Comments: lines starting with `#` or `;` skipped
- Types inferred from `defaults` table types

**Legacy cfg paths** (`mod_Foo.cfg` in run directory) used by `layered_clouds`, `realtime_sky`.

---

## `minecraft.util`

| Function | Description |
|----------|-------------|
| `clamp(v, lo, hi)` | |
| `trim(s)` | Whitespace trim |
| `in_rect(x,y,l,t,w,h)` | Hit test |
| `real_world(event)` | `event.mod_generation ~= false` |
| `resolve_seed(text)` | Numeric or Java `String.hashCode` → int64 |
| `json_encode(value)` | JSON string; errors via `error()` |
| `json_decode(text)` | table, err |
| `copy(table)` | Shallow copy |
| `parse_boolean(s, fallback)` | Parse bool strings |

### Seed resolution

```lua
local seed = minecraft.util.resolve_seed("hello")  -- Java-style hash
local seed = minecraft.util.resolve_seed("12345")  -- numeric
```

Backed by `minecraft._resolve_seed` (native).

---

## `minecraft.time`

```lua
local utc_ms = minecraft.time.utc_millis()
```

Wall-clock UTC epoch milliseconds. Used with astronomy APIs.

---

## `minecraft.options` (client)

```lua
local fancy = minecraft.options.get("fancy_graphics")  -- snake or camelCase
local keys = minecraft.options.keys()  -- all persist keys
```

Reads from `OptionRegistry` + extras:

| Key | Type |
|-----|------|
| `skin` | string |
| `lastServer` | string |
| `fancyGraphics` | bool |
| `thirdPerson` | bool |
| `hideHud` | bool |
| `renderClouds` / `clouds_enabled` | bool |
| `key_*` / `key.*` | int scancode |

Returns `nil` if unknown or no client.

---

## `minecraft.files`

Native file picker (outside mod sandbox):

```lua
local path = minecraft.files.pick({ extension = "json" })
local path = minecraft.files.pick("json")  -- shorthand
local text, err = minecraft.files.read(path)
```

`pick` returns `nil` if cancelled. `read` returns `nil, err` on failure.

---

## `minecraft.astronomy`

```lua
local azimuth_deg, altitude_deg = minecraft.astronomy.horizontal_from_equatorial(
  ra_hours, dec_degrees, utc_millis, latitude_deg, longitude_deg)
```

See [Volume VI](06-rendering.md).

---

## Keyboard

### `minecraft.is_key_down(scancode)`

Poll current key state.

### `minecraft.key_code(name_or_number)`

Returns LWJGL scancode. If client active, resolves **bound keys** for:

`forward`, `left`, `back`, `right`, `jump`, `sneak`, `drop`, `inventory`, `chat`, `fog`

### Full default name table

| Name | Code | Name | Code |
|------|------|------|------|
| escape | 1 | q | 16 |
| 1..0 | 2..11 | w | 17 |
| enter | 28 | e | 18 |
| a | 30 | r | 19 |
| s | 31 | t | 20 |
| d | 32 | y | 21 |
| f | 33 | u | 22 |
| g | 34 | i | 23 |
| h | 35 | o | 24 |
| j | 36 | p | 25 |
| k | 37 | z | 44 |
| l | 38 | x | 45 |
| space | 57 | c | 46 |
| up | 200 | v | 47 |
| left_arrow | 203 | b | 48 |
| right_arrow | 205 | n | 49 |
| down | 208 | m | 50 |

### Prelude constants

```lua
minecraft.keys.escape  -- 1
minecraft.keys.enter   -- 28
minecraft.keys.space   -- 57
minecraft.keys.up      -- 200
minecraft.keys.down    -- 208
```

---

## Assets

| Function | Returns |
|----------|---------|
| `asset_path(relative)` | absolute path in package |
| `read_asset(relative)` | text \| nil |
| `read_asset_bytes(path, {gzip?})` | binary \| nil |
| `read_nbt_asset(path)` | table, err |

NBT assets: auto-detect gzip (.nbt.gz). Max size enforced by engine.

---

## Runtime crafting

```lua
local ok, err = minecraft.crafting.add_shaped_recipe({
  output_item_id = 30777,
  output_count = 1,
  pattern = { "###", "# #", "###" },
  key = "#",
  item_id = 5,
})
```

Does not use `assert`; safe for player-triggered registration.

---

*Registration errors: [Volume IV](04-registration.md)*
