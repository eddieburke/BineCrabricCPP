# EXECUTOR P1a REPORT — WI-3 / WI-5 (light/block snapshot race + GL-state confinement)

Lane: P1a. Items: WI-3 (per-chunk light/block write guard) and WI-5 (alphaTestRef +
GLCore init confinement). No builds/tests run (compile-fixer only). All edits were
grep-verified against the working tree before editing; the earlier power-outage attempt
left no partial edits (all cited regions were pristine).

---

## WI-3 — Close the block/light-array snapshot race (R1/R4)

### `src/net/minecraft/world/chunk/Chunk.hpp`
- `:21-25` — new `net::minecraft::detail::tl_renderWriteLockDepth` (`inline thread_local int`),
  backs the Debug lock-order asserts.
- `:138-147` — `Chunk::setLight` now wraps the nibble write in `lockRenderWrite()`/`unlockRenderWrite()`
  (whole-function scope; no re-entrant callbacks, per plan-corrections).
- `:212-251` — Debug asserts added to `tryAcquireRenderPin` (pin must be acquired with no chunk
  write lock held) and `beginRenderEviction` (write lock never held across eviction); public
  `lockRenderWrite() const noexcept` / `unlockRenderWrite() const noexcept` declared with the
  WI-3 lock-order documentation (pin → lock; never across `beginRenderEviction`; never while
  taking `LightingEngine::outboxMutex_`/`registryMutex_`).
- `:435` — `mutable std::atomic_flag renderWriteLock_{};` member (C++20 default ctor = clear).
- `:457-472` — RAII `ChunkRenderWriteScope` (releases on scope exit, exception-safe).

### `src/net/minecraft/world/chunk/Chunk.cpp`
- `:16-26` — lock/unlock implementation. The lock is a cross-thread spinlock but **same-thread
  reentrant** (depth-tracked): `ChunkCache::decorate` must hold it across `generator_->decorate`,
  whose feature decorators re-enter `Chunk::setBlock`; a strict non-recursive spinlock would
  self-deadlock there (the exact hazard in plan-corrections row 2). **Deviation, documented.**
  The plan-mandated scoping in `setBlock`/`setBlockMeta` (raw writes only, released before
  callbacks) is preserved.
- `:36-47` (`setBlock`, metadata overload) and `:84-95` (`setBlock`, plain overload) —
  `blocks[]` write guarded, released; `onBreak` runs **outside** the guard; `meta` write guarded,
  released; then heightmap/`queueLightUpdate`/`lightGaps`/`onPlaced`/`setBlockDirty` all run
  outside the guard. Guard acquired twice (one per raw write) rather than reordering `meta.set`
  before `onBreak`, so callback behavior is byte-for-byte unchanged.
- `:126-128` — `setBlockMeta` guards the `meta` write only.

### `src/net/minecraft/client/render/chunk/RegionSnapshot.cpp`
- `:42` — `copyChunkBand` takes `ChunkRenderWriteScope` for the whole copy (~11 KB, held µs).

### `src/net/minecraft/world/chunk/ChunkCache.cpp`
- `:174-180` — `adoptChunk` guards the `chunk->populateBlockLight()` call (R4); `load()` is left
  unguarded because it reaches `registerChunkForLighting` → `registryMutex_` (forbidden while
  holding the chunk guard).
- `:377` — `saveChunk` guards `takeSnapshot`/`storage_->saveChunk` (R4 snapshot read).
- `:397` — `decorate` guards `generator_->decorate(...)` (reentrant-safe for the inner `setBlock`).

### `src/net/minecraft/world/light/LightingEngine.cpp`
- **Not edited.** `setBrightness` (`:249-258`) writes through `Chunk::setLight`, which is now
  guarded whole-function in Chunk.hpp — the WI-3 requirement is satisfied there. `getLight` is a
  byte-atomic relaxed read (ChunkNibbleArray) and needs no guard.

---

## WI-5 — Confine GL state writes to the main GL thread

### `src/net/minecraft/mod/model/ModModels.cpp`
- Removed `core::setAlphaTestRef(0.1f)` from `drawLuaBlockWorld` (`:626`). Grep confirms **zero**
  `setAlphaTestRef` calls remain in ModModels.cpp.

### `src/net/minecraft/client/render/chunk/ChunkMeshJob.hpp`
- `:56-59` — added `float alphaTestRef = 0.1f;` (default preserved per QD-20).

### `src/net/minecraft/client/render/chunk/ChunkBuilder.cpp`
- `:8,:15` — added includes `RenderCore.hpp`, `ThreadNames.hpp`.
- `:198` — `ChunkMeshJob::capture` snapshots `core::alphaTestRef()` at enqueue time (main thread;
  the existing lightLevelToLuminance/blockRenderLayers snapshot site).
- `:223-234` — `~ChunkMeshJob` gains the **R3 main-thread-destruction assert**
  (`assertOnMainThread()`, NDEBUG-gated) on the `meshJobInFlight` write, owned here for
  file-disjointness with WI-4.
- Note: the captured field needs no draw-path consumer this pass — the main-thread mod-mesh draw
  path (`WorldRenderer::renderModChunkMeshes` → `ModChunkMeshScope` → `RenderType::solid()/
  translucent()`, both alphaRef=0.1) already applies the ref. WorldRenderer.cpp (P1b/P2a-owned)
  untouched.

### `src/net/minecraft/client/render/RenderCore.cpp` / `.hpp`
- `.cpp` — added `ThreadNames.hpp` include; `setAlphaTestRef` (`:672-677`) now calls
  `util::concurrent::assertOnMainThread()` under `#ifndef NDEBUG`. `:479-481` (uniform upload) and
  `:70` (declaration) are reads/no-change.
- `.hpp` — `setAlphaTestRef` doc comment updated to state main-GL-thread write only.

### `src/net/minecraft/client/gl/GLCore.cpp`
- `:124` — `static bool g_loaded` → `static std::once_flag g_initOnce;` (HZ-15).
- `:144-152` — `GLCore::init()` early-returns only when `wglGetCurrentContext() == nullptr`, then
  runs the whole load body inside `std::call_once`. The call_once body calls
  `setMainThread()` (see deviation below). Driver-hint block (L1's `:288-290`) unchanged, just
  re-wrapped inside the lambda.

### New `tests/region_snapshot_race_test.cpp`
- Self-contained, GL-free, GTest: (1) a lighting-style writer hammers one `blockLight` nibble via
  `Chunk::setLight` while a reader constructs `RegionSnapshot` 1000×, asserting the copied nibble
  is always exactly one of the two written values; (2) a writer updates two correlated nibbles
  inside one `lockRenderWrite` section while the reader snapshots 1000×, asserting the pair is
  never split by `copyChunkBand` (proves the lock serializes the whole copy).

---

## Deviations / notes for the compile-fixer
1. **Guard is same-thread-reentrant, not strictly non-recursive** — required by `ChunkCache::decorate`
   holding it across `generator_->decorate` (features re-enter `Chunk::setBlock`). Plan-mandated
   scoping (raw writes only, callbacks outside) is preserved in `setBlock`/`setBlockMeta`.
2. `assertOnMainThread()` depends on the main-thread marker, which **nothing calls
   `setMainThread()` for yet** (L1 left it unwired; PASS-2 WI-13 wires it). To keep the Debug
   assert functional (and Debug builds abort-free), `GLCore::init()` now latches the marker in its
   `call_once` body (first call is the main GL thread at display setup). In Release (`NDEBUG`,
   the default build) the asserts compile out.
3. `LightingEngine.cpp`, `RenderCore.cpp:479-481`, `RenderCore.cpp:70` were **not** edited (no
   change needed); only the WI-3/WI-5 cited regions were touched.
4. New test file is pre-registered in CMakeLists only as a **comment block** (per L1's WI-T
   design); the compile-fixer must add `tests/region_snapshot_race_test.cpp` to
   `MINECRAFT_TEST_SOURCES`.

## Line-drift corrections (vs plan-master §4.1 / plan-corrections)
- `ChunkCache.cpp` adopt was `:157-201` → actual `:158-203`; save was `:363-379` → actual
  `saveChunk` `:371-388`; `takeSnapshot` call is `:380` (plan said `:371`).
- `RegionSnapshot.cpp copyChunkBand` `:32-67` → actual `:32-68`; guard at `:42`.
- `ChunkBuilder.cpp` capture snapshot site `:170-196` → actual `ChunkMeshJob::capture` `:132-200`,
  `blockRenderLayers` snapshot at `:191-196`; `~ChunkMeshJob` `:220-225` → actual `:223-234`.
- `GLCore.cpp` `g_loaded` `:122` (plan `:121`) → now `g_initOnce` `:124`; init head `:142-146` →
  actual `:144-152`.
- `ModModels.cpp` `setAlphaTestRef` was at `:626`; removed (site now `:625-627`).
- `Chunk.hpp` `setLight` `:133-138` → actual `:138-147` after the detail-namespace insert.
