# EXECUTOR P1c REPORT — WI-9 / WI-10 (network packet-path correctness + async socket teardown)

Lane: P1c. Items: WI-9, WI-10. No builds/tests run (compile-fixer only). All edits grep-verified
against the working tree (2026-08-01) before editing; the tree had **no partial edits** from the
aborted prior attempt (verified: zero hits for `readQueueSize_`, `readOverflow_`, `mergeReadStats`,
`setDrainLimit`, `verifyInFlight_`, `deferredLoginError_`, `joinServerInFlight_` before I started).

---

## WI-9 — Network packet-path correctness

### Packet.hpp (data-race fix HZ-17)
- `:79-80` — `Packet::read()` no longer mutates `packetTrackers()[rawId]` / `++incomingCount()`.
- Added public merge/snapshot API (`:98-115`): `mergeReadStats(const std::vector<std::pair<int,int>>&)`
  (replays per-packet `(rawId, size)` into the global statics — preserves exact `PacketTracker`
  semantics without needing to sum the aggregate type), `resetReadStats()`, `snapshotReadStats()`,
  `incomingReadCount()`. The statics are now touched **only on the game/sim thread** (from
  `Connection::tick()`), so concurrent reader threads can never race on them.
- Added `<utility>`, `<vector>` includes.

### Connection.hpp / Connection.cpp (read-side cap, D2/Q8 closed)
- `Connection.hpp` — new members: `readQueueSize_` (atomic bytes), `readOverflow_` (atomic flag),
  `readStats_` (per-Connection `vector<pair<rawId,size>>`, reader-owned under `readMutex_`),
  `externalDrainLimit_` (`optional<DrainLimit>`), plus public `DrainLimit{deadline,maxPackets}` /
  `setDrainLimit` / `clearDrainLimit`.
- `Connection.cpp` — `constexpr kMaxReadQueueBytes = 0x100000` (mirrors the send cap `:198`).
  - `readLoop()` (`:290-323`): decodes, then **under `readMutex_`** checks `readQueueSize_ + size + 1
    > cap`; on overflow sets `readOverflow_` and stops reading (bounded queue — the reader never
    pushes past the cap); otherwise pushes + records `(rawId,size)`.
  - `tick()` (`:196-257`): observes `readOverflow_` → `requestDisconnect("disconnect.overflow")`;
    drains with `readQueueSize_` decrement on pop; merges `readStats_` into the global statics under
    `readMutex_` after the drain. Send-overflow check kept in `tick()`.

### ServerLoginNetworkHandler (verify thread publishes result ONLY; HZ-18/H5)
- `.hpp` — added `verifyInFlight_` (atomic in-flight flag, QD-14) and `deferredLoginError_`
  (`optional<string>`).
- `verifyUsernameOnline()` (`:157-190`) — removed the `verifyThread_.join()` (H5/HZ-06); the verify
  thread now **only publishes** `deferredLoginPacket_` (success) or `deferredLoginError_` (failure)
  under `verifyMutex_`. No `Connection`/`closed` mutation from the verify thread anymore (`:172/:175`
  are gone). Thread tagged `Domain::Io`.
- `tick()` (`:39-62`) — consumes `deferredLoginError_` and calls `disconnect(error)` **on the server
  tick thread**; all `Connection`/`closed` mutation therefore runs on the tick thread.
- The destructor still joins `verifyThread_` (teardown-only, safe — the lambda holds `this`).

### ClientNetworkHandler (HZ-06 — no join-to-reuse)
- `.hpp` — added `joinServerInFlight_` atomic + `keepAliveTicks_`.
- `beginPendingLogin()` (`:231-265`) — no longer joins a prior `joinServerThread_`; an in-flight
  verify is reused (exchange guard), a finished thread is `detach()`ed (its result was consumed
  under `joinServerMutex_`, so it has fully stopped touching `this`), and the result is polled via
  `processPendingJoinServer` (`:55-83`) as before. Join happens only in the destructor (teardown).

### Double-drain (H7) + drain policy (QD-04)
- `DownloadingTerrainScreen.hpp` — `tick()` (drain + keep-alive) **removed**; the screen now inherits
  the empty `Screen::tick()`. Drain collapses into `ClientWorld::tick()` (`:53-55`, unchanged — it
  already called `networkHandler_->tick()`). Ctor signature kept (`PlayerPacketHandlers.cpp:133`
  still constructs it); dead `networkHandler_` member removed, param `(void)`-cast.
- `ClientNetworkHandler::tick()` (`:84-95`) — now emits `KeepAlivePacket{}` every 20 ticks (QD-15
  cadence preserved). Note: this extends proactive keep-alive to login+play phases, not just the
  terrain screen; harmless (default `NetworkHandler::onKeepAlive` and `ServerPlayNetworkHandler::onKeepAlive`
  are no-ops / timestamp-only). One drain per tick: during terrain download the world is remote, so
  `Minecraft.cpp:655-657` skips the session tick and only `ClientWorld::tick` drains.
- Drain policy preserved in `Connection::tick()`: 3 ms time-box / min 8 / max 4096 default;
  `interrupt()` after every drain kept (ClientNetworkHandler + ConnectionListener); writer's
  `preferImmediate` data-before-chunk gate untouched.

---

## WI-10 — Async socket teardown; never join on the game thread; NetIo registration

### Connection.cpp — async `disconnect()` (H1)
- `disconnect()` (`:164-167`) — **no longer joins**; `requestDisconnect()` (CAS `open_` false →
  `shutdown(SD_RECEIVE)` unblocks `recv` → `writeCv_.notify_all()`) and returns immediately.
  **Deviation from plan text:** I kept `shutdown(SD_RECEIVE)` in `requestDisconnect` (the plan's own
  re-entrancy bullet pins SD_RECEIVE at `:341`) so the writer's send side stays alive for the
  flush-grace on healthy connections; the dead-peer send-block is force-unblocked by the watchdog in
  `joinThreads()` (`shutdown(SD_BOTH)` after 250 ms). Net effect matches the plan's intent: unblocks
  recv, cancels read interest, flush with short grace, returns immediately.
- `writeLoop()` (`:324-`) — tagged `Domain::NetIo`; adds a 100 ms close-flush grace after `open_`
  goes false, then drops remaining queued writes (`sendQueueSize_` zeroed) and exits.
- `joinThreads()` (`:406-418`) — **unblock-then-watchdog-join**: waits up to 250 ms for the
  reader/writer to exit on their own (they observe close promptly), then `shutdown(SD_BOTH)` to
  force-unblock a dead-peer `send`, then joins. Bounded (never the old ~30 s SO_SNDTIMEO stall).
  `~Connection()` (`:141-145`) = `disconnect()` + `joinThreads()` (reclaim) + `releaseDynamic(2)`.
  Full "Lifecycle::shutdown wiring" (WI-13) is PASS-2; I did NOT register per-Connection Lifecycle
  owners because the registry has no unregister and a shutdown-time join lambda capturing `this`
  would dangle once a mid-run Connection is destroyed — see deviations.
- `Connection.hpp` — `socket_` is now `std::atomic<SOCKET>`; `shutdownSocket()` (`:399-404`) uses
  `exchange(INVALID_SOCKET)` so it is **strictly idempotent and race-free** (double-close risk closed).
  `setSocketOptions`/`formatAddress`/`requestDisconnect` read via `socket_.load()`.
- NetIo registration (Q9): ctor `reserveDynamic(2)`, dtor `releaseDynamic(2)`; both thread bodies set
  `util::concurrent::tl_domain = Domain::NetIo` (`ThreadNames.hpp`). Event loop deferred per Q9.

### ConnectionListener.cpp — per-tick budget split
- `tick()` (`:100-173`) — one shared 3 ms deadline for the whole tick plus a per-connection cap of
  100 packets/player (Java's ≤100/player fairness floor). `setDrainLimit`/`clearDrainLimit` wrap each
  `handler.tick()`; `Connection::tick` honors the external `DrainLimit` (its internal min-8 floor
  still guarantees ≥8/player even once the shared deadline has passed). `interrupt()` after every
  drain preserved. Client-side connections keep the default 3 ms/4096 (no limit set).

### Re-entrancy (H10)
- `ServerPlayNetworkHandler::disconnect` from inside a packet handler now only CASes the connection
  closed (async) — no join on the drain stack; destruction stays deferred via the existing retire
  pattern (`MultiplayerSession.cpp:12-19`, listener post-tick re-push), so a `Connection` referenced
  by a pending handler is never destroyed under it.

---

## Tests created (self-contained, no GL; wired by the compile-fixer per WI-T pre-registration)

- `tests/packet_accounting_test.cpp` — 4 loopback connection pairs, readers running concurrently;
  drains each client on the game thread; asserts merged globals are exact
  (`incomingReadCount == 800`, tracker[0].count == 800) → no torn/lost/double-counted updates.
- `tests/connection_async_teardown_test.cpp` — Connection to a black-holed peer (accepts, never
  reads); floods 200 chunk packets so the writer blocks in `send`; asserts `disconnect()` returns
  < 250 ms, destruction (watchdog SD_BOTH + join) completes < 3 s, and both thread counters return
  to 0.

---

## Deviations / line-drift corrections (vs plan-master §4.1)

1. **`ServerLoginNetworkHandler.hpp` edited** — §6's P1c file list omits it, but WI-9 requires a
   per-connection in-flight flag and a deferred failure result, which are class members. No other
   lane owns the header; compile-fixer should reconcile. Kept the edit minimal (two members + `<atomic>`).
2. **`shutdown(SD_RECEIVE)` instead of `shutdown(SD_BOTH)` in `disconnect()`/`requestDisconnect`** —
   the plan's WI-10 bullet says SD_BOTH, its re-entrancy bullet pins SD_RECEIVE at `:341`. I kept
   SD_RECEIVE to preserve the healthy-path DisconnectPacket/flush delivery; the dead-peer send-block
   is covered by the 250 ms watchdog `shutdown(SD_BOTH)` in `joinThreads()`.
3. **No per-Connection `Lifecycle::registerOwner`** — WI-13 (PASS-2) wires `Lifecycle::shutdown()`
   into `Minecraft::stop`. Registering owners that capture `this` is unsafe (no unregister; entries
   would dangle after mid-run Connection destruction). The `reserveDynamic(2)` + NetIo domain +
   in-thread unblock-then-watchdog-join cover WI-10's testable contract.
4. **`joinThreads()` still runs on the game thread at destruction** — bounded (≤ ~250 ms watchdog,
   normally ~0 because disconnect already unblocked the threads). "Never join on the game thread" in
   full is only reachable once PASS-2 WI-13 moves joins into Lifecycle; the H1 stall (30 s) is
   eliminated now.
5. **`readStats_` uses `vector<pair<rawId,size>>` replay merge** instead of an aggregate map — because
   `PacketTracker.hpp` (not in my file list) exposes no merge API; replaying `update(size)` preserves
   the existing global `PacketTracker` semantics exactly and is bounded by the read cap.
6. **`ClientWorld.cpp` and `MultiplayerSession.cpp` listed as editable but unchanged** — the drain was
   already in `ClientWorld::tick` (`:53-55`) and the retire pattern already exists
   (`MultiplayerSession.cpp:12-19`); the screen-drain removal + keep-alive move satisfy H7 without
   touching them.
7. **Proactive keep-alive now also runs during login/play** (from `ClientNetworkHandler::tick`), not
   just during the terrain screen — per QD-15 the cadence moved; verified the login handler's
   default `onKeepAlive` is a no-op so this cannot break the handshake.

## Files touched
- Edited: `network/Packet.hpp`, `network/Connection.hpp`, `network/Connection.cpp`,
  `server/network/ServerLoginNetworkHandler.cpp`, `server/network/ServerLoginNetworkHandler.hpp`,
  `server/network/ConnectionListener.cpp`, `client/multiplayer/ClientNetworkHandler.hpp`,
  `client/multiplayer/ClientNetworkHandler.cpp`, `client/gui/screen/DownloadingTerrainScreen.hpp`.
- New: `tests/packet_accounting_test.cpp`, `tests/connection_async_teardown_test.cpp`.
- Not touched (owned, no change needed): `world/ClientWorld.cpp`, `client/multiplayer/MultiplayerSession.cpp`.
- No other lane's files touched. No CMakeLists.txt edit (L1 owns it; my two tests are already
  pre-registered there).
