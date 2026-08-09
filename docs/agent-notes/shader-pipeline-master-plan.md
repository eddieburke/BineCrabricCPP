# Shader, framebuffer, chunk, and render-pipeline master refactor

Status: active implementation plan, 2026-08-08.

This is the canonical plan. It supersedes proposals that add a `ShaderOrchestrator` facade beside `PackManager` and `Pipeline`, launch GL compilation on arbitrary worker threads, or delete a vaguely named “fallback” without tracing what it owns.

## 1. Outcome

The end state has one concrete shader runtime, one terrain GPU representation, one render-pass execution path, one cache identity model, and one frame timeline. Pack discovery, immutable source data, activated GPU state, frame execution, and terrain publication have distinct owners. No render callback globals remain. No shader source is parsed, resolved, compiled, or looked up by string in a steady-state draw loop. No GL object is created, linked, queried, read back, or destroyed from a worker.

The required user-visible result is:

- Rethinking Voxels loads terrain, entities, hand, clouds, prepare/deferred/composite passes, and final output correctly.
- No `GL_INVALID_ENUM`, incomplete framebuffer, stale program, or silent all-black fallback is permitted during activation.
- Warm pack activation obtains programs from the binary cache and avoids source compilation.
- Changing an option reads no pack files and rescans no unrelated source.
- Chunk and mod geometry use the same section-owned buffer/range publication model.
- `Unmeasured` is a small residual, not an accumulation bucket for half the renderer.
- Every activation failure names the pack, dimension, program, stage, source chain, target, GL operation, and fallback decision.

## 2. What the current source actually does

### 2.1 Ownership and callback machinery

`GameRenderer` owns `PackManager`. `PackManager` owns both pack instances and `Pipeline`, then registers five process-global callbacks:

- `RenderType` world-program resolution
- `RenderType` pass-directive application
- shader object-ID resolution
- `RenderCore` uniform upload
- `RenderCore` material binding

The registrations are in `pipeline/Manager.cpp`; their storage and invocation are in `RenderType.cpp` and `RenderCore.cpp`. This makes draw behavior depend on construction/destruction order and a hidden current pipeline phase. It is not an orchestrator; it is a distributed service locator.

### 2.2 Pack loading and activation

The current code already removed the former incremental activation/prewarm state machine. Activation is synchronous at a controlled transition. That direction stays.

`PackLoader` still combines several jobs in one 2,000-line translation unit:

- source discovery and cached reads
- option discovery and option rewriting
- include expansion
- directive/comment scanning
- target-format inference
- pack property parsing
- program and pass construction
- dimension overlay construction

`PackInstance` then owns raw-source and resolved-source maps, settings, pass buckets, custom uniforms, program cache, and GPU resource state. `resolvedSourceCache` is used both for include-resolved source and transformed stage output. The key domains are strings and are not type-separated.

### 2.3 Compilation and binary cache

GL compilation correctly belongs to the current render context. A former worker-thread GL compilation path deadlocked the NVIDIA driver and was removed. The replacement is not another shared-context compilation pool. CPU-only discovery, file reads, hashing, include resolution, and transformation may run off-thread; `glCreateShader`, link, binary load, reflection, and deletion remain render-thread operations.

Before this work, `ProgramCache` discarded its cache-directory argument. `ShaderBinaryCache`, `compileToBinary`, `extractProgramBinary`, and `loadFromBinary` existed but had no production caller. Every activation therefore source-compiled, regardless of cache files on disk.

The first implementation slice now reconnects those paths. The content identity includes all stages, preamble, compute/raster mode, and the vertex ABI salt. A binary hit loads once. A driver-rejected binary is deleted and source compilation is attempted once. New links request a retrievable binary and enqueue one atomic cache write. Counters distinguish memory hits, binary hits/misses/rejections, source compiles, failures, and stores.

### 2.4 Per-frame shader execution

`Pipeline::worldProgram` and the composite renderer still perform work that belongs to activation:

- world program keys are constructed and compared as strings
- pass resource tables are rebuilt as maps/vectors
- color/image names are erased and reinserted
- compute image uniform names are concatenated each dispatch
- output lists and write-buffer vectors are allocated
- log messages are formatted before one-time suppression
- `ensurePackResources` repeatedly iterates pack declarations

Program compilation is no longer intentionally lazy, yet the draw path retains lazy-compile branches and source-oriented identifiers.

### 2.5 Render targets and GL errors

The target format table now distinguishes integer external formats and integer formats use nearest filtering. The current user log also shows the intended RVox formats, including `r32ui` and `rgba16i`. That does not prove the startup error is gone: `Pre startup` is a late error drain that can report any GL fault since context creation.

`ColorTargets` and `GlFramebuffer` still keep string-keyed/dynamic attachment maps. Flips dirty the G-buffer FBO and force later reattachment/revalidation. Clear batches create or rebuild attachment state. Composite writes resolve string names to handles and may copy read to write before each pass.

The full-frame copies and the synchronous one-pixel `glReadPixels` in center-depth sampling are real GPU/CPU synchronization candidates. They currently fall into profiler `Unmeasured`.

### 2.6 Profiler coverage

Profiling begins inside `renderToCurrentTarget`, after some frame work, and ends before scene-capture resolve/postprocess. Only sky, cull, compile, terrain, entities, particles, hand, translucent terrain, and clouds are named. Prepare, shadow rendering, shadow composite, depth captures, deferred, postprocess, final presentation, weather, overlays, Lua hooks, and framebuffer resolves are mostly outside named stages.

The profiler rejects nested scopes instead of maintaining a stack. `Unmeasured` is frame duration minus the named sum, so missing scopes naturally produce a large and unstable number. The screenshot’s `Hand` scope is nevertheless a real wall-time scope around `renderFirstPersonHand`; it must be remeasured after the later negative-uniform-location caching change, then sampled if still slow.

### 2.7 Terrain and mod meshes

There is no current CPU immediate-mode chunk fallback to “simply delete.” There are two VBO representations:

- terrain atlas geometry: one section-local VBO per terrain layer, drawn through indexed quads
- mod-texture geometry: one `TessellatorMesh` and VBO per texture/layer, collected separately, flattened into a new vector, stable-sorted by texture every render-layer call, then submitted through another draw path

Mod meshes retain their CPU vertex vectors after GPU upload. The publication result moves those vectors into each live section. This duplicates ownership and grows memory with visible mod geometry.

Chunk building itself already has useful boundaries: main-thread capture and pin acquisition, compute-pool snapshot/tessellation, main-thread version validation and GL upload. Jobs use a weak owner and section version. Missing invariants remain:

- a pack’s block-layer mapping is copied into a job but pack generation is not validated at publication
- cancellation is epoch-based and nonblocking; completed and in-flight objects still need a main-thread drain
- completed-channel pressure can block compute workers while upload budgets defer publication
- per-frame near/ring/deferred vectors and full priority scans allocate or scan more than necessary
- block-entity resolution and diffing allocate sets during upload

## 3. Non-negotiable invariants

1. The render thread is the only thread that creates, links, loads, binds, queries, reads, or deletes GL objects.
2. An active runtime is immutable except for explicit frame state, ping-pong indices, temporal history, and counters.
3. Activation is transactional: the old runtime remains drawable until the candidate is completely resolved, compiled/loaded, reflected, target-validated, and framebuffer-complete.
4. A pack switch, option change, dimension change, resource reload, and driver/ABI change each have a named generation and invalidation boundary.
5. No callback or pointer whose meaning depends on “current manager” is stored globally.
6. No shader source text, filesystem path, or arbitrary program name participates in a steady-state draw decision.
7. Every draw program and every pass output is resolved before the first frame of a generation.
8. Failed compilation is memoized for that exact content identity; it never retries every frame.
9. Framebuffer formats, sampler types, image formats, clear operations, and filters are derived from one typed format descriptor.
10. A published chunk result matches section version, world generation, render-settings generation, and pack material-generation.
11. Workers never dereference live world or renderer state after capture.
12. Every frame stage is accounted exactly once; nested CPU scopes work and GPU measurement cannot create illegal overlapping time-elapsed queries.
13. Vanilla/no-pack and shader-pack rendering share draw submission and geometry ownership. Their difference is activated program/pass data, not a second renderer.
14. Hot-path containers have stable capacity or fixed inline storage. A counter proves zero allocations in the measured steady-state path.
15. Debug builds stop activation on target/FBO/program incompatibility and preserve the last valid runtime instead of drawing black.

## 4. End-state ownership

No additional manager/orchestrator layer is introduced. The existing concrete `Pipeline` becomes the sole shader runtime owner and orchestrator, then receives a final name only if the rename improves clarity. `PackManager` is deleted after its state is moved, not wrapped.

```text
GameRenderer
  owns ShaderPipeline
    owns PackCatalogState
    owns ImmutablePackSource
    owns ActivePackRuntime
      owns typed program table and ProgramCache
      owns typed pass execution arrays
      owns target set and framebuffer cache
      owns custom textures, buffers, images, uniforms
      owns generation and runtime counters
  owns WorldRenderer
    owns ChunkSectionSystem
    owns ChunkCompilePipeline
    sections own unified GPU buffers and draw ranges
```

The GUI uses the same `ShaderPipeline` object for catalog selection and settings. It does not call through a second management facade.

`RenderType` remains an immutable descriptor. World shader selection becomes a small `WorldProgramId`/`RenderStageId`, not a string. A render pass is begun with an explicit concrete pipeline reference or an already-resolved POD binding. `RenderCore` receives the resolved program/material/uniform data and never calls back into shader ownership.

## 5. Activated data model

### 5.1 Immutable source snapshot

One pack scan produces:

- normalized virtual paths and one immutable byte buffer per source
- file content hashes and pack aggregate hash
- include graph with reverse dependencies and cycle diagnostics
- option declarations with source locations
- token/comment spans generated once
- parsed properties and dimension overlays
- program-stage source IDs and pass declarations

Directory and zip packs expose the same snapshot API. Reads are bounded and path traversal is rejected before indexing.

### 5.2 Resolved generation

An option/dimension generation contains:

- canonical sorted option values and one option hash
- resolved include/source units keyed by typed `(sourceId, optionHash, dimensionId)`
- transformed shader stages keyed by typed stage identity
- merged target declarations and pack constants
- typed program definitions and ordered pass definitions

Changing an option invalidates only source units whose dependency bitset references it, plus downstream include dependents. It performs zero filesystem or zip reads.

### 5.3 GPU runtime generation

Activation converts resolved declarations into compact runtime arrays:

- `ProgramRuntime`: GL handle, fixed reflected uniform slots, sampler slots, image slots, debug source identity
- `WorldProgramTable`: fixed array from `WorldProgramId` and draw phase to program index/fallback index
- `PassRuntime`: program index, fixed output slot list, flip mask, sampler/image bindings, dispatch or fullscreen-draw data, mipmap work, profiler stage
- `TargetRuntime`: typed format descriptor, read/write handles, clear value, scale, generation
- `FramebufferRuntime`: numeric attachment key, draw-buffer array, FBO handle and completeness state
- `ResourceRuntime`: resolved buffer/image/texture handles and fixed binding slots

Names remain only in loader diagnostics and the settings UI. Runtime arrays use indices and bitsets.

## 6. Activation transaction

The activation pipeline is linear:

1. Observe one catalog/watcher generation.
2. Read or reuse the immutable source snapshot.
3. Resolve selected options and dimension on CPU.
4. Transform stages and compute stable content identities on CPU.
5. On the render thread, load binary programs or source-compile misses.
6. Reflect every program once into fixed slots.
7. Allocate and validate candidate targets/resources.
8. Build the pass execution arrays and exact framebuffer attachment keys.
9. Validate every required world-stage fallback, sampler/image compatibility, draw-buffer limit, target format, and framebuffer.
10. Atomically exchange active generation.
11. Retire the old generation on the render thread after no active scope references it.

If any required step fails, candidate objects are destroyed, the old generation stays active, and one structured diagnostic is shown. Optional program failure follows Iris-compatible fallback resolution and is recorded; it does not silently bind program zero unless that is the declared fallback.

Settings changes use the same transaction but reuse the immutable pack snapshot. Dimension changes reuse source bytes and option metadata. Filesystem watcher callbacks only publish a changed generation; they never mutate active pack state.

## 7. Compiler and cache design

### 7.1 Stable cache identity

The cache key includes:

- canonical transformed bytes of every present stage
- version/preamble and injected defines
- compute/raster/tessellation mode
- vertex ABI salt and transformer version
- option and dimension effects already represented by transformed bytes
- binary cache layout version

Driver binaries are validated by `glProgramBinary` and link status. A driver rejection deletes the entry. A later phase may namespace storage by vendor/renderer/version/binary formats to avoid even that rejected attempt after a driver change.

### 7.2 Storage

- one file per content identity
- fixed header with magic, layout version, identity, binary format, flags, byte count, and checksum
- strict maximum size and full-length read checks
- write to same-directory temporary file, flush/close, atomic replace
- bounded writer queue with duplicate-key coalescing
- writer shutdown drains accepted writes
- corrupt/truncated/rejected entries are removed
- no directory enumeration on the render hot path

### 7.3 Compilation scheduling

CPU source work can run concurrently under an activation cancellation token. GL binary loads and links run on the render thread behind the existing progress UI. `glMaxShaderCompilerThreadsKHR` may allow the driver to parallelize internally. Shared-context worker compilation is prohibited until a dedicated, driver-tested context-ownership design exists; it is not required for this refactor.

Activation reports per program and aggregate:

- CPU resolve/transform time
- binary lookup/read time
- binary hit/miss/reject
- shader compile and program link time
- reflection time
- cache enqueue/write time

Warm activation acceptance is based on hit counters, not elapsed time alone.

## 8. Render pass execution

At activation, build the exact ordered pass arrays for begin, shadow composite, prepare, deferred, composite, and final. Each entry already knows its program, outputs, flip transition, resource bindings, dispatch dimensions, and profiler stage.

At frame time a pass does only:

1. Select its prebuilt framebuffer key from current numeric flip bits.
2. Bind program/FBO if changed.
3. Upload changed frame/pass uniform slots.
4. Bind fixed sampler/image/buffer slots.
5. Dispatch or draw.
6. Apply its numeric flip mask.

No pass-time include resolution, program compile, string comparison, `to_string`, map erase/insert, output-name lookup, or fresh vector is allowed.

World rendering resolves one `WorldProgramId` at pass begin. Terrain sections then update only section offset and per-draw material data. Uniform upload is generation/dirty-bit driven; `glGetUniformLocation` is reflection-time work. Missing locations occupy a valid cached sentinel.

## 9. Targets, framebuffers, and GL correctness

### 9.1 Typed formats

One descriptor supplies internal format, external upload/clear format, scalar type, integer/signed/normalized/floating class, filter legality, image format, bytes per texel, and clear API. All allocation, clear, image binding, and validation use it.

Integer color targets use integer external formats and nearest filtering. Integer clear uses the correct signed/unsigned API. Unsupported pack formats fail candidate activation with the target name and capability.

### 9.2 Framebuffer lifecycle

Target allocation creates stable read/write handles. Flips swap numeric indices; they do not destroy an FBO. Activation simulates the deterministic pass sequence and creates the exact attachment combinations it will use. Conditional combinations are cached by fixed numeric key on first use. Attachment arrays use inline storage bounded by queried GL limits.

Completeness is checked once after construction and whenever a referenced texture is reallocated. Resizes build a complete replacement set before exchange.

### 9.3 Copy and synchronization removal

Audit every `prepareWrite` copy against pass semantics. Preserve read contents only for outputs that need partial-write history. Precompute that requirement from pass output/flip/liveness data. Do not full-screen blit every output by default.

Replace synchronous center-depth readback with a small ring of pixel-pack buffers and fences, consuming the newest completed value without waiting. If the value is only needed by shaders, keep it on GPU. Depth capture uses explicit blit/copy commands selected once by capability and measured as named stages.

### 9.4 Error attribution

Enable the GL debug callback in debug builds and attach pack/program/pass/target labels to GL objects and debug groups. Drain/assert errors at context initialization, candidate target allocation, candidate program load/link, pass execution boundaries, and presentation. `Pre startup` must no longer be the first attribution point.

## 10. Profiler rebuild

Move frame begin/end to encompass scene capture, world rendering, postprocess, final resolve, HUD/GUI, and presentation as far as the application loop permits.

CPU scopes use a real stack. Parent inclusive and self time are both recorded. The stage set includes at minimum:

- frame setup
- pack prepare
- shadow cull/build/draw
- shadow composite
- sky
- cull
- chunk capture/build-ready/upload
- opaque terrain
- entities/block entities
- particles
- opaque depth capture
- hand
- hand depth capture
- deferred
- translucent entities/particles/terrain
- center-depth issue/consume
- weather/clouds/overlays
- composite/final
- scene resolve
- Lua hooks
- HUD/GUI
- present/wait

GPU measurement uses timestamp queries or non-overlapping leaf queries; nested `GL_TIME_ELAPSED` scopes are not emitted. Query results are consumed several frames later without waiting.

`Unmeasured` becomes the clamped difference between outer-frame time and top-level accounted time. It is not accumulated across frames. HUD output includes whether CPU/GPU samples are pending and shows counters for draw calls, program binds, uniform uploads, location queries, binary hits, source compiles, FBO creations, bytes copied/uploaded, sync waits, and hot-path allocations.

## 11. Hand-rendering diagnosis and target

The 92 ms screenshot predates later uniform-miss caching in the current source, so the first gate is a fresh run with cache counters and complete stages. If hand remains slow:

1. sample program bind, uniform upload, texture lookup/upload, model tessellation, and draw separately
2. prove whether the stall is CPU work or waiting for earlier GPU work
3. ensure skin download completion is applied outside repeated hand draw setup
4. cache the resolved skin texture handle by texture generation, not merely by player string
5. reuse static arm/item geometry where animation only changes matrices
6. bind the hand world program and fixed uniforms once per hand pass

Acceptance: steady-state hand CPU under 2 ms at the reference resolution, zero source compiles and zero uniform-location queries in the hand scope, with no correctness regression for empty hand, map, item, water, fire, and block overlays.

## 12. Unified chunk and mod mesh pipeline

### 12.1 GPU representation

Each section generation publishes one or a small fixed number of GPU buffers with draw ranges:

- layer
- first vertex/index and count
- material/texture ID
- sort class
- bounds

Terrain-atlas and mod-texture faces differ only by material ID/range. `TessellatorMesh` is a worker result, not live render ownership. CPU vertex arrays are released immediately after successful upload unless an explicit debug/recovery mode requests retention.

`ChunkBuilder::drawLayer` and `WorldRenderer::renderModChunkMeshes` collapse into one section-layer command traversal. The command builder groups opaque/cutout ranges by material at publication or visible-set change. Translucent ordering remains explicit and correct; it is not hidden behind an opaque texture sort.

There is no per-layer per-frame flat vector construction or `stable_sort`. If supported and beneficial, adjacent ranges sharing state use multi-draw, but the semantic path remains identical without it.

### 12.2 Publication identity

Every mesh job captures:

- section weak owner and section version
- world generation
- render-settings generation
- material/block-layer pack generation
- immutable region snapshot and copied render inputs

Main-thread publication rejects and reschedules any mismatched identity. Pack activation increments material generation and dirties only sections whose material/layer mapping is affected.

### 12.3 Queueing and cancellation

- bounded work queue and bounded completed queue have explicit high/low water marks
- main thread always drains completion metadata even when upload byte/time budget is exhausted
- deferred uploads retain compact results and are capped by bytes, not only job count
- cancellation marks generations; completed stale jobs are cheaply discarded on main thread
- section destruction never depends on worker destruction order
- render pins are acquired/released exactly once and reported by counters
- upload budget accounts bytes and GL calls, with near-camera priority but no unlimited bypass
- dirty scheduling uses stable priority queues or buckets rather than scanning the entire section order each frame

### 12.4 Buffering

Use capacity-aware buffers: reuse when capacity suffices, orphan/grow when needed, and retire safely after GPU use. Track allocated capacity, live bytes, upload bytes, reallocations, and fragmentation. Evaluate persistent mapping only after the unified ownership is correct and profiled; it is not a prerequisite.

## 13. Deletion list

The refactor is incomplete until these are removed:

- all five global shader callback setters, globals, registrations, and clears
- `PackManager` forwarding methods and duplicate ownership
- string world-program resolver and last-key caches
- render-time lazy source compilation branches
- shared use of `resolvedSourceCache` for unrelated key domains
- per-frame composite string maps and generated uniform names
- flip-driven G-buffer FBO destruction/rebuild
- synchronous center-depth `glReadPixels`
- separate live `modLayerMeshes_` CPU/GPU representation
- per-frame mod-mesh flatten and stable sort
- profiler’s single active-stage slot and incomplete frame boundary
- dead cache/prewarm/activation state and stale documentation after migration

## 14. Migration phases and gates

### Phase A — make failure and cost observable

- complete frame profiler and nested CPU timeline
- add binary/source/link/reflection counters and timings
- add GL debug attribution and object labels
- add target/FBO/program activation validation
- record hot-path allocation, copy, upload, draw, and sync counters

Gate: one RVox frame has named accounting for at least 95% of outer-frame CPU time; every GL error names its operation; hand and `Unmeasured` have causal subscopes.

### Phase B — finish compiler/cache repair

- verify the newly connected production binary cache with an actual cold/warm test
- add checksum and bounded/coalesced writer queue
- separate raw, include-resolved, and transformed caches by typed key
- remove render-time lazy compilation
- pre-reflect every fixed location and binding
- make activation failure transactional

Gate: second activation has binary-hit count equal to every cacheable program and source-compile count zero; rejected binary recovers once; failed program never retries per frame.

### Phase C — split scan from resolve

- create immutable source snapshot and include dependency graph
- move pure scanners out of the loader god translation unit by responsibility
- option/dimension changes reuse source bytes and dependency metadata
- introduce typed source/program/pass IDs
- cancel superseded CPU resolve jobs by generation

Gate: an option change performs zero file/zip reads, touches only dependent units, and reports exact invalidation counts.

### Phase D — collapse manager and callbacks

- move catalog, selected pack, active/base instances, watcher state, and activation into `Pipeline`
- route UI calls to that concrete owner
- thread explicit pipeline/pass bindings through renderer entry points
- replace `RenderType` strings with fixed world-program IDs
- delete callback globals and `PackManager`

Gate: no shader setter/global callback symbols remain; constructing two renderer instances cannot cross-wire state; vanilla and pack rendering pass the same draw-submission tests.

### Phase E — compile pass execution arrays

- build fixed runtime program/target/resource tables
- precompute pass outputs, bindings, flips, copies, mipmaps, dispatches, and profile IDs
- remove per-frame maps, strings, vectors, and lazy resolution
- bind/upload only dirty state

Gate: after warmup, the shader execution path allocates zero heap blocks and performs zero `glGetUniformLocation` calls per frame.

### Phase F — target/FBO/sync rebuild

- make all target operations use the typed format descriptor
- prebuild/cache numeric framebuffer attachment combinations
- eliminate flip-triggered FBO rebuild
- precompute necessary preservation copies
- replace synchronous depth readback

Gate: zero GL errors; zero incomplete FBOs; zero synchronous readback waits; framebuffer creations stop after activation/resize; image-heavy RVox passes match declared formats.

### Phase G — unified section meshes

- publish section GPU buffers with atlas/mod material ranges
- remove live CPU retention after upload
- build stable draw commands on publish/visibility change
- add pack/world/settings generation validation
- bound completion/deferred queues by bytes and remove full scans
- delete separate mod-mesh render path

Gate: every visible section/layer uses one command representation; per-frame mod sort/allocation is zero; stale jobs cannot publish across world/pack/settings changes; mesh memory and upload counters stabilize.

### Phase H — cleanup and parity hardening

- remove superseded code and stale docs
- compare fallback, flip, target, shadow, prepare, deferred, and final semantics to Iris
- add pack corpus diagnostics and deterministic activation reports
- tune only from measured stage/counter data

Gate: reference packs meet correctness and budget matrix; no compatibility exception reintroduces a second render path.

## 15. Verification matrix

### Automated

- source/comment/directive scanner tests with active, inactive, and commented declarations
- include graph, cycle, missing include, option dependency, and dimension overlay tests
- transformed-stage golden tests including RVox `prepare`
- program cache cold store, warm binary load, corruption, truncation, driver rejection, and failure memoization
- integer/float/normalized target allocation, clear, filtering, image binding, flip, resize, and FBO completeness
- pass execution-plan tests for output masks, flip transitions, partial-write preservation, and fallback selection
- profiler nesting, inclusive/self accounting, delayed GPU query, frame-boundary, and residual tests
- chunk job generation/cancel/stale publish tests
- unified atlas/mod draw-range golden tests and CPU-release assertions
- bounded queue/backpressure tests

Final focused commands after the one required Debug client build:

```powershell
.\build-omega.ps1 -BuildType Debug -Target Client
cmake --build build-omega --target minecraft_omega_tests -j 4
.\build-omega\minecraft_omega_tests.exe --gtest_filter="ShaderGlIntegrationTest.*:RvoxDiag.CompileAllPrograms:RvoxDiag.EnsureSceneTargetsRthinkingVoxels:MacroParity.*:ColorTargetsTest.*:RenderProfilerTest.*:MeshCancelTest.*:ChunkMeshGoldenTest.*"
```

### Manual reference packs

- vanilla/no pack
- Rethinking Voxels r0.1 beta 9
- one Complementary-family pack
- one lightweight Iris-compatible pack
- pack with compute, tessellation, integer images, custom textures, SSBOs, custom uniforms, dimension overrides, and option profiles

For each: cold startup, warm startup, world enter, dimension switch, pack switch, repeated option change, resize, resource reload, chunk travel/teleport, mod-block rebuild, hand/item/map/overlay, weather/clouds, shadow on/off, and clean shutdown.

### Initial budgets

- no startup/world-load GL errors
- all expected chunks visible; no cloud-only/black-smudge frame
- steady Hand CPU under 2 ms
- steady profiler residual under 5 ms and under 5% of frame time
- zero steady-state source compiles, program links, uniform-location queries, FBO creations, and shader hot-path allocations
- warm pack activation uses binaries for all cacheable programs
- option-change source I/O count zero
- no main-thread wait on shader-cache I/O
- no synchronous center-depth readback wait
- chunk upload work remains within declared frame budget except explicit loading screens

Budgets are tightened after Phase A records reproducible hardware data; correctness gates are not relaxed to meet timing.

## 16. Work already landed in this effort

- production `ProgramCache` now owns and uses `ShaderBinaryCache`
- content-keyed binary load precedes source compilation
- rejected binaries are deleted and recover through one source compile
- successful links request/extract/store program binaries
- cache-path counters expose whether a warm activation truly hit disk binaries
- a GL integration test exercises cold compile/store followed by warm binary reload

The next implementation order is Phase A profiler/error attribution, then Phase B cache hardening and activation transaction. Chunk unification begins after generation counters are established so stale worker results cannot cross the new runtime boundary.
