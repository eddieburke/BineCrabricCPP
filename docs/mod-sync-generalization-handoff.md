# Mod client/server sync + naming cleanup — implemented

## Why this document exists

Record of a session that started as a minecart-mount bug fix, widened into a
complaint about client/server naming clarity, and finished with the mod-entity
sync mechanism actually fixed (not just planned). Kept as a reference for the
current shape of this code, not as a TODO — everything below is done and green.

## What shipped (all in existing files, no new files)

### Correctness fixes (unrelated to naming, found along the way)
- **Minecart mount**: `Entity::setVehicle` made `virtual`
  ([entity/Entity.hpp](../src/net/minecraft/entity/Entity.hpp)); added override
  `ServerPlayerEntity::setVehicle` that sends `EntityVehicleSetS2CPacket` + teleport
  ([entity/player/ServerPlayerEntity.cpp](../src/net/minecraft/entity/player/ServerPlayerEntity.cpp)).
  Right-click-to-mount only worked server-side before this; the client never
  learned it happened.
- **Minecart slope math**: `snapPositionToRail` restored Java's `* 2.0` on the
  vertical delta ([entity/vehicle/MinecartEntity.cpp](../src/net/minecraft/entity/vehicle/MinecartEntity.cpp)).
- **Particle viewport**: particles now anchor to the frame camera origin instead
  of recomputing their own from the camera entity
  ([client/particle/ParticleManager.hpp](../src/net/minecraft/client/particle/ParticleManager.hpp)).

### Naming
- `clientPitch`/`clientYaw` → `clientTargetYaw`/`clientTargetPitch` in
  `MinecartEntity` and `BoatEntity` — the Java decompile named these swapped
  (pitch field held yaw, yaw field held pitch); Java breadcrumb comments mark it.
- `runtime::activeWorld()` → `runtime::contextOrClientWorld()`
  ([mod/runtime/ModHostUtil.hpp](../src/net/minecraft/mod/runtime/ModHostUtil.hpp)):
  same behavior, but the name and doc comment now say what it does — return the
  mod-context world, or fall back to the CLIENT replica world when no scope is
  active. `LuaChunkContext::activeWorld()` (a separate worldgen-only accessor)
  was left alone.

### Event glue consolidation (the "lua event glue... consolidated" ask)
[mod/runtime/LuaEventGlue.hpp](../src/net/minecraft/mod/runtime/LuaEventGlue.hpp) /
[.cpp](../src/net/minecraft/mod/runtime/LuaEventGlue.cpp) gained two canonical hooks
that replace scattered ad hoc `#ifdef MINECRAFT_NATIVE_EXPORTS` + `Minecraft::INSTANCE`
checks with one real definition each:
- `isClientBuild()` — are we the client process at all.
- `isLocalPlayer(const PlayerEntity*)` — is this entity the local client's own
  player. Previously a private copy duplicated inside
  [mod/runtime/LuaEventSubscribers.cpp](../src/net/minecraft/mod/runtime/LuaEventSubscribers.cpp);
  deleted from there, now calls the shared one (header was already included).

Left alone: the handful of `Minecraft::INSTANCE != nullptr` checks in
LuaScreenBindings/LuaTextureBindings/LuaWorldBindings/ModHost — each guards a
genuinely different client-only resource (textRenderer, currentScreen, a raster
image, a fallback world), not the same duplicated boolean. Consolidating those
would just be indirection for its own sake.

### Mod-entity sync — the actual "god awful" fix

Investigation found **two** parallel sync mechanisms, not one uniformly bad
system:
- Mod **block entities** already ride vanilla's dirty-block flush
  (`BlockEntity::markDirty()` → `ChunkMap` → `createUpdatePacket`) — this was
  already correct and automatic. Left untouched.
- Mod **entities** were the actual problem: every `setData`/`setRegistryId` call
  bumped a `syncVersion_` counter, and the tracker compared it every tick and, on
  change, resent a **full spawn packet** — which hard-reset the client replica's
  position and killed any interpolation. Client mod entities also ran full
  gravity/movement simulation on the packet-driven replica, fighting the
  positions packets kept snapping it to.

Fix, entirely in the existing files (no new packet kind, no new codec, no new
mixin — the earlier plan's "collapse to Snapshot/DataUpdate + ModSyncCodec"
turned out to be unnecessary complexity once the client stopped snapping):

- [mod/lua/LuaModEntity.hpp](../src/net/minecraft/mod/lua/LuaModEntity.hpp) /
  [.cpp](../src/net/minecraft/mod/lua/LuaModEntity.cpp):
  - `syncVersion_` (counter) → `dirty_` (bool) + `takeDirty()` (consume-and-clear,
    same shape as `DataTracker::getDirtyEntries`).
  - Added the same client-interpolation fields and pattern already established by
    `MinecartEntity`/`BoatEntity` (`clientX_/Y_/Z_`, `clientTargetYaw_/Pitch_`,
    `clientInterpolationSteps_`), plus an override of
    `setPositionAndAnglesAvoidEntities` that stores interpolation targets instead
    of snapping.
  - `tick()` now branches on `world->isRemote()`: the client replica advances
    interpolation only (no gravity/move — it is packet-driven, not simulated);
    the server keeps the original physics unchanged.
- [server/entity/EntityTrackerEntry.cpp](../src/net/minecraft/server/entity/EntityTrackerEntry.cpp) /
  [.hpp](../src/net/minecraft/server/entity/EntityTrackerEntry.hpp): the
  `syncVersion() != modSyncVersion_` comparison and its `modSyncVersion_` field
  are gone. The tracker now does `if (modEntity->takeDirty()) { resend }` —
  still the same wire packet (`LuaModSyncKind::EntitySnapshot` /
  `createAddEntityPacket`), because...
- [client/multiplayer/ClientNetworkHandler.cpp](../src/net/minecraft/client/multiplayer/ClientNetworkHandler.cpp)
  `onLuaModSync` now tells first-sight from a resend (`newEntity` was already
  computed): a new entity still gets a hard `setPositionAndAngles` + prev/lastTick
  reset; an existing one routes the transform through
  `setPositionAndAnglesAvoidEntities` (the new interpolation override) instead.
  So the same `EntitySnapshot` packet now doubles as a data update without ever
  disturbing smoothed motion — no new kind needed.

Net effect: mod authors call `entity:set_data(...)` as before; the client copy
glides continuously and updates data with no flicker/respawn; the server side is
otherwise unchanged (same physics, same packet, same wire format).

## Verification

- `native/build-omega.ps1` (exit 0) after every step above.
- `native/build-omega.ps1 -RunTests`: 48/48 passed, including
  `PacketRegistry.LuaModSyncRoundTrip`.
- Not yet done: driving a live LAN/MP session to eyeball a mod entity gliding
  and mounting a minecart end-to-end. Worth doing before calling this closed.

## Deliberately not done

- Splitting `mod/runtime/` into client/ and server/ subdirectories, or tagging
  events with a `remote` flag — would be pure file churn with no behavior or
  clarity payoff beyond what the naming/hook fixes above already gave. Only
  worth revisiting if a specific new confusion shows up.
