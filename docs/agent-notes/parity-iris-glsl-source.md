# PARITY AUDIT — Iris GLSL Source-Level Parity (preprocessing / macros / source transform)

Auditor role: **IRIS-PARITY AUDITOR** (review-only; no edits, no builds).
Date: 2026-08-01. Target: the planned multithreading/main-thread refactor (plan-initial.md, WI-1…WI-14).
Scope: GLSL source-level parity between the C++ port (`src/net/minecraft/client/render/shaders/*`,
`pipeline/*`, `gl/*`) and the Java Iris 26.1 mirror (`third_party/mcp/iris`).

> **Prior-notes warning.** The task brief referenced `docs/agent-notes/glsl.md`, `dualpaths.md`,
> `lua-iris-dualpaths.md`, `deabstract.md`, `dealias.md`, `passindex.md`. **None of these files exist
> in the current tree** (verified by glob). One surviving pointer: `GlslSnippets.hpp:13` still comments
> "see docs/agent-notes/glsl.md". This audit was therefore done from source directly; if those notes
> existed in an older working tree their claims were not re-confirmed.

> **Reference-tree note.** The repo contains two byte-identical mirrors of Iris:
> `third_party/mcp/iris` (615 files) and `third_party/iris/common/src/main/java/net/irisshaders/iris`
> (615 files; `IrisFunctions.java` hashes equal). All citations below use `third_party/mcp/iris`.

---

## 1. Verdict summary

1. **Preprocessor fidelity is a deliberate "good-enough-for-legacy" approximation, not JCPP parity.**
   Java uses the **JCPP** C-preprocessor (`JcppProcessor.java:12-71`), which implements full C macro
   semantics (`##`, `#`, `defined()`, string literals, `#error`, balanced-`#if` diagnosis, correct
   function-like-macro expansion). The C++ port re-implements a C-subset by hand
   (`PreProcessor.cpp:43-290` + `SourceProcessor.cpp:184-311`): no `##`/`#`, no string/char-literal
   awareness in directive scanning (`lineForDirectiveParse`, `PreProcessor.cpp:26-41`), no
   multi-line block-comment spanning, unconditional parenthesization of function-like args in `#if`,
   whitespace before `(` allowed for function-like macros (wrong per C), unbalanced `#if`/`#endif`
   silently tolerated (Java throws), and `#error` passed through to the GLSL compiler (Java aborts).
   Severity is MED for the legacy pack class the port targets; parity breaks only on packs that
   actually use those C constructs inside `#if`.

2. **The biggest intentional divergence is the dialect fork, and it is load-bearing.** Java Iris 26.1
   forces every shader to **GLSL 330 core** (`TransformPatcher.java:146-197`) and rewrites
   `attribute`/`varying`/`gl_*` builtins (`CommonTransformer.upgradeStorageQualifiers` etc.). The C++
   port **keeps GLSL 120** for legacy packs and only core-ifies sources that declare their own
   `#version 130+` (`sourceCompilesAsModern`, `SourceProcessor.cpp:535-544`). This is why several
   Java-26.1-only transforms (fragment `gl_TexCoord`/`gl_Color`/`gl_FragColor` rewriting,
   `CommonTransformer.java:182-232`) have **no C++ counterpart** — they are unnecessary in 120 but
   are **missing for 130+ packs**, which is a real gap (a 130+ pack using `gl_TexCoord` compiles in
   Java, fails in C++).

3. **Macro-set parity has concrete gaps** (see §2 rows M1–M8): `IRIS_REQUIRES_SEPARATE_ENTITY_DRAWS`,
   `IRIS_HAS_TRANSLUCENCY_SORTING`, `IRIS_TAG_SUPPORT`, `MC_RENDER_QUALITY`, `MC_SHADOW_QUALITY` are
   never defined in C++; `MC_NORMAL_MAP`/`MC_SPECULAR_MAP` are only defined for LabPBR packs (Java
   defines them unconditionally, `StandardMacros.java:101-102`); `MC_GLSL_VERSION` is the *shader's*
   version in C++ but the *driver's* GLSL version in Java (`StandardMacros.java:53`); `CAT_MOUNTAIN`
   and `CAT_UNDERGROUND` are missing (`BiomeCategories.java` has 19, C++ has 17).

4. **`iris_*` GLSL helper functions: the brief's premise is wrong.** `parsing/IrisFunctions.java`
   (43 KB) is **not** a set of injected GLSL functions. It is the **CPU-side function library for
   custom-uniform expression evaluation** (`CustomUniforms.java:21,46` uses
   `IrisFunctions.functions` as a `FunctionResolver`). The C++ 1:1 counterpart is
   `CustomUniforms.cpp` (parser + `callFunction`, `CustomUniforms.cpp:551-699`), which is a faithful
   port (see §2 rows U1–U4). The actual GLSL snippets the engine injects live in
   `src/net/minecraft/client/render/shaders/glsl/*.glsl` (embedded into the executable by CMake)
   and match their Java transform sources well (`MC_HAND_DEPTH 0.125` ==
   `HandRenderer.java:43`; `iris_FogFragCoord`/`iris_FrontColor` == `CommonTransformer.java:160-180`).

5. **`const …Format = RGB8;` strip has no visible Java equivalent.** `isBufferFormatDirective` +
   `stripFormatDirectives` (`IncludeResolver.cpp:10-22,39-42`, on at `Compiler.cpp:142`) removes
   `const <type> colortexNFormat/shadowcolorNFormat = <ident>;` lines from every stage before compile.
   Real packs ship these *uncommented* (`shaders/SEUS PTGI Iris/shaders/deferred10.fsh:18`), and
   they are a GLSL compile error if left in, so Java must remove them somewhere — but this mirror
   only *parses* them (`ConstDirectiveParser.java:7-177`, `ProgramDirectives.java:115-119`) and shows
   no source removal. **Open item the refactor must close** (empirically or by finding the Java
   removal site): stripFormatDirectives is load-bearing for the port; do not touch it without proof
   of the Java-side behavior.

6. **Threading status in this subsystem today:** preprocessing/source derivation runs **on the main
   thread** (`PackCompiler::compile` called from `Pipeline::programFromPack`/`programFromPack` sites;
   `PackCompiler::prewarm` from `PackManager::prewarmPacks` on main; `poll` on main). The **only**
   off-thread work is GL compile+link+binary extraction inside `ShaderCompileService` workers
   (`ShaderCompileService.cpp:224-249`), which consumes already-prepared immutable strings. This is a
   clean seam and the refactor (WI-8) must keep it: **preprocessing must remain a pure, single-
   threaded-or-parallel-deterministic derivation; GL queries used by `versionPreamble` must be
   captured on the GL thread before any worker runs.**

---

## 2. Discrepancy table

Legend: MED/HIGH = can change compiled GLSL text or `#if` outcomes for real packs; LOW = edge case
or internal-only. Line refs verified against the working tree.

### 2A. Preprocessor fidelity (Java JCPP vs C++ hand-rolled)

| # | Area | C++ file:line | Java Iris file:line | Difference | Sev | Parity impact |
|---|---|---|---|---|---|---|
| P1 | `#version`/`#extension` hoisting | `normalizePackSource` strips `#version` (emit blank, `SourceProcessor.cpp:287-290`) and hoists active `#extension` into a prefix string (`:291-297`, return `:310`) | JCPP swaps `#version`→`#warning IRIS_JCPP_GLSL_VERSION` and `#extension`→`..._EXTENSION` (`JcppProcessor.java:28-29`), listener re-emits them as leading lines (`GlslCollectingListener.java:18-27`), output = hoisted lines + body (`JcppProcessor.java:66-68`) | Equivalent *order* (version first, then extensions, then body) but C++ puts the engine `versionPreamble` (which carries `#version` and all engine `#define`s) *before* the hoisted extensions, i.e. version → defines → extensions → body, whereas Java is version → extensions → body (env defines are JCPP macros, not text). Both are legal GLSL ordering. | LOW | None for well-formed packs; differs only in the text the driver sees. Keep as-is; do not "fix" to match Java textually (would change every compiled source = cache invalidation). |
| P2 | Full C preprocessor vs subset | `PPExpressionEval` (`PreProcessor.cpp:43-290`) + `expandIfExpression` (`:331-418`): supports `defined()`, `&&\|\|!?:`, bitwise, shifts, comparisons, ternary, function/object macros. **No `##`, no `#`, no string literals in expressions** | JCPP = complete C preprocessor (`org.anarres.cpp.Preprocessor`; `JcppProcessor.java:36-64`) | Missing token-paste/stringize (irrelevant in `#if` text), and function-like macro body substitution parenthesizes every argument: `#define F(x) x*2`, `F(1+2)` → C++ `(1+2)*2`=6 vs C/JCPP `1+2*2`=5. Also whitespace between macro name and `(` **is allowed** in C++ (`SourceProcessor` expansion path `PreProcessor.cpp:361-367`) but is *not* an invocation in C. | MED | `#if` arithmetic diverges for packs using function-like macros with expression args or `F (x)` spacing. Rare but real. |
| P3 | `#if` integer arithmetic | `toInt` = `long long`, division by zero yields 0 (`PreProcessor.cpp:75-77,219-224`) | JCPP long integer arithmetic; `/0` is an error/warning | Div-by-zero silently yields 0 in C++, errors in JCPP; no 64-bit overflow guards | LOW | Only packs with buggy `#if` expressions. |
| P4 | Unbalanced `#if`/`#endif` | unmatched `#endif` silently pops empty stack (`SourceProcessor.cpp:266-270`); unclosed frames silently dropped at end | JCPP throws `LexerException`/reports | Error-reporting parity only; well-formed packs identical | LOW | None for valid packs. |
| P5 | `#error`, `#warning`, `#custom`, `#moj_import` | `#error` falls through to `emit(logical)` → reaches GLSL compiler (`:302-303`); `#warning`/`#custom`/`#moj_import` dropped (`:298-301`) | JCPP: `#error` throws; `#warning` logged via listener; `#moj_import` not a thing (TransformPatcher *forbids* non-extension/pragma directives, `TransformPatcher.java:74-83`) | C++ passes `#error` to the GL driver (differs in *where* the failure surfaces); silently swallows `#warning` diagnostics | LOW | Failure locality only. |
| P6 | Directive-line comment/string scrub | `lineForDirectiveParse` strips `//` and single-line `/* */` but **not** strings and **not** multi-line `/*` (breaks at line end, `PreProcessor.cpp:26-41`) | JCPP lexes strings and multi-line comments correctly | `#define X "a//b"` → C++ truncates at `//`; a directive wrapped by a multi-line block comment *is* processed in C++ but not in Java; `#if A /*…` spanning lines misparsed | MED | Legacy packs occasionally put `//`-bearing strings or multi-line comment blocks around directive blocks (RenderPearl's `directive.glsl` uses a big `/*…*/` block — currently safe because its directives are fully inside the block, but the parser is one character away from mis-processing). |
| P7 | `#include` resolution relative-to-pack | `IncludeResolver.cpp:24-72`: relative → `parent/name`, leading `/` → `shaders/<name>`, `<…>` **supported**, bare-name include → throws "malformed"; cycle → `IncludeResolveError` | `FileNode.findIncludes` (`FileNode.java:33-65`): trimmed line starts with `#include`, strips quotes only (**no `<…>`**, no bare-name tolerance), resolves via `AbsolutePackPath` (root=`/`=shaders dir); cycles detected in `IncludeGraph.detectCycle` (`IncludeGraph.java:163-243`) and **pack load fails** | Same relative/absolute semantics; C++ is more permissive (`<…>` works, trailing junk tolerated) and throws on missing/empty included file (Java records a `RusticError`, pack load fails later — same net effect). Java **cannot** resolve `#include <x>` (path would contain literal `<`/`>`). | MED | Extra permissiveness is a superset; but **both** resolve includes *before* `#if` evaluation — includes inside `#if 0` are still inlined. This matches Java (`IncludeGraph.java:204-208` "any form of #include guard will not work") and must be preserved. |
| P8 | `const …Format` strip | `isBufferFormatDirective` (`IncludeResolver.cpp:10-22`), applied at `Compiler.cpp:142` (every stage) | `ConstDirectiveParser` parses but the mirror shows **no removal** from source (`ConstDirectiveParser.java:7-122`, `ProgramDirectives.java:115-119`) | C++ removes the lines (required for GLSL 120 compilation of packs like SEUS PTGI `deferred10.fsh:18`). Java equivalent not located in the mirror — see Verdict #5 | MED/HIGH (unverified) | If the port stopped stripping, 120 packs break. Must be kept, and the Java removal path confirmed. |
| P9 | Option rewriting timing | Done per-included-file inside `PackCompiler::resolveIncludes` via `PackLoader::rewriteOptions` (`Compiler.cpp:135-139`) | Java applies option transforms to the `IncludeGraph` lines before inlining (`ShaderPackOptions`, `IncludeGraph.map`, `FileNode.java:79-97`) | Same relative order (include→rewrite→preprocess). C++ rewrites each included file's text independently then inlines; Java rewrites each FileNode's lines then inlines | LOW | Equivalent for line-based options. |

### 2B. Macro set parity

| # | Area | C++ file:line | Java Iris file:line | Difference | Sev | Parity impact |
|---|---|---|---|---|---|---|
| M1 | `MC_GLSL_VERSION` | `#define MC_GLSL_VERSION <pack's GLSL version>` (120/330/…) `SourceProcessor.cpp:737` | `define(…, "MC_GLSL_VERSION", getGlVersion(GL_SHADING_LANGUAGE_VERSION))` = driver GLSL, e.g. `460` (`StandardMacros.java:53,205-228`) | **Semantically different.** Pack `#if MC_GLSL_VERSION >= 400` takes the *false* branch for a 120 pack in C++, the *true* branch in Java. | HIGH | Changes `#if` selection in gbuffers/composite code for any pack branching on GLSL version. |
| M2 | `MC_NORMAL_MAP` / `MC_SPECULAR_MAP` | Only for LabPBR packs (`SourceProcessor.cpp:756-760` via `mc_normal_specular_map.glsl`) | Defined **unconditionally** (`StandardMacros.java:101-102`) | Non-LabPBR pack checking `#ifdef MC_NORMAL_MAP` gets FALSE in C++, TRUE in Java | MED | Sampler-dependent feature code disabled in C++. |
| M3 | `IRIS_REQUIRES_SEPARATE_ENTITY_DRAWS` | not defined anywhere (grep: 0 hits in shaders/*) | `StandardMacros.java:58` | Missing macro | MED | Packs that branch on it for entity pass behavior. |
| M4 | `IRIS_HAS_TRANSLUCENCY_SORTING`, `IRIS_TAG_SUPPORT` | not defined | `StandardMacros.java:60-61` (`IRIS_TAG_SUPPORT=2`) | Missing | MED | Newer pack feature gates. |
| M5 | `MC_RENDER_QUALITY`, `MC_SHADOW_QUALITY` | not defined | `StandardMacros.java:103-104` (=`1.0`) | Missing | MED | Packs that scale by quality; also the renderer has no notion of these — port must decide. |
| M6 | `CAT_MOUNTAIN`, `CAT_UNDERGROUND` | `kCategories` has 17 (`SourceProcessor.cpp:745-748`) | `BiomeCategories.java` has 19 (adds MOUNTAIN, UNDERGROUND) | Two missing CAT_ macros; also shifts indices only for those two | LOW | Rarely referenced. |
| M7 | `IRIS_FEATURE_*` emission | Emits for **required** AND optional features (`SourceProcessor.cpp:771-774`), plus 4 hard-coded (`PreProcessor.cpp:475-478`: SSBO, COMPUTE_SHADERS, PER_BUFFER_BLENDING, DIRECT_IMAGE_ACCESS) | `IRIS_FEATURE_<flag>` added for all *usable* FeatureFlags **only into the shaders.properties set** (`ShaderPack.java:170-175`); the **GLSL** JCPP env set gets standard + `COLOR_SPACE_*` + **optional** features only (`:238-252`, used at `:285-317`). Required features are *not* in the GLSL defines. `IRIS_FEATURE_DIRECT_IMAGE_ACCESS` has **no** Java FeatureFlags entry | C++ defines required-feature macros in GLSL that Java does not; C++ adds a non-Java macro `IRIS_FEATURE_DIRECT_IMAGE_ACCESS`; Java's `IRIS_FEATURE_<optional>` set is the same list when the pack lists them | MED | `#ifdef IRIS_FEATURE_SSBO` in a pack that *requires* ssbo: defined in C++, undefined in Java. Usually the "on" branch is wanted, but it is not literal parity. |
| M8 | `IRIS_VERSION` | `kIrisApiSemver = "1.9.2"` → `10902` (`SourceProcessor.cpp:731-733,739`) | `getFormattedIrisVersion` from the *build* semver (`StandardMacros.java:149-174`) | Port claims a fixed API version 1.9.2 rather than a real build. Any pack gating `#if IRIS_VERSION >= …` is comparing against a number the port chose | MED | Feature gates keyed to an invented version claim; document as the port's contract. |
| M9 | `MC_VERSION` | `formatVersion122("1.7.3")` → `10703` (`SourceProcessor.cpp:730-732`) | `formatVersionString(Iris.getReleaseTarget())` (`StandardMacros.java:130-141`) | Port-appropriate (b1.7.3); Java would emit its own MC version. Semantics of the 122 encoding match (`StandardMacros.java:183-195` vs `formatVersion122` `SourceProcessor.cpp:41-75`) | INFO | Correct for this game. |
| M10 | `MAX_COLOR_BUFFERS` | `maxColorBuffers()` = clamped `GL_MAX_COLOR_ATTACHMENTS` (`SourceProcessor.cpp:32-39,740`) | `IrisLimits.MAX_COLOR_BUFFERS = 32` constant (`StandardMacros.java:59`, `IrisLimits.java:11`) | C++ = hardware query, Java = constant 32 | MED | On drivers reporting <32 attachments (common), C++ emits a smaller MAX_COLOR_BUFFERS; packs iterate to `MAX_COLOR_BUFFERS`. |
| M11 | `MC_GL_VENDOR_*` / `MC_GL_RENDERER_*` | `driverPreamble` (`SourceProcessor.cpp:77-117`) | `getVendor`/`getRenderer` (`StandardMacros.java:268-316`) | Same prefix sets incl. APPLE | LOW | Parity. |
| M12 | `MC_HAND_DEPTH` | `#define MC_HAND_DEPTH 0.125` (`mc_hand_depth.glsl`) | `HandRenderer.DEPTH = 0.125F` (`HandRenderer.java:43`) | Identical | — | Parity. |
| M13 | `MC_RENDER_STAGE_*` | 24 stages (`SourceProcessor.cpp:761-770`) | `getRenderStages` over `WorldRenderingPhase` (`StandardMacros.java:346-352`) | Same idea, values = ordinal; C++ stage set is b1.7.3-appropriate | INFO | Port-appropriate. |
| M14 | `MC_<extension>` | `#define MC_<ext>` for every supported extension (`SourceProcessor.cpp:775`) | `MC_<ext>` set from `getGlExtensions` (`StandardMacros.java:97-99,325-343`) | Parity (both prefix GL_ extensions with MC_) | LOW | Parity. |

### 2C. Source transforms / dialect

| # | Area | C++ file:line | Java Iris file:line | Difference | Sev | Parity impact |
|---|---|---|---|---|---|---|
| T1 | Dialect fork | `sourceCompilesAsModern` + dual paths in `lowerVertexSource`/`lowerFragmentSource` (`SourceProcessor.cpp:535-544,569-639,641-692`): 120 keeps `attribute`/`varying`/`gl_*`; 130+ uses in/out | Always forces `#version 330 core` (`TransformPatcher.java:146-176`) and upgrades `attribute`→`in`, `varying`→in/out (`CommonTransformer.java:136-150`) | C++ is a *superset* supporting both dialects; Java has one path. For 120 packs C++ is more faithful to OptiFine; for 130+ packs the C++ modern path is a **partial** reimplementation (see T2–T5) | HIGH | This is the port's core deliberate divergence. The refactor MUST NOT unify/collapse the two dialects. |
| T2 | Fragment `gl_TexCoord` / `gl_Color` | **not handled** in C++ `lowerFragmentSource` | Java renames `gl_TexCoord`→`irs_texCoords` + `in vec4 irs_texCoords[3];` and `gl_Color`→`irs_Color` + `in vec4 irs_Color;` (`CommonTransformer.java:189-197`) | Missing in the C++ **modern** path. A 130+ core pack using `gl_TexCoord`/`gl_Color` in fragment compiles under Java, fails under C++. (In 120 it is a built-in varying so it works — masking the gap.) | MED | 130+ packs using legacy fragment varyings. |
| T3 | Fragment `gl_FragColor` → `gl_FragData[0]` and `gl_FragData[i]`→`layout(location=i) out vec4 iris_FragDatai` | C++ does not rewrite `gl_FragColor`/`gl_FragData` (only reads them to decide alpha-test injection, `SourceProcessor.cpp:651-692`) | `CommonTransformer.java:182-225` | C++ relies on the pack staying in 120 (where `gl_FragColor`/`gl_FragData` are legal). Modern-path packs using `gl_FragData[i]` lose the Java layout-location semantics; C++ instead relies on `setDrawBufferColortexIndices` (`Compiler.cpp:214-218`). | MED | 130+ packs writing `gl_FragData[i]`. |
| T4 | `gl_FogFragCoord` / `gl_FrontColor` | Injected **only when referenced and only when modern** (`SourceProcessor.cpp:618-635,663-671`) vs Java **always** (`CommonTransformer.java:160-180`) | Java unconditional | Conditional-vs-unconditional: harmless difference in 120, but C++ skips the `out float iris_FogFragCoord;` plumbing for 120 packs that *do* use it — in 120 `gl_FogFragCoord` is legal so it works; behavior parity holds. | LOW | None for 120; missing for 130+ (see T2). |
| T5 | `ftransform` / legacy matrix/coord rewrites (vertex) | `lowerVertexSource` legacy branch (`SourceProcessor.cpp:569-607`): `ftransform()`→`(projectionMatrix*modelViewMatrix*vec4(vaPosition+chunkOffset,1.0))`, `gl_Vertex`, `gl_MultiTexCoord0/1/2`, `gl_Color`, `gl_Normal`, matrices | Vanilla/Sodium transformers (`VanillaTransformer.java:74-114,180-334`, `SodiumTransformer.java:36-117`) | Same intent; C++ is the OptiFine-flavoured rewrite keyed to its own `va*` vertex contract; Java rewrites to `iris_Position`/`iris_Color` etc. Both are engine-internal contracts — **parity here is that each engine's own rewrite is self-consistent**, not byte-equal. The C++ vertex attribute bindings (`ShaderProgram.cpp:146-157`) differ from Java's (`ProgramCreator.java:19-24`): C++ vaPosition=0..vaUV2=5,mc_Entity=6,mc_midTexCoord=7,at_tangent=11,mc_chunkFade=12; Java Position=0,UV0=1,mc_Entity/iris_Entity=11,mc_midTexCoord=12,at_tangent=13,at_midBlock=14. | LOW | Only matters if a pack hard-codes `layout(location=…)` on attributes (rare). Internal consistency is what counts. |
| T6 | `patchMultiTexCoord3` | gated on "not declared anywhere" (`SourceProcessor.cpp:550-560`) | Java also requires `!root.identifierIndex.has("mc_midTexCoord")` (`CommonTransformer.java:120-122`), injects `attribute vec4 mc_midTexCoord;` | Parity | — | Parity. |
| T7 | `replaceGlMultiTexCoordBounded(4,7)` | `SourceProcessor.cpp:564-567` | `CommonTransformer.java:380-392` | Parity | — | Parity. |
| T8 | Alpha-test injection | `lowerFragmentSource` `alphaTestRef` uniform + `alpha_test_discard.glsl` (`SourceProcessor.cpp:681-691`), gated by `programGetsCompatAlphaTest` (`:641-649`) | Java: `uniform float iris_currentAlphaTest;` + `AlphaTest.toExpression(...)` in main (`CommonTransformer.java:227-231`); gating via `AlphaTest != ALWAYS` | Same mechanism, **different uniform name** (`alphaTestRef` vs `iris_currentAlphaTest`) and different insertion point (C++ appends before closing `}` of main; Java appends to main body). Internal-consistent, but the fragment code text differs. | LOW | Internal contract; the engine uploads its own name. |
| T9 | `sildursWaterFract` patch | **not present** in C++ | `CompatibilityTransformer.java:159-162` (`fract(worldpos.y + 0.001)`→`0.01`) | Missing micro-patch | LOW | Only affects a known bad snippet; without it that pack's water seam shows. |
| T10 | Default composite vertex shader | `default_composite.vsh.glsl` = modern `in vec3 vaPosition; in vec2 vaUV0; … out vec2 texcoord;` (`SourceProcessor.cpp:518-520`, resource) | Java legacy default vsh = `#version 120` with `varying vec4 irs_texCoords[3]; varying vec4 irs_Color;` (`ProgramSet.java:127-144`) | **Different default vertex outputs** when a composite/final program has only a fragment shader. Fragment shaders expecting `irs_texCoords`/`irs_Color` (old composite style) get nothing in C++. | MED | Legacy composite-only programs. |
| T11 | `mc_chunkFade` attribute | `injectChunkFadeAttribute` (`SourceProcessor.cpp:456-473`): gbuffers_terrain → `in float mc_chunkFade;`, else `const float mc_chunkFade = -1.0;` | Vanilla path: `const float mc_chunkFade = -1.0;` (`VanillaTransformer.java:30`); Sodium: attribute via `u_SectionTimeInfo` | Port-appropriate (no Sodium); the terrain `in` attribute is the C++ analog of Sodium's fade. | INFO | Port-specific FADE_VARIABLE support. |
| T12 | `gl_FragDepth` passthrough | `appendBeforeMainClose` with `gl_frag_depth_passthrough.glsl` when `gl_FragDepth` declared but never written (`SourceProcessor.cpp:672-677`) | **no Java equivalent** found in mirror | C++-only source injection | LOW | Extra; harmless (initializes an otherwise-unwritten output). |

### 2D. Custom uniform evaluation (IrisFunctions.java counterpart)

| # | Area | C++ file:line | Java Iris file:line | Difference | Sev | Parity impact |
|---|---|---|---|---|---|---|
| U1 | Function set | `callFunction` `CustomUniforms.cpp:551-699`: radians/degrees/torad/todeg, sin/cos/tan/asin/acos/atan/atan2, pow/exp/exp2/exp10/log/log2/log10/sqrt, abs/sign/signum/floor/ceil/frac, min/max/clamp/mix/edge, fmod, random/randomInt, if, between, equals, in, smooth, vec2/3/4, swizzle, matrix-elem | `IrisFunctions.java` registrations (`:78-1212`): same set incl. variadic min/max (3–16 args `:298-370`), `if` vararg pairs (`:471-497`), `in` vararg (2–32 `:694-719`), `smooth` 1–4 args (`:499-674`), `round` **commented out on both sides** (`:274-277`) | **C++ `min`/`max` only take exactly 2 args** (`CustomUniforms.cpp:599-620`) — Java supports `min(a,b,c,…)`. C++ `if` supports pairs + else (same intent; Java's fake-vararg has a re-evaluation quirk). | MED | Packs calling `min(x,y,z)` get a 2-arg result in C++. |
| U2 | `smooth` state/RNG determinism | `smoothStates_`/`autoSmoothId_` members + `static thread_local std::mt19937 rng` (`CustomUniforms.cpp:1157-1167`) | Java per-function `SmoothFloat` instances + shared `Random` (`IrisFunctions.java:426,512…`) | C++ `random()` differs per thread (thread_local seed); Java uses one shared `Random` per run. If `evaluate` ever runs off-main/parallel, `random()` custom uniforms become nondeterministic. | MED (threading) | See §4 hazard 4. |
| U3 | Matrix element access | `gbufferProjection` etc. + `m.c.r` (`CustomUniforms.cpp:240-282,751-761`) | stareval matrix accessors (`IrisFunctions.java:796+`) | Parity | — | Parity. |
| U4 | `lookupConstant` BIOME_/CAT_ maps | `CustomUniforms.cpp:193-239` | `IrisDefines.createIrisReplacements` + `BiomeUniforms.getBiomeMap` (`IrisDefines.java:28-33`) | C++ hard-codes 13 b1.7.3 biomes + 15 cats (subset of emitted macros); Java derives from the live registry | LOW | Only affects `customuniform`/`customvariable` expressions referencing biome ids. |

---

## 3. Dual-path inventory (GLSL source derivation / shader selection forks)

Verified present in the current tree. The refactor must keep each fork's two sides **byte-for-byte
in lockstep**, or explicitly route both sides through one implementation.

| # | Fork | C++ file:line | What diverges | Refactor consequence |
|---|---|---|---|---|
| DP-1 | **Dialect: GLSL 120 vs 130+/core** | `sourceCompilesAsModern` `SourceProcessor.cpp:535-544`; `lowerVertexSource` `:569-639`; `lowerFragmentSource` `:641-692` | `attribute`/`varying` vs `in`/`out`; which snippets inject (`iris_fog_frag_coord_*`, `iris_front_color_global`) | Highest-risk fork. One code path, two dialects. Do NOT collapse; do NOT let the "modern" tests drift from the 120 outputs. WI-8 preprocessing threads must see the same `modern` value as today. |
| DP-2 | **Compute vs raster program** | `prepareProgram` compute branch `Compiler.cpp:41-51` vs raster `:53-94`; `versionPreamble(compute=true)` `SourceProcessor.cpp:709-717` (forces ≥430, no dialect split) | Different version floor, different stage enum, different compile entry (`getFromComputeSource*`) | Keep `versionPreamble(compute=true)` separate from raster; compute has no `lower*` transform. |
| DP-3 | **Dimension folders** | `dimPrefix` + fallback `Compiler.cpp:120-143`; `PackDefinition::dimensionDefinitions` `Pack.hpp:220` | Same program name resolves to `shaders/<dim>/…` first, else `shaders/…` | Source resolution is per-dimension; any caching of prepared source must key on the resolved dimension path (it already does — the resolved `path` is the include key). |
| DP-4 | **Async vs sync compile** | `PackCompiler::compile` vs `compileSync` `Compiler.cpp:144-155` (`synchronous` flag `:160-233`); `Pipeline::programFromPack` vs `programFromPackSync` `Pipeline.cpp:686-702`; `ShaderCompileService::submit` `:109-116` vs `compileBlocking` `:144-177` | Async returns nullptr + frame-poll; sync waits / runs on current context | `programFromPackSync` is currently **dead code** (declared `Pipeline.hpp:92`, defined `Pipeline.cpp:695-702`, no callers). WI-8 wants compile to be worker-only; either delete the dead sync path or keep it ONLY as the `!started()` fallback (`ShaderCompileService.cpp:157-159`) and guarantee both paths produce byte-identical `PreparedProgram`. |
| DP-5 | **ColorWheel `clrwl_*` pack path** | `mergeColorWheelMaterial` `ColorWheelMerge.cpp:53-89`; `appendColorWheelMacros` `:57-66`; shadow/fallback skip `PassIndex.cpp:179`, `Pipeline.cpp:744` (`worldProgram` returns nullptr for `clrwl_`) | A whole parallel program namespace with its own bridges/macros | `clrwl_` programs get a different preamble (`COLORWHEEL_VERSION`, stage defines) and different source merging; must remain gated on the `clrwl_` prefix and never leak into normal packs. |
| DP-6 | **Interface (base) pack vs world (active) pack** | `interfaceProgramsActive()` `Pipeline.cpp:712`, `Pipeline.hpp:101-103`; uniform uploader `Manager.cpp:92-135`; watcher/reload | GUI/hand draws bind the base pack and skip shadow textures/atlases; world draws use the active pack + shadow + atlas | Both packs run through the same `PackCompiler`; the fork is at *selection*, not derivation. Keep both compiled from the same source-derivation function. |
| DP-7 | **Lua mod render path vs Iris pipeline path** | Lua hooks: `GameRenderer.cpp:887,893`, `LuaDirectHooks`, `ModModels.cpp:626` (`drawLuaBlockWorld` on mesh workers → `setAlphaTestRef`), `LuaModEntityRenderer.cpp:11-34` | Lua mods draw through the vanilla/tessellator path (fixed-function-ish) and touch GL *state* (`g_alphaTestRef`) but do **not** generate GLSL source | This is the WI-5 hazard (worker→global alpha ref). The refactor must snapshot alphaTestRef per mesh job and keep the GLSL pipeline unaffected. No source-level fork here, but a state-level fork that must not leak into the shader uniform path. |
| DP-8 | **Two independent `#if`/`#define` engines** | GLSL: `normalizePackSource` `SourceProcessor.cpp:184-311` (CondFrame stack); Properties: `preprocessProperties` `Loader.cpp:307-438` (activeStack/matchedStack) | Both evaluate `#if/#elif/#else/#endif/#ifdef/#ifndef/#define/#undef` on the same `evaluateIfExpression` core but with **two different conditional-state machines** and different directive defaults | If a fix goes into one (e.g. `#elif` handling) and not the other, `.properties` and GLSL diverge. Consider unifying on one state machine. |
| DP-9 | **Program fallback/shadow selection** | `irisShadowProgramForGbuffers` + `resolveIrisShadowProgramKey` `PassIndex.cpp:178-212`; `programFallbackKey` `:223-261`; disabled-chain walk `Pipeline.cpp:746-772` | gbuffer→shadow mapping; ProgramId fallback chains; disabled-program fallback | These are pure key-resolution maps mirroring `ProgramId.java`/`ProgramFallbackResolver.java`; they must stay in lockstep with the two C++ implementations (they already mirror the Java chains; do not hand-edit one without the other). |

---

## 4. Threading / parallelization hazards (this subsystem)

Current seam: **derivation on main, GL compile on workers.** Today only `ShaderCompileService`
workers run off-thread and they consume immutable `PreparedProgram` strings. The refactor must
preserve this seam and avoid these hazards if/when derivation moves off-thread (WI-8 scope):

1. **GL queries in `versionPreamble` are context-bound.** `glVersionMacro` (`SourceProcessor.cpp:23-30`,
   `glGetString`), `maxColorBuffers` (`:32-39`, `glGetIntegerv`), `driverPreamble` (`:77-117`,
   multiple `glGetString`) and `supportedGlExtensions` (`GlState.cpp:238-273`, lazy `glGetIntegerv`/
   `getStringi` with a **non-synchronized `static bool initialized`**). If `prepareProgram`/`prewarm`
   run on a worker without a current context (or two threads race the lazy init), the resulting
   macros (MC_GL_VERSION, MAX_COLOR_BUFFERS, MC_GL_VENDOR_*, MC_<ext>*) can be wrong or racy.
   **Fix:** capture these once on the GL thread into `PackDefinition`/`PackInstance` (or a snapshot
   struct) before any compile is submitted; the derivation function must read the snapshot only.

2. **Determinism of prepared source = determinism of contentHash.** `ShaderProgram::contentHash`
   (`ShaderProgram.cpp:496-511`) hashes the exact bytes of (preamble, vertex, fragment, geometry,
   tessControl, tessEval). `compile()` and `prewarm()` deliberately produce byte-identical strings
   (`Compiler.cpp:24-26`) so hashes dedup and the disk cache (`ShaderCompileService::disk_`) hits.
   Any reordering of the derivation (e.g., parallelizing per-program while sharing the macro seed
   table) must be **order-stable**: `normalizePackSource` is a deterministic single pass, but the
   seed table (`seedMacrosFromDefines` parses the *preamble text*) must be built once and shared
   read-only. If text changes, disk-cached binaries silently go stale — bump the cache format, don't
   mask it.

3. **Shared mutable pack caches.** `PackInstance::sourceCache` (`cachedText`, `Compiler.cpp:113-119`),
   `compiledPrograms` (`compileImpl` `Compiler.cpp:171-174,219`), `programEnabledCache`
   (`isProgramEnabledCached`, `PassIndex.cpp:109-118`), `customUniforms.compile`/`evaluate`
   (`CustomUniforms.cpp:1113-1188`) are unsynchronized `unordered_map`/members mutated on the main
   thread today (from `PackManager::reload`/`setSetting`/`prewarmPacks` and render-time `compile`).
   If any of those move to a worker while main-thread draw still calls `worldProgram` → data race.
   The plan (WI-8) says compile moves to GlCompile; **derivation and these caches must stay on one
   thread (main) or be made per-request-pure**.

4. **`CustomUniformRuntime::evaluate` must stay single-threaded per frame.** Uses
   `static thread_local std::mt19937` (`CustomUniforms.cpp:1159`) plus member `smoothStates_`/
   `autoSmoothId_` (`:1164-1167`, seeded `100000`). `random()` custom uniforms are **not
   reproducible across threads**, and smooth ids/state are per-runtime. Called from
   `Pipeline::setFrameUniforms` (`Pipeline.cpp:435`) on main. Do not parallelize `evaluate`.

5. **Worker destroy() writing main GL static.** `ShaderProgram::s_lastBoundProgram`
   (`ShaderProgram.cpp:14,212-213`) is main-context state. `runJobOnCurrentContext`'s local
   `ShaderProgram` (`ShaderCompileService.cpp:200`) calls `destroy()` on a worker; because handles
   are shared across a share-group, a worker destroying a program whose handle equals the main
   thread's last-bound handle would clear `s_lastBoundProgram` (benign — main just re-`glUseProgram`s
   — but a real cross-thread write today). Flag for WI-8.

6. **`GlslSnippets` cache is already safe off-thread** (`gMutex`, `GlslSnippets.cpp:14-17,40`) —
   keep it; but `setSourceMapForTesting` (`:60-64`) must never run while a worker preprocesses.

7. **`Loader.cpp` format/target scanners** (`scanTargetFormats`, `inferColortexFormatsFromLayouts`,
   `Loader.cpp:581-629`, `isOffsetInComment` `:506-530`) are pure text scans run at pack load (main).
   They are read-only after load and safe to leave.

---

## 5. What the refactor MUST NOT change (GLSL source parity)

Non-negotiable invariants; changing any of these changes compiled GLSL or `#if` outcomes and will
either break packs or silently invalidate the shader-binary disk cache:

1. **The derivation pipeline order and purity:** `resolveIncludes` (include→rewrite-options→include,
   `Compiler.cpp:120-143`) → `normalizePackSource` (`SourceProcessor.cpp:184-311`) →
   `lowerVertexSource`/`lowerFragmentSource` (`:569-692`) → `mergeColorWheelMaterial`
   (`ColorWheelMerge.cpp:68-89`) → `assemble(preamble, body)` (`ShaderProgram.cpp:47-58`). This must
   remain a pure function of (programName, stage, packDefinition, dimension, source, preamble) with
   no hidden state beyond the GL snapshot in item 4.1.

2. **`normalizePackSource` directive semantics** (P1/P2/P4/P5/P6 above): `#if/#ifdef/#ifndef/#elif/
   #else/#endif` stack pairing (CondFrame), `#define`/`#undef` only when active, `#version` strip,
   active-only `#extension` hoist, `#include`/`#warning`/`#custom`/`#moj_import` drop, unknown
   directives pass through, `\` line-continuation joining, `\r` stripping. Includes must remain
   resolved **before** `#if` evaluation (P7) — matches Java's IncludeGraph.

3. **The dialect fork (DP-1 / T1):** keep `sourceCompilesAsModern` deciding 120-vs-130+; keep the
   120 path emitting `attribute`/`varying` and the modern path emitting `in`/`out`. Do not unify.

4. **The macro table and emission:** seed from the preamble text + extension flags
   (`seedMacrosFromDefines`, `PreProcessor.cpp:469-507`); `defined()` handling
   (`resolveDefinedOperators` `:293-329`); empty-body object macros → `1` in `#if`; the preamble's
   `#define` text block (MC_*, IS_IRIS, IRIS_VERSION, PPT_/CAT_/BIOME_, MC_RENDER_STAGE_,
   IRIS_FEATURE_*, MC_<ext>). Any *additions* for Java parity (M1–M8) are allowed but must bump the
   program cache/disk-cache format.

5. **`#include` resolution rule** (P7): relative-to-parent; leading `/` → `shaders/`; cycle and
   missing-file errors; dimension-prefix fallback (DP-3); `stripFormatDirectives` (P8) — and its
   Java-side counterpart must be confirmed before touching it.

6. **Vertex attribute contract:** `bindAttribLocation` list and order (`ShaderProgram.cpp:146-157`)
   plus the injected `vaPosition/vaUV0/vaUV2/vaColor/vaNormal/chunkOffset` declarations
   (`SourceProcessor.cpp:501-516`, `ColorWheelMerge.cpp:17-24`). The renderer's vertex format feeds
   these; changing them breaks every draw.

7. **Program selection maps:** `irisShadowProgramForGbuffers`, `resolveIrisShadowProgramKey`,
   `programFallbackKey` (`PassIndex.cpp:178-261`), `isProgramEnabled`/`BoolExpression`
   (`PassIndex.cpp:12-118`), and the `worldProgram` resolution order (`Pipeline.cpp:704-782`,
   including `clrwl_` early-out `:744`). These mirror `ProgramId.java`/`ProgramFallbackResolver.java`
   and must stay in lockstep.

8. **Snippet text and names** (`src/net/minecraft/client/render/shaders/glsl/*.glsl`): `mc_hand_depth` (0.125), `alpha_test_discard`
   (accessor placeholder + `alphaTestRef`), `iris_fog_frag_coord_*`, `iris_front_color_global`,
   `iris_lightmap_matrix`, `chunk_fade_*`, `compat_alpha_check` (the exact `f16vec3 color =
   f16vec3(texture(gtexture, v.coord).rgb);` needle), `default_composite.vsh`. These are the port's
   "GLSL helper library" and the compile-targets for packs.

9. **Custom uniform evaluation:** one `evaluate()` per runtime per frame on one thread
   (hazard 4); declaration-order evaluation; `autoSmoothId_` reset to 100000; the function set
   (`callFunction`). Add variadic `min`/`max` only as an additive change.

10. **GL-context snapshot before workers:** `glVersionMacro`/`maxColorBuffers`/`driverPreamble`/
    `supportedGlExtensions` values must be captured on the GL thread (see §4.1) and the derivation
    function must read the snapshot, so `prepareProgram` can be moved to a worker deterministically
    with WI-8.

11. **Keep `programFromPackSync` (or delete it deliberately):** it is currently dead. If WI-8 keeps
    a synchronous fallback, it must produce the identical prepared strings (same contentHash) as the
    async path — never a second, slightly-different derivation.

12. **Do not add a second GLSL preprocessor.** There are already two `#if` engines (DP-8). If the
    refactor introduces a worker-side preprocessing path, it must reuse `normalizePackSource` +
    `evaluateIfExpression` verbatim; do not hand-roll a third parser.
