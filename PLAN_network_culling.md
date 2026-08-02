# Plan: Network join stabilization + culling system rewrite

> Core principle (user): **timed budgets are opt-in.** Nothing should be time-boxed
> except connections to a Java multiplayer server or a hosted server. Singleplayer
> paths (integrated-server loopback, local rendering, mesh compilation) should run
> unbudgeted and drain everything.

## Symptoms

1. `ChunkPacketDrainThroughput.TickDrainsChunkPacketsUnderBacklog` fails: only 12/50
   chunk packets ever applied; `disconnect.overflow` kills the connection.
2. `MultiplayerChunkDelivery.*` crashes: `std::deque<std::unique_ptr<Packet>>::back()`
   assertion on an empty deque, right after "Can't keep up!" + `disconnect.endOfStream`.
3. F3 "Chunks drawn: X of Y" climbs without bound in singleplayer
   (observed `3279 of 10953`, which exceeds the theoretical max for the default radius).
4. The frustum/occlusion culling system is monolithic, O(n)-per-frame, and tangled with
   debug counters.

## Root causes (with references)

### R1 — Test failure: 1MB read-queue overflow, not throughput

`kMaxReadQueueBytes = 0x100000` (1MB) — `src/net/minecraft/network/Connection.cpp:15`.

- Each chunk packet is ~80KB decompressed and ~80KB on the wire (`ChunkPackets.hpp:95-97`
  returns `17 + chunkDataSize`).
- The test sends 50 packets (~4MB) without ticking during the 5s sleep
  (`tests/chunk_packet_drain_test.cpp:141-156`).
- The read thread fills `readQueue_` to 1MB at ~12 packets, sets `readOverflow_`, and
  breaks (`Connection.cpp:316-329`).
- The next `tick()` calls `requestDisconnect("disconnect.overflow")`
  (`Connection.cpp:201-203`). The connection is dead; only 12 packets ever apply.

### R2 — Hardcoded drain budget throttles everything

`Connection.cpp:214-219`: `kMinDrain = 8`, `kMaxDrain = 4096`, `kDrainBudget = 3ms`
applied to every connection, every tick. In Debug (-O0) builds, 3ms covers only ~8-15
chunk applies, so a real join with a chunk burst lags — exactly the symptom the test
tries to measure. Singleplayer loopback connections don't need this protection at all.

### R3 — `deque::back()` crash: teardown race / heap corruption

Triggered in `tests/mp_chunk_delivery_test.cpp` after the server logs "Can't keep up!"
and drops the client. The only `std::deque<std::unique_ptr<net::minecraft::Packet>>`
instances are `readQueue_`, `sendQueue_`, `delayedSendQueue_` (`Connection.hpp:118-120`),
and no `.back()` call exists in source — so this is a lifetime race (reader/writer thread
touching the Connection during destruction / `server.stopAndJoin()`) or heap corruption
from a use-after-free. Needs an ASan run to pin down.

### R4 — "Chunks drawn" climbs forever

- `chunkCount = sectionList_.size()` is recomputed every frame when the debug HUD is on
  (`WorldRenderer.cpp:273`).
- Default `chunkRadius = 16` (via `RenderSettings.cpp:28-30` with default viewDistance)
  caps sections at `33^2 * 8 = 8712`; observed 10953 exceeds that → sections accumulate
  beyond the frontier. Suspects:
  - `renderScale > 1` inflating `visualGridDiameter` (`RenderSettings.cpp:24-30`).
  - Frontier jitter at section boundaries in `updateSectionFrontier`
    (`WorldRenderer.cpp:365-438`).
  - A section that never frees: `retireOrFreeSection` defers via `retiring_` when
    `meshJobInFlight` is set (`WorldRenderer.cpp:239-251`); if a job never drains,
    `sweepRetiring` never frees it.
- `compiledChunkCount` counts *in-frustum non-empty sections*, not *actually drawn*
  sections. If the frustum test degrades to "always visible", the number tracks world
  load and reads as unbounded.

### R5 — Monolithic culling

`cullChunks` + `applyOcclusionCulling` + `rebuildVisibleDrawRings`
(`WorldRenderer.cpp:947-1076`) do full O(n) scans of `sectionList_` and a full BFS
flood-fill every frame. Frustum flags, occlusion stamps, ring lists, and debug counters
are all entangled in `WorldRenderer`. No spatial index, no occlusion-map caching, no
separation of concerns.

---

## Phase 0 — Stabilize the network/join path (test + crash)

1. **Unbudgeted drain by default.** In `Connection::tick()`, apply the time-box
   (`kMinDrain`/3ms) only when `externalDrainLimit_` is set or an explicit frame
   deadline is active. Default path drains up to `kMaxDrain` with no wall-clock cap.
   Java-MP/server code calls `setDrainLimit()` explicitly when it wants protection.
   - Files: `src/net/minecraft/network/Connection.cpp`, `Connection.hpp`.
   - Callers to audit for `setDrainLimit()`: Java-MP client join, dedicated/LAN host.

2. **Overflow becomes backpressure, not disconnect.** Reader pauses when `readQueue_`
   crosses a high-water mark, resumes once `tick()` drains below a low-water mark.
   Raise the hard cap (e.g. 32MB) so a 4MB chunk burst during join never kills the
   connection. Remove the `readOverflow_` disconnect path.
   - Files: `src/net/minecraft/network/Connection.cpp` (`readLoop`, `tick`).

3. **Fix the test to mirror the real async client.** Tick `clientConnection` during the
   send/parse phase (exactly what `ClientNetworkHandler::tick()` does every frame —
   `src/net/minecraft/client/multiplayer/ClientNetworkHandler.cpp:80-91`) so the queue
   drains as packets arrive. Keep the measurement loop and the final `ASSERT_EQ`.
   - File: `tests/chunk_packet_drain_test.cpp`.

4. **Harden teardown.** Run the mp tests under ASan/Valgrind. Audit
   `joinThreads`/`requestDisconnect`/writer `sendAll` against Connection destruction;
   the writer thread must never touch `output_` after the socket is force-closed.
   - Files: `src/net/minecraft/network/Connection.cpp`, `tests/mp_chunk_delivery_test.cpp`.

### Phase 0 acceptance
- `chunk_packet_drain_test` passes in Debug and Release builds.
- `mp_chunk_delivery_test` (both tests) pass with no crash.
- `connection_packet_drain_throughput_test` still passes.
- No timing-based budget on singleplayer/integrated-server connections.

---

## Phase 1 — Fix the chunk counter + section lifecycle

5. **Stop section accumulation.** Cap sections strictly at `renderRadiusChunks_`; remove
   the frontier `+1` drift in `chunkAvailable`/`drainPendingColumns`. Verify
   `removeColumn` fires on every boundary cross. Add a watchdog so `sweepRetiring`
   force-frees sections whose mesh job never drains.
   - Files: `src/net/minecraft/client/render/world/WorldRenderer.cpp`,
     `WorldRenderer.hpp`.

6. **Accurate counters.** Split "compiled" (mesh uploaded) from "actually drawn" (draw
   calls issued this frame via `ChunkRegionBuffer`). Report: loaded / in-frustum /
   occlusion-culled / empty / actually-drawn / draw calls.
   - Files: `src/net/minecraft/client/render/world/WorldRenderer.cpp`
     (`getChunkDebugInfo`, `rebuildVisibleDrawRings`), `ChunkRegionBuffer.hpp`.

### Phase 1 acceptance
- F3 "Chunks drawn" plateaus at the render-distance max when standing still and tracks
  movement without unbounded growth.
- Counters reconcile: loaded = in-frustum + occlusion-culled + empty.

---

## Phase 2 — Culling rewrite (collapse + separate)

7. **Extract a `ChunkCuller` module.** Owns: frustum test per section (cullingBox,
   near-bypass, shadow render-distance cut), occlusion BFS, ring building, and the
   visible draw-list output. `WorldRenderer` keeps orchestration, VBO, and render only.
   - New files: `src/net/minecraft/client/render/culling/ChunkCuller.hpp/.cpp`.
   - Moves out of: `WorldRenderer.cpp:947-1076`, `ChunkBuilder::updateFrustum`,
     `Frustum.cpp`.

8. **Spatial acceleration.** Iterate by ring/column proximity instead of full
   `sectionList_` scans for frustum + occlusion. Reuse `sections_` hash + `drawRings_`
   as the index.

9. **Incremental occlusion.** Cache the flood-fill result keyed by camera-section + a
   dirty stamp. Re-flood only when the camera crosses a section boundary or a built
   section on the BFS frontier changes. This is the biggest per-frame win.
   - `ChunkBuilder.hpp`: replace `occStamp`/`occEntryFace` with a cached reachability
     bitmask per section (computed at build time, like `visBits`), so occlusion becomes
     a bitwise propagation, not a re-flood.

10. **Budgets opt-in everywhere.** Apply the Phase 0 policy to `compileChunks`/mesh
    upload/capture budgets (`WorldRenderer.cpp:695-823`): unlimited in singleplayer,
    budgeted only under an active frame deadline (Java MP/hosting).

### Phase 2 acceptance
- Culling results are identical to (or better than) today at a lower per-frame cost.
- Singleplayer render path has no frame-budget stalls.
- Draw calls / F3 counters stable and reconcilable.

---

## Verification strategy

- `build-omega.ps1` test suite: `packet_roundtrip`, `connection_packet_drain_throughput`,
  `chunk_packet_drain`, `mp_chunk_delivery`, `connection_async_teardown`,
  `thread_budget`, `region_snapshot_race`.
- ASan/UBSan build for Phase 0 crash repro.
- F3 debug HUD in singleplayer + LAN host + Java-MP join for counter/budget checks.
- Release (-O3) and Debug (-O0) both: the original test's purpose is to quantify the
  gap; after Phase 0 the gap should be small because singleplayer is unbudgeted.
