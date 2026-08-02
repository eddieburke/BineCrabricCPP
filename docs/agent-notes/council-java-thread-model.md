# Council: Java-side threading model (Beta 1.7.3 + Iris 26.1) — what the C++ refactor must match

Role: council (review-only). No source edits, no builds.
Scope: document the JAVA-side threading/timing model so the C++ main-thread restructuring keeps functional parity with
vanilla Beta 1.7.3 and Iris 26.1 shaderpack behavior.

---

## 0. SOURCE AVAILABILITY (read this first — it changes how you trust the refs)

Verified reference trees in this repo:

| Tree | Path | Contents |
|---|---|---|
| Vanilla Java | `third_party/mcp/net/minecraft/` | **Server + shared classes only.** `src/` = 452 server/shared classes (World, Block, Entity, Packet*, Network*, Thread*, ChunkProvider*, etc.); `server/MinecraftServer.java`. |
| Iris 26.1 (mirror) | `third_party/mcp/iris/` | Mirror of `net.irisshaders.iris` packages (the one the C++ port cites). **Line numbers identical to the full tree** (verified: ProgramSet.java:64/82/86, IrisRenderSystem.java:42). |
| Iris 26.1 (full repo) | `third_party/iris/` | Full Iris source; `third_party/iris/common/src/main/java/net/irisshaders/iris/` is the same source mirrored at `third_party/mcp/iris/`. |
| C++ port (mirror of the absent client) | `src/net/minecraft/client/` | `Minecraft.hpp:3` self-describes as "Faithful port of net.minecraft.client.Minecraft (beta 1.7.3 MCP)". Use as the authoritative mirror of the CLIENT main loop, since the Java client tree is absent. |

**CRITICAL GAP:** the vanilla **client** Java sources are NOT in this repo:
`net.minecraft.client.Minecraft`, `net.minecraft.client.util.Timer`, `net.minecraft.client.NetClientHandler`,
`net.minecraft.client.sound.SoundManager`, `net.minecraft.client.renderer.EntityRenderer` — none exist anywhere in the
repo (searched whole tree; only a `class Timer` ref in Iris `uniforms/SystemTimeUniforms.java:62`).
The tree named in the task (`mcp/src/net/minecraft`) does not exist; the vanilla tree is at
`third_party/mcp/net/minecraft/` and is server-only.

Consequence for executors:
- Section 1 (client main loop/Timer) and the client thread inventory are **reconstructed** from (a) the C++ port (a faithful
  port of the Beta 1.7.3 client), (b) the server-side Java in the repo (which shares the exact same `NetworkManager`/
  `NetHandler`/`Timer`-era machinery), and (c) well-documented Beta 1.7.3 behavior. Each claim is flagged
  `[client: reconstructed]` vs `[server: file:line]` vs `[iris: file:line]`.
- If byte-level client fidelity is ever required, obtain a decompiled Beta 1.7.3 client (`Minecraft.run/gameLoop`,
  `Timer.updateTimer`, `NetClientHandler.handlePackets`) and re-verify section 1; the C++ port already mirrors it, so the
  risk is low.

---

## 1. Vanilla main loop: `Minecraft.run()` / game loop + `Timer`

### 1.1 The single-thread contract

Beta 1.7.3 (like all MC up to 1.7) is a **single-threaded game loop**: one "Client thread" runs
`run()` → `gameLoop()` → (a) `timer.updateTimer()`, (b) `elapsedTicks` × `runTick()`/`tick()`, (c) 1 × `render()`
using `timer.renderPartialTicks`, per frame. All game logic (input, world tick, entity tick, packet dispatch, chunk
render maintenance) and all rendering (incl. OpenGL) happen on that one thread. No frame graph, no async GL in vanilla.

C++ port mirrors this 1:1 (`[client: reconstructed]` but verified in C++ port):
- `src/net/minecraft/client/Minecraft.cpp:749-843` `Minecraft::run()`: loop →
  `timer.advance()` (784-794; paused only advances without changing partialTick, 788-791) →
  `for(i < timer.ticksThisFrame) tick()` (796-805) →
  `runRenderPhase(...)` once (807).
- `Minecraft.cpp:593-669` `tick()`: server-coordinator tick, mod hook, audio tick, stats/HUD, raycast, interaction,
  textureManager tick, input begin, screen tick, **network pump** (655-657: `multiplayerSession_.tick()` when no remote
  world), poll game, `runWorldSimulation()` (665).
- `Minecraft.cpp:555-592` `runWorldSimulation()`: deferred respawn apply, player join counter, camera update, weather,
  `world->tickEntities()`, `world->tick()`, `world->displayTick`, particle cleanup.
- `Minecraft.cpp:670-714` `runRenderPhase()`: `world->doLightingUpdates()` + `world->pumpChunkPublish()` (681-683),
  swap pacing + `pumpAndPresent()` (686-688), `gameRenderer->onFrameUpdate(timer.partialTick)` (700).

### 1.2 `Timer` class — tick vs render timing

Beta 1.7.3 `Timer` (fields from MCP 43; the C++ port is a faithful mirror:
`src/net/minecraft/client/util/Timer.hpp`, whole file):
- `ticksPerSecond` (default 20.0, `Timer.hpp:49`), `elapsedTicks` (`ticksThisFrame`), `renderPartialTicks`
  (`partialTick`), `tpsScale` (Lua tick-rate override, `Timer.hpp:52`).
- Time sources: wall clock (`System.currentTimeMillis()` ↔ `currentTimeMillis()`, `Timer.hpp:56-60`) and monotonic
  high-res (`System.nanoTime()` ↔ `nanoTimeMillis()`, `Timer.hpp:61-65`).
- `updateTimer()` (Java) ≈ `Timer::advance()` (`Timer.hpp:14-48`):
  1. detect clock jumps (`tickElapsed > 1000 || < 0` → resync, `Timer.hpp:19-21`);
  2. accumulate wall delta into a cooldown accumulator (`cooldownTickTime_`), and once per second re-estimate the
     ms/tick ratio from the wall:HR clocks and blend it with a 0.2 smoothing factor (`tickTimeCorrection_`,
     `Timer.hpp:23-31`) — this is the Beta-era "fix your clock" tick-rate correction;
  3. per frame: `frameDelta = (nowSeconds - timeSec_) * tickTimeCorrection_` clamped to [0,1], accumulate into
     `tickDelta`, `ticksThisFrame = floor(tickDelta)`, keep fractional remainder as `partialTick`
     (`Timer.hpp:38-47`);
  4. **cap `ticksThisFrame` at 10** (`Timer.hpp:44-46`) so a slow frame can't spiral the sim ahead of real time.

Semantics the shader path depends on:
- `partialTick` is the SAME value fed to the world interpolation, `GameRenderer::renderWorld` (C++
  `GameRenderer.cpp:504` logs `tick=%.2f`), entity rendering, and all Iris `tickDelta` uniforms
  (Iris captures it once per frame, see §3.3). One partial tick per render, monotonic within the frame.
- Up to 10 sim ticks can run before one render; the render partial tick is the leftover fraction. A shaderpack must see
  a stable, per-frame `frameTime`/`tickDelta` (Iris: `CapturedRenderingState.INSTANCE.setTickDelta(fakeTickDelta)`,
  `third_party/mcp/iris/mixin/MixinLevelRenderer.java:128-129`).

### 1.3 Where heavy work is scheduled in vanilla

- Server-side (integrated + dedicated) chunk generation/decor is **synchronous on the server thread**:
  `ChunkProviderServer.loadChunk/provideChunk` (`third_party/mcp/net/minecraft/src/ChunkProviderServer.java:48,63,93`),
  `ChunkProviderGenerate.provideChunk` (`src/ChunkProviderGenerate.java:221`). No executor. Spawn-area pregen loops
  inline (`server/MinecraftServer.java:159-198`).
- Per-tick bounded world maintenance `World.func_6156_d()` (`src/World.java:1693-1714`, max 500 pending metadata
  updates per call) is drained on the server thread (`MinecraftServer.java:193,377`).
- Region-file I/O is `synchronized` (thread-safe but NOT off-thread): `RegionFileCache.java:22,48`,
  `RegionFile.java:121,187`.
- Client-side chunk render updates are maintained on the render thread between frames (C++ mirror:
  `Minecraft.cpp:681-683` `doLightingUpdates` + `pumpChunkPublish` inside `runRenderPhase`).
- The ONLY Java scheduling primitives in vanilla Beta are raw `Thread` subclasses (§2). No
  `ScheduledExecutorService`/`Future` exists anywhere in the vanilla tree (grep confirms; the only
  executor/future usage in the whole reference set is Iris's, §3).

### 1.4 Integrated server (singleplayer)

- The integrated server is a **separate thread**, not part of the client loop
  `[client: reconstructed; server scaffolding verified]`:
  - `ThreadServerApplication("Server thread", mcServer).start()` is how `MinecraftServer.run()` is launched
    (`server/MinecraftServer.java:433`; `src/ThreadServerApplication.java:9-24` runs `mcServer.run()`).
  - The server thread runs its own 20-TPS tick loop (`MinecraftServer.java:251-339`; see §2/§4).
  - `MinecraftServer` ctor also spawns a `ThreadSleepForever` daemon (`MinecraftServer.java:61`).
- Client↔server communication in singleplayer goes through the SAME `NetworkManager` stack as real multiplayer
  (the client connects to the local server; both sides share `net.minecraft.src.NetworkManager`). The shared
  `NetworkManager`/`NetServerHandler`/`NetClientHandler` machinery in `third_party/mcp/net/minecraft/src` is this stack.
- **C++ divergence to note:** the C++ port hosts the integrated server as an EXTERNAL PROCESS
  (`src/net/minecraft/client/host/ServerProcessCoordinator.cpp:104-159` spawns `minecraft_server.exe` via
  `CreateProcessW`, ready-file handshake 174-186). That is *more* isolated than Java's in-process thread but preserves
  the functional contract (server ticks independently; client talks to it over the network stack). Keep that contract.

---

## 2. Thread inventory — vanilla Beta 1.7.3

### Server tree (authoritative, file:line)

| Thread | Purpose | Evidence |
|---|---|---|
| **"Server thread"** (`ThreadServerApplication`) | Runs `MinecraftServer.run()`: 20-TPS tick loop. Loop: `l1 += elapsed wall`; while `l1 > 50` ms → `doTick()` (50 ms budget = 20 TPS); `Thread.sleep(1)`; "Can't keep up!" guard at 2000 ms. | `server/MinecraftServer.java:251-339` (loop 259-289, budget 282-287, sleep 288, clamps 264-273), `src/ThreadServerApplication.java:9-24`. |
| **"Listen thread"** (`NetworkAcceptThread`) | Dedicated-server accept loop (`serverSocket.accept()` → `NetLoginHandler`). Client singleplayer uses a client-side socket, not this accept thread. | `src/NetworkListenThread.java:29-34` (ctor starts it), `src/NetworkAcceptThread.java:25-57`. |
| **"`<name>` read thread"** / **"`<name>` write thread"** (`NetworkReaderThread`/`NetworkWriterThread`, one pair per connection) | Socket I/O only. Reader: `readPacket()` loop + 100 ms sleep (`NetworkReaderThread.java:20-45`). Writer: `sendPacket()` loop + flush + 100 ms sleep (`NetworkWriterThread.java:22-66`). Counted via `NetworkManager.threadSyncObject`/`numReadThreads`/`numWriteThreads`. | `src/NetworkManager.java:47-50` (creation), `src/NetworkReaderThread.java`, `src/NetworkWriterThread.java`. |
| `NetworkMasterThread` (unnamed) | 5 s shutdown watchdog that force-`stop()`s the read/write threads if they won't die. | `src/NetworkManager.java:169` (spawn in `networkShutdown`), `src/NetworkMasterThread.java:20-46`. |
| `ThreadMonitorConnection` (unnamed) | 2 s watchdog on server-initiated shutdown; interrupts the writer and finalizes disconnect. | `src/NetworkManager.java:225-231` (`serverShutdown` spawns it), `src/ThreadMonitorConnection.java:20-35`. |
| `ThreadCommandReader` (unnamed, daemon) | Blocking stdin read → `mcServer.addCommand(...)` (commands drained on the server thread). | `server/MinecraftServer.java:68-70` (start), `src/ThreadCommandReader.java:19-34`. |
| `ThreadSleepForever` (unnamed, daemon) | `Thread.sleep(Integer.MAX)` loop; keeps AWT/GC references alive. | `server/MinecraftServer.java:61`, `src/ThreadSleepForever.java:9-33`. |
| `ThreadLoginVerifier` (unnamed) | Per-login HTTP check against `minecraft.net/game/checkserver.jsp`; result applied on the server thread. | `src/NetLoginHandler.java:98` (spawn), `src/ThreadLoginVerifier.java:15-51`. |
| AWT/Swing EDT | Dedicated-server GUI: `JFrame` + `GuiStatsComponent` uses `javax.swing.Timer(500,...).start()` + `repaint()`. GUI thread ≠ server thread. | `src/ServerGUI.java:19-60`, `src/GuiStatsComponent.java:9,25,36`. |
| (none for chunkgen) | Chunk gen/decor is inline on the server thread (§1.3). | `src/ChunkProviderServer.java:48-100`, `src/ChunkProviderGenerate.java:216-221`. |

### Client side `[client: reconstructed; mirrored by C++ port]`

| Thread | Purpose | C++ mirror |
|---|---|---|
| **Client thread** (main/render thread) | The entire `run()/gameLoop()`: input, ticks, packet dispatch, chunk render maintenance, all GL, all Iris pipeline. | `Minecraft.cpp:749-843`; `Minecraft.hpp:168` `util::Timer timer{20.0f}` |
| Integrated-server thread ("Server thread") | Runs `MinecraftServer.run()` at 20 TPS (§1.4). | `client/host/ServerProcessCoordinator.cpp` (external process) |
| Resource/skin download thread | Downloads `http://s3.amazonaws.com/MinecraftResources/...` into `resources/`; results consumed on the main thread. | `client/resource/ResourceDownloadThread.cpp:85-120` (std::thread, `loadFromUrl` 143-171) |
| Session/login check thread | Verifies session id against minecraft.net. | `client/session/SessionValidator.cpp:17-51` (`SessionCheckThread`, detached) |
| Sound mixing thread(s) | Java `SoundManager`/SoundSystem (OpenAL) play/update off-thread; main thread calls `SoundManager.update()`/listener once per tick/frame. (Java classes absent; C++ replaced with native device engine.) | `client/platform/audio/AudioEngine.cpp:143-320` (`start`, `tick`, mutex-protected device) |
| Network read/write threads | Same `NetworkManager` pairs as server (§4). | `client/multiplayer/ClientNetworkHandler.cpp:84-90` (tick-driven), `MultiplayerConnector.cpp:17` (connect thread) |

---

## 3. Iris 26.1 threading

### 3.1 Iris assumes ONE GL context thread and ONE game loop — enforced, not optional

- `gl/IrisRenderSystem.java` class javadoc (mirror `third_party/mcp/iris/gl/IrisRenderSystem.java:42`):
  *"This class is responsible for abstracting calls to OpenGL and asserting that calls are run on the render thread."*
  48 call sites do `RenderSystem.assertOnRenderThread()` (`IrisRenderSystem.java:80,90,100,105,112,118,...,544,549,554`).
- `gl/program/ProgramBuilder.java:36` (`begin`) and `:72` (`beginCompute`) both assert the render thread. Every Iris
  shader program — gbuffer, composite, deferred, shadow, compute, final — is built through `ProgramBuilder`, i.e.
  **all GL program compile/link is render-thread-only**.
- `pipeline/programs/ShaderCreator.java:171-194` `link()` does `GlStateManager.glCreateProgram()` →
  `createShader()` (`glShaderSource`/`glCompileShader`, 209-228) → `glLinkProgram` synchronously on the caller.
  `create()` (65-167), `createFallback()` (230-262), `createShadow()` (294-345) all call `link()` inline.
- Result: **shader compilation, FBO creation, render-target creation, buffer storage, image binding — everything GL —
  is single-threaded on the render thread.** There is no Iris GL worker thread.

### 3.2 What Iris DOES offload (bounded, non-GL, and it is NOT a general worker pool)

1. **Pack SOURCE reading only** — `shaderpack/programs/ProgramSet.java:64-90`:
   `try (ExecutorService service = Executors.newFixedThreadPool(10))` reads all gbuffer `.vsh/.fsh/.gsh/.tcs/.tes`
   sources in parallel (82: `service.submit(() -> readProgramSource(...))`), then blocks on `Future.get()` on the
   calling thread (86) and **shuts the pool down** (try-with-resources) before returning. This is pure file
   read + `ProgramSource` construction (no GL). It runs on the render thread during pack load; the join makes it
   deterministic. **Faithful C++ equivalent: parallelize the pack-file read/parse step with a bounded pool, join on
   the main thread; do NOT keep a long-lived executor.**
2. **Driver-level parallel shader compilation** — `Iris.java:133-137`:
   `glMaxShaderCompilerThreadsKHR(10)` / `ARBParallelShaderCompile.glMaxShaderCompilerThreadsARB(10)` during
   `onRenderSystemInit()`. This is a **GL driver feature inside the single GL context**, not a Java thread: the driver
   may compile shaders on its own threads while the app continues. The C++ port may do the same via GLCore if the
   driver exposes it — it changes no threading contract, only latency.
3. **PBR texture reload hooks the VANILLA resource-reload pipeline** — `mixin/texture/MixinTextureManager.java:27-35`
   injects at TAIL of `TextureManager.reload` lambdas to reload `TextureFormatLoader` + clear `PBRTextureManager` +
   bump `CapturedRenderingState.INSTANCE.incrementTextureReloadCount()`. Vanilla's resource reload is a
   multi-stage async pipeline (background executor decode → game/render-thread upload). Iris does NOT upload GL
   textures on a worker; it piggybacks the vanilla reload whose upload step is render-thread. (Texture reload counter
   is what packs use for `texture.setFrame`-style per-reload state; keep a monotonic counter.)
4. **UI/dialog and HTTP housekeeping** — `gui/FileDialogUtil.java:19,37` single-thread executor for the native file
   dialog; `UpdateChecker.java` uses `CompletableFuture` for the update HTTP check. Irrelevant to the render loop.
5. **Distant Horizons (DH) compat** — when DH is loaded, DH runs its own chunk-generation threads
   (`compat/dh/...`; the DH API world generators in `third_party/iris/.../com/seibel/distanthorizons/...`), but Iris
   itself only consumes DH's textures/matrices on the render thread. Not a C++ target; note only.

### 3.3 Iris's per-frame state capture (render thread, once per frame)

- `mixin/MixinGameRenderer.java:50-56` `iris$startFrame` (HEAD of `GameRenderer.render`): captures
  `CapturedRenderingState.INSTANCE.setRealTickDelta(...)`, starts `SystemTimeUniforms.COUNTER/TIMER.beginFrame`.
- `mixin/MixinLevelRenderer.java:121-147` `iris$setupPipeline` (HEAD of `renderLevel`): `IrisTimeUniforms.updateTime()`,
  `CapturedRenderingState.setGbufferModelView/setGbufferProjection`, `setTickDelta(fakeTickDelta)` (128-129),
  `preparePipeline(dimension)` (136), `pipeline.beginLevelRendering()` (140).
- `uniforms/FrameUpdateNotifier.java` is a plain listener list; `onNewFrame()` is invoked exactly once per frame at the
  top of `beginLevelRendering` (`IrisRenderingPipeline.java:938`), which is what advances `SystemTimeUniforms`
  frame/time counters. All of this is render-thread state.

### 3.4 Pipeline lifecycle / reload timing (render thread)

- `pipeline/PipelineManager.java:27-48` `preparePipeline(dimension)`: first visit per dimension resets
  `SystemTimeUniforms.COUNTER/TIMER` (29-30), then `pipelineFactory.apply(dimension)` (33) — which constructs the whole
  `IrisRenderingPipeline` (all GL programs, §3.1) synchronously on the caller. Lazily created; the mixin ensures it
  runs before Sodium initializes on dimension change (`mixin/MixinMinecraft_PipelineManagement.java:46-58`).
- `PipelineManager.destroyPipeline()` (`PipelineManager.java:83-93`) unbinds all 16 texture units then destroys
  pipelines; **must be immediately followed by re-prepare or state is inconsistent** (javadoc 72-82, cites
  Iris#1330). The unbind loop is at `PipelineManager.java:95-112`.
- Pack reload (`Iris.java:564+` `reload()`) runs from `handleKeybinds` (`Iris.java:173-213`, i.e. the game loop, render
  thread) and from the shader screen (`gui/screen/ShaderPackScreen.java:604,626`). `onLoadingComplete`
  (`Iris.java:159-171`) pre-builds the overworld pipeline at the title screen (Iris#323).

---

## 4. How Java multiplayer networking threads interact with the tick loop

Architecture (shared by client and server; the repo has the server+shared classes, the client mirrors the same shape):

- **Socket I/O is on the per-connection read/write threads; all packet HANDLING is on the sim/tick thread.**
  - Writer thread: `NetworkWriterThread.run()` drains `dataPackets`/`chunkDataPackets` (paced via
    `chunkDataSendCounter`/`creationTimeMillis`, `NetworkManager.java:82,94`; `Packet.java:191`) and flushes.
  - Reader thread: `NetworkReaderThread.run()` → `readPacket()` pushes parsed `Packet` objects onto the
    `Collections.synchronizedList` `readPackets` (`NetworkManager.java:23,125-150`).
  - **Queue discipline:** `dataPackets` before `chunkDataPackets`; chunk-data packets (Packet50/51) are throttled to
    1-in-`field_20175_w` (50) slots so big chunks don't starve time-sensitive packets (`NetworkManager.java:94-105`).
    C++ must keep this priority (it changes perceived multiplayer chunk streaming, which affects shaderpack `colortex`
    behavior only via scene content, but is part of vanilla parity).
- **The tick loop drains the queue on the sim thread:**
  - Server: `NetworkListenThread.handleNetworkListenThread()` (`src/NetworkListenThread.java:53-93`) → per player
    `NetServerHandler.handlePackets()` (`src/NetServerHandler.java:40-48`) → `networkManager.processReadPackets()` →
    up to **100 packets per call**, then `packet.processPacket(netHandler)` on the caller (sim) thread
    (`NetworkManager.java:191-218`, loop at 208). Called every server tick (`MinecraftServer.java:381`).
  - Client: the client's `Minecraft` game loop calls `NetClientHandler.handlePackets()` → same
    `networkManager.processReadPackets()` `[client: reconstructed; mirror: C++ ClientNetworkHandler::tick() at
    `src/net/minecraft/client/multiplayer/ClientNetworkHandler.cpp:84-90` → `connection_->tick()`, invoked from
    `ClientWorld::tick()` per `ClientNetworkHandler.hpp:141-143`]`.
  - Therefore **every `handle*` method of `NetHandler` (client: `NetClientHandler`) runs on the sim thread**, inside a
    world tick. `handleMapChunk` (Packet51) → client `world.setChunkData` → chunk render rebuild is therefore also on
    the sim/main thread (`C++ WorldPacketHandlers.cpp:62-74` `handleChunkData` → `world->handleChunkDataUpdate`).
- **Concurrency surface is tiny and explicit:** the only cross-thread structures are the three
  `Collections.synchronizedList` queues + `sendQueueLock` (`NetworkManager.java:21-25,64`), plus the
  `threadSyncObject` counters and `volatile` `field_973_b` in `NetworkListenThread.java:113`. Everything else is
  thread-confined to the sim thread (server: "Server thread"; client: "Client thread").
- **Disconnect paths spawn watchdog threads** (`NetworkMasterThread`, `ThreadMonitorConnection`, §2) that only nudge
  the socket threads; the actual handler-side disconnect (`handleErrorMessage`) runs on the sim thread
  (`NetworkManager.java:214-217`).
- **Login** (`NetLoginHandler.tryLogin` on the sim thread; `ThreadLoginVerifier` HTTP off-thread, result back on sim
  thread, `NetLoginHandler.java:98`) — client-side session check mirrors this with a detached thread
  (`SessionValidator.cpp:51`).

---

## 5. Timing / render-order contract for shaderpack parity (Iris 26.1)

Runtime order inside one `GameRenderer.render` frame (all render-thread; `third_party/mcp/iris/`):

1. `GameRenderer.render` HEAD → frame counters begin (`MixinGameRenderer.java:50-56`).
2. `renderLevel` HEAD → capture matrices/tickDelta, `preparePipeline`, `beginLevelRendering()`
   (`MixinLevelRenderer.java:121-147`).
3. `beginLevelRendering()` (`IrisRenderingPipeline.java:875-1025`):
   - image clears (892), shadow depth+color clears + shadow **compute** dispatches (894-933),
   - `PBRTextureManager.onNewFrame()` (935),
   - `updateNotifier.onNewFrame()` (938) — advances `SystemTimeUniforms` once/frame,
   - `customUniforms.update()` (941),
   - screen-resize check → `renderTargets.resizeIfNeeded` + all composite renderers `recalculateSizes()` (948-970),
   - main-target clears (979-994), rebind main FBO (1003),
   - setup computes only on resize (1006-1020),
   - **`beginRenderer.renderAll()`** (1022) — BEGIN passes. `isBeforeTranslucent=true` (1024).
4. `renderShadows(...)` (`IrisRenderingPipeline.java:1028-1034`): `shadowRenderer.renderShadows(...)` then
   **`prepareRenderer.renderAll()`** (1033). Called BEFORE the main terrain pass (`MixinLevelRenderer.java:199-203`).
   - `ShadowRenderer.renderShadows` (`shadows/ShadowRenderer.java:384-644`): sets static
     `ACTIVE/RESOLUTION` (396-397), builds shadow modelview/projection (416-435), toggles `smartCull`, forces a chunk
     re-cull with the shadow frustum (`needsUpdate()` 480, `invokeCullTerrain` 483), renders solid terrain
     (508-512), shadow callbacks (514-519), entities (521-586), block entities (576-580), copies pre-translucent depth
     (588), translucent terrain (598-602), generates mipmaps (610), restores player projection (604), restores state
     (615-623), then **`compositeRenderer.renderAll()` for shadowcomp** (631-633). Uses the frame's
     `CapturedRenderingState.getTickDelta()` (530) — same partial tick as the main frame.
5. Sky / clouds / terrain / entities / particles / weather render (gbuffer programs, phases via
   `MixinLevelRenderer.java:211-247`). **These gbuffer passes read render targets at the `flipped` snapshot**
   `() -> isBeforeTranslucent ? flippedAfterPrepare : flippedAfterTranslucent`
   (`IrisRenderingPipeline.java:364-365`, `694`, `737`).
6. `beginTranslucents()` (`IrisRenderingPipeline.java:1060-1085`): copy pre-translucent depth (1071),
   `isBeforeTranslucent=false` (1067), **`deferredRenderer.renderAll()`** (1073).
7. `finalizeLevelRendering()` (`IrisRenderingPipeline.java:1088-1093`): **`compositeRenderer.renderAll()`** (1091),
   **`finalPassRenderer.renderFinalPass()`** (1092).
8. `finalizeGameRendering()` (`IrisRenderingPipeline.java:1096-1098`): color-space conversion of the main target
   (after `renderLevel` returns, `MixinGameRenderer.java:84-87`).

**Buffer-flip semantics (the shaderpack-visible "who reads whom" state) — `targets/BufferFlipper.java` + `CompositeRenderer.java`:**
- `BufferFlipper` toggles membership (`flip` 15-20); `isFlipped` 29; `snapshot()` 37.
- The flip SEQUENCE is computed **once at pipeline construction** (a single shared `BufferFlipper` advanced stage by
  stage): BEGIN renderer flips its draw buffers (constructor 122-202), then
  `flippedBeforeShadow = flipper.snapshot()` (`IrisRenderingPipeline.java:339`); PREPARE flips →
  `flippedAfterPrepare` (346); DEFERRED flips → `flippedAfterTranslucent` (353); COMPOSITE flips → final
  (`CompositeRenderer` ctor + `FinalPassRenderer` at 355-362).
- Per pass: `flipped = bufferFlipper.snapshot()` BEFORE creating the pass program (132), each draw buffer flipped AFTER
  (178-179), explicit flips honored (`explicitFlips`, 174-176, 182-187), `flippedAtLeastOnce` tracked (120, 179, 185),
  `stageReadsFromAlt = flipped` baked per pass (192), FBO built from that snapshot (163).
- **Because BEGIN passes run before the shadow map, their shadowtex reads see LAST FRAME's shadow textures; PREPARE and
  later stages read THIS frame's.** This is the single most subtle timing constraint for the C++ refactor
  (CONTEXT D2: "begin passes must receive LAST FRAME shadow textures"). It falls out of the call order in §5 steps 3-4
  plus the construction-order snapshots; do not "fix" it by moving the shadow map earlier.
- `renderAll()` (270-358): for each pass, run computes first + `memoryBarrier` (287-298), mipmap setup (307-313),
  bind framebuffer + blend override (510-518), fullscreen quad draw (323-329); at the end unbind ALL texture units
  (343-350) — required for pack reload hygiene (also `PipelineManager.resetTextureState`, §3.4).

---

## 6. Guidance for the C++ refactor

### MUST PRESERVE (the single-main-thread contract)
1. One render thread owns: all GL calls (IrisRenderSystem asserts this), the frame loop (tick→render), packet
   dispatch, world sim, chunk render maintenance, and the entire Iris pipeline (construction, render order, flips,
   reloads). Do not introduce a second GL-touching thread.
2. Exact per-frame cadence: `timer.advance()` → `N=ticksThisFrame` sim ticks → 1 render with `partialTick`
   (cap N at 10). Keep one `partialTick` value shared by world interpolation + all Iris `tickDelta`/`frameTime`
   uniforms. `Minecraft.cpp:784-807` already does this — don't split tick/render onto different threads.
3. Render ORDER within the frame: **clear → BEGIN → shadow map → shadowcomp → PREPARE → gbuffers → deferred →
   composite → final → colorspace**, with depth copies and `beginHand`/`beginTranslucents` boundaries in the same
   places (`IrisRenderingPipeline.java:1022,1033,1073,1091-1092`; C++ `GameRenderer.cpp:1106,1122,1158,1172,1296`).
4. BufferFlipper parity: flip-state snapshots computed at pack-load time, stage-ordered
   (`flippedBeforeShadow`/`flippedAfterPrepare`/`flippedAfterTranslucent`), gbuffer samplers keyed on the
   `isBeforeTranslucent` switch, and **BEGIN reads last frame's shadow textures** (§5).
5. Per-frame "once" events in order: `PBRTextureManager.onNewFrame()` → `updateNotifier.onNewFrame()` (frame/time
   uniforms advance exactly once, before any pass) → `customUniforms.update()` → resize handling → clears → BEGIN
   (`IrisRenderingPipeline.java:935-1022`).
6. Pack load/reload + dimension change stay on the render thread, in the destroy→(immediately)re-prepare pattern with
   the 16-unit unbind first (`PipelineManager.java:83-112`). Keep a monotonic texture-reload counter
   (`CapturedRenderingState.incrementTextureReloadCount`, `MixinTextureManager.java:34`).
7. Static "currently rendering shadow" state (Java `ShadowRenderer.ACTIVE/RESOLUTION`, `ShadowRenderer.java:83-84`)
   scoped to the shadow map window, and phase tracking (`WorldRenderingPhase`) scoped to each render stage
   (`MixinLevelRenderer.java:211-247`) — both render-thread state used by uniforms and Sodium.
8. Network queue discipline: read/write threads only touch the three synchronized packet lists; ALL
   `handle*` dispatch on the sim thread, ≤100 packets per drain, data-before-chunk priority (§4).

### MAY BE PARALLELIZED (safe because it produces no GL and is joined before use)
- Pack SOURCE read/parse during load — bounded `fixedThreadPool(10)`-style, joined on the render thread
  (Iris precedent, `ProgramSet.java:64-90`). This includes `.vsh/.fsh/.gsh/.tcs/.tes/.csh` reads, directive
  scanning, and include resolution IF the result is pure `ProgramSource` data (no GL objects).
- Any CPU work with no shared mutable state and no cross-thread visibility requirement: pure string/JSON/GLSL text
  transforms that are inputs to (not outputs of) GL calls. (Do NOT parallelize `TransformPatcher` output if it feeds
  sampler/`flipped` snapshot state that must be deterministic per pack — it's cheap anyway.)
- Background I/O that only produces files/bytes consumed later on the main thread: resource downloads
  (`ResourceDownloadThread.cpp:89`), skin/image downloads, session checks — all already threaded in the C++ port.
- Graphics-API INDEPENDENT pack pre-validation (e.g. syntax checks that don't create GL objects) IF a failure is
  surfaced on the render thread at the same point it would be in Java (construction-time error).
- If the GL driver supports `GL_KHR_parallel_shader_compile`/`ARB_parallel_shader_compile`, hint
  `glMaxShaderCompilerThreads(10)` once at renderer init (Iris.java:133-137) — the driver threads stay inside the one
  context/thread; the C++ layer adds no threads for this.

### MUST NOT BE PARALLELIZED
- ANY OpenGL call (IrisRenderSystem/ProgramBuilder assert render-thread; `IrisRenderSystem.java:42`). This includes
  shader compile/link (`ShaderCreator.link`), FBO/render-target creation, buffer/image binding, samplers, texture
  upload, `glDispatchCompute`, and `RenderSystem` state. Even DSA calls must stay on the one context thread.
- The per-frame flip/`flipped` snapshot evaluation and the render order itself — they are a serial dependency chain.
- Any state the shaderpacks observe as frame-global and monotonic: frame counter, `SystemTimeUniforms`, texture reload
  counter, tickDelta. Splitting their producers across threads would desync packs.
- The `BufferFlipper` sequence (construction-time, stage-ordered) — it defines inter-stage data flow.
- Per-connection packet write ordering (writer thread is already serial per socket) and the sim-thread drain ordering.
- Anything in `PipelineManager.destroy → re-prepare` (must be atomic on the render thread).

---

## 7. Shared-file edits

None. Council stage: no files outside this notes file were modified.

---

## Appendix — verified file:line index (all under `C:\Users\Eddie\Documents\New project 2\`)

Vanilla server/shared (`third_party/mcp/net/minecraft/`):
- `server/MinecraftServer.java` — 61 (ThreadSleepForever), 68-70 (ThreadCommandReader), 95 (NetworkListenThread), 159-198 (spawn pregen), 251-339 (run loop; 257-288 timing; 282-287 50 ms budget; 288 sleep), 341-401 (doTick; 376 world tick; 381 network), 433 (ThreadServerApplication).
- `src/ThreadServerApplication.java:9-24`; `src/ThreadCommandReader.java:19-34`; `src/ThreadSleepForever.java:9-33`; `src/ThreadLoginVerifier.java:15-51`; `src/ThreadMonitorConnection.java:20-35`; `src/NetworkMasterThread.java:20-46`; `src/NetworkAcceptThread.java:25-57`.
- `src/NetworkListenThread.java:29-34,53-93,113`; `src/NetServerHandler.java:40-48`; `src/NetHandler.java:23-288` (dispatch surface).
- `src/NetworkManager.java:21-25,47-50,58-75,77-117,125-150,160-189,191-218,225-231`.
- `src/NetworkReaderThread.java:20-45`; `src/NetworkWriterThread.java:22-66`.
- `src/World.java:1693-1714,1797`; `src/ChunkProviderServer.java:48,63,93,100`; `src/ChunkProviderGenerate.java:216-221`; `src/RegionFileCache.java:22,48`; `src/RegionFile.java:121,187`; `src/Packet.java:32,191`.
- `src/NetLoginHandler.java:98`; `src/ServerGUI.java:19-60`; `src/GuiStatsComponent.java:9,25,36`.

Iris (`third_party/mcp/iris/`, line-identical to `third_party/iris/common/src/main/java/net/irisshaders/iris/`):
- `gl/IrisRenderSystem.java:42,80-554` (assertOnRenderThread ×48).
- `gl/program/ProgramBuilder.java:36,72`; `pipeline/programs/ShaderCreator.java:65-167,171-194,209-228,230-262,294-345`.
- `shaderpack/programs/ProgramSet.java:64-90`.
- `pipeline/PipelineManager.java:27-48,72-112`; `pipeline/IrisRenderingPipeline.java:311,334-365,519-605,875-1025,1028-1034,1060-1085,1088-1098,1354-1363`.
- `pipeline/CompositeRenderer.java:122-202,207-226,270-358,480-518`; `targets/BufferFlipper.java:15-39`.
- `shadows/ShadowRenderer.java:83-84,384-644` (renderShadows; 396-397 ACTIVE; 483 cull; 530 tickDelta; 631-633 shadowcomp).
- `mixin/MixinLevelRenderer.java:121-147,153-163,168-187,199-203,211-247,277-282`.
- `mixin/MixinGameRenderer.java:50-56,84-87`; `mixin/MixinMinecraft_PipelineManagement.java:46-58`; `mixin/MixinRenderSystem.java:25-32`; `mixin/texture/MixinTextureManager.java:27-35`.
- `Iris.java:126-150,133-137,159-171,173-213,564+`; `uniforms/FrameUpdateNotifier.java` (whole file); `uniforms/CapturedRenderingState.java`; `vertices/ImmediateState.java:16` (ThreadLocal); `gui/FileDialogUtil.java:19,37`; `UpdateChecker.java`; `pbr/texture/PBRTextureManager.java`.

C++ port cross-refs (`src/net/minecraft/client/`):
- `util/Timer.hpp:14-71`; `Minecraft.cpp:749-843,593-669,555-592,670-714`; `Minecraft.hpp:3,168`.
- `render/GameRenderer.cpp:1103-1172,1296` (pipeline order mirror), `multiplayer/ClientNetworkHandler.cpp:84-90`,
  `multiplayer/WorldPacketHandlers.cpp:62-74`, `resource/ResourceDownloadThread.cpp:85-171`,
  `session/SessionValidator.cpp:17-51`, `host/ServerProcessCoordinator.cpp:104-159` (external-process divergence).
