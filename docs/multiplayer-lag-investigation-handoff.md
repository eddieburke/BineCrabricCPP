# Multiplayer client-side lag investigation — handoff

## Symptom reported

Extreme client-side "lag" when joining Java-compatible multiplayer servers, reported as
clearly not the server's fault. Originally reported alongside "join fails to load chunks
about 50% of the time," which turned out to be a related but distinct, now-fixed bug (see
below). The lag itself is still an open question — this doc exists so whoever picks it up
next doesn't have to re-derive what's already known.

## Status: root cause NOT confirmed. One major cause ruled out. One hypothesis narrowed
but not proven.

Do not treat anything below as "fixed" unless it's in the "Fixed and test-verified"
section. The rest is diagnosis in progress.

---

## Fixed and test-verified (unrelated to the open lag question, but found along the way)

These were real bugs, confirmed against the Java b1.7.3 reference (`third_party/mcp/src/net/minecraft`)
and locked in with tests. They explain the *50/50 chunk load failure*, not necessarily the
*lag*.

1. **Client movement-ack echoed the wrong field** —
   [`PlayerPacketHandlers.cpp`](../src/net/minecraft/client/multiplayer/PlayerPacketHandlers.cpp)
   `makePlayerMoveResponsePacket`. After the entity-coordinate refactor (`player.y` = eye
   level, not feet), this still echoed `player.y` as the packet's `feetY` field. Java's
   client always echoes `boundingBox.minY` as feet. The server's teleport-confirmation gate
   (`ServerPlayNetworkHandler.onPlayerMove`, requires the echoed y within 0.1 of the
   teleport target) never latched, so `playerTick(true)` — the only thing that drains
   `pendingChunkUpdates` into `ChunkDataS2CPacket`s — never ran. This is why chunk loading
   was coin-flip: whether it worked depended on whether a *different* movement-packet path
   happened to send bit-exact coordinates before physics drift.
2. **Server teleport packet had swapped fields** —
   [`ServerPlayNetworkHandler.cpp:126`](../src/net/minecraft/server/network/ServerPlayNetworkHandler.cpp) `teleport()`.
   Mirror-image bug on the C++ server, needed for parity once (1) was fixed, otherwise
   players joining the project's own C++ server would land 1.62 blocks off.
3. **Lua screen event dispatch could deref a null global** —
   [`LuaDirectHooks.cpp`](../src/net/minecraft/mod/runtime/LuaDirectHooks.cpp),
   [`LuaScreenBindings.cpp`](../src/net/minecraft/mod/runtime/LuaScreenBindings.cpp).
   Unrelated to networking; found via a saved `build/crash-report.txt` from the same
   session. `g_activeLuaScreen` was cleared to `nullptr` (not restored) by nested screen
   events, and re-read once per subscribed mod callback instead of snapshotted. Fixed with
   an RAII save/restore scope and an upfront snapshot.

Tests: `tests/mp_parity_updates_test.cpp` (`MovementAckUsesActualPlayerY`,
`ServerTeleportSendsFeetYAndStanceCorrectly`). Both previously existed and were *wrong* —
one asserted the exact buggy behavior, the other asserted nothing. Both now drive real
`Connection`/socket round trips and check actual wire bytes, not just in-process structs.
92/92 tests passing as of the last full run (commit `50d8c0e3`, working tree dirty — see
"Repo state" below).

---

## Open hypotheses for the lag itself

### H1: Running an unoptimized Debug (`-O0`) build — likely, NOT directly measured

`build/CMakeCache.txt` (as of investigation start) had `CMAKE_BUILD_TYPE=Debug`,
`CMAKE_CXX_FLAGS_DEBUG=-g` — no `-O` flag at all. The exe launched from `build/` was
636 MB; a Release build from `build-omega.ps1`'s default `-BuildDir build-omega` produced
a 51 MB exe that (per repo evidence — no `client.log`/`crash-report.txt` in that directory)
had never actually been run.

**This is circumstantial, not proven.** Nobody has yet run the same join, side by side, in
Debug vs. Release and compared. That is the single highest-value next step and hasn't been
done. See "Next steps" below.

### H2: `Connection::tick()`'s packet-drain budget — REFUTED for light packets, OPEN for heavy ones

[`Connection.cpp:195-220`](../src/net/minecraft/network/Connection.cpp): drain loop reads
`readQueue_` with `kMinDrain=8`, `kMaxDrain=4096`, and a 3ms wall-clock cutoff
(`kDrainBudget`), versus Java's unconditional 101-packets-per-tick
(`third_party/mcp/src/net/minecraft/network/Connection.java:310`, `int n = 100; while
(!readQueue.isEmpty() && n-- >= 0)`).

Wrote `tests/connection_packet_drain_throughput_test.cpp` to measure this directly over a
real loopback `Connection` pair: flooded 2000 `KeepAlivePacket`s, then measured how many
`tick()` calls and how much wall time it took the receiving side to apply all of them.

**Result:** `applied=2000/2000 ticks=1 maxAppliedInOneTick=2000 wallMs=0 avgPerTick=2000.0`
— the entire backlog drained in a single `tick()` call, under 1ms, in the Debug build.
**The queue/lock/loop machinery itself is not the bottleneck.**

**But this doesn't close the question.** `KeepAlivePacket::apply()` is a no-op
(`onKeepAlive` does nothing but ack). The real join-time cost is `ChunkDataS2CPacket::apply()`
→ `NetworkHandler::handleChunkData` →
[`World::handleChunkDataUpdate`](../src/net/minecraft/world/WorldChunks.cpp:370) →
[`Chunk::loadFromPacket`](../src/net/minecraft/world/chunk/Chunk.hpp:270), which does a zlib
inflate (`ChunkDataS2CPacket::read`, [`ChunkPackets.hpp:69-71`](../src/net/minecraft/network/packet/ChunkPackets.hpp))
plus several `std::copy_n` calls moving up to ~82KB per chunk into `blocks`/`meta`/
`blockLight`/`skyLight` arrays, then a heightmap scan (`populateHeightMapOnly`). None of
that was exercised by the throughput test — it used a trivial packet specifically to
isolate the loop mechanism from per-packet cost.

**So the corrected, narrower claim: the 3ms/tick budget could still matter, but only
because of how expensive *inflate + memcpy + heightmap* are per packet in an unoptimized
build, not because the loop/queue machinery is slow.** If each `ChunkDataS2CPacket::apply()`
costs (hypothetically) 2-3ms in `-O0` vs. <0.1ms in `-O3`, then the *same* 3ms budget that
drained 2000 KeepAlives instantly might only clear 1-2 chunk packets per tick in Debug,
while clearing dozens in Release — reproducing exactly what "extreme lag, not the server's
fault" would look like on the client during a join or fast-travel chunk burst.

**This has not been measured.** It's the single open technical question this doc exists to
flag.

---

## Next steps (not yet done)

In priority order:

1. **Direct Debug-vs-Release comparison, same join.** Build both
   (`build-omega.ps1 -BuildDir build -BuildType Debug` and
   `build-omega.ps1 -BuildDir build-omega -BuildType Release`, or just `-BuildType
   RelWithDebInfo` if symbols are wanted for profiling), connect to the same server with
   each, and compare perceived lag / tick timing. This is the test that actually confirms
   or kills H1. Nobody has done this yet — it's the highest-value next action.
2. **Extend `connection_packet_drain_throughput_test.cpp` (or add a sibling test) using
   real `ChunkDataS2CPacket` payloads instead of `KeepAlivePacket`.** Flood N chunk packets
   of realistic size (16×128×16, matching what `ServerLoginNetworkHandler`/`ChunkMap` sends
   on join) through a real `Connection` pair and measure `ticks`/`wallMs` the same way. This
   directly tests the "budget matters once packets are heavy" half of H2, in isolation from
   full server/world startup cost (which the existing `MultiplayerChunkDelivery` tests
   conflate with everything else — see below).
3. **If (2) confirms the budget is the limiter under heavy packets:** the loop only needs to
   be more count-generous under backlog (closer to Java's flat 101), or the 3ms cutoff needs
   raising when a large burst is known to be in flight (e.g. during the initial spawn-chunk
   download). Both are small, contained changes to
   [`Connection.cpp`](../src/net/minecraft/network/Connection.cpp)'s drain loop — do not
   change this without (2)'s data in hand, since the loop was deliberately time-boxed for a
   documented reason (see the comment at `Connection.cpp:195`: avoiding a frozen game loop
   during local/LAN joins with big bursts). A wrong parameter change trades one failure mode
   for the other.
4. **If (1) confirms Debug/`-O0` is the dominant cost:** no code fix needed — it's a
   configuration/deployment issue (make sure `build-omega/` Release output is what actually
   gets run/distributed), not an engine bug.

### Existing data points that may help scope (2) and (1)

From the last full Debug-build test run (`ctest`, 92/92 passing):

| Test | Time | What it does |
|---|---|---|
| `ConnectionPacketDrainThroughput.TickDrainsFarMoreThanTheMinimumUnderBacklog` | 0.39s | 2000 trivial packets, real socket |
| `MultiplayerChunkDelivery.ServerDeliversChunksToSocketClient` | 10.29s | Real server + real socket client, waits for ≥9 real `ChunkDataS2CPacket`s (spawn-area chunks) |
| `MultiplayerChunkDelivery.IdleSocketClientStaysConnectedAfterInitialChunkBurst` | 13.08s | Same, plus idles past the burst watching for keep-alives |
| `IntegratedServerHost.UsesAutomaticPortAndInMemoryProperties` | 44.00s | Full server host startup (world gen included — not representative of steady-state join cost) |
| `IntegratedServerHost.PreservesCustomPort` | 49.98s | Same |

The `MultiplayerChunkDelivery` timings (~10-13s for what's a small handful of chunks) are
suggestive but **not a clean measurement** — they include server-side world generation and
decoration time, not just client-side packet-apply cost, so they can't be attributed to H1
or H2 without further isolation. That's exactly the gap step (2) above is meant to close.

---

## Repo state as of writing

HEAD: `50d8c0e3` ("Entity coordinate refactor, stb_vorbis migration, rendering fixes, MP
chunk loading fix"). Working tree has uncommitted changes; nothing in this investigation has
been committed.

Files changed by this investigation (all test-verified, 92/92 passing):
- `src/net/minecraft/client/multiplayer/PlayerPacketHandlers.cpp` — fix #1 above
- `src/net/minecraft/server/network/ServerPlayNetworkHandler.cpp` — fix #2 above
- `src/net/minecraft/mod/runtime/LuaDirectHooks.cpp` — fix #3 above
- `src/net/minecraft/mod/runtime/LuaScreenBindings.cpp` — fix #3 above
- `tests/mp_parity_updates_test.cpp` — corrected two pre-existing tests that were wrong
- `tests/connection_packet_drain_throughput_test.cpp` — new, added the H2 throughput data
- `CMakeLists.txt` — registers the new test file

Files showing as modified but **not touched by this investigation** (pre-existing, unrelated
— do not assume they're connected to this work):
- `build-omega.ps1`
- `mods/item_drop_physics/scripts/main.lua`
- `src/net/minecraft/network/packet/InventoryPackets.hpp`
- `src/net/minecraft/network/packet/PacketItems.hpp`

`build/` was observed to disappear mid-investigation (likely a `-Clean` run in progress by
whoever has the terminal) — if you're picking this up and `build/` is missing or
`.build-omega.lock` exists, check for a running build before starting another; the lock
file exists specifically to prevent two concurrent `build-omega.ps1` runs from racing.
