# PLAN-PRELIMINARY-UPDATED — Coordinated Threading / Main-Thread Restructure (Auditor-Corrected Draft)

Pipeline stage: **auditor + preliminary-plan updater (AGENT 1 of 3)**. Review-only — NO source edits,
NO builds. This is the input the final-plan creator (AGENT 2) will turn into the locked master plan, and
the fact-checker (AGENT 3) will verify against source.

Inputs folded in: `plan-initial.md`, `synthesis.md`, `audit-A-gaps.md`, `audit-B-assumptions.md`,
`parity-iris-glsl-source.md`, `parity-iris-bindings-matrices.md`, all six council docs, and a fresh
spot-verification of the disputed facts against the working tree (2026-08-01).

> **Line-number health warning (carried forward from synthesis §0).** The working tree has large
> uncommitted edits. All line numbers in this document are the **verified working-tree numbers** from
> AGENT-1's spot checks (§2), not the council/audit numbers. Executors must still grep-before-edit.

---

## 0. Verdict on `plan-initial.md` (what the audits said, and whether we adopt it)

| Auditor | Verdict | Adopted? |
|---|---|---|
| Audit-A | NOT buildable-by-executors as written — WI-2 build break (misses WorkerHandoff.hpp:14), oversubscription math self-contradictory, WI-3 misses `Chunk::setBlock`, lanes not file-independent, WI-12 gate misses WI-10, WI-6 lane incoherent, test wiring gap, HZ-08 `downloadPendingMods` unassigned, QD-18/17/14/26 dangling | Yes — all 14 findings folded in (§3, §6) |
| Audit-B | Architecture sound and central assumptions verify; but F1 (WorkerHandoff.hpp:14) is a guaranteed compile break, F2/F3 are parity ambiguities, F4 is a watchdog-feasibility gap | Yes — F1–F4 resolved here; F5–F11 folded in |
| Parity-glsl | Preprocessor is a deliberate subset; dialect fork (120 vs 330) is load-bearing; 6 macro gaps; 4 modern-path transform gaps; derivation seam must stay main-thread | Folded in (§8.1) |
| Parity-bindings | Mixed; fogMode values, clip-space, gbuffer provenance, entity-overlay path, far-plane, first-frame previous, mc_Entity are the real deviations; SSBO cap is **13** (already parity, do not touch) | Folded in (§8.2) |

**Net result:** the architecture (ThreadCoordinator + domain pools, main thread = GL thread, Channel handoff,
FramePipeline run restructure, Lifecycle teardown) is ratified. The *plan shape* is corrected: WI-2/WI-3/
WI-4/WI-12 are rewritten, WI-12 is split (a/b), two new work items (WI-15 parity fixes, WI-16 HZ-08) are
added, and the execution is re-organized into a 7-executor lane shape (§10).

---

## 1. Task statement (unchanged from plan-initial §0)

"Massive refactor of multithreading + main-thread handling." ~9 independent thread-creation sites, each
computing its own count from `hardware_concurrency()`; `Minecraft::run()` is one monolithic loop. Parity
targets: Java Beta 1.7.3 (single-thread loop, per-connection reader/writer, sim-thread packet apply) and
Java Iris 26.1 (render-thread-only GL, exact phase order, BufferFlipper stage semantics). Both are hard
invariants. **ServerProcessCoordinator (external `minecraft_server.exe`) stays out of scope** (§9 QD-11).

---

## 2. Spot-verification log (AGENT-1, task C) — disputed facts confirmed/corrected

All paths under `src/net/minecraft/`. "CONFIRMED" = the audit/named claim is true; corrections noted.

| # | Claim (audit/council) | Verified reality (working tree) | Status |
|---|---|---|---|
| 1 | `WorkerHandoff.hpp:14` default arg `recommendedThreadCount(2,2)` | `util/concurrent/WorkerHandoff.hpp:14`: `explicit WorkerHandoff(unsigned threadCount = WorkerPool::recommendedThreadCount(2, 2))` | **CONFIRMED.** WI-2 must touch this line. |
| 2 | `ChunkCache.cpp:217` `rec(3,2,4)` vs "3 workers" (network doc) | The call is at **ChunkCache.cpp:218**: `recommendedThreadCount(3, 2, 4)` (`const unsigned workers =` on :217). **4** workers on 16-logical, not 3. | **CONFIRMED 4**; the `:217` label in some docs is off-by-one — use **:218**. |
| 3 | recommendedThreadCount site count: "3 live + 1 default" vs "6" | Exactly **4 sites** reference it: definition `WorkerPool.hpp:61-68`; live calls `ChunkBuilder.hpp:156` (`rec(3,2,6)`), `ChunkCache.cpp:218` (`rec(3,2,4)`), `LightingEngine.cpp:49` (`rec(3,2,3)`); plus the **unused default** `WorkerHandoff.hpp:14` (`rec(2,2)`). Plus **2 hand-rolled `hw−2`**: `ShaderCompileService.cpp:47`, `GLCore.cpp:289`. | **CONFIRMED "3 live + 1 unused default + 2 hand-rolled"** — synthesis §4.1 M3 accurate. |
| 4 | `Chunk::setBlock` is a `blocks[]`/`meta` writer | `Chunk.cpp:16-60` (setBlock with meta): writes `blocks[index(...)]` at **:25**, `meta.set(...)` at **:32**. Second overload `Chunk.cpp:61-100` writes `blocks[...]` at **:69**, `meta.set` at **:76**. Both main-thread writers of blocks/meta. | **CONFIRMED.** Audit-A FINDING 3 is right: WI-3's lock plan must include `setBlock`. |
| 5 | `ModModels.cpp:626` worker write of `g_alphaTestRef` | `mod/model/ModModels.cpp:626`: `core::setAlphaTestRef(0.1f);` inside `drawLuaBlockWorld` (:616). Chain verified: `ChunkBuilder::buildMesh` (ChunkBuilder.cpp:253, **worker**) → `blockRenderManager.render(*block,…)` (:330) → `BlockRenderManager::render` → `mod::drawBlockWorld` (BlockRenderManager.cpp:156) → `drawLuaBlockWorld`. `RenderCore.cpp:70` `float g_alphaTestRef = 0.1f;`, main-thread reads at :479-481, setter :672-680. | **CONFIRMED.** Real worker→main UB (HZ-14), WI-5 target. |
| 6 | `FrameData.cpp` carries ~25 function-static accumulators | `client/render/uniforms/FrameData.cpp`: `previousFrame/currentFrame/initialized/frameCounter/frameTimeCounterAccumulator/previousFrameTime` (:191-196), static `cameraTracker` (:266), `smoothBlock/smoothSky` (:538-540), ~19 `static SmoothedState` (:589-604), `accumulator/initialized` (:685-686). | **CONFIRMED.** Serial-history state; must stay main-render-phase (parity-bindings §4.1). |
| 7 | SSBO cap: 8 (CONTEXT.md, stale) vs 13 | `client/render/pipeline/Resources.hpp:13`: `inline constexpr int kMaxShaderStorageBuffers = 13;`. `Resources.cpp:120,127` `clearBufferSubData`, `:123` `GL_DYNAMIC_STORAGE_BIT (0x0100)`. Enforced in Loader.cpp:1065,1125, Resources.cpp:72. | **CONFIRMED 13 + init + dynamic bit already in parity.** Do NOT "fix" back to 8 (parity-bindings §5.9). |
| 8 | Six/ten named prior-notes missing | glob `docs/agent-notes/*.md` = 13 files. **None** of `glsl.md`, `dualpaths.md`, `lua-iris-dualpaths.md`, `deabstract.md`, `dealias.md`, `passindex.md`, `matrix.md`, `uniforms.md`, `ssbo.md`, `frameorder.md`, `runpasses-split.md`, `scopes.md`, `CONTEXT.md` exist. | **CONFIRMED ALL MISSING.** The plan must NOT depend on CONTEXT.md or any prior note (§9 QD-27). |
| 9 | fogMode value contract (1/2/3 vs GL 9729/2049) | `FrameData.cpp:237` `values.fogMode = fog.enabled ? fog.mode : 0`; `RenderCore.cpp:464` same; `fogShape` OFF→−1/ON→1 (FrameData.cpp:239). Fog `mode ∈ {1,2,3}` (RenderCore.cpp:782-790). Vanilla pack `shaders/vanilla/shaders/lib/common.glsl:18-26` interprets 1/2/3 and *claims* "as Iris reports them" (false). | **CONFIRMED HIGH divergence.** See §8.2 F-1. |
| 10 | clip-space −1..1 vs Java zZeroToOne 0..1 | `camera/FrameRenderCamera.hpp:145-168` `buildCameraProjection`: `m[10]=-(f+n)/(f−n)`, `m[11]=−1`, `m[14]=−2fn/(f−n)` = classic GL [−1,1]. Java 26.1 uses [0,1]. | **CONFIRMED.** See §8.2 F-2. |
| 11 | far ≈ 2× Java | `GameRenderer.cpp:515-518,1061-1063,574`: `farPlane = renderDistanceBlocks * 2.0f`. Java `far = getEffectiveRenderDistance()*16`. | **CONFIRMED ~2×.** See §8.2 F-5. |
| 12 | gbuffer matrix back-derived vs live-captured | `FrameData.cpp:280-285` builds from recovered `FrameRenderCamera` (GameRenderer.cpp:1029-1054); Java captures live RenderSystem matrices. | **CONFIRMED.** See §8.2 F-3. |
| 13 | first-frame gbufferPrevious = current (C++) vs identity (Java) | `FrameData.cpp:660-665` copies current into previous on first frame; Java `MatrixUniforms.java:66-85` = identity. | **CONFIRMED.** See §8.2 F-6. |
| 14 | mc_Entity ivec4 + separate at_midBlock | `RenderCore.cpp:870` `vertexAttribPointer(6,4,…)` at `kOffEntity=36` (4-comp = ivec4 semantics); `at_midBlock` slot 4. Java-Sodium packed uint. | **CONFIRMED.** See §8.2 F-7. |
| 15 | nearLane set but never read | `ChunkMeshJob.hpp:54` `bool nearLane = false;`, set at `ChunkBuilder.hpp:135` (`job->nearLane = true`); **never read** in the upload path. | **CONFIRMED.** WI-4 target (HZ-31). |
| 16 | Test wiring gap (audit-A FINDING 8) | `CMakeLists.txt:339-381`: the 7 files `block_face_uv_test`, `color_targets_test`, `custom_uniforms_test`, `handshake_metadata_test`, `iris_hemisphere_chunk_offset_test`, `pack_blend_drawbuffer_test`, `shadow_celestial_modelview_test` are **NOT** in `MINECRAFT_TEST_SOURCES`/`MINECRAFT_SERVER_TEST_SOURCES`. But `mp_parity_updates_test.cpp` and `render_settings_test.cpp` **ARE** wired (:346,:348). | **CONFIRMED.** Wire the 7 in (new WI-T), and add mp_parity_updates/render_settings to the plan's matrix (they exist). |
| 17 | `meshJobInFlight` set / clearSections | `WorldRenderer.cpp:581` `chunk->meshJobInFlight = true` (startMeshJob); `clearSections` :372 → `cancelAll` :373. `~ChunkMeshJob` clears the flag (ChunkBuilder.cpp:220-225). | **CONFIRMED.** Audit-B F5: the dtor already clears it; the real silent-drop is the loader `PendingLoad`. |
| 18 | audit-B F10 citations | `gameCrashed` at Minecraft.cpp:**248** (not :837-841); `stop()` :347, `std::_Exit(0)` :391; inactive sleep block :**714-721** (not :705-710); `runRenderPhase` :670; `run()` :762; pendingScreenResize handled **inside tick()** :658-661; `compileChunks` called at GameRenderer.cpp:**1195**, `poll()` :711. | **CONFIRMED.** Use these numbers. |

---

## 3. What changed vs plan-initial (audit-findings resolution index)

| plan-initial item | Finding | Resolution in this draft |
|---|---|---|
| WI-2 | A-1/B-F1: misses `WorkerHandoff.hpp:14` → compile break | WI-2 file list += `util/concurrent/WorkerHandoff.hpp:14` (drop the default arg; sole live instantiation ChunkBuilder.hpp:155-156 passes an explicit count). |
| WI-2 | A-2: math self-contradictory ("7-8 on 8-core"), keeps per-owner pools = 3× oversubscription | **Rewritten.** One **shared `Domain::Compute` pool** sized from a single budget; worked table for 4/8/16 (§5). "7-8" figure corrected to a proper subdivision. |
| WI-3 | A-3: `Chunk::setBlock` (blocks/meta writer) missing; A-6: lock-order vs render-pin undefined; B-F7: outboxMutex_ edge unnamed | WI-3 file list += `Chunk.cpp:16-60,61-100` (`setBlock` ×2). Explicit ordering invariant: chunk lock always after pin acquire, never held across `beginRenderEviction`; never hold chunk lock while taking `queueMutex_`/`outboxMutex_`; `ioMutex_`/`readMutex_`/`writeMutex_` have no cross-nesting (verified). |
| WI-4 | A-5: `~ChunkMeshJob` already clears flag (B-F5) — epoch token's real job is protecting R3 under a shared pool | WI-4 rationale rewritten: cancel already clears flag today (main-thread-only destruction); epoch/generation token is required once the pool is shared so a worker can't drop the last `shared_ptr`. Make `~ChunkMeshJob` main-thread-only asserted or atomic (HZ-11). |
| WI-4 | A-4, A-5: lane conflicts; WI-12 gate omits WI-10 | Dependency/lane graph corrected (§7, §10): WI-12 gate = WI-4/6/7/8/9/**10**; shared files assigned to one lane (Minecraft.cpp → L2/P2b; LightingEngine.cpp → P2a; ChunkCache.cpp → P2a). |
| WI-6 | A-7: "own lane inside shared pool" is incoherent (WorkerState keyed by thread::id) | QD-03 resolved: lighting gets **dedicated pinned Compute workers** (sub-pool of the Compute domain) so `WorkerState`/pin-cache stays thread-affine; alternatively re-key per-box. Specified in WI-6. |
| WI-7 | A-10: QD-18/17/14/26 dangling; B-F5 PendingLoad is the real leak | Resolved: epoch token (WI-4), loader `PendingLoad` cancel (WI-7), budget yield order (WI-12b), in-flight-flag (WI-9), chunk-rate-gate keep (WI-9). |
| WI-8 | A-8: test wiring | New WI-T wires the 7 orphan tests into CMakeLists; plan matrix corrected (mp_parity_updates/render_settings added). |
| WI-9 | B-F6: read-cap D2 uncommitted | D2 committed: bounded read queue + `disconnect.overflow` mirrored on `0x100000`. Risk-register line added for unbounded readQueue_ growth. |
| WI-12 | B-F2: pendingScreenResize placement; B-F3: mesh upload/poll call sites vs present; B-F11: oversized | **Split WI-12a/12b.** 12a = additive FramePipeline/TaskMailbox/FrameProfiler scaffold + FrameBudget deadline API, inert, verbatim order. 12b = rewiring + budget consolidation, gated on channels + WI-10. `pendingScreenResize` stays in `tick()` (Minecraft.cpp:658-661); `compileChunks` stays at GameRenderer.cpp:1195 and `poll()` at :711 (after present); present-before-draw confirmed intentional (QD-07). "Byte-for-byte" claim corrected to "phase *shape* preserved, call-site-relative order preserved". |
| WI-13 | A-11: log-thread vs QD-06 contradiction; B-F4: watchdog can't reach owner-internal joins; B-F8/F9 risk register gaps | Log thread registered for **unblock/watchdog only, never joined** (coordinator does not own it; crash handlers keep a standalone path). Each blocking owner takes a `deadline/stop_token` from Lifecycle; keep `std::_Exit(0)` fallback; GL-context-affine workers are never "leaked" *before* `DisplayManager::destroy`. Risk-register lines added: Lua/mod state main-thread-only (B-F8), GL context-loss ordering (B-F9), read-queue growth (B-F6). |
| — | A-9: HZ-08 worst offender (`downloadPendingMods`) unassigned | **New WI-16** moves `downloadPendingMods` (ClientNetworkHandler.cpp:258-320) to staged Io-pool download + main-thread apply; `getTextureId` decode and stat-save declared main-thread **by decision** (not omission). |
| — | A-10: QD-14 "no two verifies in flight" unowned | WI-9 adds per-connection in-flight flag on the Io domain (covers ServerLoginNetworkHandler re-verify + ClientNetworkHandler re-join). |
| — | parity audits | **New WI-15** = the parity-fix batch (see §8); §8.2 table records the keep/document decisions that must NOT be changed. |

---

## 4. Target architecture (ratified, unchanged from architecture proposal + plan-initial §1)

1. **`ThreadCoordinator` singleton**, one global budget computed once, `WorkerPool::recommendedThreadCount` **deleted**. Configured in `Minecraft::init()` and `server-main.cpp`.
2. **Small fixed set of domain pools sharing the one budget** (NOT one giant pool): `Compute` (mesh+lighting+gen — **one shared pool**, the audit-A fix), `Io` (blocking file/save + HTTP one-shots), `GlCompile` (2–3, own shared GL contexts), pinned registered classes (audio 4, logging 1, network per-connection, dir watcher 1).
3. **Main thread = the GL thread, never split.** Loop stays `tick → render → present`. Iris order + BufferFlipper preserved.
4. **`Channel<T>`** = canonical produce-on-worker/consume-on-main handoff (bounded, prioritized, stop-aware, version-stamped). Replaces `WorkerHandoff::completed_`, `LightingEngine::outbox_`, and (in time) `Connection` deques.
5. **`Minecraft::run()` → phase pipeline** (`FramePipeline`): DRAIN → INPUT+UI → timer+N ticks → RENDER → PACE → DIAGNOSTICS, tick/render order preserved (see WI-12a/12b). One shared per-frame `FrameBudget` deadline.
6. **`Lifecycle` shutdown**: ordered reverse-of-creation, unblock-then-join, watchdogged joins, owner-internal deadlines (B-F4), never-join-the-log-thread.
7. **Domain-pool model ratified over chunk-workers' "tagged single pool"** (synthesis M1): GL-context affinity, blocking-I/O isolation, and INT_MIN near-lane priority cannot share one queue.

---

## 5. Corrected thread-count / oversubscription math (audit-A FINDING 2)

### 5.1 The defect the plan-initial example proved

`globalBudget = max(1, hw − 2)`; architecture deducts pinned classes first. On **8 logical cores**:
budget=6; pinned audio(4)+log(1)+GlCompile(2)=**7 > 6** → remaining negative → `compute = clamp(−,1,8) = 1`,
`io = 2`. So compute=1 for mesh+lighting+gen combined — the plan's "mesh2/light2/loader2" is impossible,
and its "7-8 total" is also just wrong arithmetic (2+2+2+1+2 = 9, and shader today on 8-core is
`min(8−2,4)` = **4**).

### 5.2 The fix

**One shared `Domain::Compute` pool.** Mesh, lighting, and chunk-gen all submit tasks to the *same*
`coordinator.pool(Compute)`; there are no per-owner pools to multiply. The budget math:

```
cpuBudget   = max(1, hw − 2)                       // 1 reserved = main/GL thread, 1 = driver/OS
glCompile   = clamp(cpuBudget/4, 1, 2)             // dedicated GL-shared-context pool
io          = clamp(2, cpuBudget/4, 3)             // blocking file/save/HTTP-tolerant
compute     = max(1, cpuBudget − glCompile − io)   // ONE shared pool: mesh + lighting + gen
```

**Blocking/device-affine threads (audio 4, log 1, network 2/conn, dir watcher 1, watchdog 1) are
registered with the coordinator but NOT deducted** from `cpuBudget` — they block on devices/sockets and
rarely consume CPU; deducting them (as the architecture §3.2 wording suggests) starves compute on
low-core machines, which is exactly audit-A's point. They are still *counted* in the runtime-total
column and capped by `reserveDynamic` for network.

### 5.3 Worked table

| hw (logical) | cpuBudget | glCompile | io | compute (shared) | CPU-scheduled total | registered pinned (blocking) | runtime total + main |
|---|---|---|---|---|---|---|---|
| 4 | 2 | 1 | 2 | 1 | 4 | audio 4 + log 1 (+net 0) | 9–10 |
| 8 | 6 | 1 | 2 | 3 | 6 | audio 4 + log 1 | 11–12 |
| 16 | 14 | 2 | 3 | 8 (cap ≤8) | 13 | audio 4 + log 1 + net 2 | 19–21 |
| 32 | 30 | 2 | 3 | 8 (cap) | 13 | audio 4 + log 1 | 18–20 |

Compare today (16-logical): mesh4 + loader4 + save1 + light3 + shader4 = 16 compute/compile threads +
audio4 + log1 + net2 + watcher1 + watchdog1 + main1 ≈ **27–30**. After: 13 CPU-scheduled + ~5-7
blocking + main1 ≈ **19–21** — and critically, mesh+light+gen share *one* pool so a busy gen phase can't
sit idle while 12 other workers spin. On 8-core: ~12 vs today's ~17.

**Low-core policy (a user question, §11 Q4):** on hw=4 the CPU-scheduled total (4) still exceeds 2 free
cores; acceptable because glCompile/io are mostly blocking, but the coordinator must cap
`maxComputeThreads` and the user may prefer to reduce audio decode threads or accept oversubscription
on tiny machines. The bundled mingw64 toolchain machine (RULES §2) is likely the lowest-core target —
**do not budget for it exclusively**.

---

## 6. Work items (corrected and expanded)

Naming unchanged (`WI-n`); all build-safe and partial-landable. Executors re-verify every `file:line`
immediately before editing. **`WI-T`, `WI-15`, `WI-16` are new.** Lane assignment per item is the §10
execution shape.

### WI-1 — ThreadCoordinator / ThreadBudget / Channel<T> / ThreadNames / Lifecycle (infra) — lane L1

- New files under `util/concurrent/`: `ThreadCoordinator.hpp/.cpp`, `ThreadBudget.hpp`, `Channel.hpp`,
  `ThreadNames.hpp`, `Lifecycle.hpp/.cpp`.
- API to land: `ThreadCoordinator::instance()`, `configure(hw, reserved, opts)`, `pool(Domain)`,
  `budget()`, `reserveDynamic(n)/releaseDynamic(n)`, `totalPending()`, `shutdown()`;
  `enum class Domain { Compute, Io, GlCompile, Audio, NetIo, Log }`; `TaskPriority { Urgent, High, Normal, Low, Idle }`.
- **Add `ThreadBudget::glDriverThreads()` to the API** (audit-A FINDING 12) — feeds the
  `GL_KHR_parallel_shader_compile` hint (GLCore.cpp:289); add it to `tests/thread_budget_test.cpp`.
- Configure in `Minecraft::init()` (Minecraft.cpp:270-340) and `src/server-main.cpp:125-126`; wire nothing else.
- **Coordinator must NOT own the log thread** (QD-06). Lifecycle registers owners; log registers for
  unblock/watchdog only (WI-13).
- Channel = mutex+CV+deque (NOT lock-free; architecture §8.7).
- Verification: compile; new `tests/channel_test.cpp`, `tests/thread_budget_test.cpp`, `tests/lifecycle_test.cpp`.

### WI-2 — Delete `recommendedThreadCount`; route ALL counts through the coordinator — lane L1

- Files (corrected — **includes WorkerHandoff.hpp:14**):
  - `util/concurrent/WorkerPool.hpp:61-68` — **delete** `recommendedThreadCount`.
  - `util/concurrent/WorkerHandoff.hpp:14` — **remove the default arg** (`explicit WorkerHandoff(unsigned threadCount) : pool_(threadCount) {}`). Grep the whole tree for `recommendedThreadCount` after the delete; there are exactly the 4 sites above — zero must survive.
  - `ChunkBuilder.hpp:156` → `coordinator.pool(Compute).threadCount()` (mesh shares the one Compute pool).
  - `LightingEngine.cpp:49` → Compute-domain count (but see WI-6: lighting becomes a **pinned sub-pool**, so the count is the sub-pool size, not the whole Compute pool).
  - `ChunkCache.cpp:218` (loader → Compute, shared), `:225` (save → `Domain::Io` count, 1).
  - `ShaderCompileService.cpp:47` → `Domain::GlCompile` count (cap 2, per §5).
  - `GLCore.cpp:289` (driver hint) → `budget().glDriverThreads()`.
- **This item changes pool *identity*, not just counts** (audit-A FINDING 2 fix): mesh/lighting/gen call
  `coordinator.pool(Compute)`, so there is exactly one compute pool. Where an owner keeps a private pool
  object (WorkerHandoff, LightingEngine jthreads, ChunkCache pools) for the transition, it must be
  sized to the *shared* compute allocation divided among owners (never each taking the full count) —
  the honest intermediate is `max(1, compute/N)` per owner until WI-4/6/7 consolidate, and that
  subdivision must sum to ≤ compute. **State this in the code comment.**
- Parity: none (thread counts only). Keep `targetInFlight = workerCount*3/6` semantics
  (WorldRenderer.cpp:780 area) unchanged.
- Verification: compile + full ctest; new `tests/thread_coordinator_test.cpp`; watch
  `tests/chunk_mesh_golden_test.cpp` (determinism — workers must not change mesh output).

### WI-3 — Close the block/light-array snapshot race (R1/R4) — lane P1a

- Objective: make `RegionSnapshot` copy safe vs lighting workers AND main-thread writers (audit-A FINDING 3).
- Files (**corrected — includes `Chunk::setBlock`**):
  - `world/chunk/Chunk.hpp` — add per-chunk `std::atomic_flag`/spinlock for block+light-array writes;
    document at `tryAcquireRenderPin`/`beginRenderEviction` (Chunk.hpp:205-224).
  - `world/chunk/Chunk.cpp:16-60,61-100` — **`setBlock` takes the per-chunk lock** while writing
    `blocks[]`/`meta` (the writers audit-A said WI-3 missed).
  - `world/light/LightingEngine.cpp:249-258` (`setBrightness`→`setLight`) — take the lock writing nibbles.
  - `client/render/chunk/RegionSnapshot.cpp:32-67` (`copyChunkBand`) — take the same lock; ~11 KB copy, held µs.
  - `world/chunk/ChunkCache.cpp:173` (`populateBlockLight`), `:373-384` (`decorate`) — main-thread writers, lock too.
- **Lock-order invariant (audit-A FINDING 6 / B-F7):** chunk light-lock is acquired only *after* a pin is
  acquired, and *before* a nibble write/copy; **never** hold the chunk lock across `beginRenderEviction`
  (Chunk.hpp:205-224); never hold the chunk lock while taking `queueMutex_` (LightingEngine.cpp:60) or
  `outboxMutex_` (LightingEngine.cpp:422 — `runUpdate` runs outside `queueMutex_`, verified). `ioMutex_`
  (ChunkCache.hpp), `readMutex_`/`writeMutex_` (Connection.hpp) have no cross-nesting with the chunk
  lock today (B-F7 verified).
- Alternative if lock contention shows: version-stamp the arrays (option b) — start with (a).
- Parity: none; removes UB. Render output may shift subtly (torn copies were nondeterministic).
- Verification: compile + ctest (`chunk_mesh_golden_test`, `unified_light_registry_test`,
  `server_world_events_test`); new stress `tests/region_snapshot_race_test.cpp`.

### WI-4 — Mesh scheduler onto shared Compute + Channel; non-blocking cancel; nearLane upload — lane P1b

- Files: `client/render/chunk/ChunkBuilder.hpp:120-157` (back `handoff_` with `coordinator.pool(Compute)`
  + `Channel<std::shared_ptr<ChunkMeshJob>>`; keep the public API shape so `WorldRenderer` compiles);
  `util/concurrent/WorkerHandoff.hpp:31-40` (`drainCompleted`→channel batch, non-blocking; **`cancelAll`
  stops calling `pool_.drain()`** — drop queued + epoch/generation token, fix HZ-10/HZ-31);
  `client/render/world/WorldRenderer.cpp:372-373` (`clearSections`→cancel), `:581` (`meshJobInFlight`),
  `:747-766` (`processUpload`: honor `job->nearLane` first, dedicated budget slice per QD-21).
- **R3 invariant (audit-B F5):** `~ChunkMeshJob` (ChunkBuilder.cpp:220-225) already clears
  `meshJobInFlight` today *because* the last `shared_ptr` dies on the main thread. The epoch token is
  required once the pool is shared (a worker-side cancel could drop the last ref). Keep
  `cancelAll`/drop **main-thread-only**, and either assert main-thread destruction or make the flag
  atomic — do NOT claim the flag is stranded today.
- `sweepRetiring` (WorldRenderer.cpp:291-300) must reap in-flight jobs on completion.
- Parity: upload stays main-thread GL; pins unchanged; version-stamp stale-drop unchanged.
- Verification: compile + ctest; new `tests/mesh_cancel_test.cpp`.

### WI-5 — Confine GL state writes to the main GL thread (alphaTestRef + GLCore init) — lane P1a

- Files: `mod/model/ModModels.cpp:626` — stop calling `core::setAlphaTestRef(0.1f)` from mesh workers;
  capture the alpha ref into the job/`RenderSettings` snapshot at capture time (ChunkBuilder.cpp:189-195
  already snapshots lightLevelToLuminance + blockRenderLayers); default must stay **0.1** (QD-20).
  `client/render/RenderCore.cpp:70,479-481,672-680` — `g_alphaTestRef` becomes main-thread-write-only
  (`ASSERT_MAIN_THREAD()` / `TL_DOMAIN` guard in `setAlphaTestRef`). `client/gl/GLCore.cpp:121,141-145`
  — `g_loaded` plain bool → `std::once_flag`/atomic (HZ-15).
- Risk register (B-F8): mesh/compute workers must NEVER call Lua or enter `ModHost`; all `luaHook*` stay
  main-thread; `luaHookRenderFrame` (Minecraft.cpp:703) stays in the render phase.
- Verification: compile + ctest; `gl_state_affinity_test` is a **compile-fixer grep checklist**
  (audit-A FINDING 13), not a GTest — the behavioral test is the chunk-mesh golden test + a Debug-build
  assert.

### WI-6 — Lighting onto the Compute domain as a pinned sub-pool; outbox → Channel — lane P2a

- **QD-03 resolved:** lighting gets **dedicated pinned Compute workers** (a sub-pool of `Domain::Compute`,
  e.g. `max(1, compute/3)` of the shared allocation, owned by LightingEngine) so the per-worker
  `WorkerState`/pin-cache keyed by `std::thread::id` (LightingEngine.cpp:50-56,207-233) keeps working.
  Alternative (only if the pinned sub-pool is rejected): re-key `WorkerState` per-claimed-box — spell out
  the change to `tryClaimBox` :149-170 / `threadLoop` :187-206. Do NOT leave it "see D3".
- Outbox → `Channel<DirtyRegion>` (replaces `outbox_`/`drainDirtyRegions` :113-126); `World::doLightingUpdates`
  (World.cpp:711-717) signature unchanged. `stop()` :131-148 keeps request_stop-under-`queueMutex_` discipline.
- Box-conflict claim (HZ-34) preserved; channel capacity bounds huge sky boxes (HZ-35).
- Verification: compile + `server_world_events_test`, `unified_light_registry_test`; new `tests/lighting_channel_test.cpp`.

### WI-7 — Chunk loader/save onto domains; fix the PendingLoad silent-drop; lighting-ready gate — lane P2a

- Files: `world/chunk/ChunkCache.cpp:213-219` (loader → shared Compute), `:221-225` (save → Io, 1).
- **The real silent-drop (audit-B F5):** `requestChunkAsync` (ChunkCache.cpp:226-249) leaves a `PendingLoad`
  with `done==false` in `pendingLoads_` when its task is cancelled; `integrateFinishedLoads` (:250-288)
  scans forever. Add an epoch/generation token so cancel marks them cancelled and integrate drops them.
- `integrateFinishedLoads` budget from the shared per-frame deadline (WI-12b), not hard 2/32/4.
- Lighting-ready gate (HZ-32, QD-05): mark a column "lit" only after its boxes drain; hold first mesh
  until lit. **Scope is a user decision (§11 Q11)** — default: defer unless cheap.
- Per-thread generator clones (ChunkCache.cpp:85-105) keyed by thread::id assume stable identity — with
  a shared Compute pool this breaks; keep the loader on its own pinned Compute sub-lane OR prune the
  id→generator map on world switch (HZ-36). `ioMutex_` discipline preserved.
- Verification: compile + `server_chunk_cache_test`, `chunk_map_test`, host tests; new `tests/chunk_cancel_test.cpp`.

### WI-8 — Shader compile count via GlCompile; frame-driven poll; GL-snapshot derivation seam — lane P2b

- Files: `client/gl/ShaderCompileService.cpp:47` → GlCompile count (cap 2). `:144-177` `compileBlocking`
  stays only for the `!started()` fallback (:157-159); normal path = submit + frame poll.
  `client/gl/ProgramCache.cpp:159-229` poll driven from the `compileDone` channel each frame.
  **`programFromPackSync` (Pipeline.cpp:695-702) is dead code** — either delete it or keep ONLY as the
  `!started()` fallback, and guarantee both paths produce byte-identical `PreparedProgram` (DP-4).
- **GL-context snapshot before any worker (parity-glsl §4.1):** capture `glVersionMacro`/
  `maxColorBuffers`/`driverPreamble`/`supportedGlExtensions` once on the GL thread into a snapshot
  struct; `prepareProgram`/`prewarm` read only the snapshot. `supportedGlExtensions` has a
  non-synchronized `static bool initialized` (GlState.cpp) — fixed by the snapshot.
- **Derivation stays on one thread** (parity-glsl §4.3): `PackInstance::sourceCache`/`compiledPrograms`/
  `programEnabledCache`/`customUniforms` caches stay main-thread or become per-request-pure. Do NOT move
  derivation to GlCompile workers.
- `CustomUniformRuntime::evaluate` stays single-threaded per frame (thread_local RNG + smooth state).
  Add variadic `min`/`max` (U1) in WI-15, not here.
- **Worker `ShaderProgram::destroy()` writes `s_lastBoundProgram` (ShaderCompileService.cpp:200)** — a
  real cross-thread write today; benign, but flag/guard in this item (parity-glsl §4.5).
- Parity: identical pack-load behavior; `GL_KHR_parallel_shader_compile` hint unchanged.
- Verification: compile + shader tests (`shader_gl_integration_test`, `shader_frame_data_test`,
  `glsl_snippets_test`, `shader_pack_loader_test`).

### WI-9 — Network packet-path correctness (accounting, read cap, verify-result-only, single drain, in-flight flag) — lane P1c

- Files: `network/Packet.hpp:79-81,121-128` — move `packetTrackers()`/`incomingCount()` mutation out of
  reader threads → per-`Connection` counters merged on the game thread (HZ-17).
  `network/Connection.cpp:184-228` — **committed read-side cap (D2):** bounded `readQueue_` +
  `disconnect.overflow` mirrored on the existing `0x100000` send cap (:185-187). Keep send-overflow in
  `tick()`.
  `server/network/ServerLoginNetworkHandler.cpp:149-178` — verify thread publishes `deferredLoginPacket_`
  under `verifyMutex_` **only**; all `Connection`/`closed` mutation (:64,:172,:175) moves to the server
  tick thread (HZ-18). Remove the join in `verifyUsernameOnline` (:150-152). Per-connection **in-flight
  flag on the Io domain** so no two verifies for the same connection run (QD-14).
  `client/multiplayer/ClientNetworkHandler.cpp:243-245` — stop joining the prior `joinServerThread_` to
  reuse it (HZ-06); polled result via `processPendingJoinServer` (:55-83).
  Double-drain (H7): one drain per `Connection` per tick; `DownloadingTerrainScreen.hpp:24-26` drain
  collapses into `ClientWorld::tick` (:53-55); keep-alive moves to `ClientNetworkHandler::tick` (QD-15).
- Drain policy (QD-04): keep time-box (3 ms / min 8 / max 4096) + per-connection cap; ≤100/player as
  floor. Keep `interrupt()` after every drain; keep data-before-chunk discipline (Connection.cpp:293-316);
  keep the loose chunk-packet rate gate (`preferImmediate`, :296-312) as an **accepted approximation**
  (QD-26 — server already throttles via `getBlockDataSendQueueSize`).
- **Do NOT move `apply` off-thread.** FIFO drain order preserved.
- Verification: compile + connection/mp tests; new `tests/packet_accounting_test.cpp`.

### WI-10 — Async socket teardown; never join on the game thread — lane P1c

- Files: `network/Connection.cpp:134-137,157-160` (`disconnect`→CAS `open_`, `shutdown(SD_BOTH)`, cancel
  read interest, flush-with-grace, return; destruction deferred off the drain stack via the existing
  retire pattern Minecraft.cpp:778-788 / MultiplayerSession.cpp:12-19 + Lifecycle-registered joins).
  `network/Connection.hpp` reader_/writer_ → registered `Domain::NetIo` with `reserveDynamic(2)` +
  unblock-then-watchdog-join (**start with the blocking NetIo pool, defer the event loop** — QD-01).
  `server/network/ConnectionListener.cpp:99-154` — per-tick budget split with per-connection caps.
- Re-entrancy (H10): `ServerPlayNetworkHandler::disconnect` from inside a packet handler stays deferred
  off the drain stack. Reader cancellable via `shutdown(SD_RECEIVE)` today; under any later event loop,
  cancel-interest (parity §5 dev 4).
- **WI-12 gate now requires WI-10** (audit-A FINDING 5) — the FramePipeline must not codify the 30 s
  socket-join stall into the canonical loop.
- Parity: Java never joins socket threads on the sim thread (`NetworkMasterThread`, NetworkManager.java:160-189).
- Verification: same tests as WI-9; new `tests/connection_async_teardown_test.cpp`.

### WI-11 — Ephemeral / ad-hoc threads → Io domain (kill fire-and-forget) — lane P2b

- Files: `client/texture/ImageDownload.cpp:7-18` (**delete `.detach()`**; texture Channel; main applies —
  HZ-16), `client/session/SessionValidator.cpp:17-51` (→ Io task, HZ-28), `client/diagnostics/ClientDiagnostics.cpp:235-264`
  (watchdog **registered**, not detached), `client/multiplayer/MultiplayerConnector.cpp:17-61,63-68`
  (never `bridge->disconnect()` → `setWorld` from the connector thread — HZ-25; publish under `mutex_`),
  `client/resource/ResourceDownloadThread.cpp:85-120,138,168` (worker produces files/bytes only; main
  applies — HZ-24), `client/gui/auth/LoginScreen.cpp:98-106,125-134,260-271`, `client/auth/microsoft/SessionRestore.cpp:55-92,163-175,237-268`
  (HTTP on Io; no mid-flight join), `world/World.cpp:300-339` (level.dat `std::launch::async` → Io pool
  with a **value** snapshot — HZ-20/QD-13; `save(true)` still waits, H9),
  `server/dedicated/gui/DedicatedServerGui.cpp:61-82,153-161` (`guiThread.detach()` + `stopped` plain-bool
  → registered thread + atomic — HZ-22).
- Skin/image apply stays main-thread-only after channel drain (QD-24).
- `ServerProcessCoordinator` (external process) stays out of scope (QD-11).
- Verification: compile + full ctest; new `tests/image_download_channel_test.cpp`, `tests/multiplayer_connector_test.cpp`.

### WI-12a — FramePipeline/TaskMailbox/FrameProfiler scaffold (inert, verbatim order) — lane L2

- New files `client/util/FramePipeline.hpp/.cpp`, `client/core/TaskMailbox.hpp/.cpp`, `client/util/FrameProfiler.hpp/.cpp`.
- **Additive and inert** (audit-B F11): new classes compile standalone, not yet wired into `run()`; the
  existing `run()`/`runRenderPhase` bodies are preserved untouched. `FrameProfiler` absorbs the two
  `static std::mutex stallMutex` blocks (Minecraft.cpp:742-743 in runRenderPhase, :827-828 in run) —
  one mutex, one file, phase-enum trace, behind `MINECRAFT_RENDER_TRACE` (QD-09).
- `FrameBudget.hpp` gains the shared `deadline` API (set once per frame); no consumers changed yet.
- Must land independently of L1 (no ThreadCoordinator dependency in the inert scaffold).
- Parity: zero behavior change (inert).
- Verification: compile; `tests/render_profiler_test.cpp`, `tests/client_timer_test.cpp` stay green.

### WI-12b — Run/render rewiring + unified frame budget (gated) — lane P2b

- **Gate: WI-4, WI-6, WI-7, WI-8, WI-9, WI-10 all landed** (audit-A FINDING 5 — WI-10 included). Every
  drain has a channel; the mailbox drains them.
- Wire `Minecraft.cpp:762-843` into `FramePipeline::run()` preserving the tick-order invariant
  (Minecraft.cpp:647-649) and the runRenderPhase order (:670-761). **Call-site-relative order is the
  contract, not literal byte equality** (audit-B F2/F3): `pendingScreenResize` handling stays inside
  `tick()` at Minecraft.cpp:658-661 (Phase 1, not Phase 0); `compileChunks` stays invoked from
  `onFrameUpdate` at GameRenderer.cpp:1195; `shaderPacks_->poll()` stays at GameRenderer.cpp:711 —
  both **after** `pumpAndPresent` (:694). Only the *drain internals* move to channels.
- Budget consolidation: `FrameBudget::deadline` per frame; `WorldRenderer::compileChunks` upload
  (:742-743) + capture (:787-789), `integrateFinishedLoads`, `Connection::tick` (3 ms) all read
  `frame.remaining()`. **Yield order (QD-17):** network drain ≥ near-mesh ≥ lighting ≥ distant mesh ≥
  integrates.
- Inactive sleep stays at Minecraft.cpp:714-721 (Phase 3); diagnostics (heartbeat :776, FPS window
  :731-737, toast :727, profiler :722-726) become Phase 4. `fpsLimit=0` keeps hot-spin parity (QD-10).
- Present-before-draw (Minecraft.cpp:694 then :708) confirmed intentional (QD-07) — preserve.
- Verification: compile + ctest; new `tests/frame_pipeline_order_test.cpp` asserts
  present-before-compileChunks-before-poll.

### WI-13 — Lifecycle teardown wiring — lane P2b

- Files: `client/Minecraft.cpp:347-393` `stop()` → `Lifecycle::shutdown()` (fence → unblock → stop+drain
  → **watchdogged join with per-owner deadlines** → session-clear → GL destroy). Encode
  ShaderCompileService::stop (:354) before DisplayManager::destroy (:388); keep `std::_Exit(0)` (:391)
  as the final fallback.
- **B-F4 fix:** the blocking joins live *inside* owners (`WorldSession::clearWorld`→`World::~World`→
  `ChunkCache` dtor `waitForPendingWrites`+`loaderPool_->drain()`; `Connection::~Connection` join;
  audio joins; ServerProcessCoordinator 10 s wait). Each owner takes a `deadline/stop_token` from
  Lifecycle so the watchdog can actually fire. **GL-context-affine workers (GlCompile) are never leaked
  before `DisplayManager::destroy`** — leak only after, or destroy shared contexts first; the leak is
  safe today only because `std::_Exit` follows (say so).
- **Log thread (A-11/QD-06):** register for unblock/watchdog only, **never join**; coordinator does not
  own it; crash handlers (Minecraft.cpp:248 `gameCrashed`, post-loop `stop()` :860) keep a standalone
  log path that works post-shutdown.
- ServerProcessCoordinator :279-290 and Logging.cpp:129-177 register with Lifecycle.
- Risk-register: GL context-loss ordering (B-F9), Lua main-thread-only (B-F8), read-queue growth (B-F6).
- Verification: compile + host/server tests + `lifecycle_test.cpp`.

### WI-14 — Apply the coordinator to the dedicated server binary (deferred) — lane P2b

- `src/server-main.cpp:125-126`, `MinecraftServer.cpp:196-214` (commandThread_), `ConnectionListener`
  accept thread, per-connection threads — same budget + registration. **Scope is a user question (§11 Q5)**;
  default: defer (separate process; server ≈21-23 threads today is acceptable).
- Verification: compile server target + `minecraft_server_tick_test`, `player_manager_test`,
  `server_command_handler_test`.

### WI-15 — (NEW) Iris parity-fix batch — lane P2b

Objective: land the parity items the audits classify as **(a) fix**, in one additive, cache-bump-gated
batch. **Do NOT touch the (b) keep items** (§8.2). Every macro change bumps the shader disk-cache
format (parity-glsl §5.4).

FIX list (a):
1. **`MC_GLSL_VERSION` semantics (M1, HIGH)** — change from *pack's* version to the *driver's* GLSL
   version, captured in the WI-8 GL snapshot (Java `StandardMacros.java:53`). This changes `#if`
   outcomes — bump cache format.
2. **Missing macros (M2-M5, M6)** — define `MC_NORMAL_MAP`/`MC_SPECULAR_MAP` **unconditionally**
   (Java `StandardMacros.java:101-102`), `MC_RENDER_QUALITY`/`MC_SHADOW_QUALITY` = 1.0 (:103-104),
   `IRIS_HAS_TRANSLUCENCY_SORTING` (:60), and add `CAT_MOUNTAIN`/`CAT_UNDERGROUND` to `kCategories`
   (SourceProcessor.cpp:745-748; Java `BiomeCategories.java` has 19, C++ 17) with index fix. **Gated on
   user decision (§11 Q6)** — define only what the engine actually implements.
3. **fogMode GL constants (F-1, HIGH)** — send `0/9729/2049` (GL_LINEAR=0x2601, GL_EXP2=0x0801) instead
   of 1/2/3, **atomically** across `FrameData.cpp:237-239`, `RenderCore.cpp:464`, the vanilla pack's
   `common.glsl:18-26`, and `CustomUniforms.cpp:343` (parity-bindings §5.4); keep `fogShape` OFF→−1/ON→1.
4. **First-frame gbufferPrevious identity (F-6, LOW)** — seed previous* = identity on frame 0
   (FrameData.cpp:660-665 → identity per `MatrixUniforms.java:66-85`).
5. **Custom-uniform variadic `min`/`max` (U1, MED)** — extend `callFunction` (CustomUniforms.cpp:599-620)
   to variadic (Java supports 3-16 args); additive.
6. **`sildursWaterFract` micro-patch (T9, LOW)** — add the known `fract(worldpos.y+0.001)→0.01` patch
   (CompatibilityTransformer.java:159-162) if a shipped pack needs it.

Verification: compile + shader tests; **no source-behavior change when the active pack uses none of
these macros** (bump cache, re-verify RenderPearl/SEUS PTGI).

### WI-16 — (NEW) HZ-08: `downloadPendingMods` off the main thread — lane P2b

- `client/multiplayer/ClientNetworkHandler.cpp:258-320` — staged Io-pool download (HTTP fetch → zip →
  parse → install) with progress surfaced to `ServerModDownloadScreen` via a channel; main thread applies
  only the final install/rescan. **Declare** `TextureManager::getTextureId` decode/upload
  (TextureManager.cpp:451-483) and the stat-save on `setScreen` (ScreenStack.cpp:33-38) **main-thread by
  decision**, not omission (audit-A FINDING 9).
- Parity: Java downloads mods on background threads too; only ownership discipline changes.
- Verification: compile + host tests; manual join-server-mod-download smoke (compile-fixer runs the app once).

### WI-T — (NEW) Wire the 7 orphan tests into ctest — lane L1

- Add to `MINECRAFT_TEST_SOURCES`/`MINECRAFT_SERVER_TEST_SOURCES` (CMakeLists.txt:339-381):
  `block_face_uv_test.cpp`, `color_targets_test.cpp`, `custom_uniforms_test.cpp`,
  `handshake_metadata_test.cpp`, `iris_hemisphere_chunk_offset_test.cpp`, `pack_blend_drawbuffer_test.cpp`,
  `shadow_celestial_modelview_test.cpp`. Confirm they build (some may need GL-stub fixtures). Fix the
  plan's §6 matrix so "green" is meaningful (audit-A FINDING 8). Add `mp_parity_updates_test` /
  `render_settings_test` to the plan's matrix rows (already wired, plan forgot them).
- Verification: `.\build-omega.ps1 -RunTests` runs every file in the matrix.

---

## 7. Dependency graph (corrected per audit-A FINDING 4/5)

```
L1 (foundational, first):
 WI-1 (infra) ──► WI-2 (delete rec + counts) ──► WI-T (test wiring)   [WI-2 deps WI-1]
 WI-3 (race)     ← independent of WI-1/2; parallel-safe with L1
L2 (foundational, first, independent of L1):
 WI-12a (inert scaffold)                       [no deps; must land alone]

P1 (after L1+L2 land):
 P1a: WI-3 (race) ──► WI-5 (GL-state)          [WI-5 uses job/snapshot from WI-3-era capture path]
 P1b: WI-4 (mesh channel)                      [deps WI-2]
 P1c: WI-9 (net correctness) ──► WI-10 (async teardown)   [WI-10 deps WI-9, WI-1(Lifecycle)]

P2 (mop-up):
 P2a: WI-6 (lighting sub-pool) ──► WI-7 (loader/save)      [WI-6/7 deps WI-2; shared files: one lane]
 P2b: WI-8 (shader) ; WI-11 (ephemeral) ; WI-15 (parity) ; WI-16 (HZ-08)
      WI-12b (rewiring)  [GATE: WI-4,6,7,8,9,10 all landed]
      WI-13 (lifecycle)  [deps WI-8, WI-11, WI-12b]
      WI-14 (server, optional)
```

Key corrections: **WI-4 now depends on WI-2** (Lane 1 in the old plan wrongly ran WI-4 before WI-2);
WI-12b's gate includes **WI-10**; shared files are confined to one lane (LightingEngine.cpp/ChunkCache.cpp
→ P2a, Minecraft.cpp → L2/P2b, Connection.cpp → P1c, GLCore.cpp → P1a+WI-8 conflict resolved by
sequencing WI-5 before WI-8 in P2b).

---

## 8. Parity decisions (folded from both parity audits)

### 8.1 Parity-glsl (source-level)

| # | Item | Decision | Where |
|---|---|---|---|
| T1/DP-1 | **GLSL 120 vs Java-forced 330 core** | **(b) KEEP** — load-bearing deliberate divergence; the port targets legacy OptiFine packs. Do NOT unify. Keep `sourceCompilesAsModern` (SourceProcessor.cpp:535-544) + both `lower*` paths. | documented, §9 assumptions |
| T2/T3 | Modern-path fragment `gl_TexCoord`/`gl_Color`/`gl_FragData` rewriting missing | **(c) out of scope for the threading refactor** — a 130+ pack using legacy fragment varyings fails to compile in C++; real gap, but a separate dialect-completeness task. Document as known gap + follow-up. | follow-up |
| P1-P6 | Preprocessor = deliberate JCPP subset | **(b) KEEP** — document; no rewrite in this refactor. | documented |
| P7 | `#include <…>` supported (superset) | **(b) KEEP** — permissive superset; matches Java's "no include guard" behavior (both inline before `#if`). | documented |
| P8 | `const …Format` strip | **(b) KEEP** — load-bearing for 120 packs (SEUS PTGI deferred10.fsh:18); Java removal site unconfirmed — do not touch without proof. | documented |
| M1 | `MC_GLSL_VERSION` semantics | **(a) FIX** → driver GLSL version (WI-15), captured in WI-8 GL snapshot. | WI-15 |
| M2 | `MC_NORMAL_MAP`/`MC_SPECULAR_MAP` unconditional | **(a) FIX** (WI-15), gated on user decision. | WI-15 |
| M3 | `IRIS_REQUIRES_SEPARATE_ENTITY_DRAWS` | **(b)/(a) user decision (§11 Q6)** — define only if the engine's entity pass genuinely matches; default **do not define** (lying macro is worse). | WI-15 |
| M4 | `IRIS_HAS_TRANSLUCENCY_SORTING`, `IRIS_TAG_SUPPORT` | Translucency-sorting: **(a) FIX** (engine sorts). `IRIS_TAG_SUPPORT`: **(c) out of scope** — b1.7.3 has no tag system. | WI-15 |
| M5 | `MC_RENDER_QUALITY`/`MC_SHADOW_QUALITY` | **(a) FIX** = 1.0 (WI-15). | WI-15 |
| M6 | `CAT_MOUNTAIN`/`CAT_UNDERGROUND` | **(a) FIX** (WI-15). | WI-15 |
| M7/M8/M10 | `IRIS_FEATURE_*` required-feature emission; `IRIS_VERSION` fixed 1.9.2; `MAX_COLOR_BUFFERS` hardware query | **(b) KEEP** — document as the port's contract; align only if a pack visibly breaks. | documented |
| U1 | variadic `min`/`max` | **(a) FIX** (WI-15), additive. | WI-15 |
| U2 | `random()` custom uniform thread_local | **(b) KEEP** single-threaded per frame (hazard 4) — never parallelize `evaluate`. | WI-8 risk |
| T9 | `sildursWaterFract` | **(a) FIX** if a shipped pack needs it (WI-15). | WI-15 |
| T10 | `default_composite.vsh` (irs_texCoords/irs_Color) | **(c) deferred** unless a legacy composite-only pack breaks; document. | follow-up |
| DP-2..9 | Compute-vs-raster, dimension, async-vs-sync, clrwl_, base-vs-world, two `#if` engines, fallback maps | **(b) KEEP in lockstep**; unify DP-8 (two `#if` engines) only as a later refactor. | documented |
| §4 | Derivation seam: GL queries context-bound; contentHash determinism; caches main-thread; `s_lastBoundProgram` | **(a) FIX / guard** in WI-8 (GL snapshot; order-stable derivation; cache bump on macro change). | WI-8 |

### 8.2 Parity-bindings (matrices / bindings)

| # | Item | Decision | Where |
|---|---|---|---|
| F-1 | **fogMode values 1/2/3 vs GL 9729/2049** | **(a) FIX** — atomic change across FrameData.cpp/RenderCore.cpp/vanilla pack/CustomUniforms (user-confirmed §11 Q3). | WI-15 |
| F-2 | **Clip-space depth −1..1 vs Java zZeroToOne 0..1** | **(b) KEEP** — the whole C++ chain (projection, shadowProjection, depthtex reconstruction, all shipped packs) is self-consistent in −1..1; flipping breaks every pack. Document; **never** change piecemeal. | documented, MUST-NOT-CHANGE |
| F-3 | gbuffer matrix provenance (back-derived vs live-captured) | **(b) KEEP + risk note** — exact for rigid cameras; must NOT become a "parallel" step; guarded by `shader_frame_data_test`/`draw_camera_state_test`. | documented |
| F-4 | Entity-overlay path (iris_overlay sampler, iris_Entity/OverlayUV/LightUV, overlay-derived entityColor) | **(c) out of scope** — substantial new feature (EntityPatcher vertex/geometry passthrough), not threading. Document as known gap; if added later, additive (uniform path stays). | follow-up |
| F-5 | far-plane ≈ 2× Java (`renderDistanceBlocks*2` vs `renderDistance*16`) | **(b) KEEP + verify** against shipped packs; flipping changes depth/fog scaling. User decision (§11) if a pack breaks. | documented |
| F-6 | First-frame gbufferPrevious = current vs identity | **(a) FIX** (WI-15, LOW). | WI-15 |
| F-7 | mc_Entity ivec4 + at_midBlock vs Sodium packed uint | **(b) KEEP** — `.x` blockId matches; `.y` metadata is a port extension; converting breaks the metadata path. | documented |
| SSBO | cap 8 (stale) vs 13; clearBufferSubData; DYNAMIC_STORAGE_BIT | **Already parity — (b) KEEP, do not "fix"** (Resources.hpp:13, Resources.cpp:120-128). | documented |
| D1-D5 | GUI-vs-world camera, frame-vs-per-draw matrices, world-vs-shadow camera, pack-vs-vanilla uniform path, base-vs-active pack | **(b) KEEP in lockstep** — all main-GL-thread, ordered publish/restore (`ScopedDrawCameraState` RenderCore.cpp:370-386). | documented |
| D6 | Lua-mod world mesh writes `g_alphaTestRef` on workers | **(a) FIX (mandatory)** — WI-5. The only outright bug. | WI-5 |
| D7 | No-pack "off" state = bundled vanilla pack | **(b) KEEP** — no real no-shader path; do not create one. | documented |
| §4.1 | FrameData ~25 statics + EMAs; RenderCore GL-state globals; RenderCameraState singleton; worldUniforms_; PackDefinition id maps; SSBO/image GL creation; ColorTargets flip; shadow state; g_shaderBlockIds atomic | **(b) KEEP main-thread-by-fiat** — the frame-data/matrix/upload computation is the least movable code; spend parallelism on mesh/lighting/network, not bindings. `g_shaderBlockIds` stays atomic if workers read it. | documented |

---

## 9. Accepted assumptions & prior-note status (was plan-initial §7)

- **CONTEXT.md and all 13 named prior-notes are MISSING** (verified, §2 #8). The plan stands entirely on
  the council/audit/parity docs + direct source verification. **Do not reference CONTEXT.md, glsl.md,
  dualpaths.md, lua-iris-dualpaths.md, matrix.md, uniforms.md, ssbo.md, frameorder.md, deabstract.md,
  dealias.md, runpasses-split.md, scopes.md, or passindex.md** as if they exist. `GlslSnippets.hpp:13`
  still comments "see docs/agent-notes/glsl.md" — a stale pointer; consider updating the comment.
- **(A-1)** The C++ port (`src/net/minecraft/client/`) is the authoritative mirror of the b1.7.3 client
  loop (Java client sources absent from the repo; `third_party/mcp` is server+shared only). QD-16 ratified.
- **(A-2)** The integrated server is an external process (`ServerProcessCoordinator`) — a deliberate
  divergence from Java's in-process server thread; the functional contract (client↔server over the
  network stack) is preserved. Out of scope.
- **(A-3)** Audio (4 pinned XA2 threads) and the pack-dir watcher stay pinned registered threads (QD-12);
  log thread is coordinator-registered but never joined (QD-06).
- **(A-4)** Iris 26.1 (`third_party/mcp/iris`, line-identical to `third_party/iris`) is the render-order
  reference.
- **(A-5)** The GLSL dialect fork (120 vs 330) is a deliberate, load-bearing divergence (§8.1 T1); the
  b1.7.3 MC_VERSION=10703 encoding is correct for this game (M9).
- **(A-6)** "Never join on the game thread" (runtime) and "watchdogged join" (final shutdown) are
  reconciled by phase (synthesis M4): runtime disconnect/teardown-on-stack never joins; `Lifecycle::shutdown()`
  (process exiting) unblocks-then-joins with a 2-5 s watchdog that logs-and-leaks.

---

## 10. Execution shape — 7 executors, lanes (the required 7-subagent split)

**Two foundational subagents run FIRST; the 2 foundational lanes must land independently of each other.**
Then 3 P1 lanes, then 2 P2 lanes. The compile-fixer runs only after all lanes; within each lane items
are ordered so every intermediate state compiles.

| Executor | Lane | Work items | Must land after | Must land before |
|---|---|---|---|---|
| 1 | **L1** (foundational) | WI-1, WI-2, WI-T | — (first) | everything |
| 2 | **L2** (foundational) | WI-12a (inert scaffold) | — (first, **independent of L1**) | WI-12b |
| 3 | **P1a** | WI-3, WI-5 | L1 (WI-2 for counts optional), L2 | P2b (WI-12b) |
| 4 | **P1b** | WI-4 | L1 (WI-2) | P2a/P2b |
| 5 | **P1c** | WI-9, WI-10 | L1 (WI-1 Lifecycle), L2 | WI-12b gate |
| 6 | **P2a** | WI-6, WI-7 | L1 (WI-2), P1b (WI-4 for pool shape) | P2b |
| 7 | **P2b** | WI-8, WI-11, WI-15, WI-16, **WI-12b**, WI-13, WI-14 | P1a/P1b/P1c/P2a all landed (WI-12b gate = WI-4/6/7/8/9/10) | compile-fixer |

Notes:
- **L1 and L2 are file-disjoint** (L1: `util/concurrent/*` + the 4 count sites + CMakeLists; L2: new
  `client/util/FramePipeline.*`, `client/core/TaskMailbox.*`, `client/util/FrameProfiler.*`, FrameBudget
  API, Minecraft.cpp stall-mutex extraction). The one overlap risk (Minecraft.cpp: L1 touches `init()`
  configure :270-340; L2 touches the stall-mutex blocks :742-743/:827-828) is two disjoint regions in the
  same file — acceptable, compile-fixer reconciles; or L2 defers the stall-mutex collapse to P2b if a
  conflict surfaces.
- **Shared-file rule (audit-A FINDING 4):** files touched by two lanes are sequenced, not concurrent —
  `LightingEngine.cpp`/`ChunkCache.cpp` only in P2a; `Connection.cpp` only in P1c; `GLCore.cpp` touched
  by P1a (WI-5, g_loaded) then P2b (WI-8) — P1a finishes before P2b starts; `Minecraft.cpp` in L1/L2
  disjoint regions then P2b (WI-12b/13).
- **Cross-lane edges now honored:** WI-4 waits on WI-2 (L1); WI-6/7 wait on WI-2 and WI-4 (pool shape);
  WI-10 waits on WI-9 and WI-1; WI-12b waits on WI-4/6/7/8/9/10 (including WI-10).

---

## 11. Test matrix (corrected for wiring)

| Test | Guards | WI |
|---|---|---|
| client_timer_test | timer cadence | 12a/12b |
| chunk_mesh_golden_test | mesh determinism | 2,4,5,12b |
| chunk_map_test / server_chunk_cache_test | loader/save/integrate | 4,7 |
| unified_light_registry_test / server_world_events_test | light | 3,6 |
| connection_packet_drain_throughput_test / chunk_packet_drain_test | drain | 9,12b |
| connection_listener_test / server_login_handler_test / packet_roundtrip_test | network | 9,10 |
| mp_chunk_delivery_test / mp_chunk_data_test | client chunk stream | 9,10 |
| multiplayer_client_player_entity_test | client entity | 11 |
| shader_gl_integration / shader_frame_data / glsl_snippets / custom_uniforms / shader_pack_loader | shader | 8,15,12b |
| draw_camera_state / camera_position_tracker / render_settings | camera/budget | 12b |
| render_profiler | profiler | 12a |
| integrated_server_host / lan_host_coordinator | host | 7,10,11,13 |
| minecraft_server_tick / player_manager / server_command_handler | server | 13,14 |
| mp_parity_updates | Connection::tick/disconnect ordering | 9,10 (added to matrix) |
| **WI-T wiring (7 files)** | block_face_uv, color_targets, custom_uniforms, handshake_metadata, iris_hemisphere_chunk_offset, pack_blend_drawbuffer, shadow_celestial_modelview | all (regression net) |

New tests: channel, thread_budget, lifecycle, thread_coordinator, region_snapshot_race, mesh_cancel,
lighting_channel, chunk_cancel, packet_accounting, connection_async_teardown, image_download_channel,
multiplayer_connector, frame_pipeline_order. All must be in `MINECRAFT_TEST_SOURCES`/
`MINECRAFT_SERVER_TEST_SOURCES`. `gl_state_affinity_test` is **not** a GTest (audit-A FINDING 13) — it is
a compile-fixer grep checklist (TL_DOMAIN guard present, `setAlphaTestRef` main-only).

---

## 12. Risk register (extended with audit-B findings)

1. Deadlock at shutdown (blocking owner internal to Lifecycle phase) — owners take deadline/stop_token;
   `_Exit(0)` fallback; log thread never joined (B-F4, A-11).
2. Light-array race regression / lock-order deadlock — pin-then-light-lock order invariant; debug assert
   (A-6/B-F7).
3. Chunk unload vs pinned worker — render-pin refcount + version stamps preserved (WI-3/4).
4. GL-context loss — GlCompile teardown before primary-context destroy; never leak GL workers before
   `DisplayManager::destroy` (B-F9, A-7).
5. Oversubscription recurrence — delete `recommendedThreadCount`; coordinator.pool() is the only legal
   source; code-review rule (A-2).
6. Performance regression from shared Compute pool — compute=1 on low-core; `totalPending()` on F3;
   per-domain caps; targetInFlight caps kept (A-2).
7. Behavior drift in run() — call-site-relative order contract; frame_pipeline_order_test; ship behind a
   flag (B-F2/F3).
8. **Lua/mod state thread-safety** — mesh workers never call Lua/ModHost; all luaHook* main-thread (B-F8).
9. **GL context-loss / TDR on window close** — ordering encoded in Lifecycle, not a comment; driver-reset
   TDR out of scope (B-F9).
10. **Unbounded readQueue_ growth** — committed bounded cap + disconnect.overflow (B-F6/D2).
11. Mesh snapshot torn copy — per-chunk lock in `setBlock` + `copyChunkBand` + lighting writes (A-3).
12. Stale `nearLane`/budget contradiction — near-lane upload priority + dedicated slice (HZ-31/QD-21).
13. Shader disk-cache staleness on macro changes — bump cache format with any WI-15 macro change
    (parity-glsl §5.4).

---

## 13. Deliverable note to AGENT 2 (plan master)

- Base the final plan on this file, not plan-initial. Carry forward the worked budget table (§5), the
  corrected dependency graph (§7), the lane assignment (§10), the parity decision matrix (§8), and the
  verified line numbers (§2).
- The user questions in `questions-for-user.md` must be answered before the final plan is locked — the
  four decisions with the widest blast radius are Q1 (run() scope), Q4 (thread budget machine), Q6
  (parity macro batch), and Q9 (how many lanes land in this pass).
