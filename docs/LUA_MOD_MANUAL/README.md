# Lua Mod Manual

Authoritative contract for runtime Lua mods. Source of truth for behavior is also `native/src/net/minecraft/mod/lua/LuaRuntimePrelude.hpp`; shipped examples are under `native/mods/`.

| Chapter | Topic |
|---------|--------|
| [01](01-introduction-and-packages.md) | Packages, sandbox, loading |
| [02](02-events-reference.md) | Events and `minecraft.on` |
| [03](03-api-functions.md) | Core tables |
| [04](04-registration.md) | Blocks, items, recipes, mod settings/keybinds |
| [05](05-world-and-generation.md) | World, chunk, spawn |
| [06](06-rendering.md) | World render, camera, FBO |
| [07](07-gui-and-screens.md) | Screens and 3D GUI |
| [08](08-inventory-audio-utilities.md) | Inventory, sound, util |
| [09](09-mod-gallery.md) | Shipped mods |
| [10](10-json-model-format.md) | JSON model format (elements, faces, cullface, rotation, parent chains) |

**Breaking change:** `minecraft.on(event, callback)` is removed. Always pass an options table: `minecraft.on(event, {}, callback)` or with filters.
