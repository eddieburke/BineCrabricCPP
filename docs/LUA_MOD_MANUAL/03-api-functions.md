# Volume III — API Functions (Complete)

Every function exposed on the `minecraft` global, grouped by namespace. Return types noted.

---

## Top-level

| Function | Args | Returns | Notes |
|----------|------|---------|-------|
| `minecraft.log` | `(level, message)` or `(message)` | — | level: `info`, `warn`, `error` |
| `minecraft.on` | event, options/callback, priority? | bool | See Vol II |
| `minecraft.at_phase` | phase_name, order, function | — | Lifecycle hook |
| `minecraft.register_block` | spec table | — | **assert** on failure |
| `minecraft.register_item` | spec table | ok, err? | Returns `false, message` |
| `minecraft.register_shaped_recipe` | spec | — | **assert**; startup defer |
| `minecraft.require` | module_name | module | Mod-local only; `package` removed after prelude |
| `minecraft.asset_path` | relative | path\|nil | Absolute path in package |
| `minecraft.read_asset` | relative | string\|nil | Text file |
| `minecraft.read_asset_bytes` | path, {gzip?} | string\|nil | Binary; optional gzip decompress |
| `minecraft.read_nbt_asset` | path | table, err | NBT → Lua tables |
| `minecraft.is_key_down` | scancode | bool | Poll keyboard |
| `minecraft.key_code` | name or number | int | See Vol II key table |

---

## `minecraft.crafting`

| Function | Returns | Notes |
|----------|---------|-------|
| `add_shaped_recipe(spec)` | ok, err | Works at startup **and** runtime |

**Shaped recipe spec:**

| Field | Required | Description |
|-------|----------|-------------|
| `output_block_id` | one of | Block output (1..BLOCK_COUNT-1) |
| `output_item_id` | one of | Item output (mutually exclusive with block) |
| `output_count` | no | 1..64, default 1 |
| `pattern` | yes | Array of 1..3 strings, each 1..3 chars wide, equal width |
| `key` | no | Ingredient character, default `#` |
| `item_id` | yes | Ingredient item id |

---

## `minecraft.time`

| Function | Returns |
|----------|---------|
| `utc_millis()` | number | UTC epoch milliseconds |

---

## `minecraft.options` (client)

| Function | Returns |
|----------|---------|
| `get(key)` | bool\|number\|string\|nil | OptionRegistry key, snake_case or camelCase |
| `keys()` | array | All persist keys |

Extra keys: `skin`, `lastServer`, `fancyGraphics`, `thirdPerson`, `hideHud`, `renderClouds`, `key_*` bindings.

---

## `minecraft.storage` / `minecraft.config`

```lua
minecraft.storage.read(path)   -- nil if missing
minecraft.storage.write(path, content)  -- bool
```

Paths: `.minecraft/config/mods/<mod_id>/...`  
Legacy: bare `foo.cfg` / `foo.txt` in run directory root.

```lua
values, found = minecraft.config.load(path, defaults, {
  keys = { "a", "b" },
  aliases = { fileName = "internalKey" },
  names = { internalKey = "fileName" },
  separator = "=",
})
minecraft.config.save(path, values, { keys, names, separator })
```

---

## `minecraft.util`

| Function | Description |
|----------|-------------|
| `clamp(v, lo, hi)` | Clamp number |
| `trim(s)` | Trim whitespace |
| `in_rect(x,y,l,t,w,h)` | Hit test |
| `real_world(event)` | `event.mod_generation ~= false` |
| `resolve_seed(text)` | Numeric or Java hash → int64 |
| `json_encode(value)` | JSON string (errors via `error()`) |
| `json_decode(text)` | table, err |
| `copy(table)` | Shallow copy |
| `parse_boolean(s, fallback)` | Parse bool strings |

---

## `minecraft.astronomy`

```lua
azimuth_deg, altitude_deg = minecraft.astronomy.horizontal_from_equatorial(
  ra_hours, dec_degrees, utc_millis, latitude_deg, longitude_deg)
```

Azimuth: degrees east from north. Altitude: degrees above horizon.

---

## `minecraft.stack`

| Function | Description |
|----------|-------------|
| `empty()` | Zero stack table |
| `is_empty(s)` | Test |
| `item_id(s)` | Numeric id |
| `copy(s)` | Clone |
| `describe(s)` | Via `items.describe` |
| `mergeable(a,b)` | Same item, compatible damage |
| `max_count(s)` | Max stack size |
| `click(slot, cursor, button)` | Vanilla slot click rules; button 0=left 1=right |
| `combine_damage(a,b)` | Repair merge; returns two stacks |

Stack table fields: `id`/`item_id`, `count`, `damage`.

---

## `minecraft.registry`

| Function | Args | Returns |
|----------|------|---------|
| `name(domain, id)` | `"biome"` or `"block"`, int | string |
| `list(domain)` | domain | array of names |

---

## `minecraft.world`

| Function | Args | Returns |
|----------|------|---------|
| `block_id(name)` | wire name string | int |
| `get_block(x,y,z)` | world coords | block id |
| `get_top_y(x,z)` | | int or -1 |
| `random(bound?)` | default 1000 | 0..bound-1 |
| `is_night()` | | bool |
| `player()` | | `{x,y,z}` or nil |
| `spawn_entity(type, pos)` | type string; pos table or x,y,z | bool |
| `count_entities(type)` | | int |
| `set_cursor(item_id, count)` | | bool |
| `set_time(ticks)` | 0..23999 | bool |
| `sample_grid(seed, x, z, opts)` | | grid table — see Vol V |
| `marker_px(grid, wx, wz)` | | col, row |

---

## `minecraft.chunk` (generation context only)

| Function | Returns |
|----------|---------|
| `set_block(lx,y,lz,id)` | bool |
| `get_block(lx,y,lz)` | int |
| `get_height(lx,lz)` | int |
| `fill(x1,y1,z1,x2,y2,z2,id)` | int changed count |

Local X/Z: 0..15. Y: 0..127.

---

## `minecraft.particles`

```lua
minecraft.particles.spawn({
  x, y, z, vx, vy, vz,
  scale = 4.0,      -- default 4
  r, g, b = 1,      -- default 1
  max_age = 40,
  gravity = 0.04,
})  -- returns bool
```

---

## `minecraft.items`

| Function | Returns |
|----------|---------|
| `ids()` | array of all item ids |
| `describe(item_id)` | metadata table or nil |

---

## `minecraft.inventory`

| Function | Description |
|----------|-------------|
| `slot_count()` | Main + armor slots |
| `main_size()` | Main inventory size |
| `get(slot)` / `set(slot, stack)` | Slot access |
| `cursor_get()` / `cursor_set(stack)` | Cursor |
| `give(stack)` | Add to player |
| `offer(stack)` | Add or return remainder stack |

---

## `minecraft.sound`

| Function | Returns |
|----------|---------|
| `register(id, path, kind?)` | ok, err? |
| `play(id, vol?, pitch?)` | bool |
| `play_at(id, x,y,z, vol?, pitch?)` | bool |
| `play_loop_at(id, x,y,z, vol?, pitch?)` | handle string or nil |
| `stop(handle)` | bool |

Kinds: `effect`, `streaming`/`stream`/`record`, `music`/`bgm`.

---

## `minecraft.render`

| Function | Context | Returns |
|----------|---------|---------|
| `quads(spec)` | `world_render` only | quad count |
| `billboards(spec)` | `world_render` only | count |
| `create_texture(w,h,values)` or `({width,height,values})` | anywhere | `{id,width,height}` |
| `release_texture(id)` | | bool |

**`quads` spec:**

| Field | Default | Description |
|-------|---------|-------------|
| `texture` | — | Resource path (empty = untextured) |
| `blend` | true | Alpha blend |
| `cull` | false | Face culling |
| `depth_test` | true | |
| `depth_write` | true | |
| `r,g,b,a` | 1 | Default vertex color |
| `vertices` | required | Array of `{x,y,z,u?,v?,r?,g?,b?,a?}` multiples of 4 |

**`billboards` spec:**

| Field | Description |
|-------|-------------|
| `billboards` or `points` | Array of points |
| `brightness` | 0 skips draw |
| `rotation_x_rad`, `rotation_y_rad` | Sky rotation |
| `blend` | `"alpha"` or `"additive"` |
| `depth_test`, `depth_write` | |

Each point: `yaw_deg`/`az`, `pitch_deg`/`el`, `size`, `alpha`.

---

## `minecraft.tessellator`

| Function | Context | Description |
|----------|---------|-------------|
| `quad(spec)` | Block/item manual draw | 4 vertices with UV |

---

## `minecraft.gui`

All draw functions require **active GUI context** (`screen_event` render, `screen_region` render, or Lua screen).

| Function | Signature |
|----------|-----------|
| `fill_rect(x,y,w,h,color)` | ARGB |
| `draw_text(x,y,text,color)` | |
| `draw_centered_text` | table `{x,y,width,text,color}` or positional |
| `draw_item(x,y,item_id,count,damage?)` | |
| `text_width(text)` | pixels |
| `texture_id(path)` | int |
| `draw_sprite(path,x,y,u,v,w,h)` | Atlas sub-rect |
| `draw_texture(tex_id,x,y,w,h)` | |
| `draw_button` | table or x,y,w,h,text,active,hover |
| `draw_slider` | table or x,y,w,h,normalized,text,hover |
| `draw_toggle` | table or x,y,w,h,label,enabled,hover |
| `begin_3d(opts)` / `end_3d()` | 3D sub-viewport |
| `draw_3d({mode,vertices,color})` | In begin_3d block |
| `unproject(opts)` | `{origin,direction}` ray |

**`begin_3d` options:**

| Field | Default | Description |
|-------|---------|-------------|
| `x`, `y` | 0 | GUI position |
| `width`, `height` or `size` | | Viewport size |
| `gui_width`, `gui_height` | display size | |
| `yaw_deg`, `pitch_deg` | 0 | Camera angles |
| `distance` / `cam_dist` | 2.05 | Camera distance |
| `fov_deg` | 40 | 10..120 |
| `clear_color` | ARGB int | Or `clear_r/g/b/a` floats |

**`draw_3d` modes:** `line_strip`, `lines`, `quads`, `points`, etc.

---

## `minecraft.screen`

| Function | Description |
|----------|-------------|
| `open(screen_id, {title?})` | Open Lua screen |
| `close()` | Close current |
| `host_field(name)` | Read host screen field |
| `host_set_field(name, val)` | Write host field |
| `open_host(id, fields_table)` | Open vanilla screen with fields |
| `add_field(name,x,y,w,h,text,maxLen?,numeric?)` | Text field widget |
| `field_text(name)` / `set_field_text(name,text)` | |
| `add_button(x,y,w,h,text,callback)` | On Lua screen |
| `set_fields_visible(bool)` | |

**Lua compositions (prelude):**

| Function | Description |
|----------|-------------|
| `screen.on_ui(screen_id, region, fn, priority?)` | Vanilla UI injection |
| `screen.on_lua_screen(screen_id, handlers, priority?)` | Custom screen |
| `screen.settings(spec)` | Declarative settings UI |
| `screen.slots(spec)` | Slot-based container UI |

**Screen ids:** `create_world`, `inventory`, `detail_settings`, `world_settings`  
**Regions:** `footer`, `screen`, `side_panel`

---

## `minecraft.files`

| Function | Returns |
|----------|---------|
| `pick({extension=})` | path or nil |
| `read(path)` | text or nil, err |

---

## `minecraft.register_item(spec)`

Returns `ok, err` (does not assert).

| Field | Required | Description |
|-------|----------|-------------|
| `id` | yes | 256 .. ITEM_COUNT-1 |
| `texture` or `texture_id` | yes | Custom path or items.png index 0..255 |
| `name` | no | Display name |
| `translation_key` | no | Lang key |
| `max_count` | no | Default 64 |
| `max_damage` | no | Default 0 |
| `model` | no | Default flat |

**Errors:** duplicate id, reserved id, missing texture for 3D models, unknown model type, bootstrap timing.

---

## Prelude helpers (`screen.slots`, `screen.settings`)

Defined in `LuaRuntimePrelude.hpp` — not separate C++ bindings.

### `minecraft.screen.slots(spec)`

Returns `{ open = fn, ctx = table }`. See [Volume VII](07-gui-and-screens.md).

### `minecraft.screen.settings(spec)`

Returns `open` function. Injects button on parent screen via `screen_ui`.

### `minecraft.screen.on_ui` / `on_lua_screen`

Compatibility wrappers over `minecraft.on` with filters preset.

---

## Internal APIs (prelude removes after load)

These exist briefly during prelude execution only:

| Function | Purpose |
|----------|---------|
| `minecraft._subscribe` | Native event bridge |
| `minecraft._register_block` | Block registration |
| `minecraft._register_shaped_recipe` | Recipe registration |
| `minecraft._read_storage` / `_write_storage` | Persistence |
| `minecraft._resolve_seed` | Seed parsing |
| `minecraft._json_encode` / `_json_decode` | JSON |

Public wrappers: `minecraft.on`, `register_*`, `storage`, `util.json_*`, `util.resolve_seed`.

---

## Prelude-removed APIs (unavailable)

The runtime prelude **nilifies** these after setup:

- `minecraft._subscribe`, `_register_block`, `_register_shaped_recipe`
- `_read_storage`, `_write_storage`
- `io`, `debug`, `dofile`, `loadfile`, `package.cpath`, `package.loadlib`
- Third/fourth package searchers

Use public wrappers only.

---

## Constants (read-only)

```lua
minecraft.events.*          -- 21 event names
minecraft.lifecycle.*       -- 14 phase names
minecraft.generation.stages.*
minecraft.generation.moments.*
minecraft.render.stages.*   -- sky, stars, clouds
minecraft.render.moments.*  -- before, after
minecraft.colors.*          -- sky, fog
minecraft.keys.*            -- escape, enter, space, up, down
```

Attempting to assign to constant tables raises: `"Minecraft constants are read-only"`.
