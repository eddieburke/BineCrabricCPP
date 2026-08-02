# MASTER REFACTOR PLAN — de-slop, collapse & simplify

> **Design principle (user):** timed budgets are OPT-IN. Nothing is time-boxed except
> a Java-MP/client connection or a hosted server. Singleplayer paths drain unbudgeted.
>
> **Approach:** four deep-trace subagent passes (frustum/camera, chunk pipeline,
> network I/O, chunk/world/budget infra). Every "delete/inline/merge" claim below was
> re-verified against source. Net intent: cut file count, kill dead members and
> duplicate logic, fold monoliths, and reduce the render/culling & network surfaces to a
> minimal, single-owner layout.

## STATUS — what was actually executed (2026-08-02)

**Wave 1 (done, compiles clean):**
- Deleted write-only `ChunkBuilder::id`, `::radius`, `WorldRenderer::nextSectionId_`, unused `<cmath>`.
- Deleted dead `ChunkRegionManager::regionFor` (0 call sites).
- Deleted empty `World::populateChunkCacheReadyChunks` stub + its call site.
- Unified `0x100000` send-cap magic literal → `kMaxSendQueueBytes`.
- Merged duplicate `configureAcceptedSocket`, `formatPeerAddress`, `ensureWinsock`
  (ServerSocket delegates to Connection).
- Folded `PacketRegistry.hpp` shell into anonymous `bootstrap()` in PacketRegistry.cpp; deleted header.

**Wave 2 (done, compiles clean):**
- `Connection::tick()` drain is now **unbudgeted by default** (drain to `kMaxDrain`), time-box
  applies ONLY via `externalDrainLimit_`. Removed hardcoded 3ms/kDrainBudget + dead
  `FrameBudget::frameDeadline()` branch (was dead on server anyway).
- Client Java-MP path (`ClientNetworkHandler::tick`) opts in via a 3ms/100-packet drain limit.
- Read-queue overflow → **backpressure, not disconnect**: cap raised to 32MB with
  high/low-water pause on `readCv_`; removed `readOverflow_` and the overflow disconnect.
  `requestDisconnect`/`tick` wake `readCv_`.

**Wave 3 (partial, compiles clean):**
- Fixed `Frustum.cpp` double `#include RenderCore.hpp`.
- Removed vestigial cull-state flag backup/restore in `push/popCullState`
  (`savedFrustumFlags_`, `savedFrustumSectionCount_`) — the main pass re-culls right after.
- **FrameBudget redesign (NEW, coherent single type):** collapsed the nested
  `Deadline` singleton into `FrameBudget` itself. One deadline + minItems floor +
  `hasRemaining(done)`/`expired()`/`active()`/`remaining()`. `beginFrame(ms)` arms the
  shared per-frame deadline; `fromSharedMs(ms, minItems)` yields to it (falling back to
  a fresh local slice when the frame already overran). `ChunkCache::pumpChunkPublish`
  now reads `frame.remaining()` instead of hand-rolling `frame.point() - now`. Removed
  stale "PASS-2 WI-6/7" ticket-ID comment in ChunkBuilder.hpp.
- **Disproven / kept:** `clean*` fields (real camera-bob compensation from iris matrix),
  `NetworkHandler::handle` (overridden by ServerLoginNetworkHandler + used by test),
  per-tick `interrupt()` (real latency win, not inert), `disconnect(reason,args)` pre-set
  (correct guard for already-closed case), `GuiProjection` (thin well-tested wrapper),
  `FrustumCuller` inline (no test coverage on culling path), inverse matrix builders
  (feed shader uniforms, precision-sensitive), `sectionList_` removal (draw-ordering),
  `computeShare` (used by LightingEngine/ChunkCache, tested).

**Deferred to Wave 4 (needs visual/game verification):** WorldRenderer split into
facade + `ChunkSectionSystem` + `ChunkCompilePipeline`; `ChunkRegionBuffer` merge into
`ChunkBuilder`; `sectionList_`/rings/pending-pair container merges; `lightingGate_` +
`markChunkColumnLit`/`markAllChunksLit` cut; `ChunkSource` noop-virtual trim;
FrameBudget/Deadline collapse.

---


## A. FRUSTUM / CAMERA — collapse to ONE source of truth

Today three objects describe the same camera every frame:
`GameRenderer::frameCamera_`, `RenderCameraState::instance().frame()`, and `RenderCore`
`g_draw*` globals. Plus a `clean*` field mirror.

### A1. Delete the `clean*` mirror set (`FrameRenderCamera.hpp:27-127`)
15 fields duplicate `view*`/`x,y,z` (`cleanEye*`, `cleanView{Right,Up,Forward}*`) +
`directionToViewClean` twin. Rename consumers (GameRenderer culler `prepare`, FrameData)
to the primary fields. **Delete the mirror, keep one.**

### A2. Inline `FrustumCuller` away (`Frustum.hpp`)
7-line wrapper (`prepare`/`isVisible`). Fold `isVisible(Box)` = offset + `intersects` into
`ChunkBuilder::updateFrustum`, hold `const Frustum&` on the builder instead of a
`FrustumCuller*` forwarded across `WorldRenderer::cullChunks`/`renderEntities`. Removes
a fwd-decl and cross-boundary pointer.

### A3. Kill duplicate matrix builders
- `buildCameraProjectionInverse` / `buildCameraProjection` (`FrameRenderCamera.hpp`) —
  one builds through the other; use `Matrix4f::invert()` once (RenderCore already inverts).
- `Frustum::compute` hand-rolled SSE/non-SSE matrix multiply (`Frustum.cpp`) duplicates
  `Matrix4f::multiply`; replace with a single `Matrix4f::multiply` fill.
- `buildCameraProjection` vs `Matrix4f::perspective` — one formula, pick the keeper.
- **Fix slop:** `Frustum.cpp` includes `RenderCore.hpp` twice (lines 9-10).

### A4. Inline `GuiProjection` (`gui_proj::load/begin`) into `RenderCore`
Two thin functions over `setDrawCameraState`. Reduces a header dep from `GameRenderer.hpp`.

### A5. Unify the three producers of the draw-camera globals
`setDrawCameraStateFromCamera`, `gui_proj::load`, and the GameRenderer hand path → one
entry + `ScopedDrawCameraState` restore.

### A6. Unify the three "48"/near-bypass distance constants
`FrameRenderCamera::frustumBypassDistance=48`, `cullChunks` (48), `kNearOcclusionBypassSq`
(48²) — one named constant.

### A7. Decide camera source of truth
Drop `RenderCameraState` duplicate publication (or make `frame()` a tiny accessor to
GameRenderer's single `frameCamera_`). Remove `prevPublishedCamera` juggling.

> **Target files (frustum/camera):** `Frustum{.hpp,.cpp}` stays (used) but slimmed;
> `FrameRenderCamera.hpp` stays (14 includers) but de-duplicated; `RenderCameraState`
> becomes a thin accessor. **Delist:** `FrustumCuller`, `GuiInvalidProjection.hpp`,
> `clean*` field set, 4 inverse builders.

---

## B. CHUNK PIPELINE — split the god-file, drop dead state

`WorldRenderer.cpp` (1413 lines) is ONE class mixing: RAII scopes, column lifecycle,
frontier/rings, occlusion BFS, frustum, mesh upload, entities, block-entities, overlays,
event dispatch. 14 containers; 6 are collapsible.

### B1. Split `WorldRenderer` into 3 (proposed NEW layout)
- `WorldRenderer` (facade) — the `GameRenderer` contract
  (`render*`, `compileChunks`, `cullChunks`, `renderEntities`, `getChunkDebugInfo`,
  event forwarding) + overlays/particles. ~350 lines.
- `ChunkSectionSystem` (NEW) — sections_, sectionList_, rings/_visibleRings_,
  create/remove/enqueue column, frontier, `applyOcclusionCulling`, `cullChunks` loop,
  `push/popCullState`. ~600 lines.
- `ChunkCompilePipeline` (NEW) — dirtyChunks_/nearDirty_, mesh queue, upload budgets,
  `swRetire`, borderRefresh, lightingGate_. ~280 lines.

### B2. Delete dead members
- `ChunkBuilder::id` (write-only), `::radius` (write-only), `WorldRenderer::nextSectionId_`
  (feeds dead `id`). **Verified dead** (only writes exist).
- `ChunkRegionManager::regionFor` — zero call sites (both subagents). **Verified.**
- `World::populateChunkCacheReadyChunks()` — **empty stub** (`WorldChunks.cpp:353`),
  called from `WorldSession::prepareWorld`. **Verified** — delete fn + call.

### B3. Drop the vestigial cull-state scratch
`pushCullState`/`popCullState` swap `visibleDrawRings_` to `savedVisibleDrawRings_` and
backup `inFrustum` flags. The code's own comment says the next cullChunks rebuilds flags
from scratch. **Delete `savedVisibleDrawRings_`, `savedFrustumFlags_`,
`SavedFrustumSectionCount_`, `cullStateSaved_` and the positional restore loop** → push/pop
become 1-liners.

### B4. Merge the 6 redundant containers
- `sectionList_` (mirror of `sections_`) — iterate `sections_`; **delete**.
- `pendingColumns_` + `pendingSet_` — one ordered container (internal seen bit).
- `drawRings_` + `visibleDrawRings_` — one per-frame ring table (all-by-ring, then in-frustum
  filter at draw time).
- `(pendingColumns_/pendingSet_)` → single.

### B5. Merge `ChunkRegionBuffer.hpp/.cpp` into `ChunkBuilder`
`ChunkRegion`, `ChunkRegionManager`, `Slot`, `DrawRange` are used only by `ChunkBuilder` +
`WorldRenderer`. Fold in + **DELETE `ChunkRegionBuffer.{hpp,cpp}`**. Keep the single global
VBO + `buildMergedRanges` memo (real benefit).

### B6. Factor duplicated camera/eye lookup (4×) and 4 copy-paste RAII scopes
- One `frameEye()` accessor replacing 4 independent eye resolutions
  (`renderChunksVbo`, `renderModChunkMeshes`, `cullChunks`, `applyOcclusionCulling`).
- Consolidate `ModChunkMeshScope`, `WorldOverlayScope`, `WorldCrackOverlayScope`,
  `BlockOutlineScope` into one parameterized scope helper.

### B7. Replace the two-hash block-entity diff / O(n) erase
`ChunkBuilder::uploadMesh` diff builds two full `unordered_set`s; `removeColumn` erases via
`std::find`. Use one sorted/boosted pass.

> **Target layout (chunk):** from 7 units to 5+1(host). Delete
> `ChunkRegionBuffer.{hpp,cpp}`; add `ChunkSectionSystem.{hpp,cpp}` +
> `ChunkCompilePipeline.{hpp,cpp}`; `WorldRenderer.{hpp,cpp}` slims to facade.
> Keep `ChunkMeshJob`, `RegionSnapshot`, `TerrainLayers` unchanged.

---

## C. NETWORK I/O — de-dup, de-scaffold (no core rewrite)

Subagent confirmed the deques are **race-safe** (`joinThreads` before member destruction;
all `front()` guarded by `empty()` under the right mutex). The reported
`deque::back()` crash is **not reproducible in the current source** — treat as: (a) null-guard
`tick()`'s captured `NetworkHandler*`, (b) keep the "never touch a deque without empty()
under its mutex" discipline.

### C1 [P0] — budgets opt-in (the core principle)
- `Connection::tick()`: apply the `kMinDrain`/3ms time-box **only when
  `externalDrainLimit_` is set** (Java-MP join / hosted server). Singleplayer/integrated
  loopback drains up to `kMaxDrain` unbudgeted.
- **Merge the two budget systems** in `tick()`: `ConnectionListener` always sets
  `externalDrainLimit_`, so the `FrameBudget::frameDeadline()` branch (`:220-223`) is dead
  on the server. Keep ONE deadline path.
- Unify the duplicated cap literal `0x100000` with `kMaxReadQueueBytes`.

### C2 — delete duplicate socket plumbing
- `Connection::configureAcceptedSocket` vs `ServerSocket::configureAcceptedSocket`
  (identical, `Connection.cpp:20` / `ServerSocket.cpp:135`) — one shared util.
- `Connection::formatAddress` vs `ServerSocket`'s `formatPeerAddress` (same
  getpeername/getnameinfo) — one util.
- Duplicated `WSAStartup` guard (`Connection::ensureWinsock` vs `ServerSocket::ensureWinsock`).

### C3 — delete dead scaffolding
- `NetworkHandler::handle(const Packet&)` (`NetworkHandler.hpp:75`) — never dispatched
  (grep: no `.apply(`/`handle(` production caller). **Verified** as a virtual that only
  provides per-packet `apply`.
- `Connection::interrupt()` per-tick call sites (ClientNetworkHandler, ConnectionListener,
  both server handlers) — the writer already `wait_for(20ms)`; the per-tick
  `writeCv_.notify_all()` is inert. Drop call sites.
- `PacketRegistry.hpp` trivial `struct { static bootstrap(); }` shell → fold into
  `Packet.cpp`.

### C4 — keep the streambuf indirection (not the de-bloat target)
`SocketInput/OutputStreamBuf` are implementation detail bled into the public header.
Privatize them in the header; do NOT convert the codec (large/risky). Keep `PacketIO.hpp`
and `Channel.hpp` as-is.

> **Network target:** Connection.hpp trimmed (streambufs private, stats private, dead
> `handle`/`interrupt` gone), dup socket util merged, `PacketRegistry.hpp` deleted,
> budget made opt-in. ~120-150 net lines removed; no behavior change to the race-safe core.

---

## D. BUDGET & WORLD INFRA — keep the real time-slicing, cut the rest

### D1. MUST STAY (genuine frame-protection; "opt-in" but live)
- `compileChunks` upload/near sites, `ChunkCache::pumpChunkPublish` adoption budget,
  `Connection::drain` frame-clock min, `beginFrame(16)`, `ThreadBudget::derive`/
  `ThreadCoordinator` pool allocation. These protect the interactive frame.

### D2. CUT (pure overhead / placeholder)
- `lightingGate_` (`ColumnLightingGate`) + `markChunkColumnLit`/`markAllChunksLit` events —
  only delays the FIRST mesh of a new column until lighting drains. Acceptable to remove:
  delete both event fan-outs, `ColumnLightingGate`, and the `first-frame` branch of
  `enqueueDirtyChunk`. *(Candidate — confirm visual tolerance to a 1-frame unlit spike.)*
- Empty `ChunkSource` noop virtuals (~8) → keep only the ~6 used by the renderer.

- `FrameBudget` / nested `Deadline` / `fromSharedMs` defensive triple-layering — collapse
  to a single `beginFrame/remaining/expired` shared deadline.
- `ThreadBudget::computeShare` "interim sub-pool" placeholder — fold to shared compute pool.

### D3. Remove ticket-ID comment slop
`P-LITGATE`, `QD-07/21`, `HZ-31/39`, `R2/R3`, `WI-6/7/12` tags scattered through
`WorldRenderer`, `ChunkBuilder`, `FrameBudget` bodies. These are transplant-plan docs that
belong in git history, not code.

---

## E. Delivery order (low risk → high risk)

**Wave 1 — pure deletions (compile-safe, no behavior change):**
`ChunkBuilder::id/radius`+`nextSectionId_`, `ChunkRegionManager::regionFor`,
`World::populateChunkCacheReadyChunks`, `NetworkHandler::handle`, `connectedStreams`
`interrupt` call sites, `PacketRegistry.hpp` shell, dup `configureAcceptedSocket`/
`formatAddress`/`Winsock`, `Connection` dup literal, `savedFrustum*`+restore loop,
`populate stub`. Run full test suite + build.

**Wave 2 — budget opt-in (the user's core principle):**
`Connection::tick()` unbudgeted default + single deadline path. Verify
`chunk_packet_drain_test`, `mp_chunk_delivery_test`, drain throughput green in Debug &
Release.

**Wave 3 — collapse (medium risk, test-guarded):**
`FrustumCuller` inline, `clean*`+inverse-builder dedup, `Gui◦Projection` inline,
camera source-of-truth, `ChunkRegionBuffer`→`ChunkBuilder`, 6 container merges
(`sectionList_`, rings, pending pair, cull-state scratch).

**Wave 4 — split the monolith + infra cut (largest diff):**
worldRenderer → facade + `ChunkSectionSystem` + `ChunkCompilePipeline`; drop
`lightingGate_`/`ChunkSource`/`FrameBudget`/`ThreadBudget` bloat. Run full suite + F3 in
singleplayer, LAN hosting, Java-MP join.

---

## Verification gates
- `build-omega.ps1` full suite green each wave (esp. `packet_roundtrip`,
  `connection_packet_drain_throughput`, `chunk_packet_drain`, `mp_chunk_delivery`,
  `connection_async_teardown`, `region_snapshot_race`, `audio`).
- Diff builds Release(-O3) vs Debug(-O0) for chunk_drain: post-Wave-2 the gap is small.
- F3 "Chunks drawn" plateaus; reconcile loaded = in-frustum + occlusion-culled + empty.
- Singleplayer renders unbudgeted; only Java-MP/host connections show budget behavior.
```