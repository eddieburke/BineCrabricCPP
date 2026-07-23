# Packages and lifecycle

## Runtime shape

The host creates `minecraft`, installs core, world, entity, block, and item
bindings, then client bindings where applicable. It evaluates the Lua prelude
after installation. See `src/net/minecraft/mod/ModHost.cpp:38-75`.

The prelude removes `io`, `debug`, `dofile`, `loadfile`, and native package
loaders. Modules therefore use `minecraft.require`, `minecraft.require_dir`, or
the guarded Lua fallback used by `mods/lib/core/init.lua:14-31`.

Client-only groups include camera, texture, model, sound, inventory, GUI, screen,
render, and raycast. A server-side mod must not require one of those namespaces
at load time.

## A normal mod lifecycle

1. Lua evaluates `config/init.lua` and the entry `scripts/main.lua`.
2. The entry imports sibling modules and registers callbacks with `minecraft.on`,
   `minecraft.event.register`, or a screen helper.
3. The host calls registered callbacks only while the mod is active; remote-world
   generation and client-mod enablement gate some callbacks.
4. A callback reads or changes the mutable fields documented for that event.

The bundled mods demonstrate several patterns, not one required layout:

- Small content mods register a block and recipe during load.
- Screen mods register a `screen_ui` footer or a Lua screen lifecycle handler.
- Generation mods subscribe to `chunk_generation` or `world_spawn_search`.
- Renderer mods use a client render hook and submit host render primitives.

## Shared library contracts

`mods/lib/core/init.lua` is the common loader, event bus, pooling, vector,
spatial-hash, math, and timer utility. Its event bus is internal: registering a
callback there alone does not subscribe it to the host. The standard template
uses this internal bus without adding a host bridge, so its normal gameplay loop
is inert unless another integration drives it.

`mods/lib/settings/init.lua` creates a cached proxy per mod id. The first call to
`settings.define` wins; later schemas for the same mod are ignored. Native UI
persistence occurs only with `native_ui=true`.

## File loading notes

Several modules are intentionally standalone rather than entry-point imports:

- `realtime_sky/astronomy/calculator.lua`, `rendering/skybox.lua`, and
  `rendering/celestial.lua` export useful helpers but the current realtime-sky
  entry does not load them.
- `lib/audio`, `lib/ui`, and `lib/rendering` require LÖVE globals and are used by
  the template rather than by a host callback.
- `item_drop_physics/physics/water.lua` uses a Minetest API surface, not the
  sibling `minecraft` runtime. See the limitations chapter before integrating it.
