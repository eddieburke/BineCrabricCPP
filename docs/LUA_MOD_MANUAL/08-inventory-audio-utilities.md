# 08 — Inventory, audio, utilities

## `minecraft.inventory.*`

All inventory functions operate on the **local player's inventory** (single-player client included). Functions that mutate inventory require a valid local player; if the player is unavailable (e.g. not in-world), mutation returns `false` and reads return `nil`.

### Slot layout

The inventory has 40 slots total:
- **Slots 0–35**: Main inventory (36 slots)
- **Slots 36–39**: Armor slots (4 slots: boots, leggings, chestplate, helmet)

Accessing a slot outside this range returns `nil` / `false`.

### `minecraft.inventory.slot_count()`

Returns the total number of inventory slots: `40` (36 main + 4 armor).

```lua
local count = minecraft.inventory.slot_count() -- 40
```

### `minecraft.inventory.main_size()`

Returns the size of the main inventory: `36`.

```lua
local main = minecraft.inventory.main_size() -- 36
```

### `minecraft.inventory.get(slot)`

Returns a table describing the item stack at `slot`. An empty slot is returned
as `{id=0, count=0, damage=0}`; an invalid/out-of-range slot returns `nil`.

The returned table:

| Field | Type | Description |
|-------|------|-------------|
| `id` | number | Numeric item ID |
| `count` | number | Stack size |
| `damage` | number | Item damage/durability value |
| `max_damage` | number | Maximum durability (only present if non-empty) |
| `damageable` | boolean | Whether the item can take damage (only present if non-empty) |
| `stackable` | boolean | Whether the item stacks (only present if non-empty) |
| `has_subtypes` | boolean | Whether the item has subtypes (only present if non-empty) |
| `max_count` | number | Maximum stack size (only present if non-empty) |

```lua
local stack = minecraft.inventory.get(0)
if stack and stack.id ~= 0 then
  print(stack.id, stack.count, stack.damage)
end
```

### `minecraft.inventory.set(slot, {id, count?, damage?})`

Sets the item stack at `slot`. Returns `true` on success, `false` if the slot is invalid or the player is unavailable.

The item spec table accepts `id` (required), optional `count` (default 1), and optional `damage` (default 0).

```lua
local ok = minecraft.inventory.set(0, {id=264, count=1})     -- place diamond in first slot
local ok = minecraft.inventory.set(36, {id=310, count=1, damage=0}) -- diamond helmet in armor slot
```

### `minecraft.inventory.cursor_get()`

Returns the item stack currently held on the cursor (being dragged from a container slot). An empty cursor is returned as an empty stack table with `id=0`, `count=0`, and `damage=0`; unavailable inventory returns `nil`.

```lua
local cursor = minecraft.inventory.cursor_get()
if cursor then
  print("Dragging:", cursor.id)
end
```

### `minecraft.inventory.cursor_set({id, count?, damage?})`

Sets the cursor stack. Returns `true` on success, `false` if the player is unavailable.

```lua
minecraft.inventory.cursor_set({id=264, count=1})
```

### `minecraft.inventory.give({id, count?, damage?})`

Gives an item stack to the player. The item is placed into any available slot
(fitting into existing stacks first, then empty slots); any overflow is dropped
into the world. For an available player this usually returns `true`, not a
signal that every item fit. It returns `false` if the player is unavailable.

```lua
local ok = minecraft.inventory.give({id=264, count=5}) -- give 5 diamonds
```

### `minecraft.inventory.offer({id, count?, damage?})`

Offers an item stack to the player. Returns the **remainder** stack (items that could not be placed) as a table in the same format as `get()`. If the entire stack was accepted, the remainder is empty (count will be 0, but the table is non-nil). If the player is unavailable, the original input stack is returned as the remainder.

```lua
local remainder = minecraft.inventory.offer({id=264, count=100})
if remainder.count > 0 then
  print("Could not fit", remainder.count, "diamonds")
end
```

---

## `minecraft.items.*`

### `minecraft.items.ids()`

Returns an array of all registered item numeric IDs. Items that are `nil` in the engine's ITEMS array are skipped.

```lua
local ids = minecraft.items.ids()
for _, id in ipairs(ids) do
  print("Item ID:", id)
end
```

### `minecraft.items.describe(item_id)`

Returns metadata about an item ID, or `nil` if the ID is invalid or the item is empty.

| Field | Type | Description |
|-------|------|-------------|
| `id` | number | The item ID |
| `max_damage` | number | Maximum durability |
| `damageable` | boolean | Whether the item can take damage |
| `stackable` | boolean | Whether the item can stack |
| `has_subtypes` | boolean | Whether the item uses damage for subtypes |
| `max_count` | number | Maximum items per stack |

```lua
local info = minecraft.items.describe(264)
if info then
  print("Diamond - max stack:", info.max_count, "damageable:", info.damageable)
end
```

---

## `minecraft.sound.*`

### `minecraft.sound.register(id, filepath, kind?)`

Registers a new sound effect with the audio engine. `id` is a string identifier, `filepath` is a resource path (resolved relative to the mod's asset directory). The optional `kind` determines how the sound is loaded. On failure it returns `false, error`.

| Kind | Description |
|------|-------------|
| `"effect"` | Short sound effect, loaded entirely into memory (default) |
| `"streaming"` | Longer sound, streamed from disk |
| `"music"` | Background music track |

Returns `false, error_message` on failure (missing file, unknown kind), or `true` on success.

```lua
local ok, err = minecraft.sound.register("my_mod:explode", "sounds/explode.ogg", "effect")
if not ok then print(err) end
```

### `minecraft.sound.play(id, volume?, pitch?)`

Plays a registered sound as a 2D (non-positional) effect. Default volume is 1.0, default pitch is 1.0. Returns `true` if the sound was started, `false` if the audio engine is unavailable.

```lua
minecraft.sound.play("my_mod:explode", 1.0, 1.0)
```

### `minecraft.sound.play_at(id, x, y, z, volume?, pitch?)`

Plays a registered sound at a 3D world position. Positional audio applies (distance attenuation, stereo panning). Default volume 1.0, default pitch 1.0. Returns `true` on success.

```lua
minecraft.sound.play_at("my_mod:explode", player_x, player_y, player_z, 1.0, 1.0)
```

### `minecraft.sound.play_loop_at(id, x, y, z, volume?, pitch?)`

Plays a registered sound as a looping 3D positional sound. Returns a **handle string** that can be passed to `stop()` to end the loop, or `nil` if the sound could not be started.

```lua
local handle = minecraft.sound.play_loop_at("my_mod:machine_hum", 100, 64, 100, 0.5, 1.0)
if handle then
  -- save handle for later
end
```

### `minecraft.sound.stop(handle)`

Stops a looping sound by its handle (returned from `play_loop_at`). The handle must be a non-empty string. Returns `true`.

```lua
if handle then
  minecraft.sound.stop(handle)
end
```

---

## JSON utilities (`minecraft.util.*`)

### `minecraft.util.json_encode(value)`

Encodes a Lua table to a JSON string. It accepts array-like tables, string-keyed
tables, and the `minecraft.util.json_null` sentinel inside tables. The top-level
argument must be a table; scalar values and top-level `nil` are rejected.

Returns `(json_string)` on success, or `(nil, error_message)` if the value is not JSON-serializable.

```lua
local json = minecraft.util.json_encode({name="Steve", health=20, items={1, 2, 3}})
-- {"name":"Steve","health":20,"items":[1,2,3]}
```

### `minecraft.util.json_decode(string)`

Decodes a JSON string to a Lua value. Supports:
- Objects → string-keyed tables
- Arrays → integer-indexed tables
- Strings, numbers (integers use `pushinteger`, floats use `pushnumber`)
- Booleans
- `null` → `minecraft.util.json_null` lightuserdata sentinel

Returns `(value)` on success, or `(nil, error_message)` on failure (empty input, invalid JSON, trailing characters).

```lua
local data, err = minecraft.util.json_decode('{"name":"Steve","health":20}')
if data then
  print(data.name, data.health)
end
```

### `minecraft.util.json_null`

A special sentinel value representing JSON `null`. Use this when you need to explicitly represent null in JSON output.

```lua
local encoded = minecraft.util.json_encode({value = minecraft.util.json_null})
-- {"value":null}

local decoded = minecraft.util.json_decode('{"value":null}')
-- decoded.value == minecraft.util.json_null
```

### `minecraft.util.resolve_seed(text)`

Resolves a seed string (numeric or textual) into a 64-bit integer, matching Minecraft's seed resolution logic. Returns the resolved integer.

```lua
local seed = minecraft.util.resolve_seed("12345")   -- 12345
local seed = minecraft.util.resolve_seed("hello")    -- numeric hash
```
