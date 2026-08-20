#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/item/tool/wooden_axe.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/server/network/ServerPlayerInteractionManager.hpp"
#include "tests/support/server_event_fixture.hpp"
namespace {
namespace fs = std::filesystem;
class TreeFellerRuntime {
 public:
 TreeFellerRuntime() {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  root_ = fs::temp_directory_path() / ("minecraft_tree_feller_test_" + std::to_string(stamp));
  const fs::path target = root_ / "mods" / "tree_feller";
  fs::create_directories(target / "scripts");
  const fs::path source = fs::path(MINECRAFT_TEST_SOURCE_DIR) / "mods" / "tree_feller";
  fs::copy_file(source / "mod.json", target / "mod.json", fs::copy_options::overwrite_existing);
  fs::copy_file(
      source / "scripts" / "main.lua", target / "scripts" / "main.lua", fs::copy_options::overwrite_existing);
  auto& host = net::minecraft::mod::runtime::host();
  host.shutdown();
  host.setRuntimeSide(net::minecraft::mod::runtime::ModRuntimeSide::Server);
  host.setPackageLoadingEnabled(true);
  host.initialize(root_);
 }
 ~TreeFellerRuntime() {
  net::minecraft::mod::runtime::host().shutdown();
  std::error_code error;
  fs::remove_all(root_, error);
 }

 private:
 fs::path root_;
};
struct TreeFellerWorld {
 ServerEventFixture fixture;
 net::minecraft::entity::player::PlayerEntity player{&fixture.world};
 net::minecraft::server::network::ServerPlayerInteractionManager interaction{&fixture.world};
 TreeFellerWorld() {
  interaction.player = &player;
  player.inventory.selectedSlot = 0;
 }
 void equipWoodenAxe(int damage) {
  player.inventory.main[0] = net::minecraft::ItemStack(
      net::minecraft::Item::byRawId(net::minecraft::item::WoodenAxeItem::kRawId), 1, damage);
 }
 void placeLogs(int count) {
  for(int index = 0; index < count; ++index) {
   ASSERT_TRUE(fixture.world.setBlock(8, 64 + index, 8, net::minecraft::Block::LOG->id));
  }
 }
};
TEST(TreeFellerMod, DamagesAxeOncePerLog) {
 if(!net::minecraft::mod::lua::luaApi().ready()) {
  GTEST_SKIP() << "Lua runtime unavailable";
 }
 TreeFellerRuntime runtime;
 TreeFellerWorld world;
 world.equipWoodenAxe(0);
 world.placeLogs(5);
 ASSERT_TRUE(world.interaction.tryBreakBlock(8, 64, 8));
 for(int index = 0; index < 5; ++index) {
  EXPECT_EQ(world.fixture.world.getBlockId(8, 64 + index, 8), 0);
 }
 const net::minecraft::ItemStack* axe = world.player.inventory.getSelectedItem();
 ASSERT_NE(axe, nullptr);
 EXPECT_EQ(axe->damage, 5);
}
TEST(TreeFellerMod, StopsWhenAxeBreaks) {
 if(!net::minecraft::mod::lua::luaApi().ready()) {
  GTEST_SKIP() << "Lua runtime unavailable";
 }
 TreeFellerRuntime runtime;
 TreeFellerWorld world;
 const net::minecraft::ItemStack probe(
     net::minecraft::Item::byRawId(net::minecraft::item::WoodenAxeItem::kRawId), 1, 0);
 world.equipWoodenAxe(probe.getMaxDamage() - 1);
 world.placeLogs(4);
 ASSERT_TRUE(world.interaction.tryBreakBlock(8, 64, 8));
 EXPECT_EQ(world.fixture.world.getBlockId(8, 64, 8), 0);
 EXPECT_EQ(world.fixture.world.getBlockId(8, 65, 8), 0);
 EXPECT_EQ(world.fixture.world.getBlockId(8, 66, 8), net::minecraft::Block::LOG->id);
 EXPECT_EQ(world.fixture.world.getBlockId(8, 67, 8), net::minecraft::Block::LOG->id);
 EXPECT_EQ(world.player.inventory.getSelectedItem(), nullptr);
}
} // namespace
