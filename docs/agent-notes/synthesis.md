# SYNTHESIS — Transcript Consolidation: Council (×6) + Initial Planner

Pipeline stage: **Transcript synthesizer**. Inputs: `RULES FOR AGENTS.md`,
`docs/agent-notes/council-concurrency-inventory.md`,
`docs/agent-notes/council-java-thread-model.md`,
`docs/agent-notes/council-chunk-workers.md`,
`docs/agent-notes/council-network-threading.md`,
`docs/agent-notes/council-architecture-proposal.md`,
`docs/agent-notes/council-mainthread-map.md`,
`docs/agent-notes/plan-initial.md`.

Audience: plan auditors, plan master, executors. **No source files were edited and
nothing was built during this stage.**

> **Line-number health warning (READ FIRST).** Every file:line below was re-verified
> against the *current working tree* on 2026-08-01. The working tree has large
> **uncommitted edits** (git diff HEAD shows ~99 changed lines in `Minecraft.cpp`,
> plus `WorldRenderer.cpp`, `GameRenderer.cpp`, `GLCore.cpp`, `ShaderCompileService.cpp`,
> `ProgramCache.cpp`, `RenderCore.cpp` and ~40 more files). Files with uncommitted
> edits have **stale line numbers in the council docs** (council numbers are typically
> ~13–27 lines early); untouched files match the docs exactly. The initial planner
> re-verified the drifted files against the working tree and is correct on nearly all
> of them. Executors **must** re-grep before editing (plan WI preamble already says
> this; this synthesis confirms it is mandatory, not a formality).

---

## 1. Consolidated statement of the problem

### 1.1 The one problem all six council docs and the planner agree on

The port is **structurally ahead of Java Beta 1.7.3** (async meshing, lighting,
chunk load/save, shader compile all exist) but the threading is **uncoordinated**:

- **No single owner of the thread budget.** Each of ~6 pool/thread-creation sites
  sizes itself independently from `hardware_concurrency()` — mostly via
  `WorkerPool::recommendedThreadCount(3, 2, N)` (each guessing "3 competing pools")
  — so total workers oversubscribe the machine. Docs agreeing:
  concurrency-inventory §2.3/§10.1 (RANK #1), chunk-workers §3, network-threading §2
  (loader row), architecture §0/§3, mainthread-map §0, plan §0/§1.
- **The "main thread" is a single monolithic serial loop** (`Minecraft::run()`) doing
  tick AND render AND present, with per-frame slices to four independent worker
  backends (lighting drain, chunk publish, mesh capture/upload, network drain), each
  with its own independent budget/constants. Docs agreeing: concurrency-inventory §1,
  java-thread-model §1.1, chunk-workers §2, mainthread-map §1/§4, architecture §4,
  plan §0/§1.5.
- **Main-thread stalls.** Worker teardown (`WorkerPool::drain`/`cancelAll`), the
  `ChunkCache::retireFromLighting` spin, socket-thread joins, HTTP-verify joins, and
  synchronous `prepareWorld` all block the render loop. Docs agreeing: all six (ranked
  #2 in the inventory; §5 in chunk-workers; H1/H5/H6/H9 in network-threading; §2 in
  mainthread-map; §6 in architecture; WI-3/4/7/9/10/12 in plan).
- **Several real data races (UB), not just staleness:**
  - the `RegionSnapshot` memcpy of live chunk light/block arrays vs lighting workers
    and main-thread writers (chunk-workers **R1**, plan WI-3);
  - the GL-state race `g_alphaTestRef` written by mesh **workers** (concurrency
    inventory §10.4, architecture §5.1.4, plan §2.12/WI-5);
  - `GLCore::init()` plain-bool `g_loaded` (inventory §8.1/§10.4, plan WI-5/WI-8);
  - `Packet::read` static packet-tracker accounting from every reader thread
    (network **H4**, plan WI-9);
  - `ServerLoginNetworkHandler::closed`/`connection_` written by the verify thread,
    read by the server tick thread (network **H5**, plan WI-9);
  - `ImageDownload` detached-thread writes to `image`/`slimArms` (inventory §8.2,
    plan WI-11);
  - `World::save` async thread reading `dimensionData_`/players concurrently
    (inventory §6, plan WI-11/D13).
- **Parity targets are the hard constraints**: Java b1.7.3 single-thread game loop
  (`timer.advance → N ticks → 1 render`) + per-connection reader/writer socket
  threads + sim-thread packet apply; and Iris 26.1 (render-thread-only GL, exact
  phase order, BufferFlipper stage semantics). Every doc and the planner treat these
  as MUST-PRESERVE; the plan's §2 NEVER-PARALLELIZE list is the consolidated form.

### 1.2 Where the docs agree (non-controversial facts)

- Thread-count *arithmetic* on 16-logical: mesh 4, loader 4, save 1, lighting 3,
  shader-compile 4 (inventory §2.3/§11, chunk-workers §3, plan WI-2). **Exception:
  network-threading §2 says loader is "3 workers" — this is WRONG (see §1.3).**
- Oversubscription total: ≈12 compute workers (mesh+loader+save+light) + 4 shader +
  network/audio/logging → client steady state ≈27, peak ≈30–31 (inventory §11);
  chunk-workers' "16 workers + main = 17" counts the same sets (12 + shader 4).
- Four independent pools all pretend to be "1 of 3 pools":
  `recommendedThreadCount(3, 2, 6/4/3)` (inventory §2.3, chunk-workers §3, plan WI-2).
- `WorkerPool::drain()` blocks the main thread; `cancelAll`/`cancelPending` drop
  queued work without notifying owners (inventory §2.1/§10.3, chunk-workers §5.1/§6,
  plan WI-4/WI-7).
- `retireFromLighting` is a 200 µs sleep spin (ChunkCache.cpp:65-67) — main-thread
  busy-wait, no CV (inventory §5, chunk-workers R2/§5.5, plan §3 context).
- nearLane priority is **documented but not implemented** (ChunkMeshJob.hpp:51-54
  stale comment; `processUpload` never checks `job->nearLane`) (chunk-workers §5.3/§8,
  plan WI-4).
- `Connection` = 2 kernel threads per socket; disconnect/`joinThreads` runs on the
  caller (game/server) thread with 30 s socket timeouts (network H1/H2, inventory
  §7.1, plan WI-10).
- Java's `networkShutdown()` is async (spawns `NetworkMasterThread`, never joins);
  the C++ port joins synchronously — the single biggest parity + responsiveness
  deviation (network §5 deviation 2 / H1; java-thread-model §4).
- C++ packet drain is time-boxed (3 ms / min 8 / max 4096) vs Java's fixed 100
  packets/tick — a deliberate improvement, but must stay budgeted (network §5
  deviation 1; mainthread-map §3.5; plan WI-9/WI-12).
- Client integrated server runs as an **external process** (`ServerProcessCoordinator`)
  — a deliberate divergence from Java's in-process server thread; keep the functional
  contract (java-thread-model §1.4; network §11 Q6; plan D11).
- The two `static std::mutex stallMutex` blocks are debug-only
  (`MINECRAFT_RENDER_TRACE`), at Minecraft.cpp:742-743 (`runRenderPhase`) and
  :827-828 (`run()`) (architecture §1/§4, plan §7/D9; verified).

### 1.3 Where the docs diverge (summarised; full list + severity in §4)

1. **Thread-count facts.** network-threading §2 lists the chunk loader pool as
   "(WorkerPool, **3** workers)". The code is
   `recommendedThreadCount(3, 2, 4)` (ChunkCache.cpp:217) = **4** workers on 16
   logical (clamp((16−2)/3, 1, 4) = 4). concurrency-inventory §2.3 and chunk-workers
   §3 say 4. **The network doc is wrong.**
2. **Thread-count *site count*.** The task brief and the inventory's TL;DR speak of
   "at least 5" / "6" `recommendedThreadCount` sites. Verification: **only 4** actual
   call sites exist — ChunkBuilder.hpp:156, ChunkCache.cpp:217, LightingEngine.cpp:49,
   and the **currently-unused default arg** WorkerHandoff.hpp:14 (the sole
   `WorkerHandoff<ChunkMeshJob>` instantiation passes an explicit count). Plus the
   definition at WorkerPool.hpp:61-68, plus **two hand-rolled `hw−2` sites**
   (ShaderCompileService.cpp:47, GLCore.cpp:289). So "6 sites" is right only if you
   count the two `hw−2` sites and *miss* the WorkerHandoff default; the precise count
   is **4× `recommendedThreadCount` call sites + 2× hand-rolled `hw−2`** (plan WI-2's
   file list is accurate).
3. **Single mega-pool vs domain pools.** architecture §3.1 explicitly rejects one
   giant shared pool (GL context affinity, blocking calls, priority) and proposes a
   small fixed set of **domain pools sharing ONE budget**. chunk-workers §8 says
   "Prefer one shared `WorkerPool` with tagged job classes (mesh / lighting / io /
   save)". These are opposite recommendations. The planner sided with **domain pools**
   (plan §1.2). Must resolve before WI-2/4/6/7.
4. **Lighting lane.** architecture §9.1 and plan D3 recommend lighting gets its own
   **lane** inside Compute (WorkerState/pin-cache thread affinity); chunk-workers §8
   allows "own lane or same queue". Needs a decision before WI-6.
5. **Hazard severity ordering.** concurrency-inventory ranks *oversubscription* #1 and
   puts the snapshot race under #4 "data races". chunk-workers calls R1 (snapshot
   memcpy) the *"dominant correctness risk"*. Both are real and both are in the plan
   (WI-2 for budget, WI-3 for R1) — the disagreement is emphasis, not scope.
6. **Blocking teardown semantics.** network-threading §9 says "**Never `join()` on the
   game thread**". architecture §3.3/§6.4 says joins are fine if *unblock-first +
   watchdogged* (2–5 s then leak). If `Lifecycle::shutdown()` runs on the game thread,
   those two statements conflict. Must resolve: shutdown-time watchdogged joins are
   acceptable (process exiting); **runtime** `disconnect()` must never join.
7. **Line-number drift.** All six council docs cite `Minecraft::run()` at
   Minecraft.cpp:749 and its internals ~13 lines early; the current tree has `run()` at
   **:762** (heartbeat :776, `flushRetired` :778, `timer.advance` :801-807, tick loop
   :809-818, `runRenderPhase` :823). WorldRenderer/GameRenderer drift is bigger
   (e.g. `setBlocksDirty` :1170 in docs vs **:1189**; `compileChunks` call at
   GameRenderer.cpp:1166-1169 in docs vs **:1195**). The planner's numbers are the
   current-tree ones (see §7 corrections).
8. **Java client sources are absent from the repo.** java-thread-model §0 (and
   chunk-workers §7) verified that `third_party/mcp/net/minecraft/` contains only
   **server + shared** classes; `net.minecraft.client.Minecraft` etc. do not exist
   anywhere. The C++ port (`src/net/minecraft/client/`) is therefore the *authoritative
   mirror of the client loop*. Every other doc (and the plan) silently rely on this.
   Must be acknowledged as an accepted assumption, not an unnoticed gap.
9. **CONTEXT.md is missing** (`docs/agent-notes/CONTEXT.md`) — referenced by the
   initial-planner brief (plan §7). Not a doc conflict, but flag to auditors.

---

## 2. Consolidated unique hazard list (merged across all six docs + planner)

Each row: one distinct hazard, all file:line citations found in the source docs
(verified against the working tree), and the owning work item. IDs `HZ-01…` are this
synthesis's own stable identifiers. Where two docs gave different line numbers, the
verified value is used and the discrepancy flagged in §7.

### A. Thread-count / oversubscription

- **HZ-01 — No global thread-count authority; independent budgeters oversubscribe.**
  - concurrency-inventory §2.3, §10.1 (RANK #1), §11; chunk-workers §3; architecture §0/§3.2; plan §1/WI-2.
  - Sites (verified): ChunkBuilder.hpp:156 (`rec(3,2,6)` → 4 @16t),
    ChunkCache.cpp:217 (`rec(3,2,4)` → 4), ChunkCache.cpp:224 (save, fixed 1),
    LightingEngine.cpp:48-49 (`max(1, rec(3,2,3))` → 3), ShaderCompileService.cpp:46-48
    (`min(hw−2, 4)` → 4), GLCore.cpp:288-290 (`hw−2` driver hint, no threads),
    WorkerHandoff.hpp:14 (`rec(2,2)` default, unused).
  - Resolution: WI-2 deletes `recommendedThreadCount` and routes all counts through
    `ThreadCoordinator`/`ThreadBudget`.

### B. Main-thread blocking / stalls

- **HZ-02 — `WorkerPool::drain()`/`WorkerHandoff::cancelAll()` block the main thread.**
  - concurrency-inventory §2.1/§10.2; chunk-workers §5.1; WorkerPool.hpp:50-53,
    WorkerHandoff.hpp:35-40; called from WorldRenderer.cpp:373 (`clearSections`),
    `~WorkerHandoff`, ChunkCache.cpp:26-35 (dtor), ChunkCache.cpp:349-355
    (`waitForPendingWrites`).
  - Resolution: WI-4 (non-blocking cancel + epoch tokens), WI-7 (loader cancel).
- **HZ-03 — `ChunkCache::retireFromLighting` busy-wait spin (no CV).**
  - concurrency-inventory §5/§10.2; chunk-workers R2/§5.5; mainthread-map §2.7.
    ChunkCache.cpp:61-67 (sleep 200 µs); used at :78 (`unloadChunk`) and :440 (`tick`).
  - Resolution: WI-4/plan recommendation — CV-signal on pin release.
- **HZ-04 — `ShaderCompileService::compileBlocking` waits on a worker CV on the main thread.**
  - concurrency-inventory §8.1/§10.2; architecture §4.1; ShaderCompileService.cpp:144-177 (wait at :167).
  - Resolution: WI-8 — normal path = submit + frame-driven poll; keep blocking only for the
    `!started()` fallback (ShaderCompileService.cpp:157-159 per plan D8).
- **HZ-05 — Socket-thread joins on the game/server thread (`disconnect`/dtor).**
  - network H1/H2/H10; mainthread-map §2.3; concurrency-inventory §7.1. Connection.cpp:157-160
    (`disconnect` → `joinThreads`), :134-137 (dtor), :352-360 (`joinThreads`); 30 s timeouts
    Connection.cpp:17-20; call sites: ClientNetworkBridge.cpp:149-158,
    ServerPlayNetworkHandler.cpp:79, ConnectionListener.cpp:139, ServerLoginNetworkHandler.cpp:62.
  - Resolution: WI-10 — async teardown, never join at runtime.
- **HZ-06 — HTTP/auth worker joins on the main thread.**
  - network H5/H6; mainthread-map §2.2; ClientNetworkHandler.cpp:243-245 (join in
    `beginPendingLogin`), :49-54 (dtor); ServerLoginNetworkHandler.cpp:150-152
    (`verifyThread_.join()` on the server tick); LoginScreen.cpp:224-226; SessionRestore.cpp:163-175/237-268.
  - Resolution: WI-9 (result-only verify, no join-to-reuse), WI-11 (HTTP on Io pool).
- **HZ-07 — Blocking saves at teardown.**
  - network H9; mainthread-map §2.6; World.cpp:312-314; ChunkCache.cpp:349-355, 409-417.
    Shutdown-only and acceptable; must remain reachable from a dedicated teardown phase (plan WI-13).
- **HZ-08 — Synchronous HTTP / file / decode on the main thread.**
  - mainthread-map §2.4/§2.5 (worst: `downloadPendingMods` unbounded HTTP,
    ClientNetworkHandler.cpp:258-320; `TextureManager::getTextureId` decode/upload,
    TextureManager.cpp:451-483; sync stat save on every `setScreen`, ScreenStack.cpp:33-38;
    `prepareWorld` force-load, WorldSession.cpp:159-198).
  - Resolution: context/scope (not an item in itself); WI-11 moves the asyncable ones
    (ImageDownload, ResourceDownload, connector). `prepareWorld` stays main-thread (parity).
- **HZ-09 — Serial main-thread CPU: `tickEntities` + `manageChunkUpdatesAndEvents` + `NaturalSpawner` + `displayTick`.**
  - mainthread-map §3.1-3.4, §7.2; WorldEntities.cpp:386-500, WorldChunks.cpp:456-575.
  - Resolution: **out of scope** — this refactor keeps the world sim on the main thread
    (architecture decision 2; plan §2.9). Not a defect, but the reason tick spikes = frame spikes.

### C. Cancellation / lost-work

- **HZ-10 — `cancelPending()` silently drops queued tasks; owners never learn.**
  - concurrency-inventory §10.3 (RANK #3); chunk-workers §5.1; WorkerPool.hpp:45-48.
  - Symptoms: `meshJobInFlight` stuck forever (ChunkBuilder.hpp:105, set at WorldRenderer.cpp:581,
    `startMeshJob` :568-589); `PendingLoad` left `done==false` in `pendingLoads_`
    (ChunkCache.cpp:226-249; `integrateFinishedLoads` :250-288 scans and skips them).
  - Resolution: WI-4 (mesh, epoch/generation token) + WI-7 (loader).
- **HZ-11 — R3: `~ChunkMeshJob` writes `builder->meshJobInFlight` (ChunkBuilder.cpp:220-225).**
  - chunk-workers R3; plan §2.8. Safe today only because the last `shared_ptr` dies on the
    main thread (WorkerHandoff.hpp:26-27, :35-40). Fragile invariant.
  - Resolution: WI-4 — keep `cancelAll`/drop main-thread-only; make the write atomic or assert main.

### D. Data races (UB)

- **HZ-12 — R1: `RegionSnapshot` memcpy of live chunk arrays vs lighting workers + main writers.**
  - chunk-workers R1 (dominant correctness risk), §2.3/§8; plan WI-3.
  - Copy: RegionSnapshot.cpp:32-67 (`copyChunkBand`), memcpy column loop :54-66.
    Writers: LightingEngine.cpp:249-258 (`setBrightness`→Chunk::setLight),
    Chunk.cpp:61-89 (`setBlock`), ChunkCache.cpp:173 (`populateBlockLight`), :373-384 (`decorate`).
  - Render pin (Chunk.hpp:205-224) protects **eviction only, not contents**.
  - Resolution: WI-3 — per-chunk lock/`atomic_flag` around light writes + copy (option a),
    or version-stamped copy (option b).
- **HZ-13 — R4: lighting workers write nibbles while main-thread `saveChunk`/`adoptChunk` snapshot.**
  - chunk-workers R4; ChunkCache.cpp:356-372 (`saveChunk`), :156-201 (`adoptChunk`).
  - Same arrays as R1 → same WI-3 fix.
- **HZ-14 — GL-state race: mesh workers write `g_alphaTestRef`.**
  - concurrency-inventory §10.4/§10.7; architecture §5.1.4; plan §2.12/WI-5.
  - ModModels.cpp:626 (`core::setAlphaTestRef(0.1f)` inside `drawLuaBlockWorld`, runs on Compute
    workers via `ChunkBuilder::buildMesh`); RenderCore.cpp:70 (`float g_alphaTestRef`),
    :671-677 (`setAlphaTestRef`), :479-481 (main-thread uniform upload reads it);
    RenderCore.hpp:290.
  - Resolution: WI-5 — capture alpha ref into the mesh job/snapshot; never write the global from workers.
- **HZ-15 — `GLCore::init()` plain-bool `g_loaded` + all function-pointer statics.**
  - concurrency-inventory §8.1/§10.4/§10.7; plan WI-5/WI-8. GLCore.cpp:121 (`static bool g_loaded`),
    :141-145 (`init` check+set), :123-131 (`loadProc`).
  - Resolution: WI-5 — `std::once_flag`/atomic for `g_loaded`.
- **HZ-16 — `ImageDownload` detached thread writes `image`/`slimArms`.**
  - concurrency-inventory §8.2/§10.4/§10.6; plan WI-11. ImageDownload.cpp:7-18 (`.detach()`),
    ImageDownload.hpp:17-18 (plain members).
  - Resolution: WI-11 — delete `.detach()`; download lands in a texture `Channel`, applied on main.
- **HZ-17 — `Packet::read` static accounting race (H4).**
  - network H4, §6 table; plan WI-9. Packet.hpp:79-81 (mutates `packetTrackers()[rawId]` + `++incomingCount()`),
    :121-128 (statics). N reader threads → concurrent writes on one `unordered_map`.
  - Resolution: WI-9 — per-`Connection` counters, merged on the game thread.
- **HZ-18 — `ServerLoginNetworkHandler` verify-thread cross-thread disconnect + `closed` race (H5).**
  - network H5, §6 table; plan WI-9. ServerLoginNetworkHandler.cpp:154 (thread spawn),
    :172/:175 (failure path calls `disconnect()` from the verify thread), :64 (`closed=true`),
    :51-53 (`connection_->tick()` on server tick); ConnectionListener.cpp:116 reads `closed`.
  - Resolution: WI-9 — verify thread publishes result only; `Connection`/`closed` mutation on the
    server tick thread.
- **HZ-19 — `Connection::disconnectReason_`/args shared reader↔main without a lock.**
  - concurrency-inventory §10.4; network §6 (assessed "benign write-before-close"). Connection.cpp:339 (write) vs :225 (read).
  - Resolution: documented benign; make explicit under WI-9/10 (or leave, but write it down).
- **HZ-20 — `World::save` async thread reads `dimensionData_`/players concurrently.**
  - concurrency-inventory §6/§10.4; plan WI-11/D13. World.cpp:325-339 (`std::async` level.dat writer),
    World.hpp:429 (`asyncSaveFuture_`), :331-338 (snapshot of `std::vector<PlayerEntity*>`).
  - Resolution: WI-11 — Io-pool task with a value snapshot; `save(true)` still waits (H9).
- **HZ-21 — `ChunkNibbleArray` bulk copies are non-atomic (torn reads on save/network).**
  - concurrency-inventory §10.4; ChunkNibbleArray.hpp:12-13 (acknowledged transient).
  - Resolution: accepted/transient; note only.
- **HZ-22 — `MinecraftServer::stopped` plain bool read by the detached GUI thread.**
  - concurrency-inventory §9/§10.4; plan WI-11. DedicatedServerGui.cpp:155 (`while(!server_.stopped)`).
  - Resolution: WI-11 — make `stopped` atomic + register GUI thread.
- **HZ-23 — Mod `ModelStore`/`InstanceStore` worker-held `BakedModel*` outlive the mutex.**
  - concurrency-inventory §10.8; ModModels.cpp:52-60, 158-165. Safe only while nothing clears the store.
  - Resolution: note; enforce store lifetime (out of the core items).

### E. Cross-thread mutation of main-thread-owned state

- **HZ-24 — `ResourceDownloadThread` worker calls `minecraft_->loadResource(...)`.**
  - concurrency-inventory §10.5; plan WI-11. ResourceDownloadThread.cpp:138, :168.
  - Resolution: WI-11 — worker produces files/bytes; main applies via channel.
- **HZ-25 — `MultiplayerConnector` thread calls `bridge->disconnect()` → `minecraft->setWorld(nullptr)`.**
  - concurrency-inventory §7.4/§10.5; plan WI-11. MultiplayerConnector.cpp:17-61 (spawn :17,
    `bridge->disconnect()` at :32/:40/:43), :83-94 (`poll` publishes under mutex);
    ClientNetworkBridge.cpp:149-158 (`disconnect` → `handler_->disconnect` at :154);
    ClientNetworkHandler.cpp:115-127 (`disconnect` → `setWorld(nullptr)` at :120).
  - Resolution: WI-11 — connector publishes result only; main thread applies.
- **HZ-26 — `ClientNetworkHandler::message` written on the connector thread.**
  - network §6 table (fragile "publication by mutex handoff", currently safe).
    ClientNetworkBridge.cpp:137, MultiplayerConnector.cpp:57, ConnectScreen.cpp:63.
  - Resolution: keep the mutex-published handoff explicit under the scheduler (network §9).
- **HZ-27 — `verifyJoinServer` reads auth globals / `username_`,`serverId_` plain fields.**
  - concurrency-inventory §7.5/§10.5; network §6 (ServerLoginNetworkHandler). Session is copied by value
    (ClientNetworkHandler.cpp:242, good); `ServerLoginNetworkHandler::username_/serverId_` are race-tolerant.
  - Resolution: covered by WI-9 (result-only verify) — reads under mutex, writes on owner thread.

### F. Detached / unowned threads

- **HZ-28 — Detached threads: ImageDownload, SessionValidator, hang watchdog, DedicatedServerGui.**
  - concurrency-inventory §10.6; plan WI-11. ImageDownload.cpp:7-18; SessionValidator.cpp:17-51
    (`SessionCheckThread(...).detach()` at :51); ClientDiagnostics.cpp:234-265 (watchdog `.detach()` at :264);
    DedicatedServerGui.cpp:65-82 (`guiThread.detach()` at :81).
  - Resolution: WI-11 — Io-domain tasks / registered short-live threads; keep watchdog registered.

### G. GL-context-thread discipline

- **HZ-29 — GL-context-thread violations/risks.**
  - concurrency-inventory §10.7; architecture §5.1.4; plan §2. Only shader workers + `GLCore::init`
    touch GL off-main; the `g_alphaTestRef` leak (HZ-14) is the concrete corruption.
    Shared-context teardown must precede primary-context destroy (`ShaderCompileService::stop`
    Minecraft.cpp:354 before `DisplayManager::destroy` :388).
  - Resolution: WI-5/WI-8/WI-13; `TL_DOMAIN` debug asserts.

### H. Chunk pipeline hazards

- **HZ-30 — `ChunkRegionBuffer` growth re-uploads the whole CPU mirror on the main thread.**
  - chunk-workers §5.2; plan (WI-12 context, OD-D19). ChunkRegionBuffer.cpp:50-62 (`reallocBuffer`
    does `bufferData` + full `bufferSubData`), :74-111 (`upload`); reserve mitigation at
    WorldRenderer.cpp:550-557.
  - Resolution: reserve worst-case, or off-main growth (deferred/out-of-scope decision).
- **HZ-31 — nearLane upload priority unimplemented.**
  - chunk-workers §5.3/§8; plan WI-4. ChunkMeshJob.hpp:51-54 (stale comment); set at ChunkBuilder.hpp:135;
    **never read** in `processUpload` (WorldRenderer.cpp:747-770).
  - Resolution: WI-4 — honor `job->nearLane` in the upload path.
- **HZ-32 — Lighting→mesh double-mesh churn (no "lighting-ready" gate).**
  - chunk-workers §6/§8; plan WI-7 (optional, D5). Outbox drain → `events_.setBlocksDirty`
    (World.cpp:711-717) → `WorldRenderer::markDirty` (:1117) → re-mesh.
  - Resolution: WI-7 optional (D5).
- **HZ-33 — Priority is not cross-pool (mesh ring vs loader ring).**
  - chunk-workers §6; ChunkCache.cpp:498 (loader `ring ≤ 1 → INT_MIN` only partially compensates).
  - Resolution: note; partially mitigated by `prefetchChunksNear`.
- **HZ-34 — Box-conflict scan is O(queue × activeBoxes) under `queueMutex_`; 200k queue cap.**
  - concurrency-inventory §4; LightingEngine.cpp:70 (kMaxQueue 200000), :149-170 (`tryClaimBox`).
  - Resolution: WI-6 (channel with own lane) keeps claim logic; no algorithm change.
- **HZ-35 — Large sky-update boxes self-resubmit and keep workers busy.**
  - concurrency-inventory §4; chunk-workers §2.5. LightingEngine.cpp:286-310 (split), :392-439 (spills).
  - Resolution: note; bounded by channel capacity (WI-6).
- **HZ-36 — `workerGenerators_` per-thread clone map never pruned.**
  - concurrency-inventory §5; ChunkCache.cpp:85-105; network §6. Per-thread clones accumulate across
    dimension switches.
  - Resolution: prune on world switch (WI-7 risk note).
- **HZ-37 — Main-thread `decorate` holds `ioMutex_`.**
  - concurrency-inventory §5; ChunkCache.cpp:179-195, 373-384 (serial main-thread cost; parity: decorate
    stays main-thread, plan §4 WI-7).
  - Resolution: out of scope; note.

### I. Loop / budget / teardown structure

- **HZ-38 — Two independent `stallMutex` blocks + loop-body diagnostics.**
  - architecture §1/§4; plan WI-12/D9. Minecraft.cpp:742-743 (`runRenderPhase`), :827-828 (`run()`),
    both `#ifdef MINECRAFT_RENDER_TRACE`.
  - Resolution: WI-12 — collapse into `FrameProfiler`; keep until then.
- **HZ-39 — No unified per-frame budget; each subsystem uses its own `FrameBudget::fromMs`.**
  - concurrency-inventory §1; architecture §8.8; plan WI-12. FrameBudget.hpp:5-14;
    users: WorldRenderer.cpp:742-743 (upload 3/6 ms), :787-789 (capture 1/2 ms);
    `ChunkCache::integrateFinishedLoads` 2/32/4 (ChunkCache.cpp:424/:473/:483);
    `Connection::tick` 3 ms (Connection.cpp:198-201).
  - Resolution: WI-12 — one per-frame deadline from `FramePipeline`.
- **HZ-40 — `std::launch::async` one-off level.dat thread, unaccounted.**
  - concurrency-inventory §6; World.cpp:325-338. Resolution: WI-11 → Io pool.
- **HZ-41 — Deferred-destruction discipline (`flushRetired`/retire) is the only thing making
  teardown safe on the drain stack.**
  - network H10; mainthread-map §6; Minecraft.cpp:778-788, MultiplayerSession.cpp:12-19,
    ClientNetworkHandler.hpp:144-151. Must survive the scheduler restructure (network §9, plan WI-10/13).
- **HZ-42 — `GLCore::setSwapPacing`/`pumpAndPresent` are main-only; `glfwCreateWindow` on main while rendering.**
  - concurrency-inventory §8.1/§10.7. Fine today; keep main-thread-only under FramePipeline.

---

## 3. Consolidated open decisions / questions (unique numbered list)

Origin mapping: `[plan Dn]` = plan-initial §5; `[net Qn]` = network-threading §11;
`[arch Qn]` = architecture §9; `[gap]` = java-thread-model / chunk-workers / planner flags.
Recommendations in **bold** are the planner's (or the strongest council lean); the auditors
should confirm each.

- **QD-01** `[plan D1 ≡ net Q1 ≡ arch Q3-part]` — **Socket I/O: event loop vs blocking NetIo pool.**
  Windows-only (winsock). Pool of 2–4 blocking workers is the smaller diff and keeps the
  `SocketInputStreamBuf`/`OutputStreamBuf` streambufs (Connection.cpp:30-119) intact, but blocking
  reads cannot be cancelled cheaply. **Recommend: start with a blocking NetIo pool (WI-10 keeps
  per-connection threads registered, unblock-then-watchdog-join); defer an event loop.**
- **QD-02** `[plan D2 ≡ net Q2]` — **Read-side inbound cap policy.**
  Disconnect-on-overflow (Java-style `disconnect.overflow`) vs backpressure (pause reading).
  **Recommend: bounded read queue + `disconnect.overflow`** mirroring the existing `0x100000` send cap.
- **QD-03** `[plan D3 ≡ arch Q1]` — **Lighting: own lane inside Compute vs shared queue.**
  Lighting's per-worker `WorkerState`/pin-cache is thread-affine (LightingEngine.cpp:50-56, :207-233).
  **Recommend: own bounded lane + sub-priority inside Compute** (or make `WorkerState` per-claimed-box).
  Must be confirmed before WI-6.
- **QD-04** `[plan D4 ≡ net Q3]` — **Drain budget policy.**
  Keep the C++ time-box (3 ms / min 8 / max 4096) + per-connection fairness vs reintroduce Java's
  hard 100/player/tick. **Recommend: time-box + per-connection cap, min 8.**
- **QD-05** `[plan D5]` — **Lighting-ready gate scope.** Fix the double-mesh on world load in WI-7
  (optional) or defer. Affects meshing volume, not correctness. **Recommend: defer unless cheap.**
- **QD-06** `[plan D6]` — **Where `ThreadCoordinator` lives / ordering vs leaked `LogDispatcher`**
  singletons (Logging.cpp:16-24, :125-127). The log writer must be usable before/after coordinator
  shutdown; **coordinator must not own the log thread.** Configure in `Minecraft::init()`
  (Minecraft.cpp:270-340) and `server-main.cpp:125-126`.
- **QD-07** `[plan D7 ≡ arch Q5]` — **Confirm present-before-draw is intentional.**
  `runRenderPhase` does `pumpAndPresent` (Minecraft.cpp:694) before `gameRenderer->onFrameUpdate`
  (:707-709) — the port's double-buffer rhythm. Must be confirmed before WI-12 hard-codes it.
- **QD-08** `[plan D8]` — **GL shared contexts for shader compile.** Keep hidden GLFW windows sharing
  the main context; GlCompile domain = 2–3 workers. Confirm `runJobOnCurrentContext` on the main
  thread stays allowed only for the non-started fallback (ShaderCompileService.cpp:157-159).
- **QD-09** `[plan D9]` — **Keep the two stall-mutex blocks until WI-12?**
  They are `#ifdef MINECRAFT_RENDER_TRACE`-only. **Recommend: keep until WI-12, delete then.**
  Confirm nothing else consumes `stall-trace.log`.
- **QD-10** `[plan D10 ≡ arch Q2]` — **`fpsLimit=0` hot-spin parity.**
  **Recommend: keep parity (hot spin) in WI-12; add an optional adaptive sleep as a follow-up.**
- **QD-11** `[plan D11 ≡ net Q6 ≡ arch Q3]` — **Dedicated server + `ServerProcessCoordinator` scope.**
  The external-process coordinator stays out of scope (process-level). In-process dedicated server
  gets the coordinator as deferred WI-14.
- **QD-12** `[plan D12 ≡ arch Q4]` — **Audio (4 pinned threads) and pack dir watcher.**
  Keep as pinned registered threads (XAudio2Backend.cpp:617-629, Manager.cpp:209-239), not folded
  into Compute.
- **QD-13** `[plan D13]` — **`World::save` snapshot safety.** The `std::vector<PlayerEntity*>` snapshot
  (World.cpp:325-339) is read off-thread while main mutates players. **Recommend WI-11 snapshots
  values (or defers to teardown); `save(blocking=true)` stays reachable at shutdown (H9).**
- **QD-14** `[net Q4]` — **HTTP pool must guarantee "no two verifies for the same connection in
  flight" without joins** (covers `ServerLoginNetworkHandler` re-verify mid-login and
  `ClientNetworkHandler` re-join). Needs a per-connection in-flight flag on the Io domain.
- **QD-15** `[net Q5]` — **Removing `DownloadingTerrainScreen`'s drain vs `KeepAlive` cadence.**
  The screen currently emits keep-alive every 20 ticks (DownloadingTerrainScreen.hpp:21-23) and drains
  the handler (:25). If the drain collapses to one-per-tick (H7), keep-alive must move to
  `ClientNetworkHandler::tick`. Confirm the cadence survives.
- **QD-16** `[gap: java-thread-model §0; chunk-workers §7]` — **Vanilla client Java sources are absent**
  from the repo (`third_party/mcp` is server+shared only). Accept the C++ port as the authoritative
  client-loop mirror, or re-verify against an external b1.7.3 decompile if byte-level client fidelity
  is ever required. **Recommend: accept the C++ port (risk is low); record as an assumption.**
- **QD-17** `[arch §8.8; plan WI-12]` — **Unified per-frame budget allocation policy.** When the
  shared frame deadline is exhausted, in what order do lighting/mesh/network/loader drains yield?
  Define a priority (recommend: network drain ≥ near-mesh ≥ lighting ≥ distant mesh ≥ integrates).
- **QD-18** `[inventory §12.2; plan WI-4]` — **Deferred-cancel + epoch token acknowledgment.**
  Who clears `meshJobInFlight` when a queued mesh job is dropped? Confirm clearing it can't let a new
  job start while the old worker still holds the builder (R3-related; `sweepRetiring`
  WorldRenderer.cpp:295-304 must reap safely).
- **QD-19** `[chunk-workers §8]` — **VBO growth off the hot path.** Background GL thread + fenced
  buffer vs per-column reserved regions vs worst-case reservation only (WorldRenderer.cpp:550-557).
  **Recommend: keep on main with worst-case reservation for now; document as follow-up.**
- **QD-20** `[plan WI-5]` — **alphaTestRef snapshot default parity.** Mod blocks currently rely on the
  global being 0.1 at draw (ModModels.cpp:626). The snapshot-captured value must default to 0.1 to
  avoid a visible alpha change.
- **QD-21** `[chunk-workers §5.3/§8]` — **nearLane upload policy.** Exempt entirely vs dedicated
  budget slice vs byte-based budget (60 B/vertex, Tessellator.hpp:23). Recommend a dedicated slice.
- **QD-22** `[arch §6.5; plan WI-13]` — **Exact teardown order** for GL-shared-context share group:
  `ShaderCompileService::stop` → world/lighting stop → `DisplayManager::destroy` →
  `RegionIo::flush` → server coordinator → audio. Encode in `Lifecycle`, not member-destructor order.
- **QD-23** `[plan D9]` — **`stall-trace.log` format consumers.** No external tooling known to depend
  on the PHASE/FRAME line format; confirm during WI-12.
- **QD-24** `[inventory §12.4]` — **Image/skin apply consumer contract.** Confirm skin apply is
  main-thread-only after the channel drain (no other reader of `image`/`slimArms`).
- **QD-25** `[mainthread-map §7.1]` — **Explicitly reject a tick/render thread split for this
  refactor.** The seam exists (`runWorldSimulation`, Minecraft.cpp:555), but `World` is not
  thread-safe and parity demands one thread. Confirm no future desire — the plan is main-thread-only.
- **QD-26** `[net §5 deviation 3]` — **Chunk-packet rate gate parity.** Java gates chunk sends with a
  countdown (`field_20175_w`=50); C++ alternates `preferImmediate` (Connection.cpp:296-312). Accept
  the loose approximation? (Server already throttles via `getBlockDataSendQueueSize`,
  ServerPlayNetworkHandler.cpp:65-70 / `playerManager.updateAllChunks`.)
- **QD-27** `[plan §7 / initial-planner brief]` — **CONTEXT.md is missing** at
  `docs/agent-notes/CONTEXT.md`. Flag to auditors; the plan stands on the council docs.

---

## 4. Doc disagreements, ranked by severity

### 4.1 MUST-RESOLVE (blocking or factually wrong)

| # | Disagreement | Docs | Severity rationale |
|---|---|---|---|
| M1 | **Domain pools (architecture) vs one shared tagged pool (chunk-workers §8)** | arch §3.1 vs chunk-workers §8 | Incompatible architectures; everything downstream (WI-2/4/6/7) depends on it. Plan already chose domain pools — auditors must ratify. |
| M2 | **Loader pool = 3 (network §2) vs 4 (inventory §2.3, chunk-workers §3)** | network vs inventory/chunk-workers | Factual error. Code = 4 (`ChunkCache.cpp:217`). Wrong counts would mislead the budget table. |
| M3 | **`recommendedThreadCount` site count: "at least 5"/"6" vs actual 4 + 2 `hw−2`** | inventory §2.3/§TL;DR vs verified tree | Factual. Missed sites (WorkerHandoff.hpp:14 unused default) would be deleted/kept incorrectly in WI-2. |
| M4 | **"Never join on game thread" (network §9) vs "watchdogged join" (arch §3.3/§6.4)** | network §9 vs arch §6.4 | Semantic conflict if `Lifecycle::shutdown()` runs on the game thread. Must split: runtime disconnect = never join; shutdown = unblock-then-watchdog-join (2–5 s, leak). |
| M5 | **Java client source absence (authoritative-mirror question)** | java-thread-model §0, chunk-workers §7 vs all other docs | All client-loop parity claims rest on the C++ port. Must be recorded as an accepted assumption (QD-16). |
| M6 | **Line-number drift across all six docs for edited files** | all council docs vs working tree | Mechanical but must-resolve: executors following stale lines will edit the wrong code. Use §7 corrections. |

### 4.2 NICE-TO-RESOLVE (design details; auditors should confirm but executors can proceed)

| # | Disagreement | Docs | Note |
|---|---|---|---|
| N1 | Lighting own lane vs shared queue | arch §9.1 / plan D3 vs chunk-workers §8 | Confirmed plan lean (own lane) — resolve before WI-6, not blocking earlier items. |
| N2 | Hazard ranking emphasis (#1 oversubscription vs R1 "dominant") | inventory §10.1 vs chunk-workers §4 | Both in plan; ranking affects narrative only. |
| N3 | nearLane exempt vs slice | chunk-workers §8 vs plan WI-4 | WI-4 implements; exact policy is QD-21. |
| N4 | Lighting-ready gate now vs defer | chunk-workers §8 vs plan WI-7/D5 | Scope of WI-7 only. |
| N5 | Read-side cap: disconnect vs backpressure | net Q2 vs plan D2 | Both fine; pick disconnect (plan). |
| N6 | Event loop vs pool | net Q1 vs plan D1 | Pool first (plan); re-evaluate later. |
| N7 | fpsLimit=0 parity | arch Q2 vs plan D10 | Keep parity. |
| N8 | Audio 4 threads keep vs fold | arch Q4 vs plan D12 | Keep pinned. |
| N9 | Stall mutexes keep vs collapse | arch §4 vs plan D9 | Keep until WI-12. |
| N10 | Server/coordinator scope | net Q6 vs plan D11 | Defer (WI-14). |
| N11 | VBO growth off main | chunk-workers §8 vs plan (reserve only) | Defer; reserve worst-case (QD-19). |

---

## 5. Recommended resolution for each MUST-RESOLVE disagreement

- **M1 (domain pools vs mega-pool).** Adopt architecture §3.2: `ThreadCoordinator` + `ThreadBudget`,
  one global budget (`max(1, hardwareThreads − reserved)`, reserved = 2), a **small fixed set of
  domain pools** (`Compute`, `Io`, `GlCompile`, pinned `Audio`/`NetIo`/`Log`), `reserveDynamic`/`release
  Dynamic` for per-connection threads, `totalPending()` for F3. Rationale: three roles genuinely cannot
  be task-scheduled onto one pool — shader compile needs GL-shared contexts bound to the thread for its
  lifetime; blocking socket/disk I/O must never occupy a tessellation worker; near-camera mesh needs
  `INT_MIN`-style priority without starving logging. chunk-workers' "tagged single pool" would require
  context handoff and priority isolation the codebase doesn't have. **This also matches the plan's
  WI-1/WI-2/WI-6/WI-7 exactly.**
- **M2 (loader = 3 vs 4).** It is **4** on 16-logical. `ChunkCache::ensureLoaderPool` uses
  `recommendedThreadCount(3, 2, 4)` (ChunkCache.cpp:216-218) = `clamp((16−2)/3, 1, 4)` = 4.
  Correct network-threading §2's table when auditors pass it on; do not propagate "3".
- **M3 (site count).** Precise inventory for WI-2: delete the definition (WorkerPool.hpp:61-68); replace
  the **3 live call sites** (ChunkBuilder.hpp:156, ChunkCache.cpp:217, LightingEngine.cpp:49); handle the
  **unused default** at WorkerHandoff.hpp:14 (delete or leave, but do not forget it exists — a future
  `WorkerHandoff` constructed without an explicit count would resurrect a budget-less pool);
  route the **2 `hw−2` sites** (ShaderCompileService.cpp:47, GLCore.cpp:289) through the budget.
- **M4 (never-join vs watchdog-join).** Split by phase: (a) **runtime** `Connection::disconnect()` and
  all per-frame/teardown-on-stack paths → **never join**; use the existing retire pattern
  (Minecraft.cpp:778-788, MultiplayerSession.cpp:12-19) + scheduler-owned teardown, exactly as network
  §9 describes and WI-10 implements. (b) **final shutdown** (`Lifecycle::shutdown()` on the main thread,
  process exiting) → unblock-first, then `request_stop`, then join **with a 2–5 s watchdog that logs and
  leaks** rather than `std::terminate` (arch §3.3/§6.4; plan WI-13). This reconciles the two statements.
- **M5 (client mirror).** Ratify the C++ port (`src/net/minecraft/client/`) as the authoritative mirror
  of the b1.7.3 client loop for this refactor. Rationale: the client Java tree is absent everywhere in the
  repo (java-thread-model §0 searched the whole tree), the C++ port self-describes as a faithful port
  (Minecraft.hpp:3), and the server/shared Java classes (`third_party/mcp`) already match the C++ network
  machinery 1:1 (network §5). Note it in the plan as an assumption; re-verify only if byte-level client
  fidelity is later demanded.
- **M6 (line drift).** Adopt the working-tree numbers in §7 as the canonical citations for edited files;
  instruct executors to grep-then-edit (never trust any doc's number blindly, including this synthesis).
  The plan's own numbers are already current-tree-accurate for the drifted files it re-verified.

---

## 6. Single source of truth summary table

Hazard → verified file:line → owning work item → resolution. (IDs match §2.)

| ID | Hazard | file:line (verified working tree) | Owner WI | Resolution |
|---|---|---|---|---|
| HZ-01 | No global thread budget; oversubscription | ChunkBuilder.hpp:156; ChunkCache.cpp:217,224; LightingEngine.cpp:48-49; ShaderCompileService.cpp:46-48; GLCore.cpp:288-290; WorkerHandoff.hpp:14 | WI-2 | Delete `recommendedThreadCount`; single `ThreadCoordinator` budget, domain pools |
| HZ-02 | `drain()`/`cancelAll()` block main | WorkerPool.hpp:50-53; WorkerHandoff.hpp:35-40; WorldRenderer.cpp:373; ChunkCache.cpp:26-35,349-355 | WI-4, WI-7 | Non-blocking cancel + epoch tokens; budgeted drains |
| HZ-03 | `retireFromLighting` spin | ChunkCache.cpp:61-67 (:78,:440) | WI-4 | CV signal on pin release |
| HZ-04 | `compileBlocking` main-thread wait | ShaderCompileService.cpp:144-177 | WI-8 | Submit + frame poll; blocking only for fallback |
| HZ-05 | Socket joins on game thread | Connection.cpp:157-160,134-137,352-360,17-20 | WI-10 | Async teardown; never join at runtime |
| HZ-06 | HTTP/auth joins on main | ClientNetworkHandler.cpp:243-245,49-54; ServerLoginNetworkHandler.cpp:150-152; LoginScreen.cpp:224-226 | WI-9, WI-11 | Result-only workers; Io pool; no join-to-reuse |
| HZ-07 | Blocking saves at teardown | World.cpp:312-314; ChunkCache.cpp:349-355 | WI-13 | Keep in dedicated teardown phase |
| HZ-08 | Sync HTTP/file/decode on main | ClientNetworkHandler.cpp:258-320; TextureManager.cpp:451-483; ScreenStack.cpp:33-38; WorldSession.cpp:159-198 | WI-11 (+context) | Move asyncable downloads/decodes; `prepareWorld` stays (parity) |
| HZ-09 | Serial sim CPU (tick entities etc.) | WorldEntities.cpp:386-500; WorldChunks.cpp:456-575 | — (out of scope) | Keep on main; document that tick spikes = frame spikes |
| HZ-10 | `cancelPending` drops queued work; owners lose count | WorkerPool.hpp:45-48; ChunkBuilder.hpp:105; WorldRenderer.cpp:581; ChunkCache.cpp:226-249 | WI-4, WI-7 | Epoch/generation tokens; explicit cancelled path |
| HZ-11 | `~ChunkMeshJob` writes `meshJobInFlight` (R3) | ChunkBuilder.cpp:220-225 | WI-4 | Keep main-thread-only destruction; atomic or assert |
| HZ-12 | R1: snapshot memcpy vs writers | RegionSnapshot.cpp:32-67 (:54-66); LightingEngine.cpp:249-258; Chunk.cpp:61-89; ChunkCache.cpp:173,373-384 | WI-3 | Per-chunk light lock (or version-stamped copy) |
| HZ-13 | R4: light writes vs save/adopt snapshot | ChunkCache.cpp:356-372,156-201 | WI-3 | Same lock as R1 |
| HZ-14 | GL race: worker writes `g_alphaTestRef` | ModModels.cpp:626; RenderCore.cpp:70,671-677,479-481 | WI-5 | Capture alpha ref into job/snapshot; main-only write |
| HZ-15 | `GLCore::init` `g_loaded` race | GLCore.cpp:121,141-145 | WI-5 | `std::once_flag`/atomic |
| HZ-16 | `ImageDownload` detached-thread race | ImageDownload.cpp:7-18 | WI-11 | Channel + main-thread apply; no detach |
| HZ-17 | `Packet::read` static accounting race | Packet.hpp:79-81,121-128 | WI-9 | Per-`Connection` counters, merged on game thread |
| HZ-18 | Login verify cross-thread disconnect + `closed` race | ServerLoginNetworkHandler.cpp:154,172,175,64,51-53; ConnectionListener.cpp:116 | WI-9 | Verify publishes result only; mutation on tick thread |
| HZ-19 | `disconnectReason_` unsynchronized | Connection.cpp:339 vs :225 | WI-9/10 | Document; keep write-before-close |
| HZ-20 | `World::save` async snapshot race | World.cpp:325-339; World.hpp:429 | WI-11 | Value snapshot on Io pool; `save(true)` waits |
| HZ-21 | `ChunkNibbleArray` torn bulk reads | ChunkNibbleArray.hpp:12-13 | — | Accepted transient |
| HZ-22 | `MinecraftServer::stopped` plain-bool spin | DedicatedServerGui.cpp:155 | WI-11 | Atomic + registered GUI thread |
| HZ-23 | Mod `BakedModel*` outlive store lock | ModModels.cpp:52-60,158-165 | — | Note; enforce store lifetime |
| HZ-24 | `ResourceDownloadThread` mutates main objects | ResourceDownloadThread.cpp:138,168 | WI-11 | Files/bytes only; main applies |
| HZ-25 | Connector thread calls `setWorld` | MultiplayerConnector.cpp:17-61,32,40,43; ClientNetworkBridge.cpp:149-158; ClientNetworkHandler.cpp:115-127 | WI-11 | Publish under mutex; main applies |
| HZ-26 | `message` publication by mutex handoff | ClientNetworkBridge.cpp:137; MultiplayerConnector.cpp:57; ConnectScreen.cpp:63 | WI-9 | Keep explicit under scheduler |
| HZ-27 | Plain-field auth races | ClientNetworkHandler.cpp:242; ServerLoginNetworkHandler.cpp:104,128 | WI-9 | Copy-by-value / result-only verify |
| HZ-28 | Detached threads (4 sites) | ImageDownload.cpp:7-18; SessionValidator.cpp:51; ClientDiagnostics.cpp:264; DedicatedServerGui.cpp:81 | WI-11 | Io tasks / registered threads |
| HZ-29 | GL-context-thread discipline | GLCore.cpp:141-145; Minecraft.cpp:354 vs :388 | WI-5/8/13 | Main-only GL; shared-context teardown first; `TL_DOMAIN` asserts |
| HZ-30 | `ChunkRegionBuffer` full-mirror re-upload | ChunkRegionBuffer.cpp:50-62; WorldRenderer.cpp:550-557 | WI-12 (QD-19) | Worst-case reserve; off-main growth deferred |
| HZ-31 | nearLane upload unimplemented | ChunkMeshJob.hpp:51-54; ChunkBuilder.hpp:135; WorldRenderer.cpp:747-770 | WI-4 | Honor `job->nearLane` in `processUpload` |
| HZ-32 | Lighting→mesh double-mesh | World.cpp:711-717; WorldRenderer.cpp:1117 | WI-7 (D5) | Lighting-ready gate (optional) |
| HZ-33 | Priority not cross-pool | ChunkCache.cpp:498 | — | Note; partial mitigation exists |
| HZ-34 | Box-conflict scan O(queue×active) | LightingEngine.cpp:70,149-170 | WI-6 | Keep claim logic; channel capacity |
| HZ-35 | Large sky boxes self-resubmit | LightingEngine.cpp:286-310,392-439 | WI-6 | Channel backpressure bounds it |
| HZ-36 | `workerGenerators_` never pruned | ChunkCache.cpp:85-105 | WI-7 | Prune on world switch |
| HZ-37 | Main-thread `decorate` under `ioMutex_` | ChunkCache.cpp:179-195,373-384 | — (parity) | Stays main-thread |
| HZ-38 | Two `stallMutex` blocks | Minecraft.cpp:742-743,827-828 | WI-12 | Collapse into `FrameProfiler` (keep until then) |
| HZ-39 | No unified per-frame budget | FrameBudget.hpp:5-14; WorldRenderer.cpp:742-743,787-789; ChunkCache.cpp:424,473,483; Connection.cpp:198-201 | WI-12 | One `FramePipeline` deadline |
| HZ-40 | One-off `std::launch::async` | World.cpp:325-338 | WI-11 | Io pool |
| HZ-41 | Deferred-destruction discipline | Minecraft.cpp:778-788; MultiplayerSession.cpp:12-19; ClientNetworkHandler.hpp:144-151 | WI-10/13 | Preserve retire-once-unwound |
| HZ-42 | Main-only GL window/present | GLCore.cpp:297-318; DisplayManager.cpp:158-161 | WI-12 | Keep main-thread-only |

---

## 7. Verification log — planner/council citation corrections

All of the following were spot-checked against the working tree on 2026-08-01.

### 7.1 Planner claims that check out (accurate)

- **Two `stallMutex` blocks at Minecraft.cpp ~742/827** — confirmed at **:742-743** (`runRenderPhase`)
  and **:827-828** (`run()`), both `#ifdef MINECRAFT_RENDER_TRACE`. (architecture §1's "~810 and ~730"
  is the stale number.)
- **R1 race in RegionSnapshot.cpp:54-66** — confirmed; the memcpy column loop is :54-66 inside
  `copyChunkBand` (:32-67). Writers at LightingEngine.cpp:249-258, ChunkCache.cpp:173/:373-384.
- **Connection.cpp:131-132** (reader/writer spawn) — confirmed.
- **`recommendedThreadCount` sites the planner lists for WI-2** — WorkerPool.hpp:61-68 (definition),
  ChunkBuilder.hpp:156, LightingEngine.cpp:49, ChunkCache.cpp:217 (loader) / :224 (save, fixed 1),
  ShaderCompileService.cpp:47, GLCore.cpp:289. All correct.
- **`run()` at Minecraft.cpp:762-843; tick-order invariant at :647-649; `runRenderPhase` :670-761;
  `stop()` :347-393** (ShaderCompileService::stop :354, DisplayManager::destroy :388, `std::_Exit` :391) —
  all confirmed.
- **ServerLoginNetworkHandler.cpp:64 (`closed`), :150-152 (join), :172/:175 (failure disconnect)** —
  confirmed. **ConnectionListener.cpp:116 reads `closed`** — confirmed.
- **ClientNetworkHandler.cpp:49-54 (dtor join), :55-83 (processPendingJoinServer), :115-127
  (disconnect→setWorld), :243-245 (beginPendingLogin join)** — confirmed.
- **DownloadingTerrainScreen.hpp:19-26** (keep-alive :21-23, drain :25) and **ClientWorld.cpp:53-55**
  (drain) — confirmed; H7 double-drain is real.
- **World.cpp:300-339 (`save`), World.hpp:429 (`asyncSaveFuture_`), :331-338 snapshot** — confirmed.
- **ChunkCache.cpp:212-219/:220-225/:226-249/:250-288/:349-355/:373-384/:61-67** — confirmed.
- **WorkerPool.hpp:45-48 (`cancelPending`), :50-53 (`drain`); WorkerHandoff.hpp:35-40 (`cancelAll`)** —
  confirmed.
- **GLCore.cpp:121 (`g_loaded`), :141-145 (`init`), :286-291 (driver hint)** — confirmed.
- **ModModels.cpp:626 (`setAlphaTestRef(0.1f)`)** — confirmed; RenderCore.cpp:70/:671-677/:479-481 —
  confirmed (note :680 is `getAlphaTestRef`, harmless off-by-few from plan's ":671-680").
- **Packet.hpp:79-81, :121-128** — confirmed.
- **RegionSnapshot.cpp memcpy** — confirmed (see above).
- **`nearLane` unimplemented** — confirmed: set at ChunkBuilder.hpp:135, comment at ChunkMeshJob.hpp:51-54,
  **never read** in `processUpload` (WorldRenderer.cpp:747-770).
- **ChunkBuilder.cpp:132-197 (`capture`), :220-225 (dtor), :226-252 (captureSnapshot/releasePins),
  :253-351 (`buildMesh`)** — confirmed; the snapshot copy runs **on the worker** (pins held), not main.

### 7.2 Planner/council citations that are wrong or drifted (corrected)

| Claim | Doc cited | Verified reality (working tree) | Fix |
|---|---|---|---|
| `Minecraft::run()` at :749; `while(running)` :760 | all six council docs | `run()` at **:762**; `while(running.load())` at **:773** | Use :762-843 |
| `pingMainLoopHeartbeat` :763; `flushRetired` :765; `timer.advance` :788-794; tick loop :796-805; `runRenderPhase` :807; stall write :808-825 | mainthread-map §1, java-thread-model §1.1 | :776, :778, :801-807, :809-818, :823, :824-843 | Use the +13-shifted numbers |
| `MinecraftServer` loader row "3 workers" | network-threading §2 | **4** (`rec(3,2,4)`, ChunkCache.cpp:217) | Correct to 4 |
| "at least 5"/"6" `recommendedThreadCount` sites | inventory §TL;DR/§2.3; task brief | **4** live/declared call sites (3 explicit + 1 unused default at WorkerHandoff.hpp:14) + 2 `hw−2` sites | Precise count in §4.1 M3 |
| `meshJobInFlight=true` set at WorldRenderer.cpp:577 | chunk-workers §3, plan WI-4 | **:581** | Use :581 |
| `clearSections` at :368-400, `cancelAll` at :369 | inventory §10.2, chunk-workers §5.1 | `clearSections` at **:372-404**, `cancelAll` at **:373** | Use :372/:373 |
| `compileChunks` at WorldRenderer.cpp:727-825 | chunk-workers §2.4, plan §3 | function at **:731** | Use :731 |
| `cullChunks` :949-1011; `applyOcclusionCulling` :1012 | mainthread-map §3 | :953; :1031 | Use current |
| `markDirty` :1098-1121; `blockUpdate` :1122; `setBlocksDirty` :1170 | chunk-workers §2.1, mainthread-map | :1117; :1141; :1189 | Use current |
| `renderChunksVbo` :603-650 / :618-643 | chunk-workers §5.4, mainthread-map §3.9 | :607-654 | Use current |
| `reload()` reserve at WorldRenderer.cpp:546-553 | chunk-workers §8, plan WI-4 | reserve at **:550-557** | Use current |
| `compileChunks` call at GameRenderer.cpp:1166-1169 | chunk-workers §2.4, inventory §1 | **:1195** | Use :1195 |
| `renderChunks`/entity staging WorldRenderer.cpp:826+ | mainthread-map §3.9 | (unverified; grep before use) | re-verify |
| `startMeshJob` :564-585 | chunk-workers §2.1 | :568-589 | Use :568 |
| `enqueueDirtyChunk` :195-201 / :195-214 | chunk-workers §2.1, plan WI-7 | :199-205 | Use :199 |
| Two `stallMutex` at "~810 / ~730" | architecture §1 | :827-828 / :742-743 | Use plan's numbers |
| `GLCore.cpp:288` driver hint | architecture §1 | `parallelShaderCompileSupported` block :286-291, `maxShaderCompilerThreadsKHR(threads)` at :290 | Use :286-291 |

### 7.3 Facts confirmed as "as-documented" (no correction needed)

- Client steady-state ≈27 threads / peak ≈30-31; server ≈21-23 (inventory §11) — arithmetic re-derived
  and consistent.
- `ChunkNibbleArray` CAS set / relaxed get, non-atomic bulk copy (ChunkNibbleArray.hpp:28-29, :35-51,
  :12-13) — confirmed.
- Render pin protects eviction only (Chunk.hpp:205-224 `tryAcquireRenderPin`/`beginRenderEviction`) —
  confirmed; `beginRenderEviction` stores `renderEvicting_` then checks the count, which is exactly why
  `retireFromLighting`'s `while(!beginRenderEviction())` spin works and stalls.
- Java network thread inventory (per-connection read/write threads, watchdog threads, `processReadPackets`
  ≤100/tick, `dataPackets`-before-chunkData, `NetworkMasterThread` async shutdown) — matches
  `third_party/mcp/net/minecraft/src/NetworkManager.java` per java-thread-model/network-threading.
- Iris 26.1 render-thread-only GL (`IrisRenderSystem.java:42`, `ProgramBuilder.java:36,72`), pack-source
  read on `newFixedThreadPool(10)` joined on the render thread (`ProgramSet.java:64-90`), `glMaxShaderCompilerThreads`
  hint (`Iris.java:133-137`), and BEGIN-reads-last-frame-shadow are treated as hard parity invariants —
  consistent across java-thread-model §3-6 and plan §2.

---

## 8. What the auditors must specifically verify next

1. **Ratify M1 (domain pools + `ThreadCoordinator`)** — the single most consequential decision; the
   plan already encodes it, but chunk-workers proposed the opposite.
2. **Ratify QD-03 (lighting lane)** before WI-6 lands.
3. **Confirm QD-07 (present-before-draw)** before WI-12 hard-codes it.
4. **Decide QD-18 (epoch/cancel acknowledgment)** before WI-4 — it is the subtle correctness core of
   non-blocking cancel.
5. **Check the two claimed "unused" pieces**: `WorkerHandoff.hpp:14` default arg (currently unused) and
   `capturedThread` static map (MinecraftServer.cpp:487-500, dead) — both are WI-2/plan "cleanup"
   candidates worth confirming.
6. **CONTEXT.md is missing** — flag as a pipeline-input gap (QD-27).
7. **All line numbers in this synthesis are working-tree-accurate as of 2026-08-01**; the tree has
   uncommitted edits, so executors must still grep-before-edit.
