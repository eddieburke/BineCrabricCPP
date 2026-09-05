**Render, Iris, atmosphere, and Lua cleanup plan — 2026-09-05**

Implementation status after the follow-up cleanup:

- Implemented: fancy clouds construct and upload geometry once for both depth/color draws; shared submission no longer duplicates buffered/streaming draw setup.
- Implemented: compositor output defaults, screen classification, shadow attachment count and flip sequences resolve during pack activation. Runtime texture maps and target-size checks still remain.
- Implemented: particles, lit particles, weather and hand now enter the existing world-render stage hook contract. Split entity phases and subscription buckets remain unfinished.
- Corrected from the preceding patch: absent pipeline snapshots no longer invalidate every draw; frame snapshot changes no longer force unchanged draw matrices; removed unused atmosphere camera field and repeated cloud-height calculation.
- Implemented: unused MVP aliases avoid matrix work; inverse MVP uses existing inverse matrices; removed duplicate identity normal upload.
- Still outstanding: begin/shadow scheduling and shadow compute support, complete frame/fog atmosphere ownership, retained cloud topology, Lua filter buckets and geometry marshalling, compositor resource binding compilation, entity preparation split, terrain grouping.
- User superseded the test/performance gates below: do not add or run tests or benchmarks for this work. Build verification only; no measured speedup or in-game parity claim.

Fix pass correctness and state ownership first, then remove repeated work from the measured hot paths. Extend the existing pipeline and runtime plans. Keep each change attributable to a specific defect or measured cost.

This is a source audit and implementation plan. No game profiling, builds, or test execution occurred during planning. The mechanisms below are visible in the current source; their contribution to the reported symptoms and frame time remains unmeasured. Source changes from concurrent work must be rechecked before implementation.

**1. Evidence and priority**

| Priority | Source observation | Consequence to verify | Work item |
|---|---|---|---|
| P0 | `GameRenderer::renderToCurrentTarget` calls `shadowmap::update` before `renderBegin`. The checked-in Iris pipeline runs `beginRenderer.renderAll()` in `beginLevelRendering`, before `renderShadows`. | A begin pass preparing data for shadow geometry runs too late. | A: frame execution contract |
| P0 | `Loader::addComputePrograms` accepts composite prefixes and `setup`; `PackInstance::buildExecutionPlan` has no shadow-compute stage. Iris explicitly dispatches `shadowComputes`. | `shadow.csh` and its supported suffixes may be silently omitted. This is separate from the existing, supported `shadowcomp*.csh` path. | A |
| P0 | `RenderCore::bindAndUploadUniforms` writes draw alpha, entity color, lighting and sky values before a possible `bindWorldProgramState` snapshot upload. `uploadShaderUniforms` writes several of the same uniforms. Fog has an explicit later override. | First-use or invalidated snapshots can overwrite current draw values. Prove individually for alpha, entity color and lighting. | B: uniform and GL state ownership |
| P0 | `blendFunc` uploads four components, but its dirty comparison checks only source/destination RGB. | Changing only alpha blend factors can leave the shader's `blendFunc.zw` stale. | B |
| P1 | `renderWorldStage` calculates rain, stars, cloud color and cloud height before checking whether any matching Lua subscriber exists, including disabled stages and the shadow entity stage. | Empty hooks still cause atmosphere queries; every subscribed stage repeats the same payload preparation. | C, E |
| P1 | Fog preparation runs before the frame's `updateSunLight`. Sky-color getters execute Lua hooks; fog preparation, frame uniforms, the dome and the void each request sky color. | A color hook can see the preceding celestial state, then run again with the current one in the same frame. | C: frame atmosphere |
| P1 | `drawBackgroundFan`, cloud color and star brightness use world time; celestial bodies use published `CelestialState`. Clouds also interpolate the camera entity independently. | Realtime sky/custom camera paths can disagree about time, color, orientation and camera origin. | C |
| P1 | Fancy clouds traverse the 40×40 grid inside both depth and color passes and emit the mesh twice. Fast clouds traverse 64×64 cells per call. | Duplicate CPU mask tests, culling, vertex construction and streaming. Actual GPU need for two passes must remain intact. | D: atmosphere geometry |
| P1 | Cloud-mask caching keys on texture ID only. | Reloading pixels into the same texture ID can retain the old silhouette. | D |
| P1 | `WorldRenderStage` exposes particles, lit particles, weather and hand, but their current draws bypass `renderWorldStage`. The stage helper discards its `shadowPass` argument. | Public hooks can be absent or cannot distinguish shadow draws. Split translucent entities also run outside the first Entities hook pair. | E: Lua event contract |
| P1 | `callLuaEvent` takes a recursive mutex and creates a fresh event table for each callback. `luaHookWorldRender` scans once for a match and dispatch scans again. | Active subscriptions incur table/marshalling/locking cost even when most filters reject them. Contention is not yet demonstrated. | E |
| P1 | `separateEntityDraws` invokes the complete `WorldRenderer::renderEntities` path twice, filtering draws later through `DrawPhase`. | Repeated culling, dispatcher preparation, model work and hooks; counters/cooldown also depend on number of render invocations. | F: entity preparation |
| P1 | `renderCompositePasses` builds three string-keyed maps per stage and copies output vectors in the raster loop. Sampler binding rediscovers active bindings. | Repeated allocations, hashing and directive/binding work scale with pack pass count. | G: compiled bindings |
| P2 | `uploadDrawMatrixAliases` computes MVP and its inverse even when a shader does not consume those aliases. Main camera transformation is rebuilt for basis extraction. | Avoidable matrix work; prioritize by profile rather than function size. | B, H |
| P2 | Terrain draw-list construction already walks visible sections once, but region changes trigger a linear search through existing region buckets. | Scattered visibility can still make grouping expensive. The old four-walk benchmark does not prove the current implementation has four walks. | H: terrain and uploads |

Primary entry points: [GameRenderer.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/client/render/GameRenderer.cpp:803>), [RenderCore.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/client/render/RenderCore.cpp:327>), [CompositeRenderer.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/client/render/pipeline/CompositeRenderer.cpp:65>), [LuaDirectHooks.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/mod/runtime/LuaDirectHooks.cpp:1193>).

**2. Establish the baseline before optimization**

Use the same executable configuration, world, camera route, display size, render distance, mod set and pack settings for before/after runs. The inspected build cache is **Release**; preserve the configured build type and recheck it at execution time. Do not compare Debug timings with Release timings.

| Scenario | Purpose |
|---|---|
| Embedded vanilla; mods disabled; stationary terrain | Cost without external shaders or subscribers |
| Embedded vanilla; runtime enabled with no matching hooks | Cost of the empty subscription path |
| One no-op hook, then many hooks filtered to other stages/entities | Dispatch and filter scaling |
| `realtime_sky`, `layered_clouds`, `northern_stars`, then combinations | Native/Lua atmosphere interactions and geometry cost |
| Entity-heavy area; separate entity draws on/off | Preparation duplication, hooks and submission |
| Flying into new terrain, then a stationary world with no dirty chunks | Capture/upload spikes versus steady rendering |
| RenderPearl, Complementary Reimagined, rethinking-voxels; same saved settings | MRT, translucency, compute and shadow workloads |
| Dawn/dusk, rain, underwater, custom camera, resize and pack reload | Correctness and invalidation boundaries |

The external pack folders found locally are `RenderPearl v2.8.0-beta.4`, `ComplementaryReimagined_r5.8.1`, and `rethinking-voxels_r0.1-beta9`; also test the local `ComplementaryCleanroom`. Their presence does not establish which pack the user was running.

Collect CPU render time and whole-frame p50/p95/p99 separately from present/vsync pacing. Use bundled gdb stack sampling for CPU attribution; report distributions separately from uninstrumented timings because attaching pauses the process. For GPU attribution, use asynchronous timestamp queries or an external GL profiler, consuming only completed query results. Do not add synchronous GPU waits for measurement.

Measure allocations/bytes, emitted and uploaded vertices, draw/program/FBO changes, uniform writes, hook counts/table construction, mesh captures/uploads and backlog. Keep these in focused benchmark/test tooling or existing diagnostics, not permanent per-frame logs. Retain raw results with the implementation report.

Performance acceptance: structural reductions below must hold; an optimized scenario must improve beyond observed run-to-run noise. Investigate repeatable p95 regressions above 5% in another baseline scenario. This is a proposed regression gate, not a promised speedup. Start with warm-up and at least three identical timed runs; extend sampling if noise obscures the result.

**A. P0 — Make the actual frame sequence testable**

Scope: `GameRenderer`, `Pipeline`, `PackInstance`, `ShadowMapPass`, compute discovery/execution. Size: large; prerequisite for changes to pass scheduling and binding caches.

1. Pin the checked-in Iris behavior in a compact contract document, with exact reference files and the cases where Beta semantics deliberately differ. Current upstream documentation supplements that reference; it does not silently redefine the target version.
2. Separate shadow target preparation from shadow geometry submission. The current `shadowmap::update` combines allocation, clearing, frustum setup and drawing, which obstructs correct ordering.
3. Represent the required sequence directly in the existing frame owner: resource transitions and clears at their specified boundaries; setup when required; shadow computes at the Iris boundary; begin; shadow geometry; shadow composite; prepare; sky/opaque world; hand/opaque depth snapshots; deferred/translucent phases; composite/final. Preserve exact clear, snapshot and compute ordering from the reference rather than treating this abbreviated sequence as sufficient by itself.
4. Add explicit discovery and a runtime slot for supported shadow compute programs. Preserve suffix ordering, dimension selection, enable conditions, indirect dispatch and workgroup behavior. Reject invalid executable plans with a useful pack/pass error instead of silently dropping work.
5. Build current frame data once. Shadow and additional views consume that frame's time/celestial data and use their own camera state without advancing primary-view history again.
6. Test a real execution path using tiny synthetic packs: begin writes a marker that shadow geometry reads; shadow compute writes a marker consumed at the reference boundary; shadow composite and prepare propagate markers into gbuffers. Include disabled shadow rendering and compute-only stages.

Acceptance: executed GL/pass events and marker values match the pinned reference, including the first frame and after resize/reload. `shadowcomp` behavior stays covered. Matrix-only tests are insufficient.

Evidence: [IrisRenderingPipeline.java](<C:/Users/Eddie/Documents/New project 2/third_party/mcp/iris/pipeline/IrisRenderingPipeline.java:875>), [Loader.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/client/render/shaderpack/Loader.cpp:1555>), [Instance.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/client/render/pipeline/Instance.cpp:99>), [ShadowMapPass.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/client/render/targets/ShadowMapPass.cpp>).

**B. P0/P1 — Give every uniform and GL state one owner**

Scope: `RenderCore`, `RenderType`, `Uniforms`, `ShaderProgram`, `WorldProgramBinder`, pipeline bindings and target state. Size: large; first land the small, proven correctness fixes.

1. Classify existing values as frame, view/pass, material or draw state. Stop the frame uploader writing values owned by a later draw. Preserve custom-uniform override semantics against Iris rather than choosing an arbitrary winner.
2. Exercise the production pipeline binding path on a shader's first draw, second draw, A→B→A program switches and a new snapshot generation. Check alpha threshold, entity color, lighting, fog, sky color, IDs and matrices using active GLSL uniforms or controlled draw output.
3. Compare all four `blendFunc` components. Test RGB-constant/alpha-changing transitions, indexed blends and restoration after nested passes.
4. Retain the existing per-program uniform snapshot cache. Audit its invalidation for reload, relink, resize, deferred flips, nested FBO rendering and direct texture changes. A correct uniform cache does not imply texture-unit bindings remain correct.
5. Compile material and sampler metadata once. `makeWorldBindContext` currently resolves PBR/material information for both state binding and material binding; avoid repeating that resolution on a transition that needs both.
6. Replace repeated `glGetIntegerv(GL_FRAMEBUFFER_BINDING)` and whole-pack directive scans only after framebuffer ownership covers all raw GL binding sites. Carry tracked draw/read FBO identities separately. A `glGet` call alone is not proof of a GPU stall.
7. Compute inverse/MVP/normal matrices only when a program consumes them and their dependencies changed. Reuse already computed camera matrices; retain distinct hand, shadow and custom-view transforms.

Acceptance: first draw equals subsequent draws for unchanged inputs; alpha-only blend changes reach GLSL; nested draws restore the parent's render target, camera, program identity, depth/blend/texture state. Unchanged draws do not trigger frame-uniform uploads or repeated binding discovery. No blind removal of pass scopes that enforce restoration.

Evidence: [Uniforms.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/client/render/uniforms/Uniforms.cpp:14>), [PackLifecycle.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/client/render/pipeline/PackLifecycle.cpp:107>), [GlState.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/client/render/GlState.cpp:497>). Iris specifies separate RGB/alpha factors and per-buffer blending: [rendering directives](https://shaders.properties/current/reference/shadersproperties/rendering/).

**C. P1 — Resolve atmosphere once, before its consumers**

Scope: `GameRenderer`, `AtmosphereContext`, `CelestialState`, fog/frame data, sky consumers and bundled sky mods. Size: medium/large; follows the ownership decisions in A/B.

1. Resolve active pack/dimension settings, the view camera and current celestial overrides before evaluating color/fog hooks that consume them. Eliminate reads of the preceding published celestial state during current-frame fog preparation.
2. Extend the existing atmosphere value/context to carry resolved sky/fog/cloud colors, rain/thunder, star brightness, cloud height and the published celestial state. Use one evaluation per logical view/frame, with explicit reuse for the shadow view. Pass references to consumers.
3. Remove effectful repeated rendering reads of `World::getSkyColor` and `getFogColor`. Preserve server/world-query behavior; move rendering hook orchestration to the frame owner instead of putting client render caches into `World`.
4. Decide each value's semantics explicitly: world clock, mod celestial override or resolved render weather. The sunset fan, native stars, clouds, pack uniforms and bundled Lua providers must not quietly select different clocks. Preserve documented independent moon/star overrides.
5. Use `FrameRenderCamera`/eye coordinates for atmosphere placement. Custom cameras and third-person views must not fall back to independently interpolated player coordinates.
6. Make sky, sun, moon and stars obey their resolved flags independently. Currently the outer `renderSky` stage gates a function that also draws both celestial bodies. Preserve horizon and void coverage while separating the gates.
7. Keep default stars and sky geometry cached. Their meshes already persist; fix ownership/reload invalidation rather than implementing another star cache. Reuse small static fan/body topology only if profiling makes it worthwhile.
8. Update `realtime_sky` and `layered_clouds` to consume the resolved fields and remove redundant native-clock reconstruction where the public API guarantees the value.

Acceptance: one current celestial answer and a bounded number of color callbacks per view; no additional color callbacks per unrelated render stage. Test discontinuous time changes, dawn/dusk boundaries, direction overrides, custom cameras, rain and disabled celestial components. Read-only stage metadata and mutable overrides must be distinguished explicitly.

Evidence: [World.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/world/World.cpp:146>), [SkyDome.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/client/render/atmosphere/SkyDome.cpp:142>), [CelestialState.hpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/client/render/celestial/CelestialState.hpp>), [realtime sky hooks](<C:/Users/Eddie/Documents/New project 2/mods/realtime_sky/scripts/hooks.lua:335>).

**D. P1 — Stop rebuilding atmosphere geometry unnecessarily**

Scope: native cloud/weather renderers, mesh submission and hot bundled Lua geometry. Size: medium; uses C's resolved state.

1. Build fancy-cloud geometry once and submit it for the required depth and color passes. Preserve color mask, blend, depth-write and shader behavior. First remove duplicate construction; do not begin with a GPU architecture rewrite.
2. Cache cloud topology by resource generation and integer cloud-cell origin. Account for camera-relative top/bottom visibility and fancy/fast mode. Continuous motion, color and opacity should update transforms or draw inputs where the existing mesh ABI permits.
3. Separate camera-dependent visibility from stable topology. Do not reuse a mesh culled for another camera orientation; choose coarse cached patches or regenerate the visible submission list. Bound memory by actual cloud coverage.
4. Invalidate the cloud mask on resource reload even if the texture ID is reused. Test arbitrary supported texture dimensions; remove fixed texture-coordinate assumptions only after matching the texture contract.
5. Reuse precipitation scratch storage. Profile repeated biome/light queries and per-column draw flushes; batch compatible rain/snow geometry while retaining texture, lighting and ordering semantics.
6. Profile Lua `quads`/`billboards` marshalling independently. Packed quads still make nine Lua number lookups per vertex; billboards convert direction→angles→direction and repeat trigonometry. Use directions directly and precompute batch rotations.
7. If bundled star/cloud geometry dominates, add the smallest retained native mesh/buffer handle needed by those mods, with explicit update/release and mod-unload cleanup. Migrate one actual caller immediately. Preserve the existing table API; do not invent a general scene scripting framework.

Acceptance: fancy depth/color passes share one geometry build; static star data is not remarshalled every frame after migration; resource reload changes cloud silhouettes; custom camera changes cannot reveal stale culling. Performance claims include both native generation time and Lua marshalling time.

Evidence: [CloudRenderer.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/client/render/atmosphere/CloudRenderer.cpp:67>), [PrecipitationRenderer.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/client/render/atmosphere/PrecipitationRenderer.cpp:22>), [LuaRenderBindings.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/mod/runtime/LuaRenderBindings.cpp:41>).

**E. P1 — Make Lua hooks complete, cheap when idle, and safe when nested**

Scope: direct hooks, event marshalling, prelude filters, renderer call sites, host lifecycle, bundled mods. Size: large; define semantics before reducing callback counts.

1. Inventory every public hook: caller, frequency, side/thread, filter keys, input fields, mutable result fields, cancellation and nested-call behavior. Cover render/tick/entity/world/generation/input hooks; shared dispatch changes can affect all of them.
2. Define `world_render` stage semantics for opaque/translucent entities, lit/translucent particles, weather, hand and framebuffer. Preserve existing names and the separate first-person-hand hook. Add an explicit view/pass discriminator where callbacks can run for shadows or additional views.
3. Route every supported stage through its contract exactly once per specified moment. Define whether disabled stages remain observable and ensure cancellation suppresses only that stage's vanilla draw. Preserve priority/order and the ability for later callbacks to observe earlier changes.
4. Cloud color/height are currently input metadata; only selected fields are read back. Make them explicitly read-only unless a concrete consumer needs mutation. A writable field must affect the actual draw. Remove unused astronomy fields only after checking bundled and public consumers; do not silently turn metadata into a different API.
5. Move the matching-subscription test ahead of payload construction and optional scopes. Retain the existing event-level `hasLuaHook` cache, stage/moment masks and entity type filter. Refine to precomputed stage/moment buckets and static native filters at registration time, replacing the current double scan. Keep dynamic `when` predicates in Lua.
6. Measure table creation and marshalling per callback. Size tables appropriately and avoid rebuilding shared immutable context fields where safe. Do not pool mutable event tables indiscriminately: a mod can retain a reference or invoke a nested hook, so reuse could mutate prior events.
7. Establish thread ownership of registration/unload/dispatch before changing synchronization. `gHookTable` publication and mod `active` checks are separate from the per-mod Lua mutex. Preserve immutable dispatch snapshots and defer unsafe registration/unload mutations to an owner boundary; prove cross-thread cases instead of assuming locks are the bottleneck.
8. Test callback errors, disable/unload during dispatch, nested hooks, cancellation chains and missing return values under the existing prelude contract. Unwind Lua stack, mod context and render state on every exit. Use a clear fault policy with bounded repeated error reporting; do not swallow failures or spam every frame.

Acceptance: no matching subscriber means no Lua table/mutex/payload construction; relevant subscribers receive the documented stages and resolved state; nested/error/unload cases restore all parent state. Count changes must be accompanied by bundled-mod behavior tests, not just syntax checks.

Evidence: [LuaEventGlue.hpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/mod/runtime/LuaEventGlue.hpp:49>), [LuaDirectHooks.hpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/mod/runtime/LuaDirectHooks.hpp:48>), [LuaRuntimePrelude.hpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/mod/lua/LuaRuntimePrelude.hpp:18>).

**F. P1 — Prepare entities once per view, submit at the required phases**

Scope: `WorldRenderer`, entity/block-entity dispatchers, living/model renderers and entity hooks. Size: large; follows A/B/E.

1. Gather the visible entity and block-entity candidates once per camera/view. Keep shadow visibility separate; a shadow caster is not necessarily visible to the main camera.
2. Prepare pose, lighting and hook results once per logical preparation contract. Retain minimal draw records for opaque/translucent submissions. Do not retain raw references to scratch model poses that later entities overwrite.
3. Classify the actual subdraw/material, not the whole entity. Eyes, glint, armor, names, held items and mixed block entities can span phases. Preserve translucent order and texture/program compatibility.
4. Prevent the second submission phase from rerunning gameplay-affecting or geometry-mutating callbacks unless the documented hook explicitly promises per-pass behavior.
5. Advance render cooldown once at its intended frame/tick boundary and publish counters for prepared/visible/submitted entities consistently. Account for mod removal or mutation of an entity between preparation and submission.
6. Reuse bounded storage. Keep GL submission on the render thread and avoid pushing mutable world or Lua operations onto workers as an incidental optimization.

Acceptance: with separate entity draws, culling and pose preparation occur once per view while required opaque/translucent draws still occur twice at their proper boundaries. Compare draw records, hook counts and rendered behavior, including mod entities and block entities.

Evidence: [WorldRenderer.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/client/render/world/WorldRenderer.cpp:287>), [LivingEntityRenderer.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/client/render/entity/LivingEntityRenderer.cpp:130>), [entity performance tests](<C:/Users/Eddie/Documents/New project 2/tests/entity_render_perf_test.cpp>).

**G. P1 — Finish compiling the pack's runtime work**

Scope: existing `RuntimeOperation`/`WorldProgramRuntime`, compositor, resource/sampler binders and color targets. Size: medium/large; follows A/B.

1. Extend existing runtime records with resolved output indices, program-local sampler/image locations, unit assignments, blend/alpha directives, mipmap dependencies and viewport policy. Resolve symbolic names at pack activation/reload.
2. Use persistent indexed storage for current texture handles. Update handles at resize, reload and flip boundaries; do not copy three string-keyed maps for each composite stage.
3. Use spans/references to precompiled output lists. Remove per-pass output-vector copies and repeated target format/size parsing. Allocate/validate attachments on resource transitions, retaining cheap runtime validation where necessary.
4. Rebind only known active program resources, preserving custom-sampler precedence, aliases, hardware shadow samplers, image units and texture-unit limits. Keep state changes required by other intervening draws.
5. Cache FBO attachment combinations and directive results with explicit invalidation. Review error exits for framebuffer/viewport/blend-lock restoration.
6. Preserve compute barriers, indirect-command barriers, preflips, postflips and mipmap dependencies exactly. A compute-only pass does not acquire a raster flip merely because it writes a color image.

Acceptance: a warmed-up unchanged pack performs no compositor map construction or output-list heap allocation; active bindings and read/write target identities remain correct across every flip. Validate alias collisions, high color indices, integer targets and resized buffers. Iris documents buffer roles and flipping here: [ColorTex](https://shaders.properties/current/reference/buffers/colortex/), [flip directives](https://shaders.properties/current/reference/shadersproperties/rendering/#flip).

**H. P2 — Remove remaining main-thread traversal and streaming costs**

Scope: camera setup, `WorldRenderer`, section culling, chunk capture/upload and streaming submission. Size: medium; select subitems from the baseline profile.

1. Keep the existing single visible-section walk. Replace linear region-bucket searches with a stable region index or bounded per-view lookup, without changing translucent ordering.
2. Key cached draw lists by the state that actually changes them: visibility/camera, mesh upload/release, material/resource generation and relevant sort inputs. Existing frustum stamps alone must not hide same-frame mesh changes.
3. Keep the existing dirty set, occupancy gating and background mesh jobs. Measure capture locks and near/background upload budgets, including their combined frame cost. Use time and byte limits where necessary and preserve near-edit responsiveness.
4. Measure `bufferData` streaming before adopting persistent mapping or a ring buffer. If streaming stalls are material, use a bounded reuse strategy with explicit fence ownership and a supported fallback. Do not add a second backend preemptively.
5. Avoid repeating raycast/camera work for a secondary view when only the primary view should update player targeting. Preserve intended camera/FOV behavior.

Acceptance: no resident-world sweep when no sections are dirty; bounded upload latency under backlog; correct draw-list invalidation on eviction/upload; unchanged geometry/lighting output. Performance tests must exercise the production grouping and capture paths.

Evidence: [terrain grouping](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/client/render/world/WorldRenderer.cpp:167>), [ChunkCompilePipeline.cpp](<C:/Users/Eddie/Documents/New project 2/src/net/minecraft/client/render/world/ChunkCompilePipeline.cpp:110>).

**3. Existing work to preserve, and test gaps to close**

| Existing protection | What it establishes | Missing protection |
|---|---|---|
| `FramePipeline.*` | Outer drain/input/tick/render/present phase enumeration | Actual shader pass execution order |
| `ShadowFrameOrder.*` | Camera/matrix/history relationships | Begin versus shadow geometry and shadow-compute execution |
| `ShaderGlIntegrationTest.*` | Real GL coverage for bindings, depth snapshots, flips, nested states and async readback | Complete production snapshot/draw interaction and stage marker sequence |
| `FogDrawIntegrationTest.*` | Important fog upload and shader-class behavior | Non-fog overlapping uniforms and snapshot-generation transitions |
| `ModRenderScope.*`, `LuaRuntimePrelude.*`, `LuaSettingsModule.*` | Scope-layer behavior, API parsing and some execution/settings behavior | Real stage delivery, lifecycle mutation, callback counts and retained/nested event semantics |
| `EntityRenderPerf.*` | CPU renderer/dispatcher smoke measurements | Full world gather plus split-phase duplication; current generous limits are not a frame budget |
| `TerrainCullPerf.*` | Synthetic culling/grouping comparisons | Production single-walk region grouping and allocation counts |
| `ComputeDispatchParity.*`, pack/target tests | Workgroup math, loader/directive and target behavior | End-to-end execution of every supported compute family |

Preserve: one shaderpack path including embedded vanilla; cached world program resolution; per-program frame snapshots; cloud cell culling; cached sky/star meshes; single-walk terrain layer gathering; occupancy/lighting capture gates; asynchronous depth readback. None should be rewritten merely to make the architecture look new.

**4. Delivery order and completion gates**

| Batch | Deliverable | Depends on | Exit gate |
|---|---|---|---|
| 0 | Repro matrix, source recheck and baseline capture | None | Measured CPU/GPU allocation breakdown; active pack/mod settings recorded |
| 1 | A plus smallest B correctness fixes | 0 | Real execution/snapshot tests demonstrate and then resolve each discrepancy |
| 2 | C and E stage/payload contract | 1 | Current-frame atmosphere agreement; complete hooks; idle fast path |
| 3 | D cloud reuse and measured Lua geometry fixes | 2 | One cloud construction for two submissions; reload/camera tests pass |
| 4 | G compiled binding/resource work | 1 | Hot-path allocation/binding counts drop; pack parity tests hold |
| 5 | F entity preparation/submission split | 1, 2 | One preparation per view; mixed-material/mod semantics preserved |
| 6 | Profile-selected H work and remaining B matrix work | Earlier measured results | Measured improvement with correct sort/upload behavior |
| 7 | Remove obsolete paths and finish validation | All selected implementation work | One owner per state; superseded code deleted; no unused knobs or temporary diagnostics |

Batches describe dependencies and reviewable changes, not automatic branch/PR operations or intermediate build triggers. If implemented as one requested overhaul, finish all intended edits before the final build pass. If later requests deliberately scope smaller work, each request ends with its own final verification. Preserve other agents' edits; no Git operations are part of this plan.

Cleanup must delete displaced logic, stale fields and forwarding-only wrappers in the touched paths. Keep real resource owners and RAII scopes. Do not replace direct code with a generic render graph, event framework, singleton registry or facade stack. Add no source comments under the user's current rule; keep design rationale in this document.

For implementation validation, use `build-omega.ps1` only, preserve the build-cache type, and provide a targeted `-TestFilter`. The current Release configuration would use this form, selecting only suites affected by the finished change:

```powershell
.\build-omega.ps1 -BuildType Release -Target Client -RunTests -TestFilter "ShaderGlIntegrationTest.*:FogDrawIntegrationTest.*:ShadowFrameOrder.*:ShaderFrameData.*:DrawCameraState.*:PackBlendDrawBufferTest.*:ComputeDispatchParity.*:ModRenderScope.*:LuaRuntimePrelude.*"
```

Add the new execution/event tests and the applicable existing pack, celestial, entity, terrain, cancellation and lighting suites to the filter. Shared Lua changes also need server compatibility verification. Never use bare `-RunTests` or launch a competing build while another owner holds the build lock. Verify a successful finish and a fresh client executable report; a restored older binary is not validation.

Final acceptance requires: targeted tests actually executed with skips reported; deterministic state/pass fixtures; matching before/after performance captures; reload/resize/dimension/custom-camera coverage; bundled sky mods still working; and explicit reporting of unresolved pack-specific behavior. A screenshot or a successful shader compile alone does not establish correctness.

The best first implementation slice is **actual pass-order tests + begin/shadow-compute scheduling + uniform snapshot ownership**. It removes correctness ambiguity before performance refactors change the same code. The first performance slice is **idle Lua stage gating + resolved atmosphere reuse + fancy-cloud build reuse**, followed by compiled compositor bindings and the higher-risk entity split.

**Implementation status (2026-09-05)**

- Complete: world-render hook stage/moment buckets; resolved atmosphere inputs; fancy-cloud shared geometry upload; render-pass state invalidation cleanup; compiled composite output/flip metadata; `shadow.csh` discovery and dispatch.
- Complete: shadow target allocation precedes `begin`; `shadow.csh` runs before shadow target clear and geometry; shadow composites remain after the shadow draw.
- Remaining: precompute compositor sampler/image locations and attachment combinations; entity prepare/submission split; terrain region index and draw-list invalidation; Lua event payload inventory and contract cleanup; pack-specific parity for layered shadows, custom images, and multi-view rendering.
