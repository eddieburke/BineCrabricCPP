# EXECUTOR P2a REPORT — P-FOGMODE + P-LITGATE

Lane: P2a. Items: P-FOGMODE (fogMode GL constants 0/9729/2049) and P-LITGATE
(lighting-ready gate, split out of WI-7). No builds/tests run (compile-fixer
only). Every cited line was grep-verified against the working tree before
editing; line numbers below are the **post-edit** live numbers.

---

## P-FOGMODE — fogMode GL constants (atomic producers + vanilla pack)

Internal `g_fog.mode` (1 linear / 2 exp / 3 exp2) is unchanged; only the
**uploaded** value changes, via a shared pure helper so both producers and the
test agree.

- `src/net/minecraft/client/render/RenderCore.hpp:76-78` — **new**
  `inline constexpr int fogModeToGlConstant(int mode)` returning
  `mode == 1 ? 0x2601 : 0x0801`. Placed beside `FogUniforms`.
- `src/net/minecraft/client/render/uniforms/FrameData.cpp:237` —
  `values.fogMode = fog.enabled ? fog.mode : 0;` →
  `values.fogMode = fog.enabled ? render::core::fogModeToGlConstant(fog.mode) : 0;`.
  `fogShape` (:239, enabled ? 1 : -1) untouched.
- `src/net/minecraft/client/render/RenderCore.cpp:465` — per-draw upload
  `set1i("fogMode", fog.enabled ? fog.mode : 0)` →
  `set1i("fogMode", fog.enabled ? fogModeToGlConstant(fog.mode) : 0)`.
  (Plan cited :464; the `fogEnd` upload is :464, the `fogMode` upload is :465.)
- `src/net/minecraft/client/render/shaders/CustomUniforms.cpp:343` —
  **verified, no change**: `return i1(frame.fogMode)` already passes the
  transformed value straight through.
- `shaderpacks/vanilla/shaders/lib/common.glsl:18-26` — `fogFactor` rewritten to
  decode the GL constants: `fogMode == 9729` → linear branch, `fogMode == 2049`
  → exp2, else `1.0`. The `==1/2/3` branches and the false "as Iris reports
  them" comment are gone. Only the vanilla pack is touched (it is the only pack
  whose fogFactor lives here).
- `uniforms/Uniforms.cpp:83` `set1i("fogMode", values.fogMode)` — no change
  needed (value transformed at the FrameData producer).

**Parity note:** internal mode 2 (exp, water/lava/mod-exponential) now uploads
0x0801 (GL_EXP2), per the plan's exact formula. The default terrain path (mode
1 → 9729, GL_LINEAR) renders identically. Packs that special-cased 1/2/3 will
flip — intended (Q3).

---

## P-LITGATE — lighting-ready gate

The gate holds the **first** mesh of a freshly-created chunk column until its
lighting drains (`World::doLightingUpdates` marks covered columns lit per
drained region), with a non-optional completion: once the lighting engine goes
fully idle every held column is released (a column with no pending boxes is
never held forever).

### Event channel (World → WorldRenderer)

World must not depend on client/render, so the lit-stamp travels the existing
`WorldEvents` / `GameEventListener` fan-out (the plan's "plain field write into
WorldRenderer's gate state", plan-corrections §2.4).

- `src/net/minecraft/world/events/GameEventListener.hpp:56-64` — **new** virtuals
  `markChunkColumnLit(int chunkX, int chunkZ)` and `markAllChunksLit()`
  (default no-ops, placed after `chunkAvailable`).
- `src/net/minecraft/world/events/WorldEvents.hpp:26-27` + `WorldEvents.cpp:66-72`
  — declarations + dispatch implementations.
- `src/net/minecraft/world/World.hpp:287-288` + `World.cpp:503-507` — public
  `markChunkColumnLit`/`markAllChunksLit` forwarding to `events_`.

### World.cpp doLightingUpdates

- `src/net/minecraft/world/World.cpp:717-738` — after each drained region is
  applied (`events_.setBlocksDirty`, existing behavior preserved), the covered
  chunk columns (block-region → column range via `MathHelper::floorDiv(·, 16)`)
  are marked lit via `markChunkColumnLit`. Then, if the engine is now fully
  idle (`!busy() && !hasDirtyRegions()`), `markAllChunksLit()` releases every
  held column (non-optional completion).
- **Outbox-safety (plan-corrections §2.4):** `drainDirtyRegions`
  (`LightingEngine.cpp:113-126`) holds `outboxMutex_` only for the copy and
  releases before the lit-stamp is dispatched — no lock is held across the
  stamp, so no deadlock.

### WorldRenderer gate state

- `src/net/minecraft/client/render/world/WorldRenderer.hpp:51-71` — **new**
  `world::ColumnLightingGate` struct (`lit`/`gate`/`release`/`releaseAll`/
  `empty` over a `std::unordered_set<SectionPos, SectionPosHash>` keyed with
  `y == 0`).
- `WorldRenderer.hpp:180-182` — member `world::ColumnLightingGate lightingGate_{}`.
- `WorldRenderer.hpp:106-107` — overrides `markChunkColumnLit`/`markAllChunksLit`.

### WorldRenderer.cpp

- `WorldRenderer.cpp:199-211` (`enqueueDirtyChunk`) — first-build gate:
  `if(!lightingGate_.lit(chunk->x >> 4, chunk->z >> 4) && !chunk->built) return;`
  before `noteNearDirty`/`dirtyChunks_.insert`. Re-meshes of already-built
  sections (`built == true`) are never held; the gate only delays the first
  build.
- `WorldRenderer.cpp:239-245` (`createColumn`) — `lightingGate_.gate(sectionX,
  sectionZ)` once per created column, so its first build is held.
- `WorldRenderer.cpp:1202-1210` (`markChunkColumnLit`) — `release(chunkX,
  chunkZ)` then re-enqueue each `dirty && !built` section via
  `enqueueDirtyChunk` (now that the column reads lit, the gate passes).
- `WorldRenderer.cpp:1214-1232` (`markAllChunksLit`) — snapshot the pending
  column keys, `releaseAll()`, re-enqueue every `dirty && !built` section.
- `WorldRenderer.cpp:408` (`clearSections`) — `lightingGate_.releaseAll()` so a
  reload / world switch / view-distance change drops stale gates.
- **No change** to the capture loops (`compileChunks`, ~:824-858): the gate is
  enforced at `enqueueDirtyChunk`, so a gated section never enters
  `dirtyChunks_`/`nearDirtyChunks_`; `markChunkColumnLit`/`markAllChunksLit`
  re-enqueue it, and the next `compileChunks` capture pass meshes it — this is
  the plan's "re-check the lit stamp in the capture loop" intent, satisfied
  without adding a dead second check.
- **No change** to `markDirty`/`setBlocksDirty` (:1149/:1232): the `!built`
  condition already guarantees the gate only delays the first mesh; subsequent
  dirt on built sections stays immediate.

---

## Tests (new; pre-registered as comments by WI-T — compile-fixer must add to MINECRAFT_TEST_SOURCES)

- `tests/fog_mode_parity_test.cpp` — FrameData producer emits 9729/2049/0 for
  linear/exp2(and exp)/off and keeps fogShape 1/−1; `core::fogModeToGlConstant`
  matches 0x2601/0x0801 (the mapping both producers use); CustomUniforms
  `fogMode` resolves to the frame GL constant (0/9729/2049); vanilla
  `common.glsl` decodes `fogMode == 9729` and `== 2049` and no longer contains
  `==1/2/3` or "as Iris reports them".
- `tests/lighting_ready_gate_test.cpp` — `ColumnLightingGate` (production type):
  a gated column is held until released (fresh column not meshed until its
  boxes drain); released columns mesh normally and repeats are idempotent;
  `releaseAll()` (engine idle) never stalls a no-pending-boxes column; ungated
  columns are never held; re-gated columns stay held until the next release.

## Deviations / notes for the compile-fixer

1. **Header edits beyond the strict file list (documented):** P-LITGATE requires
   gate state in the renderer (WorldRenderer.hpp) and a World→WorldRenderer
   signal (GameEventListener.hpp/.cpp, WorldEvents.hpp/.cpp, World.hpp), and
   P-FOGMODE requires a testable shared producer mapping (RenderCore.hpp). The
   plan-master's P2a file list omitted these headers; the plan's own design
   (plan-corrections §2.4 "plain field write into WorldRenderer's gate state")
   and §4.1 P-FOGMODE's test requirement both mandate them. All additions are
   additive (default no-op virtuals, inline constexpr, new members/methods); no
   existing signature changed. P1a (owner of RenderCore.hpp) is complete, so no
   lane conflict.
2. **Capture-loop "re-check" not added literally** — see WorldRenderer.cpp
   section above; the enqueueDirtyChunk gate + re-enqueue-on-lit is equivalent
   and cheaper.
3. **Indentation:** the codebase uses 1-space-per-level in these files (not the
   RULES' generic "2-space"). Each edit matches its surrounding anchor lines'
   actual indentation (some regions are 2-space, e.g. `createColumn`); no region
   was re-indented beyond its original local style.
4. **Line-drift corrections (vs plan-master §4.1):** plan `World.cpp:711-717`
   → actual `doLightingUpdates` `World.cpp:717-738`; plan `enqueueDirtyChunk
   :199-205` → actual gate at `:199-211`; plan capture loop `:791-826` → actual
   `:824-858` (no edit needed, see #2); plan `markDirty :1117-1140` / 
   `setBlocksDirty :1189-1191` → actual `markDirty :1149-1172` /
   `setBlocksDirty :1232` (no edit needed, gate condition covers the "first
   build only" rule); plan RenderCore.cpp `:464` → actual fogMode upload `:465`;
   CustomUniforms `:343` verified unchanged.
5. **Behavior note:** after a teleport / view-distance reload, clearSections
   drops all gates and every re-created column is re-gated until its lighting
   drains (or the engine idles). This is the intended single-mesh-at-correct-
   light behavior of Q11; worst-case it delays a first mesh by the lighting-
   drain duration (typically a few frames) instead of meshing twice.
6. `CustomUniforms.cpp` was in the ownership list but needed **no change**
   (verified pass-through).
