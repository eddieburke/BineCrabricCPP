# Lag Diagnosis & Fix Plan — chunk/network pipeline

Status: diagnosis complete. All four fixes implemented and compiled (incremental ninja
objects verified). A full 1.3h Debug rebuild is still needed to run/triple-check.

## TL;DR

The lag is **not** from the `ThreadNames.hpp` / dead-code / NSDMI cleanup (all confirmed
once-at-spawn or inert). It comes from the chunk/network pipeline refactor (`815587ef`,
`3593a2a0`) that the cleanup was committed alongside. Four issues, in priority order:

| # | Issue | Severity | Status |
|---|-------|----------|--------|
| 1 | `Connection::tick()` unbounded drain (no wall-clock budget) | HIGH | **DONE** |
| 2 | Shared `Domain::Compute` pool starvation (gen/light/mesh contend) | HIGH | **DONE** |
| 3 | Lighting gate removed → streamed chunks mesh twice | HIGH | **DONE** |
| 4 | `ChunkCache::integrateFinishedLoads` O(n²) candidate scan | MOD | **DONE** |

---

## Fix 1 (DONE): Connection drain wall-clock budget

- **File:** `src/net/minecraft/network/Connection.cpp`
- **Where:** `tick()`, drain block around lines 215-243.
- **Problem:** When `externalDrainLimit_` is unset, `drainDeadline` was `std::nullopt`, so
  the deadline break (line 240) never fired. A single `tick()` could `apply()` up to
  `kMaxDrain` (4096) packets synchronously on the main/logic thread, with
  `kMaxReadQueueBytes = 0x2000000` (32 MB) buffered. Join/LAN bursts → multi-frame stalls.
- **Change made:**
  - Removed the `nullopt` path; a fallback 3 ms budget is always applied:
    `drainDeadline = externalDrainLimit_.has_value() ? externalDrainLimit_->deadline
    : steady_clock::now() + kFallbackDrainBudget`.
  - `drainDeadline` is now a plain `time_point`; the loop break uses
    `now() >= drainDeadline` directly.
- **Verify:** object compiles (`Connection.cpp.obj`). Restores the pre-`815587ef`
  invariant (deadline always present).

---

## Fix 2 (DONE): Mesh lane starvation on shared Compute pool

- **Implemented:** **2a — priority bias**, applied in
  `ChunkCompilePipeline.cpp` (`kMeshBias = 4`, used as `priority - kMeshBias` in the
  distant-mesh `enqueue` call). A mesh at ring `r` now submits as `r - 4`, strictly
  ahead of the ring-`r` chunk-load job (`ChunkCache.cpp:615` uses `INT_MIN` for near
  loads, `ring` for distant). Near loads (INT_MIN) still win; near-lane meshes
  (`enqueueNear`) untouched. No worker-pool / thread-count changes; `WorkerPool` priority
  semantics already supported this.
- **Verify:** `ChunkCompilePipeline.cpp.obj` compiles. Watch for off-by-one ring-order
  flips; fall back to 2b (dedicated mesh pool in `ThreadCoordinator`) if scheduling feels
  wrong under load.

---

## Fix 3 (DONE): Restore lighting gate for first mesh (lean re-add)

- **Implemented (leaner than the plan's flag/placeholder sketch):** restored the old
  event chain + an inline gate set — no new class, no per-section flag:
  - `GameEventListener` + `WorldEvents` + `World` re-gain `markChunkColumnLit` /
    `markAllChunksLit` (previously deleted in `3593a2a0`).
  - `World::doLightingUpdates` re-marks the drained region's columns lit and calls
    `markAllChunksLit()` when the engine goes fully idle (non-optional completion so a
    column with no drained region is never held forever).
  - `ChunkSectionSystem` owns a `pendingLit_` set (`SectionPos`, y==0) + `columnPendingLit()`.
  - `ChunkCompilePipeline::enqueueDirtyChunk` holds only the FIRST mesh
    (`!chunk->built`) of a gated column; `markChunkColumnLit`/`markAllChunksLit`
    re-enqueue the held unbuilt sections.
- **Verify:** six objects compile (`World.cpp`, `WorldEvents.cpp`, `ChunkSectionSystem.cpp`,
  `WorldRenderer.cpp`, `ChunkCompilePipeline.cpp`). Visual: streaming join has no
  per-frame 16-upload stall and `dirtyChunks_` stays low after a burst. Watch for the
  1b2439f7 "perf death spiral" regression — the idle-release path prevents held-forever.

---

## Fix 4 (DONE): O(n²) candidate scan in integrateFinishedLoads

- **Implemented:** `ChunkCache::integrateFinishedLoads` now does a single O(n) sweep:
  erase done-but-cancelled and out-of-radius entries, collect the ready in-radius
  candidates into a small vector, `std::stable_sort` by Chebyshev distance, then adopt up
  to `budget` walking the sorted list (honoring the wall-clock budget between adopts).
  One scan per call instead of one scan per adopted chunk.
- **Semantics preserved:** min-distance-first adoption; dead/out-of-radius entries still
  dropped; `adoptChunk` re-check (`chunk == nullptr`, already-loaded) kept.
- **Verify:** `ChunkCache.cpp.obj` compiles; chunk-stream behavior unchanged, main-thread
  publish cost is now O(n) per frame instead of O(n·budget).

---

## Execution order & verification

All four fixes are implemented and compile (incremental objects). A full Debug rebuild
(1.3h+) is still needed to run and visually confirm. Suggested order to sanity-check:
Fix 1 (network burst), Fix 2 (chunk holes under fast travel), Fix 4 (stream hitch), Fix 3
(streaming double-mesh / backlog). Run the existing test suite via the build menu (option
7) when a full build is done.
