# Session handoff — 2026-08-12

## Done this session (all built + tested green, Release, exit 0)

1. **MC_MIPMAP_LEVEL fixed.** Was `log2(render-target/shadow-map dimension)` (~10-11) in
   `Resources.cpp` and `Loader.cpp`. Now sourced from the game's actual mipmap setting via
   new `TextureManager::MIPMAP_LEVEL` static (set in `GameOptions.cpp applyDerivedSettings`),
   matching Iris `StandardMacros.java:50`. Baked at shader compile time — changing the slider
   mid-session still needs a pack reload. None of the 3 installed packs read this macro, so
   it's real parity but not the outline cause.

2. **Invented `const bool shadowEntities` GLSL directive deleted** from
   `Loader.cpp` (was ~line 409). Iris has no such directive — only a `shaders.properties` key,
   which we already handled correctly and still do.

3. **`drynessHalflife` now bug-for-bug with Iris.** Java's `PackDirectives.java:280-284` has
   a copy-paste bug: both the `wetnessHalflife` and `drynessHalflife` directive lambdas
   assign `this.wetnessHalfLife`; `drynessHalfLife` is `private final`, hardcoded 200.0f,
   unreachable by any pack. We used to honor the pack's dryness value independently (more
   "correct", therefore wrong). `PackDefinition::drynessHalflife` field deleted entirely;
   `updateWetnessSmooth` now takes 3 params, holds `kDrynessHalflife=200.0f` internally.
   **Do not "fix" this back — see memory `dryness-halflife-aliases-wetness.md`.**

4. **Custom image parsing (`image.N`) no longer rejects unreadable pixel formats.**
   `ShaderProperties.java:533-535` logs and *keeps* the declaration on a bad format; only bad
   arity returns early. We were `continue`-ing (dropping) on bad format — wrong on both
   counts (an invented rejection our own comment justified with a false premise).

5. **Raw custom textures (`texture.`/`customTexture.`) now DO reject unreadable formats** —
   opposite of #4. Java's `.orElseThrow()` there sits in no try/catch, so a malformed raw
   texture takes the whole pack load down. Added a proper error message in `load()`'s
   validation block (next to the SSBO/CUSTOM_IMAGES checks) rather than silently dropping.
   PNG textures are exempt (hardcoded valid formats for the encoded case).

6. **8 invented parse-time clamps deleted.** Iris clamps exactly one const directive —
   `ambientOcclusionLevel` → `[0,1]`. Everything else (`wetnessHalflife`, `shadowDistance`,
   `voxelDistance`, `shadowIntervalSize`, `entityShadowDistanceMul`, `eyeBrightnessHalflife`,
   `noiseTextureResolution`, `shadowMapResolution`) is stored raw in Java. `CAction` enum
   collapsed `Direct/ClampMin0/ClampAO/ClampNoise/EntityShadow/CenterDepth` →
   `Direct/ClampAO/CenterDepth`. The `entityShadowDistanceMul < 0.01 → 0` snap was actively
   harmful (Java only special-cases `==1.0F || <0.0F`). Clamps that are genuinely needed for
   GL safety moved to their **allocation site**: `shadowMapResolution` already was
   (`GameRenderer.cpp:214`, unchanged); `noiseTextureResolution` moved to `Resources.cpp:46`
   (clamps `[1,4096]` right before `glTexImage2D`, compares against the *clamped* value so it
   doesn't regenerate every frame). See memory `iris-clamps-nothing-but-ao.md`.

7. **Boolean-parsing divergence fixed** (this is what Eddie explicitly flagged — "why the
   fuck that image thing is being done"). Iris has TWO boolean parsers:
   - `shaders.properties` keys (`shadowPlayer`, `separateAo`, `flip.*`, etc.) →
     `handleBooleanValue`/`handleBooleanDirective` (`ShaderProperties.java:626-643`), accepts
     `"1"`/`"0"` alongside `"true"`/`"false"`. This is our existing `boolean()` helper —
     correct, untouched.
   - `image.N`'s `clearEachFrame`/`relative` fields, and `bufferObject.N`'s long-form
     `relative` field → raw `Boolean.parseBoolean` (`ShaderProperties.java:423,537-539`) —
     **only** case-insensitive `"true"` is truthy; `"1"` is **false** there.
   Added `javaBoolean()` helper matching the strict semantics, pointed the 3 real call sites
   at it (`Loader.cpp` ~1392, 1417-1418). `bufferObject.`'s relative flag used to inline
   `lowercase(x)=="true"` which was *already* correct by accident — now shares the named
   helper instead of a silent duplicate.

All of the above pinned by new/updated gtest cases in `tests/shader_pack_loader_test.cpp` and
`tests/shader_frame_data_test.cpp`. Final run: **108/108 passed**, Release build exit 0,
`build-omega\minecraft_native.exe` fresh.

New memory files written this session (all under
`C:\Users\Eddie\.claude\projects\C--Users-Eddie-Documents-New-project-2\memory\`):
`dryness-halflife-aliases-wetness.md`, `const-directives-parse-preprocessed-source.md`,
`iris-clamps-nothing-but-ao.md`. All indexed in `MEMORY.md`.

## Still open: the outline/edge artifact (the ORIGINAL bug report)

Symptom: hard outlines around blocks and around high-contrast boundaries *inside* a single
texture (birch: where the black bark meets the white bark, within one sprite — ruled out
mipmap bleed, ruled out normal/PBR since it's config-inert for all 3 packs currently in use).

**Leading hypothesis, unconfirmed:** an image-space luma/sharpen filter.
- RVox: `IMAGE_SHARPENING=5` unsharp mask, `program/final.glsl:49-61`, reads `colortex3`
  (same buffer TAA writes), symmetric ±25%-ish ringing. Traced the tap offsets
  (`viewWidth`/`viewHeight`) end to end — they're correct, full render-target size, matches
  Iris `ViewportUniforms.java:27-28`. Not an engine bug if this is it — a pack setting.
- RenderPearl: FidelityFX CAS 1.2, `SHARPNESS=0.3`, `world_default/composite3.csh:161`.
- Both are ON by default in the saved configs. Test: set `IMAGE_SHARPENING=0` in
  `%APPDATA%\.minecraft\shaders\rethinking-voxels_r0.txt`, relaunch, check birch. **Eddie has
  not confirmed running this test yet.**

**Contradicts earlier report:** Eddie said "rvox and renderpearl are fucked too" /
"rvox definitely has it" earlier, then later said "if I switch from complementary to rvox it
fixes the weird outline" (implying Complementary shows it, RVox doesn't) — **never fully
clarified, got cut off mid-sentence.** Do not assume either direction; ask or test both ways.

**Memory `rtv-halo-was-composite-mip-side.md` (2026-08-10, i.e. 2 days before this session)
is superficially similar but a DIFFERENT bug, already fixed:** a "blur halo on
block/terrain silhouettes" under RVox was traced to `CompositeRenderer` leaking mip-filter
state across ping-pong sides (no per-side `mipmapsOnMain`/`mipmapsOnAlt` tracking at the
time). That memory explicitly says it was **NOT** the atlas. This session I already
independently re-audited the composite/ping-pong mip path (CompositeRenderer.cpp:227-231,
329-332; ColorTargets.cpp:505-546; RenderTargets.hpp:210-223) against current Iris source and
confirmed it still matches — per-side tracking present, both sides reset end of frame. **So
if Eddie's current symptom is genuinely atlas-related, it is a NEW/different bug, not a
regression of the 08-10 one** — don't assume the old fix regressed without checking.

## Live thread, interrupted mid-investigation: Complementary's options file doesn't exist

Eddie's exact words: **"Im bascially out of tokens... Then the options file isnt being made
for it because i sure as fuck selected it"** — he selected Complementary as the active pack
at some point, but `%APPDATA%\.minecraft\shaders\` has NO
`ComplementaryReimagined_r5*.txt` file (only `RenderPearl v2.8.0-beta.txt` and
`rethinking-voxels_r0.txt` exist). `options.txt` currently shows
`shaderPack:rethinking-voxels_r0.1-beta9` (RVox is active now).

**Where I got to:** read `PackLifecycle.cpp`. Found the mechanism:
- `Pipeline::select(key)` (line 328) — switches the active pack, writes `options_->shaderPack`
  to `options.txt`. Does **NOT** touch `shaders/<pack>.txt`.
- `Pipeline::setSettings(...)` (line 359) — the **only** place that writes
  `shaders/<pack>.txt` (per memory `shader-options-now-persist.md`, "the single mutation
  point"). It's called from `ShaderpackScreen.cpp` when the user changes a setting in the
  shader options GUI, and only writes if `changed` ends up true after comparing against
  current `pack->settings`.
- `initializePackRuntime` (line 312) seeds `pack.settings` from source defaults on load but
  never calls `setSettings` — so **merely selecting a pack and never opening/touching its
  settings screen writes no file.** That would explain the missing Complementary file
  *if* Eddie only ever selected it without opening its options screen.

**Not yet verified:**
- Does real Iris write `shaderpacks/<pack>.txt` purely on pack *selection*, or also only on
  option *change*? This determines whether "no file until you touch a setting" is a genuine
  divergence or matches Iris. Need to check `Iris.java` / `ShaderPackOptions` — I did not get
  to this before running out of budget.
- Whether Eddie actually opened Complementary's settings screen (would prove the
  select-only theory wrong) — need to ask him, or just fix it to write-on-select if that's
  what Iris does.
- Whether there's a *silent failure* instead (e.g. `setSettings` returning false/early for
  Complementary specifically for some other reason — bad `pack->definition.settings`, an
  exception, filename issue). Complementary's `.properties` files are unusually deeply
  structured; haven't diffed its settings-parsing path at all yet.

## Next steps (in likely priority order)

1. Finish the options-file investigation: check real Iris's write trigger, confirm/deny the
   select-vs-setSettings theory, fix `Pipeline::select` to also persist if that's what Iris
   does (or explain to Eddie why it doesn't need to).
2. Get a real yes/no from Eddie on which pack(s) currently show the outline, and get him to
   actually run the `IMAGE_SHARPENING=0` A/B test — nothing further can be concluded on the
   original bug without that data point.
3. If sharpening is cleared, chase the atlas hypothesis Eddie floated — re-verify
   `uploadStaticMipmapLevels`/`TextureMipmap.cpp` box filter and grid-atlas sampler state
   fresh (I was mid-read of `TextureManager.cpp:105-158` when cut off — nothing conclusive
   found yet, was about to check whether the atlas's actually-generated mip count vs. the
   now-fixed `MC_MIPMAP_LEVEL` macro value could disagree and cause a sampler filtering
   mismatch at high mip levels — untested idea, not confirmed).
4. Loader.cpp/SourceProcessor.cpp orchestration refactor (Eddie's "could be done with iris
   like orchestration... without facade or slop shit" comment) — raised, not started, no
   instruction to act yet.
