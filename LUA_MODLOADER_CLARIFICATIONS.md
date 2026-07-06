# Lua Modloader Clarifications

This file records the user's clarifications for the modloader direction.

## Core Goal

- A compiled game build must load third-party mod zips from `%APPDATA%\.minecraft\mods`.
- A user downloading a mod zip from someone else should not need the mod source code.
- A user downloading a mod zip should not need to recompile the game or the mod.
- Textures/assets included in the mod zip should work from that zip.
- This must be a real runtime mod API, not a compile-time mod system disguised as one.

## Required Direction

- Convert the modloader to Lua scripted zip mods.
- LuaJIT or regular Lua is acceptable; the important part is no mod compilation requirement.
- Mod packages should be script-first, for example `mod.json` plus Lua files and resources.
- The runtime should execute Lua scripts from extracted/scanned mod zips.
- The game should provide Lua APIs for hooks, assets, and gameplay systems needed by mods.

## Old Native Plugin Path

- Remove the native DLL plugin path completely.
- Do not keep native plugin DLL loading as the real path underneath.
- Do not keep `runtime_mod_plugins` as the main mod authoring pipeline.
- Do not require `bin/<id>.dll` in mod zips.
- Delete old modloader files tied to native plugin loading, then rewrite the modloader around Lua.

## Conversion Scope

- This is intended as a full conversion, not a partial compatibility layer.
- Existing bundled mods should be converted to Lua rather than packaged as native DLLs.
- Build and packaging scripts should stop producing native plugin DLL zips.
- Documentation/rules should describe Lua zip mods only.

## Acceptance Shape

A successful result should let someone:

1. Build/download the compiled game.
2. Put a Lua mod zip in `%APPDATA%\.minecraft\mods`.
3. Launch the game.
4. Have the mod script and its included textures/assets load without recompiling anything.
