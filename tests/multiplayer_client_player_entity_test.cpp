#include "net/minecraft/client/multiplayer/MultiplayerClientPlayerEntity.hpp"
#include "net/minecraft/client/util/Session.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include <gtest/gtest.h>
namespace net::minecraft::test {
TEST(MultiplayerClientPlayerEntity, WaitsForTerrainSupportBeforeStart) {
  client::multiplayer::ClientNetworkHandler handler(nullptr);
  ClientWorld world(&handler, 12345ULL, 0);
  world.updateChunk(0, 0, true);
  client::multiplayer::MultiplayerClientPlayerEntity player(nullptr, &world, client::util::Session{}, &handler);
  player.setPosition(8.0, 80.0, 8.0);
  player.velocityX = 1.0;
  player.velocityY = -3.0;
  player.velocityZ = 1.0;
  player.fallDistance = 10.0f;
  handler.started = false;
  player.tick();
  EXPECT_DOUBLE_EQ(player.x, 8.0);
  EXPECT_DOUBLE_EQ(player.y, 80.0);
  EXPECT_DOUBLE_EQ(player.z, 8.0);
  EXPECT_DOUBLE_EQ(player.velocityX, 0.0);
  EXPECT_DOUBLE_EQ(player.velocityY, 0.0);
  EXPECT_DOUBLE_EQ(player.velocityZ, 0.0);
  EXPECT_FLOAT_EQ(player.fallDistance, 0.0f);
}
} // namespace net::minecraft::test
