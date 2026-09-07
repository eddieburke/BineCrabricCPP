# Shader compatibility current state

Updated: 2026-09-06. Code checkpoint: `59defe26` on `main`, plus uncommitted work below.
A full inventory of everything confirmed, and everything confirmed twice, is under
**Verification checklist** below. Anything ticked there was read in the source; anything
marked **2x** was confirmed a second time by an independent route.

## Objective and standing constraints

Make the native engine run unmodified shader packs the way Minecraft Java + Iris does.

- Do not edit shader pack files to hide engine defects.
- Do not special-case our own packs in the loader; fix the pack, keep the loader at Iris parity.
- Build with `.\build-omega.ps1` only, once, at the end. Never `ctest` / `-RunTests`.
- No new source comments. Delete wrappers rather than adding layers.
- Prebuilt test executables may be run even while the build is locked.

## Uncommitted working tree

```
CURRENT_STATE.md                                                      (this session)
docs/render-deslop-plan.md                                        D   (this session)
mods/realtime_sky/scripts/hooks.lua                                   (this session)
src/net/minecraft/client/render/uniforms/FrameData.cpp                (this session)

mods/colorful_skies/{config/init.lua,scripts/main.lua}                (user)
mods/fog_settings/{config/init.lua,scripts/main.lua}                  (user)
mods/lib/settings/init.lua                                            (user)
src/net/minecraft/client/gl/GlConstants.hpp                           (user)
src/net/minecraft/client/gui/screen/option/ModSettingsScreen.cpp      (user)
src/net/minecraft/client/render/Tessellator.hpp                       (user)
src/net/minecraft/mod/ModSettingsRegistry.{cpp,hpp}                   (user)
src/net/minecraft/mod/runtime/LuaModSettingsBindings.cpp              (user)
```

The realtime_sky line is the `stars` hook guard described below. `FrameData.cpp` is the
`upPosition` fix and is **unbuilt**. The mod-settings work is concurrent user work — leave
it alone.

## Rethinking Voxels — closed, user-confirmed

Pack: `shaders/rethinking-voxels_r0.1-beta9`. Symptom: grid-shaped lighting/shadow
discontinuities that flickered with the camera.

Cause: `Pipeline::makeWorldBindContext` (`PackLifecycle.cpp`) excluded shadow passes from
`bindTextureAtlases`, so `bindProgramMaterialTextures` skipped the shadow program's
`atlasSize` upload and its normal/specular bindings. On a freshly linked program
`atlasSize` stayed zero, and RVox `program/shadow.glsl` gates voxel colour accumulation on
`uv_span * atlasSize > 1.5`. Removing the shadow exclusion fixed it. The *scene-target*
shadow exclusion was deliberately left in place.

Iris evidence (branch 26.1, moving reference): `CommonUniforms` makes `atlasSize` follow
the bound albedo atlas; `ShaderCreator` gives shadow shaders dynamic uniforms;
`IrisRenderingPipeline.addGbufferOrShadowSamplers` calls `addLevelSamplers` for both paths.

Known remaining parity caveats, neither shown to be a visual bug: we use the dimensions of
any tracked diffuse texture where Iris distinguishes real atlases, and Iris exposes
scene/depth samplers to shadow programs where we still do not.

## RenderPearl — four symptoms closed, one open

Pack: `shaders/RenderPearl v2.8.0-beta.4`. Reported: colored lights work; no bloom; no
volumetric light; banding at night; stars that "move around and jitter up and down" and are
"warped at the bottom"; a slanted sun; straight-edged diagonal panels across the sky.

| Symptom | Status |
|---|---|
| No bloom | **Closed** — the pack has no bloom pass |
| Warped, jittering stars | **Closed** — the pack's own XZ-plane hash, plus per-frame dither SMAA cannot resolve |
| Slanted sun | **Closed** — the pack's Chebyshev cube plus `sunPathRotation = 25.0`, reproduced numerically |
| No volumetric light / godrays | **Closed** — shipped at `VL 4` on a 0-48 scale, and gated `if (is_geo)` so it never crosses open sky |
| Diagonal sky panels | **Open** — best explanation is vanilla clouds; needs the clouds toggle |
| Banding at night | **Open** — attributed to fp16 `n_pe` quantisation, unconfirmed on hardware |

Nothing in the engine has been shown to cause any of the closed four. The defects that *were*
found are real but latent — see **Defects found this session**.

### Proven this session

**The stars are the pack's own.** `world_default/deferred.csh:472-484` synthesises a
procedural starfield:

```glsl
uvec2 seed = uvec2(ivec2(n_pe.xz * 1000.0 + sin(frameTimeCounter * 1000.0) * 0.2));
float16_t stars = max(1.0 - sky_fog_val - skyState.x, 0.0)
                * smoothstep(0.9995, 1.0, pcg(seed.x + pcg(seed.y)) / float(0xFFFFFFFFu));
color = stars + fog_color;
```

**Vanilla stars cannot reach the screen**, three independent gates:

1. `shaders.properties:62` `stars=false` (top level) → `RenderSettings.cpp:72` ANDs it into
   `frameSettings_.renderStars` → `GameRenderer.cpp:1133` skips the draw.
2. `resolveProgramKey` (`PassIndex.cpp:255-262`) walks `programFallbackKey` **only** when
   the key is absent from `definition.programs`. `world_default/gbuffers_skybasic.{vsh,fsh}`
   exist and compile, so `SkyBasic` resolves to itself. **The `SkyBasic → Basic` fallback
   theory is dead — do not revive it.**
3. That program is `prog/none`, whose vsh writes `gl_Position = vec4(NaN, NaN, Inf, 1)`.

**RenderPearl has no bloom.** No bright-pass, no blur chain. `SUN_BLOOM`/`SKY_BLOOM` are a
sun/moon glow term folded into `lib/fog.glsl:51-70`. The glow around lava in the published
screenshots is emissive + tonemap + auto-exposure. Do not add a bloom fallback.

**Volumetric light is shipped nearly off, and it can never touch the sky — SUPERSEDES the
earlier "VL should be on" note.** The user reported no beams at midday; both halves of the
answer are in the pack.

*It cannot draw beams across open sky.* The raymarch body (`composite.csh:67`) and its
application (`composite.csh:135`) are both inside `if (is_geo)`. Only pixels whose ray
terminates on a block receive VL. A shaft crossing empty sky is not something this pack
draws at any setting.

*The shipped strength is ~4e-4.* `composite.csh:165`:

```glsl
ray *= float16_t(float(VL) * 0.001 / float((offsets.length() + 1u) * uint(VL_SAMPLES))) * (1.0 - fog);
```

`config.glsl:77` ships `#define VL 4 // [0 2 4 6 8 ... 48]` — the third entry on a 0-48
list — then scales by `0.001` and divides again by the offset and sample count. Gating is
`#if VL != 0 && defined SHADOWS_ENABLED` (`composite.csh:22`); `SHADOWS_ENABLED` is defined
whenever `SM_DIST != 0` outside END/NETHER (`prelude/lib.glsl:5`, `SM_DIST` default 10), so
the feature is compiled in and simply turned down. No `shaders/*.txt` options file exists,
so nothing of ours pins the value. **Test: raise `VL` to 24-48 in the shader options.**

The shadow path VL depends on was audited in full and is clean — see the checklist section
**E. Volumetric light and the shadow samplers**.

### Why the stars look the way they do

Most of it is the pack's own hash geometry, not us. The seed is a 2D grid over `n_pe.xz`,
which is the unit direction projected onto the horizontal plane. For altitude φ:

| | cell extent in seed space |
|---|---|
| per unit azimuth | `1000 · cos φ` |
| per unit altitude | `1000 · |sin φ|` |

So the cell aspect ratio is `cot φ`: round dots at 45°, **vertically smeared toward the
horizon**, swirled into arcs at the zenith. `sky_fog = 0.25 / (max(n_pe.y, 0)² + 0.25)`
(`lib/fog.glsl:24-28`) fades stars out completely at the horizon and they only reach 25%
brightness above ~17° altitude — which is exactly the band where `cot φ ≈ 3.3` and the
smearing is worst. **The lowest visible stars are streaks by construction.** Java does the
same thing.

The jitter is the twinkle: `sin(frameTimeCounter * 1000.0)` decorrelates completely between
frames and shifts the grid by ±0.2 cells, flipping every star near a boundary. Our
`frameTimeCounter` matches Iris exactly (accumulate `frameTime`, wrap at 3600 —
`FrameData.cpp:58-62` vs `SystemTimeUniforms.java:85-90`). Its severity scales with
framerate, so it looks worse at high fps than in the reference footage.

The banding is `n_pe` being `f16vec3`: the star grid *is* the fp16 quantisation, whose step
doubles at every exponent boundary (|x| = 0.5, 0.25, 0.125…).

### The slanted sun — CLOSED, it is the pack

`deferred.csh:487-491` tests `max3(abs(n_pe - sunDirectionPlr)) < SUN_SIZE` — a Chebyshev
ball, i.e. an axis-aligned cube of half-width `SUN_SIZE` (0.06) in **world**-direction space
cut by the unit sphere. Each constraint pair `|n_i - s_i| < 0.06` cuts a tangent-plane strip
of half-width `0.06 / sqrt(1 - s_i^2)`, so the silhouette is a hexagon in general position,
oriented by the world axes rather than the screen, ~6.9 degrees across, added flat with no
falloff.

The reason it is never square in practice: `prelude/config.glsl:80` ships
`const float sunPathRotation = 25.0` (inside a `/* */` block, which the loader reads
correctly). That takes the sun off the world Y-Z plane for the whole day, so all three
strips sit in general position and the silhouette is a rotated hexagon whose tilt changes
with the sun's altitude — straightening out only near the horizon.

Reproduced numerically outside the engine, from the pack's own expressions with
independently verified inputs — camera pointed dead at the sun, zero roll — and it matches
the 2026-09-06 screenshot: a slanted, rotating quad. Do not re-open this as an engine bug.
The check is a throwaway numpy script and takes minutes to rebuild: camera basis
`Rz(roll) Rx(pitch) Ry(yaw+180)`, ray `normalize(M^-1 (ndc.x/px, ndc.y/py, -1))`, sun from
`celestialSkyMatrix` column 1, then the three expressions above.

The reconstruction chain behind it was verified end to end the same run, and the
`n_pe` half is now as closed as the sun half:

- raster modelView is `bob * MV_camera` (`GameRenderer.cpp:519-546`) and
  `buildCameraModelView` rebuilds exactly that, so `MV^-1 * P^-1 * ndc` recovers the true
  ray. Bob/hurt/nausea all being in the modelview rather than modern Minecraft's projection
  is Iris' own `MixinModelViewBobbing` rearrangement, and is associativity-neutral for the
  raster.
- `buildCameraViewRotation` re-derives the basis from `applyCameraTransform` on an identity;
  its forward vector matches Minecraft's `(-sin y cos p, -sin p, cos y cos p)` to 1e-6 at
  every orientation tested.
- `Matrix4f::invert` is a true cofactor inverse, not an affine shortcut.
- `gbufferProjection` is the clean symmetric perspective the pack's sparse `proj_inv_mmul`
  assumes: only `zoomOffsetX/Y` can break that sparsity, and `depthScale` cannot.
- the custom-uniform `.A.B` convention is column-then-row, which is what the pack's own
  `sunDirectionPlr` expansion and `mvInv0..3` both assume, so `MV_INV` really is the
  inverse rotation.
- `sunDirectionPlr` is MV-independent by construction: `sunPosition = mat3(MV) * sunWorld`
  and the pack applies `mat3(MV^-1)`, so no camera-basis difference can move or reshape it.

One latent trap found in `invert()`: the `abs(det) < 1e-10` guard silently returns identity.
A perspective det is ~1e-1 and the current shadow ortho ~1e-7, but the ortho det falls as
the cube of shadow distance and reaches ~5e-10 around shadowDistance 1024.

**Unattributed.** A 2026-09-06 screenshot shows large straight-edged diagonal panels of
slightly different blue across the sky. Nothing in `sky()` produces straight lines —
`sky_fog` is a function of `n_pe.y` so its iso-lines are conics, and both bloom terms are
radial about the sun. RenderPearl never sets `clouds=`, so vanilla clouds still draw through
`gbuffers_clouds`, and the edge of Beta's cloud plane is a straight line. Next step: toggle
clouds and see whether the bands go with them.

## Verification checklist

Everything below was read in the source at the cited location. **2x** means it was confirmed
a second time by an independent route — a different file, a numeric reproduction outside the
engine, or a hypothesis that was pursued and died against it. `[!]` marks something true but
carrying a caveat. **Do not re-walk a ticked line without new evidence.**

### A. Camera, matrices, ray reconstruction

- [x] **2x** `gbufferModelView = bob * MV_camera`. Raster path `GameRenderer.cpp:519-546`;
      `buildCameraModelView` (`FrameRenderCamera.hpp`) rebuilds exactly that. Independently
      re-derived from a second `applyCameraTransform` on identity at
      `GameRenderer.cpp:1017-1046`.
- [x] Bob/hurt/nausea living in the modelview rather than modern Minecraft's projection is
      Iris' own `MixinModelViewBobbing` rearrangement, and is associativity-neutral.
- [x] **2x** The camera basis forward vector matches Minecraft's
      `(-sin y cos p, -sin p, cos y cos p)` to 1e-6 at every orientation tested — checked in
      `buildCameraViewRotation` and again numerically outside the engine.
- [x] `Matrix4f::invert` is a true cofactor inverse, not an affine shortcut (`Matrix4f.cpp`).
- [x] `gbufferProjection` is a clean symmetric perspective, `m[11] = -1`, zero off-diagonals —
      exactly the sparsity the pack's `proj_inv_mmul` (`lib/mmul.glsl:30-36`) assumes.
- [x] `depthScale` scales row 2 only. Only `zoomOffsetX/Y` left-multiplies a translation that
      breaks that sparsity (`FrameRenderCamera.hpp:228-235`), and only while zoom is active.
- [x] **2x** The custom-uniform `.A.B` convention is column-then-row
      (`CustomUniforms.cpp:747`), cross-checked against the pack's own `sunDirectionPlr` and
      `mvInv0..3` expansions.
- [x] `sunDirectionPlr` is MV-independent by construction: `sunPosition = mat3(MV) * sunWorld`
      and the pack applies `mat3(MV^-1)`. No camera-basis difference can move or reshape it.
- [!] `invert()` silently returns identity below `abs(det) < 1e-10`. A perspective det is
      ~1e-1 and the current shadow ortho ~1e-7, but the ortho det falls as the cube of shadow
      distance and reaches ~5e-10 near shadowDistance 1024. Latent.

### B. Sky and fog colour

- [x] `fogColor` is the live GL fog colour (`FrameData.cpp:76-82`) — Iris parity.
- [x] `skyColor` is Beta's `getSkyColor(camera, tickDelta)` — Iris parity with
      `level.getSkyColor`.
- [x] In this pack the sky **is** the fog colour: `lib/fog.glsl:49` computes
      `mix(skyColorLinear, linear(fogColor) * 2.0, sky_fog)`.
- [x] `sky_fog = 0.25 / (max(n_pe.y, 0)^2 + 0.25)` (`lib/fog.glsl:24-28`), a function of ray Y
      alone, so its iso-lines are conics and it cannot produce straight edges.
- [x] `isEyeInWater` ruled out by observation — a nonzero value clamps `sky_fog` to 1 and kills
      stars entirely, which is the opposite of the reported symptom.
- [!] Parity here is measured against a **different game**. RenderPearl was authored against
      modern Minecraft's `FogRenderer` colour and modern `getSkyColor`; Beta 1.7.3's curves are
      not those. The gradient can be wrong with a correct loader. Pack-side, and not ours to
      special-case.

### C. Stars

- [x] The pack synthesises its own starfield (`deferred.csh:472-484`).
- [x] **2x** Vanilla stars cannot reach the screen — three independent gates (`stars=false`,
      `SkyBasic` resolving to itself, `prog/none` writing NaN). The `SkyBasic -> Basic`
      fallback theory was pursued and killed at `PassIndex.cpp:255-262`.
- [x] The `frameSettings_.renderStars` gate chain verified end to end.
- [x] The seed is `n_pe.xz * 1000` on an integer grid — the celestial sphere flattened onto the
      horizontal plane. Cell aspect ratio is `cot(altitude)`, so stars are round at 45 degrees
      and smear vertically toward the horizon. **This is the "warped at the bottom".**
- [x] Stars never rotate: the seed is a pure function of world direction, with no celestial
      matrix anywhere in the expression.
- [x] `ivec2` truncates toward zero, so the cells straddling x=0 and z=0 are double width — a
      cross-shaped seam through the zenith.
- [x] Above and below the horizon mirror each other: equal `abs(y)` gives the same `xz`.
- [x] `skyState.x` is `rainStrength` (`shaders.properties:139`). Stars are dimmed by rain and by
      `sky_fog`, **not** by daylight — they are drawn in the day and simply swamped.
- [x] `frameTimeCounter` matches Iris exactly: accumulate `frameTime`, wrap at 3600
      (`FrameData.cpp:58-62` vs `SystemTimeUniforms.java:85-90`).
- [x] The `sin(frameTimeCounter * 1000.0) * 0.2` term shifts the grid by up to ±0.2 cells per
      frame, flipping every star near a boundary. It reads as dither intended for TAA, and this
      pack's AA is SMAA — spatial only, and structurally unable to resolve it.
- [x] The banding is `n_pe` being `f16vec3`: the star grid *is* the fp16 quantisation, whose
      step doubles at every exponent boundary.

### D. Sun and moon

- [x] **2x** The slanted sun is the pack's Chebyshev cube test (`deferred.csh:487-491`) plus
      `sunPathRotation = 25.0`. Reproduced numerically outside the engine from the pack's own
      expressions and matched against the 2026-09-06 screenshot. CLOSED.
- [x] `sunPathRotation = 25.0` ships inside a `/* */` block at `prelude/config.glsl:80`, and the
      loader reads it correctly.
- [x] `sunAngle` and the pack's `day` uniform checked against `CelestialUniforms.getSunAngle`.
- [x] `upPosition` was shipped at unit length where Iris transforms `(0, 100, 0)`. **Fixed** at
      `FrameData.cpp:159`; no installed pack reads it, so latent. **Unbuilt.**

### E. Volumetric light and the shadow samplers

- [x] The gate is `#if VL != 0 && defined SHADOWS_ENABLED` (`composite.csh:22`).
- [x] `SHADOWS_ENABLED` is defined whenever `SM_DIST != 0` outside END/NETHER
      (`prelude/lib.glsl:5`); `SM_DIST` defaults to 10, so the feature compiles in.
- [x] Both the raymarch (`composite.csh:67`) and its application (`composite.csh:135`) are
      inside `if (is_geo)`. **Sky pixels are never touched at any setting.**
- [x] Shipped strength is ~4e-4: `float(VL) * 0.001 / ((offsets.length()+1) * VL_SAMPLES)` at
      `composite.csh:165`, with `VL 4` on a `[0 2 4 ... 48]` list (`config.glsl:77`).
- [x] **2x** No `shaders/*.txt` options file exists, so nothing of ours pins `VL` — re-checked
      this session.
- [x] `sample_sm` (`lib/sm/sample.glsl`) samples `sampler2DShadow shadowtex0HW, shadowtex1HW`
      and short-circuits to black when `solid_vis == 0`, so a missing compare mode would kill
      VL outright. It is not missing:
  - [x] The pack declares `SEPARATE_HARDWARE_SAMPLERS` in `iris.features.required`
        (`shaders.properties:54`).
  - [x] The loader parses `iris.features.required` / `.optional` (`Loader.cpp:1135-1138`) into
        `requiredFeatures` / `optionalFeatures` (`Pack.hpp:214-215`).
  - [x] `featureSupported` maps it to `samplerObjectsSupported` (`GlState.cpp:601`) and
        `featureEnabled` ANDs the two (`GlState.cpp:613`).
  - [x] `shadowSampleMode` sets `compare = separateHw && (hwFilter || sampler2DShadow)` for
        `*HW` names (`GlState.cpp:96-101`).
  - [x] The sampler object carries `GL_TEXTURE_COMPARE_MODE = GL_COMPARE_REF_TO_TEXTURE` and
        `GL_TEXTURE_COMPARE_FUNC = GL_LEQUAL` (`GlState.cpp:40-41`).
  - [x] Composite stages get the `*HW` names too — `putShadowTextures` (`GlState.cpp:163-165`)
        is called from `CompositeRenderer.cpp:115`, not only from `WorldProgramBinder`.
- [x] The `max(0.05f, shadowNearPlane)` clamp fires **only** on the perspective path
      (`shadowMapFov > 0`, `ShadowMapPass.cpp:294`). RenderPearl is orthographic, so its
      symmetric `-227 / +227` box passes through raw at `ShadowMapPass.cpp:271-272`.
- [x] The pack's ortho convention matches ours: symmetric near/far gives a zero Z offset, which
      is what `shadow_proj_scale.y = -2/(far - near)` with no translate term assumes.
- [x] `shadowDistance` and the `#if SM_DIST` plane chain read from the correct branch — closed
      by earlier work; do not re-derive.

### F. SMAA, custom textures and custom images

- [x] `smaa_area.bin` is exactly 179,200 bytes = 160 x 560 x RG8. `smaa_search.bin` is exactly
      1,024 = 64 x 16 x R8. Both match their declarations.
- [x] `readFile` opens with `std::ios::binary` (`Catalog.cpp:14-29`), so raw `.bin` payloads are
      not newline-mangled on Windows.
- [x] The 7-field raw `customTexture.` form parses correctly, including the 1D/2D/3D field-count
      branch (`Loader.cpp:1357-1405`).
- [x] Format names resolve: `RG` 0x8227, `RED` 0x1903, `UNSIGNED_BYTE` 0x1401, `HALF_FLOAT`
      0x140B (`GlState.cpp:366-406`).
- [x] The pack ships `.mcmeta` sidecars and the loader honours them (`Resources.cpp:198-207`):
      `smaa_area` gets `blur:true` -> LINEAR, `smaa_search` gets `blur:false` -> **NEAREST**,
      both CLAMP. That is exactly SMAA's requirement.
- [x] **2x** `composite1.csh:91` writes `edge` only inside `if (any(edges))`, so it never writes
      zeros and a per-frame clear is mandatory — and `Pipeline.cpp:353-384` implements
      `clearEachFrame`, with `Loader.cpp:1449` parsing it from the right field index.
- [!] The `.mcmeta` read tries only `shaders/<path>.mcmeta`, where the data read tries two
      candidates. Asymmetric; benign for this pack's layout.
- [!] `static gl::GlFramebuffer clearFbo` is shared across all declared images on the
      non-`clearTexImage` fallback path (`Pipeline.cpp:361-384`). Unverified minor risk.

### G. Pack loading, options and directives

- [x] Dimension selection: `dimension.world_default=*`, so the log line `dimension '*'` is
      correct and `world_default` really is in use.
- [x] Const directives inside `/* */` blocks are read — confirmed for `sunPathRotation` and for
      the `colortexNClear` block at `prelude/directive.glsl:20-31`.
- [x] `colortex2Clear = false` and `colortex2Format = RGBA32UI`, with no `#if` branching around
      the block.
- [x] No saved shader-options file exists for this pack, so every option is at its source
      default.

### H. Draw ordering and target routing

- [x] **2x** Clouds draw **after** `renderDeferred` (`GameRenderer.cpp:1226-1287`) — the correct
      Iris order. The "clouds relit as geometry" hypothesis was pursued and died here.
- [x] `gbuffers_clouds` writes only `RENDERTARGETS: 1`, so it can never set the
      `deferred_ignore` marker in colortex2.
- [x] `deferred.csh` leaves `DEFERRED_IGNORE` pixels untouched — the whole body is inside
      `if (!deferred_ignore)`, so it cannot paint sky over `gbuffers_basic` geometry.

### I. Surface parity

- [x] Zero of 75 Iris `shaders.properties` keys are missing.
- [x] 24 of 176 Iris uniforms are absent, and every one is Distant Horizons, Iris' own
      core-profile `iris_*`, or Sodium `u_*`.
- [x] RenderPearl compiles fully (`RvoxDiag.CompileAllRenderPearlPrograms`), the pipeline
      builds, and `client.log` has no warnings or errors.

### Hypotheses pursued and killed — do not revive

| Hypothesis | Killed by |
|---|---|
| `SkyBasic` falling back to `Basic` | `resolveProgramKey` walks the fallback only when the key is absent, and the program exists (`PassIndex.cpp:255-262`) |
| Vanilla stars leaking through | Triple-gated: `stars=false`, self-resolving program, `prog/none` writes NaN |
| Modern-MC bob-in-projection divergence | Associativity-neutral, and Iris does what we do (`MixinModelViewBobbing`) |
| Transposed `MV_INV` | The pack's own `sunDirectionPlr` expansion fixes the convention as column-then-row |
| Volumetric light painting the sky | Entirely inside `if (is_geo)` (`composite.csh:67,135`) |
| Clouds relit by the deferred pass | Clouds draw after `renderDeferred` (`GameRenderer.cpp:1231-1287`) |
| Custom images not cleared per frame | `clearEachFrame` is parsed *and* implemented (`Pipeline.cpp:361`) |
| SMAA lookup tables mis-loaded or mis-filtered | Exact byte sizes, binary-safe read, `.mcmeta` honoured with the correct per-texture filter |
| A missing hardware shadow compare mode killing VL | Full chain verified, feature flag through to `glSamplerParameteri` |
| `shadowNearPlane` clamped to 0.05 | The clamp is on the perspective path only; the pack is ortho |
| Bloom missing | The pack has no bloom pass; `SUN_BLOOM`/`SKY_BLOOM` are a fog-glow term |
| `isEyeInWater` stuck nonzero | Would kill stars entirely; they are visible |

### Open questions

1. Raise `VL` to 24-48 and look at midday. That is the only honest test of volumetric light,
   and it must be judged **on terrain**, never against open sky.
2. Toggle clouds off. The unattributed straight-edged diagonal panels are still best explained
   by vanilla clouds drawing through `gbuffers_clouds`, whose fsh deliberately mixes cloud
   colour toward `fog_col` — distant clouds are a slightly different blue by design.
3. Stationary camera, no mouse movement: do the stars still fizz? Expected yes, from the
   per-frame seed jitter. A no would reopen `MV_INV`.
4. Does the driver report `GL_EXT_shader_explicit_arithmetic_types_float16`? If fp16 is
   emulated in fp32 the star grid changes character entirely versus Java.

### Engine-side levers that could still make it worse than Java, ranked

1. **`n_pe.y` too small** — flatter rays push the `sky_fog` fade band lower and wider, so the
   streaky zone gets brighter and bigger. Sources: `projectionY`, `viewHeight`, or
   `aspectRatio` applied to the wrong axis.
2. **fp16 not actually fp16** — see open question 4. RenderPearl *requires* 16-bit types.
3. **`packedView` fp32 rounding** — found, real, small (see below).
4. **Off-centre projection** — `zoomOffsetX/Y` breaks the sparsity `proj_inv_mmul` assumes.
   Only bites while zoom is active.

## Defects found this session

**`upPosition` was 100x too small — FIXED, needs a build.** Iris'
`CelestialUniforms.getUpPosition()` transforms `(0, 100, 0)` by `gbufferModelView`;
`FrameData.cpp:159` passed `(0, 1, 0)`, so the uniform shipped at unit length. No pack in
`shaders/` reads it (checked all four plus the vanilla pack), so this was latent, not the
cause of anything observed. Changed to `100.0f` at the same call site.


**Fixed.** `mods/realtime_sky/scripts/hooks.lua:545` — the `world_render` /
`stage = "stars"` hook was missing `if event.shader_pack_active then return event end`.
Every sibling has it (the `sky` hook, both `world_color` hooks). It was writing
`star_brightness` and the mod-to-mod astronomy bus (`astronomy_enabled`,
`astronomy_utc_millis`, observer lat/long — consumed by `northern_stars`) under a shader
pack. Latent rather than causal, since `renderStars` is false anyway. Lua only, no build.

**Open — `uniform.int` expressions evaluate in float32.** `applyBinary`
(`CustomUniforms.cpp:495-501`) takes the integer path only when *both* operands are already
Int, and `floor()` returns Float, so RenderPearl's
`uniform.int.packedView = 65536 * floor(viewHeight) + floor(viewWidth)` loses low bits above
2²⁴. At 1366×768 the packed value is 50,333,014 with a ULP of 4, so `deferred.csh:187` reads
the width back as 1368. 854×480, 1920×1080 and 2560×1417 land on the ULP grid by luck. Iris
does this in integer arithmetic. Fix by letting the declared uniform type drive the
arithmetic; ~0.2% ray skew at the screen edge, so correctness only.

**Open lead — dawn/dusk stalls.** `World::updateSkyBrightness` (`World.cpp:271`) fires only
when `int(curve * 11.0f)` steps, and `ChunkSectionSystem::notifyAmbientDarknessChanged`
(`ChunkSectionSystem.cpp:444`) then invalidates and re-enqueues **every sky-lit section in
the render distance** in one loop. That is ~22 steps per Minecraft day, bunched at dawn and
dusk. realtime_sky maps one real day onto one game day, so the sun crawls ~70× slower and
the steps are ~70× rarer; switch it off and the vanilla cycle fires bursts of full-world
remeshes. This matches the user's report that turning realtime_sky off causes periodic hard
stalls. Unmeasured. `World::setTime` also calls it, so any `minecraft.world.set_time` from a
mod triggers it too.

**Open, unexercised.** Our raw `texture.<stage>.<sampler>` override is name-only, where Iris
generates a `customtex<N>` sampler and renames it through a type-gated GLSL AST pass
(`TextureTransformer.isTypeValid`). We also route `TEXTURE_1D` declarations into the 2D map
(`Resources.cpp:309`). No installed pack uses the raw form.

## Surface parity, measured

Not the problem. Zero of 75 Iris `shaders.properties` keys are missing. 24 of 176 Iris
uniforms are absent and every one is Distant Horizons, Iris' own core-profile `iris_*`, or
Sodium `u_*`. RenderPearl compiles fully — `RvoxDiag.CompileAllRenderPearlPrograms` passes —
the pipeline builds, and today's `client.log` has no warnings or errors.

`customtex` is **not** a pack-authored directive. It appears exactly once in all of Iris
(`ShaderProperties.java:460`) as an internally generated sampler name for raw
`texture.<stage>.<sampler>` overrides.

## Test entry points

`RvoxDiag.CompileAllRenderPearlPrograms` (`tests/rvox_diag_test.cpp`),
`tests/renderpearl_shadow_directives_test.cpp`,
`RealPackFormats.RenderPearlResolvesItsBlockHiddenFormats`,
`tests/custom_uniforms_test.cpp`.

`CompileAllRenderPearlPrograms` asserts non-null `worldPrograms` for TerrainSolid, Cutout,
Translucent, Entities, EntitiesTranslucent and Hand — but **not** SkyBasic or SkyTextured.
Worth adding now that sky program resolution has been reasoned about rather than tested.
