#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/registry/ContentRegistries.hpp"
#include <gtest/gtest.h>
namespace net::minecraft::test {
TEST(RegistryBootstrap, VanillaCriticalEntriesAvailable) {
  net::minecraft::block::initializeBlocks();
  ASSERT_NE(net::minecraft::block::Block::GRASS_BLOCK, nullptr);
  EXPECT_NE(net::minecraft::entity::EntityRegistry::create("Zombie", nullptr), nullptr);
  EXPECT_NE(net::minecraft::registry::BlockEntityRegistry::instance().create("Chest"), nullptr);
}
} // namespace net::minecraft::test
