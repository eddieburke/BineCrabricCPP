#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
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
#include "support/server_event_fixture.hpp"
namespace {
using net::minecraft::entity::player::PlayerEntity;
using net::minecraft::entity::player::ServerPlayerEntity;
class TickProbeBlock : public net::minecraft::block::Block {
public:
  static constexpr int kId = 250;
  int ticks = 0;
  TickProbeBlock() : Block(kId, net::minecraft::block::material::Material::STONE) {
  }
  void onTick(net::minecraft::World*, int, int, int, net::minecraft::JavaRandom&) override {
    ++ticks;
  }
};
} // namespace
namespace net::minecraft::test {
TEST(ServerWorldEvents, BlockUpdateRoutesToPlayerManager) {
  ServerEventFixture fixture;
  ASSERT_NE(fixture.world.dimension, nullptr);
  fixture.listener.blockUpdate(8, 64, 8);
  fixture.server.playerManager.markDirty(8, 64, 8, fixture.world.dimension->id);
}
TEST(ServerWorldEvents, EntityNotifyUsesEntityTracker) {
  ServerEventFixture fixture;
  net::minecraft::entity::ItemEntity entity;
  entity.id = 7;
  bool threwOnSpawn = false;
  try {
    fixture.world.notifyEntityAdded(&entity);
  } catch(const std::logic_error&) {
    threwOnSpawn = true;
  }
  EXPECT_FALSE(threwOnSpawn);
  fixture.world.notifyEntityRemoved(&entity);
}
TEST(ServerWorldEvents, WorldEventSkipsExcludedPlayer) {
  ServerEventFixture fixture;
  net::minecraft::server::network::ServerPlayerInteractionManager nearbyMgr(&fixture.world);
  net::minecraft::server::network::ServerPlayerInteractionManager excludedMgr(&fixture.world);
  auto nearby = std::make_unique<ServerPlayerEntity>(&fixture.server, &fixture.world, "nearby", &nearbyMgr);
  auto excluded = std::make_unique<ServerPlayerEntity>(&fixture.server, &fixture.world, "excluded", &excludedMgr);
  nearby->dimensionId = 0;
  nearby->x = 10.0;
  nearby->y = 64.0;
  nearby->z = 10.0;
  excluded->dimensionId = 0;
  excluded->x = 10.0;
  excluded->y = 64.0;
  excluded->z = 10.0;
  net::minecraft::server::network::ServerPlayNetworkHandler nearbyHandler(&fixture.server, nullptr, nearby.get());
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
TEST(ServerWorldEvents, ReadOnlyServerWorldSharesPersistentState) {
  net::minecraft::server::MinecraftServer server;
  net::minecraft::EmptyWorldStorage storage;
  net::minecraft::ServerWorld delegate{&server, &storage, "test", 0, 99};
  net::minecraft::server::world::ReadOnlyServerWorld readonly{&server, &storage, "test", -1, 99, &delegate};
  auto state = std::make_unique<net::minecraft::map::MapState>("share_test");
  state->centerX = 12;
  delegate.persistentStateManager.set("share_test", std::move(state));
  net::minecraft::PersistentState* shared =
      readonly.persistentStateManager.getOrCreate(typeid(net::minecraft::map::MapState), "share_test");
  ASSERT_NE(shared, nullptr);
  EXPECT_EQ(dynamic_cast<net::minecraft::map::MapState*>(shared)->centerX, 12);
}
TEST(ServerWorldEvents, SynchronizedTimeKeepsScheduledTickDelay) {
  static TickProbeBlock block;
  block.ticks = 0;
  ServerEventFixture fixture;
  fixture.world.setEventProcessingEnabled(true);
  for(int chunkX = -1; chunkX <= 1; ++chunkX) {
    for(int chunkZ = -1; chunkZ <= 1; ++chunkZ) {
      auto& chunk = fixture.world.getChunk(chunkX, chunkZ);
      (void)chunk;
    }
  }
  fixture.world.setTime(100);
  ASSERT_TRUE(fixture.world.setBlock(8, 64, 8, TickProbeBlock::kId));
  fixture.world.scheduleBlockUpdate(8, 64, 8, TickProbeBlock::kId, 20);
  fixture.world.synchronizeTimeAndUpdates(1000);
  fixture.world.setTime(1019);
  fixture.world.processScheduledTicks(false);
  EXPECT_EQ(block.ticks, 0);
  fixture.world.setTime(1020);
  fixture.world.processScheduledTicks(false);
  EXPECT_EQ(block.ticks, 1);
}
} // namespace net::minecraft::test
