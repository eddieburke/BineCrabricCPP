# Handoff — platform layer deslop (win32 → portable), 2026-08-17

## STATE: MID-REFACTOR. THE TREE DOES NOT COMPILE. NOT BUILT ONCE YET.

No build has been run. Callers of the deleted `DisplayManager` forwarders and of the
deleted `Minecraft` applet/canvas constructor have **not** been updated yet. Expect a
large error list on the first build. Nothing here has been committed.

HEAD at handoff: `04d6da9d Turn chests to face the player who places them`
(HEAD moved mid-session — another agent committed `ChestBlock` while this work was in
flight. Re-check `git log`/`git status` before staging anything.)

## Task

"Deslop and refactor the functionality system so it's not purely win32-specific and can
work on other platforms generally, including input, window systems, etc."
Follow-ups from the user during the session:
1. `deslop.md` discipline applies — **the deliverable is deletion**, not a cleaner facade.
2. "Other files like inventory and things handling shift click and lua inputs are SLOP"
   — **not started**, see *Next*.

## Working tree — do not clobber

`git status --short` at handoff:

```
 M src/net/minecraft/client/Minecraft.cpp
 M src/net/minecraft/client/gl/GLCore.cpp
 M src/net/minecraft/client/gl/GLCore.hpp
 M src/net/minecraft/client/gl/GlConstants.hpp
 M src/net/minecraft/client/gui/screen/ingame/HandledScreen.cpp
 M src/net/minecraft/client/input/InputSystem.cpp
 M src/net/minecraft/client/input/InputSystem.hpp
 M src/net/minecraft/client/platform/glfw/Window.cpp
 M src/net/minecraft/client/platform/glfw/Window.hpp
 M src/net/minecraft/client/util/DisplayManager.cpp
 M src/net/minecraft/client/util/DisplayManager.hpp
 M tests/profile_preset_apply_test.cpp
 M tests/shader_pack_loader_test.cpp
?? tests/slot_click_modifier_test.cpp
```

**Three of these are NOT this session's work and must survive.** They were already
uncommitted when the session started:

- `HandledScreen.cpp` — cursor-stack render moved after the decoration pass, z-translate
  `32.0f` → `200.0f`. Untouched by this session.
- `tests/profile_preset_apply_test.cpp`, `tests/shader_pack_loader_test.cpp`, and the
  untracked `tests/slot_click_modifier_test.cpp` — untouched by this session.
- `InputSystem.{cpp,hpp}` are **mixed**: a pre-existing in-flight change (deleted
  `guiShift`/`guiLeftShiftIntent_`/`handleKeyboardEdge`/`onScreenChanged`/`activeScreen_`,
  reworked `slotClickModifier()` to read raw L/R shift) plus this session's port. Both are
  preserved in the current file. Do not `git checkout` these files.

## Design decision (the shape everything else follows)

GLFW becomes the **only** window/input/GL-context backend on every platform. It was
already vendored and already the implementation — but the entire `Window.cpp` body sat
behind `#ifdef MINECRAFT_USE_GLFW`, which CMake defined **only** `if (WIN32)`. So on
non-Windows the window layer compiled to nothing and `InputSystem` compiled to a shell of
no-op `#else` branches returning `false`/`0`.

Platform ownership after the refactor:
- `platform::glfw::Window` owns the window, GL context, event pump, present, **and the
  cursor** (`setCursorLocked`, `cursorPosition`).
- `input::InputSystem` owns queues/routing/movement/mouse-look and makes **zero** OS
  calls. It has no `windows.h`, no `HWND`, no `#ifdef _WIN32` left.
- `util::DisplayManager` keeps only client display *policy* (setup, resize, fullscreen,
  GL error log). Its nine pass-through methods are deleted.

## DONE (edits written, unverified by any compiler)

### `gl/GlConstants.hpp`
- Deleted `#if !defined(_WIN32) #error "OpenGL rendering requires Windows" #endif`.
- Deleted `#define MINECRAFT_GL_REAL 1` (its only consumer was a dead `<GL/glu.h>`
  include in `GameRenderer.cpp` — **that include is still there, see Next**).
- `<windows.h>` + `<GL/gl.h>` → `<GLFW/glfw3.h>`. GLFW defines
  `APIENTRY`/`WINGDIAPI`/`CALLBACK` before including the GL header, so `GL/gl.h` works
  without `windows.h`. The long `#undef GL_*` block below it is unchanged and still needed.

### `gl/GLCore.hpp`
- Same header swap.
- Deleted `PFN_SwapInterval` typedef, the `swapInterval` member, and `static bool present()`.

### `gl/GLCore.cpp`
- `loadProc` (was `wglGetProcAddress` + `GetModuleHandleW("opengl32.dll")` fallback +
  the `<= 3 / -1` sentinel dance) → one line: `glfwGetProcAddress(name)`.
- `init()` guard `wglGetCurrentContext()` → `glfwGetCurrentContext()`.
- Deleted the `wglGetExtensionsStringEXT`/`ARB` + `strstr` block; `swapControlTearSupported`
  is now `glfwExtensionSupported("WGL_EXT_swap_control_tear") || (… "GLX_EXT_swap_control_tear")`.
- `activeTexture` now goes through `loadProc` instead of raw `wglGetProcAddress`.
- `setSwapPacing` calls `glfwSwapInterval(interval)` directly; the `swapInterval != nullptr`
  guard is gone.
- **Deleted `GLCore::present()`** — it was `SwapBuffers(wglGetCurrentDC())`, a *second*
  presenter that `Window::present()` tried first and only fell back from. One presenter now.
- Dropped the now-unused `<cstring>`.

### `platform/glfw/Window.{hpp,cpp}`
- Deleted the `#ifdef MINECRAFT_USE_GLFW` wrapper around the whole TU, the
  `GLFW_EXPOSE_NATIVE_WIN32`/`glfw3native.h` include, `windows.h`, and `struct GLFWwindow;`.
- Deleted `hwnd()`, the private `window()` (dead — no callers), `isFullscreen()` (dead —
  no callers), and `setParent(void*)` (empty body; existed only for the applet canvas).
- Added `static void setCursorLocked(bool)` → `glfwSetInputMode(GLFW_CURSOR, …)`.
- Promoted the anon-namespace `cursorPosition` helper to `static void cursorPosition(int&,int&)`;
  the two in-file callbacks now call `Window::cursorPosition`.
- `present()` no longer calls `GLCore::present()`; it just `glfwSwapBuffers`.

### `input/InputSystem.{hpp,cpp}` — the main event
- **Every `#ifdef _WIN32` gone.** No `windows.h`, no `HWND`, `POINT`, `RECT`.
- Deleted `init(HWND)` and `initMouse(HWND)` entirely — at startup they only zeroed
  already-default state plus seeded a cursor baseline GLFW now owns.
- Deleted `shutdown()` (it was `clearOnDeactivate()` verbatim).
- Collapsed the four `push*Event` → `ingest*` forwarding pairs into four functions.
- `lockCursor`/`unlockCursor`: ~55 lines of `GetClientRect`/`ClientToScreen`/`SetCursorPos`/
  `ClipCursor`/`ShowCursor` → `Window::setCursorLocked(bool)` + reset the look baseline.
- `pollMouseLook`: ~35 lines of `GetCursorPos` + manual re-centring → read
  `Window::cursorPosition` and diff. GLFW's `GLFW_CURSOR_DISABLED` supplies unbounded
  virtual motion, so **no re-centring is needed at all**.
  - **Sign check done:** win32 read y top-down and negated (`last.y - point.y`);
    `Window::cursorPosition` returns y bottom-up, so `y - lastY` preserves the old sign.
  - **Behaviour change to verify in game:** the baseline is now dropped on every
    lock/unlock (`mouseHasLastPoint_ = false`) so the first poll after a mode switch
    yields a zero delta instead of a jump.
- `updateCursorPosition()` → `Window::cursorPosition(cursorX_, cursorY_)`.
- Members `mouseWindow_`, `cursorGrabbed_`, `mouseLastPoint_` replaced by
  `mouseLastX_`/`mouseLastY_`.
- `acceptsInput()` → `Window::isActive()` (was `DisplayManager::isActive()`, win32-only,
  `true` elsewhere).
- `slotClickModifier()` keeps the in-flight raw-shift behaviour, minus the `#ifdef`.

### `util/DisplayManager.{hpp,cpp}`
- **Deleted nine pass-throughs**: `ensureGlContext`, `setSwapPacing`, `present`,
  `pumpMessages`, `pumpAndPresent`, `hwnd`, `isActive`, `isCloseRequested`, `destroy`.
- Deleted `scheduleScreenResize(Minecraft&)` — a two-hop forwarder
  (`Minecraft::scheduleScreenResize()` → `DisplayManager::…` → `client.pendingScreenResize_ = true`)
  reaching a private field through the `friend class util::DisplayManager` declaration.
- Deleted `windows.h` and every `#ifdef _WIN32` in the file. `setupAndCreateDisplay`,
  `toggleFullscreen` and `resize` now run on all platforms.
- `toggleFullscreen` inlines the old `pumpAndPresent` as `pumpMessages(); present();`.
- Dropped the `client.canvas != nullptr` branch (dead — see below).

### `Minecraft.cpp` (partial)
- `Minecraft(void* component, void* canvas, MinecraftApplet*, int, int, bool)` →
  `Minecraft(int width, int height, bool fullscreen)`; same for `RunnableMinecraft`.
  Verified dead first: the sole construction site is
  `std::make_unique<RunnableMinecraft>(nullptr, nullptr, nullptr, …)`, `component` was
  already `(void)component`, and `MinecraftApplet` is a header-only type that is never
  instantiated anywhere in the tree.

## NEXT — required to compile (in order)

1. **`Minecraft.hpp`** — the constructor declaration at ~L89 still has the old signature.
   Delete the `canvas` (~L186), `isApplet` (~L187) and `applet` (~L200) fields, the
   `class MinecraftApplet;` fwd decl (~L47), and `friend class util::DisplayManager;`
   (~L231) if nothing else needs it. Make `scheduleScreenResize()` set
   `pendingScreenResize_ = true` itself.
2. **`Minecraft.cpp`** — remaining sites:
   - `#include "net/minecraft/client/MinecraftApplet.hpp"` (L15) → delete, then delete
     `src/net/minecraft/client/MinecraftApplet.hpp`.
   - `applet->clearMemory()` in `stop()` (~L403) and `applet != nullptr && !applet->isActive()`
     in the Drain phase (~L858) → delete both dead branches.
   - `canvas == nullptr && DisplayManager::isCloseRequested()` (~L861) →
     `Window::isCloseRequested()`.
   - `DisplayManager::present()` ×2 (~L211, ~L899) → `Window::present()`; drop the
     `#ifdef _WIN32` around both.
   - `input::InputSystem::init(DisplayManager::hwnd())` (~L343) → **delete the call**.
   - lifecycle "input" owner (~L447) → body becomes `InputSystem::clearOnDeactivate()`;
     drop the `#ifdef _WIN32`.
   - `DisplayManager::destroy()` (~L464) → `Window::destroy()`, drop the guard.
   - `DisplayManager::setSwapPacing(…)` (~L763) → `gl::GLCore::setSwapPacing(…)`, drop guard.
   - `DisplayManager::isActive()` (~L793) → `Window::isActive()`, drop guard.
   - `DisplayManager::pumpMessages()` (~L846) → `Window::pumpMessages()`. Keep
     `diagnostics::pingMainLoopHeartbeat()` inside `#ifdef _WIN32` — it is declared only
     under that guard in `ClientDiagnostics.hpp`.
   - Add `#include "net/minecraft/client/platform/glfw/Window.hpp"`.
3. **`render/GameRenderer.cpp`** — `#ifdef _WIN32 DisplayManager::isActive() #else client->focused.load()`
   (~L615) collapses to `Window::isActive()`. `input.syncCursorFromOs()` (~L677) loses its
   guard. Delete the dead `#ifdef MINECRAFT_GL_REAL` + `#include <GL/glu.h>` (~L50) —
   **verified: no `glu*` call exists anywhere in `src/`**. Swap the `DisplayManager` include
   for the `Window` one.
4. **`texture/TextureManager.cpp`** — `DisplayManager::ensureGlContext()` ×3 (L180, L433,
   L554) → `Window::ensureGlContext()`; fix the include.
5. **`render/ProgressRenderer.cpp`** — `DisplayManager::present()` (L96) →
   `Window::present()`; fix the include. Leave the `pingMainLoopHeartbeat` guard.
6. **`CMakeLists.txt`** — the load-bearing change, not yet started:
   - Move the `glfw3` `find_package`/`FetchContent` block out of `if (WIN32)`.
   - `minecraft_link_display`: link `glfw` unconditionally; **delete the
     `MINECRAFT_USE_GLFW` define** (the macro no longer exists in any source).
   - Delete `minecraft_enable_glfw_compile` — byte-identical body to
     `minecraft_link_display`; fold its one call site (`minecraft_client`) onto the latter.
   - `minecraft_link_core_client`: `opengl32 glu32` stays `if (WIN32)`; add
     `find_package(OpenGL REQUIRED)` + `OpenGL::GL` for the rest. `glu32` can go once
     step 3 removes the `glu.h` include.
   - `GlConstants.hpp` now pulls `<GLFW/glfw3.h>`, so **every target that compiles a TU
     including it needs the glfw include dirs**. `minecraft_client` and
     `minecraft_omega_tests` already get them. **Check `minecraft_server`** — it compiles
     `ClientDiagnostics.cpp`, `TranslationStorage.cpp`, `MinecraftDirectories.cpp`,
     `PlayerTextureUrls.cpp`; if any reaches `GlConstants.hpp` it needs glfw too.
7. **Cheap follow-ons already scouted:**
   - `render/GlState.cpp` — `windows.h` (L22-24) is there for **one** call:
     `hasGlContext()` at L594 is `#ifdef _WIN32 wglGetCurrentContext() #else GLCore::activeTexture != nullptr`.
     Collapse both branches to `glfwGetCurrentContext() != nullptr` and delete the include.
   - `render/pipeline/AsyncDepthSampler.cpp` — L3-4 `<windows.h>` + `<GL/gl.h>` are only
     for GL types; replace with `GlConstants.hpp`.
   - `debug/ClientProfilerOverlay.cpp` — L2-6 `NOMINMAX`/`windows.h`/`GL/gl.h` are
     redundant, it already includes `GlConstants.hpp`.
   - `platform/Browser.cpp` — `openUrlInBrowser` returns `false` on non-Windows; add the
     `xdg-open` / `open` path.
   - Delete the empty directory `src/net/minecraft/client/platform/win32/`.
8. **Then build — once, at the end**: `.\build-omega.ps1` (Debug/Client is the agent
   default). Confirm `Finished … (exit 0)` **and** a fresh
   `Client: build-omega\minecraft_native.exe` line. Do **not** run `-RunTests` /
   `ctest` (holds the lock for minutes and restores the previous binary).

## NOT STARTED — the user's second message

> "Other files like inventory and things handling shift click and lua inputs are SLOP"

Nothing done here. Leads scouted but **unverified**:
- `HandledScreen.cpp` + `InputSystem::slotClickModifier()` + the untracked
  `tests/slot_click_modifier_test.cpp` are the shift-click surface. The in-flight change
  already deleted the `guiShift`/`guiLeftShiftIntent_`/`onScreenChanged` machinery in
  favour of reading raw L/R shift; check whether `ModifierState::shift` and
  `slotClickModifier()` are now two names for one thing, and whether `SlotClickModifier`
  (a 2-value enum wrapping a bool) earns its existence.
- Lua input path: `mod/lua/LuaHostApi.{cpp,hpp}`, `mod/runtime/LuaDirectHooks.hpp`,
  and the `luaHookKeyPress`/`luaHookMouseButton` call sites in `InputSystem::pollGameKeyboard`
  / `pollGameMouse`. Not read yet.

## Explicitly OUT OF SCOPE — still win32-only after this refactor

These are real functionality, not slop, and each needs a **new portable backend**, not a
deletion. Do not stub them (RULES §6). A non-Windows build will still fail on the first two:

| Area | File(s) | What it needs |
|---|---|---|
| Image **decode** (all PNG loading) | `texture/TextureManager.cpp` — GDI+ `Bitmap`, `HGLOBAL`, `IStream` | stb_image or a zlib-based PNG reader (zlib **is** already linked on all platforms). Not vendored — `third_party/` has no stb. |
| Image **encode** (screenshots) | `Screenshot.cpp` — unguarded `Windows.h`+`gdiplus`, hard compile failure off Windows | same decoder's writer half |
| Audio | `platform/audio/backend/XAudio2Backend.cpp` | a second `AudioBackend` impl (miniaudio/OpenAL); the interface in `AudioBackend.hpp` already exists |
| HTTP | `util/http/HttpClient.cpp` — WinHTTP | libcurl or a socket TLS client |
| Sockets | `server/network/ServerSocket.hpp`, `network/Connection.hpp` | BSD sockets behind the same shape |
| Crash/diag | `diagnostics/ClientDiagnostics.*` (dbghelp), `ShaderFail.hpp` | already `#ifdef`-guarded at the *declaration*, so callers must stay guarded |
| Shader binary cache | `gl/ShaderBinaryCache.cpp` — `CreateFileW`/`MoveFileExW` | `std::fstream` + a careful atomic replace (`std::filesystem::rename` does **not** replace an existing file on Windows — that is why `MOVEFILE_REPLACE_EXISTING` is used) |
| Shaderpack dir watcher | `render/pipeline/PackLifecycle.cpp` — `ReadDirectoryChangesW` | inotify/kqueue, or fall through to the existing `#ifndef _WIN32` polling stamp |
| Dedicated server GUI | `server/platform/win32/*`, `server/dedicated/gui/*` | a whole Win32 GUI app; rewrite, not a port |
| Installer | `installer/installer.cpp` | Windows-only by nature |

## Gotchas

- **Never built.** Every claim above is "written", not "verified". Treat the whole diff as
  unproven until `build-omega.ps1` says otherwise.
- `MINECRAFT_USE_GLFW` no longer appears in any source file. Until CMake stops defining it
  the build still works; but until CMake starts linking glfw on non-Windows, nothing
  outside Windows links.
- The `#undef GL_*` block in `GlConstants.hpp` is still required — `<GLFW/glfw3.h>` pulls
  in `<GL/gl.h>` just like the old include did.
- `Window::isActive()` is now the single focus source. `GameRenderer` previously used
  `client->focused` on non-Windows; that parallel pair must not be reintroduced.
- Another agent is active in this worktree (HEAD moved mid-session). Re-check
  `git status` / `git log` immediately before any staging, and split hunks so only your
  edits enter a commit (RULES §11/§16).
