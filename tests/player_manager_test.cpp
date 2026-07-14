#include <gtest/gtest.h>
#include <filesystem>
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/PlayerManager.hpp"
#include "net/minecraft/block/entity/FurnaceBlockEntity.hpp"
#include "net/minecraft/inventory/SimpleInventory.hpp"
#include "net/minecraft/screen/FurnaceScreenHandler.hpp"
#include "net/minecraft/screen/ScreenHandler.hpp"
#include "net/minecraft/screen/slot/Slot.hpp"
namespace net::minecraft::test {
namespace {
class CurrentPathGuard {
public:
  explicit CurrentPathGuard(const std::filesystem::path& path) : previous_(std::filesystem::current_path()) {
    std::filesystem::current_path(path);
  }
  ~CurrentPathGuard() {
    std::filesystem::current_path(previous_);
  }

private:
  std::filesystem::path previous_;
};
class RecordingScreenHandlerListener final : public screen::ScreenHandlerListener {
public:
  void onSlotUpdate(screen::ScreenHandler&, int slot, const ItemStack& stack) override {
    slotUpdates.emplace_back(slot, stack);
  }
  void onContentsUpdate(screen::ScreenHandler&, const std::vector<ItemStack>& stacks) override {
    contents = stacks;
  }
  void onPropertyUpdate(screen::ScreenHandler&, int property, int value) override {
    propertyUpdates.emplace_back(property, value);
  }
  std::vector<std::pair<int, ItemStack>> slotUpdates;
  std::vector<std::pair<int, int>> propertyUpdates;
  std::vector<ItemStack> contents;
};
class TestScreenHandler final : public screen::ScreenHandler {
public:
  explicit TestScreenHandler(Inventory* inventory) {
    addSlot(new screen::slot::Slot(inventory, 0, 0, 0));
  }
};
} // namespace
TEST(ScreenHandlerParity, ListenerReceivesInitialContentsAndChangedSlots) {
  SimpleInventory inventory("test", 1);
  inventory.setStack(0, ItemStack(1, 2, 0));
  TestScreenHandler handler(&inventory);
  RecordingScreenHandlerListener listener;
  handler.addListener(&listener);
  ASSERT_EQ(listener.contents.size(), 1U);
  EXPECT_EQ(listener.contents[0].itemId, 1);
  inventory.setStack(0, ItemStack(2, 3, 0));
  handler.sendContentUpdates();
  ASSERT_FALSE(listener.slotUpdates.empty());
  EXPECT_EQ(listener.slotUpdates.back().first, 0);
  EXPECT_EQ(listener.slotUpdates.back().second.itemId, 2);
}
TEST(ScreenHandlerParity, FurnaceListenerReceivesPropertyChanges) {
  entity::player::PlayerEntity player;
  block::entity::FurnaceBlockEntity furnace;
  furnace.cookTime = 4;
  furnace.burnTime = 5;
  furnace.fuelTime = 6;
  screen::FurnaceScreenHandler handler(&player.inventory, &furnace);
  RecordingScreenHandlerListener listener;
  handler.addListener(&listener);
  ASSERT_GE(listener.propertyUpdates.size(), 3U);
  furnace.cookTime = 7;
  handler.sendContentUpdates();
  EXPECT_EQ(listener.propertyUpdates.back(), std::make_pair(0, 7));
}
TEST(PlayerManager, WhitelistAndOperatorState) {
  const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "minecraft_player_manager_test";
  std::filesystem::remove_all(tempRoot);
  std::filesystem::create_directories(tempRoot);
  CurrentPathGuard currentPathGuard(tempRoot);
  net::minecraft::server::MinecraftServer server;
  server.properties = std::make_unique<net::minecraft::server::ServerProperties>(tempRoot / "server.properties");
  server.playerManager.configureFromProperties();
  EXPECT_TRUE(server.playerManager.isWhitelisted("anyone"));
  server.playerManager.addToOperators("Admin");
  EXPECT_TRUE(server.playerManager.isOperator("admin"));
  EXPECT_TRUE(server.playerManager.isWhitelisted("stranger"));
  server.properties->setProperty("white-list", true);
  server.playerManager.configureFromProperties();
  EXPECT_FALSE(server.playerManager.isWhitelisted("stranger"));
  EXPECT_TRUE(server.playerManager.isWhitelisted("admin"));
  server.playerManager.addToWhitelist("guest");
  EXPECT_TRUE(server.playerManager.isWhitelisted("guest"));
  server.playerManager.banPlayer("banned");
  EXPECT_FALSE(server.playerManager.isWhitelisted("banned"));
  server.playerManager.unbanPlayer("banned");
  EXPECT_TRUE(server.playerManager.getPlayerList().empty());
}
} // namespace net::minecraft::test
