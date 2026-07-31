# 01 - Core Environment & Lifecycle API

The BineCrabricCPP Lua runtime executes mods within a Lua 5.4 sandboxed environment attached to the C++ host engine. This chapter details the execution sandbox, module resolution, storage persistence, mod lifecycles, and core utility functions.

---

## 1. Lua 5.4 Sandbox Environment

For security and deterministic execution across single-player and multiplayer hosts, standard dangerous Lua libraries are disabled:

- **Whitelisted `os` Library Functions**:
  - `os.clock()`, `os.date()`, `os.difftime()`, `os.time()`
- **Blocked Functions & Libraries** (set to `nil`):
  - `io` (file I/O operations are strictly restricted to managed storage APIs)
  - `debug`, `dofile`, `loadfile`
  - `package.cpath` (forced to `""`), `package.loadlib` (set to `nil`)
  - Dynamic C library loaders (`package.searchers[3]`, `package.searchers[4]` removed)

---

## 2. Core Namespace API

The primary functions under the `minecraft` root namespace:

### `minecraft.is_client()`
- **Signature**: `minecraft.is_client() -> boolean`
- **Returns**: `true` if executing within a client runtime with rendering and GUI capabilities; `false` if executing on a dedicated server host.

### `minecraft.log(level, message)`
- **Signature**: `minecraft.log(level: string, message: string)`
- **Parameters**:
  - `level`: `"info"`, `"warn"`, or `"error"`.
  - `message`: Log string to format. Outputs to std::cout and host `client.log`/`server.log` tagged as `[lua-mod:<mod_id>:<level>]`.

### `minecraft.asset_path(relative_path)`
- **Signature**: `minecraft.asset_path(relative_path: string) -> string`
- **Returns**: Absolute path on the host filesystem resolved relative to the calling mod's root asset directory.

### `minecraft.list_assets(dir_path)`
- **Signature**: `minecraft.list_assets(dir_path: string) -> table`
- **Returns**: Array of file names (strings) existing within the specified relative asset subdirectory of the mod package.

---

## 3. Module Resolution System

Mods can structure their code into sub-modules using `require` and directory loaders.

### `minecraft.require(name)`
- **Signature**: `minecraft.require(name: string) -> table | any`
- **Global Alias**: `require = minecraft.require`
- **Behavior**: Resolves dot-separated module names (e.g. `"my_mod.submodule"`). Automatically strips leading mod ID prefixes if present and resolves against local mod paths (`root_dir/?.lua`, `root_dir/?/init.lua`).

### `minecraft.require_dir(dir)`
- **Signature**: `minecraft.require_dir(dir: string) -> table`
- **Behavior**: Scans `dir` for all `.lua` files (excluding `init.lua`), calls `minecraft.require` on each module stem, and returns an array of tables:
  ```lua
  {
    { name = "filename_stem", module = required_module_result },
    ...
  }
  ```

---

## 4. Storage & Persistence

File I/O is managed safely via the key-value/file storage API and configuration serializers.

### `minecraft.storage`
- **`minecraft.storage.read(path)`**: Reads text content stored at `path` relative to mod data directory. Returns string or `nil` if missing.
- **`minecraft.storage.write(path, content)`**: Writes text string `content` to `path`. Returns `boolean` success indicator.

### `minecraft.config`

Simple key-value format parser (supports `=` or `:` separators, `#` or `;` comments).

#### `minecraft.config.load(path, defaults, options)`
- **Signature**: `minecraft.config.load(path: string, defaults: table, options?: table) -> values: table, loaded: boolean`
- **Parameters**:
  - `path`: File path to load.
  - `defaults`: Table of default keys and initial typed values.
  - `options.aliases`: Optional map of key aliases (e.g. `{ fog_end = "end_val" }`).
- **Returns**: Parsed configuration values coerced to default types (boolean, number, string), and boolean indicating if file existed.

#### `minecraft.config.save(path, values, options)`
- **Signature**: `minecraft.config.save(path: string, values: table, options?: table) -> success: boolean`
- **Parameters**:
  - `options.keys`: Ordered list of key strings to write.
  - `options.names`: Map of canonical key names to output config key strings.
  - `options.separator`: Key-value delimiter string (defaults to `"="`).

---

## 5. Mod Package Structure & Lifecycles

Every mod resides under `native/mods/<mod_folder>/` and must define a `mod.json` manifest:

```json
{
  "id": "my_custom_mod",
  "name": "My Custom Mod",
  "version": "1.0.0",
  "entry": "scripts/main.lua"
}
```

### Lifecycle Phases (`minecraft.lifecycle`)

- **`init`**: Mod entry script execution phase. Registrations for blocks, items, recipes, and events occur here.
- **`post_init`**: Triggered after all enabled mods have executed their `init` entry scripts.
- **`ready`**: Triggered when the host engine is initialized and ready to start world ticks.

---

## 6. Core Utilities (`minecraft.util`)

Built-in mathematical and data helpers available under `minecraft.util`:

- **`clamp(val, min, max)`**: Clamps `val` between `min` and `max`.
- **`trim(str)`**: Trims leading and trailing whitespace from `str`.
- **`in_rect(x, y, left, top, width, height)`**: Returns `true` if `(x, y)` is within the axis-aligned rectangle.
- **`parse_boolean(val, fallback)`**: Coerces `"true"`, `"1"`, `"yes"`, `"on"` to `true`; `"false"`, `"0"`, `"no"`, `"off"` to `false`; otherwise returns `fallback`.
- **`copy(tbl)`**: Creates a shallow copy of table `tbl`.
- **`real_world(event)`**: Helper for generation/world events. Returns `false` if `event.mod_generation == false`.
