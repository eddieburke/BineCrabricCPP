# Native modernization refactor plan

Scope: render/tessellation pipeline, World decomposition, threading, worldgen/biome
fast paths, registration codegen, namespace migration. Build/test target:
`build-omega` only (see `.cursor/rules/build-omega-only.mdc`).

## 1. Files read (1-line takeaways)

| File | Takeaway |
|---|---|
| `client/render/Tessellator.hpp/.cpp` | Java-faithful immediate-mode builder: singleton `INSTANCE`, 6 color overloads, quad→tri expansion at vertex-insert time, `captureOnly_` hack for worker threads, double-precision vertices replayed via `glBegin/glVertex3d` per vertex. |
| `client/render/chunk/ChunkMeshJob.hpp` | Job = snapshot + 2 `TessellatorMesh` layers + raw `ChunkBuilder*` validated by version on main thread. Sound shape, wrong payload type (replay list, not GPU buffer). |
| `client/render/chunk/ChunkMeshScheduler.hpp/.cpp` | Thin priority WorkerPool wrapper + completed list under mutex. Fine; keep, retarget output type. |
| `world/World.hpp` | ~950-line god-class: public mutable fields, big inline methods (light sampling, collisions, cloud color, chunk packet serialization), weather+entities+lighting+chunks+redstone fused; `ensureBiomeSample` does noise sampling into mutable scratch per `getTemperature` call. |
| `world/biome/source/BiomeSource.hpp` | Per-query octave-simplex sampling into member `std::vector` scratch (`temperatureMap_`, `queryScratch_`); virtual `clone()` exists for thread isolation — confirms scratch is the threading liability. |
| `block/Block.hpp` | 256-slot parallel static arrays + ~90 named `Block*` statics; registration via per-class `registerClass()` + `RegisterBlock<T>` self-registration statics. |
| `block/StoneBlock.cpp` (sample of ~80 near-identical pairs) | Each trivial block = full .hpp/.cpp pair differing only in id/texture/hardness/sound/translation-key — prime codegen target. |
| `registry/Registry.hpp` | Priority-ordered bootstrap queue with concept-detected hooks (`registerBlockItems`, `registerRecipes`, smelting). Good backbone; keep, feed it generated tables instead of 80 translation units. |

## 2. Render / tessellation pipeline (top priority)

### Problems
- `TessellatorVertex` = 48 bytes (3×double pos, 2×double uv, color, normal) and
  meshes are *replayed* through immediate mode every frame — per-vertex
  `glColor4ub/glNormal3b/glTexCoord2d/glVertex3d` calls dominate render time.
- Quad→triangle expansion at insert time (4 verts become 6) inflates buffers 1.5×
  and bakes the draw mode into the data.
- Singleton `INSTANCE` + `captureOnly_` flag is the thread workaround; worker code
  must remember to flip the flag.

### Design
New types in `client/render` (later `mc::render`):

```cpp
struct MeshVertex {            // 28 bytes, GPU-uploadable as-is
    float x, y, z;             // chunk-relative; chunk origin via glTranslate/uniform
    float u, v;
    std::uint32_t rgba;        // packed ABGR like today
    std::uint32_t normal;      // packed bytes, pad
};

class MeshBuilder {            // value type, NO singleton, no GL includes
    // start(mode)/vertex()/color()/uv()/normal() — same call grammar so block
    // renderers port mechanically; stores quads verbatim (no 4→6 expansion).
    Mesh take();               // moves out vertices + flags
};

struct Mesh { std::vector<MeshVertex> vertices; int mode; VertexFlags flags; };

class MeshUpload {             // GL-thread only
    // VBO (glGenBuffers/glBufferData) + shared quad index buffer (static IBO,
    // 0,1,2,0,2,3 pattern) drawn with glDrawElements. One upload per rebuild,
    // zero per-frame per-vertex calls.
};
```

Key decisions:
- **Float positions, chunk-relative.** Mesh built in section-local space (0..16);
  translation applied at draw. Removes double precision need, halves bandwidth.
- **Shared static index buffer** for quads kills the insert-time triangle expansion.
- **Tessellator survives as façade** for GUI/entity immediate-mode call sites
  (font, items, sky): it owns a `MeshBuilder` and a `drawNow()` that uploads to a
  streaming VBO (or keeps `glBegin` replay initially). Chunk path stops using the
  singleton entirely — `ChunkMeshJob` owns a `MeshBuilder` per layer.
- `captureOnly_`, `takeMesh`, `finishWithoutDraw`, `lastDrawnVertices_` die with
  the singleton-on-worker pattern.

### Migration order (each step builds green)
1. Add `MeshVertex/Mesh/MeshBuilder/MeshUpload` alongside Tessellator.
2. Switch `ChunkMeshResult` to `std::array<Mesh, 2>`; ChunkBuilder writes via
   `MeshBuilder` (call-compatible methods → mostly find/replace of receiver).
3. Upload path: `ChunkBuilder` upload replaces display-list/replay with
   `MeshUpload`; renderer draws VBOs.
4. Delete capture-mode machinery from Tessellator; shrink it to GUI façade.

## 3. World decomposition

`World` stays as the coordinator implementing `IEntityWorld`, but each concern
becomes an owned module with its own .hpp/.cpp (pattern already proven by
`BlockMutationModule`):

| New module | Pulls out of World.hpp |
|---|---|
| `world/light/LightingEngine` | `lightingQueue_`, `doLightingUpdates`, `queueLightUpdate`, `setLight`, `updateLight`, `getLightLevel/getBrightness` recursion, `updateSkyBrightness` |
| `world/WorldChunks` | `chunks_` map, `chunkCache_`, `ensureChunk/getChunk*/isRegionLoaded/getTopY/getChunkData`, preload radius |
| `world/WorldEntities` | `entities_`, `players`, `globalEntities`, spawn/remove/tick/count, closest-player queries |
| `world/WorldWeather` | rain/thunder gradients, lightning, sky color/cloud color/fog color, `updateWeatherCycles` |
| `world/ScheduledTicks` | `scheduledUpdates_`/`scheduledUpdateSet_`, `processScheduledTicks` |
| `world/WorldCollisions` (free functions over `BlockView`) | `getEntityCollisions`, `isMaterialInBox`, fluid/fire box tests, raycast |

Rules:
- Headers declare; every >5-line inline body moves to `.cpp` (cloud color,
  `getChunkData`, collisions, light sampling).
- Public mutable fields become module state with narrow accessors; hot ones the
  renderer reads each frame (`ambientDarkness`, gradients) get plain getters.
- `getBlockId/getMaterial/isAir` stay tiny header inlines on World — they are the
  hottest calls in meshing; delegate to `WorldChunks` inline.
- Duplicated `getLightLevel`/`getBrightness` neighbor-sampling collapse into one
  templated/parameterized helper in `LightingEngine`.

Each module extraction is one commit, `build-omega` green between commits.

## 4. Threading model

Keep what works, remove ceremony:
- **GL thread only:** all `gl::GL11` calls, `MeshUpload`, BlockEntity pointer
  resolution, ChunkBuilder lifetime.
- **Workers:** chunk meshing from `RegionSnapshot` (already detached), worldgen
  noise/terrain fill, region IO. One shared `WorkerPool` (existing) with the
  existing priority lanes; `ChunkMeshScheduler` stays as the meshing front-end.
- Kill per-subsystem bespoke abstractions: no tessellator capture flags, no
  thread-aware singletons. Data crossing threads is value-typed (`Mesh`,
  `RegionSnapshot`) — same hand-back-via-`drainCompleted` pattern everywhere.
- Hazards already catalogued in memory (`chunk-mt-pipeline`): budget-feed FPS fix
  stays; snapshot must include cloned `BiomeSource` (it does) until §5 makes
  biome lookup stateless, after which clones become unnecessary.

## 5. Worldgen / biome fast paths

### Biome LUT
`BiomeInfo` is a pure function of (temperature, downfall). Replace per-query
classification + scratch vectors with:
- `BiomeTable`: 64×64 (T×D) precomputed LUT, built once at bootstrap →
  `biomeFor(t, d)` = two multiplies + index.
- `BiomeSource::sample(x, z)` returns `struct ClimateSample { double t, d; }` by
  value — **no member scratch**, making BiomeSource const/thread-safe and the
  `clone()`-per-snapshot hack removable.
- Area queries write into caller-provided spans (worldgen column fill), not
  mutable members. `World::getTemperature/getDownfall` call the value-returning
  sampler directly; `ensureBiomeSample` + `biomeScratch_` die.

### Seed probing
`resetForProbeSeed` currently rebinds full world state. Fast path:
- `SeedProbe` standalone type owning only noise samplers + LUT; `setSeed` re-seeds
  samplers without touching chunks/entities/properties.
- Seedfinder loops over `SeedProbe` instances (one per worker thread, parallel-for
  over seed ranges) instead of mutating a `World`.
- Octave samplers get batched column sampling (sample 16×16 grid in one call,
  amortizing per-octave setup) — this is where population time goes.

## 6. Registration codegen

~80 trivial block .cpp files differ only in constants. Generator:
`native/tools/gen_registry.py` + declarative data `native/tools/registry/blocks.json`
→ emits `native/src/net/minecraft/block/BlocksGenerated.inc` (included by one
`BlocksGenerated.cpp` TU). See script header for usage. Custom-behavior blocks
(piston, redstone wire, doors...) keep their classes; only their *registration
lines* move to data. Items/entities follow the same scheme later
(`items.json`, `entities.json` → same generator, different template).

Rollout: generate alongside existing files first, diff registration effect
(ids/hardness/sound/translation keys), then delete superseded per-type TUs.

## 7. Namespace migration

Target: `mc::block`, `mc::world`, `mc::render`, `mc::gen`, `mc::util`.
Strategy that never breaks the build:
1. New/refactored types go in `mc::*` directly.
2. Compat aliases at old paths: `namespace net::minecraft::client::render { using mc::render::MeshBuilder; }`.
3. Per-directory mechanical migration (script: rewrite `namespace` blocks +
   includes, add alias header), one directory per commit.
4. Delete aliases when grep shows zero remaining users. Physical file moves last
   (separate from text changes so diffs stay reviewable).

## 8. Execution order

1. §6 codegen script (done, this commit) — zero runtime risk, immediate LOC win.
2. §2 mesh pipeline steps 1–4 (top perf win).
3. §3 World extraction, one module per commit.
4. §5 biome LUT + SeedProbe.
5. §7 namespace sweep last (pure mechanics over the now-smaller tree).

Verification: `build-omega` after every step; in-game smoke (world load, chunk
rebuild under movement, lighting at cave mouths, rain) for §2/§3; seed parity
check (same seed → same spawn/biome map before vs after) for §5.
