#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/Entity.hpp"
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
