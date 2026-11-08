#include <gtest/gtest.h>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/item/HeldItemRenderer.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"

TEST(HeldItemRefresh, ReplacingSelectedSlotUpdatesWithoutScrolling) {
 using namespace net::minecraft;
 auto* previousClient = client::Minecraft::INSTANCE;
 {
  struct TestClient : client::Minecraft {
   TestClient() : Minecraft(320, 240, false) {}
   void handleCrash(const util::crash::CrashReport&) override {}
  } minecraft;
  entity::player::ClientPlayerEntity player(&minecraft, nullptr, client::util::Session{}, minecraft.options, 0);
  minecraft.player = &player;
  client::render::item::HeldItemRenderer renderer(&minecraft);
  player.inventory.selectedSlot = 0;
  player.inventory.main[0] = ItemStack(1, 64, 0);
  for(int i = 0; i < 6; ++i) renderer.tick();
  ItemStack* address = player.inventory.getSelectedItem();
  player.inventory.main[0] = ItemStack(35, 12, 5);
  EXPECT_EQ(address, player.inventory.getSelectedItem());
  for(int i = 0; i < 6; ++i) renderer.tick();
  EXPECT_EQ(renderer.stack.itemId, 35);
  EXPECT_EQ(renderer.stack.getDamage(), 5);
  player.inventory.main[0] = ItemStack(35, 7, 2);
  for(int i = 0; i < 6; ++i) renderer.tick();
  EXPECT_EQ(renderer.stack.getDamage(), 2);
  EXPECT_EQ(renderer.stack.count, 7);
  player.inventory.main[0] = ItemStack{};
  for(int i = 0; i < 6; ++i) renderer.tick();
  EXPECT_TRUE(renderer.stack.empty());
  minecraft.player = nullptr;
 }
 client::Minecraft::INSTANCE = previousClient;
}
