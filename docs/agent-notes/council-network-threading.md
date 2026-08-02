# Council: Network / Multiplayer Threading Review (review-only, no edits)

Scope: all NETWORK/MULTIPLAYER threading in the C++20 Beta 1.7.3 port, with the goal of
restructuring multithreading under a coordinated scheduler while keeping functional parity
with Java b1.7.3 multiplayer.

All paths under `src/net/minecraft/`. Line numbers are from the files as of 2026-07-31.
Java reference: `third_party/mcp/net/minecraft/src/NetworkManager.java` (b1.7.3, the
`NetworkManager` decompile used by the port).

---

## 1. Executive summary

- The port already reproduces the **b1.7.3 Java threading model** structurally: one
  `Connection` = one reader thread + one writer thread, with packet **processing** done on
  the game thread by draining a read queue inside a `tick()` call. See §5 (parity).
- Packet *apply* is always on the game thread today (client: main/GL loop; server:
  the single `runThread_` tick loop). No packet handler runs on a socket thread. That part
  is already safe and matches Java.
- The structural weaknesses for a scheduler-based restructure:
  1. **Every `Connection` owns a raw `std::thread` pair** (`Connection.cpp:131-132`), and
     `disconnect()` **synchronously joins both threads on the caller's stack**
     (`Connection.cpp:157-160`, `joinThreads` `:352-360`). With `SO_SNDTIMEO`/`SO_RCVTIMEO`
     = 30s (`Connection.cpp:17-20`) a stalled peer can block the game thread for ~30s.
     Java's `networkShutdown()` spawns an async `NetworkMasterThread` and never blocks
     (`NetworkManager.java:160-189`). This is the single biggest parity + responsiveness
     deviation.
  2. **Two dedicated kernel threads per connection, parked in blocking `recv`/`send`.**
     For N players that is 2N threads (plus the accept thread, login/verify/join HTTP
     threads, audio, chunk loader/save pools, logging, shader watcher…). A coordinated
     scheduler should fold read+write + accept into a small pool of I/O threads (or an
     event loop) and keep the game-thread drain semantics intact.
  3. **A real data race in the static packet accounting**: `Packet::read` mutates
     `packetTrackers()` and `++incomingCount()` (`Packet.hpp:79-81`) from every reader
     thread with no lock. N connections => N concurrent writers on one `unordered_map`.
  4. **`ServerLoginNetworkHandler::verifyThread_` mutates connection/handler state from a
     non-owner thread** (failure path calls `disconnect()` → `sendPacket` + `connection_->disconnect()`
     + sets `closed`; `ServerLoginNetworkHandler.cpp:172,175,64`) while the server tick
     thread also drains the same `Connection`. `closed` is a plain bool — data race.
  5. **Main-thread blocking on join/HTTP threads**: client `beginPendingLogin` joins a prior
     `joinServerThread_` in flight (`ClientNetworkHandler.cpp:243-245`); server
     `verifyUsernameOnline` joins a prior `verifyThread_` on the server tick
     (`ServerLoginNetworkHandler.cpp:150-152`). Both can stall the game loop on slow auth.
  6. **`ClientNetworkHandler::message` is written on the connector thread**, but both writes
     (`ClientNetworkBridge.cpp:137`, `MultiplayerConnector.cpp:57`) are sequenced before the
     bridge is published under `MultiplayerConnector::mutex_` and read under the same mutex
     (`ConnectScreen.cpp:63` via `activeBridge`) — so it is currently safe, but it is a
     fragile "publication by mutex handoff" pattern to preserve under a new scheduler.
  7. **Double drain during terrain download**: `DownloadingTerrainScreen::tick()` calls
     `networkHandler_->tick()` (`DownloadingTerrainScreen.hpp:24-26`) AND the active
     `ClientWorld::tick()` calls `networkHandler_->tick()` (`ClientWorld.cpp:53-55`), both on
     the main thread each tick. Effectively up to 2× the 3 ms drain budget per frame.
  8. **No backpressure / unbounded inbound queue**: `readQueue_` grows without bound until
     the 3 ms / 4096-packet drain (`Connection.cpp:195-221`); the overflow check is on the
     **send** byte count only (`sendQueueSize_ > 0x100000`, `Connection.cpp:185-187`).
  9. **Chunk `compressForSend()` runs on the caller (game) thread** inside
     `sendPacket` (`Connection.cpp:170-172`) — CPU cost on the game thread per chunk send.

---

## 2. Thread inventory (all threads in the system that touch network state)

| Thread | Owner | Created at | Role | State it touches |
|---|---|---|---|---|
| Client game/GL thread (main) | `Minecraft::run` | `Minecraft.cpp:749` | drain + apply packets, render, screens | `Connection::tick()`, `ClientNetworkHandler`, `ClientWorld`, `MultiplayerSession` |
| Per-conn reader | `Connection` | `Connection.cpp:131` | `readLoop`: decode + enqueue | `readQueue_` (mutex `readMutex_`), `Packet::read` statics (**race**), `open_`, socket |
| Per-conn writer | `Connection` | `Connection.cpp:132` | `writeLoop`: drain + write socket | `sendQueue_`/`delayedSendQueue_` (mutex `writeMutex_`), `sendQueueSize_`, socket |
| Client connect bootstrap | `MultiplayerConnector` | `MultiplayerConnector.cpp:17` | DNS+TCP connect, auth restore, send `HandshakePacket`, publish bridge | `pendingBridge_` (mutex), `handler->message`, `cancelled_` |
| Client join/auth | `ClientNetworkHandler` | `ClientNetworkHandler.cpp:247` | HTTP session verify | `joinServerResult_`/`joinServerState_` (mutex `joinServerMutex_`) |
| Server accept/listen | `ConnectionListener` | `ConnectionListener.cpp:28` | `listenLoop`: accept + rate-limit + create login handlers | `pendingConnections_` (mutex), `recentConnectionsByHost_` (mutex) |
| Server game thread | `MinecraftServer::runThread_` (or `server-main.cpp:125` `serverThread`) | `MinecraftServer.cpp:174` | tick loop: world + `ConnectionListener::tick()` | all server world state, all connections |
| Server login verify | `ServerLoginNetworkHandler` | `ServerLoginNetworkHandler.cpp:154` | HTTP `checkserver.jsp` | `deferredLoginPacket_` (mutex `verifyMutex_`), **unsafe**: `disconnect()`, `closed` |
| Server console | `MinecraftServer::commandThread_` | `MinecraftServer.cpp:197` | stdin → `queueCommands` | `pendingCommands_` (mutex) |
| Chunk loaders | `ChunkCache::loaderPool_` (WorkerPool, 3 workers) | `ChunkCache.cpp:212-219` | async chunk load/gen on per-thread clones | only per-thread `workerGenerators_` + storage under `ioMutex_` |
| Chunk saver | `ChunkCache::savePool_` (WorkerPool, 1) | `ChunkCache.cpp:220-225` | serialized chunk writes | snapshot data (copies); `pendingSaveWrites_`, cv |
| World level.dat saver | `World::asyncSaveFuture_` | `World.cpp:331-338` | async `level.dat` write | `dimensionData_` via snapshot |
| Audio / logging / shader-watcher / image-download | misc | various | not network | — |

---

## 3. Client packet flow (end to end)

### 3.1 Bootstrap (async)
1. `ConnectScreen` owns a `MultiplayerConnector` (`ConnectScreen.cpp:13-14`).
2. `MultiplayerConnector` ctor spawns `thread_` (`MultiplayerConnector.cpp:17`) which:
   - `ClientNetworkBridge::connect(...)` (`ClientNetworkBridge.cpp:122-148`) → `openClientSocket`
     (DNS + non-blocking connect with 10s deadline, `ClientNetworkBridge.cpp:76-117`) →
     creates `Connection(socket,"Client",*handler_)` → spawns reader/writer
     (`Connection.cpp:131-132`), then `handler_->bindConnection` (`ClientNetworkBridge.cpp:146`).
   - restores Microsoft account (`MultiplayerConnector.cpp:38`), sends `HandshakePacket`
     (`MultiplayerConnector.cpp:52-55`), sets `handler->message` (`:57`), publishes
     `pendingBridge_` under mutex (`:59-60`).
3. Main thread: `ConnectScreen::tick()` → `connector_.poll()` (`ConnectScreen.cpp:42`,
   `MultiplayerConnector.cpp:83-94`) adopts the bridge into `MultiplayerSession`
   (`adoptBridge`), then `multiplayerSession_.tick()` → `bridge_->tick()` →
   `handler_->tick()` (`Minecraft.cpp:655-657`, `MultiplayerSession.cpp:20-25`,
   `ClientNetworkBridge.cpp:159-163`).
4. `ClientNetworkHandler::tick()` (`ClientNetworkHandler.cpp:84-92`): clear `retiredWorlds_`,
   `processPendingJoinServer()`, then `connection_->tick()` + `connection_->interrupt()`.

### 3.2 Read side
1. Reader thread `readLoop` (`Connection.cpp:262-281`): `Packet::read(input_, isServerSide())`
   (`Connection.cpp:266-267`) decodes one packet; pushes `unique_ptr<Packet>` onto
   `readQueue_` under `readMutex_` (`:272-275`). EOF / exception → `requestDisconnect`
   (`:269,:278`). Note `Packet::read` writes the global packet-tracker statics (`Packet.hpp:79-81`).
2. Main thread `Connection::tick()` (`Connection.cpp:184-228`):
   - overflow: `sendQueueSize_ > 0x100000` → `requestDisconnect("disconnect.overflow")` (`:185-187`)
   - timeout: if `readQueueEmpty()` for `>= 1200` ticks → `requestDisconnect("disconnect.timeout")` (`:188-194`)
   - time-boxed drain: min 8 packets, max 4096, wall budget 3 ms (`:198-201`); each packet popped
     under `readMutex_` and applied on the game thread: `packet->apply(*handler)` (`:213-217`).
   - if closed and queue drained → `handler->onDisconnected(reason, args)` once (`:222-227`).
3. `apply` dispatches to `ClientNetworkHandler::on*` (`NetworkHandler.hpp:75-188`), which
   directly mutates `ClientWorld`/entities/screens (e.g. `handleChunkData` →
   `world->handleChunkDataUpdate`, `WorldPacketHandlers.cpp:62-75`; `onPlayerMove` →
   `player->setPositionAndAngles` + echo move, `PlayerPacketHandlers.cpp:49-93`;
   `onOpenScreen` → `minecraft->setScreen`, `ScreenPacketHandlers.cpp`). **All on main thread.**

### 3.3 Drain sites (who calls `connection_->tick()`)
- No remote world (connect screen / LAN / title): `Minecraft::tick()` → `multiplayerSession_.tick()`
  guarded by `world == nullptr || !world->isRemote()` (`Minecraft.cpp:655-657`).
- Remote world active: `Minecraft::runWorldSimulation()` → `world->tick()` → `ClientWorld::tick()`
  → `networkHandler_->tick()` (`Minecraft.cpp:584`, `ClientWorld.cpp:39-55`).
- **During terrain download, BOTH happen**: `DownloadingTerrainScreen::tick()` also calls
  `networkHandler_->tick()` (`DownloadingTerrainScreen.hpp:24-26`) while the remote
  `ClientWorld` is ticking (`Minecraft.cpp:584`). The read queue is drained twice per tick
  (each drain is idempotent — packets are consumed once — but the budget is effectively doubled
  and `KeepAlive` is emitted from the screen every 20 ticks, `DownloadingTerrainScreen.hpp:21-23`).

### 3.4 Send side
1. Game thread `ClientNetworkHandler::sendPacket` → `connection_->sendPacket(packet)`
   (`ClientNetworkHandler.hpp:50-62`, `Connection.cpp:166-183`): chunk packets are compressed
   **synchronously on the caller** (`compressForSend`, `:170-172`), then pushed onto
   `sendQueue_` (immediate) or `delayedSendQueue_` (`packet->worldPacket`, `:176-180`) under
   `writeMutex_`, `sendQueueSize_ += size+1`, `writeCv_.notify_one()`.
2. Writer thread `writeLoop` (`Connection.cpp:282-333`): `wait_for(20ms)` on `writeCv_`
   (`:290-292`), drains both queues to a local batch alternating `preferImmediate`
   (`:293-316`), `Packet::write` + flush (`:319-325`), decrements `sendQueueSize_` (`:323`).
   On error or after close drains → `shutdownSocket()` (`:331`).

### 3.5 Teardown
- `ClientNetworkHandler::disconnect` / `onDisconnected` / `onDisconnect` set `disconnected`,
  `setWorld(nullptr)`, retire world, show `DisconnectedScreen` (`ClientNetworkHandler.cpp:115-138,205-215`).
- `Minecraft::run()` each iteration: `multiplayerSession_.flushRetired()` then, if
  `handler == nullptr || handler->disconnected`, `retireBridge()` (`Minecraft.cpp:765-775`).
  Retired bridges are destroyed after the tick stack unwinds (`MultiplayerSession.cpp:12-19`).
- `ClientNetworkBridge::disconnect` → `connection_->disconnect()` (**joins reader+writer**,
  `Connection.cpp:157-160`) then `handler_.reset()`, `connection_.reset()` (`ClientNetworkBridge.cpp:149-158`).
- `MultiplayerConnector::~MultiplayerConnector` joins `thread_` (`MultiplayerConnector.cpp:63-68`);
  `ConnectScreen::cancelConnection` → `connector_.disconnectActive` → `bridge->disconnect()`
  **on the main thread** (`ConnectScreen.cpp:26`, `MultiplayerConnector.cpp:72-82`).

---

## 4. Server packet flow (end to end)

### 4.1 Accept / login
1. `ConnectionListener` ctor binds socket and spawns `thread_` → `listenLoop`
   (`ConnectionListener.cpp:21-29,48-80`): `accept`, per-host rate-limit (5 s window,
   `:59-70`), builds `ServerLoginNetworkHandler` (which constructs its own `Connection`
   with reader/writer, `ServerLoginNetworkHandler.cpp:23-28`), pushes to
   `pendingConnections_` under mutex (`:81-87`).
2. `MinecraftServer::tick()` → `connections->tick()` (`MinecraftServer.cpp:521-523`) →
   `ConnectionListener::tick()` (`ConnectionListener.cpp:99-154`): snapshots
   pending+play lists under mutex, calls `handler->tick()` per login/play handler
   (which drains `connection_->tick()`, `ServerLoginNetworkHandler.cpp:51-53` /
   `ServerPlayNetworkHandler.cpp:49-51`), then `connection->interrupt()` (`:121,:142`),
   then pushes survivors back under mutex (`:145-153`).
3. Login handshake → `onHello` → `verifyUsernameOnline` (online mode) spawns `verifyThread_`
   (`ServerLoginNetworkHandler.cpp:149-178`). Success: `deferredLoginPacket_` under
   `verifyMutex_`; main `tick()` picks it up and `accept()`s (`:37-45,:179-237`) —
   creates the `ServerPlayNetworkHandler`, sends login/spawn/time packets, then hands the
   **same** `Connection` to `listener_->addConnection` (`:228`). Failure: `disconnect()`
   **from the verify thread** (`:172,:175`) — see §6 hazard H4.
4. `ServerPlayNetworkHandler::tick()` (`ServerPlayNetworkHandler.cpp:46-55`): drains
   `connection_->tick()`, sends `KeepAlivePacket` every 20 ticks (`:52-54`).

### 4.2 Read side (server)
- Reader thread → `readQueue_` (same code as client, `Connection.cpp:262-281`).
- Server tick thread, per player, `ServerPlayNetworkHandler::tick` → `connection_->tick()`
  (`ServerPlayNetworkHandler.cpp:49-51`) → `packet->apply(*this)` on the server thread.
  Handlers mutate `ServerPlayerEntity` / `ServerWorld` directly, e.g. `onPlayerMove`
  (`ServerPlayNetworkHandler.cpp:139-286`: `playerTick`, `move`, collision checks,
  `playerManager.updatePlayerChunks`, teleport/`disconnect`), `handlePlayerAction`
  (`:287-353`), `onPlayerInteractBlock` (`:354-443`, incl. syncId slot reconciliation
  `:426-439` and `transactions_` bookkeeping `:635-657`), `onChatMessage`/`handleCommand`
  (`:477-563`, ops commands queued to the server `queueCommands`, `:559`).
- Keep-alive matching: `onKeepAlive` records `lastKeepAliveTime_ = ticks_`
  (`ServerPlayNetworkHandler.cpp:460-462`), matching Java's NetServerHandler keepalive.
- `ConnectionListener::tick()` (server tick thread) is the ONLY drain site per server tick,
  matching Java's per-player `processReadPackets()` inside `MinecraftServer.tick()`.

### 4.3 Send side (server)
- `ServerPlayNetworkHandler::sendPacket` → `connection_->sendPacket`
  (`ServerPlayNetworkHandler.hpp:43-53`) — same `Connection` path as client. Chunk packets
  (`worldPacket=true`) go to `delayedSendQueue_` (`Connection.cpp:176-180`).
- `getBlockDataSendQueueSize()` exposes `delayedSendQueue_` size (`ServerPlayNetworkHandler.cpp:65-70`)
  for the chunk-streaming throttle in `playerManager.updateAllChunks`
  (`MinecraftServer.cpp:524`, `PlayerManager`).

### 4.4 Teardown (server)
- `ServerPlayNetworkHandler::disconnect` (`ServerPlayNetworkHandler.cpp:71-91`): `player_->onDisconnect`,
  send `DisconnectPacket`, `connection_->disconnect()` (**joins threads on server tick thread**),
  broadcast leave, `playerManager.disconnect(player_)`, `disconnected = true`.
- `ConnectionListener::tick()` drops handlers where `disconnected || !connection->isOpen()`
  (`ConnectionListener.cpp:136-141`); `~ServerPlayNetworkHandler`/`~Connection` run when the
  `unique_ptr` is erased (`:139`) — the `Connection` dtor joins reader/writer on the server thread.
- `MinecraftServer::stop` → `connections->stopAccepting()` (`MinecraftServer.cpp:411-416`),
  `run()` end → `connections->close()` (`:464-466`).

---

## 5. Java b1.7.3 parity contract (reference: `third_party/mcp/net/minecraft/src/NetworkManager.java`)

Java b1.7.3 model:
- **2 threads per connection**: `NetworkReaderThread` + `NetworkWriterThread`
  (`NetworkManager.java:47-50`). One `NetworkManager` per socket.
- Read: reader thread `readPacket()` decodes via `Packet.readPacket(inputStream, isServerHandler)`
  and appends to `readPackets` (`NetworkManager.java:125-150`).
- Send: game thread `addToSendQueue()` — `sendQueueByteLength += size+1`; chunk-data packets go to
  `chunkDataPackets`, everything else to `dataPackets` (`:58-75`). Writer `sendPacket()` drains
  `dataPackets` first, and only sends a chunk packet after `field_20175_w` (init 50) counts down
  (`:77-117`) — a chunk-packet rate gate.
- Processing: `processReadPackets()` runs on the game thread and is called once per tick
  (client: `NetHandler`/`PacketManager`; server: per-player in the server tick):
  - send-overflow: `sendQueueByteLength > 0x100000` → `networkShutdown("disconnect.overflow")` (`:193-196`)
  - timeout: `readPackets.isEmpty()` → `timeSinceLastRead++ == 1200` → `"disconnect.timeout"` (`:197-206`)
  - drain: `for(int i = 100; !readPackets.isEmpty() && i-- >= 0; packet.processPacket(netHandler))`
    — **hard 100-packet cap per tick, no time budget** (`:207-211`)
  - `func_28138_a()` interrupts read+write threads every call (`:213`, impl `:119-123`)
  - when terminating and drained: `netHandler.handleErrorMessage(...)` (`:214-217`)
- Teardown: `networkShutdown()` is **async** — spawns `NetworkMasterThread`, closes streams/socket,
  sets `isRunning=false` (`:160-189`). `serverShutdown()` interrupts threads + spawns
  `ThreadMonitorConnection` (`:225-231`). **The game thread never joins the socket threads.**

### What the C++ port already matches
- 2 threads per connection, reader decodes to a queue, writer owns the socket write
  (`Connection.cpp:131-132,262-333`).
- Send-queue overflow constant and check on the game thread (`0x100000`, `Connection.cpp:185-187`).
- Timeout after 1200 empty ticks (`Connection.cpp:188-194`).
- Chunk packets segregated to a separate queue (`delayedSendQueue_`/`worldPacket`,
  `Connection.cpp:176-180`, `Packet.hpp:96`).
- Per-tick drain driven from the game loop (client `Minecraft.cpp:655-657`/`ClientWorld.cpp:53-55`;
  server `ConnectionListener.cpp:99-154`), matching the "one `processReadPackets` per tick" contract.
- `interrupt()` (→ `writeCv_.notify_all`) called after every drain (`ClientNetworkHandler.cpp:91`,
  `ConnectionListener.cpp:121,142`), mirroring `func_28138_a()`.
- Keep-alive cadence (every 20 ticks, `ServerPlayNetworkHandler.cpp:52-54`) and end-of-drain
  disconnect notification (`Connection.cpp:222-227`) mirror Java.

### Documented deviations (deliberate or accidental) — preserve or match under a new scheduler
1. **Drain policy**: C++ is time-boxed (3 ms / min 8 / max 4096) instead of Java's fixed 100-packet
   cap. Better for chunk floods, but a CPU-hungry client can inject far more work per tick than Java
   ever allowed. Any scheduler must keep an explicit per-tick work budget; consider also a per-source
   fairness cap so one connection cannot starve the others inside `ConnectionListener::tick()`.
2. **Blocking teardown**: Java is fully async (`NetworkMasterThread`); C++ `disconnect()` joins
   reader+writer on the caller (game) thread (`Connection.cpp:157-160`). See §7 hazard H1.
3. **Chunk-packet rate gate**: Java gates chunk sends with a countdown (`field_20175_w`, 50);
   C++ alternates `preferImmediate` (`Connection.cpp:296-312`). Loose approximation — note for parity
   auditing if chunk-streaming throttle behavior matters (the server already throttles via
   `getBlockDataSendQueueSize` in `PlayerManager::updateAllChunks`).
4. **`interrupt()` doesn't wake the reader**: Java interrupts the read thread (interrupting blocked
   IO); C++ `interrupt()` only notifies `writeCv_` (`Connection.cpp:154-156`). The C++ reader stays
   in `recv` until data/EOF/30s timeout. Fine today because `requestDisconnect` calls
   `shutdown(SD_RECEIVE)` (`Connection.cpp:341`) which unblocks `recv` with EOF — but under an
   event-loop scheduler, reads must be cancellable (close socket / cancel interest) to avoid the 30s
   join stall.

---

## 6. Cross-thread shared state (who owns what)

| State | Owner thread(s) | Guard | Ref |
|---|---|---|---|
| `readQueue_` | reader writes, game thread reads | `readMutex_` | `Connection.hpp:109`, `Connection.cpp:272-275,205-212` |
| `sendQueue_` / `delayedSendQueue_` | game thread writes, writer reads | `writeMutex_` | `Connection.hpp:110-111`, `Connection.cpp:174-181,289-316` |
| `sendQueueSize_` | all writers, game-thread overflow check | `std::atomic` | `Connection.hpp:112` |
| `open_` | any thread | `std::atomic` CAS | `Connection.hpp:101`, `Connection.cpp:336` |
| `networkHandler_` | any thread (read path) | `std::atomic<NetworkHandler*>` | `Connection.hpp:100` |
| `disconnectReason_` / args | disconnect caller; read by game thread after close | none (write-before-close, read-after-open=false via `open_`) | `Connection.hpp:117-118` |
| `Packet::packetTrackers()` / `incomingCount` | **every reader thread** | **NONE — DATA RACE** | `Packet.hpp:79-81,121-128` |
| `ClientNetworkHandler` state (`disconnected`, `world`, `retiredWorlds_`, `ownedWorld_`, screen ptrs) | main thread only (handlers + tick) | none needed | `ClientNetworkHandler.hpp:126-153` |
| `joinServerState_`/`joinServerResult_` | join thread writes, main reads | `joinServerMutex_` | `ClientNetworkHandler.cpp:62-68,253-255` |
| `joinServerCanceled_` | main sets, join thread polls | `std::atomic_bool` | `ClientNetworkHandler.hpp:165` |
| `MultiplayerConnector::pendingBridge_`/`connectError_` | connect thread writes, main reads | `mutex_` | `MultiplayerConnector.cpp:27-30,59-60,84-92` |
| `MultiplayerConnector::cancelled_` | main sets, connect thread polls | `std::atomic_bool` | `MultiplayerConnector.hpp:38` |
| `ClientNetworkHandler::message` | connect thread writes, main reads | sequenced by `mutex_` handoff (fragile) | `ClientNetworkBridge.cpp:137`, `MultiplayerConnector.cpp:57`, `ConnectScreen.cpp:63` |
| `ConnectionListener::pendingConnections_`/`playConnections_`/rate map | listen thread writes, server thread ticks | `mutex_` | `ConnectionListener.cpp:60-70,85-95,102-108,146-153` |
| `ServerLoginNetworkHandler::deferredLoginPacket_` | verify thread writes, server tick reads | `verifyMutex_` | `ServerLoginNetworkHandler.cpp:38-45,169-170` |
| `ServerLoginNetworkHandler::closed` | **verify thread writes, server tick reads — DATA RACE** | NONE | `ServerLoginNetworkHandler.cpp:64`, `ConnectionListener.cpp:116` |
| `ServerLoginNetworkHandler::connection_` | verify thread (failure `disconnect`) + server tick `tick()` — **concurrent access, DATA RACE** | none (mutex only guards deferred packet) | `ServerLoginNetworkHandler.cpp:51-53,172,175` |
| `ServerPlayNetworkHandler` state | server tick thread only | none | `ServerPlayNetworkHandler.cpp:46-55` |
| `MinecraftServer::pendingCommands_` | console thread writes, server tick reads | `pendingCommandsMutex_` | `MinecraftServer.cpp:136-158` |
| `capturedThread` (static) | server tick | `capturedThreadMutex` | `MinecraftServer.hpp:31-32`, `MinecraftServer.cpp:487-500` |
| `ChunkCache` chunk maps | game/server thread only | none (worker threads only touch their own snapshot) | `ChunkCache.cpp:226-288` |
| `pendingSaveWrites_` / cv | save workers + game thread | atomic + `saveCompleteCv_` | `ChunkCache.cpp:309-320,334-347,349-355` |
| `World::asyncSaveFuture_` | async writer + game thread | `std::future` (checked before spawn) | `World.cpp:312-314,325-338` |

---

## 7. Hazards

### H1 — Main/server thread blocks on disconnect (`joinThreads`)
- `Connection::disconnect()` → `requestDisconnect` + `joinThreads()` (`Connection.cpp:157-160`);
  `joinThreads` joins `reader_`/`writer_` (`:352-360`). Socket timeouts are 30 s
  (`Connection.cpp:17-20`), so a dead peer can stall the caller up to ~30 s (writer blocked in
  `send`; reader in `recv`).
- Call sites that block the game loop: `ClientNetworkBridge::disconnect` from `ConnectScreen`
  cancel / `cancelPendingModPrompt` / disconnect handling (`ClientNetworkBridge.cpp:149-158`);
  `ServerPlayNetworkHandler::disconnect` on the **server tick thread** (`ServerPlayNetworkHandler.cpp:79`);
  `Connection` dtor when a play handler is erased (`ConnectionListener.cpp:139`);
  `ServerLoginNetworkHandler::disconnect` from the verify thread (`ServerLoginNetworkHandler.cpp:62`).
- Parity gap vs Java's async `NetworkMasterThread`. **Fix direction**: hand the `Connection` to a
  scheduler-owned teardown (close socket, let I/O threads observe EOF, drop the object when all
  I/O references are gone) instead of joining on the game thread.

### H2 — Dedicated `std::thread` per connection, parked in blocking syscalls
- 2 threads × every connection (`Connection.cpp:131-132`), all in `recv`/`send`. With the writer's
  `wait_for(20ms)` (`Connection.cpp:290-292`) and reader's 30 s recv timeout, threads mostly sleep.
  Under a coordinated scheduler these should become I/O-pool tasks / epoll-style waits (see §9).

### H3 — Unbounded inbound queue, no read-side backpressure
- `readQueue_` has no cap; the drain is time-boxed but the queue can still grow without bound under
  a fast sender (`Connection.cpp:205-212`). The overflow check is only on the **send** byte count
  (`:185-187`). A malicious server can OOM the client by flooding chunk/entity packets. Consider a
  read-side byte cap + drop/disconnect policy (Java had the same gap, but a scheduler restructure is
  the right time to add the cap without breaking parity).

### H4 — `Packet::read` static accounting data race (server, N connections)
- `Packet::read` does `packetTrackers()[rawId].update(...)` and `++incomingCount()`
  (`Packet.hpp:79-81`) — unsynchronized statics touched by every connection's reader thread.
  On the dedicated server with several players this is a genuine race on `std::unordered_map`
  (insertion through `operator[]`). Today it's benign-looking (counters), but UB. Guard with a mutex
  or make it per-`Connection` counters that are merged on the game thread.

### H5 — `ServerLoginNetworkHandler::verifyThread_` cross-thread disconnect
- Failure path calls `self->disconnect(...)` from the verify thread (`ServerLoginNetworkHandler.cpp:172,175`),
  which runs `connection_->sendPacket` + `connection_->disconnect()` (joins threads) and sets
  `closed=true` (`:64`) — all while the server tick thread may be inside `connection_->tick()` /
  reading `closed` (`ConnectionListener.cpp:116`). Plain-bool race + concurrent `Connection` access.
  Also `verifyUsernameOnline` joins a prior in-flight `verifyThread_` on the server tick
  (`:150-152`) — server-wide stall on slow `checkserver.jsp`. **Fix**: verify thread should only set
  a result under `verifyMutex_`; all `Connection`/`closed` mutation should happen on the server tick
  thread (mirror the client's `processPendingJoinServer` pattern).

### H6 — Client `joinServerThread_` join on the main thread
- `beginPendingLogin` joins a prior `joinServerThread_` in flight before respawning
  (`ClientNetworkHandler.cpp:243-245`); `~ClientNetworkHandler` joins too (`:49-54`). HTTP session
  verify can take seconds → main-loop stall. Current design is otherwise sound (polled result under
  `joinServerMutex_` via `processPendingJoinServer`, `:55-83`). **Fix**: don't join to reuse; spawn a
  fresh thread (or submit to an HTTP pool) and let the poll pick up the result; join only on teardown.

### H7 — Double drain during terrain download
- `DownloadingTerrainScreen::tick()` → `networkHandler_->tick()` (`DownloadingTerrainScreen.hpp:24-26`)
  plus `ClientWorld::tick()` → `networkHandler_->tick()` (`ClientWorld.cpp:53-55`), both every main
  tick (world is remote so `paused==false`, `Minecraft.cpp:720`). Up to 2× the 3 ms drain budget per
  frame, and keep-alives emitted from the screen. **Fix**: make the drain idempotent-guarded
  (e.g. one drain per `Connection` per game tick) or remove the screen's call and rely on
  `ClientWorld::tick()`.

### H8 — Chunk compression on the game thread
- `Connection::sendPacket` runs `compressForSend()` synchronously (`Connection.cpp:170-172`).
  During a big chunk push (`playerManager.updateAllChunks`, `MinecraftServer.cpp:524`) this is
  meaningful CPU on the server tick thread (and on the client for mod/sign re-sends). Java also
  compressed in the packet constructor on the server thread, so it is parity — but a scheduler may
  want to offload compression to a worker and keep the queue-insertion cheap.

### H9 — Blocking saves at teardown
- `World::save(blocking=true)` waits `asyncSaveFuture_` (`World.cpp:312-314`); `ChunkCache::waitForPendingWrites`
  waits the save pool (`ChunkCache.cpp:349-355`, called from dtor and unload paths `:409,:421`).
  These are shutdown-only and acceptable, but they must remain reachable from a dedicated teardown
  phase, not the network drain, to avoid a deadlock if a network teardown waits on a save worker.

### H10 — Nested `disconnect()` re-entrancy
- `requestDisconnect` uses a CAS on `open_` (`Connection.cpp:336`) so it is idempotent, and
  `joinThreads` skips self-join (`Connection.cpp:352-360`) — good. But `ServerPlayNetworkHandler::disconnect`
  can be re-entered from within a packet handler (e.g. `onPlayerMove` → `disconnect("Flying...")`,
  `ServerPlayNetworkHandler.cpp:239`) while the handler is still on the drain stack; the Connection
  is torn down under the drainer. The `retiredWorlds_`/`retiredBridges_` deferral pattern
  (`ClientNetworkHandler.hpp:144-151`, `MultiplayerSession.hpp:12-19`) exists precisely for this on
  the client; the server relies on the `unique_ptr` ownership in `ConnectionListener`. A scheduler
  must keep this "defer destruction until the drain stack unwinds" invariant.

---

## 8. Where the main thread blocks on a future/thread today
- `Connection::disconnect()`/dtor → join reader+writer (`Connection.cpp:157-160,134-137,352-360`) — H1.
- `ClientNetworkHandler::beginPendingLogin` → `joinServerThread_.join()` (`ClientNetworkHandler.cpp:243-245`) — H6.
- `ClientNetworkHandler::~ClientNetworkHandler` → join (`:49-54`).
- `MultiplayerConnector::~MultiplayerConnector` → join connect thread (`MultiplayerConnector.cpp:63-68`)
  (usually already finished; ConnectScreen owns it).
- `ServerLoginNetworkHandler::verifyUsernameOnline` → `verifyThread_.join()` (`ServerLoginNetworkHandler.cpp:150-152`) — H5.
- `MinecraftServer::startAsync` → `startStateCv_.wait` until `init()` (`MinecraftServer.cpp:175-176`);
  `stopAndJoin` joins `runThread_` (`:184-189`).
- `World::save(true)` / `ChunkCache::waitForPendingWrites` at teardown — H9.
- `ServerProcessCoordinator::shutdown` → `WaitForSingleObject(process_, 10000)` + terminate
  (`ServerProcessCoordinator.cpp:279-290`) — LAN/dedicated process, not socket threads.

---

## 9. Recommended structure for network under a coordinated scheduler

Keep the **game-thread drain contract** (Java `processReadPackets`) — that is the compatibility
surface. Move only the *socket I/O* under the scheduler.

- **One I/O event loop (or small `WorkerPool`, 2–4 workers) owned by the scheduler**, replacing the
  per-connection `std::thread` pair:
  - Reader/writer tasks become event-driven state machines driven by a socket-poll/overlapped-IO
    loop; a connection is a small struct (socket, read buffer, decode cursor, pending sends) owned
    by the loop, never blocking.
  - `Connection` keeps its public API (`sendPacket`, `tick`, `disconnect`, `isOpen`,
    `getDelayedSendQueueSize`) but delegates I/O to the loop via `submit`/`cancel`.
- **Accept + login bootstrap also on the scheduler**: `ConnectionListener::listenLoop` becomes a
  scheduled accept task (poll `listenSocket`); rate-limiting stays (per-host 5 s window,
  `ConnectionListener.cpp:59-70`).
- **HTTP (auth/verify/mod-download) on a scheduler-managed HTTP worker pool**, never on the game
  thread and never joined mid-flight:
  - Client session verify: polled result (`joinServerState_`) exactly as today
    (`ClientNetworkHandler.cpp:55-83`), but the thread becomes a pool task; drop the
    `joinServerThread_.join()` in `beginPendingLogin` (H6).
  - Server `checkserver.jsp` verify: the verify task only publishes `deferredLoginPacket_`
    under `verifyMutex_`; `disconnect()`/`closed` move to the server tick thread (H5).
- **Teardown is async**: `Connection::disconnect()` requests close (CAS `open_` false,
  `shutdown(SD_BOTH)`, cancel read interest, flush) and returns immediately; the scheduler's loop
  observes EOF/close, drains remaining writes with a short grace, and then the object's owner
  (game thread, after the drain stack unwinds) destroys it. Never `join()` on the game thread (H1).
- **One drain per connection per tick**: track `lastDrainTick` in `Connection` (or gate in the
  handler) so the DownloadingTerrainScreen + ClientWorld double drain collapses to one (H7).
- **Per-tick budget accounting in `ConnectionListener::tick()`**: total budget (e.g. 3 ms) split
  across all connections with per-connection caps, preserving Java's "≤100 packets/player/tick"
  semantics as a *floor* fairness guarantee (parity §5 deviation 1).
- **Offload compress/expand**: move chunk `compressForSend` (`Connection.cpp:170-172`) and heavy
  chunk-apply to workers only if latency parity is preserved; otherwise keep on game thread but
  budgeted (H8).
- **Keep the retire-once-unwound invariant**: world/bridge/handler destruction must stay deferred
  off the drain stack (`Minecraft.cpp:765-775`, `MultiplayerSession.cpp:12-19`,
  `ClientNetworkHandler.hpp:144-151`) and must not depend on a scheduler thread being joined (H10).

### Cross-cutting rules for the scheduler restructure
- Packet **decode may stay on I/O tasks** (Java decodes on the reader thread), but packet **apply
  must stay on the game thread** — never move `apply` off-thread (would break every handler that
  mutates world/entity/screen, e.g. `WorldPacketHandlers.cpp`, `EntityPacketHandlers.cpp`,
  `ScreenPacketHandlers.cpp`, `ServerPlayNetworkHandler.cpp`).
- Shared state between I/O tasks and the game thread is only the **owned mailbox** (below) + the
  existing atomics (`open_`, `networkHandler_`, `sendQueueSize_`).
- Every thread that reads a `NetworkHandler*` must not assume handler lifetime (already an atomic;
  keep using an owning ref under the scheduler, e.g. shared ownership of the bridge/handler
  pair so I/O tasks never dangle during teardown).

---

## 10. Safe cross-thread packet mailbox design (bullets)

Goal: one `Connection` = one SPSC-style mailbox per direction, lock-free or near-lock-free, with
backpressure, owned by a single reader (game thread) / single writer (I/O loop).

- **Inbound mailbox (I/O → game thread):** fixed-capacity `std::deque` or intrusive ring of
  `unique_ptr<Packet>` guarded by one `std::mutex` (as today, `readMutex_`) **plus** a byte/entry
  cap. Producer (reader task) honors the cap; on overflow it drops and disconnects
  (`disconnect.overflow`) or applies read-side flow control (stop reading until drained). Expose a
  cheap `empty()`/`size()` for the game-thread `tick()` (existing `readQueueEmpty`,
  `Connection.cpp:365-368`).
- **Outbound mailbox (game thread → I/O loop):** today two queues (`sendQueue_`,
  `delayedSendQueue_`, `writeMutex_`). Keep the split (immediate vs chunk/world packets) to
  preserve the Java chunk-gating behavior. Add a **byte cap** already present
  (`sendQueueSize_ > 0x100000`, `Connection.cpp:185`) — keep checking it on the game thread in
  `tick()`, not in the producer, so disconnect semantics stay in the game loop (parity).
- **Copy-free transfer:** packet payloads are already `unique_ptr` — transfer ownership across the
  boundary (as today), never copy. Reserve `size()+1` accounting on push, decrement on write
  (`Connection.cpp:175,323`).
- **One mutex per direction** (already true) — do not share a single lock across read/write paths;
  avoids reader/writer contention. Under an event loop, the loop-side lock is held only while
  transferring the batch (existing batch pattern, `Connection.cpp:287-317`).
- **Wake-up:** keep a condition variable for the I/O side (or rely on the loop's poll tick); the
  game thread must never wait on it. `interrupt()`/close wakes the loop to observe the closed flag.
- **Lifetime:** the mailbox must outlive both the reader task and the writer task during teardown;
  hand it (or shared_ptr to the connection) to the I/O loop so a close races only against
  `open_`/socket shutdown, never against object destruction.
- **Per-connection packet accounting moves to the connection** (fix H4): counters live in the
  `Connection`; the game thread aggregates them for the debug/tracker view instead of mutating
  process-global statics from I/O threads.
- **Ordering guarantee:** FIFO within each direction; the drain must not reorder packets (Java
  preserves arrival order; `onPlayerMove`/`handleChunkData` ordering matters, e.g. teleport
  confirmation vs move echo, `ServerPlayNetworkHandler.cpp:149-155`).

---

## 11. Open questions for the planner/auditors

1. Event loop (epoll/IOCP-style, Windows `WSAPoll`/overlapped) vs a small thread pool for I/O?
   Windows-only target (winsock), so overlapping I/O or a poll loop both work; a pool of 2–4
   blocking-IO workers is the smaller diff and keeps the current `SocketInputStreamBuf`/`OutputStreamBuf`
   streambuf code (`Connection.cpp:30-119`) intact. But streambuf + blocking reads cannot be
   cancelled cheaply — evaluate replacing the reader streambuf with a packet-size-driven read into a
   scheduler-owned buffer.
2. Should the read-side cap be a disconnect (Java-style, `disconnect.overflow`) or backpressure
   (pause reading)? Parity says disconnect-on-send-overflow; read-side is new territory.
3. Keep the 100-packet/tick Java cap as the fairness floor, or keep the time-box and add
   per-connection fairness? Recommend: time-box + per-connection cap, min 8 as today.
4. `ServerLoginNetworkHandler` re-verify mid-login and `ClientNetworkHandler` re-join: confirm the
   new HTTP pool can guarantee "no two verifies for the same connection in flight" without joins.
5. Confirm `DownloadingTerrainScreen` drain removal is safe with the `KeepAlive` cadence during
   terrain download (keep-alive currently emitted by the screen; would move to
   `ClientNetworkHandler::tick`).
6. Whether `ServerProcessCoordinator` (external `minecraft_server.exe`, `ServerProcessCoordinator.cpp`)
   stays out of scope (it is process-level, not in-process threading; recommend yes).

---

## 12. Shared-file edits

None — this is a review-only council deliverable. No source files were modified.
