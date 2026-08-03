# PARITY AUDIT — Iris Bindings + Matrix Projections vs Java Iris 26.1

Status: **review-only** (no source edits, no builds). Date: 2026-08-01.
Audience: plan master / executors of the multithreading/main-thread refactor.

> **READ FIRST — audit inputs that are missing or stale.**
> 1. The prior notes named in the brief (`matrix.md`, `uniforms.md`, `ssbo.md`,
>    `frameorder.md`, `dualpaths.md`, `lua-iris-dualpaths.md`, `deabstract.md`,
>    `runpasses-split.md`, `scopes.md`, and `CONTEXT.md`) **do not exist** in
>    `docs/agent-notes/` (glob + full-tree scan). This audit is therefore built from
>    primary sources only.
> 2. The brief's Java path `third_party/mcp/iris/uniforms/builtin/*.java` does not
>    exist. The 26.1 builtin uniforms live in
>    `third_party/mcp/iris/uniforms/{CameraUniforms,MatrixUniforms,ViewportUniforms,
>    CelestialUniforms,CommonUniforms,IrisExclusiveUniforms,FogUniforms,...}.java`;
>    `uniforms/builtin/BuiltinReplacementUniforms.java` is the DSA builtin-replacement
>    set, not the per-uniform matrix/camera/viewport classes.
> 3. The brief's "CompatibilityTransformer attribute injection" is wrong for this
>    tree: `CompatibilityTransformer.java` is the GLSL parse/cleanup transformer.
>    Terrain attribute encoding is in `SodiumTransformer.java` + bindings in
>    `ProgramCreator.java`/`SodiumPrograms.java`; entity attribute/overlay injection is
>    in `EntityPatcher.java`. `mc_Overlay`/`mc_LightMap`/`mc_normal` are NOT attributes
>    anywhere on either side in this tree.
> 4. The brief's CONTEXT.md SSBO claims ("no clearBufferSubData init, no dynamic
>    storage bit, index cap 8") are **stale vs the current tree**: the C++ now does
>    `clearBufferSubData` init (Resources.cpp:120,127), `GL_DYNAMIC_STORAGE_BIT`
>    (Resources.cpp:123), and caps indices at **13** (0..12, Resources.hpp:13),
>    matching Java ShaderProperties 0..12. Do not "fix" what is already in parity.

---

## 1. Verdict

**Mixed.** The bindings/matrix layer is largely and deliberately faithful to Iris
26.1, and several feared gaps are already closed (SSBO init/dynamic-bit/cap, colortex
startIndex=4, hemisphere-chunk-offset tracker, legacy sampler aliases, shadow
compare/HW-sampler split). **However the refactor must not assume lockstep where the
port knowingly deviates.** The deviations that matter to packs are:

- **fogMode uniform values** (C++ sends 0/1/2/3; Java Iris sends GL constants
  0/9729/2049) — HIGH, pack-visible.
- **Clip-space depth convention** (C++ classic −1..1; Java Iris 26.1 uses
  zZeroToOne 0..1 for gbufferProjection/shadowProjection) — HIGH for
  depth-reconstructing packs; internally self-consistent, so "works" only because the
  whole C++ chain uses −1..1.
- **gbuffer matrix provenance** (C++ *back-derives* the camera from the composed
  vanilla MatrixStack AFTER the world renders; Java *captures* the live
  RenderSystem modelview/projection at pass start) — MEDIUM, latent risk for any
  non-rigid/custom (Lua-mod) camera composition.
- **Entity-attribute path missing** (no `iris_overlay` sampler,
  `iris_Entity`/`iris_OverlayUV`/`iris_LightUV`; `entityColor` is uploaded as a plain
  uniform instead of overlay-derived) — MEDIUM/HIGH for entity-shader packs.
- **`far` uniform / projection far** (C++ `renderDistanceBlocks*2`; Java
  `renderDistance*16`) — MEDIUM.
- **First-frame gbufferPrevious\*** (C++ seeds = current; Java `Previous` starts at
  identity) — LOW.
- **mc_Entity encoding** (C++ ivec4 id/metadata + separate at_midBlock; Java-Sodium
  packed uint `((id+1)<<1)|mid`) — pack-visible `.x` (blockId) matches, format
  divergence in `.y` and midblock.

Everything else in the discrepancy table below is LOW/informational (internal slot
numbers, resource-lifecycle details, or both-sides-absent items like the fog plane).

The port has **no vanilla-vs-shaderpack dual path**: "shaders off" does not exist as a
mode — the bundled `shaders/vanilla` pack (a genuine Iris pack: the `gbuffers_*` world
programs + `final`, no composite/deferred/compute) is always the active pack when no
user pack is selected (`PackManager::activePack()` falls back to the base pack,
Manager.cpp:594-606). The pipeline is **unconditional**: `GameRenderer::beginSceneCapture`
always allocates the scene targets, and every frame runs the full stage sequence
BEGIN → shadow → shadowComposite → prepare → world → captureOpaqueDepth → hand →
captureHandDepth → deferred → post through the same shared runners. Which of those
stages actually draw is decided solely by the pack's own pass lists: empty lists
early-out *inside* the shared fullscreen-pass runner (CompositeRenderer.cpp:101-104),
never in an engine fork. The old `activeHasPostProcess()` escape hatch — which dropped
a pack out of the pipeline entirely when its post stage lists happened to be empty
(`final` counted as a post pass, so a gbuffer+final pack like vanilla was already
inside; a gbuffer-only pack was not) — and the `captureWorldDepth` parameter that
gated the stages from inside the world render have both been deleted; every pack,
vanilla included, presents through the FBO path (final program or colortex0 blit).
The real remaining forks are (a) world-vs-GUI matrix producers, (b) frame-uniform
camera reconstruction vs live per-draw MatrixStack tops, (c) world vs shadow cameras,
and (d) the Lua-mod world-mesh path calling main-thread-only GL state setters from mesh
workers.

---

## 2. Discrepancy table

Severity: HIGH = third-party pack visibly broken; MEDIUM = wrong only in corner cases
or specific packs; LOW = harmless/internal/one-frame.

### 2.1 Matrix parity

| area | C++ file:line | Java Iris file:line | difference | severity | parity impact |
|---|---|---|---|---|---|
| gbufferModelView / gbufferProjection source | GameRenderer.cpp:1029-1054 (back-derive FrameRenderCamera from composed MatrixStack after renderWorld), FrameData.cpp:280-285 (`buildCameraProjection/buildCameraModelView` from that camera) | CapturedRenderingState.java:32-46 (`setGbufferModelView`/`setGbufferProjection`, captured live by the pipeline/mixin at pass start) | C++ reconstructs the matrices from a *recovered* rotation + two projection scale factors; Java uses the *actual* RenderSystem matrices. Recovery is exact only for rigid `R·T(−eye)` compositions with a canonical perspective. Any non-rigid composition (view-bob-with-offset, damage tilt, Lua `cameraSetup` poses, custom fov offsets) silently changes gbuffer matrices vs Java. | MEDIUM | gbufferModelView/Projection are pack-critical (depth unproject, TAA, motion vectors). Fails only for exotic camera setups today; the refactor must not make reconstruction a "parallel" step unless it is proven identical. |
| Clip-space depth range (gbuffer + shadow projections) | FrameRenderCamera.hpp:145-191 (`buildCameraProjection`: m[10]=-(f+n)/(f−n), m[11]=−1, m[14]=−2fn/(f−n) — classic GL −1..1); Matrix4f.hpp:101-110 (`perspective` m[15]=0) | ShadowMatrices.java:23,36-38 (`setOrthoSymmetric(... isZZeroToOne())`, `createPerspectiveMatrix` 0..1); Java MC 1.17+ renders with zZeroToOne | C++ uses classic [−1,1] clip; Java Iris 26.1 uses [0,1]. Internally the C++ chain is self-consistent (its own shader packs, depthtex reads, depth reconstruction). Packs ported verbatim from Java Iris that reconstruct view-space z with the [0,1] formula get different constants. | HIGH (pack-compat) | gbufferProjection[2][2], shadowProjection, and depth-inverse maths. Port is deliberately classic-GL; keep it consistent, but know it is not Iris-numeric-identical. |
| `far` uniform + projection far | GameRenderer.cpp:1061-1063 (farPlane = perspectiveFar>near ? perspectiveFar : `frameSettings_.renderDistanceBlocks * 2.0f`); FrameData.cpp:227 | CameraUniforms.java:27,37-40 (`far = getEffectiveRenderDistance() * 16`) | C++ far ≈ 2× the Java `far` value for the same render distance. | MEDIUM | `far` used by packs for depth scaling/fog. May match b1.7.3 vanilla far instead of Iris; verify against the shipped packs. |
| gbufferProjection reconstructed from 2 scalars | FrameRenderCamera.hpp:159-168 (rebuilds P from projectionX/Y + near/far) | CapturedRenderingState.java:44-46 (stores full captured projection) | Only x/y scale + near/far survive; a projection with a translation or non-canonical form is lost. Vanilla b1.7.3 perspective is canonical, so OK today. | LOW-MEDIUM | Same class of risk as the provenance row; guard if a custom projection is ever injected. |
| gbufferPreviousProjection/ModelView first frame | FrameData.cpp:660-665 (`if(firstFrame)` copies **current** into previous*) | MatrixUniforms.java:66-85 (`Previous` field = identity; first get() returns identity) | First frame: Java previous = identity; C++ previous = current. | LOW | One frame at boot; motion-vector/TAA packs may see a jump. |
| gbufferPrevious* steady state | FrameData.cpp:200-201,284-285,666 | MatrixUniforms.java:66-85 | Order matches (old current → previous, then store new). | — | PARITY |
| Hemisphere chunk offset (cameraPosition rounding) | FrameRenderCamera.hpp:198-244 (`CameraPositionTracker`, WALK 30000 / TP 1000, shift = −(v−fmod(v,30000))) | CameraUniforms.java:62-143 (same constants/algebra) | 1:1. Guarded by `tests/iris_hemisphere_chunk_offset_test.cpp`. | — | PARITY |
| cameraPosition / cameraPositionInt/Fract | FrameData.cpp:266-279 (`cameraTracker.current/currentUnshifted`) | CameraUniforms.java:28-34 | Shifted vs unshifted split matches (cameraPosition=shifted, Int/Fract=unshifted). | — | PARITY |
| View bob: geometry origin vs model view | RenderCore.cpp:234-294 (`setDrawCameraStateFromCamera` publishes `cleanEye*` as the draw origin and hands the bob to the section-local base as a view-space translation), FrameRenderCamera.hpp (`buildCameraModelView` = rotation + `R·(cleanEye − eye)`; `buildCameraModelViewInverse` = `R^T` + `eye − cleanEye`), GameRenderer.cpp:968-992 (`cleanEye*` recovered from `applyCameraTransform` alone) | GameRenderer.renderLevel (bobHurt/bobView pushed onto the pose stack), CapturedRenderingState.java:32-46 (`setGbufferModelView` captures it), CameraUniforms.java:28-34 (`cameraPosition` = `Camera.getPosition()`, never bobbed) | Match in structure: the bob lives in gbufferModelView, the geometry origin is the clean camera position. chunkOffset, `cameraPosition` and the shadow map centre (ShadowMapPass.cpp:271) are all on that one origin, so `gbufferModelViewInverse * viewPos + cameraPosition == worldPos` and `shadowIntervalSize` snapping actually holds the map still. Guarded by `tests/draw_camera_state_test.cpp` (`ViewBobStaysInTheMatrixNotTheOrigin`). Note this port recovers a rigid rotation from the view basis, so a non-rigid pose-stack term (the portal distortion scale) is still lost — see the reconstruction row above. | — | PARITY (structure); reconstruction caveat unchanged |
| eyePosition / relativeEyePosition | FrameData.cpp:455-458 (raw unshifted eye), 491-494 (unshifted − eye) | IrisExclusiveUniforms.java:79-81,273-277 | Match. | — | PARITY |
| sunPosition/moonPosition/shadowLightPosition | FrameData.cpp:308-322 (world sun dir + RZ(sunPathRotation), directionToView, scale 100), 350-357 (shadowLightPosition day/night/end-flash) | CelestialUniforms.java:30-48,94-135,164-179 (gbufferModelView·RY(−90)·RZ(sunPathRot)·RX(angle)·(0,100,0)) | The C++ claims the beta celestial frame equals the Java chain minus the RY(−90) renderSky prefix (comment FrameData.cpp:170-174). If the light-registry `sun.direction` already sits in that rotated frame the results match; otherwise a constant frame offset creeps in. Guarded by `tests/shadow_celestial_modelview_test.cpp`. | MEDIUM (verify) | sun/moon angle placement in world+shadow passes. Re-verify when the sun registry or camera model changes. |
| upPosition | FrameData.cpp:286 (`directionToView(0,1,0, camera)`) | CelestialUniforms.java:50-64 (gbufferModelView·RY(−90)·(0,100,0)) | C++ skips the RY(−90) (uses the beta frame); see celestial row. | MEDIUM (verify) | Pack "up" vector. |
| shadowModelView / shadowProjection | ShadowMapPass.cpp:284-316 (shadowCam built from `buildShadowCelestialModelView`, FrameRenderCamera.hpp:62-88: RX(90)·RZ(angle·−360)·RX(sunPathRot) + snap translate), FrameData.cpp:48-60,287-298 | ShadowMatrices.java:42-69 (createBaselineModelViewMatrix) + 71-100 (snapModelViewToGrid), MatrixUniforms.java:24-31, 29-31 (ortho) | Explicit modelview matches (hasExplicitModelView=true, ShadowMapPass.cpp:308). Ortho half-plane & snap match. **Depth-range convention differs** (see clip-space row). | MEDIUM | Shadow mapping. Guarded by `tests/shadow_celestial_modelview_test.cpp`. |
| shadowPerspective far | ShadowMapPass.cpp:311-316 (`perspectiveFar = 156.0f`) | ShadowMatrices.java:26-40 (`FAR = 156.0f`, constants NEAR −100.05 / FAR 156, ShadowMatrices.java:18-19) | Match (156). | — | PARITY |
| `near` uniform | FrameData.cpp:226 (`camera.perspectiveNear`, default 0.05) | CameraUniforms.java:26 (ONCE 0.05) | Match for default camera. | — | PARITY |
| GUI ortho path vs world path | GuiProjection.hpp:11-30 (`gui_proj::load` publishes ortho + z=−2000 translate via `core::setDrawCameraState`); GameRenderer.cpp:1064 (`setDrawCameraStateFromCamera`) | Iris renders GUI through the pipeline's gbuffer_gui programs with the pipeline camera | Two producers write the same draw-camera globals (RenderCore.cpp:256-303). GUI never renders into the pack FBOs; phase is forced to `None` (GameRenderer.cpp:668-669,689). Matrix *sources* are disjoint (ortho GUI vs perspective world). | LOW (informational) | Refactor must keep both producers on the main GL thread and preserve the publish/restore order (`ScopedDrawCameraState`, RenderCore.cpp:370-386). |
| fogMode value contract | FrameData.cpp:237 (`fog.enabled ? fog.mode : 0`), RenderCore.cpp:464 (per-draw same); fog.mode ∈ {1 linear,2 exp,3 exp2} (RenderCore.hpp:60-61) | FogUniforms.java:20-38 (OFF→0; else GL_LINEAR **0x2601=9729** when density<0, GL_EXP2 **0x0801=2049** otherwise); `fogShape` OFF→−1 else 1 (FogUniforms.java:23,37) | C++ sends the *internal* 1/2/3; Java Iris sends the **GL constants** 9729/2049. C++ fogShape (0/-1? FrameData.cpp:239 sends 1/−1) matches. The port's own vanilla pack interprets 1/2/3 (shaders/vanilla/shaders/lib/common.glsl:18-26), and no GLSL prelude redefines GL_LINEAR/GL_EXP/GL_EXP2 (SourceProcessor.cpp:735-775 injects no fog constants), so third-party packs that compare `fogMode == GL_LINEAR` etc. see a different contract than Java Iris. | HIGH | Pack fog branching. Either align values with Java Iris or document the deviation as a first-class port contract and rewrite the vanilla-pack comment (it currently *claims* "as Iris reports them", which is false numerically). |
| fogPlaneHeight / fogPlanePosition | **absent on C++ side** | **absent on Java side** (no match in third_party/mcp/iris) | Both omit OptiFine's fog-plane uniforms. | — | PARITY (both absent). Brief's assumption that this is a live parity target is wrong. |

### 2.2 Attribute / IdMap parity

| area | C++ file:line | Java Iris file:line | difference | severity | parity impact |
|---|---|---|---|---|---|
| Attribute location binding | ShaderProgram.cpp:146-156 (vaPosition 0, vaUV0 1, vaColor 2, vaNormal 3, at_midBlock 4, vaUV2 5, **mc_Entity 6, mc_midTexCoord 7**, at_tangent 11, mc_chunkFade 12); VAO layout RenderCore.cpp:839-877 | ProgramCreator.java:20-22 + SodiumPrograms.java:165-167 (**mc_Entity 11, mc_midTexCoord 12, at_tangent 13**) | Slot numbers differ. Internal (packs bind by name via glGetAttribLocation), so no pack-visible break **provided** the VAO and bindAttribLocation stay in sync — they do today. | LOW (internal) | Do not "align" slot numbers casually; they are the port's own layout. |
| mc_Entity semantics | RenderCore.cpp:869-870 ivec4 at kOffEntity=36 (blockId .x, metadata .y per ModModels.hpp:177); separate at_midBlock slot 4 (RenderCore.cpp:863-864, kOffMidBlock=28) | SodiumTransformer.java:283-304 (packed uint `((id+1)<<1)|mid`, shader decodes `int(mc_Entity>>1)−1`, midblock `&1u`), SodiumPrograms.java:165 | Pack-visible `mc_Entity.x` = blockId in both. C++ adds `.y`=metadata and splits midblock into a different attribute; Java packs using the packed-uint `.y`/`.z`/`.w` or expecting `&1` midblock get different data. | MEDIUM | Packs that use `mc_Entity.y` (metadata) work on C++ only; packs ported from Sodium expecting packed-uint decode differ. |
| mc_midTexCoord | RenderCore.cpp:871-872 vec2 float at slot 7 (raw atlas UV) | SodiumTransformer.java:317-360 (uvec2 slot 12; `iris_MidTex` scaled by textureScale) | C++ is already-scaled float2; Java scales raw coords by texture scale. If the C++ atlas UVs are final atlas coordinates the `.xy` contract matches; the vector dimension and scale chain must be re-verified per pack. | MEDIUM (verify) | Wave/foliage mid-tex coord usage. |
| at_tangent | RenderCore.cpp:873-874 slot 11, 4×GL_INT normalized (kOffTangent=52) | ProgramCreator.java:22 slot 13 | Slot differs (internal); name matches. | LOW | — |
| mc_normal / mc_Overlay / mc_LightMap | **absent** | **absent** (these are not attributes in 26.1's vanilla path; overlay/light use `iris_overlay` sampler + `iris_OverlayUV`/`iris_LightUV` uniform/input, ShaderCreator.java:118-134, EntityPatcher.java:45-56) | Both absent → no pack attribute contract to break. Brief's assumption is wrong for this tree. | — | PARITY |
| entity attribute path | No `iris_Entity`/`iris_overlay`/`iris_OverlayUV`/`iris_LightUV`. C++ uploads `entityId`/`blockEntityId`/`currentRenderedItemId` as plain uniforms (RenderCore.cpp:528-539, Uniforms.cpp:168-169) and `entityColor` as a uniform (RenderCore.cpp:453-458) | EntityPatcher.java:124-204 (vertex `iris_Entity` ivec3 → geometry/fragment passthrough) and :33-122 (overlay → `entityColor` computed from `texelFetch(iris_overlay, …)`, uniform `entityColor` deleted) | Uniform-based packs work on C++ (entityId/blockEntityId/currentRenderedItemId/entityColor all present). Attribute/overlay-derived packs (and `entityColor` semantics = overlay-tint vs engine-set uniform) differ. | MEDIUM-HIGH | Entity shaders; `entityColor` values differ (C++ engine-set vs Iris overlay-derived). Verify with a pack that reads entityColor in fragment. |
| IdMap default / fallback | Manager.cpp:75-91 (`resolveShaderObjectId`: lowercase key, then "minecraft:"+key, else `fallback` = −1); Pipeline.cpp:215-241 (`applyBlockIds`: identity fallback = vanilla numeric id 0..255); FrameData.cpp:438-442,512-513, Pipeline.cpp:346 (held item −1) | IdMap.java:149-206 (parse defaultReturnValue **−1**); IdMapUniforms.java:60-64 (`invalidate()` → intID −1, light 0), :66-103 | −1 defaults match for item/entity/block-by-name. The block-state fallback differs by design: C++ maps unmapped block ids to the legacy numeric id (identity), Java maps through modern block-state registry. In the port's 256-id space identity fallback is the sane choice. | LOW | mc_Entity.x for unmapped blocks. |
| IdMap key normalization | Manager.cpp:83 (`lower`), Pipeline.cpp:226-237 (strips `tile.`, tries bare then `minecraft:`), Loader.cpp:1578-1583 (parse `entity.`/`item.`/`block.` prefixes) | IdMap.java:160-206 (keyed `item.`/`entity.` prefix; NamespacedId), LegacyIdMap.java (legacy block ids) | Roughly matches the OptiFine namespaced-id contract. No state-property (`minecraft:stone[prop=x]`) support in C++ (Loader strips/ignores) — Java logs-and-skips too (IdMap.java:188-192). | LOW | — |
| 256-entry block id array | Pipeline.cpp:216-217 (`std::array<int,256>`) | Java block-state registry (modern, much larger) | Inherent to the b1.7.3 id space; atomic publish is fine (`g_shaderBlockIds` is `std::atomic_int[256]`, RenderType.cpp:14,29-36; readers: BlockRenderManager.cpp:145, ModModels.cpp:725). | — | PARITY-in-port |

### 2.3 Sampler binding parity

| area | C++ file:line | Java Iris file:line | difference | severity | parity impact |
|---|---|---|---|---|---|
| colortex index start for world/shadow vs fullscreen | ColorTargets.cpp:714-717 + RenderTargets.hpp:90-91 (`renderTargetSamplerStartIndex` = fullscreen ? 0 : 4); fillReadSamplers ColorTargets.cpp:595-608 | IrisSamplers.java:51-111 (`startIndex = isFullscreenPass ? 0 : 4`, :55) | Match. Note this is the **colortex index** restriction, not texture-unit numbering (the brief's "startIndex=4 for gbuffers" is correct as a colortex-slot rule). | — | PARITY |
| colortex slot cap | RenderTargets.hpp:42 (`kMaxColortex = 32`), sceneColorFormats clamp 1..32 (Pipeline.cpp:494-517) | Pack-defined render targets (Iris supports up to MAX_COLOR_BUFFERS=16/GL limit) | C++ caps at 32; Java caps at pack directive count ≤ driver limit. Fine for real packs. | LOW | Packs requesting >32 buffers unsupported (none do). |
| sampler texture-unit numbering | WorldProgramBinder.cpp:67-156 (lightmap unit 1, then normals/specular/noisetex/custom, then shadow at the next free unit; `set1i` per name) | IrisSamplers.java:36-37 (`WORLD_RESERVED_TEXTURE_UNITS={0,1,2}`: albedo 0, overlay 1, lightmap 2), ProgramSamplers.java:108-122 (dynamic units start at 3) | Unit numbers differ (C++ lightmap on 1, dynamic from 2; Java dynamic from 3). Packs see the unit through the sampler uniform (`set1i`), so it is self-consistent and not pack-visible. The **overlay reserved unit is missing on C++** (no iris_overlay). | LOW (units) / MEDIUM (overlay absent) | No pack-visible break from numbering; overlay row belongs to the entity-attribute gap above. |
| shadowtex0/1, shadowcolor0-7, shadowtex0HW/1HW | WorldProgramBinder.cpp:105-147 (compare-mode sampler objects via `samplerObject(compare)`, separate-HW branch :119-141, shadowcolor loop :142-147) | IrisSamplers.java:138-187 (addShadowSamplers incl. watershadow/shadow aliases, shadowtex0HW/1HW when separateHardwareSamplers) | Binding set matches; the port adds `watershadow`/`shadow` alias handling in refreshTextureAliases (GlState.cpp:216-223). Depth-compare texture-level state set only when `!SEPARATE_HARDWARE_SAMPLERS` (ShadowMapPass.cpp:245-246) matching `configureDepthSampler`. | LOW | PARITY (verify alias names per pack). |
| shadowcolor flip-state supplier | ShadowMapPass.cpp:333-338 (result.colorTextures[0], colorAltTextures[1]); composite attachment picks main/alt by flipped snapshot (ShadowMapPass.cpp:129-147) | IrisSamplers.java:155-176 (flipped ? alt : main for shadowcolor); RenderTargets flip machinery | Matches the main/alt flip convention. | — | PARITY (lockstep invariant, §3). |
| depthtex0/1/2 | CompositeRenderer.cpp:196-203, FinalPassRenderer.cpp:89, Pipeline.cpp:648-655 (pre-hand→depthtex2, pre-translucent→depthtex1) | IrisSamplers.java:220-235 (depthtex0/1/2, gdepthtex alias) | Match. | — | PARITY |
| legacy aliases gcolor/gdepth/gnormal/composite/gaux1-4 | Loader.cpp:583-588,741 (alias→colortex0-7), GlState.cpp:216-223 (refreshTextureAliases incl. watershadow/gdepthtex) | PackRenderTargetDirectives.LEGACY_RENDER_TARGETS (used in IrisSamplers.java:86-98) | Match (gcolor=0, gdepth=1, gnormal=2, composite=3). | — | PARITY |
| lightmap | Pipeline.cpp:449-492 (lightmapTexture_ bound on unit 1, 16×16 RGBA8 luminance+brightness gamma) | CommonUniforms.java:202-206 + iris_external sampler at LIGHTMAP unit 2 | Generation differs in content (C++ builds a legacy 16×16 map; Java uses vanilla LightTexture). Same sampler name/role. | LOW | Lightmap texture contents are a separate (known) port divergence, not a binding one. |
| noisetex + custom textures + custom images | WorldProgramBinder.cpp:89-104 (noisetex, customTextures by stage), Resources.cpp:44-69 (noise gen), 150-269 (custom textures), PackResources::bind Resources.cpp:324-345 (images via bindImageTexture) | IrisSamplers.java:113-115 (noisetex), 237-247 (custom textures/images), CustomTextureManager.java | Match. | — | PARITY |
| normals/specular (texture_normal/texture_specular) | WorldProgramBinder.cpp:86-87 (bindOptional "normals"/"specular"), Manager.cpp:109-131 (PBR holder resolution per draw) | IrisSamplers.java:214-217 (addLevelSamplers normals/specular) | Match. | — | PARITY |

### 2.4 SSBO / image / buffer parity

| area | C++ file:line | Java Iris file:line | difference | severity | parity impact |
|---|---|---|---|---|---|
| SSBO index cap | Resources.hpp:13 (`kMaxShaderStorageBuffers = 13`, indices 0..12); Loader.cpp:1065,1125 enforce it | ShaderProperties 0..12 ("can't use buffer numbers higher than 12"); ShaderStorageBufferHolder.java:26 sizes array to max index+1 | Match. Brief's "index cap 8" is stale. | — | PARITY |
| SSBO init / clearBufferSubData | Resources.cpp:95-128 (content path read, bufferStorage, then `bufferSubData` if init content else `clearBufferSubData(R8, RED/BYTE, 0)`; relative path also clearBufferSubData :120) | ShaderStorageBuffer.java:73-82 (`createStatic`: bufferStorage + bufferSubData or clearBufferSubData R8), :52-67 (`resizeIfRelative`: bufferStorage + clearBufferSubData) | Match, incl. the R8/RED/BYTE zero-clear and the relative-screen-size recompute (`(w*scaleX)*(h*scaleY)*size`). Brief's "no clearBufferSubData init" is stale. | — | PARITY |
| GL_DYNAMIC_STORAGE_BIT | Resources.cpp:123 (`bufferStorage(…, initData.empty() ? 0 : 0x0100)`) | ShaderStorageBuffer.java:75 (`content==null ? 0 : GL_DYNAMIC_STORAGE_BIT`) | Match. Brief's "no dynamic storage bit" is stale. | — | PARITY |
| VRAM guard | Resources.cpp:28-38,70-78 (`vramBytes()` NVX else 4 GiB; abort if size>vram) | ShaderStorageBufferHolder.java:28-30 (OutOfVideoMemoryError), IrisRenderSystem.java:464-470 | Match. | — | PARITY |
| bindBufferBase per program | Resources.cpp:324-332 (bind per program in PackResources::bind) | ShaderStorageBuffer.java:48-50 + ShaderStorageBufferHolder.setupBuffers():78-83 | Match (Java binds all each setup; C++ binds declared ones per program). | — | PARITY |
| image binding | Resources.cpp:333-344 (unit from imageUnitStart=0, cap 16, bindImageTexture layered if depth>1, access READ_WRITE 0x88BA) | IrisRenderSystem.java:279-286 (bindImageTexture; ImageLimits), ShaderStorageBufferHolder-independent ImageHolder | Match; image-unit space is separate from texture units so unit 0 start is safe. | — | PARITY |
| SSBO GPU lifecycle | Resources.cpp:109-136 (gen→storage→bindBase→unbind target; `pack.bufferBytes` size change triggers regen) | ShaderStorageBufferHolder.java:36-46, ShaderStorageBuffer destroy :41-46 | Match. | — | PARITY |
| bufferBlends | RenderCore.hpp:98-106 + RenderCore.cpp:1055-1102 (`lockBufferBlend` → blendFuncSeparatei / blendFunci), Manager.cpp:55-74 (per-resolved-program apply) | BlendModeStorage / BufferBlendOverride (IrisRenderSystem.java:334-353 enable/disableBufferBlend + blendFuncSeparatei) | Match in spirit. Verify `drawBufferIndex` mapping (pack `buffer` index → glDrawBuffers index) matches Java's `Ints.indexOf(drawBuffers, info.index())` (ShaderCreator.java:143-148). | LOW (verify) | Per-buffer blend on multi-colortex passes. |

---

## 3. Dual-path inventory (every fork the refactor must either unify or keep provably in lockstep)

There is **no** vanilla-vs-shaderpack pipeline fork: the "off" state is the bundled
`shaders/vanilla` pack through the same machinery, and the pipeline runs
unconditionally for every pack — a pack can no longer be dropped out of the
FBO/uniform path (`activeHasPostProcess` and the `captureWorldDepth` gate were
deleted; empty pass lists early-out inside the shared runner). The real forks:

| # | Fork (A) | Fork (B) | file:line (both) | Refactor action |
|---|---|---|---|---|
| D1 | **World perspective draw-camera** producer: `setDrawCameraStateFromCamera` (GameRenderer.cpp:1064, RenderCore.cpp:304-340) | **GUI ortho draw-camera** producer: `gui_proj::load` (GuiProjection.hpp:11-30) → same `core::setDrawCameraState` (RenderCore.cpp:256-303) | GameRenderer.cpp:1064 vs GuiProjection.hpp:11-30; restore via ScopedDrawCameraState RenderCore.cpp:370-386 | **Keep in lockstep** — both write the same globals on the main GL thread; GUI phase forced to `None` (GameRenderer.cpp:668-669,689). If matrices move to a worker, both producers and the restore scope move together and stay ordered. |
| D2 | **Pack frame-matrix source** = back-derived FrameRenderCamera → gbuffer/shadow matrices (FrameData.cpp:266-298, from GameRenderer.cpp:1029-1054) | **Per-draw vanilla matrices** = live `MatrixStack` tops on each RenderPass (RenderCore.cpp:407-419, section-local rotation-only + chunkOffset) | FrameData.cpp:280-285 vs RenderCore.cpp:406-419 | **Keep in lockstep** — they are *intended* to agree (rotation-only gbuffer convention + chunkOffset). Any refactor that computes gbuffer matrices on a worker must prove bit-identical output for the rigid-camera case (guarded by `tests/shader_frame_data_test.cpp`, `tests/draw_camera_state_test.cpp`). |
| D3 | **World camera** (frameCamera_, GameRenderer.cpp:988-1064, custom Lua camera hook :1001-1004) | **Shadow camera** (shadowCam explicit modelview, ShadowMapPass.cpp:284-316) → shadow matrices (FrameData.cpp:287-298) | ShadowMapPass.cpp:284-316 vs GameRenderer.cpp:991-1004 | **Keep in lockstep** — shadow matrices are computed in FrameData from the shadow camera produced earlier the same frame; ordering (BEGIN reads last frame's shadow → renderShadows → shadow composite → prepare) is a hard invariant (plan §2.3). |
| D4 | **Shaderpack program path** via `bindWorldProgram` (Manager.cpp:92-135 → WorldProgramBinder.cpp) — pack frame uniforms + samplers + custom uniforms | **Vanilla per-draw uniform path** `bindAndUploadUniforms` (RenderCore.cpp:387-547) — fog/sun/light/entity uniforms | Manager.cpp:92-135 vs RenderCore.cpp:387-547; both run per draw (WorldProgramBinder.cpp:57-62 calls uploadShaderUniforms + pack custom; RenderCore uploads the vanilla set) | **Keep in lockstep** — two uploaders run in sequence on the main GL thread; `g_programUniformGeneration`/`g_passUniformsUploaded` dirty-tracking (RenderCore.cpp:93-118,503-527) must stay main-thread. |
| D5 | **Interface/base pack programs** (basePack_ = shaders/vanilla) when `interfaceProgramsActive()` | **Active pack programs** | Manager.cpp:51,96-101 (pack selection in the uniform uploader), PackManager::activePack fallback Manager.cpp:470-487 | **Keep in lockstep** — the selection is per-draw (GUI/interface vs world) and reads `WorldPipelinePhase`; must remain main-thread and per-draw-ordered. |
| D6 | **Lua-mod world mesh draw** (`drawLuaBlockWorld` → `core::setAlphaTestRef(0.1f)`) runs inside `ChunkBuilder::buildMesh` on **Compute workers** (ModModels.cpp:616-629,626; registered LuaBlockRegistry.cpp:251; invoked from BlockRenderManager.cpp:156) | **Main-thread GL-state path** that consumes `g_alphaTestRef` for uniform upload (RenderCore.cpp:70,479-481,671-677) | ModModels.cpp:626 (worker write) vs RenderCore.cpp:479-481 (main read) | **Unify (mandatory)** — this is the HZ-14 data race the plan's WI-5 already targets: capture alpha ref into the mesh snapshot; never write `g_alphaTestRef` from workers. Not merely "keep in lockstep" — it is UB today. |
| D7 | **No-pack / "off" state** = bundled vanilla pack fallback (Manager.cpp:336-344,594-606) | **Active pack** (Manager.cpp:351-370) | Manager.cpp:335-373 | **Unified (done)** — both go through the same pipeline; the no-post-process escape hatch (`activeHasPostProcess` → skip scene capture) and the `captureWorldDepth` gate were deleted, so no "no-shader" code path can bypass FBO/uniform parity. |

The brief's `dualpaths.md`/`lua-iris-dualpaths.md` do not exist; the above table is the
authoritative current-tree inventory. D6 is the only fork that is an outright bug
rather than a deliberate parallel path.

---

## 4. Parallelization hazards (what can/cannot move to a worker)

### 4.1 MUST stay main-thread (serial-history / GL / shared-mutable state)

| hazard | evidence | why it cannot move |
|---|---|---|
| `buildShaderFrameData` carries ~25 function-static accumulators + 2 file-static globals | FrameData.cpp:32-33 (`g_centerDepthSmooth`, `g_wetnessSmooth`), :191-196 (previousFrame/currentFrame/initialized/frameCounter/frameTimeCounterAccumulator/previousFrameTime), :266 (static `cameraTracker`), :538-548 (static smoothBlock/smoothSky), :589-604 (~19 `static SmoothedState`) | Not re-entrant; every call mutates shared static state and depends on the *previous* frame's value (previousFrame matrices, EMA accumulators). Moving it to a worker either introduces a data race or adds one-frame latency that breaks Iris's "previous = immediately preceding frame" contract (motion vectors, TAA). Deterministic per input+history, but the history is global. **Cannot parallelize across frames; must stay on the render phase of the main thread.** |
| `updateCenterDepthSmooth` / `updateWetnessSmooth` | FrameData.cpp:32-33,670-692; callers Pipeline.cpp:429-433,632-646 | File-static EMAs, read back in the same frame's uniform snapshot (FrameData.cpp:361,658). Same serial-history argument. |
| RenderCore GL-state + dirty-cache globals | RenderCore.cpp:70,97-131,965 (g_alphaTestRef, g_uploaded*, g_gl cache, g_drawModelView/Projection, g_entityId/blockEntityId/renderedItemId/renderStage, g_attribCache, g_textureUnitOf, g_allocatedTextures) | Every OpenGL call and every per-draw uniform upload; plan §2.1 hard invariant. The dirty-cache elision logic (memcmp of last-uploaded values) is order/thread sensitive — two threads uploading would both see "unchanged" and skip. |
| `RenderCameraState::instance()` process-wide singleton | FrameRenderCamera.hpp:248-262; written GameRenderer.cpp:1060, read Manager.cpp:106, world renderers, Lua bindings | A single mutable `frame_`; parallel renderers would need per-thread/per-frame copies. |
| `Pipeline::worldUniforms_` | Pipeline.cpp:427 (written in setFrameUniforms), read by the per-draw uploader Manager.cpp:92-135 (`ctx.uniforms = &pipeline_.worldUniforms()`) | Plain struct member, no synchronization. Any worker reading it while setFrameUniforms writes → data race. |
| `PackDefinition` id maps during dimension switch | Pipeline.cpp:293-296 (mutates blockIds/itemIds/entityIds), read by resolveShaderObjectId Manager.cpp:75-91 (called on main: FrameData.cpp:438,513; EntityRenderDispatcher.cpp:178; BlockEntityRenderDispatcher.cpp:58; ItemModelRenderer.cpp:63) | Workers reading the maps while a dimension switch rewrites them → torn maps. Today all readers are main-thread; the refactor must keep it that way or make the maps immutable-per-frame. |
| SSBO/image/custom-texture GPU creation | Resources.cpp:40-139,150-269,324-345 (PackResources::ensure/bind), ColorTargets.cpp:127-152,307-338 | GL object creation/binding must stay on the GL thread (plan §2.1/2.2). |
| ColorTargets flip state (`slot.main`) | ColorTargets.cpp:667-679,718-741 | Main-thread-only; BufferFlipper stage semantics are a hard invariant (plan §2.3). |
| `ShadowMapState`/`frameShadow_` | ShadowMapPass.cpp:183-339, GameRenderer.cpp:1122-1126 | Per-frame shadow targets + BEGIN-reads-last-frame-shadow ordering. |
| `g_shaderBlockIds` (already atomic) | RenderType.cpp:14,29-36; readers BlockRenderManager.cpp:145, ModModels.cpp:725 | Safe across threads **today** (relaxed atomic); if mesh workers read it, keep it atomic — do not revert to a plain array. |

### 4.2 Static/global mutable state in the upload path that races or is thread-unsafe

| symbol | location | status |
|---|---|---|
| `g_alphaTestRef` | RenderCore.cpp:70; write ModModels.cpp:626 (worker) via BlockRenderManager.cpp:156 → drawBlockWorld → drawLuaBlockWorld; read RenderCore.cpp:479-481,680 | **Live race (HZ-14)**. The single real binding-path UB. WI-5's snapshot fix is mandatory before any mesh path is further parallelized. |
| `g_programUniformGeneration` / `g_globalsGeneration` / `g_passUniformsUploaded` | RenderCore.cpp:93-95,503-527,606-612,818-821 | Main-thread-only generation counters; any second uploader thread corrupts the elision. |
| `g_textureUnitOf` / `g_allocatedTextures` | RenderCore.cpp:966-968,1288-1351 | Guarded by `g_textureMutex` for allocation only; bind-cache map is unsynchronized and main-only. |
| `g_drawModelView/Projection/…` + `g_drawCameraValid` | RenderCore.cpp:97-110,256-355 | Main-only draw state; workers must not publish poses. |
| `previousFrame/currentFrame` + EMA statics | FrameData.cpp:191-196,538-604 | Function-local statics; not even call-safe from two threads. |

### 4.3 What *could* safely move to a worker (with effort)

- Pure value reads already passed in by value (the `PackUniformValues` struct is a
  plain value type — copying it is safe; it is the *statics inside the builder* that
  bind it to the main thread).
- `applyBlockIds` build (`std::array<int,256>`) could be computed off-main from an
  immutable snapshot of the PackDefinition maps, but the write must stay
  main-thread/atomic (it already publishes via atomics).
- IdMap lookup itself (`resolveShaderObjectId`) could run on workers **only if** the
  `PackDefinition` maps are made immutable per frame (copy-on-switch); today they are
  mutable (Pipeline.cpp:293-296).

Bottom line: **the frame-data/matrix/upload computation is the least movable code in
the render path**; the refactor should treat the whole of §4.1 as main-thread-by-fiat
and spend its parallelism budget on meshing/lighting/network (§2.1-§2.13 of the
plan), not on these bindings.

---

## 5. What the refactor MUST NOT change

Hard invariants (each is a parity requirement for Java Iris 26.1 or for the port's own
consistency):

1. **The per-frame serial chain:** camera back-derivation (GameRenderer.cpp:1029-1054)
   → `RenderCameraState::setFrame` (:1060) → `setDrawCameraStateFromCamera` (:1064) →
   `setFrameUniforms(buildFrameUniforms(...))` (:1100) → BEGIN→shadow→shadowComposite→
   prepare→world→captureOpaqueDepth→hand→captureHandDepth→deferred→post. Never reorder;
   never move a stage to a worker (plan §2.3, §2.4).
2. **`buildShaderFrameData` and its ~25 statics + `g_centerDepthSmooth`/
   `g_wetnessSmooth` stay on the main render phase.** If the EMA state is ever
   refactored into an owned object, it must remain serial-history-ordered and updated
   once per frame at the same point (FrameData.cpp:191-216,266,538-604,670-692).
3. **The gbuffer-matrix convention:** `gbufferModelView` = camera rotation only +
   `chunkOffset` for section-local terrain (RenderCore.cpp:406-419, Uniforms.cpp:67-72).
   Do not add a translation to gbufferModelView; do not switch the projection to a
   [0,1]-clip form unless the *entire* pipeline (world projection, shadow projection,
   depthtex reconstruction, all shipped packs) flips together.
4. **fogMode/fogShape value contract.** Whatever the decision (keep 1/2/3 or move to GL
   constants), the change must be atomic across `FrameData.cpp:237-239`,
   `RenderCore.cpp:464`, the vanilla pack's `common.glsl:18-26`, and `CustomUniforms.cpp:343`,
   and the `fogShape` OFF→−1/ON→1 mapping (FrameData.cpp:239, FogUniforms.java:23,37) must
   stay.
5. **`ScopedDrawCameraState` restore discipline** (RenderCore.cpp:370-386) and the
   GUI-ortho vs world-perspective producers (GuiProjection.hpp:11-30,
   GameRenderer.cpp:1064) must remain on the same thread and in the same
   publish/restore order.
6. **`g_alphaTestRef` becomes main-thread-write-only** (WI-5): never call
   `core::setAlphaTestRef` from a mesh worker (ModModels.cpp:626). The snapshot default
   must stay 0.1 (synthesis QD-20).
7. **Colortex startIndex=4 for world/shadow, 0 for fullscreen** (ColorTargets.cpp:714-717)
   and the legacy alias map gcolor=0/gdepth=1/gnormal=2/composite=3 (GlState.cpp:216-223)
   must be preserved exactly.
8. **Shadow sampler compare rules:** texture-level compare only when
   `!SEPARATE_HARDWARE_SAMPLERS` (ShadowMapPass.cpp:245-246); `shadowtex0HW/1HW` only in
   the separate-HW branch (WorldProgramBinder.cpp:133-141).
9. **SSBO path:** keep `clearBufferSubData` init + `GL_DYNAMIC_STORAGE_BIT` + cap 13
   (Resources.cpp:119-128, Resources.hpp:13) — they are already in parity; do not
   "simplify" them back to the stale CONTEXT.md description.
10. **`g_shaderBlockIds` stays atomic** (RenderType.cpp:14,29-36) if mesh workers ever
    read block ids.
11. **`RenderCameraState::instance()`** remains a main-thread single-frame publication;
    if parallel render is ever attempted, it must become per-frame value copies, not a
    locked singleton.
12. **`entityColor`/entityId/blockEntityId/currentRenderedItemId** upload points
    (RenderCore.cpp:453-458,528-543) must not move off the main GL thread; if the
    `iris_overlay` entity path is ever added, it is additive, not a replacement of the
    existing uniform upload.

---

### Appendix — files inspected

C++: `uniforms/Uniforms.{hpp,cpp}`, `uniforms/FrameData.{hpp,cpp}`, `camera/FrameRenderCamera.hpp`,
`camera/GuiProjection.hpp`, `RenderCore.{hpp,cpp}`, `shaders/WorldProgramBinder.{hpp,cpp}`,
`pipeline/Resources.{hpp,cpp}`, `pipeline/Pipeline.cpp`, `pipeline/Manager.cpp`,
`targets/RenderTargets.hpp`, `targets/ColorTargets.cpp`, `targets/ShadowMapPass.cpp`,
`shaderpack/Pack.hpp`, `shaderpack/Loader.cpp`, `gl/ShaderProgram.cpp`, `gl/GLCore.{hpp,cpp}`,
`RenderType.cpp`, `option/RenderSettings.{hpp,cpp}`, `util/math/Matrix4f.hpp`,
`util/math/MatrixStack.hpp`, `GameRenderer.cpp`, `mod/ModClient.cpp`,
`mod/model/ModModels.cpp`, `render/block/BlockRenderManager.cpp`.

Java Iris (third_party/mcp/iris): `uniforms/{CommonUniforms,MatrixUniforms,CameraUniforms,
ViewportUniforms,CelestialUniforms,FogUniforms,IrisExclusiveUniforms,IdMapUniforms,
CapturedRenderingState}.java`, `gl/IrisRenderSystem.java`, `gl/buffer/{ShaderStorageBuffer,
ShaderStorageBufferHolder}.java`, `gl/sampler/SamplerLimits.java`, `gl/program/ProgramSamplers.java`,
`samplers/IrisSamplers.java`, `targets/RenderTargets.java`, `shadows/{ShadowMatrices,ShadowRenderer}.java`,
`shaderpack/IdMap.java`, `pipeline/transform/transformer/{CompatibilityTransformer,CommonTransformer,
VanillaTransformer,SodiumTransformer,EntityPatcher}.java`, `pipeline/programs/{ProgramCreator,
SodiumPrograms,ShaderCreator}.java`.

Shipped packs sampled: `shaders/vanilla/shaders/lib/common.glsl`,
`shaders/SEUS PTGI Iris/shaders/lib/Uniforms.inc`.
