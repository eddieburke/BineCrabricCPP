# Minecraft Beta 1.7.3 — Lua Mod Programming Manual

**Document set MC-LUA-API-173 Rev 2.0**

This manual is derived from the **authoritative engine sources**, not from summaries:

| Source | Contents |
|--------|----------|
| `native/src/net/minecraft/mod/lua/LuaRuntimePrelude.hpp` | Prelude, `minecraft.on`, helpers |
| `native/src/net/minecraft/mod/runtime/LuaCoreBindings.cpp` | Core API, keys, options, assets |
| `native/src/net/minecraft/mod/runtime/LuaWorldBindings.cpp` | Blocks, world, chunk, particles |
| `native/src/net/minecraft/mod/runtime/LuaEventSubscribers.cpp` | All 21 hook events |
| `native/src/net/minecraft/mod/runtime/LuaScreenBindings.cpp` | GUI + screens |
| `native/src/net/minecraft/mod/runtime/LuaGuiViewport.cpp` | 3D viewport on Lua screens |
| `native/src/net/minecraft/mod/runtime/LuaRenderBindings.cpp` | World render batches |
| `native/src/net/minecraft/mod/runtime/LuaInventoryBindings.cpp` | Inventory |
| `native/src/net/minecraft/mod/runtime/LuaSoundBindings.cpp` | Audio |
| `native/src/net/minecraft/mod/runtime/LuaItemBindings.cpp` | Items |
| `native/src/net/minecraft/mod/lua/LuaModApi.cpp` | Registry, sample_grid, files, textures |

## Volumes

| Vol | File | Subject |
|-----|------|---------|
| I | [01-introduction-and-packages.md](01-introduction-and-packages.md) | Philosophy, zip layout, bootstrap order, sandbox |
| II | [02-events-reference.md](02-events-reference.md) | **All 21 events**, every field, mutability, filters |
| III | [03-api-functions.md](03-api-functions.md) | **Every** `minecraft.*` function with signatures |
| IV | [04-registration.md](04-registration.md) | Blocks, items, recipes (startup + runtime) |
| V | [05-world-and-generation.md](05-world-and-generation.md) | World, chunk, sample_grid, world lifecycle |
| VI | [06-rendering.md](06-rendering.md) | world_render, world_color, quads, billboards, tessellator |
| VII | [07-gui-and-screens.md](07-gui-and-screens.md) | GUI draw, Lua screens, screen_ui, regions |
| VIII | [08-inventory-audio-utilities.md](08-inventory-audio-utilities.md) | stack, inventory, sound, config, astronomy |
| IX | [09-mod-gallery.md](09-mod-gallery.md) | All shipped mods with line-by-line walkthroughs |

Read **Volume II** and **Volume III** first if you are implementing a mod. Read **Volume IX** to learn by example.

---

*“The hook is mightier than the patch.”*
