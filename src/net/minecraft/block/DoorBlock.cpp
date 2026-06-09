#include "net/minecraft/block/DoorBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

DoorBlock::DoorBlock(int id, Material& mat) : Block(id, mat)
{
    textureId = 97;
    if (&mat == &material::Material::METAL) {
        ++textureId;
    }
    constexpr float f = 0.5f;
    setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 1.0f, 0.5f + f);
}

int DoorBlock::facingFromMeta(int meta)
{
    if ((meta & 4) == 0) {
        return (meta - 1) & 3;
    }
    return meta & 3;
}

int DoorBlock::getTexture(int side, int meta) const
{
    if (side == 0 || side == 1) {
        return textureId;
    }
    const int facing = facingFromMeta(meta);
    if ((facing == 0 || facing == 2) ^ (side <= 3)) {
        return textureId;
    }
    int variant = facing / 2 + ((side & 1) ^ facing);
    int tex = textureId - (meta & 8) * 2;
    variant += (meta & 4) / 4;
    if ((variant & 1) != 0) {
        tex = -tex;
    }
    return tex;
}

int DoorBlock::getState(const BlockView* blockView, int x, int y, int z)
{
    if (blockView == nullptr) {
        return 0;
    }
    const int meta = blockView->getBlockMeta(x, y, z);
    const bool topHalf = (meta & 8) != 0;
    const int lower = topHalf ? blockView->getBlockMeta(x, y - 1, z) : meta;
    const int upper = topHalf ? meta : blockView->getBlockMeta(x, y + 1, z);
    const bool flipped = (upper & 1) != 0;
    return (lower & 7) | (topHalf ? 8 : 0) | (flipped ? 16 : 0);
}

int DoorBlock::getState(const World* world, int x, int y, int z)
{
    return getState(static_cast<const BlockView*>(world), x, y, z);
}

std::optional<net::minecraft::Box> DoorBlock::collisionShapeForState(int state, int x, int y, int z)
{
    constexpr double thickness = 0.1875;
    const bool open = (state & 4) != 0;
    const bool flipped = (state & 16) != 0;
    const int facing = facingFromMeta(state);

    if (!open) {
        switch (facing) {
        case 0:
            return net::minecraft::Box {static_cast<double>(x), static_cast<double>(y), static_cast<double>(z),
                static_cast<double>(x + 1), static_cast<double>(y + 1), static_cast<double>(z) + thickness};
        case 1:
            return net::minecraft::Box {static_cast<double>(x + 1) - thickness, static_cast<double>(y), static_cast<double>(z),
                static_cast<double>(x + 1), static_cast<double>(y + 1), static_cast<double>(z + 1)};
        case 2:
            return net::minecraft::Box {static_cast<double>(x), static_cast<double>(y), static_cast<double>(z + 1) - thickness,
                static_cast<double>(x + 1), static_cast<double>(y + 1), static_cast<double>(z + 1)};
        default:
            return net::minecraft::Box {static_cast<double>(x), static_cast<double>(y), static_cast<double>(z),
                static_cast<double>(x) + thickness, static_cast<double>(y + 1), static_cast<double>(z + 1)};
        }
    }

    switch (facing) {
    case 0:
        if (flipped) {
            return net::minecraft::Box {static_cast<double>(x), static_cast<double>(y), static_cast<double>(z),
                static_cast<double>(x) + thickness, static_cast<double>(y + 1), static_cast<double>(z + 1)};
        }
        return net::minecraft::Box {static_cast<double>(x + 1) - thickness, static_cast<double>(y), static_cast<double>(z),
            static_cast<double>(x + 1), static_cast<double>(y + 1), static_cast<double>(z + 1)};
    case 1:
        if (flipped) {
            return net::minecraft::Box {static_cast<double>(x), static_cast<double>(y), static_cast<double>(z + 1) - thickness,
                static_cast<double>(x + 1), static_cast<double>(y + 1), static_cast<double>(z + 1)};
        }
        return net::minecraft::Box {static_cast<double>(x), static_cast<double>(y), static_cast<double>(z),
            static_cast<double>(x + 1), static_cast<double>(y + 1), static_cast<double>(z) + thickness};
    case 2:
        if (flipped) {
            return net::minecraft::Box {static_cast<double>(x + 1) - thickness, static_cast<double>(y), static_cast<double>(z),
                static_cast<double>(x + 1), static_cast<double>(y + 1), static_cast<double>(z + 1)};
        }
        return net::minecraft::Box {static_cast<double>(x), static_cast<double>(y), static_cast<double>(z),
            static_cast<double>(x) + thickness, static_cast<double>(y + 1), static_cast<double>(z + 1)};
    default:
        if (flipped) {
            return net::minecraft::Box {static_cast<double>(x), static_cast<double>(y), static_cast<double>(z),
                static_cast<double>(x + 1), static_cast<double>(y + 1), static_cast<double>(z) + thickness};
        }
        return net::minecraft::Box {static_cast<double>(x), static_cast<double>(y), static_cast<double>(z + 1) - thickness,
            static_cast<double>(x + 1), static_cast<double>(y + 1), static_cast<double>(z + 1)};
    }
}

void DoorBlock::applyBoundsForState(int state)
{
    constexpr float thickness = 0.1875f;
    const bool open = (state & 4) != 0;
    const bool flipped = (state & 16) != 0;
    const int facing = facingFromMeta(state);

    if (!open) {
        if (facing == 0) {
            setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, thickness);
        } else if (facing == 1) {
            setBoundingBox(1.0f - thickness, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        } else if (facing == 2) {
            setBoundingBox(0.0f, 0.0f, 1.0f - thickness, 1.0f, 1.0f, 1.0f);
        } else {
            setBoundingBox(0.0f, 0.0f, 0.0f, thickness, 1.0f, 1.0f);
        }
        return;
    }

    if (facing == 0) {
        if (flipped) {
            setBoundingBox(0.0f, 0.0f, 0.0f, thickness, 1.0f, 1.0f);
        } else {
            setBoundingBox(1.0f - thickness, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        }
    } else if (facing == 1) {
        if (flipped) {
            setBoundingBox(0.0f, 0.0f, 1.0f - thickness, 1.0f, 1.0f, 1.0f);
        } else {
            setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, thickness);
        }
    } else if (facing == 2) {
        if (flipped) {
            setBoundingBox(1.0f - thickness, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        } else {
            setBoundingBox(0.0f, 0.0f, 0.0f, thickness, 1.0f, 1.0f);
        }
    } else if (flipped) {
        setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, thickness);
    } else {
        setBoundingBox(0.0f, 0.0f, 1.0f - thickness, 1.0f, 1.0f, 1.0f);
    }
}

std::optional<net::minecraft::Box> DoorBlock::getCollisionShape(World* world, int x, int y, int z) const
{
    return collisionShapeForState(getState(world, x, y, z), x, y, z);
}

void DoorBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    applyBoundsForState(getState(blockView, x, y, z));
}

bool DoorBlock::canPlaceAt(World* world, int x, int y, int z) const
{
    if (world == nullptr || y >= 127) {
        return false;
    }
    return world->shouldSuffocate(x, y - 1, z) && Block::canPlaceAt(world, x, y, z)
        && Block::canPlaceAt(world, x, y + 1, z);
}

int DoorBlock::getDroppedItemId(int blockMeta, JavaRandom& /*random*/) const
{
    if ((blockMeta & 8) != 0) {
        return 0;
    }
    if (&material == &material::Material::METAL && Item::IRON_DOOR != nullptr) {
        return Item::IRON_DOOR->id;
    }
    return Item::WOODEN_DOOR != nullptr ? Item::WOODEN_DOOR->id : 324;
}

void DoorBlock::onBlockBreakStart(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player)
{
    onUse(world, x, y, z, player);
}

bool DoorBlock::onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player)
{
    if (world == nullptr || &material == &material::Material::METAL) {
        return true;
    }
    int meta = world->getBlockMeta(x, y, z);
    if ((meta & 8) != 0) {
        if (world->getBlockId(x, y - 1, z) == id) {
            onUse(world, x, y - 1, z, player);
        }
        return true;
    }
    if (world->getBlockId(x, y + 1, z) == id) {
        world->setBlockMeta(x, y + 1, z, (meta ^ 4) + 8);
    }
    world->setBlockMeta(x, y, z, meta ^ 4);
    world->setBlocksDirty(x, y - 1, z, x, y, z);
    world->worldEvent(player, 1003, x, y, z, 0);
    return true;
}

void DoorBlock::setOpen(World* world, int x, int y, int z, bool open)
{
    if (world == nullptr) {
        return;
    }
    int meta = world->getBlockMeta(x, y, z);
    if ((meta & 8) != 0) {
        if (world->getBlockId(x, y - 1, z) == id) {
            setOpen(world, x, y - 1, z, open);
        }
        return;
    }
    const bool currentlyOpen = (meta & 4) != 0;
    if (currentlyOpen == open) {
        return;
    }
    if (world->getBlockId(x, y + 1, z) == id) {
        world->setBlockMeta(x, y + 1, z, (meta ^ 4) + 8);
    }
    world->setBlockMeta(x, y, z, meta ^ 4);
    world->setBlocksDirty(x, y - 1, z, x, y, z);
    world->worldEvent(nullptr, 1003, x, y, z, 0);
}

std::optional<net::minecraft::HitResult> DoorBlock::raycast(
    World* world, int x, int y, int z, net::minecraft::Vec3d startPos, net::minecraft::Vec3d endPos) const
{
    if (world == nullptr) {
        return std::nullopt;
    }
    const_cast<DoorBlock*>(this)->updateBoundingBox(world, x, y, z);
    return Block::raycast(world, x, y, z, startPos, endPos);
}

void DoorBlock::neighborUpdate(World* world, int x, int y, int z, int blockId)
{
    if (world == nullptr) {
        return;
    }
    const int meta = world->getBlockMeta(x, y, z);
    if ((meta & 8) != 0) {
        if (world->getBlockId(x, y - 1, z) != id) {
            world->setBlock(x, y, z, 0);
        } else if (blockId > 0 && blockId < Block::BLOCK_COUNT && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr
            && Block::BLOCKS[static_cast<std::size_t>(blockId)]->canEmitRedstonePower()) {
            neighborUpdate(world, x, y - 1, z, blockId);
        }
        return;
    }

    bool removed = false;
    if (world->getBlockId(x, y + 1, z) != id) {
        world->setBlock(x, y, z, 0);
        removed = true;
    }
    if (!world->shouldSuffocate(x, y - 1, z)) {
        world->setBlock(x, y, z, 0);
        removed = true;
        if (world->getBlockId(x, y + 1, z) == id) {
            world->setBlock(x, y + 1, z, 0);
        }
    }
    if (removed) {
        if (!world->isRemote()) {
            dropStacks(world, x, y, z, meta);
        }
        return;
    }
    if (blockId > 0 && blockId < Block::BLOCK_COUNT && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr
        && Block::BLOCKS[static_cast<std::size_t>(blockId)]->canEmitRedstonePower()) {
        const bool powered = world->isEmittingRedstonePower(x, y, z) || world->isEmittingRedstonePower(x, y + 1, z);
        setOpen(world, x, y, z, powered);
    }
}

} // namespace net::minecraft::block
