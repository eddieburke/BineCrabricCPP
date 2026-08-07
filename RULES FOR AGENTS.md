# RULES FOR AGENTS

> **READ THIS FILE IN FULL BEFORE DOING ANY WORK.** All agents (main agent and subagents) must read this entire document top to bottom before reading, editing, building, or committing anything. Do not skim, skip sections, or rely on a summary of these rules.

Beta 1.7.3 Minecraft port. Java reference in `mcp/`; native C++ in `native/`.

---

## 1. Project layout

- **Java reference:** `mcp/src/net/minecraft`
- **Native target:** `native/src/net/minecraft/` (mirrors Java package layout)

---

## 2. Build and compile

Use **only** `build-omega.ps1` to build or compile native C++.

- Run from project root: `.\build-omega.ps1`
- Do **not** invoke `cmake`, `ninja`, `msbuild`, `make`, or other build tools directly unless the user explicitly asks for a one-off diagnostic command.
- After editing native sources, always verify with this script.

```powershell
set location native
.\build-omega.ps1
.\build-omega.ps1 -Clean
.\build-omega.ps1 -RunTests
```

**Toolchain location**: The build script requires a bundled GCC toolchain at `toolchain\mingw64`. This contains:
- `g++.exe` - GCC compiler
- `gdb.exe` - GDB debugger (at `toolchain\mingw64\bin\gdb.exe`)
- `gdbserver.exe` - GDB server

When a toolchain is present, the script uses:
- `toolchain\mingw64` as the compiler prefix (sets `CXX="toolchain\mingw64\g++.exe"`)
- The bundled GDB for debugging, regardless of other GDB installations

**Environment path**: The script prepends `toolchain\mingw64\bin` to the PATH to ensure the correct tools are used.

In **Multitask Mode**, only the **compile fixer** stage may build or test, and it must use this script.

---

## 3. Java reference path

When porting or comparing behavior, grep and read from **`mcp/src/net/minecraft`**.

Do **not** use stale copies such as `c:\Users\Eddie\Desktop\mcp43 - Copy - Copy`. The desktop copy is duplicate/stale; the repo tree is authoritative.

---

## 4. General deslop rule (diagnosis, code, & architecture)

When diagnosing bugs, writing code, or refactoring, deslop aggressively across all layers.

### 4.1 Diagnosis: trace root causes, don't surface-hack
- **Trace end-to-end:** Never rely on screenshot pixel analysis, image captures, or superficial guesses to diagnose bugs. Trace execution through state and pipeline end-to-end to find exact root causes.
- **Rendering example:** When investigating rendering bugs (transparency, blending, alpha, missing/black textures, wrong colors):
  - Do not rely on screenshot pixel analysis.
  - Trace the C++ draw path end-to-end: `RenderType`/`RenderPassScope` -> `RenderCore` state (blend/depth/alpha test) -> `Tessellator` -> `bindAndUploadUniforms` -> shader selection (`worldProgram` resolver, program fallback chains) -> active GLSL pack.
  - Cross-reference active shaderpack shaders (e.g. `shaders/RenderPearl.../`, `shaders/SEUS PTGI Iris`) and Java Iris reference in `third_party/mcp/iris/` for expected pass behavior (blend directives, `alphaTestRef`, `entityColor`, `RENDERTARGETS`, draw buffers). Consult https://shaders.properties/current/reference/ when contracts are ambiguous.
  - Verify GLSL uniform/attribute upload sites in `RenderCore.cpp` / `WorldProgramBinder.cpp` / `Uniforms.cpp` before suspecting GLSL shaders.

### 4.2 Code & architectural deslop
- **Zero comments:** Delete narrative comments, conversational explanations, dead code blocks, and redundant docstrings inside source files. Code must be self-documenting. Only single-line Java/Iris cross-reference paths are permitted when mapping ported logic (e.g. `// see mcp/src/net/minecraft/...`).
- **Nuke overabstractions:** Eliminate speculative interfaces, single-implementation factories, facade-over-facade layers, and multi-tiered indirection. Keep code concrete, linear, and direct.
- **Delete wrapper shit:** Remove classes, structs, and forwarding methods that exist solely to wrap or delegate calls to another object. Inline wrapped logic directly into caller sites or owned value types.
- **Eliminate singleton crap:** Eradicate `getInstance()` singletons, static global instance pointers, and classes holding singleton handles. Demote global/scratch state to local stack variables or explicit parameter passing.
- **No dummy fallbacks or silent error swallowing:** Never patch runtime errors by returning empty/dummy defaults, swallowing exceptions, or ignoring broken contracts. Trace upstream and fix the true cause.
- **No speculative flexibility:** Do not write unused config knobs, hypothetical extension points, or general-purpose abstractions for one-off tasks. Hardcode the exact concrete requirement.
- **Zero hot-loop allocations:** Avoid allocating heap objects, dynamic arrays, or temporary wrappers inside frame/tick hot loops. Pass buffers by reference or stack allocate.
- **No macro or helper clutter:** Do not write custom macros, trivial single-line inline helpers, or redundant getter/setter pairs for public struct fields. Expose fields directly.

---

## 6. Never stub

When porting Java → C++:

- **Never** write stub or placeholder implementations.
- Port **1:1** with real, faithful behavior.
- Do **not** add new `PORT-STUB` shims. Existing ones (Stat, Stats, PlayerStats, etc.) already block faithful ports of dependents.
- If a dependency is itself a stub, either un-stub it faithfully or pick work whose dependencies are already real (e.g. network packets into fully-fleshed network handlers).

---

## 7. Agent workflow

### Normal mode

- Work **inline** — Read, Grep, Glob directly.
- **Do not** spawn Agent/subagent tasks unless the user explicitly asks.

See **`caveman.md`** at `c:\Users\Eddie\Documents\New project 2\caveman.md` for inline-work and communication preferences.

### Multitask Mode (Cursor)

When asked, and when given extremely vague and general requests or seemingly nonsense try to figure out what they want before you: Run this pipeline in order. Only the **compile fixer** Nothing should attempt a cmake or anything until finished and will be specifically given knowledge of the existance of other agents, being team players, and such. They will want to read each others transcripts to see their progress if edits fail, and attempt to coordinate.

1. **Council (3–6 subagents, parallel)** — Review approach, risks, edge cases, likely files. No implementation, builds, or tests.
2. **Initial planner (1 subagent)** — Draft initial plan from council context.
3. **Transcript synthesizer (1 subagent)** — Consolidate council + planner transcripts; note disagreements and open questions.
4. **Plan auditors (2 subagents, parallel)** — Audit plan for gaps and wrong assumptions. No implementation or builds.
5. **Plan master (1 subagent)** — Fix auditor feedback; grep codebase; produce final master plan. No implementation.
6. **Executors (1–2 subagents)** — Implement master plan. No builds or tests.
7. **Compile fixer (1 subagent, final)** — Only stage that may build/test via `build-omega.ps1`. Fix failures; verify against plan.

Launch parallel stages where possible; wait for prerequisites; re-run the pipeline on material scope changes.

| Stage | Read / grep | Plan | Edit | Build / test |
|---|---|---|---|---|
| Council | ✓ | ✓ | ✗ | ✗ |
| Initial planner | ✓ | ✓ | ✗ | ✗ |
| Transcript synthesizer | ✓ | ✓ | ✗ | ✗ |
| Plan auditors | ✓ | ✓ | ✗ | ✗ |
| Plan master | ✓ | ✓ | ✗ | ✗ |
| Executors | ✓ | ✗ | ✓ | ✗ |
| Compile fixer | ✓ | ✗ | ✓ (fixes) | ✓ |

---

## 8. Wenyan-ultra mode (default)

**Activation:** on by default, for every agent, in every session, from the first response — no request needed. It stays on unless the user explicitly turns it off ("no wenyan", "normal mode", "plain English", "verbose"), and it resumes as the default in the next session regardless.

**What it is:** maximum information density per token, in *prose to the user*. It changes how you write, not what you do — every other rule in this file (never stub, build via `build-omega.ps1`, commit discipline, banned phrases) still applies in full.

### 8.1 Prose style

Classical Chinese grammatical backbone, German compounds for complex concepts, Latinate terms for precision, technical domain terms unchanged.

- Short classical phrases carry full narrative arcs; omit subject/pronoun once established.
- German compounds replace multi-word descriptors (e.g. `Klanglandschaftsentwicklung`).
- Latin for terminus/paradigm concepts (e.g. `Discessus`, `Paradigma`).
- Never abbreviate technical domain terms, file paths, symbol names, or flags — those stay verbatim and unmangled.

### 8.2 Ultimate terseness

- No preamble, no postamble, no restating the request, no summarizing what you are about to do.
- No "here's what I found", no bullet recaps of work already visible in the diff.
- Answer first, in the fewest tokens that remain unambiguous. One line is a complete response.
- Omit hedging, encouragement, and transitional filler entirely.
- Report failures, skipped scope, and uncertainty **in full** — terseness compresses phrasing, never content. A shortened report that hides a broken build violates this mode.

### 8.3 Comments in code

- **Add no comments** while this mode is active.
- **Sole exception:** a comment that points at another file — a cross-reference to the Java source being ported (`mcp/src/net/minecraft/...`), the Iris reference (`third_party/mcp/iris/`), a shaderpack GLSL, or a design doc. These are permitted and encouraged where the reason for the code lives elsewhere.
- Cross-reference comments are one line, path first: `// see mcp/src/net/minecraft/client/render/EntityRenderer.java:412`.
- Never delete existing comments just because this mode is on. Silence applies to new writing only.
- Doc comments required by the surrounding file's convention still get written; matching local style outranks this rule.

---

## 9. Shell scripts

Use **PowerShell 5** syntax in project scripts and commands. Do not use PowerShell 7-only features (e.g. `&&` chaining).

---

## 10. World profile / generation rules

See `native/docs/world-profile-generation-refactor.md` before editing world profiles, chunk decoration, or terrain generation.

- Profile hooks must receive the active `Chunk&`; do not call `world->getChunk` from a profile hook to rediscover the chunk being decorated.
- Whole-column profile transforms must use raw chunk writes plus one heightmap/light refresh, not thousands of live `Chunk::setBlock` calls.
- Terrain shapes run before vanilla features. Surface repaint/cap themes run after vanilla features.
- New world profile state must exist before `WorldSession::prepareWorld` builds spawn chunks.
- Remote Java multiplayer client worlds are packet-driven only; profile terrain generation/decorate hooks must not run there.
- Foliage theme tint applies to fixed spruce/birch/default leaf colors as well as biome-derived colors.

---

## 11. Git / PR discipline (subagents only, large changes only)

Applies **only** when a **subagent** executes a **massively large change** (touches many files, large diffs, or files likely shared with other agents). The main agent and small routine edits ignore this rule.

1. **Never commit or push on `main` directly.** All work goes through a feature branch + pull request.
2. **One branch per task/subagent.** Before starting, branch off current `main`: `git checkout -b <name>/<task>`. Do not share branches.
3. **Full staging separation before every commit.** The shared worktree may hold other agents' uncommitted changes in the same files. Use the checkout-and-restore split (`git checkout -- <file>`, re-apply only your edit, `git add`, restore the combined file) or hunk-level staging so **only your changes** enter the commit. Never `git add -A` / blind `git add <file>` on a mixed file without verifying `git diff --cached` shows nothing foreign.
4. **Verify staged content before committing:** `git diff --cached --stat` + spot-check hunks; confirm `git status --short` shows your files `M ` (staged) and others' ` M` (unstaged).
5. **Commit only your own work.** Leave other agents' files/diffs untouched in the worktree.
6. **Rebase onto latest `main` before pushing** (`git fetch` + `git rebase`), resolving conflicts to keep BOTH agents' changes. Never force-push.
7. **PR merges must not clobber concurrent work.** Merge `main` into your branch (not reverse); if another agent is actively editing a file, coordinate or split your hunk and leave theirs untouched.
8. **Assume other agents are active.** Re-check `git status`, `git diff`, `git log` immediately before any `add`/`commit`/`push` — index and HEAD can change mid-task.
9. **Handle races gracefully:** if the index is reset or someone commits while you're staging, re-verify your edits are intact and re-stage — never blind-reapply or overwrite.
10. **Clean up:** delete your branch after the PR merges; never leave the repo half-staged.

---

## 12. Banned phrases

Never use the following phrases when responding to the user:

- **"You're absolutely right"**
- **"You're right to push back"**

Affirming agreement is fine; use a different, non-robotic phrasing (e.g. "Understood", "Agreed", or just restate the point). Do not use these phrases in any form, variant, or paraphrase intended to reproduce them.

---

## 13. Commits and build locks (all agents)

Applies to **every** agent (main agent and subagents), for both small routine edits and large changes.

1. **Never commit with compilation errors.** Do not `git commit` or push any change until the build passes cleanly via `build-omega.ps1` — no compile errors, and tests pass when `-RunTests` applies. Code that fails to compile (or is unverified) is a hard block on committing.
2. **If the build is locked, wait — do not resolve it yourself.** If `.build-omega.lock` exists, `build-omega.ps1` reports another build in progress, or the build directory is held by another process, stop and wait. Do **not** kill processes, delete the lock file, or force-edit the build directory to break the lock. The main agent will eventually address the locked build. Only the main agent (or, in Multitask Mode, the compile fixer stage) may touch a locked build.

---

## 14. Collapse indirection — delete wrappers, don't layer over them

When a type exists only to wrap another — a singleton with `getInstance()` plus a wrapper class whose whole body is a pointer to that singleton, a forwarding method that just calls through, a file holding one struct nobody else uses — **delete the wrapper and inline what it wrapped**. Do **not** add a cleaner layer on top of the mess; no extra abstraction, no facade over the facade.

- **Default bar:** remove the useless file/type entirely.
- **Escape clause:** if collapsing it would add a shitload more code than keeping it, leave it as-is — keep the wrapper only when the cost of removal clearly exceeds the mess it makes.
- Applies to: overabstractions (singleton + pass-through wrapper), compatibility shimming (adapters that only forward), leftover immediate-mode or one-off glue, dead intermediate types.
- Worked pattern: a `Foo::getInstance()` global plus a separate `FooBar` class holding a pointer to that singleton and a few scratch fields, where callers had to `compute()` the global in one place and `prepare()` the wrapper later under a duplicated guard. Collapse to one value type (compute / prepare / exposed operation) owned as a local: one type and one global deleted, scratch state demoted from members to locals.
