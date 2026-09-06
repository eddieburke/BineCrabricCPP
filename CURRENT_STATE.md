# Shader compatibility current state

Updated: 2026-09-06. Code checkpoint: `fab0f948` on `main`.

## User objective and constraints

Fix native engine compatibility with unmodified shader packs, matching Minecraft Java + Iris expectations. Do not edit shader pack files to hide engine defects. User confirmed the latest Rethinking Voxels fix worked and moved on to RenderPearl. Latest request is to commit first and record this state; RenderPearl implementation is paused at investigation.

Preserve concurrent work. Do not branch or publish without explicit request. No new source comments. Review executable behavior and upstream implementations rather than trusting existing parity comments.

## Rethinking Voxels: user-confirmed result

Pack: `shaders/rethinking-voxels_r0.1-beta9`.

Original symptom: grid-shaped lighting/shadow discontinuities on a beach, flickering or moving with the camera. The user estimated roughly 16x16 spacing; this was never measured and must not be treated as established chunk alignment.

Concrete defect: `Pipeline::makeWorldBindContext` in `src/net/minecraft/client/render/pipeline/PackLifecycle.cpp` excluded shadow passes from `bindTextureAtlases`. `bindProgramMaterialTextures` therefore skipped the shadow program's `atlasSize` upload and normal/specular bindings. With a newly linked program, atlasSize remained zero.

RVox `shaders/program/shadow.glsl` uses atlasSize to select texture LOD and, around line 368, gates voxel color accumulation on the UV span multiplied by atlasSize exceeding 1.5 pixels. Zero dimensions prevent these writes. Removing the shadow exclusion from material bindings fixes this broken input path. The scene-target shadow exclusion was left unchanged.

After this change, the user said: "Oh holy fuuck that actually worked." This is real visual confirmation of the resulting build, not a controlled isolation of every change in the checkpoint.

Other changes in the checkpoint:

- Draw-camera position storage and its consumers now retain double precision until camera-relative conversion. This avoids premature rounding of absolute coordinates. Its independent contribution to the reported symptom is unconfirmed.
- ComputeDispatcher applies image/texture/storage barriers before a dispatch when requested, including command barriers for indirect dispatch. Group-end barriers remain. The earlier barrier-only build did NOT fix the user's symptom.
- Removed the misleading ComputeDispatcher comment claiming Iris has no pre-dispatch barrier. Iris ComputeProgram dispatch behavior and CompositeRenderer group-end synchronization are separate obligations.

## Iris evidence

Upstream branch inspected: 26.1. These are moving branch references, not pinned revisions.

- [CommonUniforms.java](https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CommonUniforms.java): dynamic atlasSize follows the bound albedo atlas; non-atlas textures return zero.
- [ShaderCreator.java](https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/programs/ShaderCreator.java): shadow shaders receive dynamic uniforms.
- [IrisRenderingPipeline.java](https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/IrisRenderingPipeline.java): addGbufferOrShadowSamplers calls addLevelSamplers for both paths.
- [IrisSamplers.java](https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/samplers/IrisSamplers.java): level samplers include normal and specular textures.

Remaining parity caveats: native code currently uses dimensions of any tracked diffuse texture, while Iris distinguishes actual atlases. Iris also exposes scene/depth samplers to shadow programs; native code still excludes scene targets there. Neither discrepancy has been established as a remaining visual bug. RVox default RP_MODE is 1, so speculation that specular sampling caused false emission under RP_MODE >= 2 was not established for the user's configuration.

## Validation at checkpoint

Final build command succeeded:

```powershell
.\toolchain\mingw64\bin\cmake.exe --build build-omega --target minecraft_omega_tests minecraft_native --parallel 6
```

19 targeted tests passed: all 9 DrawCameraState tests, all 7 ComputeDispatchParity tests, and ShaderGlIntegrationTest cases ShadowProgramsReceiveTerrainAtlasDimensions, ComputeDispatcherSynchronizesBeforeReadingVoxelWrites, and ComputeWritesRenderTargetInPlaceWithoutPingPongCopy.

The new atlas regression creates a real Pipeline in Shadow phase with a tracked 256x128 texture, calls the production material binding path, and reads atlasSize back from GL. It restores camera and Minecraft singleton state through scopes. It does not render an entire RVox scene or test every PBR sampler.

`git diff --check` passed. No shader pack files changed. Commit was made locally; nothing pushed.

## Earlier RVox investigation: inspected, not universally ruled out

- Terrain geometry uses 128x64x128 render regions and 16-block sections; region origins are subtracted in mesh construction. Shared quad indices and aligned base vertices preserve modulo-four vertex IDs. No specific defect found there.
- Mid-block vectors use block-center offsets scaled by 64; signed-byte attribute interpretation and lightmap UV range were inspected. No specific vertex ABI mismatch established.
- Current/previous camera integer and fractional values, frame counter, model-view inverses, and shadow snapping were traced. No additional stale-frame defect proven. Early dismissal of precision based only on coordinates near 240 was not a valid general exclusion.
- Shadow culling has an independent section scan and voxel safe-zone handling. No demonstrated culling defect for this scene.
- Custom 3D image dimensions, layered image bindings, integer/float filtering, compute ordering and dispatch sizes were inspected. No proven cause beyond the binding/barrier changes above.
- Region light snapshots copy a neighbor shell. Lighting update concurrency remains unproven, not eliminated.
- A GL 4.3 image-clear fallback appeared to assume 2D textures when clearTexImage is unavailable. Actual use on the user's GPU was not demonstrated; do not present it as the cause.
- RVox in-place irradiance compute may merit race analysis, but no engine-specific issue was demonstrated and the user-confirmed fix makes it a low priority.

## RenderPearl: current investigation, no fix yet

Pack: `shaders/RenderPearl v2.8.0-beta.4`.

User reports missing colored lighting, bloom and motion blur, with basic shadows and a broken sky. Treat this as the observed complaint, not proof that every named effect is implemented by this pack. Iris/OptiFine compatibility does not automatically add post effects absent from pack code.

Inspected facts:

- All programs live in dimension folders. `dimension.properties` maps `world_default` to `*`, plus named Nether and End folders.
- Deferred and composite processing is compute-based: deferred.csh, composite.csh, composite1.csh, composite2.csh, composite3.csh, composite3_a.csh and composite3_b.csh. There is setup.csh and no final raster program in world_default.
- Historical `build-omega/client.log` entries show rgba8/rgba16f/rgba32ui scene targets and "no final program ... presenting via colortex0 blit". This message alone does NOT prove failure: the final compute writes colorimg0. Those entries are historical, not a fresh RenderPearl capture.
- Required features include BLOCK_EMISSION_ATTRIBUTE, COMPUTE_SHADERS, CUSTOM_IMAGES, ENTITY_TRANSLUCENT, PER_BUFFER_BLENDING, SEPARATE_HARDWARE_SAMPLERS and SSBO. allowConcurrentCompute=true, separateAo=true, separateEntityDraws=true.
- Light collection happens in `prog/lit.vsh`, conditional on LLCollect. It combines mc_Entity.x and at_midBlock.w for emission, filters by vertex ID, distance and intensity, then pushes into the light queue. Custom uniforms schedule LLCollect/LLDedup/LLSort using frameCounter modulo LL_RATE. Trace the queue and its consumers rather than assuming this is RVox's shadow-voxel path.
- shaders.properties declares SSBOs, custom SMAA images/textures, packedView and matrix-derived custom uniforms. Post settings include tonemapping, exposure, SMAA and sharpening; atmospheric settings include SUN_BLOOM/SKY_BLOOM. General bloom/motion-blur support has not been established.

Existing test entry points: RvoxDiag.CompileAllRenderPearlPrograms in tests/rvox_diag_test.cpp; tests/renderpearl_shadow_directives_test.cpp; RealPackFormats.RenderPearlResolvesItsBlockHiddenFormats; tests/custom_uniforms_test.cpp. These existed before this checkpoint and were not rerun for RenderPearl in this investigation.

Next useful work: run the existing real-pack compile check; capture current stage validity and dispatches; verify light-list collection/SSBO contents, emission inputs, custom uniform scheduling, compute image writes and colortex0 presentation. Inspect sky custom uniforms and pack directives with runtime evidence. Do not infer failure from the missing final shader or invent fallback effects. No RenderPearl source edits have been made.
