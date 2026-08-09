# Shader system — the actual refactor plan

Written 2026-08-08. Supersedes `shader-system-rebuild.md` entirely (that document's Stage D
and Stage G are cancelled; only its file:line inventory is still worth reading).

Ordered by **measured cost and by what deleting it removes**, not by how ugly it looks.

---

## Budgets — every per-frame operation, in nanoseconds

No target in this document is expressed in "single digit ms" or "about a second". Those are
vibes. Below is the enforceable table. `RenderProfiler` already keeps everything in ns
internally (`nanoTime()`, `cpuNs_`, `gpuAvgNs_`) and only loses it in `formatMs` at print
time — add a ns column, do not add a new timer.

Frame budget: **16,666,667 ns** at 60 fps, **6,944,444 ns** at 144 fps. Everything below must
fit inside that with the GPU as the limiter, not the CPU.

| Operation | Times per frame (RVox) | Budget | Now |
|---|---|---|---|
| `ShaderProgram::location()` — cached hit | ~10k | **≤ 60 ns** | ~40 ns (fine) |
| `ShaderProgram::location()` — driver query | **0** | **0 ns** | ~10k × ~2,000 ns = the 1 fps |
| `bindAndUploadUniforms` + draw, per section-layer | ~800 | **≤ 2,000 ns** | ~26,000 ns |
| Solid terrain stage, total | 1 | **≤ 1,600,000 ns** | 13,110,000 ns |
| Translucent terrain stage, total | 1 | **≤ 1,600,000 ns** | 18,600,000 ns |
| Composite pass setup (binds + uniforms, no alloc) | ~25 | **≤ 5,000 ns** | ~6,000 ns in string churn alone |
| All composite passes, total | 1 | **≤ 125,000 ns** | inside the 37,330,000 ns "Unmeasured" |
| Prewarm / compile work during a frame | **0** | **0 ns** | up to 60,000,000 ns |
| Heap allocations in the steady-state frame path | **0** | **0** | thousands |
| `Unmeasured` bucket | 1 | **≤ 2,000,000 ns** | 37,330,000 ns |

Two rules that make the table enforceable rather than decorative:

1. **Zero-count budgets are asserts, not aspirations.** In the debug build, count driver
   uniform queries, `glLinkProgram`/`glProgramBinary` calls, and heap allocations inside the
   frame path. Any of them non-zero in a steady-state frame is a failure, not a regression to
   discuss. A counter that can only ever read 0 is worth more than a timing average.
2. **Per-operation, not per-stage.** "Solid terrain got faster" hides a 26,000 ns draw behind
   a smaller section count. Measure ns/draw and ns/pass; the stage totals are derived.

### The one number that cannot be nanoseconds — stated plainly

A cold GLSL compile + link is **driver work**: 10,000,000-50,000,000 ns per program, inside
the NVIDIA/AMD compiler, and no refactor of this codebase changes it. Anyone promising ns
there is lying. There are exactly three honest levers, and all three are already in this plan:

- **Do it zero times per frame.** That is P1. This is the entire user-visible win.
- **Do it once per content hash, ever.** `ShaderBinaryCache` already does this;
  `glProgramBinary` on a disk hit is ~1,000,000-3,000,000 ns instead of ~30,000,000 ns.
  Target: **≥ 95 % disk-cache hit rate** on the second and every later load of an unchanged
  pack. Measure it — log hits/misses per activation. If it is not ~100 %, the content hash is
  unstable and that is a bug worth chasing.
- **Do it behind a progress bar, not during gameplay.** ~160 programs × ~30 ms cold is the
  first-ever pack load and is unavoidable; × ~2 ms warm is every load after. Both are honest.

So the compile budget is: **cold ≤ 50 ms/program (driver-bound, not ours), warm ≤ 3 ms/program,
and 0 ns of either inside any rendered frame.**

---

## The one insight

**Almost every complaint traces to a single subsystem: incremental pack activation.**

Somebody decided shader compilation must be spread across frames so the game never freezes.
To do that they built: a prewarm queue with a cursor and a hardcoded time budget, a four-state
machine per pack, a *pending* pack, a *staged* pack, a pack cloner, and a per-frame readiness
poll. Everything the user is angry about is downstream of that decision:

| Complaint | Caused by |
|---|---|
| "freezes, stutters really hard, then smooths out" | 5 prewarm entries per frame × 12 ms budget each = up to **60 ms/frame of deliberate stall**, and the budget is checked *after* a 50-300 ms link, so it overruns anyway |
| "changing a shader property takes 2000 ms" | a settings change calls `clonePack`, which re-enumerates the directory / reopens the zip and re-runs the **entire** `PackLoader::load` from disk, just to change `#define` values |
| "pointer hell" | up to **four** live `PackInstance`s at once (base, active, pending, staged), so every function takes `PackInstance*` and null-checks it, at every layer |
| "the entire cache is fucked" | each `PackInstance` carries its own six caches, and `clonePack` duplicates all of them — so "which cache is authoritative" has no answer |

None of it buys anything. The total time to switch packs is *longer* than compiling
synchronously, because the work is the same and the scheduling adds overhead, re-parsing and
duplicate state on top.

**Iris does not do this.** It compiles the pack synchronously behind a loading screen. This
repo already has the loading screen: `render/ProgressRenderer.hpp` implements
`gui::screen::LoadingDisplay` with `progressStage(...)` and `progressStagePercentage(int)`.

So: **delete incremental activation.** Pack switch = load, compile everything, swap, behind a
progress bar. ~160 RVox programs at ~30 ms cold ≈ 5 s once, then the disk binary cache makes
every later load ~1 s. That is what every shader mod in existence does and what users expect.

---

## P0 — DONE (verify in-game before anything else)

Two root causes, both fixed, both build clean, neither confirmed in-game. See
`HANDOFF-shader-fixes.md`.

1. **Black smudge / dead voxel lighting.** `scanTargetFormats` skipped every
   `const int colortexNFormat` because packs wrap them in `/* */` — the normal idiom, which
   `scanPackConstants` two functions below already reads through. Zero formats were read from
   rethinking-voxels; all 15 targets fell back to `rgba8`. `colortex9` is declared `R32UI`
   for `imageAtomicMax`; as `rgba8` it is not an integer texture, so the reprojection atomics
   wrote nothing.
2. **1 fps.** `ShaderProgram::location()` cached only *successful* lookups, so every uniform a
   pack doesn't declare cost a `glGetUniformLocation` on every use. `uploadFogUniforms` issues
   six unconditionally and `bindAndUploadUniforms` runs per chunk section per terrain layer
   per frame — order 10,000 driver round-trips per frame.

The profiler said this all along and I misread it: `Total measured 57.74 CPU / **1.25 GPU**`
against a ~95 ms frame. The GPU was asleep. Read that split first, always.

---

## P1 — Delete incremental activation

### Delete outright

From `render/pipeline/Manager.{hpp,cpp}`:

```
advancePackActivation()      preparePendingPack()     prepareStagedPack()
commitStagedPack()           discardStagedPack()      cancelPendingPack()
packReady()                  clonePack()              prewarmPacks()
warmBasePrograms()           activatePack()
pendingIndex_    stagedPack_    stagedIndex_
```

From `shaders/Compiler.{hpp,cpp}`:

```
buildPrewarmQueue()   prewarmStep()   kTimeBudgetMs   validate()
```

From `pipeline/Instance.hpp`:

```
prewarmQueue    prewarmCursor    programState    PackProgramState (the whole enum)
```

And the six `prewarmStep` call sites (`Manager.cpp` 377, 385, 487, 539, 620;
`Pipeline.cpp` 321).

### Replace with

One function, called only when the pack actually changes — never per frame:

```
bool activatePack(index):
    definition = resolve(scan, settings, dimension)     // no disk IO, see P2
    progress.progressStart("Compiling shaders")
    for each enabled program (i of n):
        progress.progressStagePercentage(i * 100 / n)
        compile(program)                                 // ProgramCache: disk hit or one link
    if all ok: swap into active_, else: keep previous and report
    progress.progressStop()
```

`GameRenderer` stops calling `advancePackActivation()` per frame. `PackManager::prepareFrame`
stops calling `preparePendingPack` / `prepareStagedPack`. `Pipeline::prepareFrame` stops
calling `prewarmStep`. A frame does **zero** compilation work.

Dimension change (overworld → nether) is the same call — it is a pack re-resolve, and it
already stops the world, so a progress bar there is correct and honest.

**Acceptance (mechanical):** `grep -rn "prewarm" src/` returns nothing. A frame in a steady
world performs zero `glLinkProgram`/`glProgramBinary` calls — assert with a counter in the
debug build.

### What this fixes for free

- The stutter (no per-frame budget exists to overrun).
- The pointer hell: with `pending` and `staged` gone there are exactly **two** packs alive,
  `base_` and `active_`. Change every `PackInstance*` parameter to `PackInstance&` and delete
  the null checks that open nearly every function in `Pipeline.cpp`.
- Half the cache-ownership problem: nothing clones a pack, so no cache is ever duplicated.

---

## P2 — A settings change must not touch the disk

`clonePack` re-runs `PackLoader::load` — full directory walk / zip reopen, full re-scan — to
change `#define` values. That is the 2000 ms.

Split the load in two, along the line that already exists in the data:

- **`PackScan` — immutable.** Resource list, raw file text, include graph, and the per-file
  directive scan (formats, constants, `RENDERTARGETS`, option declarations). Depends only on
  the bytes on disk. Rebuilt **only** when the pack directory stamp changes (the watcher
  already computes one).
- **`resolve(scan, settings, dimension) -> PackDefinition` — pure, no IO.** Applies option
  values, evaluates `#if` for the directive scan, picks the dimension overlay, computes the
  enabled-program set. Milliseconds.

A settings change re-runs `resolve` only. A dimension change re-runs `resolve` only. Neither
opens a file.

This also kills the redundant read caches: `PackInstance::sourceCache` and the
`loadReadCache` lambda inside `PackLoader::load` are the same map built twice; `PackScan`
owns file text and nothing else may.

---

## P3 — Canonicalization: keep the transform, delete the preprocessor

`prepareSource` = `normalizePackSource` → `canonicalizeCoreSource` → `mergeColorWheelMaterial`,
1,344 lines across four files, run per stage per program.

**The token transform is genuinely required and must stay.** RVox is `#version 130`
compatibility GLSL using `gl_Vertex`, `ftransform()`, `gl_ModelViewMatrix`; the engine
compiles core 330. Iris does the same rewriting. Do not "simplify" `CoreGlslTransformer`'s
substitution list — it is long because the spec is long.

**The preprocessor in `normalizePackSource` is ceremony and should go.** It walks every line
maintaining a `ConditionalState` stack and a `PPMacroTable`, evaluating `#if` to decide which
lines to emit — i.e. it does dead-branch elimination that **the driver's own preprocessor does
anyway, immediately afterwards**. And the engine macros it evaluates against are already
emitted as real `#define` lines in the version preamble, so the driver has them. We are
preprocessing the source so we can hand the driver source it will preprocess again.

What `normalizePackSource` must actually still do, and it is about thirty lines:

1. strip the pack's `#version` (the preamble supplies one),
2. hoist `#extension` directives to the top (GLSL requires them before declarations),
3. blank out `#include` lines already consumed by `IncludeResolver`,
4. preserve line count so driver error messages still point at real lines.

Token replacement and missing-declaration injection are safe to run over inactive `#if`
branches — an unused `in vec3 vaPosition;` or a rewritten `gl_Vertex` inside a dead branch
compiles fine and is discarded by the driver.

Deleting that loop removes, in one go: the per-call `PPMacroTable macros = *cachedTable;`
**deep copy of several hundred string pairs**, the process-lifetime `static tableCache`, the
`CacheKey` struct that rebuilds two `vector<string>` from two `set<string>` on **every call**
just to linear-scan the cache it fronts, and `seedEngineMacros`' second role.

**Keep the preprocessor for the loader.** `scanPackConstants` genuinely must resolve
`#if SM_DIST` to read the right `const float shadowDistance` — that is directive scanning, a
different job, and it stays. The mistake was sharing one preprocessor between "decide what
this pack declares" (needs `#if`) and "hand text to the driver" (does not).

**Then add the cache that should have existed:** the transform *output*, keyed by
`(program name, stage)`. Today the input is cached (`resolvedSourceCache`) and the output
binary is cached (`ProgramCache` + disk), and the 1,344-line transform between them re-runs
on every cold path.

---

## P4 — Cache collapse (mostly falls out of P1-P3)

Ten caches, four key spaces, no single invalidation authority. After P1-P3, what should
remain is three, one key space and one owner each:

| Cache | Key | Invalidated by |
|---|---|---|
| `PackScan` | file path | directory stamp change |
| `ProgramSet` | **program name** | settings / dimension / labPBR change |
| `ShaderBinaryCache` | content hash | never (correct — it is content-addressed) |

`ProgramSet` absorbs `compiledPrograms`, `programCacheKeys`, `programDrawBuffers`,
`programEnabledCache` and `ProgramCache::cache_`. The intermediate "cache key"
(`name|vsh|gsh|tcs|tes|fsh`) disappears: it exists only so two program names can share one
compiled binary, which the content hash already does correctly and for free.

`ShaderProgram::uniformCache_` stays — it belongs to the object it describes and has exactly
one invalidation trigger (relink). That is the shape all of them should have had.

**Acceptance:** `grep -c "programCacheKeys"` returns 0; every remaining cache has exactly one
`clear()` call site. Two call sites means the collapse is fake.

---

## P5 — Per-frame churn in the composite walk (only if still visible)

The `Unmeasured 37.33 ms`. `renderCompositePasses` rebuilds two `unordered_map<string,int>`
and calls `refreshColorMaps` (erase-scan + ~30 `std::string` constructions) four-plus times
**per pass**, ~25 passes. `logOnce` builds an `ostringstream` for every compute parent **every
frame** before checking whether it already logged — that is the flood in `client.log`.
`ensurePackResources` calls `glGetString(GL_EXTENSIONS)` + `strstr` per stage per frame.

Do the two cheap ones first (check the dedupe set *before* formatting; make the VRAM query a
function-local `static`), then re-measure before touching the map rebuilding.

---

## Order

1. **Verify P0 in-game.** `scene targets` must read `colortex9=r32ui`; terrain CPU stages must
   fall to single digits. Nothing below matters until this holds.
2. **P1** — delete incremental activation. Biggest deletion, biggest user-visible win, and it
   makes P2/P4 tractable.
3. **P2** — scan/resolve split. Kills the 2000 ms settings change.
4. **P3** — delete the compile-path preprocessor, add the transform-output cache.
5. **P4** — collapse what's left. One pass, build once.
6. **P5** — only if the profiler still shows it.

Each of 2-5 is one coherent change: write every edit, then build **once** with
`.\build-omega.ps1 -BuildType Debug -Target Client`, and confirm **both** `exit 0` and a fresh
`build-omega/minecraft_native.exe` timestamp. Never `ctest`, never `-RunTests`.

## What is NOT being done

`shader-system-rebuild.md`'s `FrameGraph`/`PassStep`/`FrameExecutor`/`UniformPlan` rewrite.
It was justified by per-frame re-resolution cost, but the measurement says the GPU is idle at
1.25 ms and the CPU cost was uniform-name lookups — three lines in P0. That rewrite would have
been weeks of churn against a three-line bug, and it would have added a new invalidation
surface to a codebase whose actual disease is too many things with unclear invalidation.

`PackManager`'s dozen pure forwarding methods are still slop worth deleting — but as fallout
of P1/P4, once the state they forward to has one owner. Not as a project.
