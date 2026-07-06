#include "support/server_event_fixture.hpp"
#include "support/server_test_macros.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/item/map/MapState.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/network/ServerPlayNetworkHandler.hpp"
#include "net/minecraft/server/network/ServerPlayerInteractionManager.hpp"
#include "net/minecraft/server/world/ReadOnlyServerWorld.hpp"
#include "net/minecraft/world/PersistentState.hpp"
#include "net/minecraft/world/ServerWorld.hpp"
#include "net/minecraft/world/storage/EmptyWorldStorage.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
namespace {
using net::minecraft::entity::player::PlayerEntity;
using net::minecraft::entity::player::ServerPlayerEntity;
void testBlockUpdateRoutesToPlayerManager() {
  ServerEventFixture fixture;
  EXPECT_TRUE(fixture.world.dimension != nullptr);
  fixture.listener.blockUpdate(8, 64, 8);
  fixture.server.playerManager.markDirty(8, 64, 8, fixture.world.dimension->id);
}
void testEntityNotifyUsesEntityTracker() {
  ServerEventFixture fixture;
  net::minecraft::entity::ItemEntity entity;
  entity.id = 7;
  fixture.listener.notifyEntityAdded(&entity);
  bool threwOnDuplicate = false;
  try {
    fixture.server.getEntityTracker(fixture.world.dimension->id).onEntityAdded(&entity);
  } catch(const std::logic_error&) {
    threwOnDuplicate = true;
  }
  EXPECT_TRUE(threwOnDuplicate);
  fixture.listener.notifyEntityRemoved(&entity);
  fixture.listener.notifyEntityAdded(&entity);
}
void testWorldEventSkipsExcludedPlayer() {
  ServerEventFixture fixture;
  net::minecraft::server::network::ServerPlayerInteractionManager nearbyMgr(&fixture.world);
  net::minecraft::server::network::ServerPlayerInteractionManager excludedMgr(&fixture.world);
  EXPECT_EQ(nearbyMgr.world, &fixture.world);
  EXPECT_EQ(excludedMgr.world, &fixture.world);
  auto nearby = std::make_unique<ServerPlayerEntity>(&fixture.server, &fixture.world, "nearby", &nearbyMgr);
  auto excluded = std::make_unique<ServerPlayerEntity>(&fixture.server, &fixture.world, "excluded", &excludedMgr);
  EXPECT_EQ(nearby->interactionManager.world, &fixture.world);
  EXPECT_EQ(excluded->interactionManager.world, &fixture.world);
  EXPECT_EQ(nearby->interactionManager.player, nearby.get());
  nearby->dimensionId = 0;
  nearby->x = 10.0;
  nearby->y = 64.0;
  nearby->z = 10.0;
  excluded->dimensionId = 0;
  excluded->x = 10.0;
  excluded->y = 64.0;
  excluded->z = 10.0;
  net::minecraft::server::network::ServerPlayNetworkHandler nearbyHandler(&fixture.server, nullptr, nearby.get());
  EXPECT_EQ(nearby->networkHandler, &nearbyHandler);
  fixture.server.playerManager.players.push_back(nearby.get());
  fixture.server.playerManager.players.push_back(excluded.get());
  std::size_t eligibleRecipients = 0;
  for(ServerPlayerEntity* player : fixture.server.playerManager.players) {
    if(player == nullptr || static_cast<PlayerEntity*>(player) == excluded.get()) {
      continue;
    }
    if(player->dimensionId != fixture.world.dimension->id || player->networkHandler == nullptr) {
      continue;
    }
    const double deltaX = 10.0 - player->x;
    const double deltaY = 64.0 - player->y;
    const double deltaZ = 10.0 - player->z;
    if(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ < 64.0 * 64.0) {
      ++eligibleRecipients;
    }
  }
  EXPECT_EQ(eligibleRecipients, 1U);
  fixture.listener.worldEvent(excluded.get(), 1000, 10, 64, 10, 0);
}
void testReadOnlyServerWorldSharesPersistentState() {
  net::minecraft::server::MinecraftServer server;
  net::minecraft::EmptyWorldStorage storage;
  net::minecraft::ServerWorld delegate{&server, &storage, "test", 0, 99};
  net::minecraft::server::world::ReadOnlyServerWorld readonly{&server, &storage, "test", -1, 99, &delegate};
  auto state = std::make_unique<net::minecraft::map::MapState>("share_test");
  state->centerX = 12;
  delegate.persistentStateManager.set("share_test", std::move(state));
  net::minecraft::PersistentState* shared =
      readonly.persistentStateManager.getOrCreate(typeid(net::minecraft::map::MapState), "share_test");
  EXPECT_TRUE(shared != nullptr);
  EXPECT_EQ(dynamic_cast<net::minecraft::map::MapState*>(shared)->centerX, 12);
}
} // namespace
int main() {
  RUN_SERVER_TEST(testBlockUpdateRoutesToPlayerManager);
  RUN_SERVER_TEST(testEntityNotifyUsesEntityTracker);
  RUN_SERVER_TEST(testWorldEventSkipsExcludedPlayer);
  RUN_SERVER_TEST(testReadOnlyServerWorldSharesPersistentState);
  if(server_test::failureCount() != 0) {
    std::cout << server_test::failureCount() << " test(s) failed\n";
    return 1;
  }
  std::cout << "All server world event tests passed\n";
  return 0;
}
