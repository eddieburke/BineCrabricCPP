# Server port fix coordination (feature/server-port)

## Known failure clusters
| ID | Symptom | Suspected root | Owner |
|----|---------|----------------|-------|
| F1 | undefined ref core symbols when linking client+core | CMake `--start-group` placement | parent (fixed) |
| F2 | ServerLoginNetworkHandler uses `client::resource::httpRequest` | wrong layer coupling | F2 |
| F3 | `\xC2\xA7` hex escape warnings in ServerPlayNetworkHandler | invalid C string escapes | F3 (fixed) |

### F3 findings (fixed)
- **Root cause:** `\xA77` / `\xA7e` etc. — hex escape consumes extra digit (`A77` out of range).
- **Pattern:** adjacent string literals `"\xC2\xA7" "7"` (UTF-8 § + Minecraft color code); compile-time concat, MinGW-safe.
- **Files:** `ServerPlayNetworkHandler.cpp` (5), `ServerCommandHandler.cpp` (3 `kColor*` constexpr).
- **Checked, no hits:** `DedicatedServerGui`, `PlayerManager`, rest of `native/src/net/minecraft/server/`.
| F4 | ~~`World::worldEvent` missing; tryBreakBlock substitute~~ | **fixed** — `World::worldEvent` fans out via `WorldEvents`; `tryBreakBlock` calls `worldEvent(2001, …)` | F4 |
| F5 | Server tests fail compile/link | test fixtures, EXPECT_FALSE | F5 — done |
| F6 | ~~`BlockEntity::createUpdatePacket` only Sign~~ | ChunkMap blockers | F6 (fixed) |

### F6 findings (fixed)
- Java `BlockEntity.createUpdatePacket()` base returns `null`; only `SignBlockEntity` overrides (Beta 1.7.3 — furnace/chest/jukebox/dispenser/spawner/note/piston do not).
- Native: added virtual `BlockEntity::createUpdatePacket()` → `nullptr`; `SignBlockEntity` override → `UpdateSignPacket`.
- `ChunkMap::sendBlockEntityUpdate` now dispatches via base virtual (removed `SignBlockEntity` `dynamic_cast`).
- **Emitting update packets:** `Sign` only.
| F7 | Server should not need full client+GLFW | slim server link | F7 (fixed) |

### F7 findings (fixed)

**Dependency trace (server-main → …):**
- `server-main` → `MinecraftServer` → `ConnectionListener` → `ServerLoginNetworkHandler` (HTTP via `util/http/HttpClient`, not client — F2 done)
- `PlayerManager` → `ChunkMap` (core only)
- Win32: `DedicatedServerGui` (core `server/dedicated/gui/`, `comctl32`/`gdi32`/`user32` only)
- `LoadingDisplay` — header-only virtual progress API (no client TU)
- Lua: `Registry::bootstrap` → `ModHost` (client GL paths gated by `MINECRAFT_NATIVE_EXPORTS`; server does not define it)
- Residual core→client **link** edges (not include-only): `Block::getName` → `I18n` → `TranslationStorage::get`; `PlayerEntity::updateCapeUrl` → `msauth::legacyCapeUrl`; `ModTexture::glId`/`bind` (weak-stubbed in core; strong in client `ModTextureGl.cpp`); `LuaBlockModel` draw fns (weak in core, strong in client `LuaBlockModelDraw.cpp`)

**Approach: hybrid B + C** (not full `minecraft_client` OBJECT lib):
1. **`minecraft_server_support`** OBJECT lib — 2 client TUs: `TranslationStorage.cpp`, `PlayerTextureUrls.cpp`
2. **`minecraft_link_server()`** — `minecraft_core` + support objects + zlib + platform + lua runtime + Win32 GUI libs; **no** GLFW, audio, `opengl32`/`glu32`
3. Small source splits so server never needs the 185-file client OBJECT closure (weak/strong draw + `ModTextureGl`)

**Link graph**

Before:
```
minecraft_server.exe
  ├─ server-main.cpp
  ├─ $<TARGET_OBJECTS:minecraft_client>   # ~185 client TUs (GLFW, all renderers, …)
  ├─ minecraft_core.a
  ├─ minecraft_link_audio / minecraft_link_display (glfw)
  └─ opengl32, glu32, comctl32, gdi32, user32, …
```

After:
```
minecraft_server.exe
  ├─ server-main.cpp
  ├─ $<TARGET_OBJECTS:minecraft_server_support>   # 2 TUs
  ├─ minecraft_core.a
  └─ minecraft_link_server → zlib, ws2/winhttp/…, lua54.dll, comctl32/gdi32/user32

minecraft_native.exe (unchanged intent)
  ├─ main.cpp + $<TARGET_OBJECTS:minecraft_client>
  ├─ minecraft_core.a
  └─ audio + glfw + opengl32 + …
```

**Build:** `minecraft_server.exe` links clean on branch. `minecraft_native` GLFW `Window.cpp` missing from client glob is a **pre-existing** branch issue (not introduced by F7).

Agents: append findings under your ID. Run `format-omega.ps1` after edits. Parent runs `build-omega.ps1`.

### F5 — server test suite
- Added `tests/support/server_test_macros.hpp` (`EXPECT_TRUE`/`EXPECT_FALSE`/`EXPECT_EQ`, `RUN_SERVER_TEST`).
- Added `tests/support/server_event_fixture.hpp` — registers `worlds[0]`, calls `configureFromProperties()` before `markDirty`/`blockUpdate`.
- `server_world_events_test.cpp`: `ServerPlayerInteractionManager(&world)` not `&server`; `ServerPlayerEntity(server, world, name, &mgr)`; `ItemEntity` for tracker parity; worldEvent exclusion checks eligible recipient count.
- `server_properties_test.cpp`: `EXPECT_FALSE` on disk-reloaded `white-list=false`.
- CMake (F5): tests used `minecraft_link_core_client()` + full `minecraft_client` objects.
- CMake (F7): tests now use `minecraft_link_server()` + `minecraft_server_support` (same slim link as `minecraft_server.exe`).

## F4 findings

**Java call chain (block break FX):**
1. `ServerPlayerInteractionManager.tryBreakBlock` → `world.worldEvent(player, 2001, x, y, z, blockId + meta * 256)`
2. `World.worldEvent(player, …)` → iterates `eventListeners`
3. `ServerWorldEventListener.worldEvent` → `PlayerManager.sendToAround(…, WorldEventS2CPacket)`
4. Client `ClientNetworkHandler` packet handler → `world.worldEvent(eventId, x, y, z, data)` (no player)
5. `WorldRenderer.worldEvent` case `2001` → break sound + `addBlockBreakParticles`

**Native equivalent (after fix):**
1. `ServerPlayerInteractionManager::tryBreakBlock` → `world->worldEvent(player, 2001, x, y, z, blockId + meta * 256)`
2. `World::worldEvent` → `WorldEvents::worldEvent` → each `GameEventListener::worldEvent`
3. `ServerWorldEventListener::worldEvent` → `PlayerManager::sendToAround` + `WorldEventS2CPacket`
4. `ClientNetworkHandler::onWorldEvent` case `2001` → `block::sounds::playBreak` + `spawnBlockBreakParticles`

**Removed:** server-side direct `playBreak` / `spawnBlockBreakParticles` in `tryBreakBlock` (those belong on client via S2C packet, matching Java).
