# Refactor progress / handoff

Living checklist for the native modernization (see `refactor-plan.md` for the
full design). Update this file as steps land so another agent can resume.

**Build/verify:** ONLY `native/build-omega.ps1` (PowerShell). ~50–90s, ~410 TUs.
Last full green build: after WorldEvents filesystem merge + World.hpp hygiene +
WorldEntities/WorldChunks/WorldCollisions extraction (exit 0).
All steps 1–4 below are BUILT GREEN, incl. WorldWeather + ScheduledTickQueue +
LightUpdateQueue modules, WorldEvents filesystem merge, World module split, and
the SimpleBlocks.cpp consolidation.

**Constraints recap:** parity-first (golden tests depend on stable FP, esp.
seedfinding); idiomatic C++ over Java fidelity; minimal scope per change; verify
each step with build-omega. No subagents.

---

## Status checklist

### 1. Simple-block registration consolidation — DONE (codegen REMOVED by user)
- **User feedback:** JSON+Python codegen was overcomplication — deleted
  gen_registry.py, tools/registry/blocks.json, BlocksGenerated.inc/.cpp,
  BlocksGeneratedIncludes.inc, and `RegisterBlockFn` from Registry.hpp.
  Do NOT reintroduce codegen/data-file indirection.
- Replacement: hand-written `native/src/.../block/SimpleBlocks.cpp` — one TU,
  one static registrar calling `Registry::addBlock(id, lambda)` per
  registration group (exact same bootstrap order; multi-block groups like the
  5 ores register together at their original group id). Covers 24 blocks:
  DIRT, COBBLESTONE, BEDROCK, SAND, GRAVEL, GOLD/IRON/COAL/LAPIS/DIAMOND_ORE,
  SPONGE, LAPIS_BLOCK, COBWEB, GRASS(tall), DEAD_BUSH, DANDELION, ROSE,
  BROWN/RED_MUSHROOM, GOLD/IRON/DIAMOND_BLOCK, MOSSY_COBBLESTONE, OBSIDIAN,
  SPAWNER, WHEAT, FARMLAND, CACTUS, SUGAR_CANE, NETHERRACK, SOUL_SAND, CAKE.
- Deleted 24 registration-only per-block TUs + NetherrackBlock.hpp (subclass
  added nothing over plain Block). Kept behavior headers lost
  registerClass/kRegisters/kBlockId; comment points at SimpleBlocks.cpp.
- CMake GLOB_RECURSE auto-picks TU changes (CONFIGURE_DEPENDS) — no CMake edit.
- **Remaining (optional):** blocks with recipe/item hooks keep their own TUs
  (the hooks live there); fold further only if those hooks move too.

### 2. Mesh / Tessellator pipeline — DONE, built green
- `client/gl/GL11.hpp` — added GL_DOUBLE/BYTE/FLOAT + client-array enums
  (VERTEX/NORMAL/COLOR/TEXTURE_COORD_ARRAY) with #undef guards.
- `client/render/Tessellator.hpp/.cpp` rewritten:
  - Dropped insert-time quad→triangle expansion; quads stored verbatim,
    drawn as GL_QUADS.
  - `drawMesh` now batched client-array draw (glVertexPointer + glDrawArrays)
    instead of per-vertex glVertex3d/glColor4ub/etc.
  - Removed dead API: finishWithoutDraw, vertices(), lastDrawnVertices_,
    unused getters, kVertexStride/bufferPosition bookkeeping.
  - Kept singleton INSTANCE + full build API (53 GUI/world/font files depend
    on it) and capture-mode flag (worker meshing).
  - Positions kept `double` for pixel parity (float conversion deferred — needs
    visual verification not possible from build alone; see plan §2).
- ChunkBuilder unchanged (uses takeMesh/drawMesh/setCaptureOnly — all kept).

### 3. World decomposition — DONE, built green
- **WorldEvents filesystem merge — DONE, built green:**
  - Moved the lone `world/event/listener/GameEventListener.hpp` orphan into
    `world/events/GameEventListener.hpp`.
  - Updated all includes (`World.cpp`, `ClientWorld.cpp`, `WorldRenderer.hpp`,
    `BlockEntity.cpp`, `WorldEvents.hpp`) and verified no
    `world/event/listener` references remain.
- **World.hpp hygiene — DONE, built green:**
  - Moved the remaining bulky inline bodies out of `World.hpp` while preserving
    the public API/signatures. Small trivial accessors remain inline.
  - Out-of-line bodies now cover time/sky helpers, region/chunk/light/block
    accessors, brightness helpers, chunk collection helpers, terrain climate
    helpers, and collision/block material predicates.
- **WorldEntities module — DONE, built green:**
  - New `world/WorldEntities.cpp` owns entity collection queries, spawning,
    add/remove notifications, player lookup, spawn-group counts, player add,
    entity ticking/movement bookkeeping, deferred unloads, sleeping-player
    state, block-entity tick processing, and near-entity chunk preloading.
- **WorldChunks module — DONE, built green:**
  - New `world/WorldChunks.cpp` owns chunk cache creation, chunk/block access,
    region/position load checks, packet chunk-data serialize/load helpers,
    top-y/spawn-y helpers, chunk ticking, active-chunk event/random tick loop,
    brightness/light lookups, climate sample accessors, and chunk container
    accessors.
- **WorldCollisions module — DONE, built green:**
  - New `world/WorldCollisions.cpp` owns entity/block collision collection,
    material/fluid/fire/lava box queries, movement-in-fluid flow accumulation,
    submerged checks, fire extinguishing, raycast, visibility sampling, spawn
    collision checks, and block suffocation/solid/opaque/material predicates.
- **WorldWeather module — DONE, built green:**
  - New `world/weather/WorldWeather.hpp` (header-only): owns rain/thunder
    gradient floats (now private), `lightningTicks` (was World public
    `lightningTicksLeft`), `ticksSinceLightning`, `enabled` flag.
    Methods: `rainGradient/thunderGradient(partialTicks)` (enable-gated),
    `tickGradients(raining, thundering)` (shared ±0.01 chase + clamp),
    `decayGradients()` (disabled-weather path), `setRainGradient`,
    `resetGradients`, `beginActiveWeather(thundering)`.
  - World: raw public weather fields REMOVED; protected `weather_` member +
    public `weather()` accessor. getRainGradient/getThunderGradient/
    setRainGradient delegate; isRaining/isThundering unchanged (built on those).
  - Tick control flow stays in World/ClientWorld updateWeatherCycles (server vs
    client differ); they call the module's tick methods.
  - Updated direct-field users: World.cpp (sky flash, prepareWeather,
    applyWorldSettings, updateWeatherCycles, lightning spawn),
    ClientWorld.cpp updateWeatherCycles, Minecraft.cpp (lightning flash
    decrement), LightningEntity.cpp (sets lightningTicks=2).
  - FP parity: gradient math byte-identical (same double-domain ±0.01 + clamp).
- **ScheduledTickQueue module — DONE, built green:**
  - New `world/tick/ScheduledTickQueue.hpp` (header-only): paired
    ordered-set + hash-set (Java TreeSet+HashSet TickNextTick mirror) with
    `schedule` (dedup insert), `tickBudget` (≤1000 + out-of-sync throw),
    `peek`, `popFront`, `empty`.
  - World: `scheduledUpdates_`/`scheduledUpdateSet_` members replaced by one
    `scheduledTicks_`; scheduleBlockUpdate/processScheduledTicks rewired.
    No external callers touched raw containers.
- **LightUpdateQueue module — DONE, built green:**
  - New `world/light/LightUpdateQueue.hpp` (header-only): owns the pending
    LightUpdate vector + both reentrancy counters (per-instance process depth,
    shared-static enqueue depth as before). `push` (merge-into-last-5 + 1M
    pressure valve), `process(world)` (LIFO, budget 100, ≤50 nesting),
    `empty`, `overlapsRegion`.
  - World: `lightingQueue_`/`lightingUpdateCount_`/static `lightingQueueCount_`
    removed; `queueLightUpdate` keeps only world-state checks (pos loaded,
    chunk non-empty) then delegates; `doLightingUpdates` +
    `hasPendingLightingUpdates` overloads delegate inline.
  - **BUG FIX (toward Java parity):** old `queueLightUpdate` early-returned
    inside `try` and skipped the trailing counter decrement (Java used
    `finally`), leaking enqueue depth on every guard return until light
    enqueues died permanently at 50. Module decrements on all paths.
- CMake GLOB_RECURSE auto-picks the new world module TUs — no CMake edit.

### 4. Biome fast path — DONE, built green
- `world/biome/source/BiomeSource.hpp/.cpp` — added `ClimateSample` +
  const, stateless `sampleClimate(x,z)` (local buffers, no member mutation).
  Byte-identical to getBiomesInArea per-cell math.
- `world/World.hpp` — getTemperature/getDownfall now call sampleClimate;
  removed `ensureBiomeSample` helper and `mutable biomeScratch_` member.
- **Deferred:** 64×64 biome LUT; SeedProbe parallel seedfinder fast path
  (plan §5) — larger, parity-sensitive.

### 5. Namespace migration (mc::*) — NOT STARTED (plan §7, do last)

### 6. Filesystem / dead-file hygiene — DONE, built green
- Deleted 6 dead world/ headers (grep-confirmed zero includers, symbols
  self-referenced only): `chunk/storage/DataFile.hpp`,
  `storage/DataFilenameFilter.hpp`, `storage/DimensionFileFilter.hpp`,
  `storage/DimensionFileFilterSubclass.hpp` (empty stub),
  `WorldTypes.hpp` + `WorldForward.hpp` (dead forward-decl pair — WorldTypes
  used by nobody, WorldForward only by WorldTypes).
- Consolidated the two lighting dirs: moved `chunk/light/LightUpdate.{hpp,cpp}`
  into `world/light/` (alongside LightUpdateQueue.hpp) and removed the now-empty
  `world/chunk/light/`. Updated the 3 include paths (World.hpp,
  LightUpdateQueue.hpp, LightUpdate.cpp). Aligns with plan §3's `world/light/`.
- **Deleted 16 orphan headers outside world/** (user-approved; all grep-confirmed
  zero includers + symbol unreferenced): `network/packet/SubPacketTracker.hpp`,
  `util/math/Box.hpp` + `util/math/BlockPos.hpp` (dup types — canonical in
  `util/math/Types.hpp`), `util/math/RandomTypes.hpp`,
  `util/math/noise/NoiseSampler.hpp` (empty abstract stub),
  `util/MD5MessageDigest.hpp`, `util/LongObjectHashMap.hpp`, `util/Tickable.hpp`,
  `block/BlockIds.hpp`, `client/ClientTypes.hpp`, `client/util/ClientTypes.hpp`,
  `client/gui/GuiTypes.hpp`, `client/gui/ParticlesGui.hpp`,
  `client/gui/layout/KeybindLayout.hpp`, `client/particle/ParticleTypes.hpp`,
  `client/render/entity/model/EntityModels.hpp`.

### 7. World field encapsulation — PARTIAL, built green
- Privatized 7 of World's public mutable fields (pure field→accessor, identical
  semantics, parity-safe; field names kept so World's own member files need no
  edits):
  - Internal-only (no accessor, zero external churn): `allowMonsterSpawning`,
    `allowMobSpawning`, `worldTimeMask`, `allPlayersSleeping`.
  - Mode flags → accessors: `newWorld` → `isNewWorld()`;
    `eventProcessingEnabled` → `isEventProcessingEnabled()` /
    `setEventProcessingEnabled()`; `instantBlockUpdateEnabled` →
    `setInstantBlockUpdate()`. Rewrote 11 external sites across 6 files
    (WorldSession, Minecraft, ChunkCache, LegacyChunkCache, 2 seedfinder probes,
    SpringFeature, NetherLavaSpringFeature).
- **Left public (deliberate):** `ambientDarkness` (read in meshing hot paths —
  Chunk.hpp, RegionSnapshot, WorldBlockViewAdapter, WorldRegion — direct field
  read is perf-justified), `difficulty` (×9 external files), `pauseTicking` (×6),
  `isRemote_`, `dimension`, `persistentStateManager`, and the public vectors
  `globalEntities`/`blockEntities`/`players`. These want a dedicated reviewed
  sweep (higher churn / hot-path), not a drive-by.

---

## Next action
Steps 1–4 + WorldWeather module all built green. Next deferred items
(highest value first):
1. Biome 64×64 LUT + SeedProbe parallel seedfinder (plan §5) — parity-sensitive,
   add a same-seed→same-output regression check.
2. Float mesh positions (plan §2) — needs in-game visual verification.
3. Namespace mc::* sweep (plan §7) — last, pure mechanics.
Migrate more trivial blocks into blocks.json anytime (zero-risk LOC win).

## Notes for resuming agent
- Caveman/wenyan-ultra response style is a user preference; code/docs normal.
- Nothing committed yet this session — `git status` shows working-tree changes
  only. User decides when to commit.
