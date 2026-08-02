# EXECUTOR L1 REPORT — WI-1 / WI-2 / WI-T (foundational lane)

Lane: L1 (foundational). Items: WI-1, WI-2, WI-T. No builds/tests run (compile-fixer only).
All edits were grep-verified against the working tree (2026-08-01) before editing.

## WI-1 — ThreadCoordinator / ThreadBudget / Channel / ThreadNames / Lifecycle (new infra)

### New files created
- `src/net/minecraft/util/concurrent/ThreadBudget.hpp` — pure-header budget struct. `derive(hw, reserved=2, maxComputeThreads=8)`;
  fields `cpuBudget/glCompile/io/compute/maxComputeThreads`; `computeShare(owners)` = `max(1, compute/owners)`;
  `glDriverThreads()` = `max(1, cpuBudget/4)` (audit-A FINDING 12). Worked-table comment (8/16/32-core) in header.
- `src/net/minecraft/util/concurrent/ThreadCoordinator.hpp` + `.cpp` — singleton (`instance()`), `configure` (idempotent-once,
  first call wins), `pool(Domain)` (Compute/Io/GlCompile lazily create real WorkerPools; NetIo/Audio/Log share a 1-thread
  placeholder pool + `reserveDynamic` bookkeeping, Q7), `budget()`, `reserveDynamic/releaseDynamic`, `totalPending()`,
  `shutdown()`, `computeShare(owners)`. `enum class Domain { Compute, Io, GlCompile, NetIo, Audio, Log }`;
  `enum class TaskPriority { Urgent=0, High=1, Normal=2, Low=3, Idle=4 }`. Ctor pre-computes the hw-based fallback budget,
  so count sites used before `configure()` runs still get a sane value.
- `src/net/minecraft/util/concurrent/Channel.hpp` — `Channel<T>` (mutex+CV+deque, NOT lock-free): bounded+prioritized
  (Urgent-first, ties by sequence) + stop-aware (`std::stop_source`; `request_stop()`/`stop_requested()`/`get_stop_token()`;
  stop => push/pop return false) + version-stamped (`push(T,stamp)`/`latestStamp()`; `tryPop` drops entries with
  `stamp < latestStamp()`). API: `push`/`tryPush` (never blocks, false at capacity), `pop`/`tryPop`, `drain`, `reset`
  (clears + un-stops + wakes producers), `size`, `capacity`, `setCapacity`.
- `src/net/minecraft/util/concurrent/ThreadNames.hpp` — header-only: `setCurrentThreadName` (Windows `SetThreadDescription`
  loaded dynamically; empty-name + null-fn fallbacks; no-op elsewhere), `assertOnMainThread()` debug helper (NDEBUG-gated),
  `inline thread_local Domain tl_domain`, `tl_is_main_thread` + `setMainThread()`.
- `src/net/minecraft/util/concurrent/Lifecycle.hpp` + `.cpp` — owner registry (`registerOwner(name, Owner{unblock, stop,
  join, deadline=3s})`, `instance()`, also default-constructible for tests, `ownerCount`). `shutdown()` runs unblock-all →
  request_stop-all → watchdogged join each (2-5 s then fprintf+detach log-and-leak). **Never joins the log thread.**
  Fixed a lifetime bug during review: the watchdog `packaged_task` now copies the join callable so a leaked join never
  touches the about-to-destroy owners vector.

### Wiring
- `src/net/minecraft/client/Minecraft.cpp:75` (include), `:261-264` — `init()` now calls
  `ThreadCoordinator::instance().configure(std::thread::hardware_concurrency(), 2, {.maxComputeThreads = 8})` after setup.
  Nothing else in the file touched (L2/PASS-2 lanes own the rest).

## WI-2 — Delete `recommendedThreadCount`; route all counts through the coordinator

Grep before/after: **zero `recommendedThreadCount` hits remain in `src/`** (docs under `docs/agent-notes/` are historical
records and were not edited). `hardware_concurrency()` now appears only in ThreadCoordinator.cpp (the one authority) and
the Minecraft.cpp configure call.

- `util/concurrent/WorkerPool.hpp` — deleted `recommendedThreadCount` (was :61-68); file now :58 `threadCount()` only.
- `util/concurrent/WorkerHandoff.hpp:14` — removed default arg → `explicit WorkerHandoff(unsigned threadCount) : pool_(threadCount) {}`.
  Only construction site in tree: ChunkBuilder.hpp (fixed below).
- `client/render/chunk/ChunkBuilder.hpp:12` (include), `:156-160` — `handoff_` now
  `ThreadCoordinator::instance().computeShare(3)` (mesh owner takes max(1, compute/3); comment in code re WI-4/WI-6/7 consolidation).
- `world/light/LightingEngine.cpp:6` (include), `:47-49` — worker count = `ThreadCoordinator::instance().computeShare(3)`.
- `world/chunk/ChunkCache.cpp:7` (include), `:218-222` — loader pool = `computeShare(3)`; `:228-230` — save pool stays `1U`
  (single writer; Domain::Io concept documented in comment).
- `client/gl/ShaderCompileService.cpp:6` (include), `:47` — `count = ThreadCoordinator::instance().budget().glCompile`
  (already <= 2; dropped the old `min(count, 4)`).
- `client/gl/GLCore.cpp:8` (include), `:289` — driver hint = `budget().glDriverThreads()`.

## WI-T — ctest wiring + PASS-1 test pre-registration

- `CMakeLists.txt` (tests region, was :339-381) — added to `MINECRAFT_TEST_SOURCES`:
  the 7 orphan tests (`block_face_uv_test`, `color_targets_test`, `custom_uniforms_test`, `handshake_metadata_test`,
  `iris_hemisphere_chunk_offset_test`, `pack_blend_drawbuffer_test`, `shadow_celestial_modelview_test`) — all verified to
  exist on disk and be GL-free — plus my 4 tests (`channel_test`, `thread_budget_test`, `lifecycle_test`,
  `thread_coordinator_test`).
- The 8 other PASS-1 tests that do NOT exist on disk yet (`region_snapshot_race_test`, `mesh_cancel_test`,
  `packet_accounting_test`, `connection_async_teardown_test`, `fog_mode_parity_test`, `lighting_ready_gate_test`,
  `if_engine_unified_test`, `macro_parity_test`) are pre-registered as a **comment block** in CMakeLists (listing a
  non-existent source breaks `configure`); the compile-fixer adds each to `MINECRAFT_TEST_SOURCES` once its file lands.

### Tests created (self-contained, no GL)
- `tests/channel_test.cpp` — tryPush-at-capacity returns false; blocking push waits for space; Urgent-first pop order;
  reset wakes blocked producers; stop => push/pop false; version-stamp stale-drop.
- `tests/thread_budget_test.cpp` — 8/16/32-core worked table; `glDriverThreads`; `computeShare`; low-core (hw=2) never-zero.
- `tests/lifecycle_test.cpp` — unblock→request_stop→join order (2 owners); watchdog leak path (blocking join + 50 ms deadline
  => shutdown returns < 2 s); `ownerCount`.
- `tests/thread_coordinator_test.cpp` — 8-core budget derivation; configure idempotency; pool counts (3/2/1); computeShare; reserveDynamic bookkeeping.

## Line-drift / corrections vs plan (all pre-verified against the tree)
- Plan's `io = clamp(2, cpu/4, 3)` literal argument order is misleading; the worked table confirms intent = `clamp(cpu/4, 2, 3)`.
  Implemented as `std::clamp(cpuBudget/4, 2, 3)` — table rows 8/16/32 → io 2/3/3 as specified.
- `compute` subtraction uses **signed** intermediate math (`spare = cpu - glCompile - io` in `int`) to avoid unsigned
  underflow on low-core systems (hw=2 would otherwise compute=8 instead of 1); covered by `LowCoreCountsNeverZero`.
- `ChunkCache.cpp:225` save pool keeps `1U` (Domain::Io *concept*) rather than `pool(Io).threadCount()` (=2 on 8-core):
  the plan's own PASS-2 WI-7 pins the save pool at 1, and chunk writes are serialized by `ioMutex_`. Comment documents this.
- ChunkBuilder/LightingEngine/ChunkCache keep **private** sub-pools sized `computeShare(3)` per the WI-2 "pool-identity" note
  (consolidation is WI-4/WI-6/7). The shared `pool(Domain::Compute)` is created lazily but unused by these owners this pass.
- `register` (C++ keyword) implemented as `registerOwner`.
- ThreadCoordinator `configure` runs at the *end* of `Minecraft::init()` (after `bootstrapAfterDisplay()`). Numerically
  identical to pre-start because the singleton ctor already derives the same hw-based budget as the default configure args.
- `Lifecycle` ctor made public so tests can build fresh instances (singleton `instance()` kept for the app path).

## Files touched (ownership respected)
- New: util/concurrent/{ThreadBudget,ThreadCoordinator,ThreadCoordinator.cpp,Channel,ThreadNames,Lifecycle,Lifecycle.cpp}, tests/{channel,thread_budget,lifecycle,thread_coordinator}_test.cpp.
- Edited: util/concurrent/WorkerPool.hpp, util/concurrent/WorkerHandoff.hpp, client/render/chunk/ChunkBuilder.hpp,
  world/light/LightingEngine.cpp, world/chunk/ChunkCache.cpp, client/gl/ShaderCompileService.cpp, client/gl/GLCore.cpp,
  client/Minecraft.cpp (init only), CMakeLists.txt (tests region only).
- **Not touched:** `util/concurrent/FrameBudget.hpp` (L2), all other files.
