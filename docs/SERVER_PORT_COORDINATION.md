# Server port coordination (feature/server-port)

Branch: `feature/server-port` from `main`.

## Rules
- Java reference: `mcp/src/net/minecraft/server/`
- Native target: `native/src/net/minecraft/server/` (mirror package layout)
- **No stubs** — faithful 1:1 ports per `port-never-stub.mdc`
- Token-dense clang-format; run `native/format-omega.ps1` after edits
- Win32 GUI: `native/src/net/minecraft/server/platform/win32/` + `dedicated/gui/`
- RAII: use `Win32Raii.hpp`, `ServerSocket` patterns
- **Do not build** — parent runs `build-omega.ps1` after all agents finish

## Alphabetical assignment
| Agent | Scope | Status |
|-------|-------|--------|
| S1 | ChunkMap | done |
| S2 | command/* | done |
| S3 | ConsoleFormatter, ServerLog | done |
| S4 | dedicated/gui/* + win32 platform | done |
| S5 | entity/* | done |
| S6 | MinecraftServer + server-main bootstrap | done |
| S7 | network ConnectionListener, ServerSocket, ServerLoginNetworkHandler | done |
| S8 | network ServerPlayNetworkHandler, ServerPlayerInteractionManager | done |
| S9 | PlayerManager, ServerProperties | done |
| S10 | world/* + tests + CMake | done |

## Cross-agent interfaces (update when done)
- `MinecraftServer` exposes: `playerManager`, `entityTrackers`, `worlds`, `connections`, `properties`, `tick()`
- `ConnectionListener::tick()` called from server main loop
- `PlayerManager` uses `ChunkMap` per dimension
- GUI: `DedicatedServerGui::create(server)` when not headless

## Agent notes
(agents append brief completion notes below)

### S1 — ChunkMap (done)
- `ChunkMap.hpp` + `ChunkMap.cpp`: faithful port of per-dimension chunk player tracking, dirty-block batching (`BlockUpdateS2CPacket` / `ChunkDataS2CPacket` / `ChunkDeltaUpdateS2CPacket`), `ChunkStatusUpdateS2CPacket` load/unload.
- Uses `util::LongObjectHashMap<std::shared_ptr<TrackedChunk>>`; `TrackedChunk` nested in `.cpp`.
- **S9** (`PlayerManager`): wire `chunkMaps[2]`, call `updateChunks`/`markBlockForUpdate`/`addPlayer`/`removePlayer`/`updatePlayerChunks`.
- **Blockers for others:** `BlockEntity::createUpdatePacket()` not on base class — `SignBlockEntity` handled via `dynamic_cast`; other block entities with update packets need virtual when ported.

### S2 — command/*
`Command`, `CommandOutput`, `ServerCommandHandler` ported 1:1 from Java. All 20 console commands + `whitelist` subcommands (`on`/`off`/`list`/`add`/`remove`/`reload`). `split(" ")` semantics matched via `splitOnSingleSpace`. Depends on S6 (`MinecraftServer::stop`, `properties`) and S9 (`PlayerManager` command API: `getPlayerList`, `savePlayers`, ops/ban/whitelist/chat helpers).

### S5 — entity/*
`EntityTracker` + `EntityTrackerEntry` ported 1:1. Added `SpawnableEntity` marker (`Monster`, `AnimalEntity`, `WaterCreatureEntity`). Depends on s2c play packets: `EntityDestroy`, `EntityEquipmentUpdate`, `EntityMoveRelative`, `EntityPosition`, `EntityRotate`, `EntityRotateAndMoveRelative`, `EntitySpawn`, `EntityTrackerUpdate`, `EntityVelocityUpdate`, `ItemEntitySpawn`, `LivingEntitySpawn`, `PaintingEntitySpawn`, `PlayerSleepUpdate`, `PlayerSpawn`. Sends via `ServerPlayerEntity::networkHandler->sendPacket`.

### S4 — dedicated/gui + win32 platform
Win32 faithful port of `DedicatedServerGui`, `LogHandler`, `PlayerListGui`, `PlayerStatsGui`. Layout: 854×480 frame, west Stats group (custom memory graph + LISTBOX players), center Log and chat (readonly multiline EDIT + command EDIT). `LogHandler` extends `server::LogHandler`, registered on `ServerLog::LOGGER` (Java `LOGGER.addHandler`). Thread-safe log/player refresh via `PostMessage`/`WM_APP+*`. Platform: `WindowClass`, `MessageLoop`, `Win32Raii` (GDI in stats paint). Depends on S6 for `MinecraftServer::queueCommands`, `stop`, `stopped`, `addTickable`.

### S3 — ConsoleFormatter, ServerLog
- `ConsoleFormatter`: `formatThrown` mirrors Java `Throwable.printStackTrace` header (`TypeName: message`); demangle on GCC.
- `FileLogHandler`: open in ctor (throws `std::runtime_error` on failure, matching Java `FileHandler` + `init` catch); persistent `ofstream` + flush per publish.
- `ServerLog::init`: single shared `ConsoleFormatter` on console + file handlers via `shared_ptr` (Java one-instance reuse).

### S8 — ServerPlayNetworkHandler, ServerPlayerInteractionManager
- `ServerPlayNetworkHandler`: 16/16 packet handlers ported (movement anti-cheat, keep-alive send, chat `/me` `/kill` `/tell` + op `queueCommands`, block action/interact → `interactionManager`, entity interact, inventory click/ack, sign edit, respawn, disconnect). Join flow wired from `ServerLoginNetworkHandler::accept` (login hello, spawn pos, world info, join chat, teleport, time).
- `ServerPlayerInteractionManager`: break progress, `onBlockBreakingAction`/`continueMining`/`tryBreakBlock`/`interactItem`/`interactBlock`; sends `BlockUpdateS2CPacket` on successful harvest break.
- **Blockers:** `World::worldEvent` not on native `World` — `tryBreakBlock` uses `spawnBlockBreakParticles` + `block::sounds::playBreak` instead of Java `worldEvent(2001, …)`. `handle()` logs generic warning (Java logs packet class name).

### S6 — MinecraftServer + server-main bootstrap
- `MinecraftServer`: faithful `init()`/`run()` loop (50 ms tick cadence, catch-up cap), `ConnectionListener` via `std::unique_ptr<network::ConnectionListener> connections`, command stdin thread, `pendingCommands` + `queueCommands`/`runPendingCommands`, `tickables`, `stop`/`stopped`, properties (`online-mode`, `spawn-animals`, `pvp`, `allow-flight`, `allow-nether`), `ReadOnlyServerWorld` for nether, spawn preload + `progressMessage`/`progress`, `WorldConversionProgress`, `capturedThread` + `Box`/`Vec3d::resetCacheCount`, `BackgroundDaemon`.
- `server-main.cpp`: `Stats::initialize()` → `MinecraftServer` → Win32 `DedicatedServerGui::create` unless `nogui` argv → `run()` on dedicated thread (join). `ServerLog::init()` remains in `init()` like Java.
- **S4** GUI: `queueCommands`, `stop`, `stopped`, `addTickable` wired. **S2**: `stop()`, `properties`. **S7**: `connections` member ready for `ConnectionListener::tick()` from main loop.

### S7 — network accept + login (done)
- `ServerSocket.hpp/cpp`: Win32 `ws2_32` RAII (`SOCKET` move + dtor `closesocket`), `ensureWinsock`, `bindAndListen`/`accept`/`boundPort`, `configureAcceptedSocket` (TCP_NODELAY, timeouts, IP_TOS).
- `ConnectionListener.hpp/cpp`: dedicated listen `std::thread`, per-host 5s rate limit (loopback exempt), `pendingConnections` + `playConnections` tick on server thread; snapshot-tick merges back without clobbering accepts from listen thread; promotes to `ServerPlayNetworkHandler` via `addConnection`.
- `ServerLoginNetworkHandler.hpp/cpp`: handshake (online `serverId` hex / offline `"-"`), protocol 14 gate, offline immediate accept, online `checkserver.jsp` verify thread → deferred accept; `accept()` → `PlayerManager::connectPlayer`, world info, join chat, teleport, `addConnection`, `WorldTimeUpdate`, `initScreenHandler`.
- Wired: `MinecraftServer::connections` (`unique_ptr<ConnectionListener>`), constructed in `init()` from `server-ip`/`server-port`/`online-mode`; `connections->tick()` in `MinecraftServer::tick()`.
- Tests (for S10 CMake): `tests/connection_listener_test.cpp`, `tests/server_login_handler_test.cpp`.
- **S8**: `ServerPlayNetworkHandler::onDisconnected` still missing — play removal also checks `connection->isOpen()` until S8 lands.

### S9 — PlayerManager, ServerProperties (done)
- `PlayerManager`: faithful 1:1 port; `chunkMaps_[2]` wired in `configureFromProperties()` (dim 0 / -1, `view-distance` from `ServerProperties`), called from `MinecraftServer::init` after properties load. All ChunkMap hooks: `addPlayer`, `removePlayer`, `updatePlayerChunks`, `updateChunks` (via `updateAllChunks`), `markBlockForUpdate` (via `markDirty`), dimension switch via `getChunkMap` + `updatePlayerAfterDimensionChange`.
- Command API complete for S2: `getPlayerList`, `savePlayers`, `banPlayer`/`unbanPlayer`, `banIp`/`unbanIp`, `addToOperators`/`removeFromOperators`, `addToWhitelist`/`removeFromWhitelist`/`getWhitelist`/`reloadWhitelist`, `isWhitelisted`/`isOperator`, `getPlayer`, `messagePlayer`, `kick` (via `players` + disconnect), `sendToAll`, `sendPacket`, `broadcast`, `sendToAround`, `sendToDimension`.
- `ServerProperties`: load/save/`getProperty` (string/int/bool)/`setProperty(bool)`/`generateNew` faithful to Java; missing keys auto-persist with fallback.
- Fixed `sendToAround` template overload (packet param); `ServerCommandHandler` `properties->setProperty` for whitelist on/off.
