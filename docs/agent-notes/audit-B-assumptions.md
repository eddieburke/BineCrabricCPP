# AUDIT B — Wrong-Assumption / Risk Audit of `plan-initial.md`

Pipeline stage: **plan auditor B**. Inputs: `RULES FOR AGENTS.md`, `synthesis.md`,
`plan-initial.md`, council docs (architecture-proposal, chunk-workers, concurrency-inventory,
network-threading, mainthread-map, java-thread-model). **No source files edited; nothing built.**

All file:line references below were re-verified against the current working tree (2026-08-01).
The tree has large uncommitted edits (Minecraft.cpp, WorldRenderer.cpp, GLCore.cpp, etc.), so a
few council numbers are stale; the ones cited here are the live ones.

---

## FINDINGS

### F1 — HIGH — WI-2 is not compile-safe as written: `WorkerHandoff.hpp:14` default-arg site is missing from the WI-2 file list

**Finding.** WI-2 deletes `WorkerPool::recommendedThreadCount` (`WorkerPool.hpp:61-68`) and lists the
call sites to touch as ChunkBuilder.hpp:156, LightingEngine.cpp:49, ChunkCache.cpp:217/:224,
ShaderCompileService.cpp:47, GLCore.cpp:289. It **omits** the one remaining reference to the function:
`WorkerHandoff.hpp:14`, `explicit WorkerHandoff(unsigned threadCount = WorkerPool::recommendedThreadCount(2, 2))`.

Because `WorkerHandoff` is a class template that is still instantiated at the WI-2 stage
(`ChunkBuilder.hpp:155-156` still constructs `WorkerHandoff<ChunkMeshJob>` with an explicit count;
WI-2 explicitly keeps each owner's pool object), the constructor's *default argument* is part of the
instantiated signature. Referencing a deleted function from a default argument is a hard error at
instantiation even if the argument is never passed. Result: the batch fails to compile, and the
compile-fixer must hunt this down.

**Evidence.** `WorkerHandoff.hpp:14`; `ChunkBuilder.hpp:154-157`; plan-initial §3 WI-2 file list (does
not include WorkerHandoff.hpp). The synthesizer flagged this exact site in M3 (synthesis.md §4.1 M3,
§8 item 5) but the plan never absorbed it.

**Fix.** Add `src/net/minecraft/util/concurrent/WorkerHandoff.hpp:14` to WI-2 (delete the default
argument or replace it with a `ThreadCoordinator::instance().pool(...)` default), so it is fixed in
the *same* batch that deletes the function. Executors must grep for `recommendedThreadCount` after the
delete and remove every remaining reference.

---

### F2 — MED — `pendingScreenResize` phase placement contradicts "tick order preserved byte-for-byte"

**Finding.** The plan's WI-12 Phase 0 (DRAIN) lists `pendingScreenResize` as a drain-phase item, but in
the current tree the resize is handled **inside `tick()`** at `Minecraft.cpp:658-661`, i.e. after
`input::beginFrame` (`:650`), `screenStack_.tickScreens` (`:651`) and `multiplayerSession_.tick`
(`:655-657`). `resize()` calls `render::core::viewport` + `currentScreen()->init(...)`
(`DisplayManager.cpp:135-147`). If WI-12 actually moves it to Phase 0 (before input capture), the
screen is re-inited before input settles — a one-frame visible behavior change (resize handling,
`connectScreen->init`, cursor state). If it stays in `tick()`, the Phase-0 list is wrong and executors
will misplace it.

**Evidence.** `Minecraft.cpp:658-661`, `:650-657`; plan-initial WI-12 Phase 0 list.

**Fix.** Pin down one of the two: either keep `pendingScreenResize_` handling inside `tick()` (and
remove it from the Phase-0 list), or explicitly document the reorder as an intentional behavior change.
Do not leave it ambiguous.

---

### F3 — MED — "mesh uploads move to the mailbox" must not move the call sites relative to `present`

**Finding.** The plan's WI-12 says budget sources and drains "move to the mailbox (mesh uploads,
shader poll, texture stream, chunk integrates)". Today those drains run at fixed, GL-framing points
*inside the render pass after present*: `worldRenderer->compileChunks(*camera, false)`
(`GameRenderer.cpp:1195`, called from `onFrameUpdate`, which runs at `Minecraft.cpp:708`) and
`shaderPacks_->poll()` (`GameRenderer.cpp:711`, via `ProgramCache::poll` at ProgramCache.cpp:159).
`pumpAndPresent` runs earlier at `Minecraft.cpp:694`.

If a mailbox drain for mesh uploads is hoisted ahead of `pumpAndPresent` (or the shader poll ahead of
`beginSceneCapture`), a completed mesh links/uploaded before present would be drawn this frame instead
of next — a visible pop-in/latency change, and Iris frame-data (`flipped`/`partialTick`) would see the
upload one frame early. The claim "render order preserved byte-for-byte" is only true if the mailbox
replaces the *internals* of `compileChunks`/`poll` at the *same call sites*.

**Evidence.** `Minecraft.cpp:670-761` (`runRenderPhase` order); `GameRenderer.cpp:1195`, `:711`;
plan-initial §3 WI-12 (runRenderPhase bullet), §1.3.

**Fix.** State explicitly in WI-12 that `compileChunks` remains invoked from `onFrameUpdate` at
`GameRenderer.cpp:1195` (after present) and `shaderPacks_->poll()` at `GameRenderer.cpp:711`, and that
only the *drain internals* move to the channel. Add `tests/frame_pipeline_order_test.cpp` asserting
present-before-compileChunks-before-poll.

---

### F4 — MED — Lifecycle watchdog cannot reach the blocking joins inside session-clear; "log-and-leak" is unsafe for GL-context-affine workers

**Finding.** WI-13's order (fence → unblock → stop+drain → watchdog join → session-clear → GL destroy)
matches `Minecraft::stop()` today (ShaderCompileService::stop :354 → setWorld(nullptr) :365 →
RegionIo::flush :370 → clearAllocatedTextures :374 → serverProcessCoordinator :378 → mod host :380 →
audio :381 → InputSystem :383 → DisplayManager::destroy :388 → `_Exit` :391). The gaps:

1. The blocking work lives *inside* the owners, not at the Lifecycle boundary. `setWorld(nullptr)` →
   `WorldSession::clearWorld` (`WorldSession.cpp:26-41`) → `ownedWorld_.reset()` → `World::~World`
   (`World.cpp:724-727`) → `ChunkCache::~ChunkCache` → `waitForPendingWrites()` + `loaderPool_->drain()`
   (`ChunkCache.cpp:27-36`), and `Connection::~Connection` → `disconnect()`+`joinThreads()`
   (`Connection.cpp:134-137`). A stuck disk write (`saveCompleteCv_`, ChunkCache.cpp:356-362) or a
   blocked socket join blocks the main thread *inside* a Lifecycle "phase", so an external watchdog
   never gets a chance to fire. The watchdog is decorative unless Lifecycle injects a deadline into
   each owner (passing a `stop_token`/deadline into the drain calls).
2. The WI-13 "watchdog leaks" a hung worker and continues to `DisplayManager::destroy`, which runs
   `glfwTerminate` (`Window.cpp:363-373`) — tearing down every GL context, including the shared
   contexts held by leaked GlCompile workers. It is only safe because `std::_Exit(0)` (`Minecraft.cpp:391`)
   follows, so the leaked worker's crash is never observed. This must be stated, otherwise an executor
   that removes `_Exit` (to run destructors) turns the leak into a crash.

**Evidence.** `Minecraft.cpp:347-393`, `:391`; `WorldSession.cpp:26-41`; `World.cpp:724-727`;
`ChunkCache.cpp:27-36,356-362`; `Connection.cpp:134-137`; `Window.cpp:363-373`.

**Fix.** In WI-13, specify that each blocking owner (ChunkCache drain, Connection join, audio joins,
`ServerProcessCoordinator` 10 s wait) takes a *deadline/stop* argument from Lifecycle; keep the
`_Exit(0)` fallback; and record that GL-context-affine workers must never be "leaked" *before*
`DisplayManager::destroy` (leak only after, or destroy the shared contexts first).

---

### F5 — MED — The "stuck meshJobInFlight" framing in WI-4 is imprecise; the destructor already clears it, and the real leaks are the loader `PendingLoad` and the R3 invariant

**Finding.** `~ChunkMeshJob` sets `builder->meshJobInFlight = false` (`ChunkBuilder.cpp:220-225`). A
task dropped by `WorkerPool::cancelPending()` (`WorkerPool.hpp:45-48`) destroys its lambda, which
destroys the captured `shared_ptr<ChunkMeshJob>`; if that was the last reference the destructor runs on
the thread that called `cancelPending()` — today always the main thread (`clearSections`,
`WorldRenderer.cpp:373`; `WorkerHandoff` dtor, `WorkerHandoff.hpp:16-18`) — so the flag *is* cleared.
The plan's claim that rank-#3 "meshJobInFlight stuck" is *fixed* by an epoch token is therefore
misattributed: the epoch token is still the right design (explicit cancellation acknowledgement, and it
protects the R3 invariant when WI-4 wires the scheduler onto a shared Compute pool where a
cancel-from-worker path could drop the last reference), but the plan should not claim the flag is
stranded today.

The genuine silent-drop leak is the loader: `requestChunkAsync` (`ChunkCache.cpp:226-249`) leaves a
`PendingLoad` with `done==false` in `pendingLoads_` when its queued task is cancelled; nothing notifies
or removes it, and `integrateFinishedLoads` (`:251-288`) only scans for `done` entries. That part of
WI-7 is correct and necessary.

**Evidence.** `ChunkBuilder.cpp:220-225`; `WorkerPool.hpp:45-48`; `WorkerHandoff.hpp:16-18,35-40`;
`ChunkCache.cpp:226-249`; `WorldRenderer.cpp:373`.

**Fix.** Rewrite the WI-4 rationale: cancel clears the flag today *because* destruction is
main-thread-only (R3); the epoch token is required once the pool is shared so a worker cannot drop the
last `shared_ptr` (assert/diagnose the main-thread-only destructor, or make the write atomic). Keep
WI-7's PendingLoad cancellation as the real fix for the loader silent drop.

---

### F6 — MED — Unbounded `readQueue_` memory growth: the read-side cap is still an open decision (D2), not a committed fix

**Finding.** `Connection::readLoop` pushes every decoded packet into `readQueue_` with no cap
(`Connection.cpp:272-275`); the game thread drains it in a 3 ms time-box (`tick`, `:198-221`). A peer
that outpaces the sim leaves `readQueue_` unbounded (client RAM growth / OOM — the audit's "memory
growth" risk). The plan's only mitigation is WI-9's read-side cap, but the policy is still an **open
decision** (`D2`, plan §5), i.e. uncommitted. The risk register does not state the unbounded-growth
risk at all.

**Evidence.** `Connection.cpp:272-275`, `:198-221`; plan-initial §5 D2, WI-9.

**Fix.** Commit D2 before WI-9 lands (recommend the plan's own recommendation: bounded read queue +
`disconnect.overflow` on the mirror `0x100000` constant, `Connection.cpp:185-187`), and add a risk-register
line for unbounded `readQueue_` growth with the chosen bound.

---

### F7 — LOW — WI-3's lock-order note is incomplete: the new per-chunk lock's nesting partners are `outboxMutex_`/`queueMutex_`, and the register never covers `ioMutex_`/`readMutex_`/`writeMutex_`

**Finding.** WI-3's risk note is "never hold chunk lock while taking `queueMutex_`". After WI-3, the
writers of light arrays are `LightingEngine::setBrightness` (`LightingEngine.cpp:249-258`) and the
snapshot copy (`RegionSnapshot.cpp:54-66`). Inside `runUpdate`, `setBrightness` (chunk-lock scope) is
followed by `outboxMutex_` (`LightingEngine.cpp:422`) — a new edge the plan's rule doesn't name.
Today I found **no** actual cross-nesting hazard: `outboxMutex_` and `queueMutex_` are never held
simultaneously (`runUpdate` runs outside `queueMutex_`, threadLoop `:187-206`); `ioMutex_`
(`ChunkCache.hpp:89`) is never nested with the lighting mutexes; `readMutex_`/`writeMutex_`
(`Connection.hpp:106-107`) are used on disjoint sides. But WI-3 introduces the chunk lock, so the
ordering rule should enumerate all lock-acquisition edges (chunk lock → never hold while acquiring
`queueMutex_` **or** `outboxMutex_`; `ioMutex_` stays leaf-order-only).

**Evidence.** `LightingEngine.cpp:249-258,422`, `:187-206`; `ChunkCache.hpp:89`; `Connection.hpp:106-107`;
plan-initial WI-3 risks.

**Fix.** Expand the WI-3 lock-order note to name `outboxMutex_` and state that `ioMutex_`,
`readMutex_`/`writeMutex_` have no cross-nesting with the chunk lock today (verified), keeping the
chunk-lock scope tiny per the plan.

---

### F8 — LOW — Lua/mod-state thread-safety and the `mod.hook` paths are absent from the risk register

**Finding.** The audit's "thread-safety of Lua mod state" and "mod.hook paths (LuaRenderBindings /
ModHost stateMutex)" are not in the plan's risk register. Verification: the only mod entry from a mesh
worker is `drawLuaBlockWorld` (`ModModels.cpp:616-630`), which iterates *pre-baked* quads and does not
re-enter Lua; its `ctx.textureManager` is null on the worker constructor (`BlockRenderManager.cpp:30-33`),
so `bindTextureFor` is a no-op (`BlockRenderContext.hpp:245-255`). All `luaHook*`/binding sites run on
the main thread (e.g. `luaHookRenderFrame`, `Minecraft.cpp:703`). `ModHost::LoadedLuaMod::stateMutex`
(`ModHost.hpp:65`) is a recursive mutex used only from main-thread Lua entry points. So the state is
currently main-thread-only, but this is an **unstated invariant** that WI-5/WI-12 must not break (e.g.,
an executor "optimizing" a mod draw onto a worker, or moving `luaHookRenderFrame` relative to present).

**Evidence.** `ModModels.cpp:616-630`, `:571-576`; `BlockRenderContext.hpp:245-255`;
`BlockRenderManager.cpp:30-33`; `ModHost.hpp:65`; `Minecraft.cpp:703`.

**Fix.** Add a risk-register line: "Mesh/compute workers must never call Lua or enter `ModHost`; all
`luaHook*` remain main-thread. `luaHookRenderFrame` stays in the render phase (main)." Re-run the
`TL_DOMAIN` debug assert from WI-5 across the mod draw entry points.

---

### F9 — LOW — "GL context loss on window close" is not a named risk (ordering covers it today, but it must be stated)

**Finding.** Window close → `isCloseRequested` (`Window.cpp:386-388`) → `scheduleStop`
(`Minecraft.cpp:793-795`) → loop breaks → `stop()` → `ShaderCompileService::stop()` (joins workers,
destroys worker windows: `ShaderCompileService.cpp:63-86`) before `DisplayManager::destroy()`
(`Minecraft.cpp:354` vs `:388`). The ordering is correct and WI-13 encodes it. But the plan never lists
"context loss / context destruction while a worker still holds a shared context" as a named risk; a
future executor reordering stop() (or removing the `_Exit`) would reintroduce it silently. Driver-reset
context loss (TDR) is out of scope and should be noted as such.

**Evidence.** `Minecraft.cpp:347-393`; `ShaderCompileService.cpp:63-86`; `Window.cpp:386-388`.

**Fix.** Add a risk-register line and keep the ordering assertion (`ShaderCompileService::stop` before
`DisplayManager::destroy`) as an encoded invariant, not a comment.

---

### F10 — LOW — WI-12/WI-13 citation drift (plan says wrong line ranges for things it claims to preserve)

**Finding.** Two concrete mis-citations in the plan, both for files whose behavior WI-12/WI-13
promise to preserve:

- WI-12 "inactive sleep :705-710" — the inactive sleep block is at `Minecraft.cpp:714-721`.
- WI-13 "Minecraft.cpp:837-841 crash handlers" — that range is the FRAME stall-trace writer
  (`#ifdef MINECRAFT_RENDER_TRACE`, `:824-843`); the crash entry is `Minecraft::gameCrashed`
  (`Minecraft.cpp:248`) and the post-loop `stop()` at `:860`.

**Evidence.** `Minecraft.cpp:714-721`, `:824-843`, `:248`, `:860`.

**Fix.** Correct the citations; instruct executors to grep before editing (already the plan preamble —
keep enforcing it).

---

### F11 — LOW — WI-12 is a single oversized item; recommend a behavior-neutral scaffold step so the compile-fixer has a smaller fix surface

**Finding.** The pipeline's constraint is that only the compile-fixer builds at the end, and only once
(per RULES §7 "Only the compile fixer ... may build"). If WI-12 lands as one large item with a
non-building intermediate state, the entire batch fails and the fixer must repair a very large surface
in one pass (F1 is an example of how one missed site breaks the batch). The plan already orders items
to be build-safe, but WI-12 touches `Minecraft.cpp` run/render, adds three new classes, replaces two
budget sources and rewires five drains — a single commit that is very unlikely to be compile-clean on
the first try.

**Evidence.** plan-initial §3 WI-12, §4 dependency graph ("Gate for WI-12: all of WI-4/6/7/8/9").

**Fix.** Split WI-12 into (a) additive `FramePipeline`/`TaskMailbox`/`FrameProfiler` scaffolding that
is compiled but inert (behavior-neutral), then (b) the run/render rewiring, then (c) budget-source
consolidation. Each sub-step compile-safe on its own.

---

## VERDICT

**The plan's architecture is sound and its central assumptions verify.** The domain-pool +
`ThreadCoordinator` model, the "main thread = GL thread" invariant, the R1 fix shape, the ordering of
`runRenderPhase`, and the WI sequencing are all validated against the code. **However the plan is not
executable as-is in one batch**: F1 is a guaranteed compile break (missing `WorkerHandoff.hpp:14`
site), F2/F3 are parity ambiguities that will change visible behavior if executed literally, and F4 is
a feasibility gap in the watchdog semantics. F1-F4 should be resolved by the plan master before any
executor starts; F5-F11 are clarifications/risk-register completions.

Verified TRUE:
- GL shared-context windows for shader-compile workers (`ShaderCompileService.cpp:8-19,45,52,225`).
- Main thread is the only GL-context thread besides the shader-compile workers.
- Chunk-mesh workers issue **no GL calls** (`buildMesh` is CPU-only; `bindTextureFor` is a no-op on
  workers because `ctx.textureManager` is null); the only worker→GL-state leak is the documented
  `g_alphaTestRef` global write (`ModModels.cpp:626`, `RenderCore.cpp:70,671-680`) addressed by WI-5.
- GPU upload already runs on the main GL thread (`compileChunks`→`uploadMesh`, `WorldRenderer.cpp:731-770`,
  called at `GameRenderer.cpp:1195`); "defer upload to GL thread" = keep it in the render phase.
- Domain separation is required and correct: shader compile needs a GL-shared context bound for the
  thread lifetime; network reader/writer block on sockets (`Connection.cpp:262-281`); audio uses four
  dedicated XA2 threads (`XAudio2Backend.cpp:617-628`). Folding them into one Compute pool would break
  all three. The plan's domain set matches the code.
- `runRenderPhase` internal order and present-before-draw are as the plan lists (QD-07 confirmed);
  Java's render-after-tick and the port's double-buffer rhythm are preserved by keeping that order.
- World teardown stops lighting before freeing chunks (`World.cpp:724-727` → `ChunkCache.cpp:27-36`);
  `Minecraft::stop()` orders shader-context teardown before the primary-context destroy — "workers
  before World teardown" holds and WI-13 encodes it.
- R1/R4 snapshot race is real (`RegionSnapshot.cpp:54-66` memcpy vs `LightingEngine.cpp:249-258` and
  main-thread writers); render pins protect eviction only (`Chunk.hpp:205-225`); WI-3's per-chunk lock
  is the right shape.
- `drain()`/`cancelAll()` block the main thread and `cancelPending()` silently drops queued work —
  the architecture-doc criticism is validated.
- `recommendedThreadCount` call-site inventory (4 live sites + 2 `hw−2` sites + 1 unused default at
  `WorkerHandoff.hpp:14`) is accurate.
- No Lua/mod state is currently touched off the main thread; all `luaHook*`/bindings are main-thread.
- No cross-nesting deadlock exists today among `ioMutex_`/`outboxMutex_`/`readMutex_`/`writeMutex_`/
  `queueMutex_` (each is used on one side or never nested with the others).

Verified FALSE / needs correction:
- "cancelPending silently strands `meshJobInFlight` forever" — the destructor (`ChunkBuilder.cpp:220-225`)
  already clears the flag on cancel (main-thread drop); the genuine silent-drop leaks are the loader's
  `PendingLoad` (WI-7) and the fragile R3 invariant (WI-4 must preserve main-thread-only destruction).
- WI-2 is compile-safe — **it is not**: it misses `WorkerHandoff.hpp:14`.
- WI-12 preserves tick/render order byte-for-byte — **not literally**: `pendingScreenResize`
  placement and mesh-upload/shader-poll call sites are ambiguous and would change behavior if moved.
- WI-13's watchdog can enforce its own order — **partially**: the blocking joins are inside the owners
  (`WorldSession`/`ChunkCache`/`Connection`/audio), outside the watchdog's reach as specified.
- "Minecraft.cpp:837-841 crash handlers" and "inactive sleep :705-710" — wrong line ranges.
