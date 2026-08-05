# Render-path Iris parity — handoff

Status as of 2026-08-04. Written because the shadow-map investigation exposed that the
engine carries **three different vertex/matrix conventions** at once, and packs only
tolerate one of them.

## Why this matters (the bug that started it)

RenderPearl's `world_default/shadow.vsh` does:

```glsl
// `gl_ModelViewMatrix` can be cut to a `mat3` since `shadowIntervalSize == 0.0`, as long as
// model -> view conversion only needs rotation and/or scale, which seems to always be the case in Iris.
immut vec3 clip = shadow_proj_scale.xxy * (mat3(gl_ModelViewMatrix) * model);
```

It **discards the modelview translation**. That is safe in real Iris because MC 1.17+
applies the `PoseStack` CPU-side in `VertexConsumer.vertex(pose, x, y, z)`, so vertices
arrive already camera-relative and `ModelViewMat` is the bare camera matrix.

This engine kept the Beta immediate-mode convention: model-local vertices plus a pose in
the matrix stack. In the shadow pass the `mat3()` dropped `T(entityPos - camera)`, so every
entity collapsed onto the camera with its parts splayed from a point — the "squid legs".

**This is not a pack quirk to work around. Any pack may cut the modelview to a mat3.**
Parity with Iris' vertex convention is the actual fix.

## What already landed

The bake shim (`drawPoseNeedsBake` / `takeDrawPose` / `bakePoseIntoVertices`, which
recovered the pose by inverting the camera matrix every draw) is **gone**, replaced by
the real thing: producers publish the pose, and it never enters a matrix.

Two mutually exclusive per-draw channels in `RenderCore`:

| Channel | Publish | Read | Who |
|---|---|---|---|
| Pose | `setDrawPose(P)` | `drawPose()` | world-camera producers — entities, block entities, particles, world text |
| Matrix | `setDrawModelView(M)` | `drawModelView()` | GUI, hand, sky/clouds — passes that own their stack |

`Tessellator::vertex` applies the pose to positions and `mat3(P)` to the normal at emit
time (`finishQuad` derives tangents from the already-posed corners), so the uploaded
`modelViewMatrix` stays the bare camera matrix — `drawCameraModelView()` returns
`g_drawSectionLocalModelView` whenever a pose is live. This is Java's
`VertexConsumer.vertex(pose, x, y, z)` split, and it is what lets a pack cut
`gl_ModelViewMatrix` to a `mat3` without losing anything.

### The pose is STATE, not a one-shot token

This is the rule that was wrong and cost a full debugging session. `Tessellator::reset()`
used to call `resetDrawPose()`, so the pose died with the batch that consumed it. But a
model publishes **one** bone pose and emits **many** batches from it —
`BipedEntityModel::render` calls `head.render(scale)`, `body.render(scale)`, … and each
`ModelPart::render(scale)` reads `drawPoseValid()` afresh. Every part after the first
found no pose, fell back to the matrix channel, and got the bare camera matrix — which
has no entity translation in it. Result: every entity collapsed onto the camera with its
parts splayed from a point, i.e. exactly the "squid legs" that the shadow investigation
found, except now in the main pass.

The pose is cleared only at real boundaries:

- `setDrawCameraState` — a pass boundary (its raw callers are GUI and hand).
- `setDrawModelView` — the producer claimed the other channel and owns the whole
  transform; leaving the pose live would apply it twice.
- `ScopedDrawCameraState` and `RenderPassScope` save and restore it, so nested renders
  hand the caller its pose back untouched.

### Geometry that renders in BOTH spaces

`ModelPart`, `InventoryBlockRenderer`, `TextRenderer`, `HeldItemRenderer` and
`drawLuaBlockInventory` all run under a world camera (dropped items, a mob's held item,
nametags) *and* in the GUI. They must compose onto whichever channel is live:
`core::drawBase()` to read, `core::publishDrawBase()` to write. `InventoryBlockRenderer`
composing its `-0.5` centring onto `drawModelView()` unconditionally is why dropped block
items sat on the camera.

## Current conventions inventory

| Path | Vertices | modelViewMatrix | Where |
|---|---|---|---|
| Terrain sections | section-local, `vaPosition + chunkOffset` | `g_drawSectionLocalModelView` (camera) | `ChunkRegionBuffer`, `WorldRenderer::renderChunksVbo` |
| Entities / block entities / particles | camera-relative, posed at emit | camera | `Tessellator::vertex` |
| GUI / hand | own space | own stack | `GuiProjection`, `GameRenderer::renderFirstPersonHand` |

Terrain and entities are both Iris-shaped by construction now. What remains is the
terrain *structure* work below (regions, multi-draw), not the convention.

## Target (what Iris/Sodium actually do)

1. Producers emit **camera-relative world positions** directly. No matrix-stack pose is
   ever handed to the GPU for world geometry.
2. `ModelViewMat` = camera matrix, uploaded once per pass, not per draw.
3. Terrain keeps its section-local trick: `vaPosition` is section-local and `chunkOffset`
   is the per-section uniform. This is the one legitimate exception and Iris has it too.
4. Vertex format is a single shared ABI. `VertexAbi.hpp` is already this — keep it.

## The chunk/terrain handoff

This is the part that should be done separately; it is the largest and the least
entangled with the entity work.

### What exists

- `ChunkRegionBuffer` (`src/net/minecraft/client/render/chunk/ChunkRegionBuffer.hpp`) —
  one growable VBO per terrain layer, with a CPU mirror (`shadow_`), a free list, and
  per-section `Slot{offset, capacity, count}` in vertices.
- Per frame: `beginFrame()` → `addVisible(slot, ox, oy, oz)` per visible section →
  `flush()`. `buildMergedRanges()` coalesces adjacent ranges that share a `chunkOffset`.
- `WorldRenderer::renderChunksVbo` computes `ox/oy/oz = chunk->{x,y,z} - sectionOrigin()`
  and walks `chunkSections_.visibleDrawRings()`.
- `ChunkSectionSystem` owns section lifetime, the frontier, draw rings, and culling
  (including the shadow-pass branch that uses `renderCamera.shadowTerrainFrustum`).

### What is actually wrong with it

Nothing about the *convention* — it already matches Iris. The problems are structural:

1. ~~**One draw call per `chunkOffset`.**~~ **STALE — this landed.** Measured 2026-08-05:
   `flush()` uploads `chunkOffset` **once per region per layer** and issues a single
   `glMultiDrawArrays` over every merged range, and `RenderCore` additionally dedups the
   uniform write against the last value. Cost today is one uniform + one draw call per
   region per layer, not per range.

   The remaining gap is *cross-region* batching, which is a materially bigger job than the
   old wording implies: Sodium puts many regions into one indirect multi-draw with no
   state change between them, which needs `glDrawArraysInstanced` + `glVertexAttribDivisor`
   or `glMultiDrawArraysIndirect`. **None of those were loaded in `GLCore`** — only
   non-indirect `glMultiDrawArrays`. `drawArraysInstanced` / `vertexAttribDivisor` and a
   `GLCore::instancedDrawSupported` cap are now added, unused, as the groundwork.

   **The blocker beyond that is the pack ABI, not the GL.** `Core330Transformer.cpp:111`
   rewrites `gl_Vertex` to `vaPosition + chunkOffset` and packs read `chunkOffset` **by
   name as a uniform**. Moving the offset to an instanced attribute means the engine and
   pack programs disagree about where it lives, so it needs a shader-side variant selected
   per program, not a swap. Do not start it without that plan.

   Design note when it is picked up: use a small per-region **instance buffer** (3 floats
   per region, divisor 1), NOT a per-vertex attribute. `VertexAbi.hpp` has no free slot
   (12 is `ChunkFade`, 13 is `IrisLightUv`) and widening `TessellatorVertex` 76 -> 88 bytes
   to carry a value constant across a whole region costs ~16% of terrain memory and trips
   the `static_assert(Stride == 76)` plus every offset assertion after it.
2. **`shadow_` CPU mirror doubles terrain memory.** It exists to support the free
   list/realloc; Sodium uses arena-style GPU allocation without a full mirror.
   **It has a second consumer the doc used to miss:** `[terrain-probe]` samples
   `shadow_[range.first + i]` directly to reconstruct the drawn AABB. Dropping the mirror
   therefore also breaks the main diagnostic for this very work, and needs the probe ported
   to a GPU readback (`glGetBufferSubData` or a mapped range) first.
3. **Region granularity is a single global pool** (`ChunkRegionManager::pool()` returns one
   `ChunkRegion`), so `flush()` cannot exploit per-region locality and `clear()` is a
   no-op. Sodium buckets sections into 8×4×8 regions, each with its own buffer.

### Suggested sequence

1. Introduce real regions: key sections by `(x>>3, y>>2, z>>3)`, one `ChunkRegion` each.
   Keep the existing `Slot`/free-list logic per region — this is mostly plumbing in
   `ChunkRegionManager` and `ChunkBuilder::region_`.
2. Move `chunkOffset` from a uniform to a per-vertex or per-instance attribute so a whole
   region draws in one call. `VertexAbi.hpp` already reserves attribute slots; adding one
   is cheap. **Keep the `chunkOffset` uniform working** — the pack shader transform in
   `Core330Transformer.cpp:111` rewrites `gl_Vertex` to `vaPosition + chunkOffset`, and
   packs read the uniform by name.
3. Replace per-range `glDrawArrays` with `glMultiDrawArrays` (or indirect, if the GL
   version in `GLCore` supports it — check `gl::GLCore` caps first, this is a 2.1-era
   context in places).
4. Only then consider dropping `shadow_`, and only if the free-list rebuild can be done
   from GPU-side bookkeeping.

4. **Quad alignment was load-bearing folklore.** Every `Slot` offset/capacity must be a
   multiple of 4 vertices — `flush()`'s GL_QUADS fast path and its indexed fallback both
   address whole quads. It held only by induction from "terrain always emits through
   `startQuads()`", with nothing enforcing it, and the indexed fallback's
   `(range.count / 4) * 6` **silently dropped up to 3 trailing vertices** instead of
   failing. `allocate()`, `upload()` and the indexed path now assert it.

### Invariants that must survive

- `sectionOrigin()` is the **one** geometry origin. `WorldRenderer::renderChunksVbo`,
  `ChunkSectionSystem::cullChunks` and the shadow map centre all read it. If they diverge,
  shadows detach from terrain and it is very hard to see.
- The shadow pass reuses the same section list through
  `pushCullState()`/`popCullState()`. It runs *before* the main pass's `cullChunks`.
- `cameraPosition` uniform + `chunkOffset` must satisfy
  `gbufferModelViewInverse * viewPos + cameraPosition == worldPos`.

## Traps

- Build once at the end of a multi-file refactor, not per file.
- The test suite is slow, not hung. Killing it mid-run locks the build dir and later
  builds silently keep the old binary.
- Do not delete the `[shadow-probe]` / `[origin-probe]` / `[stage-probe]` diagnostics until
  shadows are confirmed correct. They have been deleted and re-added twice.
- `[origin-probe]` only checks the *camera* origin. It reported "ok" for months while every
  entity draw violated the same invariant. If you add an invariant probe, make it cover
  per-draw matrices, not just the frame camera.
- A producer that publishes a pose and then draws more than once is the *normal* case, not
  an edge case. Any change that makes the pose channel one-shot will silently collapse
  entities onto the camera again. `DrawCameraState.PoseSurvivesRepeatedDraws` pins this.

## Stage 2 (in progress): the shadow contract

Measured 2026-08-05 with `[terrain-probe]` (`WorldRenderer::renderChunksVbo` +
`ChunkRegionBuffer::flush`). Do not re-derive these.

### What the pack actually consumes

`world_default/shadow.vsh` **never reads `gl_ProjectionMatrix`.** It builds clip space
itself:

```glsl
immut vec3 clip = shadow_proj_scale.xxy * (mat3(gl_ModelViewMatrix) * model);
gl_Position = vec4(clip.xy * distortion(clip.xy), clip.z, 1.0);
```

with `shadow_proj_scale = vec2(1.0 / shadowDistance, -2.0 / (shadowFarPlane - shadowNearPlane))`
(`prelude/lib.glsl:9`), all three of those being pack **consts** from
`prelude/directive.glsl` selected by `SM_DIST`. So the only engine inputs that reach the
shadow map are **`gl_ModelViewMatrix` (cut to a mat3) and `gl_Vertex`**. The shadow
projection matrix the engine builds affects culling and the engine's own math only.

Verified sound, do not re-check:

- Const parsing. `SM_DIST == 10` declares `shadowDistance 160 / near -227 / far 227`, and
  the engine's `[shadow-probe]` reports exactly those.
- `distortion()` (`lib/sm/distort.glsl`): `1 / (r*d + 1 - d)` over a Fernández-Guasti
  squircle radius — `1/(1-d) ≈ 10` at the centre, exactly `1` at `r == 1`. It expands the
  middle and pins the edge, so it maps `|clip| <= 1` onto `|clip| <= 1`. It cannot be what
  loses the geometry.
- `clip.z` has no offset term, correctly, because `shadowNearPlane == -shadowFarPlane`
  makes `(f+n)/(f-n)` zero. `viewZ == 0` therefore lands at depth 0.5, which is what the
  probe sees.
- Terrain geometry and the engine's ortho. `[terrain-probe]` in the shadow pass:
  `regions=12 ranges=96 verts=730644 cameraRelativeAabb min=(-120,-68.6,-29.8)
  max=(136,19.4,130.2)`, `uploaded proj m0=0.0063 m5=0.0063 m11=0.0`, `modelView
  translation=(0,0,0)`. `2/320 == 0.00625`, matching `orthoHalf=160`.

### The discrepancy

Submitted casters span ~148 blocks along the light axis `(0, 0.906, -0.423)` and ~256
blocks across it. The map holds `rowWritten=340/2048 colWritten=444/2048 reach=0.217` and
`depth min=0.516044 max=0.521371` — a **2.4-block-thick slab** ~7 blocks off centre.
Expected depth spread for 148 blocks is `148 / 454 ≈ 0.33`; measured is 0.0053, i.e. **~60x
compressed**, with ~21% of the map covered.

So ~98% of correctly-positioned, correctly-culled, correctly-submitted geometry never
lands in the depth buffer. Since the projection is not in the pack's path, the remaining
suspects are exactly two:

1. the **mat3 of the uploaded shadow `gl_ModelViewMatrix`** — if it carries a scale, or is
   not the light-space rotation the engine thinks it is; and
2. **`gl_Vertex` for terrain in the shadow pass** — the `Core330Transformer` rewrite to
   `vaPosition + chunkOffset`, and whether the shadow pass's `chunkOffset` (region origin
   − shadow camera) is the one the vertices were baked against.

Probe those two before anything else. `[shadow-probe]`'s frozen `rowWritten`/`reach`
across frames (identical to 6 decimal places over 38s while the player moved, while
`sections kept` changed) says the map content does not track the camera at all, which
points at (2).

## Still open

The terrain shadow cutoff the investigation started from is **not** explained by any of
this. Measured: shadow map written edge to edge (`reach=1.000000`), casters culled at
~200 blocks, matrices and `s_distortion` correct. Re-check whether it is still present now
that entity shadows are no longer splayed across the frame.
