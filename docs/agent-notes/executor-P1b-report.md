# EXECUTOR P1b — WI-4 (mesh scheduler onto shared Compute + Channel) — report

Status: COMPLETE. This report was written post-hoc after the executor was cancelled by a
power outage; the code changes were verified present and coherent in the working tree.

## What changed (verified 2026-08-01)

- `util/concurrent/WorkerHandoff.hpp` — rewritten:
  - Constructor now takes `WorkerPool& pool` (caller owns the pool) instead of a thread count.
  - Completed jobs cross back on a bounded `Channel<std::shared_ptr<Job>>` so the main thread is
    the only thread that drops the last `shared_ptr` (R3 invariant).
  - `cancelAll()` is non-blocking: it bumps an epoch token (`epoch_.fetch_add`) and drains the
    completed channel. It no longer calls `pool_.drain()`. Queued/in-flight tasks whose epoch is
    stale skip `work(*job)` and still hand their job back through the channel for a main-thread drop.
  - `pendingJobs()` returns the in-flight atomic count; `idle()` checks both in-flight and channel size.
- `client/render/chunk/ChunkBuilder.hpp` — `ChunkMeshScheduler` backs `handoff_` with
  `ThreadCoordinator::instance().pool(Domain::Compute)` (the shared Compute pool) + the Channel-based
  `WorkerHandoff<ChunkMeshJob>`. Public API shape preserved (`enqueue/enqueueNear/drainCompleted/
  cancelAll/idle/pendingJobs/workerCount`). `enqueueNear` sets `job->nearLane = true` and uses the
  min-priority lane.
- `client/render/world/WorldRenderer.cpp`:
  - `compileChunks` (~:733-837): near-camera uploads get a dedicated 2 ms `nearBudget` slice
    (QD-21/HZ-31); `processUpload` honors `job->nearLane` first (near lane uploads before the ring
    backlog), then the ring backlog against the normal budget; both share the version-stamp
    stale-drop (retired / failed / version-mismatch handling unchanged).
  - `clearSections` (~:372) now calls the non-blocking `cancelAll()`.
  - `pendingJobs()` used for backlog/in-flight accounting (lines ~740, :754, :813).
  - `sweepRetiring` (reaps in-flight jobs on completion) intact.
- `tests/mesh_cancel_test.cpp` — created: `MeshCancel.CancelAllIsNonBlockingAndClearsOwnerFlags`
  (1000 jobs, cancelAll < 50 ms, every owner flag cleared) and
  `MeshCancel.CompletedJobsDrainInSubmissionOrder` (single worker FIFO order). Pre-registered in
  CMakeLists by WI-T (L1).

## Parity
Upload stays main-thread GL; pins unchanged; version-stamp stale-drop unchanged
(`WorldRenderer.cpp:760-765`). Near-vs-ring capture loops not reordered.

## Note
The R3 destructor assert on `meshJobInFlight` lives in `ChunkBuilder.cpp` and is owned by P1a
(WI-5); the epoch token here makes the shared-pool case safe so a worker can never drop the last
`shared_ptr`. `cancelAll`/drop remain main-thread-only.
