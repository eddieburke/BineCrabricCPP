# EXECUTOR P2b — P-IFENGINE + P-MACROS (PASS-1, lane 7) report

Pipeline: multitask PASS-1, lane **P2b** (executor 7 of 7, ran last). Items: **P-IFENGINE**
(unify the two `#if`/`#define` engines) and **P-MACROS** (full Java-textual macro set, Q6).
No builds, no tests, no git run (per execution rules); every line was grepped before editing.

---

## 1. What changed (file:line)

### 1.1 P-IFENGINE — shared conditional-state machine

**NEW `src/net/minecraft/client/render/shaders/ConditionalState.hpp`** (98 lines)
- `ConditionalState` — ONE conditional-stack implementation (`push`/`elif`/`else_`/`endif`/
  `active`) with `enum class Flavor { Glsl, Properties }`.
- `push(bool condition)`: parent = `active()` at push, frame = `{parent, parent && condition,
  parent && condition}` (replicates both engines' "parent&&condition on push").
- `elif`/`else_`: Glsl flavor uses `parentActive && !matched` (keeps an inactive-parent frame
  off); Properties flavor uses the matchedStack-only rule (can flip the top frame's active bit
  on; its `active()` = all-of masks it — preserves the emitted text exactly).
- Top-level sentinel: Properties flavor pushes a never-popped `{true, true, true}` sentinel, so
  an unmatched top-level `#else`/`#elif` disables the rest of the file (the GLSL engine ignores
  those on an empty stack). Both historical behaviors preserved.

**`client/render/shaders/SourceProcessor.cpp` — `normalizePackSource`**
- :193 `ConditionalState stack(ConditionalState::Flavor::Glsl);` replaces the local `CondFrame`
  struct + `std::vector<CondFrame>` + `active` lambda (:192-200 old).
- :231 `stack.push(condition)` for `#if/#ifdef/#ifndef` (the `if(stack.active())` guard that
  skips expression evaluation under an inactive parent is preserved).
- :236 `stack.elif(...)`, :241 `stack.else_()`, :246 `stack.endif()`.
- :250/:284 `stack.active()` replaces the old `active()` for directive gating and the
  non-directive emit decision.
- Still calls the existing `evaluateIfExpression` / `parseDirective` / `isIdentChar` — no third
  parser added (parity-glsl §5.12).

**`client/render/shaderpack/Loader.cpp` — `preprocessProperties`**
- :355 `ConditionalState stack(ConditionalState::Flavor::Properties);` replaces
  `activeStack`/`matchedStack` (:353-357 old).
- :377/:381 `stack.push(...)` for `#ifdef/#ifndef/#if`; :385 `stack.elif(...)`;
  :389 `stack.else_()`; :393 `stack.endif()`. `lineActive()` = `stack.active()` (:355-357) used
  for `#define`/`#undef` gating (:396/:400) and the kept-line test (:409).
- **Linkage change (deviation, see §3):** moved `preprocessProperties` OUT of the anonymous
  namespace — `} // namespace` at :308, reopened `namespace {` at :417 — so the definition at
  :309-416 has external linkage and the new test can call it. No header edit (Loader.hpp is
  outside this lane's ownership; the test forward-declares it).

### 1.2 P-MACROS — full Java-textual macro set

**`client/render/shaders/SourceProcessor.cpp` — `versionPreamble`** (Java `StandardMacros.java`
verified: `IRIS_REQUIRES_SEPARATE_ENTITY_DRAWS` :58, `IRIS_HAS_TRANSLUCENCY_SORTING` :60,
`IRIS_TAG_SUPPORT=2` :61, `MC_NORMAL_MAP`/`MC_SPECULAR_MAP` :101-102,
`MC_RENDER_QUALITY`/`MC_SHADOW_QUALITY=1.0` :103-104)
- :722 `result += "#define IRIS_REQUIRES_SEPARATE_ENTITY_DRAWS\n";` (unconditional)
- :725 `result += "#define IRIS_HAS_TRANSLUCENCY_SORTING\n";`
- :726 `result += "#define IRIS_TAG_SUPPORT 2\n";`
- :745-748 `#define MC_NORMAL_MAP`, `#define MC_SPECULAR_MAP`, `#define MC_RENDER_QUALITY 1.0`,
  `#define MC_SHADOW_QUALITY 1.0` — now unconditional, emitted before the LabPBR block (:751-753,
  left untouched; identical redefinition is legal GLSL and resources/glsl is out of ownership).
- :731 `kCategories` 17 → **19** entries; appends `MOUNTAIN` (index 17) and `UNDERGROUND`
  (index 18), matching `BiomeCategories.java` ordinals (verified 19 entries).
- Comments at :718-721 and :742-744 document the Q6 decision (capability macros defined though
  the engine does not implement them).
- `MC_GLSL_VERSION` unchanged (pack-version semantics; driver-GLSL switch is PASS-2 WI-15).

**`client/render/shaders/PreProcessor.cpp` — `seedMacrosFromDefines`** (:479-490)
- Mirrored the same additions into the fixed seed table so `#if defined(MC_NORMAL_MAP)` evaluates
  identically from a hand-built preamble that omits them:
  `IRIS_REQUIRES_SEPARATE_ENTITY_DRAWS`, `IRIS_HAS_TRANSLUCENCY_SORTING`,
  `IRIS_TAG_SUPPORT = "2"`, `MC_NORMAL_MAP`, `MC_SPECULAR_MAP`,
  `MC_RENDER_QUALITY = "1.0"`, `MC_SHADOW_QUALITY = "1.0"`.

**`client/gl/ShaderBinaryCache.cpp:9`**
- `constexpr std::uint32_t kFileVersion = 2;` (was 1). Mandatory for any macro change
  (parity-glsl §5.4).

### 1.3 Tests (new, self-contained, no GL)

**`tests/if_engine_unified_test.cpp`** — feeds the same directive text to both engines
(`normalizePackSource` vs `preprocessProperties`, content-lines compared) and asserts identical
output across a battery of `#if/#elif/#else/#endif/#ifdef/#ifndef/#define/#undef` incl. nested
and dead-region cases; direct `ConditionalState` unit tests for both flavors, the sentinel, and
the one observable divergence (unmatched top-level `#else`: GLSL keeps, .properties disables).

**`tests/macro_parity_test.cpp`** — pins the full textual macro set against a
`StandardMacros.java` fixture in `versionPreamble`; asserts `CAT_MOUNTAIN 17`/`CAT_UNDERGROUND 18`;
asserts `seedMacrosFromDefines` mirrors the set and that `#if defined(MC_NORMAL_MAP) &&
MC_RENDER_QUALITY == 1.0 && IRIS_TAG_SUPPORT == 2` evaluates true; behaviorally pins the cache
format bump (a version-1 cache file is rejected, a version-2 file loads, store→tryLoad roundtrip).

---

## 2. Correctness argument for the unified machine

- For every well-formed construct the two engines already agreed; I proved the frame states are
  equivalent and the effective `active()` (GLSL top-frame, transitive vs Properties all-of)
  coincides on all inputs. The only observable historical divergence is the **top-level
  unmatched `#else`/`#elif`** (GLSL no-op vs Properties sentinel-disable), preserved via the
  sentinel + `Flavor` (documented in ConditionalState.hpp and covered by
  `IfEngineUnified.PreservesTopLevelElseDivergenceBetweenEngines`).
- Both engines keep computing their own conditions (`evaluateIfExpression`, `defined` checks)
  and only the stack pairing is shared — no third parser (parity-glsl §5.12).
- `normalizePackSource`'s `#elif` now evaluates `evaluateIfExpression` unconditionally instead of
  only inside the `parentActive && !taken` branch; `evaluateIfExpression` is a pure function with
  no side effects, so frame outcomes are byte-identical to the old engine.

## 3. Deviations / decisions

1. **`preprocessProperties` linkage** (Loader.cpp:308/:417): it was inside the anonymous
   namespace (internal linkage), unreachable from a test. Moved it to the outer
   `net::minecraft::client::render` namespace (closed/reopened the anonymous namespace around it)
   so the required cross-engine test can call it. Loader.hpp was NOT touched (outside ownership);
   the test forward-declares the function.
2. **Indentation:** RULES FOR AGENTS.md says 2-space; the existing SourceProcessor.cpp uses a
   1-space indent. New code in SourceProcessor.cpp matches that file's 1-space style to minimize
   diff noise; the new `ConditionalState.hpp` uses 2-space per the RULES.
3. **LabPBR block kept:** `mc_normal_specular_map.glsl` is still emitted for LabPBR packs after
   the unconditional defines (identical empty-body redefinition, legal GLSL); resources/glsl is
   outside this lane's ownership and parity-glsl §5.8 pins snippet text.
4. **Golden-compare on shipped packs:** could not be executed (no build/test allowed). Instead
   regression coverage rests on the unchanged existing tests (`shader_pack_loader_test`,
   `shader_frame_data_test`, `glsl_snippets_test`) plus the new cross-engine identity battery and
   ConditionalState flavor tests; the compile-fixer should run the full shader suite.
5. **Line-drift:** plan citations re-verified against the live tree before editing
   (`normalizePackSource` :184-311, `versionPreamble` :709-778, `kCategories` :745-748,
   `preprocessProperties` :307-435, conditional block :353-416, `seedMacrosFromDefines` :469-507,
   `kFileVersion` :9). Post-edit line numbers are reported in §1.

## 4. Notes for the compile-fixer

- `ConditionalState.hpp` is header-only; both `SourceProcessor.cpp` and `Loader.cpp` include it.
- Both new test files are pre-registered (commented) in CMakeLists under PASS-1; uncomment/wire
  per WI-T once files are present.
- Run the focused set: `if_engine_unified_test`, `macro_parity_test`, plus `shader_pack_loader_test`,
  `shader_frame_data_test`, `glsl_snippets_test`, `shader_gl_integration_test`, `custom_uniforms_test`.
