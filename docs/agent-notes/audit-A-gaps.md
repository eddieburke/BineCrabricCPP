# AUDIT-A — Gaps / Wrong-Assumption Audit of plan-initial.md

Auditor: **PLAN AUDITOR A**. Scope: `docs/agent-notes/plan-initial.md` against the six
council docs + `synthesis.md` + the **current working tree** (verified 2026-08-01).
No source files were edited; nothing was built. All `plan §N` references are to
`plan-initial.md` line ranges; code citations are working-tree verified.

Verdict in brief at the end: **NOT buildable-by-executors in the current form** —
three build-breaking/ordering defects (FINDING 1, 2, 8) and one correctness hole
(FINDING 6) must be fixed first.

---

## FINDINGS (numbered, severity HIGH/MED/LOW)

### 1. HIGH — WI-2 deletes `recommendedThreadCount` but forgets its own default-arg call site → guaranteed build break
- **Evidence:** plan WI-2 (plan-initial.md:149-174) file list is `WorkerPool.hpp:61-68`,
  `ChunkBuilder.hpp:156`, `LightingEngine.cpp:49`, `ChunkCache.cpp:217/:224`,
  `ShaderCompileService.cpp:47`, `GLCore.cpp:289`. The synthesis (synthesis.md:482-483,
  M3) explicitly warns the **unused default at `WorkerHandoff.hpp:14`**
  (`explicit WorkerHandoff(unsigned threadCount = WorkerPool::recommendedThreadCount(2, 2))`)
  must be handled, "delete or leave, but do not forget it exists". Deleting
  `recommendedThreadCount` (WorkerPool.hpp:61-68) makes `WorkerHandoff.hpp:14` a
  compile error in every TU that instantiates `WorkerHandoff<ChunkMeshJob>`
  (ChunkBuilder.hpp:12,155).
- **Why it matters:** plan §3 preamble (plan-initial.md:10-13) makes **every WI
  build-safe and partial-landable**. WI-2 as written breaks the build until WI-4
  (which is in a different parallel lane) happens to touch WorkerHandoff.hpp.
- **Fix:** add `WorkerHandoff.hpp:14` to WI-2's file list with explicit instruction
  (remove the default arg; the sole live instantiation at ChunkBuilder.hpp:155-156
  passes an explicit count, so nothing is lost).

### 2. HIGH — WI-2 oversubscription math is self-contradictory: the plan's own example proves the fix doesn't work on the target machines
- **Evidence:**
  - Plan WI-2 (plan-initial.md:161-162): "on an 8-core machine counts land at
    mesh2/light2/loader2/save1/shader2 — total worker threads drop from ~11 to ~7-8".
  - Architecture formula (council-architecture-proposal.md:159-166): `globalBudget =
    max(1, hw−2)`; **mandatory pinned classes deducted first** (audio 4, logging 1,
    GL-compile, network); then `compute = clamp(remaining, 1, 8)`, `io =
    clamp(2, remaining/4, 3)`.
  - On 8 logical cores: budget = 6. Pinned audio(4)+log(1)+GlCompile(2) = **7 > 6**,
    so remaining is negative → `compute = clamp(negative, 1, 8) = 1`, `io = 2`.
    That gives **compute=1** for mesh+lighting+gen combined, not "mesh2/light2/loader2"
    (=6 workers from a compute budget of 1).
  - And WI-2 says (plan-initial.md:166-167): "keep each owner's existing pool object"
    — meaning after WI-2, mesh keeps its own `WorkerHandoff` pool, lighting its own
    `jthread` vector, loader its own `WorkerPool`, **each sized from
    `pool(Compute).threadCount()`**. Three owners × the full compute count = **3×
    oversubscription relative to the budget** — the exact defect HZ-01/WI-2 claim to
    remove.
- **Why it matters:** audit item 6 — a single global budget with domain pools does
  **not** fix the "independent authorities" problem when (a) pinned classes (audio,
  GL-compile, network reserveDynamic) are not capped and alone exceed the budget on
  low-core machines, and (b) until WI-4/6/7 land, owners keep separate pools and
  multiply the count. On the likely low-core target (bundled mingw64 toolchain,
  RULES FOR AGENTS.md:29-38) WI-2 can even be *worse* than today.
- **Fix:** define the subdivision: `pool(Compute)` must be a **single shared pool**
  sized `compute`, with WI-2 switching owners onto it (or specify an explicit
  per-owner share that sums to ≤ `compute`). Present a worked table for 4/8/16
  logical cores including pinned classes. Note the plan's "7-8" figure is also just
  wrong arithmetic (2+2+2+1+2 = 9, and shader on 8-core today is min(8−2,4)=**4**).

### 3. HIGH — WI-3 does NOT close R1/R4 on `blocks`/`meta`: main-thread `Chunk::setBlock` is a writer but is missing from the lock plan
- **Evidence:** WI-3 (plan-initial.md:183-192) file list = `Chunk.hpp:133-140`
  (`setLight`), `LightingEngine.cpp:249-258`, `RegionSnapshot.cpp:32-67`
  (`copyChunkBand`), `ChunkCache.cpp:173` + `:373-384`. The council's own option (a)
  (council-chunk-workers.md:339-348) requires **"a single per-chunk rwlock used by
  `setBlock`, `decorate`, `populateBlockLight`, and `copyChunkBand` closes both"**
  (R1 *and* R4). `Chunk.cpp:61-89` `setBlock` is a main-thread writer of
  `blocks`/`meta` (council-chunk-workers.md:197) — it is **not in WI-3's file list**.
  `copyChunkBand` memcpys `blocks`, `meta`, `skyLight`, `blockLight` together
  (RegionSnapshot.cpp:54-66, verified). Locking only light writes + the copy leaves
  the blocks/meta torn-copy race open whenever any block edit happens during a mesh
  worker's snapshot (every frame).
- **Why it matters:** R1 is the council's "dominant correctness risk"
  (council-chunk-workers.md:26-28). The plan's headline claim "Close the light-array
  snapshot race (R1/R4)" is only half-true as written.
- **Fix:** add `Chunk.cpp:61-89` (`setBlock`) and the meta/block paths to WI-3's file
  list and take the same per-chunk lock; or explicitly adopt option (b)
  version-stamped copy for blocks/meta too.

### 4. HIGH — The "parallel lanes" are not file-independent and not dependency-correct; cross-lane edges break the multitask executor model
- **Evidence:** plan §4 (plan-initial.md:511-521) says lanes are "safe to run
  concurrently as separate executors, since items are additive and touch disjoint
  files", but:
  - **Lane 1 = WI-3 → WI-4 → WI-5**; **Lane 2 = WI-1 → WI-2 → WI-6/7/8**. WI-4's
    own dependency line (plan-initial.md:501) says "WI-2 → WI-4 ← depends: WI-2
    (uses Compute count)". So Lane 1's second item **requires Lane 2's WI-2**. If a
    Lane-1 executor edits `ChunkBuilder.hpp:120-157` to use
    `pool(Compute).threadCount()` before WI-2 lands, it cannot compile.
  - WI-10 (Lane 3) and WI-11 (Lane 4) both depend on WI-1 (Lane 2)
    (plan-initial.md:498-500), and WI-10 also depends on WI-9 (Lane 3).
  - Same-file conflicts across lanes: `LightingEngine.cpp` is edited by WI-2, WI-3,
    and WI-6; `ChunkCache.cpp` by WI-3 and WI-7; `Minecraft.cpp` by WI-1, WI-11
    (SessionValidator consumption, Minecraft.cpp:604-606), WI-12, WI-13.
- **Why it matters:** the RULES pipeline (RULES FOR AGENTS.md:97-103) runs executors
  and later a single compile-fixer. "Disjoint files" is the only thing that makes
  parallel executors safe; it is false here.
- **Fix:** either make lanes genuinely sequential on the shared files
  (Lane 1 = WI-3 then **wait** for WI-2, then WI-4, WI-5) or assign shared files to a
  single lane and have other lanes make only header/API-level changes that the
  compile-fixer reconciles. Add explicit "depends on lane L2 item WI-x" notes.

### 5. MED-HIGH — WI-12's gate omits WI-10, so the FramePipeline can be landed while `flushRetired` still joins socket threads on the game thread
- **Evidence:** WI-12 Phase 0 DRAIN (plan-initial.md:433-435) explicitly lists
  "bridge retire :783-788". Verified in the tree: `run()` at Minecraft.cpp:762; the
  bridge-retire + `multiplayerSession_.flushRetired()` block is at Minecraft.cpp:782-788,
  and `ClientNetworkHandler` dtor joins `joinServerThread_` (Minecraft.cpp:
  `MultiplayerSession.cpp:12-19` retire pattern destroys the `Connection`, whose dtor
  joins reader/writer — Connection.cpp:134-137, HZ-05). The WI-12 dependency line
  (plan-initial.md:506-508) lists only WI-4/6/7/8/9; the gate (plan-initial.md:517)
  is "all of WI-4/6/7/8/9". WI-10 (async socket teardown, the fix for HZ-05/H1) is
  **not** required.
- **Why it matters:** the whole point of WI-12 is to eliminate main-thread blocking
  stalls in the loop (plan §1.5, HZ-05). Landing WI-12 without WI-10 codifies the
  30 s socket-join stall (network-threading.md:302-315) into the "canonical" pipeline.
- **Fix:** add WI-10 to WI-12's dependency list and to the gate, or state explicitly
  that `flushRetired` remains blocking until WI-10 lands.

### 6. HIGH — WI-3's own "lock ordering" rule is under-specified w.r.t. the lighting worker's existing lock discipline; R2 spin stays
- **Evidence:** the plan (WI-4, plan-initial.md:206-223) keeps `retireFromLighting`
  (ChunkCache.cpp:62-68 spin, HZ-03) as-is except for "CV signal on pin release"
  (synthesis §6:550). But `retireFromLighting` spins on `beginRenderEviction()`
  (Chunk.hpp:205-224), which is released by workers holding pins during
  `captureSnapshot` (ChunkBuilder.cpp:253-258) and lighting `runUpdate`. WI-3 adds a
  per-chunk lock inside `setLight` and `copyChunkBand`; `retireFromLighting`'s eviction
  protocol does **not** take that lock, and the plan never says whether the light-lock
  and the render-pin/eviction path compose (does a lighting worker take the light lock
  while a pin is held? does `beginRenderEviction` need the light lock?). Unspecified
  lock order → deadlock risk (worker holds light-lock, main holds eviction path).
- **Why it matters:** R2 is a real stall (council-chunk-workers.md:204-207) and
  correct composition of the two synchronization mechanisms is precisely the kind of
  subtlety that silently regresses.
- **Fix:** in WI-3, state the ordering invariant explicitly: light-lock is always
  acquired *after* pin acquisition and *before* nibble write/copy; `beginRenderEviction`
  must not be called while holding a light-lock; document in Chunk.hpp:205-224.

### 7. MED-HIGH — WI-6's "own bounded lane inside the same Compute pool" is incoherent: WorkerState is keyed by `std::thread::id`; a lane inside a shared pool does not give a stable thread
- **Evidence:** WI-6 (plan-initial.md:267-271) recommends "give lighting its own
  bounded lane (own channel, own sub-priority) inside the same pool so `WorkerState`
  keyed by `std::thread::id` still works". But a shared Compute pool dispatches tasks
  to any worker; a "lane" is a queue, not a thread pin. `WorkerState`/pin-cache is
  per-worker-thread (LightingEngine.cpp:50-56, :207-233, council-chunk-workers.md:123-124).
  QD-03 (synthesis.md:396-399) is explicitly left "must be confirmed before WI-6",
  and the plan itself (plan-initial.md:535-537, D3) admits it. The plan proceeds as if
  resolved.
- **Why it matters:** audit item 3/4 — lighting box-conflict correctness (HZ-34/35)
  depends on per-worker pin-cache coherence; if lighting tasks float across Compute
  workers, pins/claims are wrong or `WorkerState` must be re-keyed per-box (a design
  change the plan does not specify).
- **Fix:** resolve QD-03 in the plan: either (a) pin dedicated Compute workers to the
  lighting lane (sub-pool), or (b) re-key `WorkerState` per-claimed-box (spell out the
  change to tryClaimBox :149-170 / threadLoop :187-206). Do not leave it "see D3".

### 8. MED-HIGH — Test wiring gap: the plan's own regression net and WI-8/WI-11 verification reference test files that are NOT in the ctest build
- **Evidence:** 7 test files exist on disk but are absent from both
  `MINECRAFT_TEST_SOURCES` and `MINECRAFT_SERVER_TEST_SOURCES`
  (CMakeLists.txt:339-384): `block_face_uv_test.cpp`, `color_targets_test.cpp`,
  `custom_uniforms_test.cpp`, `handshake_metadata_test.cpp`,
  `iris_hemisphere_chunk_offset_test.cpp`, `pack_blend_drawbuffer_test.cpp`,
  `shadow_celestial_modelview_test.cpp`. The plan's §6 "regression net" row
  (plan-initial.md:591) lists six of these as "must stay green for every WI"; WI-8
  verification (plan-initial.md:322) references `tests/custom_uniforms_test.cpp`; WI-11
  verification (plan-initial.md:416) references `tests/handshake_metadata_test.cpp`.
  None of them are compiled or run by `build-omega.ps1 -RunTests` / `gtest_discover_tests`.
- **Why it matters:** audit item 5 — "keep every currently-passing test green"
  (plan-initial.md:11) is unverifiable for these files, and executors will be
  instructed to run tests that the suite does not execute. Worse, custom_uniforms and
  handshake_metadata are exactly the files WI-8/11 claim as coverage.
- **Fix:** either wire the 7 files into the CMake test lists (and confirm they build),
  or remove them from the plan's test matrix and verification lists. Also add the two
  missing-but-relevant existing tests that the plan's matrix omits: `mp_parity_updates_test.cpp`
  (exercises `Connection::tick`/`disconnect`/packet ordering — directly hit by WI-9/10)
  and `render_settings_test.cpp` (chunk-update budget math feeding `compileChunks` —
  touched by WI-12).

### 9. MED — HZ-08's worst offender (`downloadPendingMods`) has NO work item; only the easy pieces were moved
- **Evidence:** synthesis §2 HZ-08 (synthesis.md:208-214) cites the "worst"
  main-thread blocker as `downloadPendingMods` (ClientNetworkHandler.cpp:258-320,
  unbounded sequential HTTP + zip + install), plus `TextureManager::getTextureId`
  decode/upload (TextureManager.cpp:451-483) and sync stat-save on every `setScreen`
  (ScreenStack.cpp:33-38). The plan's WI-11 file list (plan-initial.md:389-410) covers
  ImageDownload, SessionValidator, ClientDiagnostics, MultiplayerConnector,
  ResourceDownloadThread, LoginScreen, SessionRestore, World::save, DedicatedServerGui —
  **not** `downloadPendingMods`, **not** TextureManager decode, **not** ScreenStack.
- **Why it matters:** audit items 1 and 7 — the user's ask is "entire main thread
  handling restructured entirely" and "using C++ and multithreading to your advantage
  properly". The plan defers or omits precisely the biggest main-thread stall while
  moving the small ones.
- **Fix:** add an explicit WI (or a clearly-labeled deferred decision with rationale)
  for `downloadPendingMods` → Io-pool staged download + main-thread apply; state that
  `getTextureId` decode and stat-save stay main-thread *by decision*, not by omission.

### 10. MED — Open decisions left dangling with no plan-side resolution, including two the plan itself says must be confirmed
- **Evidence:**
  - QD-18 (synthesis.md:443-446) — "who clears `meshJobInFlight` when a queued job is
    dropped" is the subtle core of WI-4's non-blocking cancel, and synthesis §8.4
    (synthesis.md:678-679) says "Decide QD-18 before WI-4". Plan WI-4 (plan-initial.md:213-215)
    only says "epoch/generation token so owners learn", never who clears the flag or how
    `sweepRetiring` (WorldRenderer.cpp:291-300) reaps safely.
  - QD-17 (synthesis.md:440-442) — the unified per-frame budget yield priority
    (network ≥ near-mesh ≥ lighting ≥ distant ≥ integrates) is undefined; WI-12
    (plan-initial.md:445-449) says "one shared per-frame deadline" but no drain order.
  - QD-14 (synthesis.md:430-432) — "no two verifies for the same connection in flight"
    (per-connection in-flight flag on Io domain) is referenced by no WI.
  - QD-26 (synthesis.md:465-468) — chunk-packet rate gate parity (`field_20175_w`) is
    never addressed (WI-9 keeps `preferImmediate`, Connection.cpp:296-312, without
    deciding parity).
  - QD-01/D1 (synthesis.md:388-392, plan-initial.md:527-531) is self-contradictory:
    "start with a blocking NetIo pool" vs "WI-10 keeps per-connection threads
    registered". WI-10 (plan-initial.md:371-372) leaves it as "either…OR…".
- **Why it matters:** executors cannot decide architecture policy; the plan must.
- **Fix:** plan master resolves QD-18/17/14/26 and rewrites D1/WI-10 to one option.

### 11. MED — WI-13's registration of the log dispatcher with Lifecycle contradicts QD-06 ("coordinator must not own the log thread")
- **Evidence:** QD-06 (synthesis.md:406-408) — "the log writer must be usable
  before/after coordinator shutdown; coordinator must not own the log thread."
  WI-13 (plan-initial.md:469-472) says Logging.cpp:129-177 "register with Lifecycle",
  and Logging uses **leaked** singletons that "outlive main-thread statics"
  (concurrency-inventory.md:405-411). If Lifecycle::shutdown() unblocks/joins the log
  thread (arch §6:409-410), then crash-handler logging after shutdown
  (Minecraft.cpp:837-841, which WI-13 also routes through Lifecycle) writes to a dead
  thread.
- **Why it matters:** teardown ordering is the whole point of WI-13 (QD-22); this
  contradiction can turn a crash-path log into a hang.
- **Fix:** define the log thread's contract explicitly: register for
  unblock/watchdog only, never join (or keep logging re-entrant post-shutdown), and
  state the ordering vs crash handlers.

### 12. LOW — WI-1's `ThreadBudget` surface is under-specified: `glDriverThreads()` is referenced by WI-2 but not defined in WI-1's API list
- **Evidence:** WI-2 (plan-initial.md:160) routes `GLCore.cpp:289` to
  `budget().glDriverThreads()`. WI-1's API list (plan-initial.md:130-134) has
  `configure/pool/budget/reserveDynamic/releaseDynamic/totalPending/shutdown` and the
  `thread_budget_test.cpp` spec only pins `compute`/`io` formulas (plan-initial.md:144-145).
  `glDriverThreads` (the `GL_KHR_parallel_shader_compile` hint, GLCore.cpp:286-291)
  is never defined.
- **Why it matters:** minor; an executor must invent the field.
- **Fix:** add `glDriverThreads()` to the ThreadBudget API in WI-1 and to the budget
  test.

### 13. LOW — Plan §6 test matrix and new-test list duplicate/forget items; `region_snapshot_race_test` and `gl_state_affinity_test` are static-analysis-ish, not behavioral
- **Evidence:** §6 (plan-initial.md:593-600) lists 14 new tests; §3 WI-4/WI-5
  verification (plan-initial.md:227-251) describes `mesh_cancel_test` /
  `gl_state_affinity_test` whose assertions ("static-analysis-style: verify
  `setAlphaTestRef` has a TL_DOMAIN guard under #ifdef") are not GoogleTest-runnable
  as described. Also the §6 "regression net" row (plan-initial.md:591) includes
  `lab_pbr_mipmap_test.cpp` which IS wired, mixed with 6 unwired files (FINDING 8),
  which will confuse executors about what actually runs.
- **Why it matters:** auditors/executors need a single accurate runnable-test list.
- **Fix:** restate each new test's concrete assertion; move static-analysis checks to
  a compile-fixer grep checklist, not a GTest.

### 14. LOW — Plan never records the accepted "C++ port is the authoritative client mirror" assumption (QD-16/M5), despite relying on it for every parity claim
- **Evidence:** synthesis M5/QD-16 (synthesis.md:484-485, 436-439) says the client
  Java tree is absent and the C++ port is the authoritative mirror; the plan relies on
  this for §2 NEVER-PARALLELIZE and WI-12 parity but never states it as an assumption
  (plan §7 only flags CONTEXT.md, plan-initial.md:606-608). It also never explicitly
  states the external-process server divergence (D11) in the target architecture.
- **Why it matters:** audit item 4/7 — an auditor downstream has no recorded basis for
  the parity contract.
- **Fix:** add a short "Accepted assumptions" section to the plan (client mirror;
  external-process server; audio kept pinned per QD-12; Iris 26.1 as the render-order
  reference).

---

## Orphan / coverage cross-check summary (audit item 1)

Hazards HZ-01…42 (synthesis §2:546-589): all 42 have an owning WI or are explicitly
"note/out-of-scope" **except** the sub-parts of HZ-08 that are neither moved nor
explicitly deferred (FINDING 9). Open decisions QD-01…27: all have a plan D-number or
WI **except** QD-14, QD-17, QD-18 (partial), QD-26 (FINDING 10) and the 
audio/watcher "pinned registered" execution of QD-12 — **no WI registers the audio
threads or pack-dir watcher with the coordinator** (plan §8 has no file for
XAudio2Backend.cpp or render/pipeline/Manager.cpp; QD-12/D12 only says "keep").

## Verdict

**NOT buildable-by-executors in its current form.** Must change first:

1. **WI-2 rewrite** (FINDINGS 1, 2): include `WorkerHandoff.hpp:14`; decide
   single-shared-Compute-pool vs per-owner subdivision with a real worked table;
   otherwise the centerpiece of the refactor does not compile and does not fix
   oversubscription.
2. **WI-3 correction** (FINDINGS 3, 6): add `Chunk::setBlock` to the lock set; state
   the light-lock / render-pin composition invariant.
3. **Dependency & lane fix** (FINDINGS 4, 5): cross-lane edges (WI-4←WI-2,
   WI-10/11←WI-1, WI-12←WI-10) must be honored; shared files assigned to one lane.
4. **Resolve QD-03/17/18 before WI-4/WI-6/WI-12** (FINDINGS 7, 10) — these are
   correctness cores, not style.
5. **Fix the test wiring** (FINDING 8) so "green" is meaningful.

Once those are fixed, the plan is a solid, well-evidenced ordering with good parity
documentation; the remaining findings (9-14) are scope/robustness items that can land
as edits in the plan-master stage.
