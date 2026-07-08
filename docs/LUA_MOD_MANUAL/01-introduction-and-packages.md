# Volume I — Introduction and Mod Packages

**Document MC-LUA-API-173 Rev 2.0**

This manual documents the **complete** Lua mod API exposed by the Beta 1.7.3 native client. Every function, event field, and registration rule is derived from engine sources under `native/src/net/minecraft/mod/`.

---

## Philosophy

Mods are **zip packages** of Lua scripts and assets. The engine provides:

1. **Generic hooks** — 21 event types via `minecraft.on`
2. **Declarative registration** — blocks, items, recipes at startup
3. **Managed drawing** — no raw OpenGL; draw only inside sanctioned callbacks
4. **Mod-scoped sandbox** — no `io`, no `package.cpath`, no host filesystem except scoped APIs

The engine contains **no per-mod C++ gameplay logic**. Shipped mods in `native/mods/` are reference implementations, not special cases.

---

## Package layout

```text
<mod_id>/
  mod.json              required manifest
  scripts/
    main.lua            entry point (required)
    *.lua               loadable via minecraft.require
  assets/               mod-owned files (read_asset)
  resources/            vanilla resource overlay
    mods/<mod_id>/...   textures, sounds, lang
  lang/                 optional translations
```

**Package** with `native/package-runtime-mods.ps1`. Output goes to `%APPDATA%/.minecraft/mods/<mod_id>.zip`.

### `mod.json` (minimal)

```json
{
  "id": "my_mod",
  "name": "My Mod",
  "version": "1.0.0"
}
```

The `id` must match the directory name and is used for logging, storage paths, and resource prefixes.

---

## Bootstrap order

When the client starts:

```
1. Discover enabled mod zips
2. For each mod (load order):
     Create isolated Lua state
     Install native API tables (Core, World, Render, Screen, Inventory, Sound, Item, Generic)
     Execute runtime prelude (LuaRuntimePrelude.hpp)
     Execute scripts/main.lua
       → register_block / register_item / register_shaped_recipe queue specs
       → minecraft.on registers callbacks (stored, not fired yet)
3. Registry::bootstrap() — lifecycle phases in order:
     BlockRegistration          (vanilla by id, Lua @ order 50000)
     BlockRegistryFinalize
     BiomeRegistration
     ItemRegistration         (Lua @ 50000)
     BlockItemRegistration
     SmeltingRecipeRegistration
     CraftingRecipeRegistration (vanilla + Lua @ 50000, then sort)
     EntityRegistration
     BlockEntityRegistration
     FuelRegistration
     ClientRendererRegistration
     ParticleRegistration
4. ModLifecycle::frozen()
5. Game runs; callbacks fire on hooks
```

**Critical rule:** `register_block`, `register_item`, and `register_shaped_recipe` only succeed during step 2. After bootstrap they return errors.

**Exception:** `minecraft.crafting.add_shaped_recipe(spec)` works at runtime (re-sorts recipe table).

---

## Sandbox and security

After prelude load, these are **nil or removed**:

| Removed | Replacement |
|---------|-------------|
| `io` | `minecraft.read_asset`, `minecraft.storage` |
| `debug` | — |
| `dofile`, `loadfile` | `minecraft.require` |
| `package.cpath`, `package.loadlib` | — |
| `package.searchers[3]`, `[4]` | mod-local path only |
| `minecraft._subscribe`, `_register_*`, `_read_storage`, etc. | public wrappers |

**`os` is restricted** to: `clock`, `date`, `difftime`, `time`.

**`minecraft.require(name)`** — loads only from the mod package (`scripts/`). Names must match `^[%w_.-]+$` and cannot contain `..`.

**Storage paths:**

- Default: `.minecraft/config/mods/<mod_id>/...`
- Legacy: bare `foo.cfg` or `foo.txt` in run directory root (backward compatibility)

---

## The `minecraft` global

Installed by `ModHost` in this order:

| Installer | Namespace |
|-----------|-----------|
| `installCoreApi` | `log`, `time`, `options`, `at_phase`, assets, keys |
| `installWorldApi` | `chunk`, `world`, `particles`, `items.ids`, `_register_block` |
| `installItemApi` | `register_item` |
| `installRenderApi` | `render`, `tessellator` |
| `installScreenApi` | `gui`, `screen` |
| `installInventoryApi` | `inventory`, `items.describe` |
| `installSoundApi` | `sound` |
| `installGenericModApi` | `registry`, `world.sample_grid`, `files`, `render.create_texture` |
| Prelude | `events`, `lifecycle`, `on`, `register_*`, `util`, `config`, `screen.*` helpers |

---

## Subscription model

```lua
-- Preferred
minecraft.on(event_name, { filter = value, priority = 0, once = false, when = fn }, callback)

-- Compatibility
minecraft.on(event_name, callback, priority)
```

- **Priority:** lower integer runs **earlier** on the same hook.
- **C++ fast filters:** `screen_id`, `region`, `stage`, `moment` (string equality only).
- **All other option keys** match `event[key]` in Lua (`event_matches` in prelude).
- Filter values: scalar, array of accepted values, or predicate function.
- **Return:** mutable events accept returned table; `handled` / `canceled` fields apply.
- Unknown event names: logged, subscription fails (`native_subscribe` returns false).

---

## World context fields

Many events include these (from `setWorldContextFields`):

| Field | Type | Description |
|-------|------|-------------|
| `has_world` | bool | World pointer non-null |
| `world_name` | string | Save directory name |
| `is_overworld` | bool | Not nether, no ceiling |
| `mod_generation` | bool | World runs mod chunk callbacks |

Use `minecraft.util.real_world(event)` to skip probe/synthetic worlds (`mod_generation == false`).

---

## Logging

```lua
minecraft.log("info", "message")
minecraft.log("warn", "message")
minecraft.log("error", "message")
minecraft.log("message")  -- defaults to info
```

Messages appear in client log with mod id prefix.

---

## Read order

| Goal | Volumes |
|------|---------|
| Write first mod | I, II, III, IV |
| Sky/render mod | II, VI |
| GUI mod | II, VII |
| Generation mod | II, V |
| Learn from examples | IX |

---

*Next: [Volume II — Event Reference](02-events-reference.md)*
