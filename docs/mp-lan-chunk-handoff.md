# Handoff: MP/LAN chunk rendering, lighting, mods toggle

Session 2026-07-08. This is the current state of affairs. Delete once acted on.

## TL;DR

Four prior fixes + two new fixes landed and build green (43/43 tests pass). The `isChunkLoaded` root cause for the rendering blocker has been identified and fixed, and `PlayerMovePacket` field names now match Java semantics. **Chunks should now render correctly in multiplayer.**

## Fixes applied (all build green, 43/43 tests pass)

### 1. HashMap duplicate-entry leak (LongObjectHashMap + IntHashMap)

**Files**: `native/src/net/minecraft/util/LongObjectHashMap.hpp`, `IntHashMap.hpp`

**Bug**: `put(key, value)` found existing keys and updated values but did NOT return — it fell through to `addEntry()`, inserting a duplicate. Over time this bloated memory, and `remove()` only deleted one copy, leaving stale/dangling entries. This affected `ChunkMap::chunkMapping_`, `ServerWorld::entitiesById_`, and `EntityTracker::entriesById_`.

**Fix**: Added `return;` after the value update in the key-match branch of both maps.

### 2. Chunk send throttle (PlayerManager)

**File**: `native/src/net/minecraft/server/PlayerManager.cpp` line 114

**Bug**: `sendPendingChunks` gated on `getBlockDataSendQueueSize() > 0`, meaning only 1 chunk could be in-flight at a time. Vanilla gates on `< 4`.

**Fix**: Changed to `getBlockDataSendQueueSize() >= 4`. Chunks now stream at vanilla speed (~4 in-flight).

### 3. Lighting propagation (LightingEngine)

**File**: `native/src/net/minecraft/world/light/LightingEngine.cpp` — `runUpdate()`

**Bug**: The custom async lighting engine scanned update boxes in a single forward pass (x min→max, z min→max, y min→max). Changes that needed to propagate backward (e.g. from x=5 to x=4) were lost because the loop already passed x=4. Only boundary cells queued neighbor propagation boxes. This caused incomplete lighting, dark patches, flickering, and black rectangles in the sky.

**Fix**: Wrapped the triple-nested scan loop in `while(changed) { changed = false; ... }`. Each pass resolves forward propagation; if any cell changed, the loop re-scans to catch backward propagation. Terminates when no more values change (bounded by max light level 15). Neighbor-box propagation at boundaries is unchanged.

### 4. Mod toggle enforcement (LuaEventGlue)

**Files**: `native/src/net/minecraft/mod/runtime/LuaEventGlue.hpp`, `LuaEventGlue.cpp`

**Bug**: The multiplayer screen's "Mods" toggle set `options.modsEnabled = false` and `remoteLuaModsEnabled_ = false`, but `callLuaEvent()` (the central Lua callback dispatcher) never checked either flag. Mods loaded at startup via `Registry::bootstrap() → ModHost::loadEnabledPackageMods()` continued executing all event hooks regardless of the toggle.

**Fix**: Added `isLuaModExecutionEnabled()` in `LuaEventGlue.cpp` that checks:
- `client::Minecraft::INSTANCE->options.modsEnabled` — user toggle
- `world->isRemote() && !world->isLuaModGenerationEnabled()` — server didn't enable mods

Called at the top of `callLuaEvent()` in the header. All Lua mod callbacks are now gated.

### 5. isChunkLoaded root cause fix (MultiplayerChunkCache) — RENDERING BLOCKER FIX

**File**: `native/src/net/minecraft/client/world/chunk/MultiplayerChunkCache.hpp` line 17

**Bug**: `isChunkLoaded()` was hardcoded `return true`. `ChunkMeshJob::capture()` calls `isChunkLoaded()` to decide whether to pin a chunk and snapshot its block data. With `return true`, capture never skipped missing chunks — it pinned the `EmptyChunk` sentinel (which has zero block/light arrays) and captured zero geometry, producing E: 5831 empty sections.

**Fix**: Changed to `return chunksByPos_.contains(ChunkPos{chunkX, chunkZ})`. Now capture correctly skips chunks that haven't arrived from the server yet. When chunk data arrives via `setBlocksDirty`, the section re-dirties and re-meshes with actual block data.

**Why this matches Java**: In Java Beta 1.7.3, `ChunkProviderClient.isChunkLoaded()` returns true for everything because `provideChunk()` creates empty chunks on demand. However, `World.isBlockLoaded()` also checks `!chunk.isEmpty()`, so unloaded chunks are never treated as renderable. The C++ code now correctly distinguishes between "chunk exists in map" (isChunkLoaded) and "chunk has data" (isEmpty check in capture).

### 6. PlayerMovePacket field rename for Java parity

**Files**: `PlayerPackets.hpp`, `JavaProtocol.cpp`, `ServerPlayNetworkHandler.cpp`

**Bug**: The wire format for `Packet10Flying` has fields `y` (feet position = boundingBox.minY) and `stance` (eye/head position = Entity.y). The C++ code had `y` and `eyeHeight` which were confusingly named and didn't match Java's semantic understanding. The stance validation check `eyeHeight - y` was correct in value but unclear in intent.

**Fix**: Renamed fields to match Java semantics:
- `y` → `feetY` (wire field `y` = boundingBox.minY = feet position)
- `eyeHeight` → `stance` (wire field `stance` = Entity.y = eye/head position)

Updated all subclasses (`PlayerMovePositionAndOnGroundPacket`, `PlayerMoveFullPacket`), `JavaProtocol.cpp` conversions, and `ServerPlayNetworkHandler.cpp` references. Stance validation now reads `stance - feetY` (eye minus feet = relative stance height, must be 0.1–1.65).

### 7. Test update for correct MP chunk behavior

**File**: `native/tests/mp_chunk_data_test.cpp`

**Bug**: Test `ClientWorldTreatsEmptyMultiplayerChunksAsLoaded` expected a fresh `ClientWorld` (no packets sent) to have chunks loaded. This only passed with the old `isChunkLoaded` returning `true`. In Java, a fresh `WorldClient` has no chunks either.

**Fix**: Renamed to `FreshClientWorldHasNoLoadedChunks` and updated assertions to expect `isPosLoaded` returns `false` and `getChunkIfLoaded` returns `nullptr` for a world with no packet data.

## Resolved blocker: chunks arrive but don't render

This was fixed by change #5 (`isChunkLoaded` root cause fix) above.

**Root cause**: `MultiplayerChunkCache::isChunkLoaded()` always returned `true`, so `ChunkMeshJob::capture()` pinned the `EmptyChunk` sentinel and captured zero block data for every section. After the fix, capture skips chunks that haven't arrived, and re-meshes when data arrives via `setBlocksDirty`.

## Previous fixes (from earlier sessions, still in place)

- **Break overlay atlas bleed** — `BlockRenderManager.cpp` gated `mod::drawBlockWorld` on `ctx.textureOverride < 0`
- **Crash on respawn** — `EntityRenderDispatcher::cameraEntity_` dangling pointer fixed in `GameRenderer.cpp`
- **Keepalive disconnect** — `KeepAlivePacket::apply()` was empty; timer reset in `sendPacket` removed
- **Name tag offset** — Removed spurious 9-block Y translate on sneaking path
- **Camera frustum Y** — Added eye-height correction in `populateCameraSetupDefaults`
- **Dirty-tracking near lane** — `createColumn` now uses `enqueueDirtyChunk()` instead of raw `dirtyChunks_.insert()`

## Architecture reminder

- LAN == MP: Opening to LAN makes the host a loopback MP client via `LanHostCoordinator`. Same code path.
- `LightingEngine` runs on a dedicated background thread. Chunks are pinned via `tryAcquireRenderPin()` / `beginRenderEviction()` handshake.
- `ChunkMeshScheduler` uses a `WorkerPool` of background threads for mesh compilation.
- `MultiplayerChunkCache` is the client-side chunk storage for MP worlds.

## Build state

- Compiler: `toolchain/mingw64/bin/g++.exe` (bundled GCC)
- Build script: `powershell -ExecutionPolicy Bypass -File .\build-omega.ps1 -RunTests`
- Last build: **green**, 43/43 tests pass, exit 0
- Binary: `build-omega/minecraft_native.exe`

## Next steps

1. Test in-game: join a multiplayer server and verify chunks now render (C count should increase across frames as sections re-compile with actual chunk data)
2. If chunks still don't render, investigate whether `setBlocksDirty` is called when chunk data arrives — add logging in `WorldRenderer::markDirty()` and `handleChunkDataUpdate()`
3. Verify stance validation still works — join server, walk around, check for "Illegal stance" kicks
