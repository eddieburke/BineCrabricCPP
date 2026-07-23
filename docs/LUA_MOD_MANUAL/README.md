# BineCrabricCPP Lua API Reference Manual

This manual provides a comprehensive, source-traced API specification for the C++ Lua modding runtime in **BineCrabricCPP** (Minecraft Beta 1.7.3 Native Port).

It documents all native C++ bindings (`LuaHostApi`, `LuaGameApi`, `LuaCoreBindings`, `LuaWorldBindings`, `LuaEntityBindings`, `LuaRenderBindings`, `LuaScreenBindings`, `LuaSoundBindings`, `LuaRaycastBindings`, `LuaDirectHooks`), Lua runtime prelude wrappers, event channels, content registries, and data codecs.

---

## Chapter Index

1. [Core & Lifecycle](01-core-and-lifecycle.md)
   - Runtime Environment & Lua 5.4 Sandbox (`os`, `package`, blocked libs)
   - Module Loading (`minecraft.require`, `minecraft.require_dir`)
   - Storage & Persistence (`minecraft.storage`, `minecraft.config`)
   - Mod Package Metadata (`mod.json`) & Execution Lifecycles (`init`, `post_init`, `ready`)
   - Core Utilities (`minecraft.util.*`)

2. [Event System & Engine Hooks](02-event-system.md)
   - Subscription Model (`minecraft.on`, `minecraft.event.register`)
   - Event Priority, Cancellation, and Property Matching
   - Complete Catalog of 30+ Engine Events (Tick, World, Entity, Render, Screen, Generation, FOV, Raycast, Input)

3. [Registries & Content Definition](03-registries-and-content.md)
   - Custom Block Registration (`minecraft.register_block`)
   - Custom Item Registration (`minecraft.register_item`)
   - Crafting Recipe Registration (`minecraft.register_shaped_recipe`)
   - Custom Lua Mod Entities (`LuaModEntity` state, tick, bounding boxes)

4. [World, Generation & Entities](04-world-and-entities.md)
   - World Query & Mutation API (`minecraft.world.*`)
   - Procedural Generation Pipeline (`minecraft.generation`: stages & moments)
   - Entity Query & Manipulation API (`minecraft.entity.*`)
   - Raycasting Engine (`minecraft.raycast`)

5. [Rendering, Models & Camera](05-rendering-and-models.md)
   - Render Pipeline & Pass Hooks (`minecraft.render.*`: stages & moments)
   - Model Baking & Voxel Builders (`minecraft.model.build`, `minecraft.model.voxels`, `minecraft.model.voxel`)
   - Texture Metadata & Pixel Inspection (`minecraft.texture.*`)
   - Camera Manipulation (`minecraft.camera.*`) & Sky / Fog Color Systems

6. [GUI Screens & Audio](06-gui-and-audio.md)
   - Screen Injection & Event System (`minecraft.screen.*`)
   - GUI Rendering Primitives (`minecraft.gui.*`)
   - Mod Settings Screen Builder (`minecraft.screen.settings`)
   - Audio Engine (`minecraft.audio.*`, positional sound, music channels)

---

## Global Namespaces

- **`minecraft`**: Primary root namespace exposing all engine sub-systems and functions.
- **`minetest`**: Global alias bound directly to `minecraft` for cross-engine compatibility.

---

## Legacy Documentation & Mod Rundown

The previous file-by-file catalogue and mod-specific trace documentation can be found in the [mod_rundown/](mod_rundown/README.md) subfolder.
