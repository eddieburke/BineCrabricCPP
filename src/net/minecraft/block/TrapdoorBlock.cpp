#include "net/minecraft/block/TrapdoorBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

TrapdoorBlock::TrapdoorBlock(int id, Material& mat) : Block(id, mat)
{
    textureId = 84;
    if (&mat == &material::Material::METAL) {
        ++textureId;
    }
    constexpr float f = 0.5f;
    setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 1.0f, 0.5f + f);
}

void TrapdoorBlock::applyBoundsForMeta(int meta)
{
    constexpr float thickness = 0.1875f;
    setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, thickness, 1.0f);
    if (!isOpen(meta)) {
        return;
    }
    if ((meta & 3) == 0) {
        setBoundingBox(0.0f, 0.0f, 1.0f - thickness, 1.0f, 1.0f, 1.0f);
    } else if ((meta & 3) == 1) {
        setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, thickness);
    } else if ((meta & 3) == 2) {
        setBoundingBox(1.0f - thickness, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    } else {
        setBoundingBox(0.0f, 0.0f, 0.0f, thickness, 1.0f, 1.0f);
    }
}

std::optional<net::minecraft::Box> TrapdoorBlock::getCollisionShape(World* world, int x, int y, int z) const
{
    if (world == nullptr) {
        return std::nullopt;
    }
    const int meta = world->getBlockMeta(x, y, z);
    const_cast<TrapdoorBlock*>(this)->applyBoundsForMeta(meta);
    return Block::getCollisionShape(world, x, y, z);
}

void TrapdoorBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
    applyBoundsForMeta(meta);
}

void TrapdoorBlock::setupRenderBoundingBox()
{
    constexpr float thickness = 0.1875f;
    setBoundingBox(0.0f, 0.5f - thickness / 2.0f, 0.0f, 1.0f, 0.5f + thickness / 2.0f, 1.0f);
}

std::optional<net::minecraft::HitResult> TrapdoorBlock::raycast(
    World* world, int x, int y, int z, net::minecraft::Vec3d startPos, net::minecraft::Vec3d endPos) const
{
    if (world == nullptr) {
        return std::nullopt;
    }
    const_cast<TrapdoorBlock*>(this)->updateBoundingBox(world, x, y, z);
    return Block::raycast(world, x, y, z, startPos, endPos);
}

bool TrapdoorBlock::canPlaceAt(World* world, int x, int y, int z, int side) const
{
    if (world == nullptr || side == 0 || side == 1) {
        return false;
    }
    int anchorX = x;
    int anchorZ = z;
    if (side == 2) {
        ++anchorZ;
    } else if (side == 3) {
        --anchorZ;
    } else if (side == 4) {
        ++anchorX;
    } else if (side == 5) {
        --anchorX;
    }
    return world->shouldSuffocate(anchorX, y, anchorZ);
}

void TrapdoorBlock::onBlockBreakStart(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player)
{
    onUse(world, x, y, z, player);
}

bool TrapdoorBlock::onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player)
{
    if (world == nullptr || &material == &material::Material::METAL) {
        return true;
    }
    const int meta = world->getBlockMeta(x, y, z);
    world->setBlockMeta(x, y, z, meta ^ 4);
    world->worldEvent(player, 1003, x, y, z, 0);
    return true;
}

void TrapdoorBlock::setOpen(World* world, int x, int y, int z, bool open)
{
    if (world == nullptr) {
        return;
    }
    const int meta = world->getBlockMeta(x, y, z);
    const bool currentlyOpen = (meta & 4) != 0;
    if (currentlyOpen == open) {
        return;
    }
    world->setBlockMeta(x, y, z, meta ^ 4);
    world->worldEvent(nullptr, 1003, x, y, z, 0);
}

void TrapdoorBlock::neighborUpdate(World* world, int x, int y, int z, int blockId)
{
    if (world == nullptr || world->isRemote()) {
        return;
    }
    const int meta = world->getBlockMeta(x, y, z);
    int anchorX = x;
    int anchorZ = z;
    if ((meta & 3) == 0) {
        ++anchorZ;
    } else if ((meta & 3) == 1) {
        --anchorZ;
    } else if ((meta & 3) == 2) {
        ++anchorX;
    } else {
        --anchorX;
    }
    if (!world->shouldSuffocate(anchorX, y, anchorZ)) {
        world->setBlock(x, y, z, 0);
        dropStacks(world, x, y, z, meta);
        return;
    }
    if (blockId > 0 && blockId < Block::BLOCK_COUNT && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr
        && Block::BLOCKS[static_cast<std::size_t>(blockId)]->canEmitRedstonePower()) {
        setOpen(world, x, y, z, world->isEmittingRedstonePower(x, y, z));
    }
}

void TrapdoorBlock::onPlaced(World* world, int x, int y, int z, int direction)
{
    if (world == nullptr) {
        return;
    }
    int facing = 0;
    if (direction == 2) {
        facing = 0;
    } else if (direction == 3) {
        facing = 1;
    } else if (direction == 4) {
        facing = 2;
    } else if (direction == 5) {
        facing = 3;
    }
    world->setBlockMeta(x, y, z, facing);
}

} // namespace net::minecraft::block
