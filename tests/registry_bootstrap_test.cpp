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
  net::minecraft::block::initializeBlocks();
  ASSERT_NE(net::minecraft::block::Block::GRASS_BLOCK, nullptr);
  EXPECT_NE(net::minecraft::entity::EntityRegistry::create("Zombie", nullptr), nullptr);
  EXPECT_NE(net::minecraft::registry::BlockEntityRegistry::instance().create("Chest"), nullptr);
}
TEST(RegistryBootstrap, RunsCallbacksEnqueuedDuringCurrentPhase) {
  net::minecraft::block::initializeBlocks();
  EXPECT_TRUE(gDynamicPhaseCallbackRan);
}
TEST(RegistryBootstrap, RejectsContentRegistrationAfterInit) {
  net::minecraft::block::initializeBlocks();
  mod::lua::BlockRegistrationSpec blockSpec;
  blockSpec.blockId = 250;
  blockSpec.terrainTextureId = 1;
  mod::lua::ItemRegistrationSpec itemSpec;
  itemSpec.itemId = 31000;
  itemSpec.itemsTextureId = 1;
  std::string error;
  EXPECT_FALSE(mod::lua::registerBlockSpec(blockSpec, error));
  error.clear();
  EXPECT_FALSE(mod::lua::registerItemSpec(itemSpec, error));
}
} // namespace net::minecraft::test
