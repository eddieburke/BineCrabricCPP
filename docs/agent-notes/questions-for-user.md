# QUESTIONS FOR USER — decisions needed before the final plan is locked

Pipeline stage: **auditor + preliminary-plan updater (AGENT 1)**. These are the open decisions that
need human input before AGENT 2 (plan master) locks the final plan. Each question gives: the question,
the options, the recommendation, and what breaks if it is not answered.

Source: QD-01…27 from synthesis.md, the unresolved audit-A/audit-B findings, and the parity
fix-vs-keep decisions from both parity audits. Recommended answers are in **bold**.

---

## Q1 — Scope: how much of the `Minecraft::run()` restructure lands in THIS pass?

The plan splits WI-12 into WI-12a (inert FramePipeline/TaskMailbox/FrameProfiler scaffold + shared
`FrameBudget::deadline` API — zero behavior change) and WI-12b (the actual rewiring of `run()` into the
phase pipeline, unified frame budget, drain relocation).

Options:
- **(a) Both 12a and 12b.** Full restructure now. Recommended only if you accept the higher regression
  risk on the game loop (mitigations exist: flag-gated, call-site-relative order test).
- **(b) 12a now, 12b in a follow-up pass** after the channel work (WI-4/6/7/8/9/10) proves itself.
- **(c) 12a now, 12b never** — just clean the loop shape and keep per-subsystem budgets.

Recommendation: **(a) is the honest reading of "entire main thread handling restructured entirely"; but
(b) is safer.** The pipeline is set up so 12b is gated on WI-4/6/7/8/9/10 anyway. If you answer
anything other than (a), the final plan marks 12b as optional/deferred.

**What breaks if unanswered:** the final plan cannot size executor-lane P2b, cannot state the gate, and
the "restructure the main thread entirely" ask is either over- or under-delivered.

---

## Q2 — The GLSL-330-vs-120 dialect divergence: fix now, keep, or defer?

Java Iris 26.1 forces **GLSL 330 core** on every shader and rewrites `attribute`/`varying`/`gl_*`.
The C++ port **keeps GLSL 120** for legacy packs and only core-ifies sources that declare `#version
130+`. This is the port's core deliberate divergence (parity-glsl T1/DP-1), and several Java-26.1-only
transforms (fragment `gl_TexCoord`/`gl_Color`/`gl_FragColor` rewriting, T2/T3) are therefore **missing
for 130+ packs** (a 130+ pack using `gl_TexCoord` compiles in Java, fails in C++).

Options:
- **(a) Keep 120 as the primary dialect and do NOT unify.** Add the missing modern-path transforms
  (T2/T3) as a separate dialect-completeness follow-up.
- **(b) Keep 120; defer even T2/T3.** Full out-of-scope for this refactor.
- (c) Migrate everything to 330 core like Java. Not recommended — breaks the legacy OptiFine-pack target
  this port exists for.

Recommendation: **(a)**. The threading refactor must not touch the dialect fork; the modern-path gaps are
real but belong to a shader-completeness workstream, not the multithreading refactor.

**What breaks if unanswered:** executors may "simplify" the two `lower*` paths in WI-8's seam work, or a
reviewer may flag T2/T3 as a bug this plan must fix, when it is out of scope.

---

## Q3 — Parity bugs: fix fogMode values and the entity-overlay path now, or keep as documented deviations?

Two bindings-parity items are pack-visible:

1. **fogMode**: C++ sends `0/1/2/3`; Java Iris sends GL constants `0/9729/2049` (GL_LINEAR/GL_EXP2).
   The vanilla pack's `common.glsl:18-26` currently *claims* "as Iris reports them" — which is false.
   Fixing means changing `FrameData.cpp`, `RenderCore.cpp`, the vanilla pack, and `CustomUniforms.cpp`
   **atomically**.
2. **Entity-overlay path**: Java derives `entityColor` from an `iris_overlay` sampler +
   `iris_Entity`/`iris_OverlayUV`/`iris_LightUV` (EntityPatcher). C++ uploads `entityColor` as a plain
   engine-set uniform and has no `iris_overlay`. Packs reading `entityColor` in fragment see different
   values.

Options:
- (a) Fix both now (WI-15 adds fogMode + first-frame-previous; entity overlay becomes a new, larger WI).
- **(b) Fix fogMode (cheap, HIGH pack-visible) in this pass; keep entity-overlay as a documented,
  out-of-scope gap** with a follow-up ticket.
- (c) Keep both as documented port contracts.

Recommendation: **(b)**. fogMode is a small, bounded, atomically-applicable fix that shipped packs can
hit; the entity-overlay path is a substantial new feature (vertex/geometry passthrough + sampler +
texelFetch) that does not belong in a threading refactor.

**What breaks if unanswered:** the parity decision table (§8.2) stays ambiguous; a pack that branches on
`fogMode == GL_LINEAR` silently takes the wrong branch and looks like a threading bug downstream.

---

## Q4 — Thread budget: what machine do we size the `ThreadCoordinator` budget for?

The corrected math (§5 of the plan) sizes `compute` from `hardware_concurrency()`. The bundled mingw64
toolchain machine (RULES §2) may be low-core, but the target user machines could be 8–16 logical. On
**4 logical cores**, `cpuBudget=2` leaves `compute=1` for mesh+lighting+gen combined — functional but
slow chunk gen; the plan needs to know whether that is acceptable or whether low-core machines need
special capping (e.g. cap audio decode threads, or raise `maxComputeThreads`).

Options:
- **(a) Budget for the typical user machine (8–16 logical); accept `compute=1` on 4-core as the honest
  floor.**
- (b) Budget for the lowest machine we must support (4-core): cap glCompile/io harder, document
  throughput loss.
- (c) Add a `maxComputeThreads` option the user can tune; default to 8.

Recommendation: **(a)** with the option (c) exposed in `ThreadCoordinator::configure`. The oversubscription
fix is the point; on 4-core a shared pool of 1 is still better than today's ~10 independent threads.

**What breaks if unanswered:** WI-2's worked table (§5.3) has no target; executors may either starve
low-core machines or reintroduce oversubscription "to be safe".

---

## Q5 — Is the dedicated server binary in scope for THIS pass?

The client and the in-process dedicated server are separate binaries (`src/server-main.cpp`). The
server has its own ~21-23 threads (tick thread, command thread, accept thread, 2/connection, loader 4,
save 1, light 3, log 1, GUI). The plan has WI-14 as deferred; `ServerProcessCoordinator` (the external
`minecraft_server.exe` spawned for singleplayer) is explicitly out of scope.

Options:
- **(a) Client only this pass; WI-14 (server coordinator wiring) deferred** to a follow-up. Server
  behavior unchanged; only the client gets the budget/coordinator.
- (b) Apply the coordinator to the server binary too (WI-14 lands in this pass).

Recommendation: **(a)**. The server is a separate process, structurally simpler (no render/GL, no
FramePipeline), and its oversubscription (≈22 threads) is less damaging. Keeping it out shrinks the
execution surface and the test matrix.

**What breaks if unanswered:** the plan cannot decide whether server-side `ConnectionListener` budget
accounting (WI-10 touches it) must be coordinated or left as-is; WI-14's lane slot is ambiguous.

---

## Q6 — Parity macro batch (WI-15): how aggressive on the missing-macro fixes?

The GLSL macro set has gaps vs Java: `MC_GLSL_VERSION` semantics (pack-version vs driver-version — HIGH,
changes `#if` outcomes), `MC_NORMAL_MAP`/`MC_SPECULAR_MAP` not unconditional, `MC_RENDER_QUALITY`/
`MC_SHADOW_QUALITY` missing, `IRIS_HAS_TRANSLUCENCY_SORTING`/`IRIS_TAG_SUPPORT` missing, `CAT_MOUNTAIN`/
`CAT_UNDERGROUND` missing, `IRIS_REQUIRES_SEPARATE_ENTITY_DRAWS` missing. Every macro change bumps the
shader disk-cache format.

Options:
- **(a) Define only the macros the engine actually implements** (`MC_RENDER_QUALITY`/`MC_SHADOW_QUALITY`
  =1.0, `CAT_*`, `IRIS_HAS_TRANSLUCENCY_SORTING`, unconditional `MC_NORMAL_MAP`/`MC_SPECULAR_MAP`,
  `MC_GLSL_VERSION`=driver version). Leave `IRIS_REQUIRES_SEPARATE_ENTITY_DRAWS` and `IRIS_TAG_SUPPORT`
  undefined (the engine has no separate-entity-draw mode and b1.7.3 has no tag system) — defining a
  macro that lies about capability is worse than not defining it.
- (b) Full set including the two capability macros, matching Java textually even where the engine
  doesn't implement the capability.

Recommendation: **(a)**. Fix the ones that make packs behave correctly; do not define capability macros
the engine cannot honor.

**What breaks if unanswered:** WI-15's scope is undefined; a wrong `#if` in a pack either disables a
feature the engine supports (silent) or enables a code path the engine can't feed (broken rendering).

---

## Q7 — Thread count: is oversubscription on the *worker* side the real target, or do we also budget the blocking/device threads?

The corrected math (§5) registers but does NOT deduct blocking threads (audio 4, log 1, network 2/conn,
watcher 1) from `cpuBudget`, on the argument that they block on devices/sockets. Audit-A's core
objection was that deducting them starves compute on low-core machines. This is a policy choice.

Options:
- **(a) Register, don't deduct** (the plan's §5 default). Runtime totals still drop ~27→19 on 16-logical.
- (b) Deduct pinned classes first (strict architecture §3.2 wording) — compute collapses on low-core,
  but the "total under budget" guarantee is absolute.
- (c) Deduct with a floor (`max(1, …)`), so low-core machines never go below 1 compute thread.

Recommendation: **(a)** for correctness of the CPU story, with the *counts* still authoritative via
`reserveDynamic`. The blocking threads are already near-idle; the oversubscription that matters is the
12 spinning compute/compile workers, which the shared pool eliminates.

**What breaks if unanswered:** the budget table (§5.3) has two defensible numbers; WI-2's comments and
the `thread_budget_test` would encode the wrong policy.

---

## Q8 — Read-side cap policy (D2/QD-02): disconnect-on-overflow or backpressure?

The inbound `readQueue_` is unbounded today; a fast/malicious peer can OOM the client. The overflow
check in `Connection::tick` is send-side only. Java's parity behavior is `disconnect.overflow` on
send-overflow; the read side is new territory.

Options:
- **(a) Bounded read queue + `disconnect.overflow`** mirrored on the existing `0x100000` constant
  (simple, matches the existing send-overflow pattern, self-cleaning).
- (b) Backpressure: stop reading when the queue is full, resume when drained (keeps the connection,
  more state).
- (c) Bound + drop-oldest (loses ordering, unsafe for entity/teleport packets).

Recommendation: **(a)**. It matches the existing overflow idiom, needs no new state machine, and
disconnecting a flooding peer is the safer multiplayer behavior.

**What breaks if unanswered:** audit-B F6 explicitly flags this as still-open; WI-9 cannot commit a cap,
and the unbounded-growth risk stays in the register unclosed.

---

## Q9 — Socket I/O: blocking NetIo pool now, or event loop?

Two kernel threads per connection parked in blocking `recv`/`send`; `disconnect()` joins them on the
caller (game/server) thread with 30 s timeouts — the biggest parity/responsiveness deviation. The fix
(WI-10) can either keep per-connection threads registered under the coordinator (smaller diff, keeps
the streambuf code) or fold into a small event-loop/IO pool (epoll/IOCP/WSAPoll-style).

Options:
- **(a) Start with per-connection threads registered as `Domain::NetIo` with `reserveDynamic(2)` +
  unblock-then-watchdog-join; defer the event loop.** The async *teardown* (never join on the game
  thread) is the important half and is scheduler-independent.
- (b) Move to an event-loop/IOCP now.

Recommendation: **(a)**. The blocking-streambuf reads can't be cancelled cheaply anyway (only
`shutdown(SD_RECEIVE)` unblocks them); the event loop is a bigger rewrite with no user-visible win until
there are dozens of connections.

**What breaks if unanswered:** WI-10's "either…OR…" (audit-A FINDING 10) stays ambiguous; executors
can't implement teardown semantics they don't know.

---

## Q10 — Dual-path lockstep: unify the Lua-mod and Iris render paths, or keep them in lockstep?

Parity-bindings found the real forks: world-vs-GUI matrix producers, frame-uniform camera reconstruction
vs live MatrixStack tops, world-vs-shadow cameras, and the Lua-mod world-mesh path calling main-thread GL
state setters from mesh workers. The last one is a real bug (WI-5). The others are deliberate parallel
paths that must stay in lockstep. Parity-glsl also found two independent `#if` engines (GLSL and
.properties) that could drift.

Options:
- **(a) Keep the dual paths, fix the one bug (WI-5), and document the lockstep invariants.** Do not
  unify in this refactor.
- (b) Unify the Lua-mod and Iris render paths into one pipeline.
- (c) Unify the two `#if`/`#define` engines into one state machine.

Recommendation: **(a)** for both. Unification is a large, risky rewrite orthogonal to threading; the
two `#if` engines are already tested and the GLSL one is deterministic per pack. WI-5 fixes the only
fork that is an outright bug.

**What breaks if unanswered:** executors may "simplify" a deliberate fork while touching the file for a
threading change (e.g. hoisting matrix computation to a worker, or merging the two `#if` engines) and
silently change shader output.

---

## Q11 — Lighting-ready gate (double-mesh fix): now or defer?

Freshly lit chunks are meshed before async lighting finishes, then re-invalidated and meshed again on
world load (double mesh work). The fix (WI-7 optional) marks a column "lit" only after its propagation
boxes drain and holds first mesh until then. Affects meshing volume, not correctness.

Options:
- **(a) Land it in WI-7** (cheap, reduces world-load mesh churn noticeably).
- **(b) Defer** to a follow-up; WI-7 ships without it.

Recommendation: **(a) if it stays a small change; (b) otherwise.** It is the one pure-wins optimization
in the chunk pipeline, but it touches `enqueueDirtyChunk`/`drainDirtyRegions` ordering that the shared
pool (WI-4/6) also touches — sequencing matters.

**What breaks if unanswered:** WI-7's scope is fuzzy; a lane may implement it in a way that fights WI-6's
channel rework or double-mesh stays and the world-load cost persists.

---

## Q12 — How aggressive: how many work items land in this pass?

The 7-executor shape (§10) covers 16 work items (WI-1…WI-16 + WI-T) across L1/L2 + P1a-c + P2a/b.

Options:
- **(a) Everything — all lanes land in one pass** (the "massive refactor" ask, maximum blast radius,
  one compile-fixer pass at the end).
- **(b) Foundational + P1 only** (WI-1/2/T, WI-12a, WI-3/4/5/9/10) in this pass; WI-6/7/8/11/15/16/12b/13/14
  as a second pass.
- (c) Foundational only (WI-1/2/T, WI-12a) as a pure-additive first pass.

Recommendation: **(a) is the stated task ("a few dozen things… restructured entirely"); (b) is the
conservative fallback** if you want the UB fixes (WI-3/5) and async teardown (WI-10) proven before the
channel/budget rework lands. The plan is already structured so each is shippable alone.

**What breaks if unanswered:** the executor-lane assignment and the compile-fixer's fix surface cannot
be finalized; you cannot tell whether to book one pass or three.
