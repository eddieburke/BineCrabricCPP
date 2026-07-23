#include <gtest/gtest.h>
#include "net/minecraft/client/multiplayer/MultiplayerClientPlayerEntity.hpp"
#include "net/minecraft/client/util/Session.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
namespace net::minecraft::test {
TEST(MultiplayerClientPlayerEntity, TicksNormallyWhenPosLoaded) {
 client::multiplayer::ClientNetworkHandler handler(nullptr);
 ClientWorld world(&handler, 12345ULL, 0);
 world.updateChunk(0, 0, true);
 client::multiplayer::MultiplayerClientPlayerEntity player(nullptr, &world, client::util::Session{}, &handler);
 player.setPosition(8.0, 80.0, 8.0);
 player.velocityX = 1.0;
 player.velocityY = -3.0;
 player.velocityZ = 1.0;
 player.fallDistance = 10.0f;
 player.tick();
 EXPECT_NE(player.x, 8.0);
 EXPECT_NE(player.y, 80.0);
}
TEST(MultiplayerClientPlayerEntity, RegistersOnlyOnce) {
 client::multiplayer::ClientNetworkHandler handler(nullptr);
 ClientWorld world(&handler, 12345ULL, 0);
 auto player = std::make_unique<client::multiplayer::MultiplayerClientPlayerEntity>(
     nullptr, &world, client::util::Session{}, &handler);
 player->setPosition(8.0, 80.0, 8.0);
 ASSERT_TRUE(world.spawnEntity(player.get()));
 ASSERT_TRUE(world.spawnEntity(player.get()));
 EXPECT_EQ(world.entities().size(), 1U);
 EXPECT_EQ(world.players.size(), 1U);
}
TEST(MultiplayerClientPlayerEntity, DropDispatchesToServerOverride) {
 client::multiplayer::ClientNetworkHandler handler(nullptr);
 ClientWorld world(&handler, 12345ULL, 0);
 world.updateChunk(0, 0, true);
 client::multiplayer::MultiplayerClientPlayerEntity player(nullptr, &world, client::util::Session{}, &handler);
 player.inventory.selectedSlot = 0;
 player.inventory.setStack(0, ItemStack(1, 1, 0));
 entity::player::PlayerEntity* basePlayer = &player;
 basePlayer->dropSelectedItem();
 const ItemStack* selected = player.inventory.getSelectedItem();
 ASSERT_NE(selected, nullptr);
 EXPECT_EQ(selected->count, 1);
}
} // namespace net::minecraft::test
