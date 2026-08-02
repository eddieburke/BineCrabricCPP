# PLAN-MASTER — Coordinated Threading / Main-Thread Restructure (FINAL)

Pipeline stage: **plan master (AGENT 2 of 3)**, fact-checked and corrected by **AGENT 3 (fact checker /
plan corrector)** — every `file:line` re-verified against the working tree (2026-08-01); corrections folded
in per `plan-corrections.md`. Review-only — NO source edits, NO builds.
Inputs folded in: `plan-preliminary-updated.md` (primary), `questions-for-user.md` (12 decisions,
**all answered**, see §0), `synthesis.md`, `plan-initial.md`, `audit-A-gaps.md`, `audit-B-assumptions.md`,
`parity-iris-glsl-source.md`, `parity-iris-bindings-matrices.md`, all six council docs, and fresh
spot-verification of every `file:line` against the working tree (2026-08-01).

> **READ FIRST — line-number health warning (carried from synthesis §0 / preliminary §intro).**
> The working tree has large uncommitted edits. All line numbers here are **working-tree-verified**
> (AGENT-2 re-grepped every citation while writing this document). Executors MUST grep-before-edit:
> `RULES FOR AGENTS.md §7` makes every executor responsible for re-verifying the live number.
> The **compile-fixer is the only stage that builds/tests** (RULES §2, §7).

---

## 0. Locked user decisions (questions-for-user.md → this plan)

| Q | Decision (final) | Effect on this plan |
|---|---|---|
| Q1 | **12a now, 12b follow-up** | WI-12a lands (inert scaffold, zero behavior change); WI-12b is PASS-2, gated on WI-4/6/7/8/9/10. |
| Q2 | **MIGRATE ALL SHADERS TO GLSL 330 CORE** (like Java Iris 26.1) | New PASS-2 item **P-GLSL330**; adds fragment `gl_TexCoord`/`gl_Color`/`gl_FragColor` rewriting (T2/T3); drops the 120-primary fork. Planned here, executed in pass 2. |
| Q3 | **FIX BOTH fogMode + entity-overlay NOW** | PASS-1 **P-FOGMODE** (0/9729/2049, atomic 4-file) and PASS-2 **P-ENTITYOVERLAY** (full `iris_overlay`/`iris_Entity`/`iris_OverlayUV`/`iris_LightUV` + overlay-derived `entityColor`). Entity-overlay is large → planned here (pass 2); fogMode is small → this pass. |
| Q4 | **Typical 8–16 logical; tunable `maxComputeThreads` default 8** | `ThreadCoordinator::configure(hw, reserved, {.maxComputeThreads = 8})`. |
| Q5 | **Client only this pass; dedicated server deferred** | WI-14 PASS-2 (server-main config optional/deferred). ServerProcessCoordinator (external process) **out of scope**. |
| Q6 | **FULL Java-textual macro set** (match Java even where engine lacks the capability) | PASS-1 **P-MACROS** includes `IRIS_REQUIRES_SEPARATE_ENTITY_DRAWS`, `IRIS_TAG_SUPPORT`, etc. Bumps shader disk-cache format (`ShaderBinaryCache.cpp:9`). |
| Q7 | **Register, don't deduct** blocking/device threads | Audio(4)/log(1)/NetIo/watcher registered but NOT deducted; `reserveDynamic` makes counts authoritative. |
| Q8 | **Bounded read queue + `disconnect.overflow`** mirrored on `0x100000` | Committed in WI-9 (D2 no longer open). |
| Q9 | **Per-connection threads as `Domain::NetIo` + async teardown; event loop deferred** | Committed in WI-10 (D1 resolved: blocking NetIo pool first). |
| Q10 | **UNIFY the two `#if` engines** into one state machine; KEEP render dual paths in lockstep; fix WI-5 | PASS-1 **P-IFENGINE** (unify); WI-5 is the one dual-path bug fix; world-vs-GUI / frame-vs-draw / world-vs-shadow cameras stay dual + lockstep (MUST-NOT). |
| Q11 | **Land lighting-ready gate in this pass** (split out of WI-7 as its own item) | PASS-1 **P-LITGATE**. |
| Q12 | **Foundational + P1 only this pass** | PASS-1 = WI-1, WI-2, WI-T, WI-12a, WI-3, WI-4, WI-5, WI-9, WI-10, P-FOGMODE, P-LITGATE, P-IFENGINE, P-MACROS. **PASS-2 (planned, not scheduled into the 7 executors)** = WI-6, WI-7, WI-8, WI-11, WI-12b, WI-13, WI-14, WI-15, WI-16, P-GLSL330, P-ENTITYOVERLAY. |

The **7 executors in THIS pass** execute PASS-1 only (§6). PASS-2 items are fully specified (§4) for a later pipeline run — **do not schedule PASS-2 items into the 7 lanes**.

---

## 1. Final target architecture

Ratified from council-architecture-proposal + audit-A/B + preliminary §4, **extended with the
330-core and entity-overlay workstreams** the user committed in Q2/Q3.

1. **`ThreadCoordinator` singleton** (`util/concurrent/ThreadCoordinator.hpp/.cpp`) — ONE global thread
   budget computed once in `Minecraft::init()` (`Minecraft.cpp:256-346`) and (PASS-2, WI-14)
   `src/server-main.cpp:105-126`. `WorkerPool::recommendedThreadCount` is **deleted**.
   API: `instance()`, `configure(hw, reserved, opts)`, `pool(Domain)`, `budget()`,
   `reserveDynamic(n)`/`releaseDynamic(n)`, `totalPending()`, `shutdown()`;
   `enum class Domain { Compute, Io, GlCompile, NetIo, Audio, Log }`;
   `TaskPriority { Urgent, High, Normal, Low, Idle }`; **`maxComputeThreads` default 8** (Q4).
   **Blocking/device-affine threads are registered but NOT deducted** (Q7): audio(4), log(1),
   NetIo, dir-watcher(1). Budget math (preliminary §5.2):
   ```
   cpuBudget = max(1, hw - 2);  glCompile = clamp(cpuBudget/4, 1, 2);  io = clamp(2, cpuBudget/4, 3);
   compute   = min(maxComputeThreads, max(1, cpuBudget - glCompile - io));   // ONE shared pool
   ```
   Worked table (Q4): 8-core → cpu 6, glCompile 1, io 2, compute 3; 16-core → 14/2/3/**8**; 32-core → 30/2/3/**8** (cap).
2. **Small fixed set of domain pools sharing the one budget** (NOT one giant pool): `Compute`
   (mesh + lighting + chunk-gen — **one shared pool**, the audit-A FINDING-2 fix), `Io` (blocking
   file/save + HTTP one-shots), `GlCompile` (1–2, own shared GL contexts), pinned registered classes
   (audio 4, log 1, network per-connection = `NetIo` via `reserveDynamic`, dir watcher 1).
3. **Main thread = the GL thread, never split.** Loop stays `tick → render → present` (Minecraft.cpp:647-649
   tick-order invariant; `runRenderPhase` :670-761 order). Iris phase order + BufferFlipper preserved.
4. **`Channel<T>`** (`util/concurrent/Channel.hpp`) = canonical produce-on-worker/consume-on-main
   handoff (bounded, prioritized, `stop_token`-aware, `push/tryPush/tryPop/drain/reset/size`,
   version-stamped). Mutex+CV+deque (NOT lock-free). Replaces `WorkerHandoff::completed_`,
   `LightingEngine::outbox_` (PASS-2 WI-6), and eventually Connection deques.
5. **`Minecraft::run()` → phase pipeline** (`FramePipeline`, PASS-1 WI-12a inert; PASS-2 WI-12b rewire):
   DRAIN → INPUT+UI → timer+N ticks → RENDER → PACE → DIAGNOSTICS. Tick/render order preserved by
   **call-site-relative order** (not byte equality — audit-B F2/F3). One shared per-frame
   `FrameBudget::deadline` (`FrameBudget.hpp:5-14`).
6. **`Lifecycle` shutdown** (`util/concurrent/Lifecycle.hpp/.cpp`): ordered reverse-of-creation,
   unblock-then-join, watchdogged joins (2–5 s then log-and-leak), owner-internal deadlines
   (audit-B F4), **never join the log thread** (QD-06). Wired in PASS-2 WI-13.
7. **One `#if`/`#define` state machine (PASS-1 P-IFENGINE)** — the GLSL engine
   (`SourceProcessor.cpp:184-311`, CondFrame stack) and the `.properties` engine
   (`Loader.cpp:307-435`, activeStack/matchedStack) unify onto one shared conditional-state
   implementation. Render dual paths (world-vs-GUI matrices `GuiProjection.hpp:11-30` vs
   `GameRenderer.cpp:1064`; frame-uniform back-derivation `FrameData.cpp:280-298` vs live per-draw
   `RenderCore.cpp:406-419`; world-vs-shadow cameras `GameRenderer.cpp:988-1064` vs
   `ShadowMapPass.cpp:284-316`) stay **in lockstep, NOT unified** (Q10).
8. **GLSL 330-core workstream (PASS-2 P-GLSL330)** — `sourceCompilesAsModern`
   (`SourceProcessor.cpp:535-544`) becomes the only path; `lowerVertexSource`/`lowerFragmentSource`
   (:569-639 / :655-692) drop the 120 `attribute`/`varying` branch; fragment `gl_TexCoord`→`irs_texCoords[3]` +
   `gl_Color`→`irs_Color` (T2) and `gl_FragColor`→`gl_FragData[0]`, `gl_FragData[i]`→
   `layout(location=i) out vec4 iris_FragDatai` (T3) added per Java `CommonTransformer.java:182-232`;
   `versionPreamble` (`SourceProcessor.cpp:709-778`) forces 330 core. **Bumps disk-cache format.**
9. **Entity-overlay workstream (PASS-2 P-ENTITYOVERLAY)** — additive `iris_overlay` sampler
   (Java `IrisSamplers.java:209-211`), `iris_Entity` ivec3 attribute, `iris_OverlayUV`/`iris_LightUV`
   vertex inputs, and `entityColor` **derived from `texelFetch(iris_overlay, …)`** per
   `EntityPatcher.java:33-122`. The existing uniform upload path
   (`RenderCore.cpp:89,453-458,528-543,903-908`) stays as the additive base (parity-bindings §5 MUST-NOT 12).
10. **Migration order (bottom-up, build-safe)**: infra → kill `recommendedThreadCount` → compute/chunk
    coordination → network → shader/macros → (pass 2) lighting/loader → ephemeral threads → main loop →
    teardown → server.

---

## 2. NEVER-PARALLELIZE / GL-thread-confined invariants (hard, all passes)

1. **Every OpenGL call** — create/delete/upload/link/bind/draw/uniform. Includes mesh
   `ChunkRegionBuffer::upload`/`glBufferSubData` (`ChunkRegionBuffer.cpp:74-111`),
   `ChunkBuilder::uploadMesh` (`ChunkBuilder.cpp:352-415`), `ProgramCache` linking, texture upload
   (`TextureManager::tick`/`getTextureId` :451-483), present/swap, FBO/render-target creation, `glDispatchCompute`.
2. **Shader compile/link** only on `GlCompile` workers on their own hidden GLFW windows sharing the main
   context (`ShaderCompileService.cpp:8-19,41-61`). **Blobs cross threads; live `GLuint`s never do.**
   Main thread applies binaries. `runJobOnCurrentContext` stays main-thread-only for the `!started()`
   fallback (`ShaderCompileService.cpp:157-159`).
3. **Iris flip/render order** — per-frame `flipped` snapshot evaluation and stage chain
   (BEGIN-reads-last-frame-shadow → renderShadows → shadowComposite → prepare → world →
   captureOpaqueDepth → hand → captureHandDepth → deferred → post; `GameRenderer.cpp:1083-1330`).
   Never move a stage to a worker; never hoist a drain ahead of `pumpAndPresent`
   (`Minecraft.cpp:694` then `onFrameUpdate` :708; `compileChunks` stays at `GameRenderer.cpp:1195`,
   `shaderPacks_->poll()` stays at :711 — audit-B F3).
4. **Frame-global monotonic state** — frame counter, `partialTick`/`tickDelta`, texture reload counter;
   `buildShaderFrameData` + its ~25 function-static accumulators + `g_centerDepthSmooth`/
   `g_wetnessSmooth` (`FrameData.cpp:32-33,191-196,266,538-604,670-692`) stay on the render phase.
5. **`PipelineManager::destroy → re-prepare` with 16-unit unbind** — atomic on the render thread.
6. **Packet apply (all `handle*`/`on*`)** — sim thread (client main, server tick) only
   (`Connection.cpp:184-228`). Socket threads decode/push only. FIFO drain order preserved.
7. **World simulation state mutation** — `World`/entities/block entities/`WorldEvents`, chunk decorate
   (`ChunkCache.cpp:380-391`), `setBlock`, heightmaps. Workers never dereference live world state; they
   operate only on `RegionSnapshot` (+ record block-entity positions, `ChunkMeshJob.hpp:27`).
8. **`~ChunkMeshJob` / R3** — the last `shared_ptr<ChunkMeshJob>` must die on the main thread
   (`ChunkBuilder.cpp:220-225`). `cancelAll`/drop stays main-thread-only; WI-5 asserts/atomics the write.
9. **`Timer`/tick cadence** — no tick/render thread split (QD-25); cap N at 10; paused-partialTick freeze.
10. **`Minecraft::stop()` GL/context ordering** — `ShaderCompileService::stop()` (`Minecraft.cpp:354`)
    before `DisplayManager::destroy()` (:388); world/workers stopped before contexts destroyed.
11. **Per-connection packet write ordering + sim-thread drain ordering** — writer serial per socket; FIFO.
12. **GL state writes incl. `g_alphaTestRef`** — main-GL-thread only (WI-5 fixes the one worker→main UB).
    `GLCore::init()` `g_loaded` (GLCore.cpp:121,141-145) → `std::once_flag`/atomic.

### Parity MUST-NOT lists (do not change; from parity-glsl §5 and parity-bindings §5)

- **GLSL derivation pipeline order + purity** (`Compiler.cpp:120-143` resolveIncludes → `normalizePackSource`
  → `lowerVertexSource`/`lowerFragmentSource` → `mergeColorWheelMaterial` → `assemble`). Pure function of
  (programName, stage, pack, dimension, source, preamble) + a GL snapshot (PASS-2 WI-8 captures it once on GL thread).
- **The two `lower*` dialect paths** stay until P-GLSL330 (then unify to 330); the 120 path must not be
  "simplified away" in this pass.
- **Macro table & emission** (`seedMacrosFromDefines`, `PreProcessor.cpp:469-507`; preamble block
  `SourceProcessor.cpp:724-777`): additions allowed (P-MACROS) but **every change bumps the shader disk-cache
  format** (`ShaderBinaryCache.cpp:8-9` `kFileVersion = 1` → 2).
- **`#include` resolution rule** (`IncludeResolver.cpp:24-72`); **`stripFormatDirectives`** (P8) stays.
- **Vertex attribute contract** (`ShaderProgram.cpp:146-157`; `RenderCore.cpp:839-877`) — port's own layout.
- **Program selection maps** (`PassIndex.cpp:178-261`; `Pipeline.cpp:704-782` incl. `clrwl_` early-out :744).
- **Custom-uniform evaluation** — one `evaluate()` per runtime per frame on one thread; `random()` is
  `thread_local`; never parallelize (parity-glsl §4 hazard 4).
- **`programFromPackSync`** (`Pipeline.cpp:695-702`) — dead; either delete or keep ONLY as `!started()`
  fallback producing byte-identical `PreparedProgram` (WI-8).
- **fogShape** OFF→−1/ON→1 mapping stays (`FrameData.cpp:239`, `RenderCore.cpp:465`).
- **SSBO path already in parity — do NOT "fix"**: cap **13** (`Resources.hpp:13`), `clearBufferSubData`
  init (`Resources.cpp:120,127`), `GL_DYNAMIC_STORAGE_BIT` (:123).
- **Clip-space depth −1..1** — the whole C++ chain is self-consistent; NEVER flip piecemeal (parity-bindings §5.3).
- **Colortex startIndex=4 world/shadow, 0 fullscreen** (`ColorTargets.cpp:714-717`); legacy alias map
  gcolor=0/gdepth=1/gnormal=2/composite=3 (`GlState.cpp:216-223`).
- **Shadow sampler compare rules** (`ShadowMapPass.cpp:245-246`; `WorldProgramBinder.cpp:133-141`).
- **`ScopedDrawCameraState` publish/restore order** (`RenderCore.cpp:370-386`) + GUI-ortho vs world-perspective producers.
- **`g_shaderBlockIds` stays atomic** (`RenderType.cpp:14,29-36`).
- **`entityColor`/entityId/blockEntityId/currentRenderedItemId** uploads stay on the main GL thread;
  the `iris_overlay` path is **additive**, never a replacement (P-ENTITYOVERLAY).

---

## 3. Risk register (carried + extended)

1. Deadlock at shutdown → owner deadline/stop_token; `_Exit(0)` fallback; log thread never joined (B-F4/A-11).
2. Light-array race regression / lock-order deadlock → pin-then-light-lock order; debug assert (A-6/B-F7).
3. Chunk unload vs pinned worker → render-pin refcount + version stamps preserved (WI-3/4).
4. GL-context loss → GlCompile teardown before primary-context destroy; never leak GL workers before `DisplayManager::destroy` (B-F9).
5. Oversubscription recurrence → delete `recommendedThreadCount`; `coordinator.pool()` the only legal source (A-2).
6. Performance regression from shared Compute → `totalPending()` on F3; per-domain caps; `targetInFlight` kept (`WorldRenderer.cpp:780`).
7. Behavior drift in `run()` → call-site-relative order contract; `frame_pipeline_order_test`; ship behind flag (B-F2/F3).
8. **Lua/mod state thread-safety** — mesh workers never call Lua/`ModHost`; all `luaHook*` main-thread (B-F8).
9. **GL context-loss / TDR on close** — ordering encoded in Lifecycle (WI-13), not a comment (B-F9).
10. **Unbounded `readQueue_`** → committed bounded cap + `disconnect.overflow` (WI-9, D2 closed).
11. Mesh snapshot torn copy → per-chunk lock in `setBlock` + `copyChunkBand` + lighting writes (WI-3).
12. Stale `nearLane`/budget contradiction → near-lane upload priority + dedicated slice (WI-4).
13. Shader disk-cache staleness on macro/330 changes → bump cache format with every change (P-MACROS/P-GLSL330).
14. **P-MACROS scope creep** (Q6 full set incl. capability macros) → bounded: defines only; verified behavior-neutral for packs not branching on them; cache bump isolates.

---

## 4. THE WORK BREAKDOWN (numbered; both passes)

Naming: `WI-n` = threading-refactor items (from preliminary), `P-*` = parity/workstream items introduced
by the locked decisions. Every item is build-safe and partial-landable. **Every `file:line` is
working-tree-verified 2026-08-01; grep before edit.** PASS-1 items are §4.1, PASS-2 items §4.2.

### 4.1 PASS-1 items

#### WI-1 — ThreadCoordinator / ThreadBudget / Channel / ThreadNames / Lifecycle (infra) — PASS-1, L1
- **Objective:** create the ONE authority for thread counts, the canonical handoff, thread naming, ordered teardown. Configure; wire nothing else.
- **Files (new):** `src/net/minecraft/util/concurrent/ThreadCoordinator.hpp/.cpp`,
  `src/net/minecraft/util/concurrent/ThreadBudget.hpp`, `src/net/minecraft/util/concurrent/Channel.hpp`,
  `src/net/minecraft/util/concurrent/ThreadNames.hpp`, `src/net/minecraft/util/concurrent/Lifecycle.hpp/.cpp`.
- **Change:** API per §1 item 1. **Add `ThreadBudget::glDriverThreads()`** (audit-A FINDING 12) and
  `maxComputeThreads` (Q4) + `computeShare(owners)` helper for the WI-2 interim subdivision. Channel =
  mutex+CV+deque, bounded+prioritized+stop-aware+version-stamped. `Minecraft::init()` (`Minecraft.cpp:256-346`)
  calls `ThreadCoordinator::instance().configure(hardware_concurrency(), 2, {.maxComputeThreads=8})`.
  `src/server-main.cpp` stays untouched this pass (Q5/WI-14).
- **Coordinator must NOT own the log thread** (QD-06). `Lifecycle` registers owners; log registers for
  unblock/watchdog only (wired in WI-13).
- **Parity:** none (dead code until WI-2).
- **Risks:** Channel must never block producers on main; keep `tryPush` for main-thread producers.
- **Verification:** compile. New GoogleTests (all wired by WI-T): `tests/channel_test.cpp`
  (tryPush at capacity blocks, tryPop ordering, Urgent-first, reset wakes producers, stop push→false),
  `tests/thread_budget_test.cpp` (formulas §1; glDriverThreads), `tests/lifecycle_test.cpp`
  (unblock→request_stop→join; watchdog leak path), `tests/thread_coordinator_test.cpp`.
- **Blocked-by:** none.

#### WI-2 — Delete `recommendedThreadCount`; route ALL counts through the coordinator — PASS-1, L1
- **Objective:** kill oversubscription root cause (rank #1). One authority for "how many threads".
- **Files & change (all four `recommendedThreadCount` sites + two `hw−2` sites — grep whole tree, zero may survive):**
  - `util/concurrent/WorkerPool.hpp:61-68` — **delete** `recommendedThreadCount`.
  - `util/concurrent/WorkerHandoff.hpp:14` — remove the default arg:
    `explicit WorkerHandoff(unsigned threadCount) : pool_(threadCount) {}` (audit-A F1 / audit-B F1 — compile break if missed).
  - `client/render/chunk/ChunkBuilder.hpp:156` — `handoff_` count → `coordinator.pool(Domain::Compute).threadCount()`.
  - `world/light/LightingEngine.cpp:49` — `workers` → Compute-domain count (sub-pool sizing for WI-6 note).
  - `world/chunk/ChunkCache.cpp:218` (loader) → Compute count; `:225` (save) → `Domain::Io` count (1).
  - `client/gl/ShaderCompileService.cpp:47` → `Domain::GlCompile` count (cap 2).
  - `client/gl/GLCore.cpp:288-290` (driver hint) → `budget().glDriverThreads()`.
- **Pool-identity note (audit-A FINDING 2):** this item changes pool *identity* only where the owner can
  already submit to the shared pool. Where an owner keeps a private pool for the transition (WorkerHandoff,
  LightingEngine jthreads, ChunkCache pools), size it to **`max(1, compute / N)`** (N = number of compute
  owners = mesh/lighting/loader = 3) so the subdivision sums to ≤ compute. **State this in the code comment.**
  The genuine single-shared-Compute consolidation is WI-4 (mesh) and PASS-2 WI-6/7 (lighting/loader).
- **Parity:** none (counts only). Keep `targetInFlight = workerCount*3/6` (`WorldRenderer.cpp:780`) unchanged.
- **Risks:** forgetting the subdivision (each owner taking the full count = 3× oversubscription).
- **Verification:** compile + full ctest. `tests/chunk_mesh_golden_test.cpp` must stay byte-identical
  (workers must not change mesh output). New `tests/thread_coordinator_test.cpp` asserts budget derivation.
- **Blocked-by:** WI-1.

#### WI-T — Wire the 7 orphan tests into ctest; pre-register all new PASS-1 tests — PASS-1, L1
- **Objective:** make "green" meaningful (audit-A FINDING 8).
- **File:** `CMakeLists.txt:339-381`.
- **Change:** add to `MINECRAFT_TEST_SOURCES`/`MINECRAFT_SERVER_TEST_SOURCES`: `block_face_uv_test.cpp`,
  `color_targets_test.cpp`, `custom_uniforms_test.cpp`, `handshake_metadata_test.cpp`,
  `iris_hemisphere_chunk_offset_test.cpp`, `pack_blend_drawbuffer_test.cpp`, `shadow_celestial_modelview_test.cpp`.
  Confirm they build (some may need GL-stub fixtures). **Also pre-register every new PASS-1 test file**
  (`channel_test`, `thread_budget_test`, `lifecycle_test`, `thread_coordinator_test`,
  `region_snapshot_race_test`, `mesh_cancel_test`, `packet_accounting_test`,
  `connection_async_teardown_test`, `fog_mode_parity_test`, `lighting_ready_gate_test`,
  `if_engine_unified_test`, `macro_parity_test`) so **no other lane edits CMakeLists** (file-ownership rule §6).
- **Parity:** none.
- **Risks:** unwired files silently not run. Note `custom_uniforms_test`/`handshake_metadata_test` are the
  WI-8/WI-11 coverage files — must be wired before those PASS-2 items claim them.
- **Verification:** `.\build-omega.ps1 -RunTests` runs every file in the matrix. `mp_parity_updates_test.cpp`
  (:346) and `render_settings_test.cpp` (:348) already wired — keep.
- **Blocked-by:** none (can land with or before WI-1).

#### WI-12a — FramePipeline / TaskMailbox / FrameProfiler inert scaffold + FrameBudget deadline API — PASS-1, L2
- **Objective:** additive, inert, zero behavior change (audit-B F11); land independently of L1 with **no file overlap**.
- **Files (new):** `src/net/minecraft/client/util/FramePipeline.hpp/.cpp`,
  `src/net/minecraft/client/core/TaskMailbox.hpp/.cpp`, `src/net/minecraft/client/util/FrameProfiler.hpp/.cpp`.
- **File (edit):** `src/net/minecraft/util/concurrent/FrameBudget.hpp:5-14` — add `FrameBudget::deadline`
  (set once per frame) + `FrameBudget::remaining()`. **No consumers changed yet.**
- **Change:** classes compile standalone. `FrameProfiler` provides a phase-enum trace API behind
  `MINECRAFT_RENDER_TRACE` but does **not** modify `Minecraft.cpp` this pass (the two `stallMutex` blocks at
  `Minecraft.cpp:742-743` and `:827-828` are absorbed by WI-12b, keeping L1/L2 file-disjoint).
  `TaskMailbox` exposes the urgent/tick/render drain-queue API (unused). `FramePipeline` documents the
  DRAIN→INPUT→TICKS→RENDER→PACE→DIAGNOSTICS phase shape (unused).
- **Parity:** zero (inert).
- **Risks:** none functional; keep classes header-only-friendly so WI-12b can wire without churn.
- **Verification:** compile. Existing `tests/render_profiler_test.cpp`, `tests/client_timer_test.cpp` stay green.
- **Blocked-by:** none. **No dependency on WI-1/L1.**

#### WI-3 — Close the block/light-array snapshot race (R1/R4) — PASS-1, P1a
- **Objective:** make `RegionSnapshot` copy safe vs lighting workers AND main-thread writers
  (audit-A FINDING 3 — includes `Chunk::setBlock`; audit-B F7 lock-order).
- **Files & change:**
  - `world/chunk/Chunk.hpp` — add a per-chunk light/block-write guard (atomic flag/spinlock); document at
    `tryAcquireRenderPin`/`beginRenderEviction` (`Chunk.hpp:205-224`): chunk lock always acquired **after** a pin,
    **never** held across `beginRenderEviction`, never held while taking `outboxMutex_`
    (`LightingEngine.cpp:114`) or `registryMutex_`; `ioMutex_`/`readMutex_`/`writeMutex_` have no
    cross-nesting (verified). The only non-leaf nesting is `ioMutex_` → `chunkFileMutex()`
    (`AlphaChunkStorage.cpp:24-25`, taken at :202/:238); the save-pool worker takes `chunkFileMutex()` without
    `ioMutex_` and never re-enters ChunkCache, so no cycle exists. **The guard is non-recursive and must be
    scoped to the raw array writes only.**
  - `world/chunk/Chunk.cpp:16-60` and `:61-100` — `setBlock` (both overloads) takes the guard while writing
    `blocks[]` (:25/:69) and `meta` (:32/:76) **and releases it before `onBreak`/`onPlaced`/
    `updateHeightMap`/`lightGaps`/`world->queueLightUpdate` run** — those callbacks re-enter `Chunk::setBlock`
    for neighbor updates and take `queueMutex_` via `World::queueLightUpdate`→`LightingEngine::push`
    (`World.cpp:693-709`, `LightingEngine.cpp:60`); a whole-function guard self-deadlocks a non-recursive
    spinlock. Same scoping for `Chunk::setBlockMeta` (`Chunk.cpp:102-112`, writes `meta` at :107).
  - `world/light/LightingEngine.cpp:249-258` — `setBrightness`→`setLight` (`Chunk.hpp:133-138`) takes the guard
    writing nibbles; `setLight` has no re-entrant callbacks, so a whole-function scope is safe here.
  - `client/render/chunk/RegionSnapshot.cpp:32-67` — `copyChunkBand` takes the guard; ~11 KB copy, held µs.
  - `world/chunk/ChunkCache.cpp:174` (`populateBlockLight` call site) and `:380-391` (`decorate`) — main-thread
    writers, take the guard too (R4: save/adopt snapshot `ChunkCache.cpp:157-201` adopt, `:363-379` save;
    the light/block snapshot read is `AlphaChunkStorage::takeSnapshot` at `ChunkCache.cpp:371`).
- **Parity:** none; removes UB. Render output may shift subtly (torn copies were nondeterministic).
- **Risks:** lock-order deadlock — enforce the pin→light-lock order with a Debug assert; the guard must be
  scoped to the raw writes (see above) so `setBlock`'s block callbacks/`queueLightUpdate` run outside it.
- **Verification:** compile + ctest (`chunk_mesh_golden_test`, `unified_light_registry_test`,
  `server_world_events_test`). New `tests/region_snapshot_race_test.cpp`: one thread snapshots while a
  lighting-style writer mutates nibbles; assert no torn value, 1000×.
- **Blocked-by:** none (P1a runs after L1 by lane order; no WI-1/2 dependency).

#### WI-4 — Mesh scheduler onto shared Compute + Channel; non-blocking cancel; nearLane upload — PASS-1, P1b
- **Objective:** canonical channel handoff; `clearSections` stops blocking the main thread; honor `nearLane` (HZ-31).
- **Files & change:**
  - `client/render/chunk/ChunkBuilder.hpp:120-157` — back `handoff_` with `coordinator.pool(Domain::Compute)`
    + a `Channel<std::shared_ptr<ChunkMeshJob>>`; keep the public API shape (`enqueue/enqueueNear/
    drainCompleted/cancelAll/idle/pendingJobs/workerCount` — `ChunkBuilder.hpp:122-152`) so `WorldRenderer`
    compiles unchanged (`pendingJobs()` is used at `WorldRenderer.cpp:738` and `:781`).
  - `util/concurrent/WorkerHandoff.hpp:31-40` — `drainCompleted()` → channel batch (non-blocking);
    **`cancelAll()` stops calling `pool_.drain()`** — drop queued + **epoch/generation token** so owners
    learn their jobs were dropped (fixes HZ-10/HZ-31; `meshJobInFlight` can never strand).
  - `client/render/world/WorldRenderer.cpp:372-404` (`clearSections`→cancelAll), `:581` (`meshJobInFlight`),
    `:747-770` (`processUpload`: honor `job->nearLane` first, dedicated budget slice per QD-21).
  - `client/render/world/WorldRenderer.cpp:295-304` (`sweepRetiring`) — reap in-flight jobs on completion.
- **R3 invariant (audit-B F5):** `~ChunkMeshJob` (`ChunkBuilder.cpp:220-225`) already clears `meshJobInFlight`
  today because destruction is main-thread-only. The epoch token is required once the pool is shared so a
  worker cannot drop the last `shared_ptr`. **Keep `cancelAll`/drop main-thread-only**; the destructor
  assert/atomic write is owned by WI-5 (P1a) for file-disjointness.
- **Parity:** upload stays main-thread GL; pins unchanged; version-stamp stale-drop unchanged
  (`WorldRenderer.cpp:760-765`).
- **Risks:** do not reorder near-vs-ring capture loops (`WorldRenderer.cpp:792-826`).
- **Verification:** compile + ctest (`chunk_mesh_golden_test`, `chunk_map_test`). New
  `tests/mesh_cancel_test.cpp`: enqueue 1000 jobs, `cancelAll()` returns <50 ms, every affected builder's
  `meshJobInFlight` cleared, completed jobs still drain in order.
- **Blocked-by:** WI-2 (Compute count exists).

#### WI-5 — Confine GL state writes to the main GL thread (alphaTestRef + GLCore init) — PASS-1, P1a
- **Objective:** kill the worker→main GL-state data race (rank #4, HZ-14). The one outright bug in the dual-path inventory (parity-bindings D6).
- **Files & change:**
  - `mod/model/ModModels.cpp:626` — stop calling `core::setAlphaTestRef(0.1f)` from mesh workers
    (`drawLuaBlockWorld` runs inside `ChunkBuilder::buildMesh` on Compute workers). Capture the alpha ref
    into the job snapshot at capture time (`ChunkBuilder.cpp:170-196`, where `lightLevelToLuminance`/
    `blockRenderLayers` are already snapshotted). **Default must stay 0.1** (QD-20).
  - `client/render/chunk/ChunkMeshJob.hpp` — add `float alphaTestRef = 0.1f;` field; read from snapshot in tessellation path.
  - `client/render/RenderCore.cpp:70,479-481,671-680` — `g_alphaTestRef` becomes main-thread-write-only:
    `TL_DOMAIN`/`ASSERT_MAIN_THREAD()` guard in `setAlphaTestRef` (Debug).
  - `client/gl/GLCore.cpp:121,141-145` — `g_loaded` plain bool → `std::once_flag`/atomic (HZ-15).
  - `client/render/chunk/ChunkBuilder.cpp:220-225` — add the **R3 main-thread-destruction assert/atomic**
    on the `meshJobInFlight` write (owned here for file-disjointness with WI-4; see WI-4 note).
- **Parity:** no visible change; removes UB. Mod blocks rely on global 0.1 → snapshot default 0.1 preserves.
- **Risks (B-F8):** mesh/compute workers must NEVER call Lua or enter `ModHost`; all `luaHook*` stay
  main-thread; `luaHookRenderFrame` (`Minecraft.cpp:703`) stays in the render phase.
- **Verification:** compile + ctest (`chunk_mesh_golden_test` mod-free). `gl_state_affinity_test` is **not**
  a GTest — it is a **compile-fixer grep checklist**: `setAlphaTestRef` has `TL_DOMAIN` guard; no
  `setAlphaTestRef` call remains in `ModModels.cpp` worker path.
- **Blocked-by:** none (grouped with WI-3 in P1a; uses the WI-3-era snapshot capture path).

#### WI-9 — Network packet-path correctness (accounting, read cap, verify result-only, single drain) — PASS-1, P1c
- **Objective:** eliminate N-connection static data race (H4), unbounded inbound queue (B-F6/D2 — now
  committed), cross-thread disconnect (H5), double-drain (H7) — without changing the 2-thread + sim-apply model.
- **Files & change:**
  - `network/Packet.hpp:79-81,121-128` — move `packetTrackers()`/`incomingCount()` mutation out of reader
    threads → per-`Connection` counters merged on the game thread (HZ-17).
  - `network/Connection.cpp:184-228` (`tick`) — **committed read-side cap (Q8/D2):** bounded `readQueue_` +
    `disconnect.overflow` mirrored on the existing `0x100000` send cap (:185-187). Keep send-overflow in `tick()`.
  - `server/network/ServerLoginNetworkHandler.cpp:149-178` — verify thread publishes `deferredLoginPacket_`
    under `verifyMutex_` (:169-170) **only**; all `Connection`/`closed` mutation moves to the server tick
    thread (HZ-18, `:64`, `:172`, `:175`). Remove the join in `verifyUsernameOnline` (:150-151).
    Per-connection **in-flight flag on the Io domain** so no two verifies run for the same connection (QD-14).
  - `client/multiplayer/ClientNetworkHandler.cpp:243-245` — stop joining the prior `joinServerThread_` to
    reuse it (HZ-06); result polled via `processPendingJoinServer` (:55-83).
  - Double-drain (H7): one drain per `Connection` per tick; `DownloadingTerrainScreen.hpp:19-26` drain
    collapses into `ClientWorld::tick` (:39-69); keep-alive moves to `ClientNetworkHandler::tick` (QD-15).
- **Drain policy (QD-04):** keep time-box (3 ms / min 8 / max 4096, `Connection.cpp:198-221`) +
  per-connection cap; ≤100/player floor. Keep `interrupt()` after every drain; keep data-before-chunk
  (`Connection.cpp:293-316`); keep the loose chunk-packet rate gate (`preferImmediate`, :296-312) as the
  accepted approximation (QD-26). **Do NOT move `apply` off-thread.**
- **Parity:** preserves Java drain-on-sim-thread, 100-packet-flavor fairness, send-overflow `disconnect.overflow`.
- **Risks:** FIFO order must hold (`onPlayerMove` echo vs teleport, `ServerPlayNetworkHandler.cpp:149-155`).
- **Verification:** compile + connection/mp tests (`connection_packet_drain_throughput_test`,
  `chunk_packet_drain_test`, `connection_listener_test`, `server_login_handler_test`, `packet_roundtrip_test`,
  `mp_chunk_delivery_test`, `mp_chunk_data_test`, `mp_parity_updates_test`). New
  `tests/packet_accounting_test.cpp` (2+ connections reading concurrently → no torn counters).
- **Blocked-by:** WI-1 (Lifecycle) by lane order.

#### WI-10 — Async socket teardown; never join on the game thread; NetIo registration — PASS-1, P1c
- **Objective:** fix the single biggest parity/responsiveness deviation — `disconnect()`/`joinThreads`
  joining reader+writer on the caller's stack (H1, HZ-05). Java uses async `NetworkMasterThread`.
- **Files & change:**
  - `network/Connection.cpp:134-137,157-160` — `disconnect()` becomes: CAS `open_` false,
    `shutdown(SD_BOTH)` (unblocks `recv`), cancel read interest, flush remaining writes with a short grace,
    return immediately. Destruction deferred off the drain stack via the existing retire pattern
    (`Minecraft.cpp:778-788` / `MultiplayerSession.cpp:12-19`) + Lifecycle-registered joins
    (PASS-2 WI-13). **Never join on the game thread.**
  - `network/Connection.hpp:113-114` — `reader_`/`writer_` → registered `Domain::NetIo` with
    `reserveDynamic(2)` + unblock-then-watchdog-join (Q9: **blocking NetIo pool first; event loop deferred**).
  - `server/network/ConnectionListener.cpp:99-154` — per-tick budget split with per-connection caps.
- **Re-entrancy (H10):** `ServerPlayNetworkHandler::disconnect` from inside a packet handler stays deferred
  off the drain stack. Reader cancellable via `shutdown(SD_RECEIVE)` (`Connection.cpp:341`).
- **Parity:** Java never joins socket threads on the sim thread (`NetworkManager.java:160-189`).
- **Risks:** double-close (`Connection.cpp:345-351` `shutdownSocket` must stay idempotent); retire must not
  destroy a `Connection` still referenced by a pending handler.
- **Verification:** connection/mp tests as WI-9. New `tests/connection_async_teardown_test.cpp`:
  connect to a black-holed peer, `disconnect()` returns <250 ms, later flush completes.
- **Blocked-by:** WI-9, WI-1 (Lifecycle).

#### P-FOGMODE — fogMode GL-constants 0/9729/2049 (atomic 4-file) — PASS-1, P2a
- **Objective:** HIGH pack-visible parity fix (parity-bindings F-1): C++ sends internal 1/2/3; Java Iris sends
  GL constants `0/GL_LINEAR(0x2601=9729)/GL_EXP2(0x0801=2049)`. Change **atomically**.
- **Files & change:**
  - `client/render/uniforms/FrameData.cpp:237` — `values.fogMode = fog.enabled ? (fog.mode == 1 ? 0x2601 : 0x0801) : 0`.
  - `client/render/RenderCore.cpp:464` — same mapping for the per-draw upload
    (`fog.enabled ? (fog.mode == 1 ? 0x2601 : 0x0801) : 0`). `g_fog.mode` values stay internal (1/2/3,
    `RenderCore.cpp:782-803`) — only the **uploaded** value changes.
  - `client/render/shaders/CustomUniforms.cpp:343` — `return i1(frame.fogMode)` now returns the GL constant.
  - `shaders/vanilla/shaders/lib/common.glsl:18-26` — interpret `0/9729/2049` (rewrite the function body;
    `fogMode == 9729` → linear branch, `fogMode == 2049` → exp2; the current `==1/2/3` and the **false
    comment "as Iris reports them"** go away).
- **Keep:** `fogShape` OFF→−1/ON→1 (`FrameData.cpp:239`, `RenderCore.cpp:465`) — unchanged.
- **Parity:** now matches Java Iris numerically. `uniforms/Uniforms.cpp:83` (`set1i("fogMode", …)`) needs no
  change — the value is transformed at the producers.
- **Risks:** a pack already special-casing 1/2/3 will flip; that is the point of the fix (Q3). Verify
  RenderPearl/vanilla/SEUS PTGI still render identically for the default (linear) path.
- **Verification:** compile + ctest; **new `tests/fog_mode_parity_test.cpp`**: FrameData/RenderCore producers
  emit 0/9729/2049 for off/linear/exp2; CustomUniforms `fogMode` matches; vanilla `common.glsl` decodes both.
- **Blocked-by:** none (runs after P1 lanes; touches `RenderCore.cpp:464` only, disjoint from WI-5's :70/:479-481/:671-680).

#### P-LITGATE — Lighting-ready gate (double-mesh fix), split out of WI-7 — PASS-1, P2a
- **Objective:** mark a column "lit" only after its propagation boxes drain; hold first mesh until lit
  (HZ-32 / QD-05, committed in Q11). Pure-wins world-load mesh churn reduction.
- **Files & change:**
  - `world/World.cpp:711-717` (`doLightingUpdates`) — when a drained `DirtyRegion` is applied
    (`events_.setBlocksDirty`), mark the affected columns "lit" in the chunk renderer's gate state.
  - `client/render/world/WorldRenderer.cpp:199-205` (`enqueueDirtyChunk`) — hold the **first** build of a
    freshly-created column until its column is marked lit (re-check the lit stamp in the capture loop
    `:791-826`); do not hold re-meshes of already-lit columns.
  - `client/render/world/WorldRenderer.cpp:1117-1140` (`markDirty`) and `:1189-1191` (`setBlocksDirty` chains) —
    the gate only delays the *first* mesh; subsequent dirt stays immediate.
- **Parity:** none (optimization; meshing volume changes, not correctness).
- **Risks:** a column whose lighting never drains must not stall forever — add the "lit" stamp as a
  non-optional completion (columns are lit when their final box is drained; a boxed region with no boxes is
  lit immediately).
- **Verification:** compile + ctest; **new `tests/lighting_ready_gate_test.cpp`**: a freshly-lit column is
  not meshed until its boxes drain; once lit it meshes normally; a column with no pending boxes is never held.
- **Blocked-by:** WI-4 (mesh scheduler shape) conceptually; runs in P2a after P1 lanes. **Outbox-safety note:
  the gate marks columns lit after `doLightingUpdates` drains; `drainDirtyRegions` holds `outboxMutex_`
  only for the copy (`LightingEngine.cpp:113-126`) and releases before the lit-stamp is applied, so
  P-LITGATE cannot deadlock the LightingEngine outbox.**

#### P-IFENGINE — Unify the two `#if`/`#define` engines into one state machine — PASS-1, P2b
- **Objective:** DP-8 (parity-glsl §3): the GLSL engine (`SourceProcessor.cpp:184-311`, CondFrame stack) and
  the `.properties` engine (`Loader.cpp:307-435`, activeStack/matchedStack) evaluate the same directives with
  two different conditional-state machines; a fix to one can silently diverge the other. **Unify.**
- **Files & change:**
  - New shared header (suggest `client/render/shaders/ConditionalState.hpp`) holding ONE
    conditional-stack implementation (push-if / elif / else / endif with matched-tracked active semantics).
  - `client/render/shaders/SourceProcessor.cpp:192-270` — `normalizePackSource` uses the shared machine.
  - `client/render/shaderpack/Loader.cpp:353-416` — `preprocessProperties` uses the same machine.
  - Both keep calling the existing `evaluateIfExpression` (`PreProcessor.hpp:18`, `PreProcessor.cpp:421`) —
  **do not add a third parser** (parity-glsl §5.12).
- **Parity:** identical `#if` outcomes for every shipped pack (regression-locked by `glsl_snippets_test`,
  `shader_pack_loader_test`, `shader_frame_data_test`).
- **Risks:** subtle `#elif`/`#else` matched-track differences between the two old machines — the unified
  machine must replicate BOTH current behaviors (GLSL: parent&&condition on push; properties: matchedStack
  semantics). Golden-compare both engines' outputs on all shipped packs before/after.
- **Verification:** compile + shader tests; **new `tests/if_engine_unified_test.cpp`**: feeds the same
  directive text to both engines and asserts identical output; asserts GLSL and `.properties` produce
  identical results for a battery of `#if/#elif/#else/#endif/#ifdef/#ifndef/#define/#undef` cases.
- **Blocked-by:** none.

#### P-MACROS — Full Java-textual macro set (Q6) — PASS-1, P2b
- **Objective:** match Java Iris `StandardMacros.java` textually for the missing macros — **including**
  capability macros the engine may not implement (Q6: full set). Bump the disk-cache format.
- **Files & change:**
  - `client/render/shaders/SourceProcessor.cpp:709-778` (`versionPreamble`): define **unconditionally**
    `MC_NORMAL_MAP`, `MC_SPECULAR_MAP` (Java `StandardMacros.java:101-102`; currently only LabPBR packs at
    :756-760), `MC_RENDER_QUALITY`/`MC_SHADOW_QUALITY` = `1.0` (:103-104), `IRIS_HAS_TRANSLUCENCY_SORTING`
    (:60), `IRIS_TAG_SUPPORT` (:61, =2), `IRIS_REQUIRES_SEPARATE_ENTITY_DRAWS` (:58).
    `kCategories` (:745-748): add `CAT_MOUNTAIN`/`CAT_UNDERGROUND` to reach Java's 19 with the index fix
    (Java `BiomeCategories.java` has 19; C++ 17).
  - `client/render/shaders/PreProcessor.cpp:469-507` (`seedMacrosFromDefines`) — the preprocessor-side
    table mirrors the same additions so `#if defined(MC_NORMAL_MAP)` in source evaluates identically.
  - `client/gl/ShaderBinaryCache.cpp:8-9` — bump `kFileVersion = 1` → **2** (parity-glsl §5.4; mandatory for
    any macro change).
  - **`MC_GLSL_VERSION` stays pack-version semantics THIS pass** — switching it to the *driver's* GLSL
    version (M1) needs the WI-8 GL snapshot; that part of the fix lands in **PASS-2 WI-15**.
- **Parity:** matches Java textual set (Q6). No source-behavior change when the active pack uses none of these
  macros (cache bump isolates).
- **Risks:** `IRIS_REQUIRES_SEPARATE_ENTITY_DRAWS`/`IRIS_TAG_SUPPORT` are now **defined** but the engine
  doesn't implement those capabilities (Q6 explicitly chose this). A pack that enables a code path the engine
  can't feed may misrender — accepted per user decision; document in the code comment.
- **Verification:** compile + shader tests (`glsl_snippets_test`, `shader_gl_integration_test`,
  `shader_pack_loader_test`, `custom_uniforms_test`); **new `tests/macro_parity_test.cpp`** asserts the full
  textual macro set matches `StandardMacros.java` (fixture of the expected names/values) and that the cache
  format version is bumped.
- **Blocked-by:** none. If it proves too large for this pass, **defer wholesale to PASS-2 WI-15** (the plan
  treats P-MACROS as "pass 1 if cheap").

### 4.2 PASS-2 items (planned for the later pipeline run — do NOT schedule into the 7 lanes)

#### WI-6 — Lighting onto Compute as a pinned sub-pool; outbox → Channel — PASS-2
- **Status:** implemented. Lighting submits bounded, high-priority per-claimed-box tasks to shared `Domain::Compute`, capped by `computeShare(3)`; box-local `WorkerState` preserves render pins without reserving long-lived pool threads. Stop admission and task retirement remain serialized by `queueMutex_`; same-type box conflicts are unchanged.
- **Outbox:** `Channel<DirtyRegion>` capacity 4096; a full channel coalesces one queued region with the new region, retaining dirty coverage. `World::doLightingUpdates` remains unchanged.
- **Touched:** `src/net/minecraft/world/light/LightingEngine.hpp`, `src/net/minecraft/world/light/LightingEngine.cpp`, `tests/lighting_channel_test.cpp`, `CMakeLists.txt`.
- **Verification pending:** final compile pass via `build-omega.ps1`; focused `lighting_channel_test`, `server_world_events_test`, and `unified_light_registry_test`. No compile/tests run by this executor.

#### WI-7 — Chunk loader/save onto domains; fix PendingLoad silent-drop; generator clone pruning — PASS-2
- **Status (PASS-2 implementation): COMPLETE, verification pending.** Private loader/save pools are removed.
  Loads submit to shared Compute with per-request generation cancellation; synchronous takeover, unload, and
  center changes explicitly cancel stale work. Saves use one FIFO drain scheduled on shared Io. Destruction
  waits only owned in-flight counts, then prunes per-thread generator clones. Decoration remains main-thread.
- **Files:** `world/chunk/ChunkCache.cpp:213-219` (loader → shared Compute), `:221-225` (save → Io, 1),
  `:226-249` (`requestChunkAsync` — `PendingLoad` gains epoch/generation token so cancel marks them cancelled),
  `:251-295` (`integrateFinishedLoads` drops cancelled, budget from the shared frame deadline in WI-12b),
  `:85-105` (`workerGenerators_` per-thread clone map — prune on world switch, HZ-36). `ioMutex_` discipline preserved.
- **Parity:** `decorate` stays main-thread (:380-391); remote-client worlds packet-driven only (RULES §10).
- **Verification:** `server_chunk_cache_test`, `chunk_map_test`, host tests; new `tests/chunk_cancel_test.cpp`.
- **Blocked-by:** WI-2, WI-4 (pool shape), WI-6.
- **NOTE:** the lighting-ready gate has been split out to **PASS-1 P-LITGATE** (Q11) — do not re-implement it here.

#### WI-8 — Shader compile count via GlCompile; frame-driven poll; GL-snapshot derivation seam — PASS-2
- **Status:** implemented. **Touched:** `ShaderCompileService.cpp`, `ShaderProgram.cpp`, `ProgramCache.*`, `Compiler.*`, `Pipeline.*`, `Manager.cpp`, `GlState.cpp`, `SourceProcessor.*`. **Verification:** pending final compile/fast focused checks; slow server-boot/integration/manual checks deferred.
- **Files:** `client/gl/ShaderCompileService.cpp:47` (→ GlCompile count, cap 2), `:144-177` (`compileBlocking`
  stays only for the `!started()` fallback :157-159; normal path = submit + frame poll),
  `client/gl/ProgramCache.cpp:159-229` (poll from the `compileDone` channel each frame),
  `client/render/pipeline/Pipeline.cpp:695-702` (`programFromPackSync` dead — delete or keep ONLY as fallback,
  byte-identical), `client/render/shaders/GlState.cpp:238-273` (non-synchronized `static bool initialized` →
  fixed by the GL snapshot).
- **GL-snapshot before any worker (parity-glsl §4.1):** capture `glVersionMacro`/`maxColorBuffers`/
  `driverPreamble`/`supportedGlExtensions` once on the GL thread into a snapshot struct; `prepareProgram`/
  `prewarm` read only the snapshot. Derivation caches (`PackInstance::sourceCache`/`compiledPrograms`/
  `programEnabledCache`/`customUniforms`) stay main-thread or become per-request-pure.
- **Worker `ShaderProgram::destroy()` writes `s_lastBoundProgram` (`ShaderProgram.cpp:14,212-213`; worker-side
  local at `ShaderCompileService.cpp:200`)** — flag/guard (parity-glsl §4.5).
- **Verification:** shader tests (`shader_gl_integration`, `shader_frame_data`, `glsl_snippets`, `shader_pack_loader`, `custom_uniforms`).
- **Blocked-by:** WI-2.

#### WI-11 — Ephemeral / ad-hoc threads → Io domain (kill fire-and-forget) — PASS-2
- **Files:** `client/texture/ImageDownload.cpp:7-18` (delete `.detach()`; texture Channel; main applies, HZ-16),
  `client/session/SessionValidator.cpp:17-51` (→ Io task, HZ-28), `client/diagnostics/ClientDiagnostics.cpp:235-264`
  (watchdog registered, not detached), `client/multiplayer/MultiplayerConnector.cpp:17-61,63-68` (never
  `bridge->disconnect()`→`setWorld` from the connector thread — HZ-25; publish under `mutex_`, main applies),
  `client/resource/ResourceDownloadThread.cpp:85-120,138,168` (worker produces files/bytes only; main applies, HZ-24),
  `client/gui/auth/LoginScreen.cpp:98-106,125-134,260-271`, `client/auth/microsoft/SessionRestore.cpp:55-92,163-175,237-268`
  (HTTP on Io; no mid-flight join), `world/World.cpp:300-339` (level.dat `std::launch::async` → Io pool with a
  **value** snapshot — HZ-20/QD-13; `save(true)` still waits, H9), `server/dedicated/gui/DedicatedServerGui.cpp:61-82,153-161`
  (`guiThread.detach()` + `stopped` plain-bool → registered thread + atomic, HZ-22).
- **Parity:** Java downloads on background threads too; ownership discipline only.
- **Verification:** `handshake_metadata_test`, `integrated_server_host_test`, `lan_host_coordinator_test`,
  `multiplayer_client_player_entity_test`; new `image_download_channel_test`, `multiplayer_connector_test`.
- **Blocked-by:** WI-1 (Io pool).

  - **Completed slices (PASS-2):** `ImageDownload` now submits URL fetch/decode to `Domain::Io` with a shared `Channel`; `TextureManager` applies the completed CPU image only on its caller/main path before upload or skin inspection. `SessionValidator` submits a copied `Session` to `Domain::Io`; its sole completion effect remains the atomic failure timestamp. Saved Microsoft-account startup restore now runs in `Domain::Io`, with its result retained under the existing mutex and applied only by `tickRestoreSavedAccount`; cancellation no longer joins a worker. `MultiplayerConnector` has an Io-owned result state; main-thread `poll()` applies the authenticated session, handshake, handler message, and bridge adoption, while cancellation never invokes bridge disconnect on the Io task. `ResourceDownloadThread` now has Io-owned download/file discovery with a bounded `Channel`; `Minecraft::tick()` applies resources. `LoginScreen` now uses per-request shared Io work states/channels; the screen consumes results and cancellation only signals its work state. The client hang watchdog is a registered `std::jthread`, stopped by `disarmHangWatchdog`; dedicated-server `stopped` is atomic.
  - **Completed follow-through:** `World` non-blocking level save now submits an immutable properties/player-NBT
    value snapshot to `Domain::Io`; blocking save and destruction wait on the owned state. `DedicatedServerGui`
    now owns a registered `jthread`, supports explicit shutdown, and no longer detaches or terminates the process.
  - **Touched files:** `src/net/minecraft/client/texture/ImageDownload.hpp`, `src/net/minecraft/client/texture/ImageDownload.cpp`, `src/net/minecraft/client/texture/TextureManager.cpp`, `src/net/minecraft/client/session/SessionValidator.cpp`, `src/net/minecraft/client/auth/microsoft/SessionRestore.hpp`, `src/net/minecraft/client/auth/microsoft/SessionRestore.cpp`, `src/net/minecraft/client/multiplayer/MultiplayerConnector.hpp`, `src/net/minecraft/client/multiplayer/MultiplayerConnector.cpp`, `src/net/minecraft/client/resource/ResourceDownloadThread.hpp`, `src/net/minecraft/client/resource/ResourceDownloadThread.cpp`, `src/net/minecraft/client/gui/auth/LoginScreen.hpp`, `src/net/minecraft/client/gui/auth/LoginScreen.cpp`, `src/net/minecraft/client/Minecraft.cpp`, `src/net/minecraft/client/diagnostics/ClientDiagnostics.cpp`, `src/net/minecraft/server/MinecraftServer.hpp`.
  - **Verification pending:** compile-fixer must run `build-omega.ps1` plus focused fast unit/static checks; `image_download_channel_test` remains to add. Server-boot, integration, and manual host/multiplayer smoke tests are deferred by test policy.

#### WI-12b — Run/render rewiring + unified frame budget (gated) — PASS-2
- **Status (PASS-2 implementation): COMPLETE, verification pending.** `Minecraft::run` now executes the six
  `FramePipeline` phases; mailbox drains and `FrameProfiler` instrumentation live at the phase boundary.
  Present-before-draw remains fixed. Inactive pacing and diagnostics are separate phases. One shared 16 ms
  deadline now bounds connection drain, near/distant mesh budgets, and chunk integration; integration moved
  after render and still guarantees one completed item. The two mutex/file stall tracers were removed.
- **Gate: WI-4, WI-6, WI-7, WI-8, WI-9, WI-10 all landed.** Every drain has a channel; the mailbox drains them.
- **Files:** `client/Minecraft.cpp:762-843` (`run()` → `FramePipeline::run()`), `:670-761` (`runRenderPhase`),
  `:742-743` + `:827-828` (two stallMutex blocks → `FrameProfiler`), `:658-661` (`pendingScreenResize` stays
  inside `tick()`), `:714-721` (inactive sleep, Phase 3), `:694` then `:708` (present-before-draw preserved —
  QD-07). `GameRenderer.cpp:1195` (`compileChunks` stays), `:711` (`shaderPacks_->poll()` stays).
  Budget consolidation: `FrameBudget::deadline` per frame; `WorldRenderer.cpp:742-743,787-789`,
  `ChunkCache::integrateFinishedLoads`, `Connection::tick` (3 ms) read `frame.remaining()`. **Yield order
  (QD-17):** network drain ≥ near-mesh ≥ lighting ≥ distant mesh ≥ integrates.
- **Parity:** call-site-relative order is the contract (not byte equality — audit-B F2/F3).
- **Verification:** new `tests/frame_pipeline_order_test.cpp` asserts present-before-compileChunks-before-poll;
  `client_timer_test`, `render_settings_test`, `chunk_mesh_golden_test`, `draw_camera_state_test`.
- **Blocked-by:** the full gate.

#### WI-13 — Lifecycle teardown wiring — PASS-2
- **Status (PASS-2 implementation): COMPLETE, verification pending.** `Minecraft::stop` now delegates ordered,
  deadline-bearing owner shutdown to `Lifecycle`: shader compile precedes render-resource release; server process,
  mod runtime, audio, input, and shared worker domains precede display destruction. The old detached lifecycle
  watchdog was removed; owners must honor the supplied deadline. Crash entry stops the run loop. World async save
  and the dedicated GUI thread now have explicit owned completion paths.
- **Files:** `client/Minecraft.cpp:347-393` (`stop()` → `Lifecycle::shutdown()`; keep `ShaderCompileService::stop`
  :354 before `DisplayManager::destroy` :388; keep `std::_Exit(0)` :391 fallback), `:248` (`gameCrashed`),
  `:860` (post-loop `stop()`), `client/host/ServerProcessCoordinator.cpp:279-290` (10 s wait), 
  `util/logging/Logging.cpp:129-177` (register for unblock/watchdog only, **never join** — QD-06/A-11).
  Each blocking owner (ChunkCache drain `ChunkCache.cpp:27-36,356-362`, Connection join, audio, coordinator)
  takes a deadline/stop_token so the watchdog can fire (audit-B F4). GL-context-affine workers never leak
  before `DisplayManager::destroy`.
- **Verification:** `lifecycle_test.cpp`, host/server tests.
- **Blocked-by:** WI-8, WI-11, WI-12b.

#### WI-14 — Apply the coordinator to the dedicated server binary (deferred, Q5) — PASS-2
- **Status (PASS-2 implementation): COMPLETE, verification pending.** `server-main` now configures the shared
  coordinator and runs the server directly; the accept loop is a registered `jthread`; command input is a
  registered Io-domain owner; online username verification uses the shared Io pool and a bounded main-thread
  result channel. Existing per-connection NetIo registration remains intact. No server-boot test was run.
- **Files:** `src/server-main.cpp:105-126`, `server/MinecraftServer.cpp:196-214` (commandThread_), `ConnectionListener`
  accept thread (:28), per-connection threads. Same budget + registration.
- **Verification:** compile server target + `minecraft_server_tick_test`, `player_manager_test`,
  `server_command_handler_test`.
- **Blocked-by:** WI-2 (optional).

#### WI-15 — Iris parity-fix batch remainder (macros not in P-MACROS + small fixes) — PASS-2
- **Status (PASS-2 implementation): COMPLETE, verification pending.** `MC_GLSL_VERSION` now comes from the
  immutable driver GLSL snapshot rather than the emitted pack dialect; first-frame previous projection/model-view
  matrices are identity; custom-uniform `min`/`max` fold 2–16 scalar, integer, or vector arguments; shader binary
  cache epoch is 4.
- **Verified:** omega client/server build; focused fast suite; real-GL compatibility compile and RenderPearl every
  program in root/overworld/nether/end. Server boot/manual multiplayer tests intentionally not run.
- **Files:** `client/render/shaders/SourceProcessor.cpp:737` (`MC_GLSL_VERSION` → driver GLSL version from the
  WI-8 GL snapshot, M1; bump cache), `client/render/uniforms/FrameData.cpp:660-665` (first-frame gbufferPrevious
  = **identity** per `MatrixUniforms.java:66-85`, F-6), `client/render/shaders/CustomUniforms.cpp:599-620`
  (variadic `min`/`max`, U1 — 3–16 args; additive), optional `sildursWaterFract` micro-patch (T9).
  If P-MACROS was deferred (too big), the full macro batch lands here instead.
- **Verification:** shader tests; no source-behavior change for packs using none of these.
- **Blocked-by:** WI-8 (GL snapshot for M1).

#### WI-16 — HZ-08: `downloadPendingMods` off the main thread — PASS-2
- **Status (PASS-2 implementation): COMPLETE, verification pending.** Session join-auth and mod download now
  use shared-state Io tasks with bounded channels. HTTP/zip validation/temp-file production occurs on Io;
  `ServerModDownloadScreen::tick` consumes progress; final overwrite/rescan and login continuation occur on the
  main thread. Cancellation owns no raw worker reference and performs no join. Manual server-download smoke is
  deferred per user test policy.
- **Files:** `client/multiplayer/ClientNetworkHandler.cpp:258-320` — staged Io-pool download (HTTP → zip →
  parse → install) with progress to `ServerModDownloadScreen` via a channel; main thread applies only the final
  install/rescan. **Declare** `TextureManager::getTextureId` decode/upload (`TextureManager.cpp:451-483`) and
  the stat-save on `setScreen` (`ScreenStack.cpp:33-38`) **main-thread by decision** (audit-A FINDING 9).
- **Verification:** host tests; manual join-server-mod-download smoke (compile-fixer runs the app once).
- **Blocked-by:** WI-1 (Io pool).

#### P-GLSL330 — Full GLSL 330-core migration (Q2), incl. T2/T3 — PASS-2
- **Status (PASS-2 implementation): COMPLETE, verification pending.** Raster source normalization has one
  330-core target and no 120/modern fork. Vertex `attribute`/`varying`, legacy built-ins, texture/shadow calls,
  fragment fog/color/texcoord, and literal fragment outputs are rewritten in the shared preparation seam.
  Fragment outputs receive explicit locations, compatibility alpha reads `iris_FragData0`, raster remains ≥330
  while retaining explicitly requested higher feature levels, compute remains ≥430,
  and the default composite bridge publishes the rewritten legacy varyings.
- **Verified:** ten source-transform cases, macro/cache/frame suites, compatibility GL compile, and RenderPearl
  full-program/full-dimension GL compile all pass.
- **Files & change:**
  - `client/render/shaders/SourceProcessor.cpp:535-544` (`sourceCompilesAsModern`) — becomes unconditional
    modern; the 120 branch in `lowerVertexSource` (`SourceProcessor.cpp:569-639`) and `lowerFragmentSource`
    (`SourceProcessor.cpp:655-692`) is removed. Do NOT touch `programGetsCompatAlphaTest`/
    `fragmentWritesLegacyFragOutput` (`:641-654`) — they gate alpha-test injection, not dialect.
  - Add fragment rewriting (T2/T3, Java `CommonTransformer.java:182-232`): `gl_TexCoord`→`irs_texCoords` +
    `in vec4 irs_texCoords[3];`, `gl_Color`→`irs_Color` + `in vec4 irs_Color;`; `gl_FragColor`→`gl_FragData[0]`,
    `gl_FragData[i]`→`layout(location=i) out vec4 iris_FragDatai`.
  - `client/render/shaders/SourceProcessor.cpp:709-778` (`versionPreamble`) — emit `#version 330 core` (compute
    keeps ≥430); `#define MC_GLSL_VERSION` = driver GLSL (or leave to WI-15 — sequence: P-GLSL330 after WI-15's M1).
  - Upgrade port snippets to 330 forms (`src/net/minecraft/client/render/shaders/glsl/*.glsl`: `iris_fog_frag_coord_*`,
    `iris_front_color_global`, `alpha_test_discard`, `iris_lightmap_matrix`, `compat_alpha_check`,
    `default_composite.vsh` `SourceProcessor.cpp:518-520`).
  - `client/gl/ShaderBinaryCache.cpp:8-9` — bump cache format again (≥3).
- **Parity:** unifies on Java 26.1's single 330-core path. This is the port's largest deliberate-divergence
  reversal; verify **every shipped pack** (RenderPearl, SEUS PTGI, vanilla) compiles + renders identically.
- **Risks:** legacy OptiFine packs that were written for 120 break (the historical reason for the fork); the
  `const …Format` strip (`IncludeResolver.cpp:10-22`, on at `Compiler.cpp:142`) must stay — it is required for
  SEUS PTGI `deferred10.fsh:18`.
- **Verification:** full shader suite; new `tests/glsl330_rewrite_test.cpp` (fragment rewrite battery:
  `gl_TexCoord`/`gl_Color`/`gl_FragColor`/`gl_FragData[i]` cases).
- **Blocked-by:** P-MACROS, WI-8 (snapshot), WI-15 (M1 ordering).

#### P-ENTITYOVERLAY — Full entity-overlay path (Q3) — PASS-2
- **Status (PASS-2 implementation): COMPLETE, verification pending.** Program preparation now carries one
  cross-stage transform context. Entity-capable programs derive `entityColor` from reserved-unit-2 `iris_overlay`,
  pass color and integer entity information through vertex/tessellation/geometry/fragment stages, and retain the
  existing uniform upload additively. The vertex ABI now supplies integer overlay UV, light UV, and per-draw entity/
  block-entity/item IDs; `LivingEntityRenderer` publishes hurt/tint color before the entity draw. Units 0/1/2 are
  fixed reservations and optional pack textures begin at 3.
- **Verified:** source-stage context battery, integer ABI compilation, omega client/server link, and RenderPearl
  full-program GL compile pass. Manual hurt-flash visual smoke intentionally deferred.
- **Objective:** add the Java `EntityPatcher`-equivalent: `iris_overlay` sampler + `iris_Entity` ivec3 +
  `iris_OverlayUV`/`iris_LightUV` vertex inputs + `entityColor` **derived from the overlay texel** (parity-bindings F-4).
- **Files & change:**
  - Source transform: `client/render/shaders/SourceProcessor.cpp` `lowerVertexSource`/`lowerFragmentSource`
    (+ optional `EntityPatcher.cpp`) — for entity programs: delete the `uniform vec4 entityColor;` declaration
    (Java `EntityPatcher.java:20-21,39`), inject `uniform sampler2D iris_overlay;`, `out vec4 entityColor;`,
    `out vec4 iris_vertexColor;`, `in ivec2 iris_UV1;` (vertex, `EntityPatcher.java:45-49`), prepend
    `vec4 overlayColor = texelFetch(iris_overlay, iris_OverlayUV, 0); entityColor = vec4(overlayColor.rgb, 1.0 - overlayColor.a);`
    + the `entityColor.rgb *= float(entityColor.a != 0.0)` workaround (:54-62); fragment gets
    `in vec4 entityColor;` + `in vec4 iris_vertexColor;` (:107-109). `entityId`→`iris_entityInfo`/`iris_Entity`
    rewrite (:124-204). Geometry/tess pass-through per `EntityPatcher.java:63-121`.
  - Vertex data: `client/render/Tessellator.hpp:8-22` (`TessellatorVertex`) — add overlay-UV + light-UV fields;
    feed from `client/render/entity/LivingEntityRenderer.cpp:206-240` (the existing overlay color draw passes
    already compute overlay colors at :230-234 — the UVs come from the same `getOverlayColor` source, defined at :310).
  - Binding: `client/render/shaders/WorldProgramBinder.cpp:67-156` — reserve the overlay texture unit
    (Java `WORLD_RESERVED_TEXTURE_UNITS={0,1,2}`, `IrisSamplers.java:36-37`), bind the white-pixel fallback
    (`IrisSamplers.java:209-211`); `client/render/RenderCore.cpp:839-877` attribute layout + new slots.
  - Keep the uniform upload (`RenderCore.cpp:453-458,528-543,903-908`) — the overlay path is **additive**.
- **Parity:** packs reading `entityColor` in fragment now get overlay-derived values like Java Iris.
- **Risks:** entity programs are many (`gbuffers_entities`/`basic`/`textured`/`line`/`hand`); the transform must
  be gated to the exact program set Java patches; verify a pack that reads `entityColor` in fragment.
- **Verification:** new `tests/entity_overlay_test.cpp` (source transform battery); manual entity-hurt-flash
  smoke with a pack reading `entityColor` (compile-fixer runs the app once).
- **Blocked-by:** P-GLSL330 (dialect unification) is the clean base; can land after or with it.

---

### 4.3 PASS-3 — 330-core collapse and hardening

- **Status:** COMPLETE.
- `SourceProcessor.cpp` is reduced from 1,030 lines to a 30-line orchestration seam. GLSL source lexing,
  conditional normalization, immutable driver/macro environment, 330-core lowering, and entity-overlay
  patching now have independent translation units and narrow headers.
- Legacy `attribute`/`varying` handling is confined to the input normalization seam. Internal declaration
  discovery now accepts only core `in`/`out`; the old alternate-storage branch is deleted.
- Raster preamble version negotiation now considers vertex, fragment, geometry, tess-control, and
  tess-evaluation sources together. Raster floor is 330, tessellation floor is 400, compute floor remains
  430, and any higher pack-declared version is retained.
- Storage-qualifier lowering is global-declaration-aware instead of rewriting every identifier token.
- RenderPearl black-output follow-up fixed `parsePackProperties`: encountering `profile.*` no longer stops
  the property scan, so required features, custom images/textures, and SSBO declarations after profiles are
  retained. Compute dispatch now passes the active `PackDefinition` into sampler binding, restoring
  `SEPARATE_HARDWARE_SAMPLERS` compare state. Regression coverage asserts RenderPearl's two custom images,
  `areatex`/`searchtex`, and required hardware-sampler feature.
- RenderPearl fog-only follow-up fixed `ColorTargets::prepareWrite`: attaching the draw texture had rebound
  both framebuffer targets after the read FBO was selected, so compute pre-copy blits read from their own
  destination and discarded the rendered gbuffer. Read/draw attachments are now configured first, then bound
  independently. A real-GL regression verifies exact main-to-write texture transfer.
- Layered-cloud follow-up collapsed Lua/GLSL pass ownership: the clouds hook already canceled native clouds,
  but each Lua layer was independently inferred as `gbuffers_particles_translucent`. The world-render context
  now carries the Clouds layer into `render.quads`, and the shared mod-pass selector resolves it to
  `gbuffers_clouds`; explicit Lua layers still override stage defaults. Regression coverage verifies routing
  and nested scope restoration.
- **Verified:** omega client/server final build; 24/24 source-normalization, macro, and snippet tests; 2/2
  real-GL tests including RenderPearl every program in every dimension. Slow server boot, multiplayer
  integration, full ctest, and manual rendering remain excluded by user test policy.
- Latest focused verification: omega client link plus `PrepareWriteCopiesReadTextureToWriteTexture`,
  `RenderPearlDeferredExposesImageUniforms`, and `RenderPearlDeferredWriteBuffersMatchComposite` (3/3).
- Layered-cloud focused verification: omega client/server link plus `ModRenderScope.*` (2/2); no server boot,
  game launch, multiplayer integration, or full test suite.
- Shaderpack-switch follow-up fixed terrain mesh publication. `WorldRenderer::createColumn` served both true
  chunk arrivals and renderer-only reloads, but unconditionally gated every first mesh on lighting. Switching
  pack A→B cleared the old VBOs, rebuilt already-lit columns, then held all replacement terrain/water/shadow
  meshes behind a gate intended only for newly published chunks. Gating now begins in `chunkAvailable`; generic
  reload/frontier reconstruction immediately queues ready chunks.
- Compute image binding now derives `colortex*` image formats from the allocated `ColorTargets`, matching the
  raster path and eliminating the definition/runtime format split. `shadowcolor*` retains its own declaration
  source rather than being misread through scene targets. The narrow frame profiler remains CMake opt-in;
  temporary per-draw render logging is removed.
- Shaderpack-switch focused verification: omega client/server/installer link; `LightingReadyGate.*`,
  `PrepareWriteCopiesReadTextureToWriteTexture`, `RenderPearlRgba16fPrepareWriteAndImageBinding`,
  `RenderPearlDeferredExposesImageUniforms`, `RenderPearlGbufferDrawBuffersParse`,
  `RenderPearlDeferredWriteBuffersMatchComposite`, and `ModRenderScope.*` (13/13). No server boot, game launch,
  multiplayer integration, or full test suite.
- Pack activation is now staged rather than observable while incomplete. Selection, dimension changes, and
  shader-setting changes build a candidate runtime while the old pack keeps rendering. The candidate merges
  its final dimension first, prewarms every enabled program, waits for all async keys, validates every linked
  program, and preallocates colortex/custom-image/SSBO resources before one commit reloads mesh IDs and VBOs.
  Failed shaders or GPU allocation leave the previous pack active instead of presenting fog colour or black.
- `ProgramCache::poll` now resolves every cache key sharing one content-hash job; the former one-hash/one-key
  map left aliases pending forever. Cancelled queued compile jobs are discarded before consuming a worker.
  Resource-pack/LabPBR invalidation now precedes candidate readiness and resets all readiness state.
- Dimension transitions use the same staged runtime clone, eliminating the old live-pack cache destruction;
  settings reuse that path rather than maintaining another mutation path. Superseded candidates release their
  async waiters and GPU state, and deactivated packs release no-longer-used GPU resources after commit.
- Removed the RenderPearl hunt probes that still performed framebuffer rebinding, `glReadPixels`, draw-buffer
  queries, and `glGetError` consumption despite tracing being disabled. Present, deferred, compute, and terrain
  submission no longer mutate GL state for diagnostics.
- Final verification: omega client and `minecraft_omega_tests` link; RenderPearl every-program/every-dimension,
  compute images/write buffers/draw buffers, async duplicate-key cache, lighting publication, and Lua cloud
  scope tests pass (15/15). No server boot, game launch, full ctest, or multiplayer integration.
- Deslop follow-up removed the temporary render-log/probe system from the render tree, including disabled-call
  argument construction, GL error consumption, framebuffer queries, and cull-stall file output. The production
  frame profiler remains independent.
- Directory and zip pack loading now share one loader and one runtime initializer. Pack readiness is one
  `Cold → Submitted → Ready/Failed` state; selection, settings, and dimension changes share the same explicitly
  named staged runtime and commit/discard lifecycle. Custom-uniform compilation, program-cache reset, enabled
  pass indexing, and bucket publication now have one `PackInstance::rebuildRuntime` implementation.
- World-program selection is decided once by `PackManager::renderPack`; binders and `Pipeline` no longer carry
  parallel active/base selection logic. The remaining opt-in phase profiler uses the accurately scoped
  `MINECRAFT_FRAME_PROFILE` option instead of the deleted render-trace switch.
- Cleanup verification: `minecraft_native` and `minecraft_omega_tests` link; the focused RenderPearl, async
  cache, lighting gate, Lua render-scope, frame-pipeline, mailbox, and profiler set passes 22/22. No server/game
  boot or full ctest was run.

## 5. Dependency graph (PASS-1 only; PASS-2 deps listed per-item in §4.2)

```
L1 (first): WI-1 → WI-2 → WI-T            [WI-2 deps WI-1]
L2 (first, independent of L1): WI-12a      [no deps; must land alone]
P1a: WI-3 → WI-5                            [same lane, ordered; WI-5 uses snapshot capture]
P1b: WI-4                                   [deps WI-2]
P1c: WI-9 → WI-10                           [WI-10 deps WI-9 + WI-1]
P2a: P-FOGMODE ; P-LITGATE                 [no strict deps; run after P1 lanes]
P2b: P-IFENGINE → P-MACROS                  [same lane; both preprocessor]
```

PASS-2 gate for WI-12b: WI-4 + WI-6 + WI-7 + WI-8 + WI-9 + **WI-10** (audit-A FINDING 5).

---

## 6. THE 7-EXECUTOR LANE ASSIGNMENT (PASS-1)

Sequential stages; within each stage lanes run concurrently and are **file-disjoint**.
Cross-stage same-file edits are **sequenced** (the later stage edits the merged tree); the
compile-fixer reconciles the small number of same-file, disjoint-region cases listed below.

| Executor | Lane | Items | Runs after | Gate |
|---|---|---|---|---|
| 1 | **L1** (foundational) | WI-1, WI-2, WI-T | — (first) | — |
| 2 | **L2** (foundational) | WI-12a | — (first, independent of L1) | no WI-1 dep |
| 3 | **P1a** | WI-3, WI-5 | L1, L2 | — |
| 4 | **P1b** | WI-4 | L1 (WI-2) | — |
| 5 | **P1c** | WI-9, WI-10 | L1 (WI-1), L2 | — |
| 6 | **P2a** | P-FOGMODE, P-LITGATE | P1 lanes | — |
| 7 | **P2b** | P-IFENGINE, P-MACROS | P1 lanes | — |

### File-ownership matrix (verified disjoint within stage; cross-stage = sequenced)

| Lane | Files it may edit (new = ✚) |
|---|---|
| **L1** | `util/concurrent/{ThreadCoordinator,ThreadBudget,Channel,ThreadNames,Lifecycle}` ✚, `util/concurrent/WorkerPool.hpp`, `util/concurrent/WorkerHandoff.hpp`, `client/render/chunk/ChunkBuilder.hpp`, `world/light/LightingEngine.cpp`, `world/chunk/ChunkCache.cpp`, `client/gl/ShaderCompileService.cpp`, `client/gl/GLCore.cpp`, `client/Minecraft.cpp` (init :256-346 only), `CMakeLists.txt` |
| **L2** | `client/util/FramePipeline.*` ✚, `client/core/TaskMailbox.*` ✚, `client/util/FrameProfiler.*` ✚, `util/concurrent/FrameBudget.hpp` |
| **P1a** | `world/chunk/Chunk.hpp`, `world/chunk/Chunk.cpp`, `world/light/LightingEngine.cpp`, `client/render/chunk/RegionSnapshot.cpp`, `world/chunk/ChunkCache.cpp`, `mod/model/ModModels.cpp`, `client/render/chunk/ChunkBuilder.cpp`, `client/render/chunk/ChunkMeshJob.hpp`, `client/render/RenderCore.cpp`, `client/render/RenderCore.hpp`, `client/gl/GLCore.cpp` |
| **P1b** | `client/render/chunk/ChunkBuilder.hpp`, `util/concurrent/WorkerHandoff.hpp`, `client/render/world/WorldRenderer.cpp` |
| **P1c** | `network/Packet.hpp`, `network/Connection.hpp`, `network/Connection.cpp`, `server/network/ServerLoginNetworkHandler.cpp`, `server/network/ConnectionListener.cpp`, `client/multiplayer/ClientNetworkHandler.hpp/.cpp`, `client/gui/screen/DownloadingTerrainScreen.hpp`, `world/ClientWorld.cpp`, `client/multiplayer/MultiplayerSession.cpp` |
| **P2a** | `client/render/uniforms/FrameData.cpp`, `client/render/RenderCore.cpp`, `client/render/shaders/CustomUniforms.cpp`, `shaders/vanilla/shaders/lib/common.glsl`, `world/World.cpp`, `client/render/world/WorldRenderer.cpp` |
| **P2b** | `client/render/shaders/SourceProcessor.cpp`, `client/render/shaders/PreProcessor.hpp/.cpp`, `client/render/shaderpack/Loader.cpp`, `client/gl/ShaderBinaryCache.cpp`, ✚ `client/render/shaders/ConditionalState.hpp` |

**Disjointness proof:**
- **L1 ∩ L2 = ∅.** L1 touches concurrent + count-site files + `Minecraft.cpp`; L2 touches only new client
  classes + `FrameBudget.hpp`. (L1 does NOT touch `FrameBudget.hpp`; L2 does NOT touch `Minecraft.cpp` —
  the stall-mutex absorption was moved to WI-12b to keep them disjoint.)
- **P1a ∩ P1b = ∅.** P1a owns `ChunkBuilder.cpp`/`ChunkMeshJob.hpp`/`RenderCore.*`/`GLCore.cpp`/
  `ModModels.cpp`/chunk-light files; P1b owns `ChunkBuilder.hpp` (header)/`WorkerHandoff.hpp`/
  `WorldRenderer.cpp`. The R3 destructor assert is owned by P1a (in `ChunkBuilder.cpp`) so P1b never edits that file.
- **P1a ∩ P1c = ∅, P1b ∩ P1c = ∅.** P1c is exclusively network/multiplayer files.
- **P2a ∩ P2b = ∅.** P2a = frame-data/fog/chunk-gate; P2b = preprocessor + cache. (Both preprocessor items
  — P-IFENGINE, P-MACROS — are in the SAME lane P2b because they both touch `SourceProcessor.cpp`.)
- **Cross-stage (sequenced, not concurrent) same-file cases**, with disjoint regions:
  `LightingEngine.cpp` (L1 :49 → P1a :249-258), `ChunkCache.cpp` (L1 :218/:225 → P1a :174/:380-391),
  `GLCore.cpp` (L1 :288-290 → P1a :121/:141-145), `ChunkBuilder.hpp` (L1 :156 → P1b :120-157),
  `WorldRenderer.cpp` (P1b scheduler :372-404/:581/:747-770 → P2a lit-gate :199-205/:791-826/:1117-1140/:1189-1191),
  `RenderCore.cpp` (P1a alphaRef :70/:479-481/:671-680 → P2a fogMode :464),
  `Minecraft.cpp` (L1 init only; run-loop edits are PASS-2 WI-12b/13). Each pair is disjoint-region and
  sequential; the compile-fixer reconciles.

**CMakeLists ownership:** only L1 (WI-T) edits `CMakeLists.txt`. All new PASS-1 tests are pre-registered by
WI-T. No other lane touches it.

---

## 7. Build strategy for the compile-fixer (final stage)

1. **Only the compile-fixer builds/tests** (RULES §2, §7), via `.\build-omega.ps1` / `.\build-omega.ps1 -Clean`
   / `.\build-omega.ps1 -RunTests`, from project root.
2. **Compile order to check (in this order, after all 7 lanes merge):**
   a. `util/concurrent/` — the new infra must compile alone (L1).
   b. `client/util/FramePipeline.*`, `client/core/TaskMailbox.*`, `client/util/FrameProfiler.*`, `FrameBudget.hpp` (L2).
   c. Whole-tree grep: **`recommendedThreadCount` must have ZERO hits** (WI-2); grep `setAlphaTestRef` in
      `ModModels.cpp` → zero worker-path calls (WI-5 checklist).
   d. `CMakeLists` configure — verify all 7 orphan + all new PASS-1 tests resolve (WI-T pre-registration).
   e. Full client + server targets.
3. **If a lane landed broken (typical cases + fix surface):**
   - `WorkerHandoff.hpp:14` default-arg residue → compile error in every TU instantiating `WorkerHandoff`
     (F1) — restore the explicit-count construction (ChunkBuilder.hpp:156) or remove the default.
   - Missing count-site replacement → `recommendedThreadCount` undefined at a call site — grep, route through
     `coordinator.pool(Domain::X).threadCount()`.
   - New-test source not yet on disk but listed in CMake (pre-registered) → create the empty/discovery test
     body or move the listing; do NOT delete the WI-T wiring wholesale.
   - Same-file merge conflicts on the cross-stage files (§6) → merge the disjoint regions; run `git diff`
     review per lane transcript; keep each lane's region intact.
   - `ConditionalState.hpp` (P2b) missing include → add to the affected TUs.
4. **ctest run order:** `.\build-omega.ps1 -RunTests` runs the full suite once. Post-fix re-run the focused
   set per changed lane: `chunk_mesh_golden_test` (WI-2/3/4/5), `region_snapshot_race_test` (WI-3),
   `mesh_cancel_test` (WI-4), `packet_accounting_test`/`connection_async_teardown_test` (WI-9/10),
   `fog_mode_parity_test` (P-FOGMODE), `lighting_ready_gate_test` (P-LITGATE), `if_engine_unified_test` +
   `macro_parity_test` (P2b), then the full suite again.
5. **Golden determinism check:** `chunk_mesh_golden_test` byte-identical before/after the whole pass.

---

## 8. Deliberately open items (PASS-2, per the locked decisions)

These are **defined but not scheduled into the 7 executors**; they run in a later pipeline pass:

- **WI-12b** — `run()` rewiring + unified frame budget (gated on WI-4/6/7/8/9/10). Q1 explicitly deferred.
- **P-GLSL330** — full 330-core migration incl. T2/T3 (Q2; large).
- **P-ENTITYOVERLAY** — full `iris_overlay`/`iris_Entity`/`OverlayUV`/`LightUV` + overlay-derived
  `entityColor` (Q3; large).
- **WI-6, WI-7, WI-8, WI-11, WI-13, WI-14, WI-15, WI-16** — lighting channel; loader/save domains;
  shader poll + GL snapshot; ephemeral threads → Io; Lifecycle wiring; server coordinator (Q5 deferred);
  macro-batch remainder; `downloadPendingMods` (Q12 second pass).
- **F-2/F-5/F-7 and parity-bindings §8.2 KEEP items** — clip-space −1..1, far≈2×, mc_Entity ivec4, SSBO 13,
  colortex startIndex — remain documented port contracts, **MUST-NOT-CHANGE** in both passes.
- **GLSL dialect:** the legacy-120 support remains until P-GLSL330 lands (this pass only adds macros, not the
  migration); the port's fragment-`gl_TexCoord` gap (T2/T3) is closed only in P-GLSL330.

---

## 9. Deliverable note to AGENT 3 (fact checker)

Verify against source: (a) every `file:line` in §4 against the live working tree (line numbers drift — the
tree has uncommitted edits); (b) the disjointness proof in §6 (grep each lane's file list against the tree);
(c) the fogMode value mapping and that `g_fog.mode` internal 1/2/3 is preserved; (d) `kFileVersion` bump;
(e) that no PASS-2 item is scheduled into a PASS-1 lane; (f) the Q1–Q12 decisions map to the items listed.

---

## 10. PASS-3 RenderPearl contract collapse

Cross-reference baseline: local Iris `VanillaCoreTransformer`, `BuiltinReplacementUniforms`, `IdMap`,
`BlockMaterialMapping`, `IrisVertexFormats`, `XHFPTerrainVertex`, `ProgramDirectives`, and RenderPearl
`prog/lit.vsh`, `lib/norm_light_level.glsl`, `block.properties`, world programs, and target directives.

- One `VertexAbi.hpp` now owns the 76-byte terrain/entity vertex, attribute slots, aliases, GL formats,
  offsets, and invariants. `Tessellator`, `RenderCore`, and `ShaderProgram` consume it; their duplicate
  hard-coded layouts and bindings are gone.
- One `canonicalizeCoreSource` stage now owns legacy texture calls, storage qualifiers, entity inputs,
  fixed-function vertex lowering, fragment outputs, and alpha compatibility. `SourceProcessor` no longer
  runs separate overlapping legacy/core patch chains.
- Iris' fixed lightmap transform is restored exactly, including the `0.03125` translation. RenderPearl's
  compatibility branch now maps packed light `0..240` to normalized `0..1`, not `-0.033..0.967`.
- A present `block.properties` now defaults unmapped blocks to `-1`, matching Iris. Legacy numeric IDs are
  retained only when no custom block map exists; sand no longer becomes emission level 12 accidentally.
- `mc_Entity.y` now carries the shader ABI fluid sentinel (`1` for fluid, `-1` otherwise), not the beta
  renderer's unrelated render-type ordinal. RenderPearl water/shadow wave paths can classify water again.
- World draw-buffer selection and per-buffer blending now share the compiled program's colortex mapping at
  pass entry. The binder-side second path is removed; program destruction clears both GL attachments and
  semantic colortex indices, preventing stale bindings after shaderpack switches.

Verification: `minecraft_native` and `minecraft_omega_tests` build; 39 focused tests pass across
RenderPearl all-program/all-dimension GL compilation, source preparation, draw-buffer blending, custom
block IDs, fluid ABI, GLSL snippets, and deterministic chunk meshes. Server/game startup is intentionally
excluded.

---

## 11. PASS-4 Iris-style pack loading collapse

- `PackProgramId` is now the single program/fallback table. The loader stores only shader programs that
  actually exist; `resolveProgramKey` alone walks missing or disabled fallbacks. One source is transformed,
  compiled, cached, and addressed by its real name instead of being copied under every requested alias.
- `PackProgramSource.stage` is removed. A compute source is identified by its compute member, eliminating a
  string tag that duplicated the source shape.
- Blend and alpha directives consume the resolved program name directly. The fragment-path-to-program-name
  reconstruction shim is removed.
- Fragment-only raster programs now receive Iris' synthesized legacy vertex source and then enter the same
  canonical core transform as normal sources. They no longer discard their own fragment source and borrow an
  unrelated fallback vertex/fragment pair.
- Fragment directives are scanned once. Shadow and shadow-composite outputs no longer inflate scene colortex
  allocation; each family owns its proper target count.

Verification: client and test targets build; 106 focused loader, resolver, source-transform, draw-buffer, and
RenderPearl all-program/all-dimension GL tests pass. Game/server startup remains excluded.

---

## 12. PASS-5 core-only runtime and RenderPearl target-format repair

- Both visible and shader-worker contexts remain OpenGL 4.3 forward-compatible core. A whole render-tree scan
  found no live immediate-mode or fixed-function GL calls.
- `ShaderProgram` now rejects raster preambles below `#version 330 core` and compute preambles below
  `#version 430 core`. Disk, worker, synchronous, pack, and engine shader compilation therefore converge on
  one low-level core-profile contract.
- The engine-owned synthesized `default_legacy.vsh` GLSL 120 source is removed. Missing pack vertex stages now
  use native `default_raster.vsh`, with the canonical vertex ABI, chunk offset, texture matrix, Iris lightmap
  matrix, and color path directly expressed in core GLSL. Old pack syntax remains input to the single
  canonicalizer only; it is not a runtime shader dialect.
- RenderPearl's corruption was traced to loader format inference. The old 80-character look-behind associated
  `uvec colortex2` with the following `f16 colortex1`, allocating both as `RGBA32UI`. Float writes and sampler2D
  reads against the integer `colortex1` caused the observed stippled green/white G-buffer presentation.
  Inference now reads only the declaration type immediately attached to each exact `colortexN` identifier,
  and ignores directive suffixes such as `colortexNFormat`.
- RenderPearl now resolves `colortex1=RGBA16F` and `colortex2=RGBA32UI`; its raster draw-buffer mapping remains
  `{1,2}`. Its missing `final` program is intentional: `composite3` compute writes `colortex0`, then the Iris-style
  fallback presents `colortex0`.

Verification: `minecraft_native` and `minecraft_omega_tests` build. 104/104 focused loader, GLSL source,
core-contract, RenderPearl target/image/draw-buffer, and RenderPearl all-program/all-dimension GL tests pass.
Resources were synced. Server/game startup remains excluded.
