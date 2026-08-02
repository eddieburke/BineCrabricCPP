# Council Concurrency Inventory — complete threading/multithreading survey

Review-only inventory (NO code edits). Every file:line reference is against the repo
as it stands. This is the facts base for the planned "massive refactor of
multithreading + main-thread handling".

## TL;DR

- The "main thread" is a single serial loop (`Minecraft::run`) that does BOTH tick and
  render, plus per-frame pumps of four independent worker systems. Nothing is
  coordinated; each subsystem has its own thread pool, its own queue, its own budget.
- Thread counts are computed **independently** by at least **5** different sites that each
  call `WorkerPool::recommendedThreadCount` (or a hand-rolled `hardware_concurrency()-2`)
  and each **pretends to be 1 of 3 competing pools**. Total CPU oversubscription.
- `WorkerPool::drain()`/`cancelAll()` **block the main thread** (teleport reload,
  world clear, ChunkCache dtor, save-on-exit).
- Main-thread is spin-blocked by `ChunkCache::retireFromLighting` until render-pin
  holders (mesh workers + lighting workers) finish.
- There is a real **GL-state data race**: mod block models drawn on mesh **worker**
  threads call `core::setAlphaTestRef` -> global `g_alphaTestRef`.
- `GLCore::init()` has a plain-bool `g_loaded` guard race between main + 4 shader-compile workers.
- `Minecraft::stop()` ends with `std::_Exit(0)` — shutdown that skips ~World and most
  destructors; the whole teardown is a "drain what we can, then hard-exit" plan.
- Detached threads that are never joined/owned: ImageDownload (per skin), SessionValidator
  check, hang watchdog, DedicatedServerGui guiThread.

---

## 1. The main loop (`Minecraft::run`)

`src/net/minecraft/client/Minecraft.cpp:749-843` — `Minecraft::run()`.

- `:760` `while(running.load())` — plain `std::atomic<bool>` (Minecraft.hpp:202).
- `:765` `screenStack_.flushRetired()`, `:769` `multiplayerSession_.flushRetired()` —
  deferred-destroy fences each frame.
- `:784-787` Lua hook mutates `timer.tps/tpsScale` each frame.
- `:790/:793` `timer.advance()` (`src/net/minecraft/client/util/Timer.hpp:14-48`). Classic
  catch-up accumulator; `ticksThisFrame` clamped to 10. When paused, `partialTick` is
  preserved but advance() still runs (line 788-791).
- `:796-805` `for i in ticksThisFrame: tick()`.
- `:807` `runRenderPhase(nanoTime()-tickStart, frames, fpsWindowStart)`.
- `:809-825` frame-stall log with a static mutex (only the main thread writes it).

`tick()` — Minecraft.cpp:593-669. One giant serial phase:
- `:595` serverProcessCoordinator_->tick() (polls external process, never a thread).
- `:597-598, 663-664, 667-668` Lua hooks.
- `:599` `msauth::tickRestoreSavedAccount` (joins the restore worker on main thread).
- `:600` audio.tick, `:610` inGameHud.tick, `:611` gameRenderer->updateTargetedEntity.
- `:643` `textureManager.bindTexture(...)` GL call here.
- `:650-662` InputSystem beginFrame -> screenStack.tickScreens -> multiplayerSession.tick
  (network drain) -> pollGame.
- `:665` `runWorldSimulation()` (:555-592): world tick/entities/displayTick/particles.

`runRenderPhase()` — Minecraft.cpp:670-748. Per-frame serial budget:
- `:679` `audio.updateListener`.
- `:681` `world->doLightingUpdates()` -> drains LightingEngine outbox
  (`src/net/minecraft/world/World.cpp:711-717`).
- `:682` `world->pumpChunkPublish()` -> ChunkCache::pumpChunkPublish -> integrateFinishedLoads(32)
  (`src/net/minecraft/world/chunk/ChunkCache.cpp:472-474`).
- `:688` `DisplayManager::pumpAndPresent()` — **swap happens at the START of the render
  phase, before the frame is rendered** (render-for-last-frame latency + present).
- `:700` `gameRenderer->onFrameUpdate(tickDelta)` — actual scene render. Inside this,
  `WorldRenderer::compileChunks` (the chunk mesh pump) runs on the main thread
  (`src/net/minecraft/client/render/GameRenderer.cpp:1168`).
- `:721-727` FPS counter reads/writes `ChunkBuilder::chunkUpdates` (an `inline static int`,
  ChunkBuilder.hpp:84) — incremented on the main thread only (ChunkBuilder.cpp:353).

**Conclusion: there is no separate tick thread / render thread split; "main thread
handling" is one monolithic loop with per-frame slices given to 4 worker backends
(lighting, chunk publish, mesh build, network drain), each with an independent budget.**

### FrameBudget (the slice primitive)
`src/net/minecraft/util/concurrent/FrameBudget.hpp:5-14` — plain struct, `deadline` +
`minItems`, `hasRemaining(done)`. Used in two places, both main-thread:
- WorldRenderer.cpp:738-739 (upload budget: 3ms / 6ms, min uploads 1-16).
- WorldRenderer.cpp:783-785 (capture budget: 1ms / 2ms, min captures).
Only chunks use FrameBudget. Lighting (`doLightingUpdates(maxDirtyRegions=128)`) uses a
fixed count, network drain uses a fixed 3ms (`Connection.cpp:200`). No shared clock, no
frame-time normalization, no GPU-awareness anywhere.

---

## 2. The scheduler primitives

### 2.1 `WorkerPool` — `src/net/minecraft/util/concurrent/WorkerPool.hpp`
- ctor `:19-27` spawns `threadCount` `std::jthread`s running `workerLoop(stop)`.
- `submit(task, priority)` `:37-43`; priority queue `:71-82` (lower int first, sequence tiebreak).
- **`cancelPending()` `:45-48` — `queue_ = {}` DROPS every queued-but-not-started task
  with no signal to owners.** Owners who counted the job (e.g. `meshJobInFlight=true`)
  never get a completion -> `startMeshJob` rejects that chunk forever until re-dirtied.
- **`drain()` `:50-53` — spins on `idle_` until `queue_.empty() && activeCount_==0`. Can
  block indefinitely if a worker is stuck on a slow job (terrain gen, disk read).**
- **`recommendedThreadCount(competingPools=1,reservedThreads=1,maxThreads=8)` `:61-68` —
  the core flaw.** Every caller passes its own guess of "competing pools", so each pool
  independently budgets `(hw-reserved)/poolCount`. With 5 pools all assuming 3
  competitors, 12+ threads are created on a 16-thread machine — plus shader compile
  `hw-2`, audio, network, logging. No global executor/capacity authority.
- `workerLoop` `:83-108` catches all exceptions silently.
- Threads park on `wake_` CV `:88` — idle workers consume a slot but no CPU.

### 2.2 `WorkerHandoff` — `src/net/minecraft/util/concurrent/WorkerHandoff.hpp`
- ctor `:14` default = `recommendedThreadCount(2,2)`.
- `enqueue` `:22-30`: runs `work(*job)` off-thread, pushes job into `completed_` under
  `mutex_`. The `Job` objects live on (shared_ptr) until main-thread `drainCompleted()`.
- `drainCompleted` `:31-34` main-thread only.
- **`cancelAll()` `:35-40` = `cancelPending()` + `drain()` + clear. `drain()` BLOCKS the
  caller (main thread) until every in-flight job finishes.** This is called from
  `WorldRenderer::clearSections` (WorldRenderer.cpp:369) on teleport/reload/clear and from
  the `~WorkerHandoff` dtor.
- `idle()` `:41-47` = pool empty AND completed empty — used by compileChunks to decide
  "backlog done".

### 2.3 Thread-count computation sites (each independent)
| Site | Call | Result on 16-logical |
|---|---|---|
| ChunkBuilder.hpp:156 | `recommendedThreadCount(3,2,6)` | 14/3=4.6 -> **4** |
| ChunkCache.cpp:217 | `recommendedThreadCount(3,2,4)` | 4.6 -> **4** (clamp 4) |
| ChunkCache.cpp:224 | savePool fixed | **1** |
| LightingEngine.cpp:48-49 | `max(1, recommendedThreadCount(3,2,3))` | 4.6 -> **3** |
| ShaderCompileService.cpp:46-48 | `hw>2 ? hw-2 : 1`, `min(count,4)` | 14 -> **4** |
| GLCore.cpp:288-290 | `hw>2 ? hw-2 : 1` (driver hint, no threads) | 14 |

Four pools (mesh 4 + loader 4 + save 1 + light 3 = **12**) all believe they are "1 of 3
pools". If all were actually budgeted globally, a sane total would be ~hw/2 to hw.

---

## 3. Chunk mesh building — `ChunkBuilder` / `ChunkMeshScheduler`

- Pool owner: `ChunkMeshScheduler` in `src/net/minecraft/client/render/chunk/ChunkBuilder.hpp:120-158`;
  `handoff_{WorkerPool::recommendedThreadCount(3,2,6)}` `:155-156` (4 workers @16th).
- `enqueue` `:122-133` wraps `ChunkBuilder::buildMesh`; `enqueueNear` `:134-137` uses
  `priority = INT_MIN` so near-camera edits preempt the distance ring backlog.
- Main-thread pump: `WorldRenderer::compileChunks` (WorldRenderer.cpp:727-825), called from
  `GameRenderer.cpp:1168` inside the render phase.
  - `:733-737` `workerCount = meshScheduler_.workerCount()`; backlog; `minUploadsPerFrame`.
  - `:738-739` upload `FrameBudget` (3ms/6ms).
  - `:743-766` `processUpload`: `builder->uploadMesh(*job)` — **GL upload on main thread**.
  - `:771` `drainCompleted()`; `:774` leftover jobs deferred to next frame.
  - `:776` `targetInFlight = workerCount * (force?6:3)` — i.e. up to 12 jobs in flight.
  - `:783-785` capture budget (1ms/2ms) bounds `startMeshJob`.
- Capture: `ChunkMeshJob::capture` (ChunkBuilder.cpp:132-197) runs on the MAIN thread,
  acquires per-chunk render pins (`chunk->tryAcquireRenderPin()` `:163`); actual block/light
  snapshot copy happens on the WORKER (`buildMesh` -> `captureSnapshot`, ChunkBuilder.cpp:253-258).
- Pins released by `ChunkMeshJob::~ChunkMeshJob`/`releasePins` `:242-252` (on worker when
  snapshot taken; else on main when job destroyed).
- Upload: `ChunkBuilder::uploadMesh` (ChunkBuilder.cpp:352-415) main-thread GL: region
  buffer upload `:366`, `BlockEntityRenderDispatcher::instance()` `:379`, `world->getBlockEntity`
  `:381`, `TextureRegistry` GL resolves in renderModChunkMeshes.

**Hazards**
- `clearSections()` (WorldRenderer.cpp:368-400) calls `meshScheduler_.cancelAll()` (`:369`)
  -> **blocking `drain()` on the main thread** while workers finish current builds. During a
  teleport this is a visible hitch proportional to the slowest in-flight mesh job.
- `cancelPending()` drop + `meshJobInFlight=true` (ChunkBuilder.hpp:105, set at
  WorldRenderer.cpp:577): a cancelled-in-queue job never clears the flag, so the section
  is stuck "in flight" until something re-dirties it (or forever if it was already clean).
- Backlog target `workerCount*3 = 12` jobs in flight means the worker pool is saturated
  for most of a region load; priorities (ring vs near) only order the queue, they do not
  preempt running jobs, and there is no per-frame cap on worker-side work.
- Worker threads read `Block::BLOCKS`/`BLOCKS_WITH_ENTITY`/`BLOCKS_LIGHT_OPACITY` static
  arrays and `job.opts`/`job.blockRenderLayers` captured at enqueue time (safe), but **mod
  block world draws on workers touch global RenderCore state** (see §10.4).

---

## 4. LightingEngine — own jthread pool

`src/net/minecraft/world/light/LightingEngine.hpp` + `.cpp`.

- ctor `.cpp:46-57`: `workers = max(1, recommendedThreadCount(3,2,3))` (3 @16th),
  `std::vector<std::jthread> threads_` (`:95`).
- Producers: main thread calls `push()` (`.cpp:58-91`) under `queueMutex_`, `workCv_.notify_all()`.
- Consumers: `threadLoop` (`.cpp:187-206`) claims a non-conflicting box via `tryClaimBox`
  (`:149-170`), runs `runUpdate`, releases pins, `releaseClaimedBox`.
- Light nibbles written through `ChunkNibbleArray` CAS (ChunkNibbleArray.hpp:35-51); reads
  atomic relaxed byte loads (`:28-29`).
- Chunk lifetime: workers pin via `tryAcquireRenderPin` (Chunk.hpp:205-224); unload spin-waits
  (see §10.2). Registry map guarded by `registryMutex_` (Chunk.hpp members `:90`).
- `drainDirtyRegions` (`.cpp:113-126`) — main-thread outbox pull; `doLightingUpdates` called
  each render phase (World.cpp:711-717, 128 regions) and after world load / server tick
  (MinecraftServer.cpp:378, 518).
- stop: `stop()` `.cpp:131-148` — request_stop under `queueMutex_` (correct), `threads_.clear()`.

**Hazards**
- Same-thread-count fiction (`recommendedThreadCount(3,2,3)`) — 3 more threads.
- Box-conflict scanning is O(queue * activeBoxes) on `queueMutex_`; with 200k queue cap
  (`:70`) pushes/stops contend with the main thread's `doLightingUpdates` lock of `outboxMutex_`.
- Workers hold raw `Chunk*` in `registry_`/pinCache; `unregisterChunk` only protects the map,
  not the pointer lifetime of an in-flight box against `ChunkCache::unloadChunk`
  (which busy-waits on the pin — §10.2 — so it is "safe" but stalls the main thread).
- `queuePropagationBox`/`runUpdate` self-resubmit splits of huge boxes (`:286-310`) — a
  large sky update can keep 3 workers busy long after the frame that requested it.

---

## 5. ChunkCache — loader/save pools

`src/net/minecraft/world/chunk/ChunkCache.cpp` + `.hpp`.

- `ensureLoaderPool` `.cpp:212-219` — `recommendedThreadCount(3,2,4)` (4 @16th).
- `ensureSavePool` `.cpp:220-225` — fixed **1** worker.
- `requestChunkAsync` `.cpp:226-249` — submits `produceChunk` to loader pool; result
  `PendingLoad{chunk, atomic done}`; folded in by `integrateFinishedLoads` (`.cpp:250-288`,
  budget 2/tick, 32/pump, 4/prefetch).
- Terrain gen: per-thread generator clones `workerGenerator()` (`.cpp:85-105`,
  `workerGenerators_` map guarded by `workerGeneratorMutex_`, `.hpp:86-87`).
- `ioMutex_` recursive (`.hpp:84`) serializes storage_ access + decoration across loader
  workers, save worker, and main thread.
- Saves: `enqueueSerializedWrite` (`.cpp:299-348`) submits to savePool; completion via
  `pendingSaveWrites_` atomic + `saveCompleteCv_`; `waitForPendingWrites` (`.cpp:349-355`)
  blocks until drained.
- Dtor `.cpp:26-35`: `waitForPendingWrites()` then loader `cancelPending()+drain()` then
  save `drain()` — **full blocking teardown**.
- `retireFromLighting` (`.cpp:61-67`): `while(!chunk->beginRenderEviction()) sleep(200us)` —
  **main-thread busy-wait** until render pins drop (mesh + lighting workers).

**Hazards**
- Loader pool (4) + save pool (1) are two more independent budgeters; total with mesh(4)
  and lighting(3) is 12 worker threads that all self-budget against the same hardware.
- `cancelPending()` drops queued chunk loads silently; their `PendingLoad` remains in
  `pendingLoads_` with `done==false` forever — `requestChunkAsync` will not re-issue, and
  `integrateFinishedLoads` scans but skips them (leak of queue slots + memory).
- `decorate()` is called on the main thread (adoptChunk, `.cpp:176-196`) and holds `ioMutex_`
  while calling `generator_->decorate` (`.cpp:380-383`) — a heavy, serial main-thread cost.
- Busy-wait in `retireFromLighting` couples main-thread world unload to worker progress
  without a CV.
- `workerGenerators_` are never pruned: a per-thread generator clone per pool thread, per
  world; across dimension switches clones accumulate (memory).

---

## 6. World::save — `std::async` level.dat writer

`src/net/minecraft/world/World.hpp:429` `std::future<void> asyncSaveFuture_;`
`src/net/minecraft/world/World.cpp:300-339`.

- `save(blocking=false)` `:325-339`: if no future in flight, snapshot properties+players and
  `asyncSaveFuture_ = std::async(std::launch::async, ...)` writing level.dat off-thread.
- `save(blocking=true)` `:306-324`: `asyncSaveFuture_.wait()` then write synchronously.
- The comment at `:307-311` acknowledges the real teardown hazard: `Minecraft::stop() ->
  std::_Exit(0)` skips `~World`, so a detached async save could lose the write; hence the
  blocking path.

**Hazards**
- One-off `std::launch::async` = a short-lived extra thread that is NOT accounted for in any
  budget; a new one spawns each autosave while the previous completed.
- `wait_for(0)` polling every autosave; two concurrent saves are coalesced by "skip if in
  flight" (fine) but the snapshot is a `std::vector<PlayerEntity*>` copied by value — the
  pointers are read concurrently with main-thread player mutation (no synchronization on the
  player objects' data; writers touch `players` and `properties_` from the main thread while
  the async thread re-reads `dimensionData_`).
- `dimensionData_->save` (RegionWorldStorage/AlphaWorldStorage) runs off-thread while the
  main thread may call `saveLevelProperties()` (World.cpp:294-299) — the storage classes are
  not internally locked.

---

## 7. Network

### 7.1 `Connection` — reader_/writer_ threads (client AND server)
`src/net/minecraft/network/Connection.cpp` + `.hpp`.

- ctor `.cpp:120-133`: spawns `reader_` (`:131`) and `writer_` (`:132`). **2 threads per
  connection, including the client's single connection and every server connection.**
- `readLoop` `.cpp:262-281`: blocking `recv`; pushes packets into `readQueue_` under
  `readMutex_`.
- `writeLoop` `.cpp:282-333`: batches from `sendQueue_`/`delayedSendQueue_` under
  `writeMutex_`, `writeCv_.wait_for(20ms)`, writes + flushes; `requestDisconnect` closes
  receive side to unblock the reader; `shutdownSocket` after.
- `tick()` `.cpp:184-228` (main thread): overflow/timeout checks + **time-boxed drain
  (3ms / min 8 / max 4096 packets) then `packet->apply(*handler)` on the main thread**.
- `joinThreads` `.cpp:352-360` — correctly avoids self-join (checks `get_id()`) so a
  Connection can be destroyed from the writer thread during reconnect.
- Shared state: `open_` atomic, `networkHandler_` atomic, queues under mutexes,
  `sendQueueSize_` atomic, `disconnectReason_`/args written by reader thread read by main
  (`:339`, `:225`) — plain strings shared without a lock (benign in practice but a race).

**Hazards**
- 2 OS threads per socket; a busy server with N players = 2N threads, none budgeted, all
  blocking on sockets. `readThreadCounter`/`writeThreadCounter` statics (`Connection.hpp:61-62`)
  suggest the author knew they'd multiply.
- Main-thread `tick()` drain is capped at 3ms but chunk packets (`ChunkDataS2CPacket`) are
  applied inline — a big join burst stalls the loop regardless of the budget.

### 7.2 Server accept loop — `ConnectionListener`
`src/net/minecraft/server/network/ConnectionListener.cpp`.
- ctor `:28` spawns `thread_` -> `listenLoop()` `:48-80` (blocking `accept`, creates
  `ServerLoginNetworkHandler` per socket — which constructs a `Connection` and therefore 2
  more threads).
- `stopAccepting` `:33-41` closes the socket to unblock accept and joins.
- `tick()` `:99-154` (server thread) snapshots pending/play connections under `mutex_` and
  ticks each handler (each tick pumps its Connection).

### 7.3 Server login verify — `verifyThread_`
`src/net/minecraft/server/network/ServerLoginNetworkHandler.cpp:149-178`.
- Spawns a `std::thread` doing the online-mode HTTP check; result parked under `verifyMutex_`
  into `deferredLoginPacket_`; joined at `:150-152` before a new one and in dtor.
- Shared `username_`, `serverId_` read on worker, main thread writes → plain fields,
  race-tolerant but unsynchronized.
- **A per-login transient thread, not budgeted; can outlive handler teardown while blocked
  on HTTP (no cancel).**

### 7.4 Client connect — `MultiplayerConnector`
`src/net/minecraft/client/multiplayer/MultiplayerConnector.cpp:17-61`.
- Spawns `thread_` doing DNS + `connectWithCancellation` + `bridge->connect(...)`.
  `bridge->connect` (`ClientNetworkBridge.cpp:122-148`) builds the `Connection` — spawning
  the client's reader/writer pair **on the connector thread**.
- **Cross-thread violation**: on cancel, the connector thread calls `bridge->disconnect()`
  -> `ClientNetworkHandler::disconnect` (`ClientNetworkHandler.cpp:115-127`) which calls
  `minecraft->setWorld(nullptr)` and `retireOwnedWorld()` — main-thread-owned state mutated
  from the connector thread (`setWorld` -> WorldSession::setWorld -> touches world/screens).
- `poll()` `:83-94` main-thread hands the bridge to `MultiplayerSession::adoptBridge`.

### 7.5 Client join-server auth — `joinServerThread_`
`src/net/minecraft/client/multiplayer/ClientNetworkHandler.cpp:247-256`.
- Spawned on handshake (main thread) doing `verifyJoinServer` HTTP; result under
  `joinServerMutex_`; consumed by `processPendingJoinServer` (`:55-83`) on the main thread.
- `joinServerCanceled_` atomic; joined in dtor (`:49-54`) and before re-issue (`:243-245`).
- `verifyJoinServer` reads `minecraft->session` (copied by value `:242`, good) but writes
  into a static auth cache if any — check auth:: for globals (§10.3).

---

## 8. Client subsystems

### 8.1 Shader compile workers — `ShaderCompileService`
`src/net/minecraft/client/gl/ShaderCompileService.cpp` + `.hpp`.
- `start()` `.cpp:41-61`: `count = min(hw>2 ? hw-2 : 1, 4)` (`:46-48`); creates a hidden
  GLFW window/context per worker sharing the main context (`createWorkerWindow` `:8-19`);
  spawns `workers_` (`:58-59`).
- `workerMain` `.cpp:224-249`: `glfwMakeContextCurrent`, `GLCore::ensureLoaded()`, then
  compile jobs (`runJobOnCurrentContext` `:190-222`) — GL on worker contexts.
- `stop()` `.cpp:63-86`: request stop, join all workers, destroy windows. Called from
  `Minecraft::stop()` (Minecraft.cpp:354) **before** `DisplayManager::destroy()` (good order).
- Main thread links binaries in `ProgramCache::poll()` (`ProgramCache.cpp:159-229`),
  polled from `GameRenderer::beginSceneCapture` (GameRenderer.cpp:707) and pack prewarm.
- `compileBlocking` `.cpp:144-177` — main thread can **block on a worker CV** (`:167`).

**Hazards**
- Independent budget `hw-2` on top of the 12 pool threads (§2.3).
- `GLCore::init()` guard `g_loaded` is a **plain static bool** (GLCore.cpp:121,145) written
  by main + every worker concurrently; all `GLCore::*` function-pointer statics are written
  under that race.
- GLFW windows: `glfwCreateWindow(..., shareWith=main context)` from the main thread while
  main rendering is live; worker context objects are shared with the main context — correct
  for binary extraction, but object handles (shaders/programs) created on workers must never
  be deleted from the main context without `glDelete*` being context-aware (they are shared,
  so OK) — still, cleanup on `stop()` races nothing but ordering is fragile.
- A pack switch can `submit` hundreds of jobs; workers (≤4) + GL driver `GL_KHR_parallel_shader_compile`
  hint (GLCore.cpp:286-291) are two independent parallelism knobs.

### 8.2 Skin download — `ImageDownload`
`src/net/minecraft/client/texture/ImageDownload.cpp:7-18`.
- **`std::thread(...).detach()`** writing plain members `image` and `slimArms`
  (ImageDownload.hpp:17-18). The main thread reads these (skin apply) with no
  synchronization and no ownership. **Data race + detached thread per skin; object can be
  destroyed while the worker writes it.**

### 8.3 Resource pack download — `ResourceDownloadThread`
`src/net/minecraft/client/resource/ResourceDownloadThread.cpp`.
- `start()` `:85-90` spawns `worker_`; `run()` `:97-120` fetches listing then downloads
  resources, calling back into **`minecraft_->loadResource(...)` from the worker thread**
  (`:138`, `:168`) — main-thread-owned `TextureManager`/screens mutated off-thread.
- `cancel()` `:91-93` sets atomic; dtor `:79-84` joins. `reload()` `:94-96` calls
  `loadFromDirectory` **on the main thread synchronously** (used by forceResourceReload,
  Minecraft.cpp:847-849).

### 8.4 Auth
- `SessionRestore` (`src/net/minecraft/client/auth/microsoft/SessionRestore.cpp:55-92`):
  one `std::thread` worker guarded by `running/finished/canceled` atomics; joined from
  `ensureAuthenticatedForJoin` (`:163-175`, spins 25ms while running), `tickRestoreSavedAccount`
  (`:237-268`, joined on main thread each tick), `cancelSavedAccountRestore` (`:130-150`).
  Result under `gSavedAccountRestore.mutex`. Correct-ish, but **`ensureAuthenticatedForJoin`
  blocks a connector thread up to forever while the restore runs**.
- `SessionValidator` (`src/net/minecraft/client/session/SessionValidator.cpp:17-52`):
  `SessionCheckThread(client.session).detach()` at `:51`; only writes
  `failedSessionCheckTime` (atomic). Detached, fires once at tick 6000 (Minecraft.cpp:604-606).
- `LoginScreen` (`src/net/minecraft/client/gui/auth/LoginScreen.cpp:98-106,125-134,260-271`):
  `workerThread_` per phase; joined in `mergePendingWork`/`cancelSignIn` (`:224-226`, `:146-148`)
  on the main thread. `workerFinished_`/`cancelRequested_`/`authStage_` atomics. **Blocking
  join of an HTTP worker on the main thread if the user cancels mid-request.**

### 8.5 Audio — `XAudio2Backend`
`src/net/minecraft/client/platform/audio/backend/XAudio2Backend.cpp`.
- ctor `:617-629`: `worker` (run loop), plus `2 effectDecoders` + `longDecoder`
  (`:624-627`) = **4 threads**.
- Command/queue protocol fully mutex+CV guarded (Impl members `:452-471`).
- dtor `:630-652`: stop flags + join all 4.
- Decode work (`decode` `:494-534`) can call `decodeAudioFile` (ffmpeg/stb?) off-thread;
  the main thread only enqueues.

### 8.6 Shader pack dir watcher — `PackManager`
`src/net/minecraft/client/render/pipeline/Manager.cpp:209-239`.
- `directoryWatcher_` jthread (`Manager.hpp:149`) polls the shaderpacks dir every 2s
  (`:226-228`), sets `directoryChanged_` atomic; `poll()` (`:241-247`) reloads on main thread.
- Joined in `stopDirectoryWatcher` (`:214-219`). Clean pattern (jthread + atomics).

### 8.7 Hang watchdog — `ClientDiagnostics`
`src/net/minecraft/client/diagnostics/ClientDiagnostics.cpp:234-265`.
- **Detached** thread sleeping 1s, comparing `gHeartbeat` (atomic) to detect main-loop stalls;
  writes a minidump on 6 stalled checks. Never joined, exits when `gWatchdogDisarmed` set
  (Minecraft.cpp:349).

### 8.8 Logging writer — `LogDispatcher`
`src/net/minecraft/util/logging/Logging.cpp`.
- `start()` `:129-136` spawns `thread_` -> `writerMain` `:162-177` (CV-drained queue).
- **Leaked singletons** (mutex + logger map + dispatcher, `:16-24`, `:125-127`) — deliberate:
  the writer thread outlives main-thread statics.
- `shutdown()` `:137-149` joins. `enqueue` `:150-161` never blocks (drops oldest at 16384).
- Any thread (workers, network, server) can log; correct single-writer design.

---

## 9. Dedicated server

- `src/server-main.cpp:125-126`: main thread spawns `serverThread` (runs `server->run()`),
  joins it. (Separate from `MinecraftServer::runThread_` which is only used by the
  never-called `startAsync`, MinecraftServer.cpp:174.)
- `MinecraftServer::init` (`MinecraftServer.cpp:196-214`) spawns `commandThread_` (jthread,
  console `std::cin` poll loop) when `useConsoleThread` (always true from server-main).
- `MinecraftServer::run` loop `:417-449` (20 TPS catch-up, 1ms sleep), `tick()` `:486-537`
  ticks worlds (which pump the SAME ChunkCache pools + LightingEngine), connections, commands.
- `queueCommands` (`:136-139`) is the only commandThread->serverThread handoff (mutex-guarded).
- `DedicatedServerGui` (`src/net/minecraft/server/dedicated/gui/DedicatedServerGui.cpp:61-82`):
  **`guiThread.detach()`** running a Win32 message loop; `onClose` (`:153-161`) spins
  `while(!server_.stopped) Sleep(100)` then `ExitProcess(0)`. Cross-thread: reads
  `server_.stopped` (plain bool) while the server thread writes it.
- Per-connection: `ConnectionListener::thread_` + N × Connection(reader+writer) + transient
  `verifyThread_`.

---

## 10. Cross-cutting hazards (ranked by severity)

### 10.1 RANK #1 — No global executor / thread-count authority (oversubscription)
5 independent budgeters (§2.3): mesh(4) + chunk-loader(4) + save(1) + lighting(3) +
shader-compile(4) = **12 compute threads before any IO/network/audio**, each assuming it is
1 of 3 "competing pools". Add network (2/socket), audio (4), logging (1), watcher (1),
resource download (1), main (1). No site knows about the others; no site knows the frame
budget of the others; no site is GPU-aware (mesh jobs and shader compiles queue indefinitely
and are only throttled by main-thread drain time).

### 10.2 RANK #2 — Main thread is blocked/starved by worker pools
- `WorkerPool::drain()`/`WorkerHandoff::cancelAll()` (WorkerPool.hpp:50-53, WorkerHandoff.hpp:35-40)
  called from `WorldRenderer::clearSections` (WorldRenderer.cpp:369) on teleport/reload and
  from `~WorkerHandoff`, `ChunkCache` dtor (ChunkCache.cpp:26-35), `waitForPendingWrites`
  (ChunkCache.cpp:349-355). All can block the frame for the slowest in-flight job.
- `ChunkCache::retireFromLighting` busy-wait (ChunkCache.cpp:65-67) — main-thread spin
  coupled to worker pin progress; no CV.
- `ShaderCompileService::compileBlocking` (ShaderCompileService.cpp:166-177) main-thread
  wait on a worker; `LoginScreen`/`SessionRestore` main-thread joins of HTTP workers.

### 10.3 RANK #3 — `cancelPending()` silently drops tasks; owners lose count
WorkerPool.hpp:45-48. Affected: ChunkCache loader pool (PendingLoad forever-pending,
ChunkCache.cpp:231-249) and ChunkMeshScheduler (meshJobInFlight stuck, WorldRenderer.cpp:577).
No completion callback, no "cancelled" path, no generation/epoch token on jobs.

### 10.4 RANK #4 — Real data races on shared globals
- **GL state**: mod-block world draws on mesh WORKERS call `core::setAlphaTestRef`
  (`src/net/minecraft/mod/model/ModModels.cpp:626`) -> `g_alphaTestRef` global
  (RenderCore.cpp:671-677) read/written by main thread (RenderType.cpp:88, FrameData.cpp:255).
  `g_uploadedAlphaTestRef`/`g_passUniformsUploaded`/`programChanged` are main-thread GL state
  — a worker writing `g_alphaTestRef` corrupts main-thread uniform upload decisions.
- **`GLCore::init()` `g_loaded` plain bool** (GLCore.cpp:121,145) + all function-pointer
  statics — main + 4 workers.
- `ImageDownload.image/slimArms` (ImageDownload.hpp:17-18) written by detached thread.
- `Connection::disconnectReason_`/args shared between reader and main without a lock
  (Connection.cpp:339 vs :225).
- `World::save` async thread reads `dimensionData_`/players while main writes
  (World.cpp:331-338).
- `ChunkNibbleArray::bytes` bulk copies (save/network) are non-atomic (ChunkNibbleArray.hpp:12-13)
  — acknowledged transient torn reads.
- `MinecraftServer::stopped` plain bool read by GUI thread spin (DedicatedServerGui.cpp:155).
- `Minecraft::options` / `Minecraft::INSTANCE` read by worker threads via
  `BlockRenderManager::snapshotGlobals` (BlockRenderManager.cpp:74-75) only on the main path;
  workers use captured `job.opts` (safe), but `Tessellator::INSTANCE` is used on the main
  thread while workers build with LOCAL tessellators (safe — verify no shared buffer).

### 10.5 RANK #5 — Cross-thread mutation of main-thread-owned state
- `ResourceDownloadThread` worker calls `minecraft_->loadResource(...)` (ResourceDownloadThread.cpp:138,168).
- `MultiplayerConnector` thread calls `bridge->disconnect()` -> `handler->disconnect()`
  -> `minecraft->setWorld(nullptr)` (ClientNetworkHandler.cpp:115-121 via MultiplayerConnector.cpp:32/40/43).
- `ServerLoginNetworkHandler` worker touches `username_`, `serverId_` (plain) and calls
  `self->disconnect` (mutex-guarded result only).

### 10.6 RANK #6 — Detached / unowned threads
ImageDownload (per skin), SessionValidator, hang watchdog, DedicatedServerGui guiThread.
All write process-lifetime state; none can be joined or cancelled at exit (`std::_Exit`
at Minecraft.cpp:391 makes this moot but the races remain).

### 10.7 RANK #7 — GL-context-thread discipline
- Correct: ShaderCompileService uses dedicated worker contexts sharing the main context;
  mesh upload + ProgramCache linking happen on the main context only.
- Violation risk: `GLCore::init()` writes globals from any context; `glfwCreateWindow` on
  main while rendering; `DisplayManager::setSwapPacing`/`pumpAndPresent` are main-only (fine).
- The ONLY off-main GL-context calls are the shader worker compiles + `GLCore::init` static
  writes — the `g_alphaTestRef` race (§10.4) is the concrete state leak into GL uniform uploads.

### 10.8 Global mutable singletons touched from multiple threads
- `Minecraft::INSTANCE` (Minecraft.hpp:85) — read by workers (BlockRenderManager.cpp:74/233,
  ChunkBuilder.cpp:189) and by audio/network threads; written once (Minecraft.cpp:240).
- `Tessellator::INSTANCE` (Tessellator.cpp:78) — main thread only.
- `LogDispatcher` / logger map / `loggerMutex` — leaked singletons, cross-thread, correct.
- `RegionIo` open-files registry (RegionIo.hpp:104-111) + `AlphaChunkStorage::chunkFileMutex`
  (AlphaChunkStorage.cpp:24-26) — cross-thread, locked.
- `MinecraftServer::capturedThread` static map (MinecraftServer.cpp:67-68) — written only in
  tick(), never populated; dead global.
- `UnifiedLightRegistry::blockEmission_/blockColor_` atomics (UnifiedLightRegistry.hpp:28-29).
- `ChunkBuilder::chunkUpdates` inline static int (ChunkBuilder.hpp:84) — main thread only.
- Mod `ModelStore`/`InstanceStore` (ModModels.cpp:52-60, 158-165) — mutex-guarded, but
  worker-held `BakedModel*` pointers outlive the lock; safe only while nothing clears the store.

---

## 11. Runtime thread-count estimate

### Client, 16-logical CPU (8c/16t), in-world, one player
| System | Count | Source |
|---|---|---|
| Main thread | 1 | Minecraft.cpp:749 |
| Chunk mesh pool | 4 | ChunkBuilder.hpp:156 (`rec(3,2,6)`) |
| Chunk loader pool | 4 | ChunkCache.cpp:217 (`rec(3,2,4)`) |
| Chunk save pool | 1 | ChunkCache.cpp:224 |
| LightingEngine | 3 | LightingEngine.cpp:48-49 (`rec(3,2,3)`) |
| Shader compile | 4 | ShaderCompileService.cpp:46-48 |
| Client Connection (reader+writer) | 2 | Connection.cpp:131-132 |
| XAudio2 backend | 4 | XAudio2Backend.cpp:618-627 |
| Logging writer | 1 | Logging.cpp:135 |
| Resource download | 1 | ResourceDownloadThread.cpp:89 |
| Pack dir watcher | 1 | Manager.cpp:211 |
| Hang watchdog (detached) | 1 | ClientDiagnostics.cpp:235 |
| **Steady-state total** | **≈ 27** | |
| Transient: SessionRestore, SessionValidator, MultiplayerConnector, joinServerThread, LoginScreen, ImageDownload(s) | +1–4 | §8.2-8.4, §7.4-7.5 |
| **Peak** | **≈ 30–31** | |

### Dedicated server, 16-logical, 4 players, GUI on
| System | Count |
|---|---|
| serverThread (main) | 1 |
| commandThread_ | 1 |
| ConnectionListener listenLoop | 1 |
| Per-connection reader+writer (4 players + login) | 8–10 |
| verifyThread_ (transient per login) | ≤1 |
| Chunk loader pool | 4 |
| Save pool | 1 |
| LightingEngine | 3 |
| Logging writer | 1 |
| GUI thread (detached) | 1 |
| **Total** | **≈ 21–23** |

CPU oversubscription is structural: the compute pools alone (mesh 4 + loader 4 + light 3 =
11) exceed half the logical cores, and shader compile + audio + GL driver hint stack on top.

---

## 12. What a correct refactor must reconcile (summary of coupling)

1. One authority for "how many CPU threads" must replace 5 independent
   `recommendedThreadCount`/`hw-2` calls.
2. Worker teardown must never block the render loop: replace `drain()`-on-main with
   deferred-cancel + epoch tokens so owners learn their jobs were dropped.
3. The main loop's per-frame slices (lighting drain, chunk publish, mesh capture/upload,
   network drain) need a single shared FrameBudget derived from measured frame time +
   GPU sync state, not five independent constants.
4. All worker-produced results must cross to the main thread through owned completion
   queues with explicit lifetime; `ImageDownload` and `ResourceDownloadThread` must stop
   mutating main-thread objects from workers.
5. GL-state writes (incl. `g_alphaTestRef` via mod block draws and `GLCore::init`) must be
   confined to the main GL thread.
