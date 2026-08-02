# EXECUTOR-L2 REPORT — WI-12a (FramePipeline / TaskMailbox / FrameProfiler inert scaffold + FrameBudget deadline API)

Pipeline stage: **executor L2** (foundational lane, WI-12a). No builds, no tests, no git run (per RULES §2/§7 and the execution rules). Additive + inert; zero behavior change. File-disjoint from L1.

## 1. What changed (file:line)

### Edited (additive only)
- `src/net/minecraft/util/concurrent/FrameBudget.hpp`
  - Existing aggregate `FrameBudget {deadline, minItems, fromMs, hasRemaining}` **untouched** (lines 5-13).
  - Added nested `struct Deadline` — per-frame shared deadline type: `set(int ms)` (:17), `remaining()` returning `std::chrono::milliseconds` (:20), `hasExpired()` (:27), `point_` (:30) — lines 14-31.
  - Added `static Deadline& beginFrame(int ms)` (:33) — sets shared deadline to `now + ms` and returns it (matches the spec example "FrameBudget::beginFrame(int ms) sets frameDeadline_ = now + ms").
  - Added `static Deadline& frameDeadline()` (:39) — returns the shared per-frame instance (consumers call `frameDeadline().remaining()`).

### Created (new)
- `src/net/minecraft/client/util/FramePipeline.hpp` (23 lines) — `enum class Phase { Drain, Input, Ticks, Render, Pace, Diagnostics }` (:8); `kPhaseOrder` (:9-10); `kPhaseCount` derived via `sizeof` (:11); `phaseAt(index)` (:12); `count()` (:15); `run()` declared (:19); `tickPhase(Phase)` declared (:21).
- `src/net/minecraft/client/util/FramePipeline.cpp` (11 lines) — `run()` iterates the phase enum in order calling `tickPhase` (:3-7); `tickPhase` is an empty hook (:8-10). No dependency on `Minecraft`.
- `src/net/minecraft/client/core/TaskMailbox.hpp` (28 lines) — `using Task = std::function<void()>` (:11); `pushUrgent/pushTick/pushRender` (:12-14); `drainUrgent/drainTick/drainRender` (:15-17); `size()` (:18); private `drainOne` (:21); `mutable std::mutex mutex_` + three deques (:23-26).
- `src/net/minecraft/client/core/TaskMailbox.cpp` (40 lines) — pushes lock and `push_back` (:4-15); drains swap the deque out under lock then execute outside the lock (`drainOne`, :29-39) so a task may re-push without deadlock; `size()` sums the three queues (:25-28).
- `src/net/minecraft/client/util/FrameProfiler.hpp` (31 lines) — `using Phase = FramePipeline::Phase` (:11); `Record {Phase, duration}` (:12-15); `instance()` (:16); `beginFrame/beginPhase/endPhase` declared (:20-22); `recordCount()`/`records()` accessors (:23-24). Identical class layout with the macro on or off (ODR-safe).
- `src/net/minecraft/client/util/FrameProfiler.cpp` (29 lines) — all recording bodies gated on `#ifdef MINECRAFT_RENDER_TRACE` (:4, :9, :17); no-op (incl. `(void)phase;` :13) when the macro is off; `recordCount()`/`records()` return the (empty) state.
- `tests/frame_pipeline_order_test.cpp` (87 lines) — 6 GoogleTests:
  - `FramePipeline.EnumeratesPhasesInDrainToDiagnosticsOrder` — phaseAt(0..5) equals Drain→Input→Ticks→Render→Pace→Diagnostics (:11).
  - `FramePipeline.RunVisitsEveryPhaseWithoutSideEffects` — inert `run()` smoke (:20).
  - `TaskMailbox.PreservesFifoOrderWithinPriority` — 5 tick tasks drain in FIFO order (:24).
  - `TaskMailbox.PrioritiesDrainIndependently` — urgent/render/tick queues drain independently (:34).
  - `TaskMailbox.EmptyDrainReturnsZero` (:49).
  - `FrameProfiler.RecordsPhaseDurationsWhenTraceEnabled` + `FrameProfiler.BeginFrameResetsRecordsWhenTraceEnabled` — both assert recording only under `#ifdef MINECRAFT_RENDER_TRACE` (macro is defined project-wide by CMake, so they exercise the trace-on path; `SUCCEED()` fallback keeps them compiling when off).

## 2. Deviations from the plan (2, both necessary/flag-only)

1. **Nested type is `FrameBudget::Deadline`, not `FrameBudget::deadline`.**
   `FrameBudget.hpp:6` already has a member `std::chrono::steady_clock::time_point deadline{}`. A nested class and a non-static data member with the same name in one class scope are ill-formed in C++ ("declaration changes meaning of 'deadline'"). Since the plan mandates keeping the existing `{deadline, minItems, fromMs, hasRemaining}` aggregate intact and backward-compatible, I could not use the literal type name `deadline`. The functional API matches the spec exactly: `FrameBudget::beginFrame(int ms)` sets the shared per-frame deadline to `now + ms`, and `frameDeadline().remaining()` returns the remaining duration (clamped ≥ 0). Plan-master §1.5 / WI-12a shorthand "`FrameBudget::deadline`" is preserved as the *object* semantics; only the nested *type name* differs.
2. **`tests/frame_pipeline_order_test.cpp` is not registered in `CMakeLists.txt`.**
   `MINECRAFT_TEST_SOURCES` (CMakeLists.txt:339-362) does not list it, and L1's WI-T pre-registration list (plan-master.md:240-244) omits `frame_pipeline_order_test` as well. Only L1 (WI-T) may edit CMakeLists.txt per the ownership matrix, so this is a **required registration flag** for L1/compile-fixer. Without it the test file will not be compiled or run; it compiles standalone as written.

## 3. Line-drift corrections / verification

- `FrameBudget.hpp:5-14` (plan target) verified as a 15-line file; original lines 5-13 preserved verbatim, additive block at 14-42. No drift.
- `Minecraft.cpp:742-743` and `:827-828` (the two `stallMutex` blocks) re-grepped and verified present; **NOT touched** (absorbed by WI-12b PASS-2, keeping L1/L2 file-disjoint).
- `tests/render_profiler_test.cpp` and `tests/client_timer_test.cpp` verified present; neither references the new classes. `client_timer_test.cpp` pattern (no enclosing namespace, direct includes) and `render_profiler_test.cpp` indentation were used as the GoogleTest convention template.
- No existing symbol collisions: grep confirmed no prior `FramePipeline`/`TaskMailbox`/`FrameProfiler` in `src/`; `enum class Phase` in `gui/auth/LoginScreen.hpp:29` is a different namespace; existing `RenderStage` enums are unrelated names.
- Build plumbing confirmed: CMake globs `src/net/minecraft/*.cpp` with `CONFIGURE_DEPENDS` (CMakeLists.txt:53-55) and applies `MINECRAFT_RENDER_TRACE=1` to every target via `minecraft_configure_target` (:122-138, applied at :256/:261/:266/:269/:280/:296/:320/:390), so the new `.cpp` files compile into the client lib and the test and profiler macro state is uniform (no ODR hazard).

## 4. Constraints honored

- **Zero behavior change**: no consumer of `FrameBudget` modified; `Minecraft.cpp`, `CMakeLists.txt`, `WorkerPool.hpp`, `WorkerHandoff.hpp`, and every file outside the ownership list untouched.
- **Header-only-friendly**: all four headers are self-contained (no `Minecraft`/GL dependency); `.cpp` files are trivially re-bodyable by WI-12b without header churn.
- **No build/test/git run** performed.
- **2-space indent, `#pragma once`, existing include ordering (own header first in `.cpp`, std then project in headers), comments only where the surrounding style has them** — all followed.
