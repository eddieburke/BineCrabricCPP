# PLAN-CORRECTIONS — AGENT 3 Fact-Check / Plan-Corrector Report

Pipeline stage: **fact checker / plan corrector (AGENT 3 of 3)**. Review-only — NO source edits,
NO builds. Verified every `file:line` in `plan-master.md` against the **current working tree**
(2026-08-01; the tree has large uncommitted edits — this is the live tree, not HEAD). Also
deep-verified the two subsystems the user asked about (chunk publish paths; I/O locking).

The corrected citations have been folded **into `plan-master.md` itself** (task D). This document
is the record: (1) fact-correction table, (2) lock/deadlock notes, (3) rewritten work items,
(4) verdict.

---

## 1. FACT-CORRECTION TABLE

Severity: **C** = cosmetic (line drift; grep-before-edit already mandated), **B** = breaks build,
**H** = breaks behavior if followed literally (deadlock / wrong-code target).

| # | Plan ref (WI / lane / section) | Plan's `file:line` | What's wrong | Verified correct (current tree) | Sev |
|---|---|---|---|---|---|
| 1 | §4.1 WI-3 lock-order note (plan-master.md:270-274) | "chunk lock … **never** held while taking `queueMutex_` (`LightingEngine.cpp:60`)" | The rule is **violated by the plan's own implementation**: WI-3 makes `Chunk::setBlock` take the guard, and `Chunk::setBlock` (main thread) calls `world->queueLightUpdate` (`Chunk.cpp:43-45`, `:85-86`) → `World::queueLightUpdate` (`World.cpp:693-709`) → `LightingEngine::push` (`LightingEngine.cpp:58-91`) → `queueMutex_` at `LightingEngine.cpp:60`. So the main thread WOULD hold the chunk lock while taking `queueMutex_` unless the guard is scoped to the raw writes only. | No thread ever holds `queueMutex_` while acquiring the chunk lock (`queueMutex_` holders are `push`/`tryClaimBox`/`threadLoop`/`stop`, none touch chunk state), so **no cycle exists** — but the plan must mandate the guard scope, else a naive whole-`setBlock` guard both self-deadlocks (see row 2) and contradicts the printed rule. Fix text in §3. | **H** |
| 2 | §4.1 WI-3 `setBlock` bullet (plan-master.md:275-277) | "`Chunk.cpp:16-60`/`:61-100` — `setBlock` … takes the guard while writing `blocks[]` (:25/:69) and `meta` (:32/:76)" | Missing the **re-entrancy hazard**: `setBlock` runs `previousBlock->onBreak` (`:29/:73`) and `placedBlock->onPlaced` (`:51/:91`) *inside the function after the writes*; a block callback can call `world->setBlock` back into the **same chunk**, so a non-recursive `std::atomic_flag`/spinlock guard held for the whole function **self-deadlocks**. Same for `setBlockMeta` (`:102-112`). | Guard must be scoped to the raw writes `blocks[]` (`:25/:69`) + `meta` (`:32/:76`, `:107`) and **released before** `onBreak`/`onPlaced`/`updateHeightMap`/`lightGaps`/`queueLightUpdate` run. (Compare `LightingEngine::setBrightness`→`Chunk::setLight`, `Chunk.hpp:133-138`: writes nibbles only, no callbacks → whole-function scope is safe there.) Fix text in §3. | **H** |
| 3 | §4.2 P-GLSL330 (plan-master.md:575) | "120 branch in `lowerVertexSource` (:569-639) and `lowerFragmentSource` (:641-692) is removed" | **Wrong function range.** `lowerFragmentSource` starts at **:655**, not :641. Lines **:641-654** are `programGetsCompatAlphaTest` (:641-649) and `fragmentWritesLegacyFragOutput` (:651-653) — alpha-test gating helpers that MUST NOT be deleted. | `lowerVertexSource` `SourceProcessor.cpp:569-639`; `lowerFragmentSource` `SourceProcessor.cpp:655-692`. | **H** |
| 4 | §1.7 + §4.1 P-IFENGINE (plan-master.md:75-76, 424) | `Loader.cpp:307-438` (preprocessProperties) | Function ends at **:435** (`return result;`). (:438 is the next function's `has(...)`.) | `preprocessProperties` = `Loader.cpp:307-435`; conditional machine (`activeStack`/`matchedStack` push/elif/else/endif) = `:353-414` (P-IFENGINE's `:353-416` sub-range is acceptable). | C |
| 5 | §1.8 + §4.2 P-GLSL330 + §4.1 P-MACROS (plan-master.md:86, 447, 579) | `versionPreamble` `SourceProcessor.cpp:709-777` | Function ends at **:778** (`return result;`). | `versionPreamble` = `SourceProcessor.cpp:709-778`. | C |
| 6 | §4.2 WI-8 (plan-master.md:504) | "Worker `ShaderProgram::destroy()` writes `s_lastBoundProgram` (`ShaderCompileService.cpp:200`)" | Wrong file for the write. `ShaderCompileService.cpp:200` is the worker-side local `ShaderProgram program;`. The actual write is **inside `ShaderProgram::destroy()`** at `ShaderProgram.cpp:212-213`. | `s_lastBoundProgram` written at `ShaderProgram.cpp:14,213`; worker trigger at `ShaderCompileService.cpp:190-236`. | C |
| 7 | §1.1 + §4.1 WI-2 + §6 cross-stage (plan-master.md:220, 678) | `GLCore.cpp:289` (driver hint) | The `GL_KHR_parallel_shader_compile` hint call (`maxShaderCompilerThreadsKHR(threads)`) is at **:290**; the `hw−2` `threads` calc is at :288-289, the `if(parallelShaderCompileSupported)` at :287. | Driver-hint block = `GLCore.cpp:286-291`, call at :290. | C |
| 8 | §4.1 WI-13 (plan-master.md:542) | `ChunkCache.cpp:27-36,349-355` (drain / waitForPendingWrites) | `waitForPendingWrites` definition is at **:356-362**, not :349-355 (inherited stale from synthesis HZ-02). | dtor `ChunkCache.cpp:27-36`; `waitForPendingWrites` `:356-362` (calls at :28/:416/:428). | C |
| 9 | §4.1 WI-3 (plan-master.md:279) | `ChunkCache.cpp:173-176` (`populateBlockLight`) | `populateBlockLight` is a `Chunk` method; the ChunkCache call site is at **:174**. | `chunk->populateBlockLight()` at `ChunkCache.cpp:174`; declaration `Chunk.hpp:103`. | C |
| 10 | §4.1 WI-5 (plan-master.md:317) | capture-time snapshot `ChunkBuilder.cpp:182-196` | The range misses where `lightLevelToLuminance` is actually snapshotted (:170-181). `blockRenderLayers` snapshot is :189-195; whole `ChunkMeshJob::capture` is :132-197. | Snapshot sites = `ChunkBuilder.cpp:170-196` (lightLevelToLuminance :170-181, blockRenderLayers :189-195). | C |
| 11 | §4.1 WI-4 (plan-master.md:299) | `WorldRenderer.cpp:285-304` (`sweepRetiring`) | `sweepRetiring` is :295-304; :282-294 is `retireOrFreeSection`. | `sweepRetiring` = `WorldRenderer.cpp:295-304` (called at :779). | C |
| 12 | §4.1 WI-4 (plan-master.md:305) | version-stamp stale-drop `WorldRenderer.cpp:761-764` | Version check block spans **:761-765** (`if(job->failed \|\| job->version != builder->version)` at :761; body :762-765). Note `meshJobInFlight=false` is set at :760 *before* the check. | `WorldRenderer.cpp:760-765`. | C |
| 13 | §4.1 WI-4 API list (plan-master.md:292-293) | "keep the public API shape (`enqueue/enqueueNear/drainCompleted/idle/workerCount/cancelAll`)" | **Incomplete — a build break if followed literally.** `WorldRenderer::compileChunks` also calls `meshScheduler_.pendingJobs()` at `WorldRenderer.cpp:738` and `:781`. If WI-4 drops `pendingJobs()`, the client target fails to compile. | `ChunkMeshScheduler` public API = `enqueue/enqueueNear/drainCompleted/cancelAll/idle/pendingJobs/workerCount` (`ChunkBuilder.hpp:122-152`); `pendingJobs` used at `WorldRenderer.cpp:738,781`. Fix text in §3. | **B** |
| 14 | §4.1 P-LITGATE (plan-master.md:412) | `WorldRenderer.cpp:1166-1189` (`setBlocksDirty`/`markDirty` chains) | :1166-1188 is `drainBorderRefresh`. `markDirty` is at :1117-1140, `setBlocksDirty` at :1189-1191, `blockUpdate` at :1141. | Gate integration points = `WorldRenderer.cpp:1117-1140` (`markDirty`) + `:1189-1191` (`setBlocksDirty`). | C |
| 15 | §4.1 WI-9 (plan-master.md:343) | "Remove the join in `verifyUsernameOnline` (:150-152)" | The join is at **:150-151** (line :152 is the spawn's continuation). | `ServerLoginNetworkHandler.cpp:150-151` (join), `:154` (spawn), `:169-170` (deferredLoginPacket_ under verifyMutex_), `:172/:175` (failure `disconnect` from verify thread). | C |
| 16 | §4.2 WI-7 (plan-master.md:486) | `:250-288` (`integrateFinishedLoads` drops cancelled) | Function is :251-295. Also the "scans forever" framing is exaggerated: `integrateFinishedLoads` is a bounded per-call scan that **breaks** when no done entry remains; the real defect is that a cancelled `PendingLoad` is never *removed* (only skipped), and today the only `loaderPool_->cancelPending()` is `~ChunkCache` (:30), so stale entries currently surface mainly at teardown (or a future cancel path). The epoch-token design (WI-7) is still the correct fix. | `requestChunkAsync` `ChunkCache.cpp:227-249`; `integrateFinishedLoads` `:251-295`; PendingLoad member `ChunkCache.hpp:49-54`; only cancel at `~ChunkCache` `:30`. | C |
| 17 | §4.2 P-ENTITYOVERLAY (plan-master.md:607-608) | `LivingEntityRenderer.cpp:206-254`; "overlay colors at :230-249" | The overlay draw block is :206-~240; the color component computation is **:230-234**; `getOverlayColor` is *defined* at :310. | Overlay pass `LivingEntityRenderer.cpp:206-240`; color extraction :230-234; `getOverlayColor` :310. | C |
| 18 | §6 cross-stage list (plan-master.md:677-682) | `ChunkCache.cpp` (L1 :218/:225 → P1a :173/:380-391); `WorldRenderer.cpp` (P1b → P2a :199-205) | Two omissions: P1a's ChunkCache cite should read :174 (not :173); P2a P-LITGATE also edits `WorldRenderer.cpp:791-826` (capture-loop re-check) and `:1117-1140`/`:1189-1191` — the cross-stage row should list the full P2a region set. All still sequential (P2a after P1); not a file-conflict. | See rows 9 and 14; update §6 to name the full regions. | C |
| 19 | §1.8 (plan-master.md:82-83) | "`lowerVertexSource`/`lowerFragmentSource` (:569-692)" | Combined span is misleading; the two functions are disjoint with the alpha-test helpers between them. | `lowerVertexSource` `:569-639`; `lowerFragmentSource` `:655-692`. | C |

**Verified TRUE (no correction needed) — headline claims the plan gets right:**
- `WorkerHandoff.hpp:14` default arg `recommendedThreadCount(2,2)`; `WorkerPool.hpp:61-68` definition; the **4** `recommendedThreadCount` sites (ChunkBuilder.hpp:156 `rec(3,2,6)`, ChunkCache.cpp:218 `rec(3,2,4)`, LightingEngine.cpp:49 `rec(3,2,3)`, WorkerHandoff.hpp:14 `rec(2,2)`) + **2** `hw−2` sites (ShaderCompileService.cpp:47, GLCore.cpp:288-290) — WI-2's inventory is exact.
- `ChunkMeshJob.hpp:51-54` nearLane comment + `bool nearLane=false` at :54; set only via `enqueueNear` (`ChunkBuilder.hpp:135`); **never read** in `processUpload` (`WorldRenderer.cpp:747-770`) — WI-4's HZ-31 claim verified.
- `~ChunkMeshJob` (`ChunkBuilder.cpp:220-225`) clears `meshJobInFlight`; destruction is main-thread-only today (cancelAll/clearSections/pendingMeshUploads_.clear all run on main) — R3 as stated.
- `retireFromLighting` busy-spin (`ChunkCache.cpp:62-68`, 200 µs sleep); `beginRenderEviction`/`tryAcquireRenderPin` (`Chunk.hpp:205-225`).
- `readQueue_` unbounded (`Connection.cpp:272-275`); send cap `0x100000` + `disconnect.overflow` in `tick()` (:185-187); double-drain real (`DownloadingTerrainScreen.hpp:19-26` + `ClientWorld.cpp:53-55` both call `handler->tick()`).
- `Packet::read` mutates static `packetTrackers()[rawId]` (:79) + `++incomingCount()` (:80); statics at :121-128.
- fogMode internal 1/2/3 (`FrameData.cpp:237`, `RenderCore.cpp:464`; `RenderCore.hpp:60` comment "0 off,1 linear,2 exp,3 exp2"); `g_fog.mode` internal assignments at `RenderCore.cpp:784/790/797/803`; `fogShape` OFF→−1/ON→1 (`FrameData.cpp:239`, `RenderCore.cpp:465`); vanilla `common.glsl:18-26` interprets 1/2/3 and *falsely* claims "as Iris reports them" (:19) — P-FOGMODE target confirmed.
- `kFileVersion = 1` at `ShaderBinaryCache.cpp:9`; `kCategories` 17 entries at `SourceProcessor.cpp:745-748`; Java `BiomeCategories.java` has **19** (MOUNTAIN/UNDERGROUND at :23-24); `StandardMacros.java:58,60,61,101-104,53` all as cited; `CommonTransformer.java:182-232` (T2/T3) as cited; `EntityPatcher.java:20-21,33-39,45-49,54-62` and `IrisSamplers.java:36-37,209-211` as cited.
- `programFromPackSync` dead at `Pipeline.cpp:695-702`; `clrwl_` early-out at `Pipeline.cpp:744`; `worldProgram` at :704-782.
- SSBO: cap 13 (`Resources.hpp:13`), `clearBufferSubData` init (`Resources.cpp:120,127`), `GL_DYNAMIC_STORAGE_BIT` (:123) — already in parity.
- `s_lastBoundProgram` `ShaderProgram.cpp:14`, `bindAttribLocation` list :146-156; `RenderCore.cpp:839-877` attrib layout (kOffEntity=36 ivec4 at :870) matches parity-bindings.
- `FrameData.cpp:32-33` (`g_centerDepthSmooth`/`g_wetnessSmooth`), :191-196 (frame statics), :266 (`cameraTracker`), :538-539 (`smoothBlock`/`smoothSky`), :589-604 (19 `SmoothedState`), :663-664 (first-frame previous = current) — §2.4 / WI-15 F-6 citations exact.
- `Minecraft.cpp`: init :256, tick-order invariant comment :647-649, `pendingScreenResize_` :658-661, `runRenderPhase` :670, `pumpAndPresent` :694, `luaHookRenderFrame` :703, `onFrameUpdate` :708, inactive sleep :714-721, stallMutex :742-743/:827-828, `run()` :762, `while(running.load())` :773, heartbeat :776, `screenStack_.flushRetired` :778, `multiplayerSession_.flushRetired` :782, `timer.advance` :801-807, tick loop :809-818, `runRenderPhase` call :823, `gameCrashed` :248, `stop()` :347 (ShaderCompileService::stop :354, DisplayManager::destroy :388, `std::_Exit(0)` :391), post-loop stop :860 — all as cited.
- `GameRenderer.cpp`: `compileChunks` call :1195, `shaderPacks_->poll()` :711, farPlane :515-518/:574/:1061-1063, `setDrawCameraStateFromCamera` :1064, stage chain :1083-1330 (renderBegin :1106 … sampleCenterDepth :1330).
- `ChunkRegionBuffer.cpp`: `reallocBuffer` :50-62 (bufferData + full mirror `bufferSubData`), `upload` :74-111; whole-mirror re-upload only when GPU capacity is exceeded (`gpuCapacity_ < shadow_.size()` → `reallocBuffer`, :102-104); reserve mitigation at `WorldRenderer.cpp:550-557`.
- The 7 orphan tests exist on disk and are absent from `MINECRAFT_TEST_SOURCES`/`MINECRAFT_SERVER_TEST_SOURCES` (`CMakeLists.txt:339-381`); `mp_parity_updates_test.cpp` (:346) and `render_settings_test.cpp` (:348) are wired — WI-T verified.

---

## 2. LOCK / DEADLOCK NOTES (verified lock-order + the two deep-dives)

### 2.1 Verified lock inventory (current tree)

| Mutex | File:line | Type | Held by | Nested with |
|---|---|---|---|---|
| `ioMutex_` | ChunkCache.hpp:89 | `std::recursive_mutex` | load (:112), saveEntities (:300), saveChunk sync (:374), decorate (:387), save/flush (:420), tick (:459,:466) | itself (recursion, e.g. decorate→getChunk→loadChunk→produceChunk); **ioMutex_ → chunkFileMutex()** on the storage-load path |
| `workerGeneratorMutex_` | ChunkCache.hpp:91 | mutex | workerGenerator() (:92-104) | none |
| `saveCompleteMutex_`+`saveCompleteCv_` | ChunkCache.hpp:95-96 | mutex+CV | save worker (notify :324-325/:351-352), main `waitForPendingWrites` (:357-358) | none |
| `chunkFileMutex()` | AlphaChunkStorage.cpp:24-25 (used :202,:238) | static function-local mutex | loader (under ioMutex_), save-pool worker (**without** ioMutex_) | only under ioMutex_ on the load path; never re-enters ChunkCache → no cycle |
| `readMutex_` / `writeMutex_` | Connection.hpp:106-107 | mutex | reader (:273) / writer (:289) + tick (:206) | **never** nested with each other or anything else |
| `queueMutex_` | LightingEngine.cpp:60 | mutex | `push` (main+workers :60), `tryClaimBox`/`threadLoop` (:191), `stop` (:135,:144) | none — **queueMutex_ holders never touch chunk state** |
| `registryMutex_` | LightingEngine.cpp:97/:106/:214 | mutex | register/unregister/chunkAt | none (chunkAt :214 then `tryAcquireRenderPin` :219 — pin, not a lock) |
| `outboxMutex_` | LightingEngine.cpp:114/:422 | mutex | `drainDirtyRegions` (main :114), `runUpdate` (worker :422) | none — `runUpdate` runs *outside* `queueMutex_`; never nested with it |
| per-chunk render pin | Chunk.hpp:205-225 | atomics (`renderEvicting_`, `renderPinCount_`) | main capture, lighting workers (chunkAt :219) | not a mutex; protects eviction only |

**Ordering:** pin → (optional chunk-lock per WI-3) → nibble write/copy. `ioMutex_ → chunkFileMutex()`. No thread holds `queueMutex_` while taking any other lock. **No lock-order cycle exists today.**

### 2.2 The one real hazard WI-3 introduces (must be fixed before running P1a)

WI-3 adds a per-chunk guard. Two consequences the plan's printed rule contradicts or misses:

1. **chunk-lock → `queueMutex_` (main thread).** `Chunk::setBlock` (guarded) → `world->queueLightUpdate` (`Chunk.cpp:43-45`/`:85-86`) → `World::queueLightUpdate` (`World.cpp:693-709`) → `LightingEngine::push` (`LightingEngine.cpp:60`). This is the **only** edge where the chunk lock is held while acquiring `queueMutex_`; no inverse edge exists, so **no deadlock** — but the plan's "never held while taking `queueMutex_`" rule is unsatisfiable by its own implementation unless the guard is scoped to the raw writes. The fix (row 1) is to scope the guard and reword the rule.

2. **Non-recursive-guard self-deadlock.** `Chunk::setBlock` runs `onBreak` (:29/:73) and `onPlaced` (:51/:91) *inside* the function after the writes; block callbacks can re-enter `world->setBlock` on the **same chunk**. A whole-function `std::atomic_flag`/spinlock guard self-deadlocks. The guard must cover only `blocks[]`/`meta` writes (`:25/:32`, `:69/:76`, `:107` for `setBlockMeta`) and be released before the callbacks/heightmap/`queueLightUpdate` run. (`Chunk::setLight` `Chunk.hpp:133-138` has no callbacks, so its scope may be whole-function.)

No similar hazard exists for the lighting-worker path: `setBrightness` (`LightingEngine.cpp:249-258`) → `setLight` (guard) → release, then later `outboxMutex_` at :422 with **no** chunk lock held (pins held, locks not). The "pin→chunk-lock, never chunk-lock across `beginRenderEviction`" ordering is achievable.

### 2.3 Chunk publish path — full lifecycle (verified)

1. **Worker:** `WorldRenderer::startMeshJob` (`WorldRenderer.cpp:568-589`) → `ChunkMeshScheduler::enqueue`/`enqueueNear` (`ChunkBuilder.hpp:122-137`, sets `nearLane` at :135) → `WorkerHandoff::enqueue` (`WorkerHandoff.hpp:21-30`): worker runs `ChunkBuilder::buildMesh` (`ChunkBuilder.cpp:253-351`; snapshot captured on the worker via `captureSnapshot` :226-241 while pins are held), then pushes the completed `shared_ptr<ChunkMeshJob>` into `completed_` under `mutex_` (:26-27).
2. **Main (after present):** `GameRenderer::onFrameUpdate` (`GameRenderer.cpp:1195`) → `WorldRenderer::compileChunks` (:731) → `meshScheduler_.drainCompleted()` (:775) → `processUpload` lambda (:747-770): guard `builder->retired`, budget (`FrameBudget::fromMs(3|6 ms, …)` :742-743), clear `meshJobInFlight` (:760), version-stamp check (:761-765), then `builder->uploadMesh(*job)` (:766).
3. **GPU publish (main GL thread):** `ChunkBuilder::uploadMesh` (`ChunkBuilder.cpp:352-415`) → `ChunkRegionBuffer::upload` (`ChunkRegionBuffer.cpp:74-111`) → `glBufferSubData` (or whole-mirror `reallocBuffer` re-upload when capacity exceeded :102-104 → :50-62). **Never on a worker.**
4. **Draw:** `renderChunksVbo` (`WorldRenderer.cpp:607-654`) → `ChunkRegionBuffer::flush` (`ChunkRegionBuffer.cpp:184-249`) → `glDrawElements`.

**Plan's WI-4/12a claims verified:** publish happens on the main thread inside `compileChunks`→`processUpload`→`uploadMesh`, after `pumpAndPresent` (`Minecraft.cpp:694`, present-before-draw confirmed). `nearLane` is documented (`ChunkMeshJob.hpp:51-54`) but never read in `processUpload` — WI-4's fix target is real. The version-stamp ordering (`job->version != builder->version` ⇒ drop + re-enqueue) is intact. **R3 / Channel compatibility:** WI-4's Channel + non-blocking cancel is compatible with `~ChunkMeshJob` (`ChunkBuilder.cpp:220-225`) **provided** (a) `cancelAll`/channel-drop stay main-thread-only (the plan says this), and (b) `pendingJobs()` survives (see row 13 — the plan's API list omits it). With (a)+(b) a worker can never drop the last `shared_ptr` (the task lambda moves the ref into `completed_`/the channel), so the R3 invariant holds.

### 2.4 P-LITGATE — can it deadlock the LightingEngine outbox? **No.**

`doLightingUpdates` (`World.cpp:711-717`) calls `lighting_.drainDirtyRegions` (`LightingEngine.cpp:113-126`), which holds `outboxMutex_` **only for the copy** (:114-125) and releases before returning. The "mark column lit" step runs after the drain on the main thread (plain field write into WorldRenderer's gate state), and `enqueueDirtyChunk` (`WorldRenderer.cpp:199-205`) / the capture loop (:792-826) read it on the main thread. No lock is held across the lit-stamp. The only requirement: the lit-stamp must also be set for columns adopted with lighting already drained (plan already covers "no boxes → lit immediately"). Deadlock-free as specified.

---

## 3. REWRITTEN WORK ITEM / BULLET TEXT (already applied to `plan-master.md`)

### 3.1 WI-3 — corrected lock-order note + `setBlock`/`setBlockMeta` guard scoping

Replace the WI-3 "Files & change" lock/`setBlock` bullets with:

> - `world/chunk/Chunk.hpp` — add a per-chunk light/block-write guard (atomic flag/spinlock); document at
>   `tryAcquireRenderPin`/`beginRenderEviction` (`Chunk.hpp:205-224`): chunk lock always acquired **after** a pin,
>   **never** held across `beginRenderEviction`, never held while taking `outboxMutex_`
>   (`LightingEngine.cpp:114`) or `registryMutex_`; `ioMutex_`/`readMutex_`/`writeMutex_` have no
>   cross-nesting (verified). **The guard is non-recursive and must be scoped to the raw array writes only.**
> - `world/chunk/Chunk.cpp:16-60` and `:61-100` — `setBlock` (both overloads) takes the guard while writing
>   `blocks[]` (:25/:69) and `meta` (:32/:76) **and releases it before `onBreak`/`onPlaced`/
>   `updateHeightMap`/`lightGaps`/`world->queueLightUpdate` run** — those callbacks re-enter `Chunk::setBlock`
>   for neighbor updates and take `queueMutex_` via `World::queueLightUpdate`→`LightingEngine::push`
>   (`World.cpp:693-709`, `LightingEngine.cpp:60`); a whole-function guard self-deadlocks a non-recursive
>   spinlock and contradicts the queueMutex_ rule above. Same scoping for `Chunk::setBlockMeta`
>   (:102-112, writes `meta` at :107).
> - `world/light/LightingEngine.cpp:249-258` — `setBrightness`→`setLight` (`Chunk.hpp:133-138`) takes the guard
>   writing nibbles; `setLight` has no re-entrant callbacks, so a whole-function scope is safe here.
> - `client/render/chunk/RegionSnapshot.cpp:32-67` — `copyChunkBand` takes the guard; ~11 KB copy, held µs.
> - `world/chunk/ChunkCache.cpp:174` (`populateBlockLight` call site) and `:380-391` (`decorate`) — main-thread
>   writers, take the guard too (R4: save/adopt snapshot `ChunkCache.cpp:157-201` adopt, `:363-379` save;
>   the light/block snapshot read is `AlphaChunkStorage::takeSnapshot` at `ChunkCache.cpp:371`).

### 3.2 WI-4 — corrected API list (adds `pendingJobs()`)

Replace "keep the public API shape (`enqueue/enqueueNear/drainCompleted/idle/workerCount/cancelAll`) so `WorldRenderer` compiles unchanged" with:

> keep the public API shape (`enqueue/enqueueNear/drainCompleted/cancelAll/idle/pendingJobs/workerCount` —
> `ChunkBuilder.hpp:122-152`) so `WorldRenderer` compiles unchanged (`pendingJobs()` is used at
> `WorldRenderer.cpp:738` and `:781`).

### 3.3 P-GLSL330 — corrected function ranges

Replace "the 120 branch in `lowerVertexSource` (:569-639) and `lowerFragmentSource` (:641-692) is removed" with:

> the 120 branch in `lowerVertexSource` (`SourceProcessor.cpp:569-639`) and `lowerFragmentSource`
> (`SourceProcessor.cpp:655-692`) is removed. Do NOT touch `programGetsCompatAlphaTest`/`
> fragmentWritesLegacyFragOutput` (`:641-654`) — they gate alpha-test injection, not dialect.

---

## 4. VERDICT

**The PASS-1 executor set (the 7 lanes) is file-disjoint and dependency-ordered correctly.**
- **File ownership:** verified against the tree — no two PASS-1 lanes touch the same file concurrently.
  Within stage 1, `L1 ∩ L2 = ∅`; stage 2, `P1a ∩ P1b ∩ P1c = ∅`; stage 3, `P2a ∩ P2b = ∅`. The only
  same-file touches are cross-stage (L1→P1a/P1b, P1a→P2a, P1b→P2a), all correctly identified as sequenced in
  §6, with two minor omissions (P2a also edits `WorldRenderer.cpp:791-826` and `:1117-1140`/`:1189-1191`;
  P1a's ChunkCache cite is :174, not :173) that are sequential anyway.
- **Dependencies:** L1→P1a/P1b/P1c, P1b (WI-4) deps WI-2, P1c (WI-10) deps WI-9+WI-1, P2a/P2b after P1 —
  all correct. No PASS-2 item is scheduled into a PASS-1 lane.

**Safe to run as-is?** **No — two fixes must land in the plan first (both are applied to
`plan-master.md` by this pass):**
1. **WI-3** (P1a): the guard must be scoped to the raw `blocks[]`/`meta` writes in `Chunk::setBlock`/
   `setBlockMeta`, released before the block callbacks and `queueLightUpdate` run; otherwise a naive
   whole-function spinlock **self-deadlocks** and the printed "never hold chunk lock while taking
   `queueMutex_`" rule is violated by the plan's own implementation (no cycle exists, but the rule must
   match the implementation).
2. **WI-4** (P1b): the kept-API list must include `pendingJobs()` (`WorldRenderer.cpp:738,781`), or the
   client target breaks the build.

Everything else is cosmetic line drift (rows 4-19) or verified-correct. The single wrong *function-range*
citation (`lowerFragmentSource :641-692`) is in **PASS-2 P-GLSL330**, so it does not block this pass, but it
is corrected now (row 3) so the later run does not delete the alpha-test gating helpers.

**Deep-dive verdicts:** (a) Chunk publish is entirely main-thread post-present; WI-4's nearLane/version/R3
claims all verify; WI-4's Channel + non-blocking cancel is R3-compatible under the two conditions above.
(b) I/O locking has no deadlock cycle today or after WI-3 (the one new edge is one-way); P-LITGATE cannot
deadlock the LightingEngine outbox.
