#include <gtest/gtest.h>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/mod/lua/LuaBlockRegistry.hpp"
#include "net/minecraft/mod/lua/LuaItemRegistry.hpp"
#include "net/minecraft/registry/ContentRegistries.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::test {
namespace {
bool gDynamicPhaseCallbackRan = false;
void runDynamicPhaseCallback() {
 gDynamicPhaseCallbackRan = true;
}
void enqueueDynamicPhaseCallback() {
 registry::Registry::enqueue(mod::LifecyclePhase::Init, 75000, runDynamicPhaseCallback);
}
struct DynamicPhaseRegistration {
 DynamicPhaseRegistration() {
  registry::Registry::enqueue(mod::LifecyclePhase::Init, 50000, enqueueDynamicPhaseCallback);
 }
};
DynamicPhaseRegistration gDynamicPhaseRegistration;
} // namespace
TEST(RegistryBootstrap, VanillaCriticalEntriesAvailable) {
 ASSERT_NE(net::minecraft::block::Block::GRASS_BLOCK, nullptr);
 EXPECT_NE(net::minecraft::entity::EntityRegistry::create("Zombie", nullptr), nullptr);
 EXPECT_NE(net::minecraft::registry::BlockEntityRegistry::instance().create("Chest"), nullptr);
}
TEST(RegistryBootstrap, RunsCallbacksEnqueuedDuringCurrentPhase) {
 EXPECT_TRUE(gDynamicPhaseCallbackRan);
}
// Content registration is deliberately NOT gated on the lifecycle phase. Mods register
// after Init all the time — that is what live enable/disable and the Reload List depend
// on, and registrations persist for the session by design (see the duplicate-id branches
// in registerBlockSpec / registerItemSpec). This used to assert the opposite and expect
// both calls to be refused; no such gate exists in either function, and adding one would
// break live mod reloading.
TEST(RegistryBootstrap, AcceptsContentRegistrationAfterInit) {
 mod::lua::BlockRegistrationSpec blockSpec;
 blockSpec.blockId = 250;
 mod::lua::ItemRegistrationSpec itemSpec;
 itemSpec.itemId = 31000;
 itemSpec.texturePath = "mods/test/item.png";
 std::string error;
 EXPECT_TRUE(mod::lua::registerBlockSpec(blockSpec, error)) << error;
 error.clear();
 EXPECT_TRUE(mod::lua::registerItemSpec(itemSpec, error)) << error;
}
// The guards that do exist are on the spec itself, not on when it arrives.
TEST(RegistryBootstrap, RejectsMalformedContentRegistration) {
 std::string error;
 mod::lua::BlockRegistrationSpec outOfRangeBlock;
 outOfRangeBlock.blockId = 0;
 EXPECT_FALSE(mod::lua::registerBlockSpec(outOfRangeBlock, error));
 EXPECT_FALSE(error.empty());
 error.clear();
 // Item ids below 256 belong to blocks.
 mod::lua::ItemRegistrationSpec outOfRangeItem;
 outOfRangeItem.itemId = 12;
 outOfRangeItem.texturePath = "mods/test/item.png";
 EXPECT_FALSE(mod::lua::registerItemSpec(outOfRangeItem, error));
 EXPECT_FALSE(error.empty());
 error.clear();
 mod::lua::ItemRegistrationSpec texturelessItem;
 texturelessItem.itemId = 31001;
 EXPECT_FALSE(mod::lua::registerItemSpec(texturelessItem, error));
 EXPECT_FALSE(error.empty());
}
} // namespace net::minecraft::test
