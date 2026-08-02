# Council Architecture Proposal — Coordinated Threading Model for the Beta 1.7.3 C++ Port

Status: review-only design proposal. No code was edited, nothing was built.
Scope: the native `src/` tree only. Java parity constraints (b1.7.3 vanilla loop + Iris pipeline) are treated as hard invariants.

---

## 0. Executive summary — the decisions, up front

The port today has **~9 independent thread-creation sites**, each computing its own
thread count from `hardware_concurrency()` and mostly calling
`WorkerPool::recommendedThreadCount(...)` with guessed `competingPools` values.
That is the root defect: nobody owns the total. The fix is a **single coordinator
that owns one global thread budget and hands out allocations**, plus **a canonical
handoff channel** every producer/consumer pair uses, plus **an explicit per-frame
main loop** that drains all of them against a shared budget.

Concrete decisions:

1. **Do NOT use one giant shared "do everything" pool.** Use a **small fixed set of
   domain pools sharing ONE global budget** (`ThreadCoordinator`), because three
   roles genuinely cannot be task-scheduled:
   - shader compilation needs its own GL-shared contexts (a context cannot be
     "handed to whatever worker is free"),
   - audio and logging own OS/API-affine threads,
   - blocking socket I/O must never occupy a CPU-tessellation thread.
2. **Main thread stays the GL thread.** No separate render thread. This is the single
   most defensible choice for a b1.7.3 port: the loop is `tick → render → present` on
   one thread, the Iris pipeline order is preserved exactly, and we avoid GL-context
   handoff + draw fencing entirely. "Defer GPU upload to the GL thread" therefore
   means **defer GPU upload to the render phase of the main thread** via a channel,
   which the codebase already half-does.
3. **Standardize on `std::jthread` + `stop_token` for all engine-owned threads**,
   with one ironclad rule: **never join a thread that may be blocked in a call that
   cannot observe the token** — unblock it first, then join with a watchdog.
4. **Rewrite `Minecraft::run()` as an explicit phase pipeline**: `drain mailbox →
   input/screen tick → timer + N world ticks → render phase → pace → diagnostics`.
   Keep every element of the current tick order and render order byte-for-byte.
5. **All cross-thread results travel through one canonical bounded `Channel<T>`**
   (produce-on-worker, consume-on-main), with capacity-based backpressure,
   named priorities, and version stamps for dropping stale results. Replace the
   ad-hoc queues in `WorkerHandoff`, `LightingEngine`, and `Connection` with it.
6. **One shutdown coordinator** destroys threads in an explicit, ordered,
   reverse-of-creation sequence with watchdogged joins. No more relying on
   `jthread` destructor order.

New files (design only): `util/concurrent/ThreadCoordinator.hpp/.cpp`,
`util/concurrent/Channel.hpp`, `util/concurrent/ThreadNames.hpp`,
`util/concurrent/Lifecycle.hpp`, `client/util/FramePipeline.hpp/.cpp`,
`client/util/FrameProfiler.hpp/.cpp`, `client/core/TaskMailbox.hpp/.cpp`.

---

## 1. Current-state inventory (what actually exists)

Read this as the ground truth the proposal reacts to. All paths relative to `src/net/minecraft/`.

| Site | File | Threads created | Count policy |
|---|---|---|---|
| Mesh tessellation | `client/render/chunk/ChunkBuilder.hpp` (`ChunkMeshScheduler` → `WorkerHandoff`) | `recommendedThreadCount(3, 2, 6)` | guesses 3 competing pools |
| Lighting | `world/light/LightingEngine.cpp` | `recommendedThreadCount(3, 2, 3)` | guesses 3 competing pools |
| Chunk load | `world/chunk/ChunkCache.cpp` `ensureLoaderPool` | `recommendedThreadCount(3, 2, 4)` | guesses 3 competing pools |
| Chunk save | `world/chunk/ChunkCache.cpp` `ensureSavePool` | 1 | fixed |
| Shader compile | `client/gl/ShaderCompileService.cpp` | `hw-2`, capped 4, hidden shared GL contexts | independent |
| GL driver parallel compile | `client/gl/GLCore.cpp:288` | `maxShaderCompilerThreadsKHR(hw-2)` | independent |
| Network | `network/Connection.hpp` (`reader_`, `writer_`) | 2 per connection | unbounded per-conn |
| Audio | `client/platform/audio/backend/XAudio2Backend.cpp` | worker + 2 effect decoders + long decoder = 4 | fixed |
| Logging | `util/logging/Logging.cpp` | 1 | fixed |
| Resource download | `client/resource/ResourceDownloadThread.cpp` | 1 | fixed |
| Image download | `client/texture/ImageDownload.cpp` | detached 1 | fire-and-forget |
| Shader pack dir watch | `client/render/pipeline/Manager.cpp` | 1 `jthread` | fixed |
| Auth/session/connector | `client/gui/auth/LoginScreen.cpp`, `client/session/SessionValidator.cpp`, `client/multiplayer/MultiplayerConnector.cpp`, `client/multiplayer/ClientNetworkHandler.cpp`, `client/auth/microsoft/SessionRestore.cpp` | 1 per op, on demand | fire-and-forget |
| Dedicated server | `server/MinecraftServer.cpp` (`runThread_`, `commandThread_`), `server/network/ConnectionListener.cpp`, `server/dedicated/gui/DedicatedServerGui.cpp` | 4+ | fixed |
| Client-diagnostic heartbeat | `client/diagnostics/ClientDiagnostics.cpp` | 1 | fixed |

Shared infrastructure already present and **worth keeping**:

- `util/concurrent/WorkerPool.hpp` — good single-pool primitive: `jthread` workers,
  priority queue, `stop_token`, `cancelPending`, `drain`. Defect: the 
  `recommendedThreadCount` free-for-all and the fact that it computes nothing
  globally.
- `util/concurrent/WorkerHandoff.hpp` — the correct **produce-on-worker,
  consume-on-main** shape (`drainCompleted()` on the main thread). Defect: unbounded
  `completed_` vector (no backpressure), and its pool size is where oversubscription
  enters.
- `util/concurrent/FrameBudget.hpp` — correct time-slicing primitive, already used by
  `WorldRenderer::compileChunks` (mesh uploads and captures). Defect: every caller
  creates its own budget with its own `fromMs(...)`; there is no shared per-frame cap.
- `client/render/chunk/RegionSnapshot.hpp` — the blessed worker-isolation mechanism
  (immutable chunk copy, capture-on-main, tessellate-on-worker). Keep and extend the
  doctrine, do not weaken it.
- `world/light/LightingEngine` pin protocol (`tryAcquireRenderPin`/`releaseRenderPin`)
  — the correct answer to "worker touches live chunk arrays"; it just needs to be
  written down as policy and asserted.
- `client/gl/ShaderCompileService.cpp` — correct shape: workers hold shared contexts,
  **blobs cross threads, live GL objects never do**. Keep.

The main loop (`client/Minecraft.cpp` `run()`:749, `tick()`:593, `runRenderPhase()`:670)
already has the right skeleton — retire-stack, `timer.advance`, tick N, render phase.
It is wrapped in inline stall-trace blocks with **two separate `static std::mutex`
stallMutex instances** (`run()` ~810 and `runRenderPhase()` ~730) and mixes housekeeping
(screen retire, bridge retire, fps counters, heartbeat) into the loop body rather than
phases. That is the "a few dozen things, none coordinated" smell concentrated in one
function.

---

## 2. Thread role map

Roles the engine needs, who owns them now, who owns them in the target design.

| Role | Natural owner | Target placement | Notes |
|---|---|---|---|
| **Render / GL** | Main thread (`Minecraft::run()`), window context | **Main thread = GL thread. Never split.** | All GL calls, uploads, present, Iris pipeline. Parity invariant. |
| **Game-logic / tick** | Main thread (world is `worldSession_.ownedWorld`, ticked in `Minecraft::tick`→`runWorldSimulation`) | **Main thread.** | b1.7.3 runs integrated simulation in the same loop; do not invent a separate sim thread. |
| **Dedicated-server tick** | `server/MinecraftServer.cpp` `run()` (own process/thread via `client/host/ServerProcessCoordinator`) | **Unchanged** — it is a separate process. | Client-side "host" only launches/polls it. |
| **Chunk-mesh workers** | `ChunkMeshScheduler` pool | **Compute domain pool** (shared with lighting+gen). | CPU tessellation against `RegionSnapshot`. |
| **Lighting workers** | `LightingEngine` threads | **Compute domain pool** (separate lane / same pool, still pin-guarded). | Specialized conflict-claim algorithm stays; only the count source changes. |
| **Terrain gen / chunk produce** | `ChunkCache::loaderPool_` (per-thread `ChunkSource` generators keyed by `thread::id`) | **Compute domain pool** (it is CPU, not I/O; the per-thread generator map already works and stays). | The `workerGenerators_` thread-id map is correct and survives. |
| **Chunk save / file I/O** | `ChunkCache::savePool_` (1), plus resource loading | **I/O domain pool** (small, blocking-tolerant). | Save is serialized anyway; keep 1–2. |
| **Network I/O** | `Connection::reader_/writer_` per connection | **Pinned per-connection threads, budget-capped** (register against coordinator). | Blocking sockets; unblock-then-join on shutdown. |
| **Audio** | `XAudio2Backend` worker + 2 decoders + long decoder | **Pinned audio threads, budget-capped.** | API/device affinity; never fold into compute. |
| **Logging** | `util/logging/Logging.cpp` dispatcher | **Pinned log thread, budget-capped.** | Must never block a game thread on disk. |
| **Shader compile** | `ShaderCompileService` workers on shared GL contexts | **GL-compile domain pool** (2–3 max). | Context-affine; separate from compute. |
| **Pack directory watch** | `client/render/pipeline/Manager.cpp` `jthread` | **Coordinator-registered pinned thread.** | Fine as-is; register for shutdown. |
| **Ephemeral one-shots** (login, session check, image download, multiplayer connect) | ad-hoc `std::thread`s | **I/O domain pool** `submit` (or `Lifecycle`-registered short-lived threads during the transition). | Kill the detached `std::thread` spawn sites. |

**Rule of thumb for the role table:** CPU-bound world work → Compute. Blocking
file/network/unknown-latency work → I/O. API-affine work (GL, audio, sockets, disk
logger) → its own pinned class, counted against the budget. GL-shared-context work →
GL-compile.

---

## 3. Thread-count strategy: one budget, few domain pools

### 3.1 Why not one giant shared pool

A single "everything" executor would be simplest but fails on three hard constraints
in this codebase:

1. **GL context affinity.** `ShaderCompileService` workers must each hold a hidden
   shared GL context for the lifetime of the thread (`createWorkerWindow(shareWith)`,
   `glfwMakeContextCurrent(window)`). A generic pool cannot attach a context to an
   arbitrary idle worker.
2. **Blocking calls.** Socket read/write and disk saves block. If they run on the
   compute pool, one stalled chunk save stalls tessellation.
3. **Priority fairness.** Near-camera mesh edits want `INT_MIN` priority now;
   logging wants never. A shared FIFO can't express that cleanly, and the domain
   lanes already exist.

### 3.2 The design: `ThreadCoordinator` + `ThreadBudget`

New: `util/concurrent/ThreadCoordinator.hpp/.cpp` and `util/concurrent/ThreadBudget.hpp`.

- `ThreadCoordinator::instance()` — process-wide singleton, configured once in
  `Minecraft::init()` (and in `server-main.cpp` for the server binary) via
  `configure(hardwareThreads, reservedThreads, options)`.
- It computes **one global budget** exactly once:
  `globalBudget = max(1, hardwareThreads - reserved)` where reserved defaults to
  2 (1 for the main/GL thread, 1 for the driver/OS/swapchain) — this replaces every
  `recommendedThreadCount` and every `hw - 2`.
- **Mandatory pinned classes** are deducted first: audio, logging, network
  (registered dynamically), GL-compile. **Compute and I/O** get the remainder:
  - `compute = clamp(remaining, 1, options.maxComputeThreads)` (default 8)
  - `io = clamp(2, remaining/4, 3)`
- `coordinator.pool(Domain::Compute)` returns the shared `WorkerPool` for that domain;
  every subsystem asks, nobody computes.
- `coordinator.reserveDynamic(n)` / `releaseDynamic(n)` — called when a connection
  spawns reader/writer threads, so the budget count is authoritative even for
  per-connection threads.
- A single `std::atomic<int> coordinator.totalPending()` (sum of per-domain pending)
  replaces `ChunkBuilder::chunkUpdates`-style local counters for the F3 HUD.

API sketch (documentation only):

```cpp
enum class Domain { Compute, Io, GlCompile, Audio, NetIo, Log };
class ThreadCoordinator {
  static ThreadCoordinator& instance();
  void configure(unsigned hardware, unsigned reserved, const ThreadOptions&);
  WorkerPool&  pool(Domain);
  ThreadBudget budget() const;          // the single allocation table
  void reserveDynamic(unsigned n);      // net reader/writer threads
  void releaseDynamic(unsigned n);
  std::atomic<int>& totalPending();     // F3 debug
  void shutdown();                      // ordered Lifecycle teardown (section 7)
};
```

`WorkerPool::recommendedThreadCount` is **deleted**. All call sites switch to
`ThreadCoordinator::instance().pool(Domain::X)`:

- `ChunkBuilder.hpp:156` → `coordinator.pool(Domain::Compute)`
- `LightingEngine.cpp:49` → `coordinator.pool(Domain::Compute)` (or its own lane)
- `ChunkCache.cpp:217` loader → `Domain::Compute`; `:224` save → `Domain::Io`
- `ShaderCompileService.cpp:47` → `coordinator.pool(Domain::GlCompile).threadCount()`
- `GLCore.cpp:289` → `coordinator.budget().glDriverThreads()`

### 3.3 jthread / stop_token policy

Already half-adopted (WorkerPool, LightingEngine, pipeline Manager). Standardize:

- **All engine-owned threads are `std::jthread`** with a `stop_token`-aware loop:
  `wait(predicate = stop_requested() || workAvailable)`.
- **The one rule that prevents hangs:** a thread whose loop can block inside a call
  that cannot observe the token (socket recv, `SwapBuffers`, XAudio2 device,
  `std::filesystem` read on a hung network drive) must be **unblocked first**
  (`shutdown` the socket, post an event, wake a CV), *then* `request_stop`, *then*
  `join`. `Connection::interrupt()` + `joinThreads()` already does this for sockets;
  the `Lifecycle` coordinator formalizes it (section 7).
- **Never rely on `jthread` destructor order.** Destruction order of members is
  declaration order, and several owners are globals/singletons; teardown must be
  explicit and ordered through the coordinator.
- If `join` is still stuck (hung driver, stuck socket), join with a watchdog:
  after a grace period (2–5 s) log once and **leak** (process is exiting anyway);
  a hung `std::terminate` at shutdown is worse than a leaked blocked thread.

---

## 4. Main-thread restructure — the new `Minecraft::run()`

The current `run()` is functionally the target; the refactor is to make its phases
explicit, move all cross-thread draining into one mailbox, and remove inline
diagnostics. **The tick order inside `tick()` (the "INVARIANT: Tick order — do not
reorder" comment at Minecraft.cpp:647) and the render order inside
`runRenderPhase()` are preserved verbatim.** We only change *where* and *when* the
per-frame drains happen, never *what* runs.

### 4.1 Phase shape

New files: `client/util/FramePipeline.hpp/.cpp` (owns the loop), 
`client/core/TaskMailbox.hpp/.cpp` (owns deferred main-thread work),
`client/util/FrameProfiler.hpp/.cpp` (owns stall traces).

```
FramePipeline::run():
  while(running):
    tickDelta = 0
    // ── Phase 0: DRAIN (deferred main-thread work, non-blocking) ──────────
    mailbox.drainUrgent();      // screenStack.flushRetired, multiplayerSession.flushRetired,
                                // bridge retire, world teardown (SessionLock handling),
                                // close-requested / applet-exit checks, pendingScreenResize
    // ── Phase 1: INPUT + UI TICK ──────────────────────────────────────────
    timer.advance();            // with the paused-partialTick preservation
    mailbox.drainTick();        // per-tick deferred: audio state, msauth restore, stats
    for i in timer.ticksThisFrame:
      ++ticksPlayed
      tick()                    // UNCHANGED body (see tick-order invariant)
    // ── Phase 2: RENDER (drain results, upload, draw, present) ────────────
    audio.updateListener(player, timer.partialTick)
    if world:
      world->doLightingUpdates(frame.drainBudget(LightRegions))
      world->pumpChunkPublish()
    mailbox.drainRender();      // mesh uploads (budgeted), shader-compile results,
                                // texture stream, finished chunk loads, GL uploads
    setSwapPacing(...); pumpAndPresent()
    thirdPerson wall check
    if !skipGameRender:
      luaRenderFrame hook; interactionManager->update; gameRenderer->onFrameUpdate
    // ── Phase 3: PACE ─────────────────────────────────────────────────────
    frame.pace();               // vsync (already via SwapBuffers) + optional sleep
    // ── Phase 4: DIAGNOSTICS ──────────────────────────────────────────────
    profiler.recordFrame(tickDuration, frames); toast.tick; ++frames
    profiler.emitStallTraceIfSlow()     // single mutex, single file
```

Concretely vs. today:

- **The mailbox absorbs every "drain N results" call.** `WorldRenderer::compileChunks`
  keeps its own mesh upload loop (it already has the right FrameBudget logic), but the
  **budget source moves to a shared per-frame cap** so the render phase cannot
  over-consume: `FrameBudget` gains a per-frame deadline set once by `FramePipeline`
  (`frame.remaining()`), instead of every subsystem calling `fromMs(...)` with its own
  number. Same for `ChunkCache::integrateFinishedLoads(budget)` and
  `World::doLightingUpdates(maxDirtyRegions)` — all driven off one deadline.
- **No blocking waits in the hot loop.** Audit each hot-path call:
  - `drainCompleted` (WorkerHandoff) → non-blocking lock+exchange, fine, becomes a
    `Channel::tryPop` batch drain.
  - `LightingEngine::drainDirtyRegions` → non-blocking, fine.
  - `ShaderCompileService::compileBlocking` used from `ProgramCache` **blocks the main
    thread** waiting for a worker; this is a real stall risk during pack load. Keep it
    only for the fallback/urgent path; the normal path must be `submit` + drain the
    `completed_`/channel each frame (ProgramCache already has `pollCompleted`-style
    plumbing; drive it from `mailbox.drainRender()`).
  - `World::finishLightingUpdates()` (`doLightingUpdates(MAX)`) is only reached on the
    world-load path — keep it, but route it through the frame's loading budget rather
    than an unbounded drain.
- **Timer / frame pacing.** `Timer` and its paused-partialTick preservation stay
  untouched (`Timer.hpp` is already self-contained and correct). Pacing stays
  vsync-driven (`setSwapPacing` → `glfwSwapInterval`; present = `SwapBuffers` blocks).
  Keep the existing present-before-draw ordering inside `runRenderPhase`
  (pumpAndPresent at :688, then `onFrameUpdate` render at :700) — that is the port's
  established double-buffer rhythm. The only addition: `FramePipeline::pace()` owns
  the "window inactive → sleep 10ms" path and the `fpsLimit=0` unlimited-spin case
  (optional adaptive sleep, gated behind an option so it does not change vanilla "Max"
  semantics).
- **Stall trace.** The two inline `static std::mutex stallMutex` blocks
  (`run()`:810, `runRenderPhase()`:729) collapse into `FrameProfiler` — one mutex, one
  log file, one phase-enum, and per-phase `traceLight/tracePresent/traceRender`
  timings recorded by `FramePipeline`. Diagnostics (heartbeat, fps window) become
  explicit Phase-4 calls instead of loop-body statements.

---

## 5. Cross-thread data rules

### 5.1 Ownership / affinity policy (write these down, enforce with asserts)

1. **Main-thread-owned (affinity, single-threaded, no locks):** `World` session graph,
   entity lists, screen stack, `NetworkHandler` application state, `Timer`, `GameOptions`,
   all `RenderCore`/GL state, `Tessellator`, texture manager, audio *command submission*
   (push only), timer/particles, `WorldRenderer` builder list.
2. **Worker-owned-while-in-flight:** nothing outlives its task. The only exception is
   the **light arrays**: `Chunk` sky/block-light nibble arrays may be written by
   lighting workers **only while the chunk holds a render pin** (existing
   `tryAcquireRenderPin` protocol). Everything else in a `Chunk` (blocks, meta, block
   entities, heightmap) is main-thread. Main-thread reads of light arrays during
   entity render are safe because a pin guarantees the worker and main thread never
   write the same cell concurrently (the region-conflict claim in `tryClaimBox`
   enforces disjointness between workers; pins enforce main-vs-worker).
3. **Never touch live world state on a worker.** `RegionSnapshot` is the only
   sanctioned worker view of chunks (mesh). Block entities are never dereferenced off
   thread — the job records positions and the main thread resolves pointers at upload
   (`ChunkMeshJob::blockEntityPositions`, `ChunkMeshJob.hpp:27`). Keep this doctrine;
   extend it to any future worker.
4. **GL-context rules:**
   - All GL object creation/deletion/upload happens on the main window context
     (main thread) in the render phase, **never in a compute task**.
   - Shader compilation happens only on `GlCompile` domain workers holding their own
     hidden shared contexts. **Blobs cross threads; live `GLuint`s never do.**
     `ShaderCompileService` already returns `ProgramBinaryBlob`; the main thread
     applies. This is the pattern to keep everywhere.
   - GL-shared-context teardown must be ordered before the primary context is
     destroyed (section 7), or the driver invalidates the share group.

### 5.2 The canonical handoff: `Channel<T>` (produce-on-worker, consume-on-main)

New: `util/concurrent/Channel.hpp`. Replaces `WorkerHandoff::completed_`,
`LightingEngine::outbox_`, and (in time) `Connection`'s mutex-deque.

```cpp
// Bounded, prioritized, MPMC with stop_token-aware blocking push.
// Producer: workers block on push() when full (natural backpressure throttle).
// Consumer: main thread uses tryPop batch drains against the frame budget.
template <typename T>
class Channel {
  bool push(T value, TaskPriority priority = Normal); // blocks; false on stop
  bool tryPush(T value, TaskPriority priority);       // non-blocking
  bool tryPop(T& out);                                // non-blocking
  void drain(std::vector<T>& out, std::size_t max);   // batch, non-blocking
  std::size_t size() const;                           // atomic, for F3 + backpressure
  void reset();                                       // drop queued, notify producers
};
```

Policies on top of the primitives:

- **Backpressure = bounded capacity + version stamps.** Each domain channel has a
  capacity and an in-flight cap (`WorldRenderer` already does the right thing:
  `targetInFlight = workerCount * 3`, `pendingMeshUploads_` double buffer).
  Producers block at capacity, which throttles *cheap-to-produce* work (mesh captures,
  light boxes) so the consumer never drowns.
- **Stale-result dropping** uses the existing version/generation pattern
  (`ChunkMeshJob::version` vs `ChunkBuilder::version`); generalize to a
  `Generation` stamp on the channel so dropped results don't wake the consumer.
- **Named priorities.** New enum in `ThreadCoordinator`:
  `Urgent` (= near-camera mesh, `INT_MIN` as today), `High` (lighting, chunk loads near
  player), `Normal` (distant mesh, decoration), `Low` (prefetch), `Idle`. Every
  submit site maps its current magic `priority` int to the enum.
- **GPU uploads deferred to the GL thread (= the render phase).** Mesh jobs finish on
  Compute workers → land in the mesh `Channel` → `WorldRenderer::compileChunks`
  drains them in Phase 2 against `frame.remaining()` and calls `ChunkBuilder::uploadMesh`
  on the main context. Textures streamed by `ResourceDownloadThread`/`ImageDownload`
  land in a texture `Channel` → main-thread `TextureManager` uploads. This is the
  entire "defer uploads to the GL thread" story in this engine, and it is already the
  architecture — it just needs to be the *only* architecture (no direct worker→GL
  calls anywhere).

### 5.3 Per-subsystem mapping onto the channel

| Producer | Channel | Consumer (frame phase) |
|---|---|---|
| Mesh workers (Compute) | `meshChannel` | `WorldRenderer::compileChunks` (Phase 2, budgeted) |
| Lighting workers (Compute) | `lightOutbox` (replaces `outbox_`) | `World::doLightingUpdates` (Phase 2) |
| Chunk loader (Compute) | `loadedChunks` (replaces `pendingLoads_`+done flags) | `ChunkCache::integrateFinishedLoads` (Phase 2, budgeted) |
| Save worker (I/O) | `saveAck` (optional) | main, for error surfacing |
| Shader compile (GlCompile) | `compileDone` (wraps `completed_`) | `ProgramCache` poll (Phase 2) |
| Network reader (NetIo) | `packetIn` (replaces `readQueue_`) | `ClientNetworkHandler`/`Connection::tick` (Phase 1) |
| Network writer (NetIo) | `packetOut` (replaces `sendQueue_`) | writer thread pops |
| Audio (pinned) | `audioEvents` | `audio.tick()` (Phase 1) |

---

## 6. Shutdown — a coherent stop/join protocol

New: `util/concurrent/Lifecycle.hpp/.cpp` (or fold into `ThreadCoordinator::shutdown()`).
Every long-lived thread owner (WorkerPool per domain, audio backend, log dispatcher,
network connections, ShaderCompileService, pipeline directory watcher, resource
downloader, session one-shots) **registers** with the coordinator at creation and is
torn down in one ordered call. The ordering is reverse-of-creation and dependency-aware:

1. **Fence (main thread):** set every domain's `accepting = false`; stop enqueueing
   new work; `scheduleStop()` for the server coordinator.
2. **Unblock:** for every registered blocking thread, trigger its unblock primitive
   *before* requesting stop — shutdown sockets (`Connection::interrupt()`), wake
   `XAudio2` CVs, `glfwPostEmptyEvent()` on shader-compile windows, wake logger CV.
3. **Stop + drain:** `request_stop` on all threads; let in-flight tasks finish
   (do not force-kill); `Channel::reset()` drops queued-but-unstarted work.
4. **Join with watchdog:** join each registered thread; on timeout log once and leak.
5. **Session-clear semantics (the part that must be ordered):** a worker must never
   observe a dying `World`. Therefore: **stop workers → `WorldSession` teardown
   (`flushRetired`, `setWorld(nullptr)`, destroy pools/lighting) → destroy
   contexts/GL.** This is the exact ordering `World::~World` (calls `lighting_.stop()`)
   and `ShaderCompileService::stop` already imply; make it explicit and single-sourced
   so `Minecraft::stop()`/`gameCrashed` and the `std::exception` handlers in `run()`
   all funnel through `Lifecycle::shutdown()` instead of each subsystem's destructor.

Special cases to call out:
- `Connection` reader/writer join (blocking sockets): already ordered correctly via
  `interrupt()`/`joinThreads()`; register the join so the shutdown coordinator waits on
  them before destroying `NetworkHandler` state.
- `ShaderCompileService::stop` must run **before** the primary GL context is destroyed
  (shared-context share-group teardown).
- Detached `std::thread` sites (`ImageDownload.cpp:7`, `ClientDiagnostics.cpp:235`)
  become registered one-shots on the I/O domain so shutdown can wait for them.

---

## 7. Incremental migration path (must keep the build green)

The codebase is mid-refactor and cannot be broken. Every step below is **additive or
mechanical**, compilable on its own, and reversible. Order is deliberately
lowest-risk-first and proceeds *bottom-up*: infrastructure → consumers → the main
loop → teardown.

| Step | What | Files | Build-safe? |
|---|---|---|---|
| **1. Infrastructure (pure additive)** | Add `ThreadCoordinator`, `ThreadBudget`, `Channel`, `ThreadNames`, `Lifecycle`. Configure the coordinator in `Minecraft::init()` but wire nothing to it yet. | new files under `util/concurrent/` | Yes — no existing call site changes. |
| **2. Kill `recommendedThreadCount`** | Mechanical: replace every `recommendedThreadCount(...)` / `hw-2` with `coordinator.budget()`-derived counts. Delete the function. | `client/render/chunk/ChunkBuilder.hpp:156`, `world/light/LightingEngine.cpp:49`, `world/chunk/ChunkCache.cpp:217,224`, `client/gl/ShaderCompileService.cpp:47`, `client/gl/GLCore.cpp:289` | Yes — pure constant change; thread counts identical on 8-core machines, no behavior delta. |
| **3. Canonical mesh channel** | Rewire `ChunkMeshScheduler` to `Channel` + Compute pool; move `completed_` out of `WorkerHandoff`; keep `WorldRenderer::compileChunks` untouched except budget source. Add in-flight caps per domain. | `client/render/chunk/ChunkBuilder.hpp`, `util/concurrent/WorkerHandoff.hpp`, `client/render/world/WorldRenderer.cpp:727-825` | Yes — `WorldRenderer`'s public API unchanged. |
| **4. Light outbox → channel** | Replace `LightingEngine::outbox_`/`drainDirtyRegions` with a `Channel`, keep `tryClaimBox` conflict logic. | `world/light/LightingEngine.cpp/.hpp` | Yes — `World::doLightingUpdates` signature unchanged. |
| **5. Chunk loader/save onto domains** | Loader → Compute pool (per-thread generator map preserved), save → I/O pool; integrate loads via budgeted channel. | `world/chunk/ChunkCache.cpp/.hpp` | Yes. |
| **6. Shader compile count + poll** | Use GlCompile-domain count; drive `ProgramCache` normal path through `compileDone` channel drained each frame (keep `compileBlocking` for fallback only). | `client/gl/ShaderCompileService.cpp`, `client/gl/ProgramCache.cpp` | Yes — fallback path unchanged. |
| **7. Ephemeral threads → I/O domain** | `LoginScreen`, `SessionValidator`, `MultiplayerConnector`, `ClientNetworkHandler`, `ImageDownload`, `SessionRestore` one-shots become `coordinator.pool(Io).submit(...)` (or registered short-live threads). Delete detached `std::thread` spawns. | `client/gui/auth/*`, `client/session/SessionValidator.cpp`, `client/multiplayer/*`, `client/texture/ImageDownload.cpp`, `client/auth/microsoft/SessionRestore.cpp` | Yes — call sites are self-contained. |
| **8. Main-loop restructure (the big one)** | Extract `FramePipeline`; move retire/flush/fps/heartbeat/stall-trace into phases; add `TaskMailbox`; unify budgets on one per-frame deadline. **Do not reorder tick/render.** | `client/Minecraft.cpp` `run()/tick()/runRenderPhase()`, new `client/util/FramePipeline.*`, `client/core/TaskMailbox.*`, `client/util/FrameProfiler.*` | Riskiest; do last, behind a flag, keep old `run()` body as reference until the new one is character-identical in phase order. |
| **9. Lifecycle teardown** | Register all owners, implement ordered `shutdown()`, route `Minecraft::stop()`/crash handlers through it. | `client/Minecraft.cpp` `stop()`, `client/host/ServerProcessCoordinator.cpp`, `server/MinecraftServer.cpp`, `util/logging/Logging.cpp` | Yes. |
| **10. Server-side parity** | Apply the same coordinator to `server-main.cpp`/`MinecraftServer` (budget once, register threads) — optional, lower priority since server is a separate process. | `server/*`, `src/server-main.cpp` | Yes. |

### 7.1 Risk register

| Risk | Where it bites | Mitigation |
|---|---|---|
| **Deadlock at shutdown** | Joining a worker that is blocked on a socket/GL/disk CV. | Unblock-first rule (6.2), watchdogged joins (6.4), Lifecycle ordering (6.5). Known today: `LightingEngine::stop` already locks `queueMutex` then joins — keep that lock-discipline in the channel version. |
| **Light array race** | Worker writing `Chunk::setLight` while main thread reads it in entity render, or two workers on adjacent cells. | Pin protocol already enforces disjointness; make it a documented invariant + debug assert (`ASSERT_LIGHT_OWNER`). Do not "optimize away" pins. |
| **Chunk unload vs pinned worker** | Unloading a chunk a mesh/light worker still references. | Existing render-pin refcount + `ChunkBuilder::retired`/`meshJobInFlight` gating (ChunkMeshJob.hpp:34-49) must be preserved; version stamps drop stale results (5.2). |
| **GL-context loss** | Destroying a shared context while shader workers still run, or uploading from a compute task. | GlCompile teardown before primary context destroy (6.5); blobs-not-objects rule (5.1.4); never GL from Compute. |
| **Oversubscription recurrence** | A new subsystem calls `hardware_concurrency()` again. | Delete `recommendedThreadCount`; make `coordinator.pool()` the only legal source; code-review rule. |
| **Performance regression from shared budget** | Compute pool undersized on small CPUs, or a blocking I/O task starving tessellation. | Compute/IO separated (3.2); counts configurable; `totalPending()` on F3 to watch queue depth; keep `targetInFlight` caps. |
| **Behavior drift in `run()`** | Refactor reorders ticks or renders → shader/parity regressions (shadow order, partial-tick interpolation). | Keep the tick-order INVARIANT comment; character-compare phase order against the old body; ship behind a flag in step 8. |
| **Main-thread stall from blocking drains** | `compileBlocking` in the pack-load path, `finishLightingUpdates` unbounded drain. | Route both through frame budget / keep fallback-only blocking (4.1). |

---

## 8. Gap analysis — what building blocks are missing

Present today and keep:
- `WorkerPool` (single-pool primitive), `WorkerHandoff` (produce/consume shape),
  `FrameBudget` (time slicing), `RegionSnapshot` (worker isolation),
  `ShaderCompileService` blob pattern (context-affine compile), light pin protocol.

Missing (each is a small, additive `util/concurrent/` unit):

1. **`ThreadCoordinator` + `ThreadBudget`** — the single global budget and the domain
   pool registry. The entire "coordination" ask. (Highest priority.)
2. **`Channel<T>`** — bounded, prioritized, stop-aware handoff with backpressure.
   Today every pair re-invents a mutex+deque. (Second priority.)
3. **`std::atomic<int>` global task counter shared by pools** — `coordinator.totalPending()`
   for F3 + debug; replaces per-subsystem counters (`ChunkBuilder::chunkUpdates`).
4. **Thread naming for debugging** — `ThreadNames::set("mesh-3")` via
   `SetThreadDescription`/`pthread_setname_np`, plus a thread-local **domain tag**
   (`TL_DOMAIN`) so `ASSERT_MAIN_THREAD()` / `ASSERT_GL_THREAD()` /
   `ASSERT_NO_GL_FROM_COMPUTE()` are cheap and catch affinity violations in debug.
5. **`Lifecycle` / ordered teardown registry** — watchdogged join, unblock-first,
   reverse-of-creation order. Today shutdown correctness is emergent (and two stall
   mutexes say so).
6. **`Generation`-stamped stale-result dropping** — generalize the mesh
   version-stamp idea to all channels so dropped work never wakes consumers.
7. **A lock-free SPSC queue — deliberately NOT in the list.** The workload
   (b1.7.3 chunks, packets, audio events) is modest; a bounded mutex+CV `Channel` is
   correct, auditable, and has exactly the backpressure semantics we want. Build
   lock-free only if profiling later shows real contention on a hot consumer drain.
8. **A unified `FrameBudget` deadline** shared per-frame (vs. per-subsystem
   `fromMs`), so the frame pipeline owns the frame's total time budget and every
   drain respects it.

---

## 9. Open questions for the plan/master stages

1. Should lighting get its **own lane inside the Compute pool** (a sub-priority and a
   separate bounded channel) or share one queue with mesh? (Proposal: own lane —
   lighting boxes have different priority and conflict semantics than tessellation.)
2. Is `fpsLimit=0` (Unlimited) allowed an optional adaptive-sleep to avoid a hot spin
   loop, or is hot-spinning considered vanilla parity? (Proposal: keep parity; add
   option.)
3. Does the **dedicated server process** need the same coordinator now, or is a
   deferred step acceptable given it is a separate binary? (Proposal: defer, step 10.)
4. The audio backend's 4 pinned threads (worker + 2 decoders + long decoder) — keep
   as-is for parity, or fold decode onto I/O domain? (Proposal: keep as-is for now;
   it's off-budget-critical and API-affine.)
5. Confirm the present-before-draw ordering in `runRenderPhase` is intentional
   (port's double-buffer rhythm) before the FramePipeline refactor hard-codes it.
