# Council: Main-Thread Map (what the main thread does each frame, where it blocks, what is asyncable)

Review-only council output. NO code changes, NO builds. All line numbers verified against the
working tree on 2026-08-01.

## 0. Thread inventory (context)

The main thread is the ONLY game thread. Everything that exists in modern Minecraft as separate
threads already exists HERE as worker threads, but they are all *feeder* threads that hand work
back to the main thread to apply. The main thread still does: world simulation (tick), packet
apply, chunk decorate, all GL, all entity/AI CPU, all culling, all uniform upload, audio listener
push, Lua hooks, GUI.

Background threads that already exist:
- `Connection` reader/writer socket threads — `Connection.cpp:131-132` (readLoop 262, writeLoop 282)
- `LightingEngine` workers — `LightingEngine.cpp:52-56` (threadLoop 187); fully async, main only drains results
- `ChunkCache` loader pool — `ChunkCache.cpp:212-219` (requestChunkAsync 226 → produceChunk on worker)
- `ChunkCache` save pool — `ChunkCache.cpp:220-225` (enqueueSerializedWrite 299/322)
- `ChunkMeshScheduler` worker pool (chunk meshing) — `ChunkBuilder.hpp:120-157` (buildMesh on worker)
- `ShaderCompileService` workers — `ShaderCompileService.cpp:41-63`
- `ResourceDownloadThread` worker — `ResourceDownloadThread.cpp:85-120`
- `ImageDownload` detached thread per skin/cape URL — `ImageDownload.cpp:7-18`
- `MultiplayerConnector` connect thread (socket + auth) — `MultiplayerConnector.cpp:17-61`
- `ClientNetworkHandler::joinServerThread_` (session auth) — `ClientNetworkHandler.cpp:247-256`
- `SessionValidator` detached check thread — `SessionValidator.cpp:17-48,50-51`
- msauth saved-account restore worker — `SessionRestore.cpp:71-91`
- XAudio2 decode threads (enqueue-only from main) — `XAudio2Backend.cpp:669-689`

## 1. Per-frame timeline of the main thread

Everything below is one iteration of `Minecraft::run()` (`Minecraft.cpp:749-843`). There is NO
explicit frame-rate sleep in the loop: pacing comes only from the vsync'd `SwapBuffers` inside
`pumpAndPresent()`. With `fpsLimit=0` (Unlimited) the loop is an unthrottled busy loop
(`GLCore.cpp:297-318`, `FramePacing.hpp:5-10`).

### Loop top (before timer)
1. `diagnostics::pingMainLoopHeartbeat()` — `Minecraft.cpp:763` (Windows hang-watchdog)
2. `screenStack_.flushRetired()` — `Minecraft.cpp:765` → `ScreenStack.cpp:64` frees last frame's retired screens
3. `multiplayerSession_.flushRetired()` — `Minecraft.cpp:769` → `MultiplayerSession.cpp:17` frees retired bridges (destroys `Connection` → joins its reader/writer threads — see §2.9)
4. Bridge teardown check — `Minecraft.cpp:770-775`: if active bridge handler is gone/disconnected, `retireBridge()`
5. Applet active check / window close check — `Minecraft.cpp:776-783`
6. `luaHookTickRate` — `Minecraft.cpp:784-787` (Lua may change tps/tpsScale)

### Timer advance (partial-tick math) — `Minecraft.cpp:788-794`, `Timer.hpp:14-48`
- `timer.advance()` once per loop iteration, NOT per tick.
- Computes `ticksThisFrame` (0..10, clamp at `Timer.hpp:44-46`) and `partialTick` (`Timer.hpp:47`).
- When `paused && world != nullptr`: advances the timer but **freezes partialTick** (`Minecraft.cpp:788-791`).
- `ticksThisFrame>10` is hard-capped (`Timer.hpp:44`); Java's cap is 10 too.

### Tick loop — `Minecraft.cpp:796-805`
`for(i in 0..ticksThisFrame-1)`: `++ticksPlayed; tick();` (see §4 for the full tick() subtree).
- Catch: `SessionLockException` → `world=nullptr; setWorld(nullptr); setScreen(WorldSaveConflictScreen)` (`Minecraft.cpp:799-804`).

### Render phase — `Minecraft.cpp:807` → `runRenderPhase()` (`Minecraft.cpp:670-747`)
Exactly once per loop iteration, regardless of how many ticks ran. `partialTick` interpolates
between the last tick and the next. Runs even with `ticksThisFrame==0`.
Order inside runRenderPhase:
1. `audio.updateListener(player, partialTick)` — `Minecraft.cpp:679`
2. `world->doLightingUpdates()` — `Minecraft.cpp:681` (drains lighting-worker results → `events_.setBlocksDirty` → WorldRenderer markDirty)
3. `world->pumpChunkPublish()` — `Minecraft.cpp:682` → `ChunkCache::pumpChunkPublish` `integrateFinishedLoads(32)` — `ChunkCache.cpp:472-474`
4. `setSwapPacing(...)` — `Minecraft.cpp:686` (applies interval 0/1/-1 via wglSwapInterval)
5. `pumpAndPresent()` — `Minecraft.cpp:688` → `DisplayManager.cpp:158-161` → `glfwPollEvents` + **`present()` (SwapBuffers — THE vsync block)**
6. `player->isInsideWall()` → force third person — `Minecraft.cpp:690-692`
7. If `!skipGameRender`: `luaHookRenderFrame` → `interactionManager->update(partialTick)` → `gameRenderer->onFrameUpdate(partialTick)` (`Minecraft.cpp:693-702`) — this is the ENTIRE render (see §4)
8. Window inactive: `sleep_for(10ms)` and auto-untoggle fullscreen — `Minecraft.cpp:705-710`
9. F3 profiler: `renderProfilerChart` or `recordFrameTime` — `Minecraft.cpp:712-716`
10. `toast.tick()` — `Minecraft.cpp:717`
11. `handleScreenshotKey()` (F2 → Screenshot::take, sync) — `Minecraft.cpp:718,407-416`
12. `++frames` — `Minecraft.cpp:719`
13. **`paused = !isWorldRemote() && currentScreen()!=nullptr && currentScreen()->shouldPause()`** — `Minecraft.cpp:720` (applied at END of frame; gates next frame's world sim)
14. FPS window text update — `Minecraft.cpp:721-727`
15. Stall trace >200ms → append to `stall-trace.log` — `Minecraft.cpp:728-747`

### Stall detection (after render) — `Minecraft.cpp:808-825`
`frameEnd - tickStart > 250ms` → append FRAME line to `stall-trace.log` (sync ofstream on main thread).

### Loop exit
Exception at any point: `SessionLockException` → world teardown+conflict screen; `bad_alloc` →
`cleanHeap()` + OutOfMemoryScreen + `break` (`Minecraft.cpp:826-835`). Outer catch → `cleanHeap()`,
`gameCrashed(...)` (`Minecraft.cpp:837-841`). Then `stop()` (`Minecraft.cpp:842` → 347-393).

### Render/render interleave vs ticks — key facts
- Ticks run before render each loop. Multiple ticks can run back-to-back before one render.
- Present happens at the START of runRenderPhase (previous frame's buffer), render after it. So
  the vsync block and the frame's GL work are back-to-back on the same thread → a slow render
  directly lengthens the next vsync wait and vice versa (no pipelining).
- During world load / dimension travel / respawn, `ProgressRenderer` renders + presents *nested
  inside* the world-gen stack (see §4, `ProgressRenderer.cpp:63-98,99-114`), i.e. loading screens
  are drawn from inside `tick()`, before the normal render phase.

## 2. Every blocking call on the main thread

### 2.1 Explicit sleep
| Where | Call | When |
|---|---|---|
| `Minecraft.cpp:709` | `std::this_thread::sleep_for(10ms)` | Every frame while the window is inactive |
| `DisplayManager.cpp:66-67` | `std::this_thread::sleep_for(1s)` | Window create retry at startup (only on first failure) |
| `ProgressRenderer.cpp:109-110` | 20ms gate (not a sleep, a rate-limit) | Loading-screen re-render during world gen |

No other sleeps on the client main thread. (The server loop sleeps 1ms/tick — `MinecraftServer.cpp:448`.)

### 2.2 Synchronous joins of worker threads (main thread)
| Where | Call | When |
|---|---|---|
| `ClientNetworkHandler.cpp:51-53` | `joinServerThread_.join()` | Handler destruction (bridge teardown) |
| `ClientNetworkHandler.cpp:243-245` | `joinServerThread_.join()` | `beginPendingLogin` before starting a new auth worker (usually a finished worker) |
| `SessionRestore.cpp:241-243` | `gSavedAccountRestore.worker.join()` | `tickRestoreSavedAccount` when the restore finished (fast reclaim) |
| `MultiplayerConnector.cpp:65-67` | `thread_.join()` | Connector destruction (screen retirement); may wait on a mid-connect worker (cancel flag polls every 100ms → bounded ~100ms) |
| `ShaderCompileService::stop` (`ShaderCompileService.cpp:70-71`) | `worker.join()` ×N | `Minecraft::stop` (`Minecraft.cpp:354`) |
| `ResourceDownloadThread` dtor (`ResourceDownloadThread.cpp:81-83`) | `worker_.join()` | stop() |
| `SessionValidator.cpp:51` | detached (NOT joined) | — |

### 2.3 Socket thread joins (bridge/connection teardown)
| Where | Call | When |
|---|---|---|
| `Connection.cpp:352-360` `joinThreads()`; `Connection.cpp:134-137` (dtor) / `:157-160` (disconnect) | `reader_.join(); writer_.join()` | Destroying a bridge/connection. `requestDisconnect` (`Connection.cpp:334-344`) does `shutdown(SD_RECEIVE)` first, which unblocks `recv()` in `readLoop` (`Connection.cpp:262-281`); writer wakes on `writeCv_` with 20ms timeout (`Connection.cpp:290-292`). So joins are normally quick but the main thread is still paused for both socket threads. Fired from `MultiplayerSession::flushRetired` → bridge dtor — `Minecraft.cpp:769` |

### 2.4 Synchronous network I/O on main thread
| Where | Call | When |
|---|---|---|
| `ClientNetworkHandler.cpp:258-320` `downloadPendingMods` | `http::httpRequest` (blocking) + `writeFileBytes` + `buildZipIndex` + `parseManifestJson` + `host().rescan()` | `ServerModDownloadScreen::accept()` — `ServerModDownloadScreen.cpp:60-74`, invoked from the screen's button handler on the main thread. This is the worst offender: N sequential HTTP downloads + zip unpack + install, all on the main loop. |
| `TextureManager.cpp:320` `loadRasterFromUrl` → `resource::fetchUrl` | blocking HTTP | Any `http://` texture URL that wasn't routed to `ImageDownload` (async) is fetched synchronously here |

### 2.5 Synchronous file I/O / decode on main thread
| Where | Call | When |
|---|---|---|
| `TextureManager.cpp:451-483` `getTextureId` | GDI+ PNG decode (`loadRasterForResource` `:210-218` → `loadRasterFromFile` `:266-270`) + `glTexImage2D` | First request of ANY texture during gameplay (entities, items, GUI, atlas sub-assets, PBR companions `getCompanionTextureId` `:533-550`). Cached thereafter. |
| `TextureManager.cpp:419-442` `getColors` | sync raster decode | First color-map load (water/grass/foliage) at bootstrap |
| `Screenshot.cpp:48-99` `take` | `glReadPixels` + GDI+ PNG encode + `Bitmap.Save` disk write | F2 (in `handleScreenshotKey`, `Minecraft.cpp:411`) |
| `StatFile.cpp:71-88` `writeStatFile`; `PlayerStats.cpp:83-86` `save` | binary ofstream | Every `ScreenStack::setScreen` (`ScreenStack.cpp:33-38`!) and every `WorldSession::setWorld` (`WorldSession.cpp:72-74`) — sync stat save on every GUI change |
| `WorldStorageSource::getSaveLoader` / `getWorldProperties` (`Minecraft.cpp:899,906`) | region-file reads | World select / start |
| `AccountStorage.cpp:119-172` saveAccount / `:98-118` loadAccount | file read/write | Auth restore tick (`SessionRestore.cpp:262-267`), join |
| `Minecraft.cpp:740,820` stall-trace | `std::ofstream` append | Only when a stall already happened (>200/250ms) |
| `GameOptions::load` (`Minecraft.cpp:272`) | options.txt read | Startup only |

### 2.6 Chunk storage (synchronous disk) on main thread
| Where | Call | When |
|---|---|---|
| `ChunkCache::produceChunk` (`ChunkCache.cpp:106-125`) holds `ioMutex_` during `storage_->loadChunk` (`:111-112`) | region read | On the loader pool normally, but **synchronously on the main thread** whenever `World::getChunk`/`ChunkCache::getChunk` force-loads (`ChunkCache.cpp:202-211`), e.g. `prepareWorld` (`WorldSession.cpp:177-185`) |
| `ChunkCache::saveChunk` sync path (`ChunkCache.cpp:366-369`) / `enqueueSerializedWrite` fallback (`:299-308,322-333`) | region write | When storage lacks async writes |
| `ChunkCache::save(...,saveEntityData=true)` → `waitForPendingWrites()` (`ChunkCache.cpp:349-355,409-417`) | condition-variable wait on save pool + `storage_->flush()` | `World::savingProgress` (`World.cpp:581-589`) on world unload/save (from `setWorld`/`unloadWorld` — `WorldSession.cpp:48,86`) and `prepareForSave` (`ChunkCache.cpp:419-422`). BLOCKS until all pending async chunk writes drain. |
| `RegionIo::flush` (`RegionIo.hpp:47-65`) | flush every open .mcr file under mutex | `Minecraft::stop` (`Minecraft.cpp:370`) |

### 2.7 Chunk generation / decoration / lighting on main thread
| Where | Call | When |
|---|---|---|
| `WorldSession::prepareWorld` (`WorldSession.cpp:159-198`): `world->getChunk` ×~97 (`:177-185`) force-loads + generates synchronously; `relightSkylightForPreparedArea` (`:191` → `WorldSession.cpp:15-24`); `finishLightingUpdates`→`doLightingUpdates(MAX)` (`:192` → `World.cpp:718-723`); `tickChunks` (`:194` → `WorldChunks.cpp:436-439`) | full terrain gen + decorate + light + chunk tick | `setWorld` for local worlds and `respawnPlayer` (`Minecraft.cpp:1102`) and portal travel (`travelToDimension` → `setWorld` `Minecraft.cpp:996`). The nested loading-screen render (`ProgressRenderer.cpp:99-114`) interleaves. |
| `ChunkCache::adoptChunk` (`ChunkCache.cpp:156-201`): `populateBlockLight` (`:173`), `chunk->load()` (`:174`), **`decorate(...)` (`:179-195`)** | decorate (trees/ores/structures) on main thread | Every chunk that completes an async load and is integrated — `integrateFinishedLoads` (`ChunkCache.cpp:250-288`) → main-thread decorate inside `pumpChunkPublish`/`ChunkCache::tick` |
| `ChunkCache::decorate` (`ChunkCache.cpp:373-384`) | generator decorate under `ioMutex_` | see above |
| `World::doLightingUpdates` (`World.cpp:711-716`) | drains lighting outbox, `setBlocksDirty` | per frame — cheap (worker did the math) |
| `LightingEngine::stop` (`LightingEngine.cpp:726`) | joins workers | `World::~World` |

### 2.8 Progress/loading screen nested present
`ProgressRenderer::renderLoadingFrame` (`ProgressRenderer.cpp:63-98`) calls `DisplayManager::present()` (`:96`) — a **nested SwapBuffers/vsync** on the main thread inside world gen.

### 2.9 Shutdown-time blocking (main thread, once)
`Minecraft::stop` (`Minecraft.cpp:347-393`): ShaderCompileService stop (join workers `:354`), applet clearMemory, resourceDownload cancel, `setWorld(nullptr)` (→ `WorldSession::unloadWorld` `:42` → world save/`savingProgress`), `RegionIo::flush` (`:370`), `clearAllocatedTextures`, `serverProcessCoordinator_->shutdown()` (`:378` → `WaitForSingleObject` up to **10s+2s** `ServerProcessCoordinator.cpp:283-286`), `mod::host().shutdown()`, `audio.shutdown()`.

### 2.10 Mutex/condition waits on the main thread
- `ChunkCache::waitForPendingWrites` (`ChunkCache.cpp:349-355`) — see 2.6.
- `ShaderCompileService.cpp:167` `job->cv.wait(...)` — synchronous compile fallback; **only** if `!started()` (`ShaderCompileService.hpp:66-68`); normally never reached because `start()` runs at bootstrap (`Minecraft.cpp:268`).
- All other cross-thread handoffs are lock-free/atomic or bounded deque pops that never block the main thread (WorkerHandoff drain, LightingEngine outbox `drainDirtyRegions` `LightingEngine.cpp:113-123`).

## 3. Heavy-but-asyncable operations currently on the main thread

Ranked roughly by frame cost. These are the prime candidates for moving off the main thread.

1. **`World::tickEntities`** — `WorldEntities.cpp:386-500`. Serially ticks every global entity, entity (`updateEntity`), and block entity + block-entity queue. All AI, pathfinding, physics on main thread. (Beta-identical to Java, but this is the #1 sim cost.)
2. **`manageChunkUpdatesAndEvents`** — `WorldChunks.cpp:456-575`. Per active chunk (9-radius, ~361): 80 random block ticks (`:554-573`), ambient cave sounds (`:481-509`), snow/ice (`:524-553`), lightning (`:510-523`).
3. **`world->displayTick`** — `World.cpp:675-692`. 1000 random `block->randomDisplayTick` per tick (16-radius cube).
4. **`NaturalSpawner::tick` / `spawnMonstersAndWakePlayers`** — `NaturalSpawner.cpp:116,206`, called from `World::tick` `World.cpp:919` (and skip-night path `:906-917`).
5. **Packet apply / network drain** — `Connection::tick` `Connection.cpp:184-228`: up to 4096 packets / 3ms budget; `packet->apply(*handler)` on the main thread. Most expensive: `ChunkDataS2CPacket` → `handleChunkDataUpdate` (`WorldChunks.cpp:364-413`) → `chunk->loadFromPacket` + `setBlocksDirty`.
6. **`World::tick`** — `World.cpp:900-934`: `chunkCache_->tick()` (`ChunkCache.cpp:423-461`: integrate loads, autosave, up to 100 unloads, `storage_->tick`, `generator_->tick`), autosave `save()` + `chunkCache_->save(false)` (budget 8) `World.cpp:924-930`, `processScheduledTicks` `:932`.
7. **Chunk decorate on integrate** — `ChunkCache.cpp:179-195,373-384` (main-thread decorate of async-loaded chunks). Gen is async; decorate is NOT. Prime asyncable.
8. **`loadChunksNearEntity`** — `WorldEntities.cpp:501-532`, every tick via `tickJoinPlayerCounter` (`WorldSession.cpp:262-270`): `prefetchChunksNear` (`ChunkCache.cpp:475-510`, up to 16 new async requests + integrates).
9. **Chunk mesh upload** — `WorldRenderer::compileChunks` `WorldRenderer.cpp:727-825`: `builder->uploadMesh` (`ChunkBuilder.cpp:352-415`) → `ChunkRegionBuffer::upload` (glBufferData) + `ModChunkMesh::uploadToGpu`; time-budgeted 3-6ms + capture budget. Meshing itself IS on workers.
10. **Culling + occlusion** — `WorldRenderer::cullChunks` `WorldRenderer.cpp:949-1011` (+ `applyOcclusionCulling` `:1012`): per-frame frustum test over all sections + BFS occlusion + visible-ring rebuild.
11. **Terrain/entity/atmosphere draw** — `renderToCurrentTarget` `GameRenderer.cpp:948-1336`: `drawSolidTerrain`/`drawTranslucentTerrain` (`:824-853`), `renderEntities` (`WorldRenderer.cpp:826`+), sky `:1120-1126`, precipitation `:1301`, clouds `:1309-1318`, first-person hand `:1246-1248,1323-1327`, `Frustum::compute` `:1107`. All GL draw calls on main thread.
12. **Shader pack pipeline** — `GameRenderer.cpp:1071-1152,1257-1266,749-764`: `prepareFrame`, `setFrameUniforms`+`buildFrameUniforms`, `renderBegin`, `shadowmap::update`, `renderShadowComposite`, `renderPreWorld`, `renderDeferred`, `renderPostProcess`. Compile is async (ShaderCompileService); the per-frame uniform build + pass GL work is not.
13. **Lua hooks** — `luaHookClientTick`/`luaHookRenderFrame`/`luaHookRaycast`/`luaHookWorldTick`/`luaHookTickRate` (`LuaDirectHooks.cpp:329,336,376,512,549`) run the Lua VM on the main thread (ModHost). Arbitrary mod code cost.
14. **`gameRenderer->updateTargetedEntity`** — `GameRenderer.cpp:253-328` + `entityRaycast`/`boxRaycast`/model raycast; run once in `tick()` (`Minecraft.cpp:612`) AND again per render pass (`GameRenderer.cpp:967`).
15. **`audio.tick` / `updateListener` / `playAt`** — `AudioEngine.cpp:212,198,235`; cheap (queue + mutex) but main-thread.
16. **`textureManager.tick`** — `TextureManager.cpp:706-746`: glTexSubImage2D for every dynamic sprite (water/lava/fire/portal/clock/compass) + mipmap uploads.
17. **`worldSoundListener->tickWeather`** — `WorldSoundListener.cpp:47-115`: up to 100 block samples (`getTopSolidBlockY`+biome) per tick while raining.
18. **`world->doLightingUpdates` per frame** — cheap, but fans `setBlocksDirty` into WorldRenderer `markDirty` (`World.cpp:711-716`).
19. **Loading screens** — the whole `prepareWorld` + `convertAndSaveWorld` (`WorldSession.cpp:159-198,199-210`) block, and `WorldStorageSource::convert` (`Minecraft.cpp:895`) runs on the main thread.
20. **`world->setChunkCacheCenterFromBlockPos`** in render (`GameRenderer.cpp:998-999`) + `WorldRenderer::updateSectionFrontier`/`drainPendingColumns`/`createColumn` (`WorldRenderer.cpp:401,475`) — column bookkeeping per frame.

## 4. Call trees

### 4.1 `Minecraft::tick()` — `Minecraft.cpp:593-669`

```
tick()                                                   Minecraft.cpp:593
├─ serverProcessCoordinator_->tick()                     Minecraft.cpp:595 (ServerProcessCoordinator.cpp:237)
├─ luaHookClientTick(before)                             Minecraft.cpp:598 (LuaDirectHooks.cpp:329)
├─ msauth::tickRestoreSavedAccount()                     Minecraft.cpp:599 (SessionRestore.cpp:237; may join worker)
├─ audio.tick()                                          Minecraft.cpp:600 (AudioEngine.cpp:212)
├─ worldSoundListener->tickWeather()                     Minecraft.cpp:602 (WorldSoundListener.cpp:47)
├─ ticksPlayed==6000 → startSessionCheck()               Minecraft.cpp:604-606 (SessionValidator.cpp:50, detached)
├─ stats->tick()                                         Minecraft.cpp:608 (PlayerStats.cpp:88)
├─ inGameHud.tick()                                      Minecraft.cpp:610 (InGameHud.cpp:85)
├─ gameRenderer->updateTargetedEntity(1.0f)              Minecraft.cpp:612 (GameRenderer.cpp:253; raycasts)
├─ luaHookRaycast(raycastEvent)                          Minecraft.cpp:635 (LuaDirectHooks.cpp:376)
├─ if !paused: interactionManager->tick()                Minecraft.cpp:637-638 (SingleplayerInteractionManager.cpp:127)
│  └─ worldRenderer->miningProgress = …                  Minecraft.cpp:640
├─ textureManager.bindTexture(terrain)                   Minecraft.cpp:643
├─ if !paused: textureManager.tick()                     Minecraft.cpp:645 (TextureManager.cpp:706; dynamic sprite GL uploads)
├─ input::InputSystem::beginFrame()                      Minecraft.cpp:650 (InputSystem.cpp:368)
├─ screenStack_.tickScreens()                            Minecraft.cpp:651 (ScreenStack.cpp:68; Screen::tickInput + Screen::tick)
├─ if !world->isRemote(): multiplayerSession_.tick()     Minecraft.cpp:655-657 (MultiplayerSession.cpp:20 → ClientNetworkBridge.cpp:159 → ClientNetworkHandler.cpp:84)
│  └─ ClientNetworkHandler::tick()                       ClientNetworkHandler.cpp:84
│     ├─ processPendingJoinServer()                      ClientNetworkHandler.cpp:86 (non-blocking consume)
│     ├─ connection_->tick()                             ClientNetworkHandler.cpp:90 (Connection.cpp:184: 3ms/4096 packet drain + apply)
│     └─ connection_->interrupt()                        ClientNetworkHandler.cpp:91
├─ pendingScreenResize_ → resize()                       Minecraft.cpp:658-661
├─ input::InputSystem::pollGame()                        Minecraft.cpp:662 (InputSystem.cpp:522; mouse click dispatch → handleMouseClick/attack)
├─ luaHookClientTick(preSim)                             Minecraft.cpp:664
├─ runWorldSimulation()                                  Minecraft.cpp:665 (see below)
└─ luaHookClientTick(after)                              Minecraft.cpp:668
```

### 4.2 `Minecraft::runWorldSimulation()` — `Minecraft.cpp:555-592`

```
runWorldSimulation()                                     Minecraft.cpp:555
├─ ClientWorld: handler->applyDeferredRespawn()          Minecraft.cpp:561-565
├─ worldSession_.tickJoinPlayerCounter()                 Minecraft.cpp:567 (WorldSession.cpp:262)
│  └─ world->loadChunksNearEntity(player)                WorldEntities.cpp:501 → ChunkCache::prefetchChunksNear (ChunkCache.cpp:475)
├─ world->difficulty …; remote → difficulty=3            Minecraft.cpp:569-572
├─ if !paused:
│  ├─ gameRenderer->updateCamera()                       Minecraft.cpp:575 (GameRenderer.cpp:220)
│  ├─ weather lightning tickdown                          Minecraft.cpp:577-579
│  └─ world->tickEntities()                              Minecraft.cpp:580 (WorldEntities.cpp:386)
├─ if !paused || isWorldRemote():
│  ├─ world->allowSpawning()                             Minecraft.cpp:583
│  └─ world->tick()                                      Minecraft.cpp:584
│     ├─ [local] World::tick()                           World.cpp:900
│     │  ├─ luaHookWorldTick(before)                     World.cpp:903
│     │  ├─ updateWeatherCycles()                        World.cpp:905 (World.cpp:624)
│     │  ├─ canSkipNight → spawnMonstersAndWakePlayers   World.cpp:906-917 (NaturalSpawner.cpp:206)
│     │  ├─ NaturalSpawner::tick()                       World.cpp:919 (NaturalSpawner.cpp:116)
│     │  ├─ chunkCache_->tick()                          World.cpp:921 (ChunkCache.cpp:423: integrate(2)+autosave+unload100+storage/generator tick)
│     │  ├─ autosave every saveInterval_                 World.cpp:924-930 (save() + chunkCache_->save(false) budget 8)
│     │  ├─ setTime(nextTime)                            World.cpp:931
│     │  ├─ processScheduledTicks(false)                 World.cpp:932 (World.cpp:768)
│     │  └─ manageChunkUpdatesAndEvents()                World.cpp:933 (WorldChunks.cpp:456-575: 80 random ticks/chunk, sounds, snow/ice, lightning)
│     └─ [remote] ClientWorld::tick()                    ClientWorld.cpp:39
│        ├─ luaHookWorldTick                             ClientWorld.cpp:40-43
│        ├─ setTime(time+1)                              ClientWorld.cpp:44
│        ├─ spawn pending entities                       ClientWorld.cpp:45-52
│        ├─ networkHandler_->tick()                      ClientWorld.cpp:54 (ClientNetworkHandler.cpp:84 → Connection::tick drain)
│        ├─ apply blockResets                            ClientWorld.cpp:56-64
│        └─ luaHookWorldTick(after)                      ClientWorld.cpp:65-68
├─ if !paused: world->displayTick(player pos)            Minecraft.cpp:587 (World.cpp:675: 1000 random display ticks)
└─ if !paused: particleManager.removeDeadParticles()     Minecraft.cpp:590
```

### 4.3 `Minecraft::runRenderPhase()` — `Minecraft.cpp:670-747`

```
runRenderPhase(tickDuration, frames, fpsWindowStart)     Minecraft.cpp:670
├─ audio.updateListener(player, partialTick)             Minecraft.cpp:679 (AudioEngine.cpp:198)
├─ if world:
│  ├─ world->doLightingUpdates()                         Minecraft.cpp:681 (World.cpp:711: drain lighting outbox → setBlocksDirty)
│  └─ world->pumpChunkPublish()                          Minecraft.cpp:682 (World.cpp:576 → ChunkCache::pumpChunkPublish ChunkCache.cpp:472)
├─ setSwapPacing(fpsLimit→interval)                      Minecraft.cpp:686 (GLCore.cpp:297)
├─ pumpAndPresent()                                      Minecraft.cpp:688 (DisplayManager.cpp:158 → glfwPollEvents + present → SwapBuffers [vsync block])
├─ if player inside wall → thirdPerson=false             Minecraft.cpp:690-692
├─ if !skipGameRender:
│  ├─ luaHookRenderFrame()                               Minecraft.cpp:695
│  ├─ interactionManager->update(partialTick)            Minecraft.cpp:697 (SingleplayerInteractionManager.cpp:103)
│  └─ gameRenderer->onFrameUpdate(partialTick)           Minecraft.cpp:700 (GameRenderer.cpp:613)  [THE RENDER]
│     ├─ shaderPacks_->pollPrograms()                    GameRenderer.cpp:621 (link worker shader binaries)
│     ├─ pollMouseLook / cinematic smoothing             GameRenderer.cpp:638-656
│     ├─ if world != nullptr:
│     │  └─ renderFrame(partialTick)                     GameRenderer.cpp:661 (GameRenderer.cpp:888)
│     │     ├─ beginSceneCapture()                       GameRenderer.cpp:892 (GameRenderer.cpp:702: shaderPacks_->poll, ensureSceneTargets, clearScene)
│     │     └─ renderToCurrentTarget(…)                  GameRenderer.cpp:895 (GameRenderer.cpp:948)
│     │        ├─ updateTargetedEntity (non-shadow)      GameRenderer.cpp:967 (2nd raycast this frame)
│     │        ├─ setChunkCacheCenterFromBlockPos        GameRenderer.cpp:998
│     │        ├─ makeAtmosphereContext / fog            GameRenderer.cpp:1004-1016
│     │        ├─ core::clear color+depth                GameRenderer.cpp:1019
│     │        ├─ renderWorld()                          GameRenderer.cpp:1020 (GameRenderer.cpp:490)
│     │        ├─ if pack: prepareFrame                  GameRenderer.cpp:1075 (Pipeline::prepareFrame)
│     │        ├─ setFrameUniforms + renderBegin         GameRenderer.cpp:1082-1094
│     │        ├─ shadowmap::update (shadow pass)        GameRenderer.cpp:1101 (ShadowMapPass.cpp update)
│     │        ├─ Frustum::compute                       GameRenderer.cpp:1107
│     │        ├─ renderSkyDome + lightmap refresh       GameRenderer.cpp:1115-1126
│     │        ├─ renderShadowComposite                  GameRenderer.cpp:1134
│     │        ├─ bindScene + renderPreWorld             GameRenderer.cpp:1144-1151
│     │        ├─ worldRenderer->cullChunks()            GameRenderer.cpp:1164 (WorldRenderer.cpp:949)
│     │        ├─ worldRenderer->compileChunks()         GameRenderer.cpp:1168 (WorldRenderer.cpp:727: budgeted mesh uploads+captures)
│     │        ├─ OpaqueTerrain stage: drawSolidTerrain  GameRenderer.cpp:1206-1209 (WorldRenderer::render :586)
│     │        ├─ Entities stage: renderEntities         GameRenderer.cpp:1217-1222 (WorldRenderer.cpp:826)
│     │        ├─ particles (before/mixed)               GameRenderer.cpp:1237
│     │        ├─ captureOpaqueDepth                     GameRenderer.cpp:1242
│     │        ├─ renderFirstPersonHand                  GameRenderer.cpp:1247
│     │        ├─ captureHandDepth                       GameRenderer.cpp:1253
│     │        ├─ renderDeferred (if pack)               GameRenderer.cpp:1260
│     │        ├─ translucent entities split pass        GameRenderer.cpp:1267-1273
│     │        ├─ particles (after)                      GameRenderer.cpp:1274-1275
│     │        ├─ block overlay (water)                  GameRenderer.cpp:1279
│     │        ├─ TranslucentTerrain stage               GameRenderer.cpp:1284-1287 (renderLastChunks if fancy :846)
│     │        ├─ sampleCenterDepth                      GameRenderer.cpp:1291
│     │        ├─ renderPrecipitation                    GameRenderer.cpp:1301
│     │        ├─ Clouds stage: renderClouds             GameRenderer.cpp:1309-1318
│     │        └─ hand (non-capture path)                GameRenderer.cpp:1323-1327
│     │     └─ if captured: resolveSceneCapture()        GameRenderer.cpp:897 (GameRenderer.cpp:734: endScene → renderPostProcess → pipeline reset)
│     │        └─ shaderPacks_->renderPostProcess(…)     GameRenderer.cpp:752
│     ├─ inGameHud.render (HUD)                          GameRenderer.cpp:672
│     └─ currentScreen()->render (GUI)                   GameRenderer.cpp:699
├─ if inactive: sleep 10ms / fullscreen toggle           Minecraft.cpp:705-710
├─ ClientProfilerOverlay renderProfilerChart|record      Minecraft.cpp:712-716
├─ toast.tick()                                          Minecraft.cpp:717 (AchievementToast.cpp:120)
├─ handleScreenshotKey() → Screenshot::take (sync)       Minecraft.cpp:718 (Screenshot.cpp:48)
├─ ++frames                                              Minecraft.cpp:719
├─ paused = … shouldPause()                              Minecraft.cpp:720
├─ FPS window text (ChunkBuilder::chunkUpdates)          Minecraft.cpp:721-727
└─ stall>200ms → stall-trace.log write                   Minecraft.cpp:728-747
```

### 4.4 World-setup paths that run inside tick/render frames (loading screens)
- `startGame` → `WorldSession::setWorld` (`WorldSession.cpp:62`) → `prepareWorld` (`WorldSession.cpp:159`): synchronous chunk force-load loop + relight + `finishLightingUpdates` + `tickChunks`, with nested `ProgressRenderer` present every 20ms.
- `respawnPlayer` → `worldSession_.prepareWorld` (`Minecraft.cpp:1102`).
- `travelToDimension` → `setWorld` (`Minecraft.cpp:996`).
- `setWorld(nullptr)` → `unloadWorld` (`WorldSession.cpp:42`) → stats save + `world->savingProgress(nullptr)` (sync save) + `clearWorld`.

### 4.5 Server main loop (comparison) — `MinecraftServer.cpp:417-473`
`run()`: `init()` → spawn-pregen 196-radius loop with synchronous `cache->loadChunk` + `doLightingUpdates` per chunk (`:361-380`) → then: `while(running) { now-lastTick → catchup; tick() every 50ms; sleep 1ms }`. `tick()` (`:486-537`): world->tick + doLightingUpdates + tickEntities per dimension, connections->tick, playerManager.updateAllChunks, entity trackers, runPendingCommands. Server is a single tick thread with its own 50ms loop; it does NOT render.

## 5. Comparison with Java Beta 1.7.3 and modern Minecraft

### vs. Java Beta 1.7.3 (`net.minecraft.client.Minecraft.run()`, MCP b1.7.3)
- **Same architecture fundamentally**: single thread does tick+render; `timer.advance()` → N ticks → `runTick`/`runGameLoop`; one render per loop with `partialTick` interpolation; `paused` gates world sim; profiler and FPS counter at loop end. This port preserves the Java ordering invariants deliberately (`Minecraft.cpp:647-649`).
- **Differences (improvements over Java b1.7.3)**:
  - Java had **zero** async chunk gen/lighting/meshing. This port already has: async chunk loading (`ChunkCache` pools), async lighting (`LightingEngine` threads), async meshing (`ChunkMeshScheduler`), async saves, async shader compile, async textures/skins, async auth. So the port is "Java b1.7.3 loop + worker-fed chunks".
  - Java's `Timer` slept to pace 20 TPS-ish; this port has no loop sleep (vsync-paced) and supports fractional `ticksThisFrame` up to 10.
  - Java's `Minecraft.runGameLoop` rendered the loading screen via `drawSplashScreen`/ProgressRenderer the same nested way; this port replicates that (`ProgressRenderer`).
  - New in this port vs Java: deferred bridge/screen retirement (`ScreenStack`, `MultiplayerSession`), stall tracing, Lua hooks, hang watchdog, MS auth, server-process coordinator, `ClientNetworkHandler` packet-time-boxed drain (Java drained all pending packets each tick — `Connection.cpp:198-221` is a deliberate improvement).
- **What is still identical to Java (and thus still main-thread):** all of world tick (entities/block entities/random ticks/NaturalSpawner/displayTick), all rendering GL, all GUI, all of `tick()`'s per-subsystem calls, world load (`prepareWorld`), respawn, dimension travel, and the fact that a long tick OR long render OR vsync wait all serialize on one thread.

### vs. modern Minecraft (render thread + server thread + IO threads)
- Modern MC splits: **client tick thread** (world sim, ~20 TPS), **render thread** (frames interpolate `partialTick`; never waits on tick), **server thread** (integrated server owns chunk gen/save/entities), **IO threads** (region read/write, async chunk loads, async saves, async chat signing), **chunk builder threads** (meshing), **render executor** (upload), plus `RegionFile` async. The client never runs world gen or disk I/O on the render thread; `prepareWorld` is done by the server ticking chunks that stream in.
- This port has: ONE thread doing tick+render+GUI+packet-apply+chunk-decorate+all-GL, with worker pools feeding it. The main thread still:
  - Runs the full world tick (modern MC: server thread + client tick thread).
  - Applies packets and decorates chunks on integrate (modern MC: client tick thread / server thread).
  - Does synchronous texture decode + upload (modern MC: background resource reload + managed texture upload queue on render thread).
  - Does synchronous stat/account/zip file I/O (modern MC: IO threads).
  - Renders nested loading screens inside world load (modern MC: separate loading screen render thread / different flow).
- **Modern MC's key structural wins this port lacks:** a separate tick cadence from render cadence (frame time never stolen by a tick catch-up burst), server tick outside the client loop, and no synchronous file/network work on the frame path. The async worker pools here already fix the worst gen stalls, but the main thread still owns decorate, packet apply, and all tick CPU, so a tick spike directly = a frame spike.

## 6. Main-thread interactions with the named subsystems (summary)
- **`ClientNetworkBridge`/`MultiplayerSession::flushRetired`** — teardown deferred off the tick stack; runs at loop top `Minecraft.cpp:769`; destroys `Connection` (socket-thread joins) on the main thread.
- **`WorldSession::setWorld`** — synchronous world save of the old world + `prepareWorld` (heavy gen + nested present) of the new one; runs on whatever stack called `setWorld` (tick, packet handler, UI). `applyDeferredRespawn` moved off `ClientWorld::tick` into `runWorldSimulation` (`Minecraft.cpp:559-565`) to keep heavy prepare off the tick stack.
- **`ScreenStack`** — setScreen parks screens; `flushRetired` at loop top destroys them; `tickScreens` runs `Screen::tickInput/tick` between input settle and poll; setScreen also does a sync `stats->save()`.
- **`ProgressRenderer`** — loading screens drawn + presented inline inside world-gen/load code paths (nested present); rate-limited to 20ms.
- **`cleanHeap`** — frees crash-reserve memory + `worldRenderer->releaseSections()` + `setWorld(nullptr)` on `bad_alloc`.
- **`paused`** — computed at the END of the render phase (`Minecraft.cpp:720`); read at the START of next tick (`Minecraft.cpp:573,582,586,589,637,644`). Pausing freezes the world sim + textureManager.tick + interactionManager.tick, and freezes `partialTick` (`Minecraft.cpp:788-791`) so the pause menu renders without a moving world.
- **`timer`** — advanced once per loop; `ticksThisFrame` (capped 10) drives the tick loop; `partialTick` drives render interpolation; `tps/tpsScale` are Lua-modifiable each loop (`Minecraft.cpp:784-787`).

## 7. Implications for restructuring (observations only, no plan)
1. The cleanest seam for "tick off the render thread" already exists: `runWorldSimulation` (`Minecraft.cpp:555`) is a self-contained block guarded by `paused`, and `runRenderPhase` only reads `timer.partialTick` + `world` state. But cross-thread `World` access is NOT currently thread-safe (no locking; `WorldEvents` dispatch is synchronous on the calling thread), so a real split needs ownership/phase gating, not just moving the calls.
2. The single biggest main-thread CPU is `tickEntities` + `manageChunkUpdatesAndEvents` + `NaturalSpawner` + `displayTick` (all of §3.1-3.4). Any "main thread must never do world sim" goal has to move ~4-8ms/tick of serial AI/RNG work.
3. The biggest *stall* risks are the synchronous paths: `downloadPendingMods` (unbounded HTTP), `prepareWorld` force-load (97 chunks + relight), `ChunkCache::save(true)`+`waitForPendingWrites` (world unload), and the first-time `TextureManager::getTextureId` decode/upload.
4. Packet apply and chunk decorate run on the main thread by design (they touch live world state + fire renderer events synchronously); making them async requires event/queue indirection, not just thread moves.
5. Present is already at the top of `runRenderPhase`; if render and tick ever split, the vsync point and the "render this frame" work must stay together or double-buffered to avoid idle vsync waits.
