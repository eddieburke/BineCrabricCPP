# Lua Mod API — Token Dense

Legend: *=mutable event field. Examples omitted.

# Events Reference
### Priority System
### `event.handled` and `event.canceled` Conventions
### `event.before` Pattern
## World Events
### `world_tick`
fields: `remote`:boolean, `before`:boolean, `side`:string
### `world_time`
fields: `world`:userdata, `old_time`:int64, `new_time`:int64, `canceled`:boolean*
### `weather_cycle`
fields: `world`:userdata, `remote`:boolean, `canceled`:boolean*
### `lightning_strike`
fields: `world`:userdata, `x`:int, `y`:int, `z`:int, `canceled`:boolean*
### `snow_ice_placement`
fields: `world`:userdata, `x`:int, `y`:int, `z`:int, `place_snow`:boolean, `place_ice`:boolean
### `random_block_tick`
fields: `world`:userdata, `block`:userdata, `x`:int, `y`:int, `z`:int, `block_id`:int, `canceled`:boolean*
### `scheduled_block_tick`
fields: `world`:userdata, `block`:userdata, `x`:int, `y`:int, `z`:int, `block_id`:int, `instant`:boolean, `canceled`:boolean*
### `schedule_block_update`
fields: `world`:userdata, `x`:int, `y`:int, `z`:int, `block_id`:int, `tick_rate`:int, `canceled`:boolean*
### `tick_rate`
fields: `target_tps`:float*, `tps_scale`:float*
### `chunk_generation`
fields: `stage`:string, `moment`:string, `cancel_vanilla`:boolean*, `vanilla_stage_ran`:boolean, `world_seed`:int64, `has_world`:boolean, `world_name`:string, `is_overworld`:boolean, `mod_generation`:boolean, `chunk_x`:int, `chunk_z`:int, `has_chunk`:boolean, `remote`:boolean, `side`:string
### `create_world`
fields: `save_name`:string, `seed`:int64, `canceled`:boolean*, `options`:table*
### `world_open`
fields: `save_name`:string, `new_world`:boolean, `options`:table, `has_world`:boolean, `world_name`:string, `is_overworld`:boolean, `mod_generation`:boolean, `remote`:boolean, `side`:string
### `world_start`
fields: `save_name`:string, `new_world`:boolean, `has_world`:boolean, `world_name`:string, `is_overworld`:boolean, `mod_generation`:boolean, `remote`:boolean, `side`:string
### `world_spawn_search`
fields: `x`:int*, `y`:int*, `z`:int*, `resolved`:boolean*, `has_world`:boolean, `world_name`:string, `is_overworld`:boolean, `mod_generation`:boolean, `remote`:boolean, `side`:string
## Entity Events
### `block_interact`
fields: `x`:int, `y`:int, `z`:int, `block_id`:int, `right_click`:boolean, `canceled`:boolean*, `handled`:boolean*, `remote`:boolean, `has_player`:boolean, `local_player`:boolean, `has_item`:boolean, `player_x`:double, `player_y`:double, `player_z`:double, `player_yaw`:float, `player_pitch`:float, `item_id`:int, `item_count`:int*, `item_damage`:int*, `item_max_damage`:int, `item_damageable`:boolean, `side`:string
### `entity_interact`
fields: `attack`:boolean, `canceled`:boolean*, `handled`:boolean*, `sneaking`:boolean, `has_player`:boolean, `local_player`:boolean, `has_target`:boolean, `entity_id`:int, `entity_type`:string, `target_id`:int, `has_item`:boolean, `item_id`:int, `item_count`:int*, `item_damage`:int*, `player_yaw`:float, `player_pitch`:float, `remote`:boolean, `side`:string
### `entity_teleport`
fields: `entity_id`:int, `entity_type`:string, `from_x`:double, `from_y`:double, `from_z`:double, `x`:double*, `y`:double*, `z`:double*, `yaw`:float*, `pitch`:float*, `canceled`:boolean*, `has_entity`:boolean, `has_player`:boolean, `has_world`:boolean, `world_name`:string, `is_overworld`:boolean, `mod_generation`:boolean
### `attack_damage`
fields: `damage`:int*, `critical`:boolean*, `canceled`:boolean*, `fall_distance`:float, `on_ground`:boolean, `target_x`:double, `target_y`:double, `target_z`:double, `has_player`:boolean, `has_target`:boolean, `remote`:boolean, `side`:string
### `player_travel`
fields: `sideways`:float*, `forward`:float*, `speed_multiplier`:float*, `has_player`:boolean, `is_local_player`:boolean, `remote`:boolean, `side`:string
### `crafting_take`
fields: `player`:userdata, `stack`:userdata, `canceled`:boolean*
### `furnace_output_take`
fields: `player`:userdata, `stack`:userdata, `canceled`:boolean*
### `entity_spawn`
fields: `entity_id`:int, `entity_type`:string, `item_id`:int, `item_count`:int, `item_damage`:int, `texture_path`:string, `mod_texture`:boolean, `atlas_index`:int
### `entity_remove`
fields: `entity_id`:int, `entity_type`:string, `item_id`:int, `item_count`:int, `item_damage`:int, `texture_path`:string, `mod_texture`:boolean, `atlas_index`:int
### `entity_tick`
fields: `entity_id`:int, `entity_type`:string, `x`:double, `y`:double, `z`:double, `yaw`:float, `pitch`:float, `remote`:boolean, `canceled`:boolean*, `side`:string
## Client Events
### `client_tick`
fields: `before`:boolean, `after_world`:boolean, `paused`:boolean, `has_player`:boolean, `has_world`:boolean, `world_name`:string, `is_overworld`:boolean, `mod_generation`:boolean, `remote`:boolean, `side`:string, `camera_y`:double, `player_y`:double, `player_fall_distance`:float, `player_on_ground`:boolean, `world_time`:double, `is_night`:boolean
### `raycast`
fields: `has_hit`:boolean, `type`:string, `hit_x`:double, `hit_y`:double, `hit_z`:double, `block_x`:int, `block_y`:int, `block_z`:int, `side`:int, `block_id`:int, `block_name`:string, `item_id`:int, `entity_id`:int, `entity_type`:string, `entity_raw_id`:int, `entity_x`:double, `entity_y`:double, `entity_z`:double, `side`:string
### `key_press`
fields: `key`:int, `pressed`:boolean, `repeat`:boolean, `handled`:boolean*
### `mouse_button`
fields: `button`:int, `pressed`:boolean, `handled`:boolean*
### `screen_event`
fields: `screen_id`:string, `phase`:string, `width`:int, `height`:int, `mouse_x`:int, `mouse_y`:int, `x`:int, `y`:int, `tick_delta`:float, `key`:int, `char`:int, `button`:int, `released`:boolean, `delta`:int, `handled`:boolean*
### `screen_ui`
fields: `screen_id`:string, `region`:string, `host_fields`:table, `ui`:table
### `screen_region`
fields: `phase_name`:string, `screen_id`:string, `region`:string, `mouse_x`:int, `mouse_y`:int, `button`:int, `scroll_delta`:int, `x`:int, `y`:int, `width`:int*, `height`:int*, `handled`:boolean*
## Render Events
### `camera_setup`
fields: `tick_delta`:float, `x`:double*, `y`:double*, `z`:double*, `yaw`:float*, `pitch`:float*, `roll`:float*, `custom_view`:boolean*, `hide_first_person_hand`:boolean*
### `render_frame`
fields: `tick_delta`:float
### `fov`
fields: `tick_delta`:float, `fov`:float*
### `world_render`
fields: `tick_delta`:float, `stage`:string, `moment`:string, `cancel_vanilla`:boolean*, `vanilla_stage_ran`:boolean, `shadow_pass`:boolean, `celestial_angle`:float*, `sky_yaw_deg`:float*, `star_brightness`:float*, `rain_strength`:float, `stars_enabled`:boolean, `astronomy_enabled`:boolean*, `astronomy_utc_millis`:double*, `observer_latitude_deg`:float*, `observer_longitude_deg`:float*, `world_time`:double, `celestial`:double, `is_night`:boolean, `camera_x`:double, `camera_y`:double, `camera_z`:double, `camera_yaw`:double, `camera_pitch`:double, `camera_roll`:double, `custom_camera`:boolean, `cloud_base_height`:float, `has_world`:boolean, `world_name`:string, `is_overworld`:boolean, `mod_generation`:boolean
### `first_person_hand`
fields: `tick_delta`:float, `width`:int, `height`:int, `eye`:int, `canceled`:boolean*, `entity_id`:int, `entity_type`:string
### `world_color`
fields: `partial_ticks`:float, `kind`:string, `r`:double*, `g`:double*, `b`:double*, `celestial`:double, `world_time`:double, `is_night`:boolean, `has_world`:boolean, `world_name`:string, `is_overworld`:boolean, `mod_generation`:boolean
### `pre_entity_render`
fields: `entity_id`:int, `entity_type`:string, `tick_delta`:float, `canceled`:boolean*, `item_id`:int, `item_count`:int, `item_damage`:int, `texture_path`:string, `mod_texture`:boolean, `atlas_index`:int
### `entity_render`
fields: `entity_id`:int, `entity_type`:string, `is_player`:boolean, `tick_delta`:float, `pose.body_yaw`:float*, `pose.head_yaw`:float*, `pose.head_pitch`:float*, `pose.limb_swing`:float*, `pose.limb_distance`:float*, `pose.yaw`:float*, `pose.pitch`:float*, `pose.roll`:float*, `pose.scale`:float*, `pose.offset_x`:float*, `pose.offset_y`:float*, `pose.offset_z`:float*, `pose.parts`:table*
## TileEntity Events
### `pre_tile_entity_render`
fields: `x`:int, `y`:int, `z`:int, `id`:string, `tick_delta`:float, `canceled`:boolean*
### `tile_entity_tick`
fields: `x`:int, `y`:int, `z`:int, `id`:string, `remote`:boolean, `removed`:boolean, `canceled`:boolean*, `world_time`:double, `animation_frame`:int, `animation_tick`:double, `animation_speed`:float*, `entity`:userdata
## Lifecycle Event
fields: `previous`:string, `current`:string
## Unsupported Event Names

# API Functions
## Logging
### `minecraft.log(level?, message)`
Writes a line to stdout with the prefix `[lua-mod:<modId>:<level>]`
- **`level`** — `"info"` (default), `"warn"`, or `"error"`
---
## Notifications (Client Only)
### `minecraft.notify(message)`
Adds a local message to the in-game HUD
---
## Context
### `minecraft.is_client()`
→ `true` if the current Lua execution context is on the client side (logical client, including integrated server)
---
## Time
### `minecraft.time.utc_millis()`
→ the current UTC epoch time in milliseconds as a number
---
## Input (Client Only)
### `minecraft.is_key_down(keyCode)`
→ `true` if the keyboard key with the given scancode is currently held
### `minecraft.is_mouse_down(button)`
→ `true` if the given mouse button is pressed (`0` = left, `1` = right, `2` = middle)
### `minecraft.key_code(name)`
Converts a named key to a keyboard scancode integer
- **Direct key names**: `"escape"`, `"1"`–`"0"`, `"q"`, `"w"`, `"e"`, `"r"`, `"t"`, `"y"`, `"u"`, `"i"`, `"o"`, `"p"`, `"enter"`, `"a"`, `"s"`, `"d"`, `"f"`, `"g"`, `"h"`, `"j"`, `"k"`, `"l"`, `"z"`, `"x"`, `"c"`, `"v"`, `"b"`, `"n"`, `"m"`, `"space"`, `"up"`, `"left_arrow"`, `"right_arrow"`, `"down"` - **Binding names** (client only, reads the player's actual keybind): `"forward"` / `"move_forward"`, `"left"` / `"move_left"`, `"back"` / `"backward"` / `"move_back"`, `"right"` / `"move_right"`, `"jump"`, `"sneak"`, `"drop"`, `"inventory"`, `"chat"`, `"fog"`
If passed a number, it is returned as-is (useful pass-through)
Convenience constants are also available on `minecraft.keys`:
---
## Options (Client Only)
### `minecraft.options.get(key)`
Reads a game option value
### `minecraft.options.keys()`
→ an array table of all option persist key strings
---
## Session
### `minecraft.session.*`
Functions for reading and writing the runtime identity
fn: `set_offline_username(name)`, `clear_offline_username()`, `is_offline_mode()`, `get_offline_username()`, `get_username()`, `is_authenticated()`
---
## Asset & File IO
### `minecraft.asset_path(relative)`
→ the absolute filesystem path to a mod asset
### `minecraft.read_asset(relative)`
Reads a mod asset file and returns its content as a string
### `minecraft.read_asset_bytes(relative, options?)`
Reads a mod asset file as a binary string (byte data)
### `minecraft.read_nbt_asset(relative)`
Reads a `.dat` or `.nbt` file as a Lua table
### `minecraft.storage.read(path)`
Reads a file from the mod's persistent storage directory (`runDir/config/mods/<sanitizeName(modId)>/`)
### `minecraft.storage.write(path, content)`
Writes content to the mod's sanitized persistent-storage directory
---
## Config File Parser
### `minecraft.config.load(path, defaults, options?)`
Loads a key-value config file from the mod's storage directory
args: `path`=Relative path within the mod's storage directory; `defaults`=Table of default values (sets expected types and fallback values); `options.aliases`=Table mapping old key names to current key names; `options.separator`=Custom separator (default `"="`)
→ `values, loaded` — `values` is a table with the parsed (or default) values, `loaded` is `true` if the file existed and was read
The type of each default value controls parsing: - **boolean** — parsed via `util.parse_boolean` (accepts `true`/`false`, `1`/`0`, `yes`/`no`, `on`/`off`) - **number** — parsed via `tonumber` - **string** — raw value (empty string preserves default)
### `minecraft.config.save(path, values, options?)`
Writes a key-value config file to the mod's storage directory
args: `path`=Relative path within the mod's storage directory; `values`=Table of key-value pairs to write; `options.keys`=Ordered array of keys to write (default: sorted keys); `options.names`=Table mapping output key names (for aliasing); `options.separator`=Custom separator (default `"="`)
---
## Mod Lifecycle
### `minecraft.at_phase(phase_name, order, callback)`
registers a callback to run during a specific lifecycle phase
**Available phase names** (in order):
`"init"`/all content: blocks, items, entities, etc; `"post_init"`/Resolve cross-references and register recipes; `"ready"`/All registration complete, game is live
The phase name is case-insensitive
### `minecraft.on(event_name, options, callback)`
Subscribes to a game event
`priority`/Integer priority (higher = runs later; default 0); `once`/If `true`, unsubscribes after the first invocation; Any event field/Filter: the callback only fires if `event[field]` matches. Can be a literal value, a table of acceptable values, or a predicate function; `when`/Function `(event) -> boolean` — fires only when it returns true
The callback receives the event table and should return the (possibly mutated) event table
See the [event reference](#event-reference) section below for supported event names and their fields
---
## Lua Helpers
### `minecraft.require(name)`
Safe module loader
Inside `require`'d files, `require` is also sandboxed to `minecraft.require`
### `minecraft.util.clamp(value, min, max)`
Clamps a number to the inclusive range `[min, max]`
### `minecraft.util.trim(str)`
Strips leading and trailing whitespace from a string
### `minecraft.util.in_rect(x, y, left, top, width, height)`
→ `true` if point `(x, y)` lies within the rectangle
### `minecraft.util.real_world(event)`
→ `true` if the event originates from the "real" world (not mod generation)
### `minecraft.util.parse_boolean(value, fallback)`
Parses a string as a boolean
### `minecraft.util.copy(table)`
Performs a shallow copy of a table
---
## JSON API
### `minecraft.util.json_encode(value)`
Encodes a Lua table to a JSON string
### `minecraft.util.json_decode(string)`
Decodes a JSON string to a Lua value
### `minecraft.util.json_null`
A sentinel value representing JSON `null` in decoded data
---
## NBT
`Compound`/Table with string keys; `List`/Array table (1-indexed); `Byte`, `Short`, `Int`, `Long`/Integer; `Float`, `Double`/Number; `String`/String; `ByteArray`/Binary string; `IntArray`/Array table of integers; `LongArray`/Array table of integers; `End`/`nil`
## Item Queries
### `minecraft.items.ids()`
→ an array table of all registered item IDs
### `minecraft.items.describe(item_id)`
→ a table describing an item, or `nil` if the ID is unknown
fields: `id`:int, `max_damage`:int, `damageable`:bool, `stackable`:bool, `has_subtypes`:bool, `max_count`:int
---
## World
### `minecraft.world.block_id(name)`
Resolves a block name to its numeric ID
### `minecraft.world.get_block(x, y, z)`
→ the block ID at the given world position
### `minecraft.world.random(bound?)`
→ a random integer from `[0, bound)` using the world's random source
### `minecraft.world.is_night()`
→ `true` if the world time is between 13000 and 23000 ticks
### `minecraft.world.get_time()`
→ the current world time in ticks, or `0` when no world is available
### `minecraft.world.get_top_y(x, z)`
→ one above the highest solid or fluid block at the given column, or `-1` if no world is available
### `minecraft.world.player()`
→ a table with the current player's position `{x, y, z}`, or `nil` if no player is available
### `minecraft.world.spawn_entity(entityId, position)`
Spawns an entity into the world
### `minecraft.world.count_entities(entityId)`
→ the count of entities with the given type ID in the world
### `minecraft.world.set_time(tick)`
Sets the world time (in ticks)
### `minecraft.world.marker_px(grid, world_x, world_z)`
Converts world coordinates to grid pixel coordinates, clamping to `[0, grid.side - 1]`
---
## Chunk Context (Generation)
### `event.chunk:set_block(localX, y, localZ, blockId)`
### `event.chunk:fill(x1, y1, z1, x2, y2, z2, blockId)`
### `event.chunk:get_block(localX, y, localZ)`
### `event.chunk:get_height(localX, localZ)`
## Entities
### `minecraft.entities.list(filter?)`
→ an array table of entity handle objects in the current world, optionally filtered by entity type string
fields: `id`, `type`, `registry_id`, `data`, `x`, `y`, `z`, `vx`, `vy`, `vz`, `yaw`, `pitch`, `on_ground`, `item_id`, `item_count`, `item_damage`, `item_max_damage`, `texture_path`, `mod_texture`, `atlas_index`
### `minecraft.entities.get(id)`
→ the entity handle object for the given network ID, or `nil` if not found
### `minecraft.entities.spawn_mod(registryId, spec)`
Spawns a custom Lua mod entity
fields: `x`, `y`, `z`, `yaw`, `pitch`, `data`
→ the spawned entity handle object on success, or `nil` on failure
Mod entities cast a vanilla blob shadow sized from their width
### `minecraft.entities.register_global_pose_hook(entityType, callback)`
registers a pose hook for all entities of a given type
### `minecraft.entities.register_local_pose_hook(entityId, callback)`
Like `register_global_pose_hook` but only for a specific entity by ID
### `minecraft.entities.unregister_local_pose_hook(entityId)`
Removes a previously registered local pose hook
---
## Particles
### `minecraft.particles.spawn(spec)`
Spawns a client-side particle
fields: `x`, `y`, `z`, `vx`, `vy`, `vz`, `scale`, `r`, `g`, `b`, `max_age`, `gravity`
---
## Raycast
### `minecraft.raycast(spec?)`
Performs a raycast from the player's camera or a custom origin
fields: `origin` / `origin_x/y/z`, `direction`, `max_distance` / `reach`, `ignore_liquids`, `blocks`, `entities`
→ a table with the hit result, or `nil` if nothing was hit
fields: `type`, `hit_x`, `hit_y`, `hit_z`, `block_x`, `block_y`, `block_z`, `side`, `block_id`, `block_name`, `entity_id`, `entity_type`, `entity_x`, `entity_y`, `entity_z`, `model_id`, `model_tag`
---
## Tile Entities
### `minecraft.tile_entities.list(filter?)`
→ an array of tile entity handles in the current world, optionally filtered by ID string
### `minecraft.tile_entities.get(x, y, z)`
→ the tile entity handle at the given position, or `nil`
### `minecraft.tile_entities.count(filter?)`
→ the count of tile entities, optionally filtered by ID
### Tile Entity Handle Methods
`:get_id()`/string; `:get_block_id()`/int; `:get_block_meta()`/int; `:is_removed()`/bool; `:mark_dirty()`/—; `:distance_from(x, y, z)`/number; `:get_world_time()`/number; `:get_data()`/table; `:set_data(table)`/—; `:get_animation_frame()`/int; `:set_animation_speed(speed)`/—
## Inventory
### `minecraft.inventory.*`
Functions for reading and writing the player's inventory
fn: `slot_count()`, `main_size()`, `get(slot)`, `set(slot, stack)`, `cursor_get()`, `cursor_set(stack)`, `give(stack)`, `offer(stack)`
Stack tables have fields: `id`, `count`, `damage`, `max_damage`, `damageable`, `stackable`, `has_subtypes`, `max_count`
---
## Sound
### `minecraft.sound.*`
fn: `register(id, path, kind?)`, `play(id, volume?, pitch?)`, `play_at(id, x, y, z, volume?, pitch?)`, `play_loop_at(id, x, y, z, volume?, pitch?)`, `stop(handle)`
---
## Screen
### `minecraft.screen.*`
Functions for opening and managing custom Lua screens
fn: `open(id, options?)`, `close()`, `open_host(screenId, fields?)`, `host_field(name)`, `host_set_field(name, value)`, `add_field(name, x, y, w, h, options?)`, `field_text(name)`, `set_field_text(name, text)`, `add_button(x, y, w, h, text, callback?)`, `set_fields_visible(visible)`
### Screen ID Constants (`minecraft.screen.ids`)
Pre-defined screen ID constants: `login`, `title`, `game_menu`, `multiplayer`, `connect`, `disconnected`, `downloading_terrain`, `death`, `chat`, `sleeping_chat`, `confirm`, `create_world`, `select_world`, `edit_world`, `world_settings`, `world_save_conflict`, `inventory`, `crafting`, `dispenser`, `double_chest`, `furnace`, `sign_edit`, `options`, `video_options`, `detail_settings`, `keybinds`, `mods`, `achievements`, `stats`, `lan`, `lan_info`, `server_mod_download`, `fatal_error`, `out_of_memory`
### Screen Region Constants (`minecraft.screen.regions`)
- `footer` - `screen` - `side_panel`
### Screen Convenience Functions
## GUI Drawing
### `minecraft.gui.*`
GUI drawing functions — only usable inside `screen_event` render phase or `screen_region` render phase contexts
fn: `fill_rect(x, y, w, h, argb)`, `draw_text(x, y, text, argb)`, `draw_centered_text({x, y, width/w, text, color?})` or `(x, y, width, text, color?)`, `draw_item(x, y, itemId, count, damage?)`, `text_width(text)`, `texture_id(path)`, `draw_sprite(path/id, x, y, u, v, w, h)`, `draw_texture(textureId, x, y, w, h)`, `draw_button({x, y, width, height, text, active?, mouse_x?, mouse_y?})`, `draw_slider({x, y, width, height, value, text, mouse_x?, mouse_y?})`, `draw_toggle({x, y, width, height, label, value, mouse_x?, mouse_y?})`
---
## Camera (Framebuffer Targets)
### `minecraft.camera.*`
Controls offscreen framebuffer objects for rendering the world to textures (viewfinder / render-to-texture)
fn: `create(width, height, colorCount?, useDepthTex?)`, `create_display_size(colorCount?, useDepthTex?)`, `destroy(handle)`, `resize(handle, width, height)`, `width(handle)`, `height(handle)`, `render(handle, x, y, z, yaw, pitch, roll, fov, tickDelta?)`, `unbind()`, `texture(handle, attachmentIndex?)`, `rendering()`
---
## Render (World-Space Drawing)
### `minecraft.render.*`
Low-level world-space drawing functions, only usable during world render events (`world_render`) or chunk context callbacks
fn: `quads({texture?, texture_id?, blend?, cull?, depth_test?, depth_write?, r?, g?, b?, a?, x?, y?, z?, yaw?, pitch?, roll?, scale?, world_space?, vertices = {{x, y, z, u?, v?, r?, g?, b?, a?}, ...}})`, `billboards({brightness?, rotation_x_rad?, rotation_y_rad?, blend?, depth_test?, depth_write?, billboards/points = {{x, y, z, size, alpha}, ...}})`, `set_item_entity_override(enabled)`
### `minecraft.tessellator.*`
fn: `quad({texture?, texture_id?, r?, g?, b?, a?, vertices = {{x, y, z, u, v}, ...}})`
---
## Texture Queries
### `minecraft.texture.*`
fn: `size(path)`, `pixel(path, x, y)`
---
## Model API
### `minecraft.model.*`
Functions for loading, building, placing, and drawing baked 3D models
fn: `load(path)`, `build({quads = {...}, key?})`, `voxels({cells, resolution, origin_x/y/z, scale, key})`, `voxel({texture, atlas_index?, mod_texture?, grid?, alpha_cutoff?})`, `place(handle, opts)`, `update(instanceId, opts)`, `remove(instanceId)`, `clear()`, `draw(handle, opts)`, `draw_item(itemId, damage, opts)`, `item_bounds(itemId, damage)`, `bounds(handle)`
---
## File Dialogs (Client Only)
### `minecraft.files.*`
fn: `pick(options?)`, `read(path)`
---
## Procedural Texture Creation (Client Only)
### `minecraft.render.create_texture(spec)`
→ a texture from pixel data
### `minecraft.render.release_texture(id)`
Releases a previously created texture
### `minecraft.render.get_texture_pixels(pathOrId)`
→ `{width, height, pixels = {argb...}}` for a texture path or mod texture ID
### `minecraft.render.update_texture(id, spec)`
Updates a texture's pixel data
### `minecraft.render.bind_texture(id, unit)`
Binds a texture ID to a specific sampler unit index (e.g
---
## Seed Resolution
### `minecraft.util.resolve_seed(text)`
Resolves a textual seed to its numeric value (supports numeric strings and named seeds)
---
## Registry Queries
### `minecraft.registry.name(domain, id)`
→ the wire name for a registry entry
### `minecraft.registry.list(domain)`
→ an array of all wire names in a registry
---
## World Grid Sampling
### `minecraft.world.sample(seed, centerX, centerZ, options?)`
Samples terrain/biome data into a grid array for minimap or visualization use
fields: `radius_chunks` / `radius`, `max_side`, `channel`, `channels`, `mod_generation`
Supported channels: `"height"`, `"surface_block"`, `"surface_block_below"`, `"biome_id"`, `"grass"` (grass color as ARGB)
`minecraft.world.sample_grid` remains an alias
→ a table with `side`, `step`, `origin_x`, `origin_z`, `center_x`, `center_z`, `channel`, `values` (primary channel array), and per-channel fields
---
## Event Reference
`client_tick`/`before`, `after_world`, `paused`, `has_player`, `has_world`, `world_name`, `is_overworld`, `camera_y`, `player_y`, `player_fall_distance`, `player_on_ground`, `world_time`, `is_night`, `mod_generation`; `render_frame`/`tick_delta`; `fog_settings`/`enabled`, `spherical`, `exponential`, `start`, `end`, `density`, `custom_color`, `red/green/blue`; `first_person_hand`/`tick_delta`, `eye`, `canceled`, `entity_id`, `entity_type`; `key_press`/`key`, `pressed`, `repeat`, `handled`; `mouse_button`/`button`, `pressed`, `handled`; `raycast`/`has_hit`, `type`, `hit_x/y/z`, `block_x/y/z`, `side`, `block_id`, `block_name`, `item_id`, `entity_id`, `entity_type`; `fov`/`tick_delta`, `fov`; `camera_setup`/`tick_delta`, `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `custom_view`, `hide_first_person_hand`; `player_travel`/`sideways`, `forward`, `speed_multiplier`, `has_player`, `is_local_player`; `tick_rate`/`target_tps`, `tps_scale`; `world_start`/`save_name`, `new_world`; `world_open`/`save_name`, `new_world`, `options` (table); `world_tick`/`remote`, `before`; `entity_tick`/`remote`, `canceled`, `entity_id`, `entity_type`, `x`, `y`, `z`, `yaw`, `pitch`; `tile_entity_tick`/`x`, `y`, `z`, `id`, `remote`, `removed`, `canceled`, `world_time`, `animation_frame`, `animation_tick`, `animation_speed`, `entity`; `create_world`/`save_name`, `seed`, `canceled`, `options` (table); `block_interact`/`x`, `y`, `z`, `block_id`, `side`, `right_click`, `remote`, `canceled`, `handled`, `has_player`, `local_player`, `has_item`, `player_x/y/z`, `player_yaw/pitch`, `item_id/count/damage/max_damage/damageable`; `entity_interact`/`attack`, `remote`, `canceled`, `handled`, `sneaking`, `has_player`, `local_player`, `has_target`, `player_yaw/pitch`, `has_item`, `item_id/count/damage`, `entity_id`, `entity_type`, `target_id`; `attack_damage`/`damage`, `critical`, `canceled`, `fall_distance`, `on_ground`, `target_x/y/z`, `has_player`, `has_target`; `entity_teleport`/`entity_id`, `entity_type`, `from_x/y/z`, `x`, `y`, `z`, `yaw`, `pitch`, `canceled`, `has_entity`, `has_player`; `world_color`/`partial_ticks`, `r`, `g`, `b`, `kind`, `celestial`, `world_time`, `is_night`; `entity_render`/`entity_id`, `entity_type`, `is_player`, `tick_delta`, `pose` (sub-table with `body_yaw`, `head_yaw/pitch`, `yaw`, `pitch`, `roll`, `scale`, `offset_x/y/z`, `parts`); `world_render`/`tick_delta`, `stage`, `moment`, `cancel_vanilla`, `vanilla_stage_ran`, `shadow_pass`, `celestial_angle`, `sky_yaw_deg`, `star_brightness`, `rain_strength`, `stars_enabled`, `astronomy_enabled`, `astronomy_utc_millis`, `observer_lat/lon_deg`, `camera_x/y/z`, `camera_yaw/pitch/roll`, `custom_camera`, `world_time`, `celestial`, `is_night`, `cloud_base_height`; `chunk_generation`/`stage`, `moment`, `cancel_vanilla`, `vanilla_stage_ran`, `world_seed`, `mod_generation`, `is_overworld`, `chunk_x`, `chunk_z`, `has_chunk`; `screen_region`/`phase_name`, `screen_id`, `region`, `mouse_x`, `mouse_y`, `button`, `scroll_delta`, `x`, `y`, `width`, `height`, `handled`; `screen_ui`/`screen_id`, `region`, `host_fields` (table), `ui` (table with `add_centered_button`, `add_button`, `add_stacked_centered_button`); `screen_event`/`screen_id`, `phase`, `width`, `height`, `mouse_x`, `mouse_y`, `tick_delta`, `key`, `char`, `button`, `released`, `delta`, `handled`; `world_spawn_search`/`x`, `y`, `z`, `resolved`; `pre_entity_render`/`entity_id`, `entity_type`, `tick_delta`, `canceled`, item fields; `pre_tile_entity_render`/`x`, `y`, `z`, `id`, `tick_delta`, `canceled`; `entity_spawn`/`entity_id`, `entity_type`, item fields; `entity_remove`/`entity_id`, `entity_type`, item fields
### Lifecycle Phase Constants
### Generation Stage Constants
### Render Stage Constants
### World Color Kind Constants

# Registration
## Block Registration
### `minecraft.register_block(spec)`
registers a new block
fields: `id`:int, `texture`:string, `texture_id`:int, `hardness`:float, `resistance`:float, `luminance`:float, `translation_key`:string, `name`:string, `material`:string, `opaque`:bool, `full_cube`:bool, `translucent`:bool, `collision_height`:float, `stack_on_same`:bool, `requires_solid_below`:bool, `coordinate_bounds`:bool, `coordinate_color`:bool, `bounds_padding`:float, `bounds_offset`:float, `min_scale`:float, `max_scale`:float, `model`:int or function, `item`:table, `tile_entity`:string, `on_use`:function, `behavior_priority`:int
### Validation Rules
## Item Registration
### `minecraft.register_item(spec)`
registers a new item
fields: `id`:int, `texture`:string, `texture_id`:int, `max_count`:int, `max_damage`:int, `translation_key`:string, `name`:string, `model`:int or function
The `ownerModId` field is set automatically from the mod context
### Validation Rules
### Block Items
## Recipe Registration
### `minecraft.register_shaped_recipe(spec)`
registers a shaped (pattern-based) crafting recipe
fields: `output_block_id`:int, `output_item_id`:int, `output_count`:int, `pattern`:array of strings, `key`:string, `item_id`:int
Exactly one of `output_block_id` or `output_item_id` must be set
### `minecraft.register_shapeless_recipe(spec)` — *Not yet implemented*
Shapeless recipe registration is reserved for future use
### `minecraft.register_furnace_recipe(spec)` — *Not yet implemented*
Furnace/smelting recipe registration is reserved for future use
### `minecraft.recipes.remove(recipe_id)`
Removes a previously registered recipe by ID
### `minecraft.recipes.remove_all()`
Removes all recipes
---
## Mod Settings & Keybinds Registry
### `minecraft.settings.register(display_name, entries)`
`display_name` is the label shown for your mod's section in the shared screen (defaults to your mod id if omitted)
fields: `key`:string, `label`:string, `kind`:string, `default`:number/boolean, `min`, `max`:number, `step`:number, `integer`:boolean, `decimals`:number
### `minecraft.settings.get(key)`
→ the current value for one of your own registered settings (number for `slider`, boolean for `toggle`), or `nil` if `key` isn't registered
### `minecraft.keybinds.register(name, spec)`
registers a rebindable keybind, stored and persisted under the id `"<your_mod_id>.<name>"`
### `minecraft.keybinds.get_code(id)`
→ the current key code for a keybind, or `0` if unbound/not found
---
## Custom Block/Item Models
### `minecraft.model.load(path)`
Loads and bakes a JSON model file from the mod's assets (including its parent chain)
### `minecraft.model.build(spec)`
Builds a baked model at runtime from quad data
fields: `quads`:array, `key`:string
Each quad in `quads` is a table with:
fields: `texture`:string, `r`, `g`, `b`:float, `a`:float, `shade`:float, `vertices`:array of 4 tables
Each vertex in `vertices`:
fields: `x`, `y`, `z`:float, `u`, `v`:float
### `minecraft.model.voxels(spec)`
Builds a model from a grid of voxel cells (integer lattice)
fields: `cells`:array, `resolution`:int, `origin_x`, `origin_y`, `origin_z`:float, `scale`:float, `key`:string
Each cell in `cells`:
fields: `x`, `y`, `z`:int, `r`, `g`, `b`:float, `a`:float
→ a model handle, or `nil, error` if no cells result in visible faces
### `minecraft.model.voxel(spec)`
Samples a sprite texture and extrudes its non-transparent pixels into a flat one-voxel-thick model, centered at `z = 0.5`
fields: `texture`:string, `atlas_index`:int, `mod_texture`:bool, `grid`:int, `alpha_cutoff`:int
→ a model handle, or `nil, error` if the texture is not found or has no opaque pixels
### Model Callbacks (for `model` field)

# World and generation
## `minecraft.world.*`
### `minecraft.world.block_id(name)`
Look up the numeric block ID by name
string→string
**Returns:** integer — numeric block ID, or `0` if not found
---
### `minecraft.world.get_block(x, y, z)`
Get the numeric block ID at the given world position in the active world
int→int; int→int; int→int
**Returns:** integer — block ID at the position, or `0` if out of bounds or no world
---
### `minecraft.world.random(bound?)`
Generate a world-scoped random integer
int (optional)→int (optional)
**Returns:** integer — random value in `[0, bound)`
---
### `minecraft.world.is_night()`
Check whether the active world is currently in night time
**Returns:** boolean — `true` if world time is between 13000 and 23000 ticks (inclusive-exclusive), `false` otherwise
---
### `minecraft.world.get_top_y(x, z)`
Get the Y coordinate immediately above the highest solid or fluid block at the given column
int→int; int→int
**Returns:** integer — top block Y + 1, or `-1` if no active world
---
### `minecraft.world.get_heightmap(x, z, width, height)`
Get a packed ARGB heightmap for loaded columns
int→int; int→int; int→int; int→int
**Returns:** integer array in row-major order, or `nil` if no world is active
---
### `minecraft.world.player()`
Get the position of the active player
**Returns:** table `{x, y, z}` containing the player's world coordinates, or `nil` if no player is available
---
### `minecraft.world.spawn_entity(entity_type, {x,y,z} or x,y,z)`
Spawn an entity of the given type at the specified position
string→string; table or vararg→table or vararg
**Returns:** boolean — `true` if the entity was spawned successfully
---
### `minecraft.world.count_entities(entity_type)`
Count the number of entities of a given type in the active world
string→string
**Returns:** integer — entity count
---
### `minecraft.world.set_time(tick)`
Set the world time
int→int
**Returns:** boolean — `true` if the time was set successfully (server-side only; `false` on client)
---
### `minecraft.world.marker_px(grid, world_x, world_z)**
Convert world coordinates to grid pixel coordinates (for minimaps, chunk markers, etc.)
table→table; number→number; number→number
**Returns:** `col, row` — clamped pixel coordinates in `[0, side-1]`, or `0, 0` if grid is invalid
---
## `ChunkHandle`
### `chunk:set_block(localX, y, localZ, blockId)`
int→int; int→int; int→int; int→int
### `chunk:fill(x1, y1, z1, x2, y2, z2, blockId)`
int→int; int→int; int→int; int→int; int→int; int→int; int→int
### `chunk:get_block(localX, y, localZ)`
int→int; int→int; int→int
### `chunk:get_height(localX, localZ)`
int→int; int→int
## `minecraft.entities.*`
### `minecraft.entities.list(filter?)`
List all entities in the world with their full state
string (optional)→string (optional)
**Returns:** array of entity state tables, each containing:
fields: `id`:int, `type`:string, `registry_id`:string, `data`:table, `x`, `y`, `z`:double, `vx`, `vy`, `vz`:double, `yaw`:float, `pitch`:float, `on_ground`:boolean, `item_id`:int, `item_count`:int, `item_damage`:int, `item_max_damage`:int, `texture_path`:string, `mod_texture`:boolean, `atlas_index`:int
---
### `minecraft.entities.apply_state(entity, state)`
Apply position, velocity, rotation, and/or custom data to one LuaModEntity
table→table; table→table
**Returns:** boolean — `true` on success
---
### `minecraft.entities.teleport(id, {x,y,z,yaw,pitch} or x,y,z, yaw?, pitch?)`
Teleport **any** entity (not just mod-spawned) to a new position/rotation
table→table; table or vararg→table or vararg
**Returns:** boolean — `true` if the entity was found and teleported
---
### `minecraft.entities.remove(entity)`
Remove a LuaModEntity from the world
table→table
**Returns:** boolean — `true` if the entity was found and marked for removal
---
### `minecraft.entities.spawn_mod(registryId, {x, y, z, yaw?, pitch?, data?})`
Spawn a custom mod entity
string→string; table→table
**Returns:** int — entity ID of the spawned entity, or `nil` if spawning failed
---
### `minecraft.entities.register_global_pose_hook(entityType, callback)`
Override the render pose for **all** entities of a given type
string→string; function→function
**Returns:** boolean — `true` if registered
---
### `minecraft.entities.register_local_pose_hook(entityId, callback)`
Override the render pose for a **specific** entity by ID
int→int; function→function
**Returns:** boolean — `true` if registered
---
### `minecraft.entities.unregister_local_pose_hook(entityId)`
Remove a previously registered local pose hook
int→int
**Returns:** boolean — `true` if a hook was removed
---
### EntityRenderPose fields
fields: `body_yaw`:float, `head_yaw`:float, `head_pitch`:float, `limb_swing`:float, `limb_distance`:float, `yaw`:float, `pitch`:float, `roll`:float, `scale`:float, `offset_x`:float, `offset_y`:float, `offset_z`:float, `parts`:table
fields: `entity_id`:int, `entity_type`:string, `tick_delta`:float
## `minecraft.tile_entities.*`
### `minecraft.tile_entities.list(filter?)`
List all tile entities (block entities) in the world
string (optional)→string (optional)
**Returns:** array of tile entity handle tables (see handle methods below)
---
### `minecraft.tile_entities.get(x, y, z)`
Get the tile entity handle at a specific world position
int→int; int→int; int→int
**Returns:** tile entity handle table, or `nil` if no tile entity at that position
---
### `minecraft.tile_entities.count(filter?)`
Count tile entities in the world
string (optional)→string (optional)
**Returns:** integer — number of tile entities
---
### Tile entity handle methods
`:get_id()`/string or nil; `:get_block_id()`/int; `:get_block_meta()`/int; `:is_removed()`/boolean; `:mark_dirty()`/nothing; `:distance_from(tx, ty, tz)`/double; `:get_world_time()`/double; `:get_data()`/table or nil; `:set_data(table)`/nothing; `:get_animation_frame()`/int; `:set_animation_speed(speed)`/nothing
## `minecraft.particles.*`
### `minecraft.particles.spawn({x?, y?, z?, vx?, vy?, vz?, scale?, r?, g?, b?, max_age?, gravity?})`
Spawn a custom client-side particle
float→float; float→float; float→float; float→float; float→float; float→float; float→float; float→float; float→float; float→float; int→int; float→float
**Returns:** boolean — `true` if the particle was spawned (always `true` on client with valid world)
---
## `minecraft.items.*`
### `minecraft.items.ids()`
→ a sequential array of all registered item numeric IDs
**Returns:** array of integers — all registered item IDs
---
## `minecraft.raycast`
### `minecraft.raycast(options?)`
Perform a raycast from the camera, or from a custom origin with a custom direction
table (optional)→table (optional)
**Options table fields:**
fields: `origin`:table, `direction`:table, `yaw`, `pitch`:float, `max_distance`:float, `reach`:float, `ignore_liquids`:boolean, `blocks`:boolean, `entities`:boolean
**Returns:** hit result table or `nil` if nothing was hit
The result table has `type` — one of `"block"`, `"entity"`, or `"model"`
**Block hit result:**
fields: `type`:string, `block_id`:int, `block_name`:string, `item_id`:int, `block_x`, `block_y`, `block_z`:int, `side`:int, `hit_x`, `hit_y`, `hit_z`:double
**Entity hit result:**
fields: `type`:string, `entity_id`:int, `entity_raw_id`:int, `entity_type`:string, `entity_x`, `entity_y`, `entity_z`:double, `hit_x`, `hit_y`, `hit_z`:double
**Model hit result:**
fields: `type`:string, `model_id`:int, `model_tag`:string, `hit_x`, `hit_y`, `hit_z`:double, `distance`:double
All hit results include `hit_x`, `hit_y`, `hit_z`
---
## World Generation Events
### `chunk_generation`
fields: `stage`:string, `moment`:string, `cancel_vanilla`:boolean, `vanilla_stage_ran`:boolean, `world_seed`:int, `has_world`:boolean, `world_name`:string, `is_overworld`:boolean, `mod_generation`:boolean, `chunk_x`, `chunk_z`:int, `has_chunk`:boolean, `chunk`:ChunkHandle
`terrain`/RawGeneration; `surface`/RawGeneration; `carver`/RawGeneration; `features`/ChunkApi
### `world_spawn_search`
fields: `x`:int, `y`:int, `z`:int, `resolved`:boolean, `has_world`:boolean, `world_name`:string, `is_overworld`:boolean
### `create_world`
fields: `save_name`:string, `seed`:int, `canceled`:boolean, `options`:table
### `world_open`
fields: `save_name`:string, `new_world`:boolean, `options`:table
### `world_start`
fields: `save_name`:string, `new_world`:boolean
### `world_tick`
fields: `remote`:boolean, `before`:boolean

# Rendering
## World Rendering Pipeline
### `minecraft.render.stages`
Constant table with one entry per stage (value equals its own key):
`"sky"`/Sky background; `"stars"`/Starfield; `"terrain_opaque"`/Opaque terrain (chunk geometry); `"entities"`/All living entities and items; `"particles_lit"`/Lit particles (torches, glowstone, etc.); `"particles"`/Regular particles; `"terrain_translucent"`/Translucent terrain (water, glass, ice); `"weather"`/Rain and snow; `"clouds"`/Clouds; `"hand"`/First-person hand; `"framebuffer"`/Framebuffer blit / post-processing
Usage:
### `minecraft.render.moments`
`"before"`/Fire just before vanilla renders the stage; `"after"`/Fire just after vanilla renders the stage
The `world_render` event also exposes the following fields on the event table:
fields: `world`:World, `camera`:Entity, `tick_delta`:number, `stage`:string, `moment`:string, `cancel_vanilla`:boolean, `vanilla_stage_ran`:boolean, `shadow_pass`:boolean, `celestial_angle`:number, `sky_yaw_deg`:number, `star_brightness`:number, `rain_strength`:number, `stars_enabled`:boolean, `astronomy_enabled`:boolean, `astronomy_utc_millis`:number, `observer_latitude_deg`:number, `observer_longitude_deg`:number
---
## `minecraft.render.*`
### `minecraft.render.quads(spec)`
Draw textured or colored quads in world space
**spec table fields:**
fields: `texture`:string, `texture_id`:int, `r`:number, `g`:number, `b`:number, `a`:number, `x`:number, `y`:number, `z`:number, `yaw`:number, `pitch`:number, `roll`:number, `scale`:number, `world_space`:boolean, `blend`:boolean, `cull`:boolean, `depth_test`:boolean, `depth_write`:boolean, `vertices`:array
**Vertex table fields** (each vertex can override the default tint):
fields: `x`:number, `y`:number, `z`:number, `u`:number, `v`:number, `r`:number, `g`:number, `b`:number, `a`:number
If `texture` and `texture_id` are both absent/empty, quads are drawn without a bound texture (colored only)
The vertex count is rounded down to the nearest multiple of 4
### `minecraft.render.billboards(spec)`
Draw always-facing quads (billboards) using 3D direction vectors
**spec table fields:**
fields: `brightness`:number, `rotation_x_rad`:number, `rotation_y_rad`:number, `blend`:string, `depth_test`:boolean, `depth_write`:boolean, `billboards`:array
**Billboard entry fields:**
fields: `x`, `y`, `z`:number, `size`:number, `alpha`:number
→ the count of billboards emitted (integer)
### `minecraft.render.set_item_entity_override(enabled)`
Suppress the native dropped-item (ItemEntity) sprite renderer so a Lua mod can draw its own custom 3D model for ItemEntities instead
---
## `minecraft.tessellator.*`
### `minecraft.tessellator.quad(spec)`
Emit a single textured/colored quad to the currently active world tessellator (block or item draw)
**spec table fields:**
fields: `texture`:string, `texture_id`:int, `r`:number, `g`:number, `b`:number, `a`:number, `vertices`:array
→ `true` if the quad was emitted, `false` otherwise
Positions are in the block/item's local model space
---
## `minecraft.camera.*` (Render Targets / Viewfinder Cameras)
fn: `create`, `create_display_size`, `destroy`, `resize`, `render`, `render_shadow_orthographic`, `render_shadow_perspective`, `texture`, `depth_texture`, `width`, `height`, `rendering`, `unbind`, `far_plane`
### `camera.create(width, height, colorCount?, useDepthTex?)`
### `camera.create_display_size(colorCount?, useDepthTex?)`
### `camera.destroy(handle)`
### `camera.resize(handle, width, height)`
### `camera.render(handle, x, y, z, yaw, pitch, roll, fov, tickDelta?)`
### `camera.texture(handle, attachmentIndex?)`
### `camera.width(handle)` / `camera.height(handle)`
### `camera.rendering()`
### `camera.unbind()`
## `minecraft.model.*`
### `minecraft.model.load(path)`
Load and bake a JSON model from a mod's assets
→ the model handle (integer ≥ 1) on success, or `nil, error` on failure
### `minecraft.model.build(spec)`
Build a baked model programmatically from an array of quads
**spec table fields:**
fields: `quads`:array, `key`:string
**Quad specification fields:**
fields: `texture`:string, `r`:number, `g`:number, `b`:number, `a`:number, `shade`:number, `vertices`:array
→ the model handle (integer) on success, or `nil, error` on failure
### `minecraft.model.place(handle, opts)`
Place an instance of a baked model in the world
**opts fields:**
fields: `x`:number, `y`:number, `z`:number, `yaw`:number, `pitch`:number, `roll`:number, `scale`:number, `pivot_y`:number, `tag`:string
→ the instance ID (integer ≥ 1) on success, or `nil, error` on failure
### `minecraft.model.update(instanceId, opts)`
Update an existing placed instance's transform
### `minecraft.model.remove(instanceId)`
Remove a placed instance
### `minecraft.model.clear()`
Remove all placed model instances belonging to the current mod
### `minecraft.model.bounds(handle)`
Get the model-space bounding box of a baked model
### `minecraft.model.draw(handle, opts)`
Draw a baked model in world space immediately
**opts fields:**
fields: `x`:number, `y`:number, `z`:number, `yaw`:number, `pitch`:number, `roll`:number, `scale`:number, `pivot_y`:number, `brightness`:number, `a`:number, `blend`:boolean, `cull`:boolean, `depth_test`:boolean, `depth_write`:boolean
### `minecraft.model.draw_item(item_id, damage, opts)`
Draw an item or block's 3D model in world space
### `minecraft.model.item_bounds(item_id, damage)`
Get the model-space bounding box of an item's 3D model
---
## `minecraft.model.voxels(opts)`
fields: `cells`:array, `resolution`:int, `scale`:number, `origin_x`:number, `origin_y`:number, `origin_z`:number, `key`:string
fields: `x`:int, `y`:int, `z`:int, `r`:number, `g`:number, `b`:number, `a`:number
## `minecraft.model.voxel(spec)`
fields: `texture`:string, `atlas_index`:int, `mod_texture`:boolean, `grid`:int, `alpha_cutoff`:int
## `minecraft.texture.*`
### `minecraft.texture.size(path)`
Get the dimensions of a texture
### `minecraft.texture.pixel(path, x, y)`
Get the color of a single pixel
---
## GUI 3D Viewport
### `gui.begin_3d(params)`
fields: `x`:int, `y`:int, `size`:int, `width`:int, `height`:int, `gui_width`:int, `gui_height`:int, `yaw_deg`:number, `pitch_deg`:number, `distance` / `cam_dist`:number, `fov_deg`:number, `clear_color`:int, `clear_r`:number, `clear_g`:number, `clear_b`:number, `clear_a`:number
### `gui.end_3d()`
### `gui.draw_3d(spec)`
fields: `mode`:string, `color`:int, `r`:number, `g`:number, `b`:number, `a`:number, `line_width`:number, `point_size`:number, `vertices`:array
### `gui.unproject(params)`
## Render Events (Reference)
`world_color`/Modify sky/fog color. `event.kind` is `"sky"` or `"fog"` (from `minecraft.colors`). Set `event.color` (Vec3d); `camera_setup`/Override camera position, rotation, and roll. Fields: `x, y, z, yaw, pitch, roll`. Set `customView = true`; `fov`/Override field of view. Set `event.fov` (float, default 70); `first_person_hand`/Cancel or control first-person hand rendering. Set `canceled = true` to hide the hand; `render_frame`/Start-of-frame hook. Fires once per frame before any world rendering; `world_render`/Per-stage render hooks. Fields: `stage`, `moment`, `cancel_vanilla`, etc; `pre_entity_render`/Pre-entity-render hook. Set `canceled = true` to skip an entity; `entity_render`/Entity render hook with pose control. Modify `event.pose` (bodyYaw, headYaw, headPitch, yaw, pitch, roll, scale, offsetX/Y/Z, parts) to override entity rendering

# GUI and screens
## GUI draw scope
## `minecraft.gui.*`
### `minecraft.gui.fill_rect(x, y, w, h, argb_color)`
Draws a filled rectangle
### `minecraft.gui.draw_text(x, y, text, argb_color)`
Draws left-aligned text at the given position
### `minecraft.gui.draw_item(x, y, itemId, count, damage?)`
Draws an item stack icon (sprite + optional count/decorations)
### `minecraft.gui.text_width(text)`
→ the pixel width of `text` as rendered by the current font renderer
### `minecraft.gui.texture_id(path)`
→ the OpenGL texture ID for a resource path (e.g
### `minecraft.gui.draw_sprite(path_or_textureId, x, y, u, v, w, h)`
Draws a sprite (sub-rectangle) from a texture atlas or full texture
The remaining arguments are: `x, y` screen position, `u, v` UV offset (in pixels) within the texture, `w, h` size of the sprite region
### `minecraft.gui.draw_texture(textureId, x, y, w, h)`
Draws a full-texture quad covering the entire texture (UV 0-1) at the given screen rectangle
### `minecraft.gui.draw_button({x, y, width, height, text, active?})`
Draws a vanilla-style button widget
fields: `x`:number, `y`:number, `width`:number, `height`:number, `text`:string, `active`:boolean, `mouse_x`:number, `mouse_y`:number, `hovered`:boolean
The button is drawn with the vanilla gui.png texture (9-slice), with text centered
### `minecraft.gui.draw_slider({x, y, width, height, value, text})`
Draws a vanilla-style slider widget (e.g
fields: `x`:number, `y`:number, `width`:number, `height`:number, `value`:number, `text`:string, `mouse_x`:number, `mouse_y`:number
### `minecraft.gui.draw_toggle({x, y, width, height, label, value})`
Draws a vanilla-style toggle/on-off button
fields: `x`:number, `y`:number, `width`:number, `height`:number, `label`:string, `value`:boolean, `mouse_x`:number, `mouse_y`:number
### `minecraft.gui.draw_centered_text(x, y, width, text, color?)`
Draws text horizontally centered within a region
**Positional form:**
**Table form:**
number→number; number→number; number→number; string→string; number→number
---
---
## `minecraft.screen.*`
### `minecraft.screen.open(screen_id, {title?, pause?})`
Opens a Lua-defined screen
fields: `title`:string, `pause`:boolean
The screen is registered implicitly on first open
### `minecraft.screen.close()`
Closes the currently open screen (returns to the previous screen or game)
### `minecraft.screen.host_field(name)`
Reads the current text value of a field on the **host screen** (the underlying engine screen, e.g
### `minecraft.screen.host_set_field(name, value)`
Sets a field's text on the host screen
### `minecraft.screen.open_host(screen_id, fields?)`
Opens a host-defined (engine/native) screen by its screen ID string
The screen must have been registered with the host's `HostScreenRegistry` by the engine
### `minecraft.screen.add_field(name, x, y, width, height, {text?, max_len?, numeric?, signed?, decimal?}))
Adds a text input widget to the Lua screen
fields: `name`:string, `x`:number, `y`:number, `width`:number, `height`:number, `text`:string, `max_len`:number, `numeric`:boolean, `signed`:boolean, `decimal`:boolean
### `minecraft.screen.field_text(name)`
→ the current text of a Lua screen field added with `add_field`
### `minecraft.screen.set_field_text(name, text)`
Sets the text of a Lua screen field programmatically
### `minecraft.screen.add_button(x, y, width, height, text, callback?)`
Adds a clickable button widget to the Lua screen
### `minecraft.screen.set_fields_visible(bool)`
Toggles visibility of all text field widgets on the Lua screen
---
## `minecraft.screen.on_ui(screen_id, region, callback, priority?)`
string→string; string→string; function→function; number→number
### Regions
`regions.footer`/`"footer"`; `regions.screen`/`"screen"`; `regions.side_panel`/`"side_panel"`
## `minecraft.screen.on_lua_screen(screen_id, handlers, priority?)`
`init`/`{screen_id, phase, width, height}`; `render`/`{screen_id, phase, width, height, mouse_x, mouse_y, tick_delta}`; `mouse`/`{screen_id, phase, x, y, button, released?}`; `key`/`{screen_id, phase, key, char, handled?}`; `tick`/`{screen}`; `scroll`/`{screen_id, phase, x, y, delta}`; `close`/`{screen_id, phase}`
## Screen constants
### `minecraft.screen.ids.*`
All screen ID string constants for the engine's built-in screens:
`ids.login`/`"minecraft:login"`; `ids.title`/`"minecraft:title"`; `ids.game_menu`/`"minecraft:game_menu"`; `ids.multiplayer`/`"minecraft:multiplayer"`; `ids.connect`/`"minecraft:connect"`; `ids.disconnected`/`"minecraft:disconnected"`; `ids.downloading_terrain`/`"minecraft:downloading_terrain"`; `ids.death`/`"minecraft:death"`; `ids.chat`/`"minecraft:chat"`; `ids.sleeping_chat`/`"minecraft:sleeping_chat"`; `ids.confirm`/`"minecraft:confirm"`; `ids.create_world`/`"minecraft:create_world"`; `ids.select_world`/`"minecraft:select_world"`; `ids.edit_world`/`"minecraft:edit_world"`; `ids.world_settings`/`"minecraft:world_settings"`; `ids.world_save_conflict`/`"minecraft:world_save_conflict"`; `ids.inventory`/`"minecraft:inventory"`; `ids.crafting`/`"minecraft:crafting"`; `ids.dispenser`/`"minecraft:dispenser"`; `ids.double_chest`/`"minecraft:double_chest"`; `ids.furnace`/`"minecraft:furnace"`; `ids.sign_edit`/`"minecraft:sign_edit"`; `ids.options`/`"minecraft:options"`; `ids.video_options`/`"minecraft:video_options"`; `ids.detail_settings`/`"minecraft:detail_settings"`; `ids.keybinds`/`"minecraft:keybinds"`; `ids.mods`/`"minecraft:mods"`; `ids.mod_settings`/`"minecraft:mod_settings"`; `ids.achievements`/`"minecraft:achievements"`; `ids.stats`/`"minecraft:stats"`; `ids.lan`/`"minecraft:lan"`; `ids.lan_info`/`"minecraft:lan_info"`; `ids.server_mod_download`/`"minecraft:server_mod_download"`; `ids.fatal_error`/`"minecraft:fatal_error"`; `ids.out_of_memory`/`"minecraft:out_of_memory"`
### `minecraft.screen.regions.*`
`regions.footer`/`"footer"`; `regions.screen`/`"screen"`; `regions.side_panel`/`"side_panel"`
---
## Settings DSL
### `minecraft.screen.settings({id, title, parent_screen?, parent_region?, button_label?, values?, sliders?, toggles?, on_change?, on_save?, on_reset?, priority?})`
→ a complete settings screen with auto-generated UI
The function: 1
Set `parent_screen = minecraft.screen.ids.mod_settings` to place the generated page under **Mod Pages** automatically
string→string; string→string; string→string; string→string; string→string; table or function→table or function; array of tables→array of tables; array of tables→array of tables; function→function; function→function; function→function; number→number
Each slider entry:
fields: `key`:string, `label`:string, `min`:number, `max`:number, `integer`:boolean, `format`:function
Each toggle entry:
fields: `key`:string, `label`:string
---
## Session API
### `minecraft.session.set_offline_username(name)`
Sets the offline-mode username override
### `minecraft.session.clear_offline_username()`
Clears the offline username override, reverting to the default behavior
### `minecraft.session.is_offline_mode()`
→ `true` if an offline username override is currently active
### `minecraft.session.get_offline_username()`
→ the current offline username override string (may be empty if not set)
### `minecraft.session.get_username()`
→ the **live session username** from the Minecraft client (the authenticated username or last-used name), regardless of offline override
### `minecraft.session.is_authenticated()`
→ `true` if the client has a valid Microsoft authentication session

# Inventory, audio, utilities
## `minecraft.inventory.*`
### Slot layout
### `minecraft.inventory.slot_count()`
→ the total number of inventory slots: `40` (36 main + 4 armor)
### `minecraft.inventory.main_size()`
→ the size of the main inventory: `36`
### `minecraft.inventory.get(slot)`
→ a table describing the item stack at `slot`
The returned table:
fields: `id`:number, `count`:number, `damage`:number, `max_damage`:number, `damageable`:boolean, `stackable`:boolean, `has_subtypes`:boolean, `max_count`:number
### `minecraft.inventory.set(slot, {id, count?, damage?})`
Sets the item stack at `slot`
The item spec table accepts `id` (required), optional `count` (default 1), and optional `damage` (default 0)
### `minecraft.inventory.cursor_get()`
→ the item stack currently held on the cursor (being dragged from a container slot)
### `minecraft.inventory.cursor_set({id, count?, damage?})`
Sets the cursor stack
### `minecraft.inventory.give({id, count?, damage?})`
Gives an item stack to the player
### `minecraft.inventory.offer({id, count?, damage?})`
Offers an item stack to the player
---
## `minecraft.items.*`
### `minecraft.items.ids()`
→ an array of all registered item numeric IDs
### `minecraft.items.describe(item_id)`
→ metadata about an item ID, or `nil` if the ID is invalid or the item is empty
fields: `id`:number, `max_damage`:number, `damageable`:boolean, `stackable`:boolean, `has_subtypes`:boolean, `max_count`:number
---
## `minecraft.sound.*`
### `minecraft.sound.register(id, filepath, kind?)`
registers a new sound effect with the audio engine
`"effect"`/Short sound effect, loaded entirely into memory (default); `"streaming"`/Longer sound, streamed from disk; `"music"`/Background music track
→ `false, error_message` on failure (missing file, unknown kind), or `true` on success
### `minecraft.sound.play(id, volume?, pitch?)`
Plays a registered sound as a 2D (non-positional) effect
### `minecraft.sound.play_at(id, x, y, z, volume?, pitch?)`
Plays a registered sound at a 3D world position
### `minecraft.sound.play_loop_at(id, x, y, z, volume?, pitch?)`
Plays a registered sound as a looping 3D positional sound
### `minecraft.sound.stop(handle)`
Stops a looping sound by its handle (returned from `play_loop_at`)
---
## JSON utilities (`minecraft.util.*`)
### `minecraft.util.json_encode(value)`
Encodes a Lua table to a JSON string
→ `(json_string)` on success, or `(nil, error_message)` if the value is not JSON-serializable
### `minecraft.util.json_decode(string)`
Decodes a JSON string to a Lua value
→ `(value)` on success, or `(nil, error_message)` on failure (empty input, invalid JSON, trailing characters)
### `minecraft.util.json_null`
A special sentinel value representing JSON `null`
### `minecraft.util.resolve_seed(text)`
Resolves a seed string (numeric or textual) into a 64-bit integer, matching Minecraft's seed resolution logic
