# PLAN-INITIAL — Coordinated Threading / Main-Thread Restructure (First Draft)

Status: initial planner deliverable (review-only; NO edits, NO builds were made).
Pipeline stage: initial planner. Inputs: RULES FOR AGENTS.md, CONTEXT.md (MISSING — see §7),
six council docs. Line numbers below were re-verified against the working tree on 2026-08-01
where the file was opened; where a line was taken from council notes without re-reading, it is
marked `[council]`.

Authoritative build/test entrypoints (RULES §2, §7): only the compile-fixer stage runs
`.\build-omega.ps1` and `.\build-omega.ps1 -RunTests` (ctest over `minecraft_omega_tests`).
Every work item below must be **build-safe and partial-landable**: it must compile on its own
(with the compile-fixer as the only builder), keep every currently-passing test green, and not
depend on later items.

---

## 0. Task statement and goal

"Massive refactor of multithreading + main-thread handling." Today ~9 independent thread-creation
sites each compute their own thread count from `hardware_concurrency()` (mostly via
`WorkerPool::recommendedThreadCount`, WorkerPool.hpp:61-68), each subsystem owns its own queue,
budget, and teardown, and `Minecraft::run()` is one monolithic loop with per-frame slices given to
4 worker backends. Functional parity targets: **Java Beta 1.7.3** (single-thread game loop:
tick→render→present on the Client thread; per-connection reader/writer socket threads; sim-thread
packet apply) and **Java Iris 26.1** (render-thread-only GL, exact per-frame phase order, BufferFlipper
stage semantics). The refactor must preserve both.

---

## 1. Target architecture (from council-architecture-proposal.md)

1. **`ThreadCoordinator` singleton** owns ONE global thread budget computed once
   (`globalBudget = max(1, hardwareThreads - reserved)`, reserved defaults 2). All thread-count
   decisions go through `coordinator.pool(Domain::X)`; `WorkerPool::recommendedThreadCount` is
   **deleted**. Configured in `Minecraft::init()` and `server-main.cpp`.
2. **Small fixed set of domain pools sharing the one budget**, NOT one giant "everything" pool
   (three roles cannot be task-scheduled onto a generic pool): shader compile needs GL-shared
   contexts; audio/logging are API-affine; blocking socket I/O must never occupy a CPU-tessellation
   thread. Domains: `Compute` (mesh+lighting+chunk gen), `Io` (blocking file/save + HTTP one-shots),
   `GlCompile` (2–3, own shared contexts), pinned classes (audio, logging, network, dir watcher)
   registered and budget-capped.
3. **Main thread = the GL thread, never split.** Loop stays `tick → render → present` on one thread.
   Iris phase order and BufferFlipper semantics preserved byte-for-byte. "Defer GPU upload to the GL
   thread" = defer to the render phase of the main thread via channels.
4. **`Channel<T>`** (bounded, prioritized, `stop_token`-aware, `push/tryPush/tryPop/drain/reset`,
   version-stamped) is the canonical produce-on-worker/consume-on-main handoff. Replaces
   `WorkerHandoff::completed_`, `LightingEngine::outbox_`, and (in time) `Connection`'s mutex-deques.
5. **`Minecraft::run()` becomes an explicit phase pipeline** (`FramePipeline`): DRAIN mailbox →
   INPUT+UI TICK → timer + N world ticks → RENDER (drain results/upload/draw/present) → PACE →
   DIAGNOSTICS. The tick-order invariant (`Minecraft.cpp:647-649`) and render order are preserved
   verbatim; only *where/when* per-frame drains happen changes. One shared per-frame `FrameBudget`
   deadline replaces five per-subsystem `fromMs(...)` budgets.
6. **`Lifecycle` shutdown coordinator**: ordered reverse-of-creation teardown, unblock-then-join,
   watchdogged joins (2–5 s then log-and-leak). Route `Minecraft::stop()` and crash handlers
   through it. GL-shared-context teardown (ShaderCompileService::stop) before primary context destroy.
7. **Migration order (bottom-up, build-safe)**: infrastructure → kill `recommendedThreadCount` →
   compute/chunk coordination → shader → network → ephemeral threads → main loop → teardown → server.

---

## 2. NEVER-PARALLELIZE / must stay on the main GL thread

These are hard invariants (council-java-thread-model §6 "MUST NOT BE PARALLELIZED", §3; 
council-architecture-proposal §5.1; council-concurrency-inventory §10.7). Executors must not move
any of these off the main thread:

1. **Every OpenGL call** — create/delete/upload/link/bind/draw/uniform. `IrisRenderSystem`
   (`third_party/mcp/iris/gl/IrisRenderSystem.java:42`) asserts render-thread for all 48 GL entry
   points; `ProgramBuilder` (`gl/program/ProgramBuilder.java:36,72`) asserts render-thread for every
   program build. This includes: mesh `ChunkRegionBuffer::upload`/`glBufferSubData`
   (`ChunkRegionBuffer.cpp:74-111`), `ChunkBuilder::uploadMesh` (`ChunkBuilder.cpp:352-415`),
   `ProgramCache` linking, texture upload (`TextureManager::tick`, `getTextureId`), present/swap,
   FBO/render-target creation, `glDispatchCompute`.
2. **Shader compile/link for live objects** — only the `GlCompile` domain workers may compile, and
   only on their own hidden GLFW windows sharing the main context (`ShaderCompileService.cpp:41-61`,
   `createWorkerWindow` :8-19). **Blobs cross threads; live `GLuint`s never do.** Main thread applies
   binaries. (This is the *allowed* exception — it stays a dedicated GL-context-affine pool.)
3. **Per-frame flip/`flipped` snapshot evaluation and the render order** — serial dependency chain
   (`BufferFlipper`, `flippedBeforeShadow/flippedAfterPrepare/flippedAfterTranslucent`, BEGIN-reads-
   last-frame-shadow). Do not move the shadow map earlier "to fix it".
4. **Frame-global monotonic state consumed by shaderpacks** — frame counter, `SystemTimeUniforms`,
   texture reload counter, `tickDelta`/`partialTick`. One `partialTick` value shared by world
   interpolation and all Iris uniforms per frame.
5. **`PipelineManager::destroy → immediately re-prepare` with 16-unit unbind** — atomic on the
   render thread (`third_party/mcp/iris/pipeline/PipelineManager.java:83-112`).
6. **Packet apply (all `handle*`/`on*` dispatch)** — must run on the sim thread (client main,
   server tick thread) exactly as today. Socket threads only decode/push; never apply
   (`Connection.cpp:184-228`). This is Java parity (`NetworkManager.java:191-218`).
7. **World simulation state mutation** — `World`/entities/block entities/`WorldEvents` dispatch,
   chunk decorate (`ChunkCache::adoptChunk`/`decorate`, `ChunkCache.cpp:156-201,373-384`),
   `setBlock`, heightmaps, block entities. Workers never dereference live world state; they operate
   only on `RegionSnapshot` (and record block-entity *positions*, `ChunkMeshJob.hpp:27`, resolved on
   main at upload `ChunkBuilder.cpp:378-386`).
8. **`WorkerHandoff`/`ChunkMeshJob` completion queue + `meshJobInFlight`** — main-thread-only
   consumer; `~ChunkMeshJob` must never drop the last `shared_ptr` on a worker (R3 invariant,
   `ChunkBuilder.cpp:220-225`). If moved to a `Channel`, the consumer side stays the render phase.
9. **`Timer` / tick cadence** — do not split tick and render onto different threads; cap N at 10;
   paused-partialTick freeze stays (`Minecraft.cpp:788-791`, `Timer.hpp:14-48`).
10. **`Minecraft::stop()` GL/context ordering** — `ShaderCompileService::stop()` (Minecraft.cpp:354)
    before `DisplayManager::destroy()`; world/workers stopped before contexts destroyed
    (architecture §6.5).
11. **Per-connection packet write ordering and sim-thread drain ordering** — writer stays serial per
    socket; drain order preserved (FIFO, no reordering).
12. **GL state writes incl. `g_alphaTestRef`** — must be confined to the main GL thread. Today mod
    block world draws on mesh WORKERS call `core::setAlphaTestRef(0.1f)` (`ModModels.cpp:626`) →
    global `g_alphaTestRef` (`RenderCore.cpp:70,671-680`) read on main for uniform upload
    (`RenderCore.cpp:479-481`). Fix by capturing alphaTestRef into the job/snapshot, NOT by writing
    the global from workers. Same for `GLCore::init()` `g_loaded` plain bool (`GLCore.cpp:121,145`)
    written by main + shader workers — make it `std::call_once`/atomic under the first context.

---

## 3. Concrete work items (ordered, build-safe)

Naming: `WI-n` items are additive/mechanical where possible; each ends with a Verification
subsection. All paths relative to repo root. Executors must re-verify each `file:line` immediately
before editing (line numbers drift).

### PHASE A — Coordination infrastructure (pure additive)

#### WI-1 — Add `ThreadCoordinator`, `ThreadBudget`, `Channel<T>`, `ThreadNames`, `Lifecycle`

- Objective: create the one authority for thread counts, the canonical handoff, thread naming, and
  ordered teardown. Configure the coordinator but wire nothing to it yet (build-safe).
- New files (mirroring architecture §0): `src/net/minecraft/util/concurrent/ThreadCoordinator.hpp/.cpp`,
  `src/net/minecraft/util/concurrent/ThreadBudget.hpp`,
  `src/net/minecraft/util/concurrent/Channel.hpp`,
  `src/net/minecraft/util/concurrent/ThreadNames.hpp`,
  `src/net/minecraft/util/concurrent/Lifecycle.hpp/.cpp`.
- API to land (architecture §3.2): `ThreadCoordinator::instance()`, `configure(hw, reserved, opts)`,
  `pool(Domain)`, `budget()`, `reserveDynamic(n)/releaseDynamic(n)`, `totalPending()`, `shutdown()`;
  `enum class Domain { Compute, Io, GlCompile, Audio, NetIo, Log }`; `TaskPriority
  { Urgent, High, Normal, Low, Idle }`. `Channel<T>` bounded + prioritized + stop-aware with
  `push/tryPush/tryPop/drain/reset/size`.
- Change to existing files: **none** — except `Minecraft::init()` (`Minecraft.cpp:270-340`) calls
  `ThreadCoordinator::instance().configure(...)`; `src/server-main.cpp:125-126` (optional) same. No
  other call site changes.
- Risks: none functional (dead code until WI-2+). Keep `Channel` simple (mutex+CV+deque, NOT
  lock-free — architecture §8.7 deliberately excludes an SPSC queue).
- Parity: no behavior change.
- Verification: compile (`.\build-omega.ps1`). Add unit tests in `tests/` (GoogleTest,
  auto-discovered via `gtest_discover_tests`, CMakeLists.txt:403): `tests/channel_test.cpp`
  (push/tryPush at capacity blocks producer, tryPop ordering, priority Urgent-first, `reset()` wakes
  blocked producers, stop_token push returns false), `tests/thread_budget_test.cpp` (budget math:
  compute = clamp(remaining,1,8), io = clamp(2, remaining/4, 3), pinned classes deducted first),
  `tests/lifecycle_test.cpp` (unblock-then-request_stop-then-join ordering; watchdog leak path).
  Existing tests must remain green (they don't touch coordinator yet).

#### WI-2 — Delete `recommendedThreadCount`; route all counts through the coordinator

- Objective: remove the oversubscription root cause (rank #1). One authority for "how many threads".
- Files to touch:
  - `src/net/minecraft/util/concurrent/WorkerPool.hpp:61-68` — **delete** `recommendedThreadCount`.
  - `src/net/minecraft/client/render/chunk/ChunkBuilder.hpp:156` — `handoff_` count →
    `ThreadCoordinator::instance().pool(Domain::Compute).threadCount()`.
  - `src/net/minecraft/world/light/LightingEngine.cpp:49` — `workers` → Compute-domain count.
  - `src/net/minecraft/world/chunk/ChunkCache.cpp:217` (loader → Compute), `:224` (save → `Domain::Io`
    count, 1–2).
  - `src/net/minecraft/client/gl/ShaderCompileService.cpp:47` → `Domain::GlCompile` count (cap 2–3).
  - `src/net/minecraft/client/gl/GLCore.cpp:289` (driver hint) → `budget().glDriverThreads()`.
- Change: mechanical constant swap; on an 8-core machine counts land at mesh2/light2/loader2/save1/
  shader2 — total worker threads drop from ~11 to ~7-8, and everything comes from one table.
- Risks: a subsystem with `pool(Domain::X)` now shares workers with siblings — must ensure no
  per-pool state assumptions (e.g. LightingEngine's per-worker `WorkerState` array, `LightingEngine.cpp:50-56`,
  is sized from the count — it must use `pool(Compute).threadCount()` as the *maximum* concurrent
  lighting workers, not spawn its own jthreads; see WI-6). Do **not** switch pools in this item;
  only change *count sources* and keep each owner's existing pool object so behavior is identical.
- Parity: none (thread counts only). Must keep `targetInFlight = workerCount*3/6` semantics in
  `WorldRenderer::compileChunks` (`WorldRenderer.cpp:780`) unchanged.
- Verification: compile + run full ctest. Add `tests/thread_coordinator_test.cpp` asserting
  `budget().compute` derived from `hardware_concurrency()` and that no site can reach
  `recommendedThreadCount` (compile-time: function deleted). Watch `tests/chunk_mesh_golden_test.cpp`
  (meshing determinism — workers must not change mesh output).

### PHASE B — Chunk/compute coordination and correctness

#### WI-3 — Close the light-array snapshot race (R1/R4)

- Objective: make `RegionSnapshot` copy safe against lighting workers + main-thread writers. This is
  the port's one genuine UB data race (`RegionSnapshot.cpp:54-66` memcpy of live `chunk.skyLight/
  blockLight/blocks/meta` while `LightingEngine::setBrightness`→`Chunk::setLight` writes
  `LightingEngine.cpp:249-258` and main `Chunk::setBlock`/decorate/populateBlockLight write).
- Files:
  - `src/net/minecraft/world/chunk/Chunk.hpp:133-140` (`setLight`) + add a per-chunk
    `std::atomic_flag`/spinlock for light-array writes (council-chunk-workers §8 recommended option a:
    lock in `setLight` and in the copy).
  - `src/net/minecraft/world/light/LightingEngine.cpp:249-258` (`setBrightness`) — take the per-chunk
    lock while writing nibbles; keep pin protocol.
  - `src/net/minecraft/client/render/chunk/RegionSnapshot.cpp:32-67` (`copyChunkBand`) — take the same
    per-chunk lock; the copy is ~11 KB/section, held for microseconds.
  - `src/net/minecraft/world/chunk/ChunkCache.cpp:173` (`populateBlockLight`), `:373-384` (`decorate`)
    — block writes are main-thread; the lock makes the copy vs write safe.
- Risks: lock ordering (never hold chunk lock while taking `queueMutex_`); keep lock scope tiny.
  Alternative if lock contention shows up: version-stamp the arrays (option b) — but start with (a).
- Parity: none; fixes UB. Render output may change subtly (torn copies were nondeterministic).
- Verification: compile + full ctest; specifically `tests/chunk_mesh_golden_test.cpp`,
  `tests/unified_light_registry_test.cpp`, `tests/server_world_events_test.cpp`. Add a stress unit
  test `tests/region_snapshot_race_test.cpp`: one thread memcpy-snapshots while a lighting-style
  writer mutates nibbles; assert the snapshot bytes are always a valid full value (no torn read),
  repeat 1000×.

#### WI-4 — Mesh scheduler onto the Compute pool + `Channel<T>`; non-blocking cancel; nearLane upload priority

- Objective: replace the ad-hoc mesh handoff with the canonical channel; make `clearSections` stop
  blocking the main thread; honor `nearLane` in the upload path.
- Files:
  - `src/net/minecraft/client/render/chunk/ChunkBuilder.hpp:120-157` (`ChunkMeshScheduler`) — back
    `handoff_` with `coordinator.pool(Compute)` + a `Channel<std::shared_ptr<ChunkMeshJob>>`; keep
    `enqueue/enqueueNear/drainCompleted/idle/workerCount/cancelAll` public API shape so
    `WorldRenderer` compiles unchanged.
  - `src/net/minecraft/util/concurrent/WorkerHandoff.hpp:31-40` — `drainCompleted()` becomes a
    `Channel::drain` batch (non-blocking); **`cancelAll()` stops calling `pool_.drain()`** (the
    blocking `WorkerPool.hpp:50-53`). Replaced by: drop queued + epoch/generation token so owners
    learn their jobs were dropped (fixes rank #3, `meshJobInFlight` stuck — `ChunkBuilder.hpp:105`,
    set at `WorldRenderer.cpp:577`).
  - `src/net/minecraft/client/render/world/WorldRenderer.cpp:372-373` (`clearSections` →
    `meshScheduler_.cancelAll()`), `:577` (`meshJobInFlight` set), `:747-766` (`processUpload`:
    check `job->nearLane` first, exempt from the upload budget — honors `ChunkMeshJob.hpp:51-54`
    stale comment; budget source moves to the shared per-frame deadline in WI-12).
- Risks: R3 invariant — the last `shared_ptr<ChunkMeshJob>` must always die on the main thread
  (`ChunkBuilder.cpp:220-225`); keep `cancelAll` main-thread-only and do not drop refs in the pool
  callback. `sweepRetiring` (`WorldRenderer.cpp:291-300`) must reap in-flight jobs when they complete.
  Do not reorder near-vs-ring capture loops (`WorldRenderer.cpp:792-826`).
- Parity: upload stays main-thread GL; pin protocol unchanged (`ChunkMeshJob::capture`,
  `ChunkBuilder.cpp:132-197`); version-stamp stale-drop unchanged (`WorldRenderer.cpp:761-764`).
- Verification: compile + ctest (`tests/chunk_mesh_golden_test.cpp`, `tests/chunk_map_test.cpp`).
  Add `tests/mesh_cancel_test.cpp`: enqueue 1000 jobs, `cancelAll()` returns promptly (assert <50 ms
  on a loaded pool) and every affected builder's `meshJobInFlight` is cleared; completed jobs still
  drain in order.

#### WI-5 — Confine GL state writes to the main GL thread (alphaTestRef + GLCore init)

- Objective: kill the worker→main GL-state data race (rank #4).
- Files:
  - `src/net/minecraft/mod/model/ModModels.cpp:626` — stop calling `core::setAlphaTestRef(0.1f)`
    from mesh workers (`drawLuaBlockWorld` runs inside `ChunkBuilder::buildMesh` on Compute workers).
    Alpha-test-ref becomes a value captured into the mesh job / `RenderSettings` snapshot at capture
    time (`ChunkMeshJob` capture, `ChunkBuilder.cpp:189-195` already snapshots
    `lightLevelToLuminance` and `blockRenderLayers`); the tessellation path reads it from the
    snapshot, never from the global.
  - `src/net/minecraft/client/render/core/RenderCore.cpp:70,671-680` — keep `g_alphaTestRef` but make
    it write-only from the main thread (assert `TL_DOMAIN == Main` in `setAlphaTestRef`).
  - `src/net/minecraft/client/gl/GLCore.cpp:121,145` — `g_loaded` plain bool → `std::once_flag`/atomic
    so main + GlCompile workers can't tear each other's init (GLCore.cpp:142-145).
- Risks: mod block meshing currently relies on the global being 0.1 at draw; the snapshot default must
  match. Any code path that reads `g_alphaTestRef` during mesh (none should) breaks — grep before
  editing.
- Parity: no visible change; removes UB. Re-run `tests/chunk_mesh_golden_test.cpp` (mod-free) and add
  a `TL_DOMAIN` debug assert (`ASSERT_MAIN_THREAD()` in `setAlphaTestRef`) — cheap, enabled in Debug.
- Verification: compile + ctest + new `tests/gl_state_affinity_test.cpp` (static-analysis-style:
  verify `setAlphaTestRef` has a `TL_DOMAIN` guard under `#ifdef`).

#### WI-6 — Lighting engine onto the Compute pool / channel; keep conflict-claim

- Objective: kill the third independent pool; make lighting workers = Compute-domain workers (own
  lane or same queue — see Open Decision D3) and the outbox a `Channel`.
- Files:
  - `src/net/minecraft/world/light/LightingEngine.hpp/.cpp:46-57` — replace the private `jthread`
    vector with tasks submitted to `coordinator.pool(Compute)`; the per-worker `WorkerState`
    conflict-claim algorithm (`tryClaimBox` :149-170, `threadLoop` :187-206) becomes a loop that pulls
    a box per task; **the box-conflict claim must still prevent two workers from touching overlapping
    boxes** (this is what makes pins correct).
  - `src/net/minecraft/world/light/LightingEngine.cpp:113-126` (`drainDirtyRegions`) — back the outbox
    with `Channel<DirtyRegion>`; `World::doLightingUpdates` (`World.cpp:711-717`) signature unchanged.
  - `stop()` :131-148 — request_stop under `queueMutex_` discipline preserved; the channel owns the
    queue.
- Risks: the per-worker `WorkerState` design is thread-affine (a worker's pin cache, `chunkAt/
  releasePins` :207-233). If lighting shares Compute with mesh, a task must not assume it always runs
  on the same OS thread. Recommendation: **give lighting its own bounded lane** (own channel, own
  sub-priority) inside the same pool so `WorkerState` keyed by `std::thread::id` still works, or make
  `WorkerState` per-claimed-box rather than per-thread. Do not weaken pin semantics.
- Parity: none visible. `doLightingUpdates` cadence unchanged (World.cpp:711-717, MinecraftServer
  :378,518).
- Verification: compile + `tests/server_world_events_test.cpp`, `tests/unified_light_registry_test.cpp`.
  Add `tests/lighting_channel_test.cpp` asserting region drain order and that no overlapping boxes ran
  concurrently (reuse existing claim logic).

#### WI-7 — Chunk loader/save onto domains; fix `cancelPending` silent-drop; lighting-ready gate

- Objective: loader → Compute, save → Io; no lost `PendingLoad` on cancel; avoid first-mesh double-work.
- Files:
  - `src/net/minecraft/world/chunk/ChunkCache.cpp:212-219` (`ensureLoaderPool` → Compute pool),
    `:220-225` (`ensureSavePool` → Io pool).
  - `src/net/minecraft/world/chunk/ChunkCache.cpp:226-249` (`requestChunkAsync`) — `PendingLoad`
    gains an epoch/generation token; `cancelPending()` (loader) marks them cancelled so
    `integrateFinishedLoads` (`:250-288`) drops them instead of scanning forever
    (`ChunkCache.cpp:231-249` leak, rank #3).
  - `src/net/minecraft/world/chunk/ChunkCache.cpp:250-288` (`integrateFinishedLoads`) — budget comes
    from the shared per-frame deadline (WI-12) instead of hard 2/32/4 constants.
  - Lighting-ready gate (council-chunk-workers §6, §8): have `drainDirtyRegions`/`chunkAvailable`
    mark a column "lit" only after its propagation boxes drain; `WorldRenderer::enqueueDirtyChunk`
    (`WorldRenderer.cpp:195-214`) holds first-build until lit. (Optional in this item; see OD-D5.)
- Risks: per-thread generator clones keyed by `std::thread::id` (`ChunkCache.cpp:85-105`) assume
  stable worker identity — if Compute is a shared pool this breaks; keep the loader on a dedicated
  lane or keep the id→generator map but tolerate churn (map grows; prune on world switch). `ioMutex_`
  (`.hpp:84`) recursive lock discipline preserved (storage + decorate serialization).
- Parity: `decorate` stays main-thread (`.cpp:373-384`); remote-client worlds are packet-driven only
  (RULES §10).
- Verification: compile + `tests/server_chunk_cache_test.cpp`, `tests/chunk_map_test.cpp`,
  `tests/integrated_server_host_test.cpp`, `tests/lan_host_coordinator_test.cpp`. Add
  `tests/chunk_cancel_test.cpp`: submit loads, cancel, assert no forever-pending entries and that
  a re-request for the same pos works.

### PHASE C — Shader compile under the budget

#### WI-8 — Shader compile count via GlCompile; frame-driven poll; atomic init

- Objective: fold shader compile into the one budget; stop main-thread blocking waits in the normal
  path; fix the `g_loaded` init race (overlaps WI-5).
- Files:
  - `src/net/minecraft/client/gl/ShaderCompileService.cpp:47` → `Domain::GlCompile` count (2–3).
  - `src/net/minecraft/client/gl/ShaderCompileService.cpp:144-177` (`compileBlocking`) — keep for the
    fallback/urgent path only; the normal path is `submit` + drain `completed_`/channel each frame.
  - `src/net/minecraft/client/gl/ProgramCache.cpp:159-229` (`poll`) — drive from the `compileDone`
    channel in the render phase (via WI-12's `mailbox.drainRender`); `GameRenderer.cpp:707` already
    polls each frame.
- Risks: `compileBlocking` `job->cv.wait` (ShaderCompileService.cpp:167) must remain reachable for the
  disk-miss fallback but never the default; shared-context teardown ordering (WI-13).
- Parity: identical pack-load behavior; GL is driver-thread compile via
  `GL_KHR_parallel_shader_compile` hint (GLCore.cpp:286-291) unchanged.
- Verification: compile + `tests/shader_gl_integration_test.cpp`, `tests/shader_frame_data_test.cpp`,
  `tests/glsl_snippets_test.cpp`, `tests/custom_uniforms_test.cpp`, `tests/shader_pack_loader_test.cpp`.

### PHASE D — Network coordination (keep the Java 2-thread + sim-drain contract)

#### WI-9 — Network packet-path correctness: per-Connection accounting, read-side cap, verify-thread result-only, single drain per tick

- Objective: eliminate the N-connection static data race, unbounded inbound queue, cross-thread
  disconnect, and double-drain — without changing the 2-thread-per-connection + sim-thread-apply model.
- Files:
  - `src/net/minecraft/network/Packet.hpp:79-81,121-128` — move `packetTrackers()`/`incomingCount()`
    mutation out of reader threads; make counters per-`Connection`, aggregated on the game thread for
    the tracker/debug view (network-threading §10, fix H4).
  - `src/net/minecraft/network/Connection.cpp:184-228` (`tick`) — add a **read-side byte/entry cap**
    + overflow policy (see OD-D2); keep the send-side overflow check (`:185-187`) in `tick()`.
  - `src/net/minecraft/server/network/ServerLoginNetworkHandler.cpp:149-178` (`verifyThread_`) — the
    verify thread only publishes `deferredLoginPacket_` under `verifyMutex_`; **all `Connection`/
    `closed` mutation moves to the server tick thread** (`closed` plain-bool race,
    `ServerLoginNetworkHandler.cpp:64` vs `ConnectionListener.cpp:116`; fix H5). Remove the join in
    `verifyUsernameOnline` (`:150-152`) — spawn fresh or use the Io pool (WI-11).
  - `src/net/minecraft/client/multiplayer/ClientNetworkHandler.cpp:243-245` — stop joining the prior
    `joinServerThread_` to reuse it (H6); result is polled via `processPendingJoinServer` (`:55-83`).
  - Double-drain (H7): gate one drain per `Connection` per game tick (track `lastDrainTick` in
    `Connection` or guard in the handler) so `DownloadingTerrainScreen::tick` (`DownloadingTerrainScreen.hpp:19-26`)
    + `ClientWorld::tick` (`ClientWorld.cpp:53-55`) collapse to one; move the screen's keep-alive to
    `ClientNetworkHandler::tick`.
- Risks: drain-order FIFO must hold (`onPlayerMove` echo vs teleport, `ServerPlayNetworkHandler.cpp:149-155`);
  keep `interrupt()` after every drain (`ClientNetworkHandler.cpp:91`, `ConnectionListener.cpp:121,142`);
  keep the `dataPackets`-before-chunk-data discipline (`Connection.cpp:293-316`). Do NOT move `apply`
  off-thread.
- Parity: preserves Java drain-on-sim-thread, 100-packet-flavor fairness, send-overflow
  `disconnect.overflow`; see OD-D4 for cap policy.
- Verification: compile + `tests/connection_packet_drain_throughput_test.cpp` (drain scales past the
  8-packet floor), `tests/chunk_packet_drain_test.cpp`, `tests/connection_listener_test.cpp`,
  `tests/server_login_handler_test.cpp`, `tests/packet_roundtrip_test.cpp`,
  `tests/mp_chunk_delivery_test.cpp`, `tests/mp_chunk_data_test.cpp`. Add `tests/packet_accounting_test.cpp`
  (2+ connections reading concurrently produce no torn counters).

#### WI-10 — Socket I/O under the scheduler; async teardown (no game-thread join)

- Objective: fix the single biggest parity/responsiveness deviation — `disconnect()`/`joinThreads`
  (`Connection.cpp:157-160,352-360`) joining reader+writer on the caller's stack can stall the game
  loop up to ~30 s on a dead peer (H1). Java uses async `NetworkMasterThread` (`NetworkManager.java:160-189`).
- Files:
  - `src/net/minecraft/network/Connection.cpp:134-137,157-160` — `disconnect()` becomes:
    CAS `open_` false, `shutdown(SD_BOTH)` (unblocks `recv`), cancel read interest, flush remaining
    writes with a short grace, return immediately. Object destruction moves off the drain stack via
    the existing retire pattern (`Minecraft.cpp:765-775`, `MultiplayerSession.cpp:12-19`) and
    `Lifecycle`-registered joins. **Never join on the game thread.**
  - `src/net/minecraft/network/Connection.hpp:113-114` (`reader_`/`writer_`) — either keep
    per-connection threads registered as `Domain::NetIo` with `reserveDynamic(2)` +
    unblock-then-watchdog-join (smaller diff), OR move to a small NetIo pool/event loop (OD-D1).
  - `src/net/minecraft/server/network/ConnectionListener.cpp:99-154` — per-tick budget accounting: a
    total budget split across connections with per-connection caps (Java ≤100/player/tick as floor).
- Risks: H10 re-entrancy — `ServerPlayNetworkHandler::disconnect` from inside a packet handler
  (`ServerPlayNetworkHandler.cpp:71-91,239`) must stay deferred off the drain stack. Reader must be
  cancellable (shutdown(SD_RECEIVE) today, `Connection.cpp:341`); under an event loop reads must be
  cancel-interest, not blocking recv.
- Parity: Java never joins socket threads on the sim thread; teardown is async.
- Verification: compile + the same connection/mp tests as WI-9. Add `tests/connection_async_teardown_test.cpp`:
  connect to a black-holed peer, `disconnect()`, assert it returns < 250 ms and a later flush of
  retired bridges completes; watchdog-leak path covered by `tests/lifecycle_test.cpp`.

### PHASE E — Ephemeral one-shot threads → Io domain

#### WI-11 — Detached / ad-hoc `std::thread`s become Io-domain tasks (or registered short-live threads)

- Objective: kill every fire-and-forget thread and the main-thread mutations they do (ranks #5/#6).
- Files:
  - `src/net/minecraft/client/texture/ImageDownload.cpp:7-18` — **delete `.detach()`**; the download
    lands in a texture `Channel`; main-thread `TextureManager` applies. Fixes the `image`/`slimArms`
    data race (`ImageDownload.hpp:17-18`) and the destroyed-object-write.
  - `src/net/minecraft/client/session/SessionValidator.cpp:17-51` — detached check → Io task; result
    still `failedSessionCheckTime` atomic, consumed at `Minecraft.cpp:604-606`.
  - `src/net/minecraft/client/diagnostics/ClientDiagnostics.cpp:235-264` — hang watchdog: keep a
    registered thread (not detached); disarmed at `Minecraft.cpp:349`.
  - `src/net/minecraft/client/multiplayer/MultiplayerConnector.cpp:17-61,63-68` — connector work on
    Io; **never call `bridge->disconnect()` → `minecraft->setWorld(nullptr)` from the connector thread**
    (`ClientNetworkHandler.cpp:115-127` cross-thread violation); publish result under `mutex_` and let
    the main thread apply (`poll`, `:83-94`).
  - `src/net/minecraft/client/resource/ResourceDownloadThread.cpp:85-120,138,168` — worker only
    produces files/bytes; `minecraft_->loadResource(...)` moves to the main thread via a channel.
  - `src/net/minecraft/client/gui/auth/LoginScreen.cpp:98-106,125-134,260-271`,
    `src/net/minecraft/client/auth/microsoft/SessionRestore.cpp:55-92,163-175,237-268` — HTTP work on
    Io; main thread never joins mid-flight (blocking join hazard, `LoginScreen.cpp:224-226`).
  - `src/net/minecraft/world/World.cpp:300-339` (`asyncSaveFuture_`, `World.hpp:429`) — `std::launch::async`
    level.dat writer → Io pool task with a snapshot copied under the same rules (fix the
    `dimensionData_`/players concurrent-read race, World.cpp:331-338); `save(blocking)` at teardown
    still waits (H9).
  - `src/net/minecraft/server/dedicated/gui/DedicatedServerGui.cpp:61-82,153-161` — `guiThread.detach()`
    and the `server_.stopped` plain-bool spin: register the GUI thread; make `stopped` atomic (H7-gui).
- Risks: every one of these is a behavior-preserving move IF the main-thread-only consumer contract is
  kept. Watch `ImageDownload` consumers (skin apply reads `image`/`slimArms` — must be main-thread
  after channel drain). `ServerProcessCoordinator` (external process) stays out of scope (OD-D9).
- Parity: Java downloads session/skin on background threads too; only ownership discipline changes.
- Verification: compile + full ctest. Existing network/host tests (`tests/handshake_metadata_test.cpp`,
  `tests/integrated_server_host_test.cpp`, `tests/lan_host_coordinator_test.cpp`,
  `tests/multiplayer_client_player_entity_test.cpp`) must stay green; add `tests/image_download_channel_test.cpp`
  and `tests/multiplayer_connector_test.cpp` (connector thread never touches `setWorld`; cancellation
  returns fast).

### PHASE F — Main loop restructure

#### WI-12 — `FramePipeline` + `TaskMailbox` + `FrameProfiler`; explicit phases; one shared frame budget

- Objective: the centerpiece. Turn `Minecraft::run()` into the phase pipeline while preserving tick and
  render order byte-for-byte (architecture §4; the "do not reorder" invariant at Minecraft.cpp:647-649).
- New files: `src/net/minecraft/client/util/FramePipeline.hpp/.cpp`,
  `src/net/minecraft/client/core/TaskMailbox.hpp/.cpp`,
  `src/net/minecraft/client/util/FrameProfiler.hpp/.cpp`.
- Files to touch:
  - `src/net/minecraft/client/Minecraft.cpp:762-843` (`run()`) — body becomes
    `FramePipeline::run()`: Phase 0 DRAIN (screenStack_.flushRetired :778, multiplayerSession_.flushRetired
    :782, bridge retire :783-788, applet/close checks :789-796, pendingScreenResize),
    Phase 1 (luaHookTickRate :797-800, timer.advance with paused freeze :801-807, tick loop :808-818),
    Phase 2 RENDER (moved `runRenderPhase` drain order), Phase 3 PACE (inactive sleep :705-710 /
    vsync), Phase 4 DIAGNOSTICS (heartbeat :776, FPS window :731-737, toast :727, profiler :722-726).
  - `src/net/minecraft/client/Minecraft.cpp:670-761` (`runRenderPhase`) — keep the ORDER
    (audio.updateListener :683 → doLightingUpdates :685 → pumpChunkPublish :686 → setSwapPacing :692 →
    pumpAndPresent :694 → wall check :698-700 → render :701-710), but budget sources and drains move to
    the mailbox (mesh uploads, shader poll, texture stream, chunk integrates).
  - `src/net/minecraft/client/Minecraft.cpp:742-743,827-828` — the two `static std::mutex stallMutex`
    blocks collapse into `FrameProfiler` (one mutex, one file, phase-enum trace). Ship behind the
    existing `MINECRAFT_RENDER_TRACE` guard.
  - `src/net/minecraft/util/concurrent/FrameBudget.hpp:5-14` — add `FrameBudget::deadline` shared
    per-frame (set once by FramePipeline, `frame.remaining()`); update the two internal users
    (`WorldRenderer.cpp:742-743,787-789`) and (later) `integrateFinishedLoads` and `Connection::tick`
    to read the same deadline instead of `fromMs(...)`.
  - `src/net/minecraft/client/core/TaskMailbox.*` — owns deferred main-thread work: urgent (retire),
    tick (audio/msauth/stats), render (mesh/shader/texture/chunk) drain queues.
- Risks: highest in the plan. Mitigations: keep the old `run()` body as a reference; character-compare
  phase order; ship behind a flag initially; do NOT reorder tick/render; present-before-draw stays
  (`runRenderPhase` :688 then :700) — confirm intent (OD-D7).
- Parity: the single-main-thread contract is the *point* of this item; Iris order, `partialTick`, and
  present rhythm are untouched.
- Verification: compile + ctest; specifically `tests/client_timer_test.cpp` (timer cadence),
  `tests/render_profiler_test.cpp`, `tests/draw_camera_state_test.cpp`, `tests/shader_frame_data_test.cpp`,
  `tests/chunk_mesh_golden_test.cpp`, `tests/camera_position_tracker_test.cpp`. Add
  `tests/frame_pipeline_order_test.cpp` instrumenting phase call order against the current order.

### PHASE G — Shutdown

#### WI-13 — Lifecycle teardown wiring

- Objective: replace ad-hoc destructor-order shutdown with the ordered, watchdogged Lifecycle pass.
- Files: `src/net/minecraft/client/Minecraft.cpp:347-393` (`stop()`) — route through
  `Lifecycle::shutdown()` (fence → unblock → stop+drain → watchdog join → session-clear → GL destroy);
  the existing order (ShaderCompileService::stop :354 before DisplayManager::destroy :388) is encoded,
  not incidental. `src/net/minecraft/client/host/ServerProcessCoordinator.cpp:279-290` (10 s wait +
  terminate) and `src/net/minecraft/util/logging/Logging.cpp:129-177` (leaked singletons stay — the log
  writer outlives main-thread statics) register with Lifecycle. `Minecraft.cpp:837-841` crash handlers
  also route through it.
- Risks: deadlock if any owner is blocked in a call that can't observe stop (socket, driver, disk CV);
  the unblock-first rule (architecture §3.3) is mandatory. Never rely on `jthread` dtor order.
- Parity: `std::_Exit(0)` may stay as the final fallback, but only after a best-effort ordered teardown.
- Verification: compile + `tests/minecraft_server_tick_test.cpp`, `tests/integrated_server_host_test.cpp`,
  `tests/lan_host_coordinator_test.cpp`, `tests/lifecycle_test.cpp`.

### PHASE H — Server parity (deferred)

#### WI-14 — Apply the coordinator to the dedicated server binary

- Objective (architecture §7.10, optional/deferred): configure the coordinator in `src/server-main.cpp:125-126`
  and `MinecraftServer.cpp:196-214` (commandThread_), `ConnectionListener` accept thread, per-connection
  threads — same budget + registration. Lower priority: server is a separate process.
- Verification: compile server target (`-Target Server`) + `tests/minecraft_server_tick_test.cpp`,
  `tests/player_manager_test.cpp`, `tests/server_command_handler_test.cpp`.

---

## 4. Dependency graph

```
WI-1 (coordinator/channel/lifecycle infra)
 ├─ WI-2  (delete recommendedThreadCount)        ← depends: WI-1
 ├─ WI-6  (lighting → compute/channel)            ← depends: WI-1, WI-2
 ├─ WI-8  (shader count+poll)                     ← depends: WI-1, WI-2
 ├─ WI-10 (async socket teardown)                 ← depends: WI-1 (Lifecycle), WI-9
 ├─ WI-11 (ephemeral → Io)                        ← depends: WI-1 (Io pool); parallel-safe with B/C/D
 └─ WI-13 (lifecycle shutdown wiring)             ← depends: WI-1, WI-8, WI-11
WI-2 → WI-4 (mesh scheduler on Compute+Channel)   ← depends: WI-2 (uses Compute count); WI-3 optional
WI-3 (light-array race)                           ← independent; can run first/parallel with WI-1
WI-4 → WI-5 (GL-state confinement)                ← depends: WI-4 (worker path touched)
WI-2 → WI-7 (loader/save domains)                 ← depends: WI-2, WI-6 (shared Compute domain); WI-3
WI-9 (network correctness)                        ← independent of A–C; depends only on nothing new
WI-12 (FramePipeline/mailbox/budget)              ← depends: WI-4, WI-6, WI-7, WI-8 (channels feed
                                                     mailbox drains), WI-9 (packetIn drain phase)
WI-14 (server)                                    ← depends: WI-2 (optional)
```

Parallelizable lanes (safe to run concurrently as separate executors, since items are additive and
touch disjoint files):
- Lane 1 (chunk compute): WI-3 → WI-4 → WI-5.
- Lane 2 (infra + counts): WI-1 → WI-2 → WI-6/WI-7/WI-8.
- Lane 3 (network): WI-9 → WI-10.
- Lane 4 (ephemeral): WI-11.
- Gate for WI-12 (the big one): all of WI-4/6/7/8/9 must have landed so every drain has a channel.
- WI-13 last; WI-14 optional after WI-2.

Each lane is compile-safe at every step (no item depends on a later item). The compile-fixer runs at
the very end over the merged tree.

---

## 5. Open decisions for the plan auditors

- **D1 — Event loop vs blocking NetIo pool for sockets.** Windows-only (winsock): overlapped/WSAPoll
  loop vs a small 2–4 worker blocking-IO pool. Council network-threading §11.1 notes the pool is the
  smaller diff and keeps `SocketInputStreamBuf`/`OutputStreamBuf` (Connection.cpp:30-119) intact, but
  blocking reads can't be cancelled cheaply. Recommend: **start with the blocking NetIo pool (WI-10
  keeps per-connection threads registered), defer an event loop**; re-evaluate if packet latency shows.
- **D2 — Read-side cap policy.** Disconnect-on-overflow (Java-style `disconnect.overflow`) vs
  backpressure (pause reading). Parity says send-side disconnect; read-side is new territory. Recommend:
  **bounded read queue + disconnect.overflow**, mirrored on the existing `0x100000` constant.
- **D3 — Single shared Compute pool vs domain lanes.** Architecture §9.1 and §8 recommend lighting gets
  **its own lane** (own bounded channel, sub-priority) inside Compute because of the per-worker
  `WorkerState`/pin-cache affinity. Auditors must confirm this before WI-6.
- **D4 — Drain budget policy.** Keep C++ time-box (3 ms / min 8 / max 4096) + add per-connection
  fairness, or reintroduce Java's hard 100/player/tick floor? Council network §11.3 recommends
  time-box + per-connection cap. Confirmed by the throughput test `connection_packet_drain_throughput_test.cpp`.
- **D5 — Lighting-ready gate scope.** Fix the double-mesh on world load (WI-7 optional) or defer to a
  follow-up? It changes meshing work volume, not correctness.
- **D6 — Where `ThreadCoordinator` lives.** Singleton `instance()` configured in `Minecraft::init()`
  and `server-main.cpp`. Confirm no ordering hazard vs the leaked `LogDispatcher` singletons (the log
  writer must be usable before/after coordinator shutdown — coordinator must not own the log thread).
- **D7 — Present-before-draw ordering.** Confirm `runRenderPhase` `pumpAndPresent` (:688) before
  `gameRenderer->onFrameUpdate` (:700) is the intended double-buffer rhythm before WI-12 hard-codes it
  (architecture §9.5).
- **D8 — GL shared contexts for shader compile.** Keep the hidden GLFW window + shared-context design;
  workers = GlCompile domain (2–3). Confirm teardown ordering: `ShaderCompileService::stop` before
  `DisplayManager::destroy` (already correct at Minecraft.cpp:354/388). Open: whether GlCompile workers
  may ever fall back to `runJobOnCurrentContext` on the main thread (allowed; already the non-started
  fallback, ShaderCompileService.cpp:157-159).
- **D9 — Keep the two stall-mutex blocks?** Architecture says collapse into `FrameProfiler` (WI-12).
  Recommend: keep them until WI-12 lands (they're `#ifdef MINECRAFT_RENDER_TRACE` debug-only), delete
  then. Auditors confirm no other debug output depends on `stall-trace.log` format.
- **D10 — fpsLimit=0 adaptive sleep.** Architecture §9.2: keep parity (hot spin) unless an option is
  added. Recommend: keep parity in WI-12; note as follow-up.
- **D11 — Dedicated server + ServerProcessCoordinator scope.** Confirm the external-process coordinator
  stays out of scope (network §11.6) and the in-process dedicated server gets WI-14 (deferred).
- **D12 — Audio (4 pinned threads) and pack dir watcher.** Keep as pinned registered threads, not folded
  into compute (architecture §9.4). Confirm.
- **D13 — `World::save` snapshot safety.** The `std::vector<PlayerEntity*>` snapshot (World.cpp:325-339)
  is read off-thread while main mutates players. Confirm the fix (WI-11) snapshots *values* or defers to
  teardown, and that `save(blocking=true)` remains reachable at shutdown (H9).

---

## 6. Test matrix (GoogleTest, `.\build-omega.ps1 -RunTests`)

| Test file | Guards | Relevant WI |
|---|---|---|
| tests/client_timer_test.cpp | Timer cadence | WI-12 |
| tests/chunk_mesh_golden_test.cpp | mesh determinism, options | WI-2, WI-4, WI-5, WI-12 |
| tests/chunk_map_test.cpp | chunk map bounds | WI-4, WI-7 |
| tests/server_chunk_cache_test.cpp | loader/save/integrate | WI-7 |
| tests/unified_light_registry_test.cpp | light registry | WI-3, WI-6 |
| tests/server_world_events_test.cpp | world events | WI-3, WI-6 |
| tests/connection_packet_drain_throughput_test.cpp | drain scales past floor | WI-9, WI-12 |
| tests/chunk_packet_drain_test.cpp | chunk packet drain | WI-9 |
| tests/connection_listener_test.cpp | server listener | WI-9, WI-10 |
| tests/server_login_handler_test.cpp | login verify | WI-9 |
| tests/packet_roundtrip_test.cpp | packet codec | WI-9 |
| tests/mp_chunk_delivery_test.cpp / mp_chunk_data_test.cpp | client chunk stream | WI-9, WI-10 |
| tests/multiplayer_client_player_entity_test.cpp | client entity | WI-11 |
| tests/shader_gl_integration_test.cpp / shader_frame_data_test.cpp / glsl_snippets_test.cpp / custom_uniforms_test.cpp / shader_pack_loader_test.cpp | shader path | WI-8, WI-12 |
| tests/draw_camera_state_test.cpp / camera_position_tracker_test.cpp | camera state | WI-12 |
| tests/render_profiler_test.cpp | profiler | WI-12 |
| tests/integrated_server_host_test.cpp / lan_host_coordinator_test.cpp | host/coordinator | WI-7, WI-10, WI-11, WI-13 |
| tests/minecraft_server_tick_test.cpp / player_manager_test.cpp / server_command_handler_test.cpp | server | WI-13, WI-14 |
| tests/registry_bootstrap_test.cpp / mod_settings_registry_test.cpp / world_required_mods_test.cpp / model_registry_parent_test.cpp / block_face_uv_test.cpp / color_targets_test.cpp / lab_pbr_mipmap_test.cpp / pack_blend_drawbuffer_test.cpp / iris_hemisphere_chunk_offset_test.cpp / shadow_celestial_modelview_test.cpp / custom_uniforms_test.cpp | regression net | untouched; must stay green for every WI |

New tests to add (executors write these; compile-fixer runs them): `tests/channel_test.cpp`,
`tests/thread_budget_test.cpp`, `tests/lifecycle_test.cpp`, `tests/thread_coordinator_test.cpp`,
`tests/region_snapshot_race_test.cpp`, `tests/mesh_cancel_test.cpp`, `tests/gl_state_affinity_test.cpp`,
`tests/lighting_channel_test.cpp`, `tests/chunk_cancel_test.cpp`, `tests/packet_accounting_test.cpp`,
`tests/connection_async_teardown_test.cpp`, `tests/image_download_channel_test.cpp`,
`tests/multiplayer_connector_test.cpp`, `tests/frame_pipeline_order_test.cpp`.
All new tests must be listed in the `MINECRAFT_TEST_SOURCES`/`MINECRAFT_SERVER_TEST_SOURCES` lists in
`CMakeLists.txt` (~:380) so `gtest_discover_tests` picks them up.

---

## 7. Notes for later pipeline stages

- **CONTEXT.md is missing** at `docs/agent-notes/CONTEXT.md` (referenced by the initial-planner brief).
  The transcript synthesizer should flag this to the auditors; the council docs contain the facts, so the
  plan stands on them.
- Two `static std::mutex stallMutex` blocks exist (`Minecraft.cpp:742-743` in `runRenderPhase`,
  `:827-828` in `run()`) — both behind `MINECRAFT_RENDER_TRACE`; do not "merge" them prematurely (OD-D9).
- All council file:line references were spot-verified against the tree where opened; any `[council]`
  refs (mostly secondary GUI/auth/session files not opened this session) must be re-verified by the
  executor before editing.
- RULES §10 (world profiles): profile hooks must receive the active `Chunk&`, and remote Java
  multiplayer client worlds are packet-driven only — network/loading items must not run profile
  terrain/decorate hooks there.
- Never stub (RULES §6): every item above is a real migration; no PORT-STUB additions.

## 8. Files changed summary (authoritative list for the plan master)

New: `util/concurrent/ThreadCoordinator.*`, `util/concurrent/ThreadBudget.hpp`,
`util/concurrent/Channel.hpp`, `util/concurrent/ThreadNames.hpp`, `util/concurrent/Lifecycle.*`,
`client/util/FramePipeline.*`, `client/core/TaskMailbox.*`, `client/util/FrameProfiler.*`.

Modified (by WI): `client/Minecraft.cpp`, `util/concurrent/WorkerPool.hpp`,
`util/concurrent/WorkerHandoff.hpp`, `util/concurrent/FrameBudget.hpp`,
`client/render/chunk/ChunkBuilder.hpp`, `client/render/world/WorldRenderer.cpp`,
`world/chunk/ChunkCache.cpp/.hpp`, `world/chunk/Chunk.hpp`, `world/light/LightingEngine.cpp/.hpp`,
`client/render/chunk/RegionSnapshot.cpp`, `client/render/core/RenderCore.cpp`,
`client/gl/ShaderCompileService.cpp/.hpp`, `client/gl/ProgramCache.cpp`, `client/gl/GLCore.cpp`,
`network/Connection.cpp/.hpp`, `network/Packet.hpp`,
`server/network/ServerLoginNetworkHandler.cpp`, `server/network/ConnectionListener.cpp`,
`client/multiplayer/ClientNetworkHandler.cpp/.hpp`, `client/multiplayer/MultiplayerConnector.cpp`,
`client/multiplayer/ClientNetworkBridge.cpp`, `world/ClientWorld.cpp`,
`client/gui/screen/DownloadingTerrainScreen.hpp`, `client/texture/ImageDownload.cpp/.hpp`,
`client/session/SessionValidator.cpp`, `client/diagnostics/ClientDiagnostics.cpp`,
`client/resource/ResourceDownloadThread.cpp`, `client/gui/auth/LoginScreen.cpp`,
`client/auth/microsoft/SessionRestore.cpp`, `world/World.cpp/.hpp`,
`server/dedicated/gui/DedicatedServerGui.cpp`, `client/host/ServerProcessCoordinator.cpp`,
`util/logging/Logging.cpp`, `server/MinecraftServer.cpp`, `src/server-main.cpp`, `CMakeLists.txt`
(test-source lists), `tests/*` (new tests).
