# Shader system rebuild — full plan

> **SUPERSEDED 2026-08-08. Read `HANDOFF-shader-fixes.md` instead.**
>
> This document got the *inventory* right and the *diagnosis* wrong. Two root causes were
> found afterwards by tracing the actual profiler numbers, and neither needs any of the
> architecture work below:
>
> 1. **The black smudge / broken lighting** is not the dimension-merge bug in §1. It is
>    `scanTargetFormats` refusing to read `const int colortexNFormat` declarations that sit
>    inside a `/* */` block comment — which is exactly where every real pack puts them
>    (`rethinking-voxels`' `lib/pipelineSettings.glsl` wraps all 15 in one). The loader read
>    **zero** formats from that pack; every target fell back to `rgba8`. The neighbouring
>    `scanPackConstants` in the same file already reads through block comments on purpose,
>    so the two scanners disagreed. One condition.
> 2. **The 1 fps** is not the composite-pass architecture in §5. `ShaderProgram::location`
>    caches only *successful* lookups, so every uniform a pack does not declare costs a
>    `glGetUniformLocation` on every use, forever. `uploadFogUniforms` issues six of them
>    unconditionally per chunk section per terrain layer per frame, five of which are
>    guaranteed misses in that pack — order 10,000 driver round-trips per frame. The
>    profiler confirms it: terrain stages burn 13–18 ms CPU each while total **GPU time is
>    1.25 ms**. Removing one `if` fixes it.
>
> §2.2 and §2.3 gestured at the second one but buried it inside a `UniformPlan` rewrite
> instead of naming the three-line fix. **Stage D and Stage G are cancelled.** Stages A, B,
> C and F were applied and are fine, but they were not the bug. The file:line inventory in
> §2 and §5 is still accurate and still useful; the priorities and the work order are not.

Written 2026-08-08 against `main` @ cf0cd0e8. Every claim below has a `file:line`. Nothing
here is inferred from a screenshot.

---

## 0. What is actually wrong (measured, not adjectival)

Four distinct failures, four distinct mechanisms. They are not the same bug and they do not
share a fix.

| Symptom | Mechanism | Where |
|---|---|---|
| Black smudge, only clouds draw | Every `colortexNFormat` the pack declares is discarded; all 15 targets allocate `rgba8` | `Loader.cpp:1859`, `Loader.cpp:1802-1839` |
| 1 fps in RVox | Per-pass FBO destroy+recreate+`glCheckFramebufferStatus`; ~150 uncached `glGetUniformLocation` per program bind; full GLSL re-transform per failed-program lookup per draw | `ColorTargets.cpp:246`, `ShaderProgram.cpp:330`, `Compiler.cpp:196-217` |
| ~2000 ms to load / switch pack / change an option | Every cold program is compiled and linked **twice**; a settings change re-parses the entire pack from disk | `ShaderCompileService.cpp:38-57` + `ProgramCache.cpp:101`, `Manager.cpp:557-591` |
| root fsh scan 364 ms + dims ~450 ms | Directive scanning is O(fsh × total-included-bytes) instead of O(unique files); `buildCommentMask` heap-allocates one byte per source byte, per expansion | `Loader.cpp:1594-1620`, `Loader.cpp:634` |

---

## 1. The correctness bug — why the screen is black

`shaders/rethinking-voxels_r0.1-beta9` declares its formats in one file:

```
shaders/lib/pipelineSettings.glsl:2   const int colortex0Format = R11F_G11F_B10F;
                                :3    const int colortex1Format = R32F;
                                :9    const int colortex9Format = R32UI;
                                :11   const int colortex11Format= RGBA16I;
```

`build-omega/client.log` shows what the engine actually allocated:

```
scene targets 854x480 {colortex0=rgba8, ... colortex8=rgba16f, ... colortex14=rgba8}
```

**14 of 15 formats are wrong.** HDR lighting written to an 8-bit UNORM clamps at 1.0; the
`R32UI` atomics target for reprojection validation is not an integer texture at all, so
`imageAtomicMax` in `prepare1` writes nothing; `RGBA16I` reads back garbage. That is the
black smudge and that is why the voxel light propagation produces nothing. It is not
"volumetrics".

### Mechanism

RVox has **no programs at `shaders/*.fsh`** — everything lives under `shaders/world0/`,
`world-1/`, `world1/`, with `dimension.world0=*`.

1. The root pass (`PackLoader::load` → `loadProgramSet`, `Loader.cpp:1730`) scans every
   `.glsl` in the pack, including `shaders/lib/pipelineSettings.glsl`, and fills
   `out.targets` with all 15 correct formats.
2. The dimension pass (`Loader.cpp:1802-1839`) rebuilds a *separate* `PackDefinition` from
   `mapped` — only the files under `shaders/world0/`, rewritten to look like `shaders/…`.
   `shaders/lib/` and `shaders/program/` are **not in `mapped`**, and `world0/` contains no
   `.glsl` at all. So the dimension definition's format table is populated only by whatever
   `layout(...) uniform image2D colorimgN` declarations happen to be dragged in through a
   `.fsh` include chain (`inferColortexFormatsFromLayouts`, `Loader.cpp:784`). That is where
   the lone surviving `colortex8=rgba16f` comes from —
   `program/deferred1_csh.glsl:138  layout(rgba16f) uniform image2D colorimg8;`.
3. `Loader.cpp:1848` — because the root has no programs, the seed block runs:

```cpp
out.programs = seed->programs;
out.passes   = seed->passes;
out.targets  = seed->targets;      // Loader.cpp:1859 — assignment, not merge
```

   The correct root table is **overwritten wholesale** by the impoverished dimension table.
   `loadPack` then snapshots `pack->rootDefinition = pack->definition`, so the loss is
   permanent; `Pipeline::selectDimension` (`Pipeline.cpp:241-244`) merges dimension over
   root, but root is already destroyed.

### Fix

A dimension folder is an **overlay on the pack**, not a pack of its own. That is Iris'
model and it is the only one that can be right: a `world0/composite.fsh` that `#include`s
`/lib/pipelineSettings.glsl` is by definition subject to those constants.

- The dimension pass must start from the root `PackDefinition` and *override*, not
  construct from zero. `loadProgramSet` gains a "seed from" argument, or — better — the
  scan phase is separated from the program-assembly phase entirely (§5.2) so there is only
  one format table for the whole pack, built once, and dimensions only override
  `programs` / `passes` / `flips` / `programScales`.
- Delete `out.targets = seed->targets` and the two `std::max` lines beside it. Nothing
  should ever replace a scan result with a subset of itself.
- Acceptance test (mechanical): `ShaderPackLoader.DimensionFolderPackKeepsRootFormats` —
  load a fixture shaped like RVox (formats in `lib/`, programs only in `world0/`), assert
  `definition.targets.at("colortex9").format == "R32UI"` **and** the same after
  `selectDimension("*")`. Today that test fails; if it passes without the loader changing
  shape, the fix is fake.

### Second-order: integer targets are never validated

Once formats are right, `bindColorImages` (`GlState.cpp:375`) and `bindSamplers` must
match `SamplerKind::Integer`/`Unsigned` (already reflected in `ShaderProgram::reflectSamplers`,
`ShaderProgram.cpp:336`) against the target's actual format, and refuse — loudly, once —
when a `uimage2D` is bound to an `rgba8`. Every hour lost on this bug would have been
saved by that one check. It is not a "dummy fallback"; it is the error the rules demand
you surface instead of swallow.

---

## 2. The 1 fps — ranked by cost, each with its mechanism

### 2.1 Write-FBO churn (worst)

`ColorTargets::bindWrite` (`ColorTargets.cpp:242`):

```cpp
writeFbo_.destroy();               // every pass, every frame
... addColorAttachment × N ...
writeFbo_.drawBuffers(drawBuffers);
writeFbo_.removeDepthAttachment();
const unsigned status = writeFbo_.checkStatus();   // driver sync point
```

RVox runs ~25 raster composite/deferred/prepare passes per frame. That is 25 FBO
create/destroy cycles and 25 `glCheckFramebufferStatus` calls per frame, each of which can
stall the driver. FBOs are cheap to *keep* and expensive to *validate*.

**Fix:** one FBO per distinct output-set, created when the frame graph is built (§5.3),
keyed by the output names + flip parity. `checkStatus` runs at build time only. Per frame
the pass does `bindFramebuffer(cachedHandle)` and nothing else. Ping-pong flips select
between two prebuilt FBOs per set — the flip parity is already computed deterministically
in `Pipeline::initShadowColorFlips` (`Pipeline.cpp:777`), so it is knowable at build time.

### 2.2 Uncached negative uniform lookups

`ShaderProgram::location` (`ShaderProgram.cpp:322`):

```cpp
const int location = GLCore::getUniformLocation(program_, std::string(name).c_str());
if(location >= 0) {
 uniformCache_.emplace(std::string(name), location);     // only hits are cached
}
return location;
```

`uploadShaderUniforms` (`Uniforms.cpp:47-229`) issues ~180 uniform writes, of which ~120 go
through the by-name path. A pack declares maybe 30 of them. **~90 `glGetUniformLocation`
calls plus 90 `std::string` heap allocations, per program bind, forever.** Add
`uploadDrawMatrixAliases` (`Uniforms.cpp:14`), which does a full 4×4 multiply and a 4×4
inverse on the CPU per call, and re-uploads a constant identity `textureMatrix` every time.

**Fix — the one that deletes the most code:** build a `UniformPlan` at link time.
`reflectSamplers` already walks `glGetActiveUniform` over the whole program; extend that
walk to emit, for each *declared* uniform the engine knows about, a
`{GLint location, uint16 offsetInPackUniformValues, UniformType}` record into a flat
vector. Upload = walk the vector. Result: zero string lookups, zero misses, zero
allocations, and `Uniforms.cpp`'s 230-line call list collapses to a table. The MVP/normal
matrices become entries in `PackUniformValues` computed once per frame in `FrameData`,
not per program bind.

### 2.3 Failed programs re-run the full source transform, per draw, per frame

`PackCompiler::compile` (`Compiler.cpp:196-217`): the fast path requires
`compiledPrograms` to hold the name, or `programs->find(cacheKey)` to return non-null.
`ProgramCache::find` returns `nullptr` for a *failed* entry (`ProgramCache.cpp:79-83`).
So for any program that failed to compile, every call falls through to `prepareProgram`,
which runs `prepareSource` on every stage:

- `normalizePackSource` (`SourceProcessor.cpp:248`) copies the entire `PPMacroTable` —
  hundreds of `std::string` pairs (biomes, render stages, every GL extension) — per call
  (`SourceProcessor.cpp:276`), and rebuilds the `CacheKey`'s two `vector<string>` from
  `set<string>` on every call just to do a linear scan;
- then a full line-by-line preprocessor pass over the whole expanded source;
- then `canonicalizeCoreSource`, then `mergeColorWheelMaterial`;
- then `rememberDrawBuffers` re-parses the entire fragment source (`Compiler.cpp:52`).

`RenderType::setupRenderState` calls the resolver **fresh on every `RenderPassScope`**
(`RenderType.cpp:60-62`) — many per frame. One unusable `gbuffers_terrain` turns each frame
into tens of megabytes of string processing. This is very plausibly the difference between
1 fps and 60.

**Fix:** memoize failure. `compiledPrograms` maps to `ShaderProgram*`; make it map to a
small `ProgramSlot { ShaderProgram* program; bool failed; }` and insert on failure too. A
pack reload/dimension change clears it, which is already the only time it may change.
Also cache `prepareSource` output keyed by `(cacheKey, stage)` — the resolved-source cache
(`pack.resolvedSourceCache`) exists for the *include* step but stops short of the transform
step, which is the expensive half.

### 2.4 Per-frame allocation churn in the composite walk

`renderCompositePasses` (`CompositeRenderer.cpp:82`) per stage per frame:

- builds `unordered_map<string,int> textures` + `colorImages` from scratch, then calls
  `refreshColorMaps` (erase-scan + refill, `CompositeRenderer.cpp:44`) **four or more times
  per pass** — each refill allocates ~30 `std::string`s via `"colortex" + to_string(i)`
  (`ColorTargets.cpp:444`, `:452`);
- `logOnce` builds an `ostringstream` and a formatted `std::string` for every compute
  parent, **every frame**, then throws it away on a `std::set` dedupe
  (`CompositeRenderer.cpp:236-244`, `:267`). The log excerpt in `build-omega/client.log` is
  literally this firing. A `logOnce` that formats before checking is not once;
- `std::vector<bool> computeDispatched(passes.size())` per stage per frame;
- `program->location("colorimg" + to_string(i))` for every color target × every compute
  pass × every frame (`CompositeRenderer.cpp:229`);
- `dispatchOrphansBefore` is O(stageComputes × programChain) and is called once per pass —
  O(n³) overall;
- `ensurePackResources` runs per stage and calls `vramBytes()`, which does
  `glGetString(GL_EXTENSIONS)` + `strstr` (`Resources.cpp:26-35`), plus re-walks every
  custom texture, image and target declaration (`Resources.cpp:37-279`).

All of this is per-frame recomputation of things that only change when the pack, the
dimension or the resolution changes. §5.3 is the fix: compute it once into a flat plan.

### 2.5 Smaller, still real

- `binaryProcs()` calls `wglGetProcAddress` three times on **every** `loadFromBinary` /
  `extractProgramBinary` (`ShaderProgram.cpp:516`).
- `ShaderProgram::applyDrawBuffers` does `glGetIntegerv(FRAMEBUFFER_BINDING)` — a readback —
  and allocates a `vector<unsigned>` per call (`ShaderProgram.cpp:298`), and it is invoked
  from the world-pass directive applier on every `RenderPassScope`
  (`Manager.cpp:47-65`).
- `bindWorldProgram` constructs two `unordered_map<string,int>` per bind
  (`WorldProgramBinder.cpp:75-77`) and builds `"shadowcolor" + to_string(i)` strings per
  shadow bind (`:125`).
- `Pipeline::worldProgram` copies the key into `lastWorldProgramKey_` and
  `resolvedWorldProgramKey_` (`std::string` assignment) on every call, then does a chain of
  string comparisons to derive a render stage that is a property of the `RenderType`, known
  at construction (`Pipeline.cpp:611-656`). Put `core::RenderStage` in `RenderType` as a
  field and the whole cascade disappears.
- `printf("[diag] …")` debug noise still in `ColorTargets.cpp:63, :106, :204` — violates
  rule 20 and prints inside allocation paths.

---

## 3. The 2000 ms load / pack switch / option change

### 3.1 Every cold program compiles and links twice

This is the big one and it is a two-file read to confirm.

`ShaderCompileService::runJobOnCurrentContext` (`ShaderCompileService.cpp:27-59`):
on a disk-cache miss it builds a **stack-local** `ShaderProgram`, compiles it, links it,
runs `reflectSamplers` + `refreshUniformLocations` on it, extracts the binary, and destroys
it. It returns only the binary.

`ProgramCache::compileSync` (`ProgramCache.cpp:95-128`) then takes that binary and calls
`program.loadFromBinary(...)` on a **second** `ShaderProgram` — `glProgramBinary`, another
link-equivalent, another `reflectSamplers`, another `refreshUniformLocations`.

So every cold program pays: full GLSL compile + link, binary extract, `glProgramBinary`
re-link, and two full uniform reflections. RVox has ~160 enabled programs. If the driver
takes 6 ms for the compile and 4 ms for the binary reload, that is the 2000 ms exactly.

`ShaderCompileService` is a 60-line, fully synchronous type whose entire job is "hash the
request, look in a directory, call ShaderProgram". It is the singleton-plus-wrapper shape
the rules name. **Delete it.** `ProgramCache` owns the `ShaderBinaryCache` directly and:

```
hash → disk hit?  → loadFromBinary into the real program.      (one link)
       disk miss? → compile into the real program, then
                    extractProgramBinary → storeAsync.          (one link)
```

Never round-trip a binary you produced in this process. That deletes two files
(`ShaderCompileService.{hpp,cpp}`), the `ShaderCompileRequest`/`ShaderCompileResult` pair,
the `CacheBinaryOutcome` enum and `loadBinary`/`buildRequest` in `ProgramCache.cpp`, and
halves cold-load time.

### 3.2 The prewarm budget is not a budget

`PackCompiler::prewarmStep` (`Compiler.cpp:262-306`) takes a 12 ms budget but checks the
deadline only *after* compiling a program, so one slow program overruns arbitrarily. Worse,
four independent call sites drive it in the same frame:
`Pipeline::prepareFrame` (`Pipeline.cpp:301-311`), `PackManager::preparePendingPack`
(`Manager.cpp:536`), `prepareStagedPack` (`Manager.cpp:617`) and `advancePackActivation`
(`Manager.cpp:370-389`). Up to 4 × 12 ms of *intended* stall per frame, plus overrun.

**Fix:** one owner, one call per frame, and the budget checked before starting a program
using a measured moving average of recent compile times. When the next program's predicted
cost exceeds the remaining budget, stop. A frame that renders at 40 fps while warming is
worth more than one that finishes warming two frames sooner.

### 3.3 An option change re-reads and re-parses the whole pack from disk

`PackManager::setSettings` → `clonePack` (`Manager.cpp:557-591`) re-enumerates the
directory (or reopens the zip), then calls the full `PackLoader::load` again — including
the 364 ms + 450 ms scan of §4 — to produce a definition that differs only in
`#define` values. `prepareStagedPack` does the same on every dimension change.

Options affect three things and only three: `rewriteOptions` output, `#if` evaluation in
`scanPackConstants`/`preprocessProperties`, and `isProgramEnabled`. None of them need a
filesystem round trip.

**Fix:** the raw file contents are already cached (`pack.sourceCache`). Separate the
immutable part of a load (resource list, raw text, per-file directive scan — §5.2's
`PackScan`) from the option-dependent part (macro seeding, `#if` resolution, program
enable set, format overrides). Re-running only the second part on a settings change turns
~2000 ms into single-digit milliseconds. Nothing about "keeping the ability to load
shaders" is lost — the pack is loaded, it is just not loaded three times.

---

## 4. The 364 ms root scan and the ~450 ms dimension scan

`loadProgramSet` (`Loader.cpp:1594-1620`):

```cpp
for(const std::string& path : resources) {
 if(!path.ends_with(".fsh")) continue;
 std::string source = resolveShaderIncludes(..., path, false, expanded);  // FULL expansion
 scanOnce(source);                                                        // scans the whole thing
 ...
}
```

`scanOnce` hashes the entire expanded string and, on a new hash, runs `scanTargetFormats`
and `scanPackConstants` over all of it. Every expansion is unique, so the dedupe never
fires. `lib/common.glsl` and everything it pulls in is therefore re-scanned once **per
`.fsh` in the pack**. RVox `world0` has ~60 `.fsh` files; each expands to hundreds of KB.
That is tens of megabytes of scanning per pack.

On top of that, `scanTargetFormats` calls `buildCommentMask` (`Loader.cpp:634`), which
`assign`s a `vector<unsigned char>` **the size of the whole expanded source** — so it is
also tens of megabytes of allocate/zero/free. And `scanPackConstants` copies the seeded
`PPMacroTable` per call (`Loader.cpp:298`).

The dimension loop then repeats the entire thing per dimension folder, with a fresh
`expanded` memo and a fresh macro seed (`Loader.cpp:1830`) — hence "dims ~450 ms" for three
folders.

**Fix — the shape change, not a micro-optimisation:** directives are *file-scoped*. Scan
each **unique raw file once**, never an expansion. Build a `PackScan`:

```
struct PackScan {                       // one per pack, built once
  ResourceIndex files;                  // path -> raw text (already: pack.sourceCache)
  IncludeGraph  graph;                  // path -> direct includes, cycle-checked
  FileDirectives perFile[…];            // formats, constants, RENDERTARGETS, options, version
};
```

`FileDirectives` for `lib/pipelineSettings.glsl` is computed once and applies to every
program whose include closure contains it. A program's effective directive set is the
union over its closure, computed by walking the (tiny) graph — not by re-scanning text.
`IncludeGraph` already exists (`IncludeResolver.cpp:25`) and already memoizes; it just is
not being used as the scan unit.

Expected: 364 ms + 450 ms → the cost of reading each file once and scanning each file once.
For RVox that is a few hundred KB total, so low tens of milliseconds. This also *fixes*
§1 for free: the format table becomes a property of the pack's file set, so no subset pass
can lose it.

The "unmeasured ~100 ms": once the above lands, re-run `MINECRAFT_STARTUP_PROFILE=1` and
re-attribute. Do not chase it before the 800 ms is gone — it is currently below the noise
floor of what these two passes do.

---

## 5. The architecture — the orchestrator

### 5.1 What is wrong with the current shape

Not "too many classes". The problem is that **every layer recomputes the same thing at the
wrong lifetime**, and the seams between layers are parameter lists instead of state.

Count the coupling:

- `PackManager` has **12 methods whose entire body is `return pipeline_.same_thing(activePack(), …);`**
  (`Manager.cpp:690-781`). That is the textbook forwarding wrapper the rules say to delete.
- Seven pipeline entry points thread the *same six* shadow parameters:
  `(shadowDepthTextureId, shadowOpaqueDepthTextureId, shadowColorTextureIds,
  shadowColorTextureCount, shadowTargets, shadowColorAltTextureIds)` —
  `Pipeline.hpp:61-76`, mirrored verbatim in `Manager.hpp:90-117`, and then copied into
  six matching *members* on `Pipeline` (`Pipeline.hpp:156-161`) by four of those seven
  functions, with the same 8-line copy loop each time (`Pipeline.cpp:664-678`, `:686-696`,
  `:740-748`). The parameters and the members are the same state, passed twice.
- `Pipeline` is 952 lines spanning four lifetimes at once: pack loading (`selectDimension`),
  GPU resource allocation (`ensureSceneTargets`), per-frame uniform state
  (`setFrameUniforms`), and per-draw program resolution (`worldProgram`). Plus a lightmap
  generator and a PBR-format sniffer.
- `PackInstance` is a 50-field public blob (`Instance.hpp:31-84`) holding parse output,
  compile caches, GPU handles, per-frame published textures and a log dedupe set —
  four lifetimes in one struct, which is *why* everything needs a pointer to it.

### 5.2 The split — by lifetime, not by layer

This is the part that must not become another facade. The rule to hold yourself to: **each
new type owns state with exactly one invalidation trigger, and no type forwards to
another.**

| Type | Owns | Invalidated by | Replaces |
|---|---|---|---|
| `PackScan` | resource list, raw text, include graph, per-file directives | file on disk changes | the scan half of `Loader.cpp` |
| `PackDefinition` | resolved programs/passes/targets/settings for one (pack, dimension, settings) | settings or dimension change | today's `PackDefinition` (unchanged shape, correct content) |
| `ProgramSet` | linked `ShaderProgram`s + disk binary cache + failure memo | definition change | `ProgramCache` + `ShaderCompileService` + `PackInstance::compiledPrograms/programCacheKeys/programDrawBuffers` |
| `TargetSet` | colortex/shadowcolor/depth textures, prebuilt FBOs per output-set, flip parity | resolution or format change | `ColorTargets` + the FBO churn |
| `FrameGraph` | flat `vector<PassStep>`, fully resolved | any of the four above | `PackInstance::{post,deferred,compute,begin,shadowComposite,prepare,setup}Passes` + the entire per-frame half of `CompositeRenderer.cpp` |
| `FrameExecutor` | nothing — walks the graph | — | `Pipeline::render*` × 5 and `PackManager::render*` × 5 |

`PackManager` and `Pipeline` both cease to exist as types. `PackLibrary` (load, enumerate,
select, settings) is what is left of `PackManager` after the 12 forwarders are deleted, and
it holds `PackScan` + `PackDefinition` + `ProgramSet`. Nothing forwards: `GameRenderer`
talks to `FrameExecutor` for frames and `PackLibrary` for pack management, and those two do
not talk through each other.

### 5.3 `PassStep` — the thing that makes the frame cheap

```
struct PassStep {                 // built once per (definition, resolution)
  Kind            kind;           // Raster | Compute | Blit
  ShaderProgram*  program;        // resolved, non-null by construction
  unsigned int    framebuffer;    // prebuilt, status-checked at build time; 0 = screen
  Viewport        viewport;
  SamplerBinding  samplers[N];    // {unit, textureId slot} — names resolved at build
  ImageBinding    images[M];
  DrawBufferList  drawBuffers;    // GLenum array, prebuilt
  BlendState      blend;          // from bufferBlends, resolved
  float           alphaRef;
  MipmapList      mipmaps;
  FlipList        flips;          // which slots swap after this step
  ComputeGroups   groups;         // Compute only
};
```

Executing a frame becomes: for each step, bind FBO, set viewport, bind program, bind the
prebuilt sampler/image units, set blend, upload the `UniformPlan`, draw, apply flips. No
maps, no `std::string`, no `find`, no allocation, no `glCheckFramebufferStatus`, no
`glGetIntegerv`.

The texture *ids* referenced by `SamplerBinding` are indices into a small
`FrameResources` array (colortex read side, shadow read side, depthtex0/1/2, custom
textures, noise). Flips mutate that array, which is why the bindings can be resolved at
build time even though the ping-pong parity changes: the parity is a property of the pass
*order*, and the order is fixed. `Pipeline::initShadowColorFlips` (`Pipeline.cpp:777`)
already proves this is precomputable — it just does it for shadowcolor only, per frame,
into a member.

`shadowTargets`, the six threaded parameters, and `PackInstance::publishedTextures` all
collapse into `FrameResources`.

### 5.4 What GameRenderer stops doing

`GameRenderer.cpp` currently calls the pipeline at 12 separate points (`:566, :646, :649,
:657, :663, :980-982, :1007, :1013-1014, :1096, :1101, :1104, :1109, :1132`), each with its
own guard and its own shadow-parameter tuple. After the graph exists, it calls:

```
executor.beginFrame(frameUniforms);       // begin + prepare + shadowcomp
executor.bindGbuffers();                  // world draws happen here
executor.endGbuffers();                   // capture depths, deferred
executor.present();                       // composite + final
```

Four calls. The ordering constraints that are currently encoded in GameRenderer's control
flow move into the graph, where they belong and where they can be asserted.

---

## 6. Work order

Build **once**, at the end. Do not build at stage boundaries. Do not run `ctest`.
See §8.

**Stage A — prove the format bug and kill it.**
1. Add a one-shot dump (temporary, removed in stage H) of `definition.targets` after
   `PackLoader::load` and after `selectDimension`, for root and each dimension.
2. Delete `Loader.cpp:1859`'s `out.targets = seed->targets` and make the dimension pass an
   overlay on root rather than a fresh construction.
3. Write `ShaderPackLoader.DimensionFolderPackKeepsRootFormats` (fixture shaped like RVox).
   Do not run the suite; the test pins the invariant for later.
4. Add the integer-sampler/format mismatch check (§1) and let it log once per program.

*Acceptance:* the log line `scene targets …` reads
`colortex0=r11f_g11f_b10f, colortex1=r32f, colortex9=r32ui, colortex11=rgba16i, …`.
If it does not, stop — everything downstream is measured on a broken pack.

**Stage B — halve cold compile.**
Delete `ShaderCompileService.{hpp,cpp}`, `ShaderCompileRequest`, `ShaderCompileResult`,
`CacheBinaryOutcome`, `loadBinary`, `buildRequest`. `ProgramCache` owns `ShaderBinaryCache`
and compiles once (§3.1). Hoist `binaryProcs()` to a one-time load.

*Acceptance:* exactly one `glLinkProgram`-or-`glProgramBinary` per program per session.
Assert it with a counter in the debug build; the count must equal the enabled-program count.

**Stage C — memoize failure and the source transform.**
`compiledPrograms` becomes `{program, failed}`; insert on failure. Cache `prepareSource`
output per `(cacheKey, stage)`. Fix `normalizePackSource`'s per-call macro-table copy and
`CacheKey` rebuild (`SourceProcessor.cpp:249-276`).

*Acceptance:* with a deliberately broken `gbuffers_terrain`, frame time is unchanged from
the working case. Today it is ~60× worse.

**Stage D — the scan rewrite.**
`PackScan` + per-file directives + include-graph closure (§4). `loadProgramSet`'s two
resource loops die. `buildCommentMask` runs once per raw file, or is replaced by a
streaming scanner that needs no mask at all.

*Acceptance:* pack load wall time for RVox drops below 100 ms total, and
`ShaderPackLoader.*` fixtures produce byte-identical `PackDefinition`s to stage A.
Byte-identical is the test — a faster scan that changes one parsed value is a regression.

**Stage E — option and dimension changes stop re-loading.**
Split `PackLoader::load` into scan (immutable) and resolve (option-dependent).
`clonePack` (`Manager.cpp:557`) and `prepareStagedPack` re-run only resolve.
Delete `PackInstance::sourceCache`'s duplicate in `loadReadCache` (`Loader.cpp:1667`).

*Acceptance:* changing one shader option is under 50 ms wall time, no filesystem access.

**Stage F — `TargetSet` and prebuilt FBOs.**
FBO per output-set, built and status-checked once. `bindWrite` becomes a handle lookup.
Flip parity precomputed for all stages, not just shadowcomp.

*Acceptance:* zero `glCheckFramebufferStatus` and zero `glGenFramebuffers` between the
first and second frame at a fixed resolution.

**Stage G — `FrameGraph` / `PassStep` / `FrameExecutor`.**
The large one. `CompositeRenderer.cpp`'s per-frame body becomes the graph *builder*;
the executor is a flat walk. `Pipeline` and `PackManager` are deleted; their 12 forwarders
and the six-parameter thread go with them. `UniformPlan` replaces `Uniforms.cpp`'s call
list. `core::RenderStage` moves into `RenderType` as a field, deleting the string cascade
in `Pipeline::worldProgram`.

*Acceptance (mechanical, the one that proves the structure moved):*
- `grep -c "shadowColorAltTextureIds" src/` returns 0 outside `FrameResources`.
- No function in the frame path takes more than three parameters.
- A frame at steady state performs **zero** heap allocations in the composite walk —
  assert with a scoped allocation counter in the debug build.
- `PackManager` and `Pipeline` do not appear in the tree.

**Stage H — strip instrumentation, then build.**
Remove the stage-A dump and the three `printf("[diag]…")` in `ColorTargets.cpp`.
Make `logOnce` check the dedupe set *before* formatting. Then `.\build-omega.ps1` once.

---

## 7. Where the deslop rule got taken too literally

You asked. These are the places where "delete the wrapper" was applied to something that
was carrying its weight, and the result is worse:

- **Free functions with 6–9 parameters instead of a value type.** `ensurePackResources`,
  `addPackTextures`, `bindSamplers`, `dispatchSetupIfNeeded`, `renderCompositePasses` all
  take long positional lists because the struct that should hold them was refused as "a
  wrapper". A struct whose fields are the state is not a facade — a facade is a type whose
  *methods* forward. `FrameResources` and `PassStep` are the right answer and they are not
  a violation.
- **`logOnce` as a call-site idiom rather than a type.** It is reimplemented three times
  (`Manager.cpp:756`, `Pipeline.cpp:88`, and the `LogFnLevel` lambda plumbing threaded
  through every `PackCompiler` entry point) because a small logger object looked like slop.
  The cost is a `std::function` parameter on eight signatures and string formatting on the
  hot path.
- **Caches deleted as "state stored alongside what it derives from".** The negative
  uniform-location cache and the failed-program memo are exactly that pattern — and both
  are load-bearing. The distinction the rule is reaching for is *stale* derived state, not
  *memoized* derived state. A memo with one explicit invalidation point is fine.
- **"No speculative flexibility" used to justify recomputing per frame.** Precomputing a
  frame graph is not speculative; the pass list genuinely does not change between frames.

The rule that still holds, and that this plan obeys: **nothing new may forward to something
else.** Every type in §5.2 owns state and is the only owner of it.

---

## 8. Traps

- `.build-omega.lock` — if present, wait. Do not kill processes or delete it (rule 13).
- Match `build-omega/CMakeCache.txt`'s existing `CMAKE_BUILD_TYPE`. Debug from clean is 1.3 h.
- Do not run `ctest` / `-RunTests`. Write the tests; do not execute the suite.
- After building, confirm `Finished … (exit 0)` and a fresh `build-omega/minecraft_native.exe`.
  "Already in use" restores the *previous* binary — a fix that never entered the exe looks
  exactly like a fix that did not work.
- Keep the `[shadow-probe]` / `[renderpearl-probe]` diagnostics in
  `targets/ShadowMapPass.cpp`. Shadows are still unresolved and an empty shadow map and a
  misoriented one look identical on screen.
- Shadow measurements in `shadow-cutoff-not-culling` were taken on a corrupted heap and are
  invalid. Re-measure after stage A; several of them will read differently once the target
  formats are correct.
- `PackLoader::load` calls `resolveShaderIncludes`, which **throws** `IncludeResolveError`
  on a missing or empty include (`IncludeResolver.cpp:69, :78`). `loadProgramSet` does not
  catch. Any restructuring of the scan must keep a pack with one bad include from taking
  down the load of every other pack.
- One shader path only. Vanilla is a shipped pack. Do not add a "no real pack" shortcut
  anywhere in the graph builder.
