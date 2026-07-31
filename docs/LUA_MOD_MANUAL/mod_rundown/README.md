# Legacy Mod Rundown & Source Catalogue

> [!NOTE]
> This folder contains the legacy file-by-file catalogue and mod-specific trace manual. For the formal C++ Lua engine API documentation, see the main [API Reference Manual](../README.md).

## Read this first

- [Packages and lifecycle](01-introduction-and-packages.md)
- [Events and execution phases](02-events-reference.md)
- [Runtime API](03-api-functions.md)
- [Registration and settings](04-registration.md)
- [World profiles and generation](05-world-and-generation.md)
- [Rendering, sky, and simulation](06-rendering.md)
- [Screens, inventories, audio, and utilities](07-gui-and-screens.md)
- [Integrated mod catalogue](08-inventory-audio-utilities.md)
- [File-by-file source coverage](09-mod-gallery.md)
- [Data formats and known limitations](10-json-model-format.md)

## Documentation conventions

`Client only` means a namespace is installed only in a client runtime. `Declared`
means a field is exposed through `lib.settings`; `consumed` means this source
actually reads it during execution. Source locations are repository-relative and
are intended to make every statement traceable.

## Coverage

The catalogue covers all 78 Lua files in `native/mods`. The runtime chapters are
also traced against 62 Lua-facing C++ binding files in `native/src/net/minecraft/mod`.
The source was read in full before this manual was written.
