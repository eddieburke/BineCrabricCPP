# Council Findings — Chunk Mesh Building / Render Data Pipeline

Review-only notes for the multithreading restructure. Focus: chunk meshing, chunk
loading, lighting, and GPU upload coordination with the main render thread.
No code was edited; no build was run.

Session date: 2026-08-01. C++20 Beta 1.7.3 port at repo root `src/`.

---

## 1. Executive summary

The pipeline is already *structurally* modern — meshing, lighting, chunk
load/generate, and save are all off the main thread — but there is **no coherent
scheduler**. Four independent thread pools (mesh, lighting, loader, save) plus a
separate GL-sharing shader-compile pool each size themselves by assuming exactly
3 competing pools (`WorkerPool::recommendedThreadCount(3, 2, N)`), so total
workers overshoot the machine on any core count. The GL upload of built geometry
is correctly main-thread-only; **no mesh worker touches GL**. The dominant
correctness risk is a **lock-free data race**: the mesh worker memcpy's live
`Chunk` block/light arrays into a `RegionSnapshot` while (a) the main thread
writes blocks during tick/decoration and (b) the lighting engine writes light
nibbles on its own workers. The render pin protects *eviction/lifetime* only,
not *contents*. Secondary issues: the near-camera "skip budget" upload
(nearLane) is documented but **not implemented**; lighting→mesh has no
"lighting-ready" gate so freshly lit chunks are meshed twice; `ChunkRegionBuffer`
realloc re-uploads the *entire* CPU mirror on the main thread; `cancelAll()`
drains the pool synchronously and stalls the main thread on teleport/reload.

---

## 2. Data-flow map (with file:line)

### 2.1 Mesh job creation (main thread)

- `WorldRenderer::markDirty` — `WorldRenderer.cpp:1098-1121`. Called from
  `setBlocksDirty` (`:1170-1172`, +1 block) and `blockUpdate` (`:1122-1124`,
  +1 block). Iterates affected 16³ sections, `builder->invalidate()` (bumps
  `version`, `ChunkBuilder.hpp:76-79`) and `enqueueDirtyChunk`.
- `enqueueDirtyChunk` — `WorldRenderer.cpp:195-201`. Skips if
  `meshJobInFlight`; then `noteNearDirty` (`:202-214`, within 32 blocks,
  capped 64).
- `startMeshJob` — `WorldRenderer.cpp:564-585`. Guards
  `meshJobInFlight`/`dirty` (`:569`), then `ChunkMeshJob::capture` (`:572`,
  main thread).
- `ChunkMeshJob::capture` — `ChunkBuilder.cpp:132-197`. Computes the 3×3
  chunk neighborhood (`:143-150`), acquires a **render pin** on each loaded
  chunk (`tryAcquireRenderPin`, `:163`), clones the biome source
  (`:174-176`), snapshots `lightLevelToLuminance` and the active shader pack's
  `blockRenderLayers` (`:189-195`). All main-thread reads of live state.
- Priority: `enqueueNear` = `INT_MIN` (nearLane), `ChunkBuilder.hpp:134-137`;
  normal = `ringOf(section)` (`WorldRenderer.cpp:817`). Lower value pops first
  (`WorkerPool.hpp:75-81`).

### 2.2 Mesh build (worker thread)

- `ChunkMeshScheduler::enqueue` — `ChunkBuilder.hpp:122-133`. Wraps the job in a
  `WorkerHandoff` lambda: `ChunkBuilder::buildMesh(job)` with a try/catch that
  sets `job->failed`.
- `WorkerHandoff::enqueue` — `WorkerHandoff.hpp:22-30`. `pool_.submit(work;
  push to completed_)`. Workers do **not** touch the main-thread `completed_`
  vector except under `mutex_` (`:26-27`).
- `buildMesh` — `ChunkBuilder.cpp:253-351`. First calls
  `job->captureSnapshot()` (`:257-259`) which is the *expensive memcpy of the
  3×3 band* — this runs **on the worker while pins are held**
  (comment `:254-256`). Then `columnHasBlocks` early-out (`:271`),
  `computeVisibilityBits` flood-fill (`:277`, `:48-130`), and per-layer
  tessellation against the snapshot (`:283-349`). Block-entity *positions* are
  recorded, not pointers (`:309-311`); the live pointer resolve happens at
  upload on the main thread (`:373-386`).
- `captureSnapshot` / `releasePins` — `ChunkBuilder.cpp:226-252`. After the
  `RegionSnapshot` ctor the pins are released, so workers are detached from live
  chunks for the rest of meshing. **The copy itself is the racy window**
  (see §4).

### 2.3 Snapshot design

- `RegionSnapshot` — `RegionSnapshot.hpp:22-118`. Immutable `BlockView` copy.
  `getBlockEntity` returns nullptr by design (`:41-45`). Per-chunk band copy:
  blocks (1B/cell), meta/skyLight/blockLight nibbles (`:76-83`).
- `RegionSnapshot` ctor — `RegionSnapshot.cpp:69-119`; `copyChunkBand`
  (`:32-67`) memcpy's `chunk.blocks`, `chunk.meta.bytes`,
  `chunk.skyLight.bytes`, `chunk.blockLight.bytes` (`:60-65`) **with no lock**.
- Light queries read the copy; outside the captured region they assume
  fully-sky-lit (`:203-209`) to avoid seams.

### 2.4 Handoff and GPU upload (main thread)

- `compileChunks` — `WorldRenderer.cpp:727-825`. Called once per frame from
  `GameRenderer.cpp:1166-1169` (after `cullChunks`).
  - Backlog estimate `:733-735`; upload `FrameBudget` 3 ms (6 ms under loading
    backlog), min 1–6 uploads (`:736-739`).
  - `processUpload` (`:743-766`): drops retired jobs (`:748-751`), defers past
    budget (`:752-755`), drops stale/failed jobs by `version` mismatch and
    re-enqueues (`:757-761`), else `builder->uploadMesh(*job)` (`:762`) — **on
    the main thread**.
  - Enqueue gating: `targetInFlight = workerCount * 3` (*6 if `force`),
    `:776`; capture budget 1 ms (2 ms on load/force) `:783-785`; near-dirty
    lane first `:788-801`, then rings in ascending order `:802-822`.
- `uploadMesh` — `ChunkBuilder.cpp:352-415`. `++chunkUpdates` (`:353`),
  `region_->layers[layer].upload(...)` for each non-empty layer (`:357-372`),
  block-entity resolve against the live world (`:378-386`), block-entity
  add/remove diff into the shared list (`:387-403`), commits `hasSkyLight`/
  `visBits` (`:404-405`), then **uploads mod meshes to GL** via
  `mesh.uploadToGpu()` (`:409-413`).
- `ChunkRegionBuffer::upload` — `ChunkRegionBuffer.cpp:74-111`. `std::copy`
  into the CPU mirror `shadow_` (`:97`), then `glBufferSubData` (`:105-108`).
  On growth: `reallocBuffer` (`:50-62`) does `glBufferData` (orphaning) **plus
  a full `bufferSubData` of the entire mirror** (`:56-61`) — a main-thread
  stall proportional to all geometry uploaded so far.
- Draw: `renderChunksVbo` — `WorldRenderer.cpp:603-650` collects visible slots
  per frame; `ChunkRegionBuffer::flush` (`ChunkRegionBuffer.cpp:184-249`)
  builds merged ranges and issues `glDrawElements` per merged range. All main
  thread. Note the `logRenderf` DEBUG-TRACE per flush (`:186-201`, `:246-248`).
- Shared single pool: `ChunkRegionManager::pool()` (`ChunkRegionBuffer.hpp:83-88`)
  — **one VBO per layer shared by every section**.

### 2.5 Lighting engine (worker threads)

- `LightingEngine` — `LightingEngine.hpp:24-97`. **3** jthreads
  (`LightingEngine.cpp:46-57`, `recommendedThreadCount(3, 2, 3)`).
  Box-split propagation (`runUpdate`, `:286-440`), sharding by non-overlapping
  boxes (`conflictsWith`, `:58-64`), per-worker chunk pin cache
  (`chunkAt`/`releasePins`, `:207-233`).
- **Workers write live chunk light arrays**: `setBrightness` → `chunk->setLight`
  (`LightingEngine.cpp:249-258` → `Chunk.hpp:133-140`), no lock.
- Handoff: `outbox_`/`drainDirtyRegions` (`:113-126`). Main thread drains each
  frame (`World.cpp:711-717` → `events_.setBlocksDirty` per region), called from
  `Minecraft.cpp:681` (`runRenderPhase`) and server `MinecraftServer.cpp:378,518`.
  `setBlocksDirty` → `WorldRenderer::setBlocksDirty` (`WorldRenderer.cpp:1170`)
  → `markDirty` → invalidate + enqueue → new mesh job.
- Block edits on the main thread queue light work: `Chunk::setBlock`
  (`Chunk.cpp:61-89` → `queueLightUpdate` at `:85-86`).

### 2.6 Chunk loading / saving (worker threads)

- `ChunkCache` — `ChunkCache.hpp:22-96`. `loaderPool_`
  (`recommendedThreadCount(3, 2, 4)`, `ChunkCache.cpp:212-219`), `savePool_`
  (1 thread, `:220-225`).
- `requestChunkAsync` — `ChunkCache.cpp:226-249`. Per-thread generator clones
  (`workerGenerator`, `:85-105`; `createChunkGeneratorFromSeed` forces
  `localBiomeSource=true`, `Dimension.cpp:51-56` — so workers use private noise/
  biome state). Storage reads serialized under `ioMutex_` (`:111`).
- Integration on main thread: `integrateFinishedLoads` (`:250-288`, budget
  `2`/frame from `tick`, `32` from `pumpChunkPublish`), `adoptChunk`
  (`:156-201`) runs `populateBlockLight` (`:173`), `load()` (`:174`), and
  **`decorate` on the main thread under `ioMutex_`** (`:176-196`, `:373-384`),
  then `chunkAvailable` (`:198`) → `WorldRenderer::chunkAvailable`
  (`WorldRenderer.cpp:1125-1138`) → `enqueueColumn` + `pendingBorderRefresh_`.
- Eviction: `retireFromLighting` **spins** on render pins
  (`ChunkCache.cpp:61-68`, `sleep_for(200µs)`), used in `unloadChunk` (`:78`)
  and `tick` (`:440`).

### 2.7 Shader compile service (only workers that touch GL)

- `ShaderCompileService.cpp:41-61` — `min(hw-2, 4)` workers, each with its own
  GLFW window **sharing the main context** (`createWorkerWindow`, `:8-19`).
  `workerMain` (`:224-249`) makes the shared context current (`:225`) and
  compiles shaders to program *binaries* (`:236`). The main thread links
  (`peekCompleted`, `:118-121`, documented main-thread-only).
- This pool is separate from the mesh pool and competes for the same cores; it
  also holds the GL **shared context** current on worker threads, so driver
  serialization can overlap with the main thread's linking/drawing.

---

## 3. Thread inventory and oversubscription

All pools call `recommendedThreadCount` (`WorkerPool.hpp:61-68`) with
`competingPools=3, reservedThreads=2`:

| Pool | Size formula | hw=16 | hw=8 |
|---|---|---|---|
| Mesh (`ChunkBuilder.hpp:156`) | `(hw-2)/3`, ≤6 | 4 | 2 |
| Lighting (`LightingEngine.cpp:49`) | `(hw-2)/3`, ≤3 | 3 | 2 |
| Loader (`ChunkCache.cpp:217`) | `(hw-2)/3`, ≤4 | 4 | 2 |
| Save (`ChunkCache.cpp:224`) | fixed 1 | 1 | 1 |
| Shader compile (`ShaderCompileService.cpp:47-48`) | `min(hw-2,4)` | 4 | 4 |
| **Workers total** | | **16** | **11** |
| + main render/tick thread | | **17** | **12** |

On hw=8 logical cores this is ~1.5× oversubscription even before the shader
pool; each pool independently assumes the other two pools don't exist, and the
save pool (+1) and shader pool (+up to 4) are counted by nobody. During world
load + pack reload (meshing + lighting + loader + shader all hot), the CPU is
contended and the main thread competes with 10-16 workers. There is no shared
"worker budget" object anywhere; coordination is by convention only.

---

## 4. Races (correctness)

### R1 — Mesh snapshot memcpy vs lighting workers (active, benign-ish UB)
`RegionSnapshot` ctor (`RegionSnapshot.cpp:54-66`) memcpy's live
`chunk.skyLight`/`blockLight`/`blocks`/`meta` while lighting workers write
nibbles via `setBrightness` (`LightingEngine.cpp:249-258`) and the main thread
writes blocks via `Chunk::setBlock` (`Chunk.cpp:61-89`) / decoration
(`ChunkCache.cpp:373-384`) / `populateBlockLight` (`ChunkCache.cpp:173`).
The render pin (`Chunk.hpp:205-218`) blocks **eviction only**; it does not
block writers. Observed symptoms will be intermittent (torn band copies →
ghost/flickering faces at a section edge, baked stale light, and duplicate
rebuilds). This is an unsynchronized data race (UB), not just staleness.

### R2 — `retireFromLighting` spin + concurrent `chunk->unload()`
`unloadChunk`/`tick` (`ChunkCache.cpp:65-67, 78, 440`) wait for pins to drain;
a mesh copy or lighting box in progress delays the main thread. Bounded, but a
stall source during fast camera movement (see §5).

### R3 — `ChunkMeshJob::~ChunkMeshJob` writes `builder->meshJobInFlight`
`ChunkBuilder.cpp:220-225`. Safe *today* only because the last `shared_ptr`
always dies on the main thread (worker pushes the job into `completed_` before
dropping its copy, `WorkerHandoff.hpp:26-27`; `cancelAll` drains before
`completed_.clear()`, `WorkerHandoff.hpp:35-40`). Any future path that drops
the last reference on a worker (e.g. cancelling from a pool callback, moving
jobs between pools) becomes a race with the main thread's reads at
`WorldRenderer.cpp:196, 284, 569, 749, 756`. Fragile invariant, worth
documenting or making explicit (atomic or main-thread-only destructor
guarantee).

### R4 — Pools writing `Chunk` concurrently with main-thread iteration
Lighting workers write light nibbles while the main thread `saveChunk`
(`ChunkCache.cpp:356-372`) snapshots the chunk and while `adoptChunk` runs
`populateBlockLight`/`load`. Same class of race as R1, on the same arrays.

### Non-races (verified)
- Mesh workers never dereference `builder` during `buildMesh`; all inputs are
  job-copies (`ChunkMeshJob.hpp:37-74`). The `builder` pointer is only used
  by the main thread (version/retired checks) and the destructor (R3).
- No mesh/loader/save/lighting worker calls GL. Only `ShaderCompileService`
  workers make GL calls, and only with their own shared context current.
- Loader workers use per-thread generator clones with private
  `BiomeSource`/noise (`ChunkCache.cpp:97`, `Dimension.cpp:55`,
  `OverworldChunkGenerator.hpp:37-38`), so no shared mutable gen state.
- `decorate` is main-thread + `ioMutex_`-serialized (`ChunkCache.cpp:373-384`);
  storage reads serialized under `ioMutex_` (`:111`).
- Per-builder ordering: `meshJobInFlight` (single in-flight per section) +
  `version` check drops stale results (`WorldRenderer.cpp:757`); a new job for
  the same builder starts only after the old one completes.

---

## 5. Main-thread bottlenecks that could be async

1. **`cancelAll()` blocks the main thread.** `WorkerHandoff::cancelAll` →
   `pool_.cancelPending()` + `pool_.drain()` (`WorkerHandoff.hpp:35-40`),
   and `drain` waits for all *active* tasks (`WorkerPool.hpp:50-53`). Called
   from `clearSections` (`WorldRenderer.cpp:369`) on every teleport/reload
   (`updateSectionFrontier:425-435`). A 4-thread mesh pool with long jobs ⇒
   tens of ms stall, in the middle of the frame.
2. **`ChunkRegionBuffer` growth re-uploads everything.** `uploadMesh` →
   `reallocBuffer` (`ChunkRegionBuffer.cpp:50-62`) `glBufferData`s the whole
   VBO and re-uploads the whole CPU mirror `shadow_` on the main thread.
   The `reserve` in `reload` (`WorldRenderer.cpp:546-553`) mitigates initial
   load, but any overflow (dense terrain, big radius) still stutters.
3. **GPU upload of near-lane edits is not exempt from the budget.**
   `nearLane` is set at enqueue (`ChunkBuilder.hpp:135`) and stored
   (`ChunkMeshJob.hpp:54`) but is **never read in the upload path** —
   `processUpload` (`WorldRenderer.cpp:743-766`) applies the same
   `FrameBudget` to near and far alike, and the comment
   `ChunkMeshJob.hpp:51-54` ("its upload skips the per-frame time budget") is
   stale/unimplemented. A block edit next to the player can sit in
   `pendingMeshUploads_` for frames while distant sections consume the budget.
4. **Per-frame draw-list build + occlusion BFS** — `renderChunksVbo`
   (`WorldRenderer.cpp:618-643`), `buildMergedRanges` + `flush`
   (`ChunkRegionBuffer.cpp:157-249`), `applyOcclusionCulling`
   (`WorldRenderer.cpp:1012-1086`), `updateSectionFrontier`/`drainPendingColumns`
   (`:401-500`). All O(sections) main-thread. The flush also logs per-frame
   (`ChunkRegionBuffer.cpp:186-201, 246-248`).
5. **`retireFromLighting` spin loop** (`ChunkCache.cpp:65-67`) — main thread
   busy-waits for lighting/mesh pins to drain during unload.
6. **Entity-render staging** (`renderEntities`, `WorldRenderer.cpp:826-948`)
   — main-thread entity cull/render; out of scope here but competes with the
   above in the same frame.

---

## 6. Ordering hazards

- **Newer-before-older on the same builder is safe** (version check
  `WorldRenderer.cpp:757` + single in-flight). ✓
- **Near-vs-far completion order is not controlled.** `drainCompleted`
  (`WorkerHandoff.hpp:31-34`) returns jobs in *completion* order, which only
  approximates priority; a ring-5 section can be uploaded before a ring-1
  section that completed later. Not a correctness bug, but the near-lane edit
  latency is unbounded by the upload budget (see §5.3).
- **Lighting→mesh churn.** There is no "lighting-ready" gate. A section is
  meshed as soon as `chunkAvailable`/`setBlocksDirty` fires, *before* the
  async lighting for that region has finished propagating (boxes spill and
  drain across frames, `LightingEngine.cpp:392-439`). Result: the freshly
  meshed section is lit with stale light, then the drained region re-invalidates
  it and it meshes again. During world load this doubles meshing work.
- **Priority is not cross-pool.** Mesh enqueue priority (`ringOf`) and loader
  enqueue priority (`ChunkCache.cpp:498`) are independent; a near mesh job
  waits on a loader pool that may be saturated by far columns. `prefetchChunksNear`
  biases ring ≤ 1 first (`:498`), which partially compensates.

---

## 7. Vanilla Beta 1.7.3 comparison

The Beta 1.7.3 client renderer (`RenderGlobal`) is **not** in this repo
(`third_party/mcp/net/minecraft/src/` holds only server-side + shared classes;
confirmed by listing — no `RenderGlobal.java`, no `WorldRenderer.java`), so the
comparison below is from the MCP sources present plus known vanilla behavior:

- **Meshing: single-threaded on the client thread.** Vanilla had no chunk-mesh
  worker pool; `RenderGlobal.updateAllRenderers()` (`IWorldAccess.java:28`)
  re-meshed renderers inline on the render thread, sliced per frame by renderer
  count, not by time budget.
- **Lighting: single-threaded on the server/world thread** (`EnumSkyBlock`
  propagation ran synchronously inside the world tick / chunk load); clients
  received already-lit chunks. There was no async lighting box engine.
- **Chunk IO: synchronized on the caller.** `RegionFileCache.java:22,48` and
  `RegionFile.java:90-187` are `synchronized`; saves ran on the server thread,
  no async save pool.
- **Network: reader/writer threads per connection** (`NetworkManager.java:47-50`).
- **Implication:** this port is deliberately far ahead of vanilla (async
  meshing, async lighting, async load/save). The risk is not "too little
  threading" but "threads without a budget" — the opposite failure mode of
  vanilla's single-threaded client.

---

## 8. Recommended structure (coordinated scheduler)

- **One shared worker budget, sized once from `hardware_concurrency()`, not
  per-pool.** Replace every `recommendedThreadCount(3, 2, N)` call with a single
  `Scheduler` that owns a fixed worker count and partitions it among *classes*
  of work: mesh CPU, lighting CPU, IO (load/save), shader compile. This is the
  only fix for the §3 oversubscription (which pools live on is far less
  important than *how many workers exist*).
- **Make the worker pool elastic per class, not per subsystem.** Prefer one
  shared `WorkerPool` with tagged job classes (mesh / lighting / io / save) over
  five separate pools; let each worker pull from a priority queue where the
  priority is a *camera-distance × recency* metric, and let blocking IO jobs
  yield the worker (so IO doesn't eat a mesh slot while the disk blocks).
  Shader compile can stay a separate pool (it needs its own GL context) but must
  be **counted in the same budget**.
- **Close the R1/R4 race at the source.** Options, in decreasing preference:
  (a) make `Chunk::setLight`/light-array writes go through a per-chunk
  spinlock/`std::atomic_flag` and have `copyChunkBand` take the same lock —
  the copy is ~11 KB/section and the lock is held for microseconds; (b) version
  the light arrays (a `std::atomic<uint32_t>` bumped by writers) and have the
  snapshot copy re-run on a version change; (c) at minimum, move lighting's
  writes into a staging array and apply to the chunk under the pin protocol.
  Block writes (main-thread `setBlock`) are also writers; a single per-chunk
  rwlock used by `setBlock`, `decorate`, `populateBlockLight`, and
  `copyChunkBand` closes both.
- **Implement nearLane upload priority.** `processUpload`
  (`WorldRenderer.cpp:743-766`) should check `job->nearLane` and upload those
  first, exempt from (or given a dedicated slice of) the frame budget; the
  current behavior contradicts `ChunkMeshJob.hpp:51-54`.
- **Gate first mesh on lighting readiness.** Track per-column lighting state in
  the `LightingEngine` outbox (or have `drainDirtyRegions`/`chunkAvailable`
  mark a column "lit" only after its propagation boxes drain) and hold
  `enqueueDirtyChunk` for first-build until lit. Eliminates the §6 double-mesh
  churn on world load and teleport.
- **Move VBO growth off the hot path.** Give each layer a generously reserved
  ring buffer (per-column regions, not one giant shared VBO) or grow via
  `glBufferData`+`glBufferSubData` **on a background GL thread with a
  synchronized (fence-protected) buffer**, so `reallocBuffer`'s full-mirror
  re-upload (`ChunkRegionBuffer.cpp:56-61`) never lands on the main thread.
  If that's out of scope, at least reserve the worst-case section count
  (`WorldRenderer.cpp:546-553`) rather than the 2M-vertex cap.
- **Make cancellation non-blocking.** `clearSections` should drop queued jobs
  and mark builders retired, and let `sweepRetiring` (`WorldRenderer.cpp:291-300`)
  reap in-flight jobs when they complete, instead of `pool_.drain()`
  (`WorkerHandoff.hpp:35-40`) blocking the main thread inside
  `WorldRenderer.cpp:369`.
- **Replace the `retireFromLighting` spin with an event.** Signal a CV when a
  pin is released (`ChunkCache.cpp:61-68`) so unload doesn't busy-wait at 5 kHz.
- **Make the `builder->meshJobInFlight` write in `~ChunkMeshJob` explicit**
  (atomic, or assert main thread) so the R3 invariant is enforced, not assumed.
- **Consider a one-frame latency budget for uploads** — time-slice uploads by
  *bytes* (vertex count × 60 B, `Tessellator.hpp:23`) rather than only job
  count, since `bufferSubData` cost scales with geometry size.
- **Keep the good parts:** pin-protected capture, per-builder versioning, ring
  priority, snapshot detachment, main-thread-only GL upload, per-thread
  generator clones, lighting box sharding, and the existing `FrameBudget` for
  main-thread work — these are the right shapes; they just need a common budget
  and a few synchronization holes closed.

---

## Shared-file edits

None. Review-only.
