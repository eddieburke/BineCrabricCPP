# RULES FOR AGENTS

Beta 1.7.3 Minecraft port. Java reference in `mcp/`; native C++ in `native/`.

---

## 1. Project layout

- **Java reference:** `mcp/src/net/minecraft`
- **Native target:** `native/src/net/minecraft/` (mirrors Java package layout)

---

## 2. Build and compile

Use **only** `native/build-omega.ps1` to build or compile native C++.

- Run from `native/`: `.\build-omega.ps1`
- Do **not** invoke `cmake`, `ninja`, `msbuild`, `make`, or other build tools directly unless the user explicitly asks for a one-off diagnostic command.
- After editing native sources, always verify with this script.

```powershell
Set-Location native
.\build-omega.ps1
.\build-omega.ps1 -Clean
.\build-omega.ps1 -RunTests
```

In **Multitask Mode**, only the **compile fixer** stage may build or test, and it must use this script.

---

## 3. Java reference path

When porting or comparing behavior, grep and read from **`mcp/src/net/minecraft`**.

Do **not** use stale copies such as `c:\Users\Eddie\Desktop\mcp43 - Copy - Copy`. The desktop copy is duplicate/stale; the repo tree is authoritative.

---

## 4. Never stub

When porting Java → C++:

- **Never** write stub or placeholder implementations.
- Port **1:1** with real, faithful behavior.
- Do **not** add new `PORT-STUB` shims. Existing ones (Stat, Stats, PlayerStats, etc.) already block faithful ports of dependents.
- If a dependency is itself a stub, either un-stub it faithfully or pick work whose dependencies are already real (e.g. network packets into fully-fleshed network handlers).

---

## 5. Agent workflow

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

## 6. Wenyan-ultra (optional)

Only when the user sets or requests this mode: high information density using Classical Chinese grammatical backbone, German compounds for complex concepts, Latinate terms for precision, and unchanged technical domain terms.

- Short classical phrases carry full narrative arcs; omit subject/pronoun once established.
- German compounds replace multi-word descriptors (e.g. `Klanglandschaftsentwicklung`).
- Latin for terminus/paradigm concepts (e.g. `Discessus`, `Paradigma`).
- Never abbreviate technical domain terms.

---

## 7. Shell scripts

Use **PowerShell 5** syntax in project scripts and commands. Do not use PowerShell 7-only features (e.g. `&&` chaining).
