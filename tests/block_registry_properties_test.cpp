#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/ai/pathing/PathNodeNavigator.hpp"
#include "net/minecraft/entity/passive/SheepEntity.hpp"
#include "net/minecraft/world/World.hpp"

#include <cstddef>

namespace {

int opacity(int id)
{
    return net::minecraft::block::Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(id)];
}

bool opaque(int id)
{
    return net::minecraft::block::Block::BLOCKS_OPAQUE[static_cast<std::size_t>(id)];
}

bool allowsVision(int id)
{
    return net::minecraft::block::Block::BLOCKS_ALLOW_VISION[static_cast<std::size_t>(id)];
}

} // namespace

TEST_CASE("block registry finalization uses derived opacity defaults")
{
    using net::minecraft::block::Block;

    net::minecraft::block::initializeBlocks();

    REQUIRE(Block::GRASS != nullptr);
    REQUIRE(Block::DEAD_BUSH != nullptr);
    REQUIRE(Block::TORCH != nullptr);
    REQUIRE(Block::LEAVES != nullptr);
    REQUIRE(Block::FLOWING_WATER != nullptr);
    REQUIRE(Block::SLAB != nullptr);

    CHECK_FALSE(Block::GRASS->isOpaque());
    CHECK_FALSE(opaque(Block::GRASS->id));
    CHECK(opacity(Block::GRASS->id) == 0);
    CHECK(allowsVision(Block::GRASS->id));

    CHECK_FALSE(Block::DEAD_BUSH->isOpaque());
    CHECK_FALSE(opaque(Block::DEAD_BUSH->id));
    CHECK(opacity(Block::DEAD_BUSH->id) == 0);
    CHECK(allowsVision(Block::DEAD_BUSH->id));

    CHECK_FALSE(Block::TORCH->isOpaque());
    CHECK_FALSE(opaque(Block::TORCH->id));
    CHECK(opacity(Block::TORCH->id) == 0);

    CHECK(opacity(Block::LEAVES->id) == 1);
    CHECK(opacity(Block::FLOWING_WATER->id) == 3);
    CHECK(opacity(Block::SLAB->id) == 255);
}

TEST_CASE("sneaking edge clamp is not undone by step-up")
{
    using net::minecraft::World;
    using net::minecraft::block::Block;
    using net::minecraft::entity::Entity;

    net::minecraft::block::initializeBlocks();
    REQUIRE(Block::STONE != nullptr);

    World world("movement-test", 1);
    for (int x = -2; x <= 4; ++x) {
        for (int y = 0; y <= 1; ++y) {
            for (int z = -2; z <= 2; ++z) {
                world.setBlockWithoutNotifyingNeighbors(x, y, z, 0);
            }
        }
    }
    REQUIRE(world.setBlockWithoutNotifyingNeighbors(0, 0, 0, Block::STONE->id));

    Entity entity(&world);
    entity.setBoundingBoxSpacing(0.6f, 1.8f);
    entity.stepHeight = 0.5f;
    entity.setPosition(1.29, 1.0, 0.5);
    entity.onGround = true;
    entity.setSneaking(true);

    const double beforeX = entity.x;
    entity.move(0.3, 0.0, 0.0);

    CHECK(entity.x == doctest::Approx(beforeX));
}

TEST_CASE("world mob spawn hook initializes sheep color through LivingEntity")
{
    using net::minecraft::JavaRandom;
    using net::minecraft::World;
    using net::minecraft::entity::passive::SheepEntity;

    net::minecraft::block::initializeBlocks();

    World world("sheep-spawn-test", 1);
    (void)world.getChunk(0, 0);

    JavaRandom expectedRandom = world.random();
    const int expectedColor = SheepEntity::generateDefaultColor(expectedRandom);

    auto* spawned = dynamic_cast<SheepEntity*>(world.spawnMob("Sheep", 0.5, 64.0, 0.5));
    REQUIRE(spawned != nullptr);
    CHECK(spawned->getColor() == expectedColor);
}

TEST_CASE("client world construction uses a valid multiplayer save handler")
{
    using net::minecraft::ClientWorld;

    ClientWorld world(nullptr, 123456789ULL, 0);

    CHECK(world.isRemote());
    CHECK(world.getChunkSource() != nullptr);
    CHECK(world.getDimensionData() != nullptr);
    CHECK(world.getDimensionData()->getWorldPropertiesFile("level.dat").empty());
    CHECK(world.getSeed() == 123456789ULL);
    CHECK(world.getSpawnPos().x == 8);
    CHECK(world.getSpawnPos().y == 64);
    CHECK(world.getSpawnPos().z == 8);
}

TEST_CASE("path navigator returns stable results across repeated searches")
{
    using net::minecraft::World;
    using net::minecraft::block::Block;
    using net::minecraft::entity::Entity;
    using net::minecraft::entity::ai::pathing::PathNodeNavigator;

    net::minecraft::block::initializeBlocks();
    REQUIRE(Block::STONE != nullptr);

    World world("pathfinding-test", 1);
    (void)world.getChunk(0, 0);
    for (int x = -1; x <= 5; ++x) {
        for (int y = 0; y <= 2; ++y) {
            for (int z = -1; z <= 1; ++z) {
                const int blockId = y == 0 ? Block::STONE->id : 0;
                REQUIRE(world.setBlockWithoutNotifyingNeighbors(x, y, z, blockId));
            }
        }
    }

    Entity entity(&world);
    entity.setBoundingBoxSpacing(0.6f, 1.8f);
    entity.setPosition(0.5, 1.0, 0.5);

    PathNodeNavigator navigator(&world);
    auto firstPath = navigator.findPath(&entity, 3, 1, 0, 16.0f);
    REQUIRE(firstPath.length == 4);
    const auto* firstEnd = firstPath.getEnd();
    REQUIRE(firstEnd != nullptr);
    CHECK(firstEnd->x == 3);
    CHECK(firstEnd->y == 1);
    CHECK(firstEnd->z == 0);

    auto secondPath = navigator.findPath(&entity, 3, 1, 0, 16.0f);
    REQUIRE(secondPath.length == firstPath.length);
    const auto* secondEnd = secondPath.getEnd();
    REQUIRE(secondEnd != nullptr);
    CHECK(secondEnd->x == firstEnd->x);
    CHECK(secondEnd->y == firstEnd->y);
    CHECK(secondEnd->z == firstEnd->z);
}
