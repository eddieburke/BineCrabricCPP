#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/PistonHeadBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/PistonBlock.hpp"
#include "net/minecraft/block/PistonConstants.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

PistonHeadBlock::PistonHeadBlock(int blockId, int textureId) : Block(blockId, textureId, material::Material::PISTON)
{
    setHardness(0.5f);
}

int PistonHeadBlock::getTexture(int side, int meta) const
{
    const int facing = getFacing(meta);
    if (side == facing) {
        if ((meta & 8) != 0) {
            return textureId - 1;
        }
        return textureId;
    }
    if (side == PistonConstants::TEXTURE_SIDES[static_cast<std::size_t>(facing)]) {
        return PistonConstants::TEXTURE_TOP;
    }
    return PistonConstants::TEXTURE_EXTENSION;
}

bool PistonHeadBlock::canPlaceAt(World* /*world*/, int /*x*/, int /*y*/, int /*z*/) const
{
    return false;
}

bool PistonHeadBlock::canPlaceAt(World* /*world*/, int /*x*/, int /*y*/, int /*z*/, int /*side*/) const
{
    return false;
}

void PistonHeadBlock::onBreak(World* world, int x, int y, int z)
{
    const int headMeta = world != nullptr ? world->getBlockMeta(x, y, z) : 0;
    Block::onBreak(world, x, y, z);
    if (world == nullptr || Block::PISTON == nullptr || Block::STICKY_PISTON == nullptr) {
        return;
    }
    const int offsetIndex = PistonConstants::TEXTURE_SIDES[static_cast<std::size_t>(getFacing(headMeta))];
    x += PistonConstants::HEAD_OFFSET_X[static_cast<std::size_t>(offsetIndex)];
    y += PistonConstants::HEAD_OFFSET_Y[static_cast<std::size_t>(offsetIndex)];
    z += PistonConstants::HEAD_OFFSET_Z[static_cast<std::size_t>(offsetIndex)];
    const int pistonId = world->getBlockId(x, y, z);
    if ((pistonId == Block::PISTON->id || pistonId == Block::STICKY_PISTON->id)
        && PistonBlock::isExtended(world->getBlockMeta(x, y, z))) {
        Block::BLOCKS[static_cast<std::size_t>(pistonId)]->dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
        world->setBlock(x, y, z, 0);
    }
}

namespace {

void addBoxIfIntersecting(
    const net::minecraft::Box& query, double minX, double minY, double minZ, double maxX, double maxY, double maxZ,
    std::vector<net::minecraft::Box>& boxes)
{
    const net::minecraft::Box candidate(minX, minY, minZ, maxX, maxY, maxZ);
    if (query.intersects(candidate)) {
        boxes.push_back(candidate);
    }
}

} // namespace

void PistonHeadBlock::addIntersectingBoundingBox(
    World* world, int x, int y, int z, const net::minecraft::Box& box, std::vector<Box>& boxes) const
{
    if (world == nullptr) {
        return;
    }
    const double bx = static_cast<double>(x);
    const double by = static_cast<double>(y);
    const double bz = static_cast<double>(z);
    switch (getFacing(world->getBlockMeta(x, y, z))) {
    case 0:
        addBoxIfIntersecting(box, bx, by, bz, bx + 1.0, by + 0.25, bz + 1.0, boxes);
        addBoxIfIntersecting(box, bx + 0.375, by + 0.25, bz + 0.375, bx + 0.625, by + 1.0, bz + 0.625, boxes);
        break;
    case 1:
        addBoxIfIntersecting(box, bx, by + 0.75, bz, bx + 1.0, by + 1.0, bz + 1.0, boxes);
        addBoxIfIntersecting(box, bx + 0.375, by, bz + 0.375, bx + 0.625, by + 0.75, bz + 0.625, boxes);
        break;
    case 2:
        addBoxIfIntersecting(box, bx, by, bz, bx + 1.0, by + 1.0, bz + 0.25, boxes);
        addBoxIfIntersecting(box, bx + 0.25, by + 0.375, bz + 0.25, bx + 0.75, by + 0.625, bz + 1.0, boxes);
        break;
    case 3:
        addBoxIfIntersecting(box, bx, by, bz + 0.75, bx + 1.0, by + 1.0, bz + 1.0, boxes);
        addBoxIfIntersecting(box, bx + 0.25, by + 0.375, bz, bx + 0.75, by + 0.625, bz + 0.75, boxes);
        break;
    case 4:
        addBoxIfIntersecting(box, bx, by, bz, bx + 0.25, by + 1.0, bz + 1.0, boxes);
        addBoxIfIntersecting(box, bx + 0.375, by + 0.25, bz + 0.25, bx + 0.625, by + 0.75, bz + 1.0, boxes);
        break;
    case 5:
        addBoxIfIntersecting(box, bx + 0.75, by, bz, bx + 1.0, by + 1.0, bz + 1.0, boxes);
        addBoxIfIntersecting(box, bx, by + 0.375, bz + 0.25, bx + 0.75, by + 0.625, bz + 0.75, boxes);
        break;
    default:
        break;
    }
}

void PistonHeadBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    setBoundingBox(getRenderBounds(blockView, x, y, z));
}

net::minecraft::Box PistonHeadBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const
{
    if (blockView == nullptr) {
        return {minX, minY, minZ, maxX, maxY, maxZ};
    }
    switch (getFacing(blockView->getBlockMeta(x, y, z))) {
    case 0:
        return {0.0f, 0.0f, 0.0f, 1.0f, 0.25f, 1.0f};
    case 1:
        return {0.0f, 0.75f, 0.0f, 1.0f, 1.0f, 1.0f};
    case 2:
        return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.25f};
    case 3:
        return {0.0f, 0.0f, 0.75f, 1.0f, 1.0f, 1.0f};
    case 4:
        return {0.0f, 0.0f, 0.0f, 0.25f, 1.0f, 1.0f};
    case 5:
        return {0.75f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    default:
        return {minX, minY, minZ, maxX, maxY, maxZ};
    }
}

void PistonHeadBlock::neighborUpdate(World* world, int x, int y, int z, int neighborId)
{
    if (world == nullptr || Block::PISTON == nullptr || Block::STICKY_PISTON == nullptr) {
        return;
    }
    const int facing = getFacing(world->getBlockMeta(x, y, z));
    const int pistonX = x - PistonConstants::HEAD_OFFSET_X[static_cast<std::size_t>(facing)];
    const int pistonY = y - PistonConstants::HEAD_OFFSET_Y[static_cast<std::size_t>(facing)];
    const int pistonZ = z - PistonConstants::HEAD_OFFSET_Z[static_cast<std::size_t>(facing)];
    const int pistonId = world->getBlockId(pistonX, pistonY, pistonZ);
    if (pistonId != Block::PISTON->id && pistonId != Block::STICKY_PISTON->id) {
        world->setBlock(x, y, z, 0);
        return;
    }
    Block::BLOCKS[static_cast<std::size_t>(pistonId)]->neighborUpdate(world, pistonX, pistonY, pistonZ, neighborId);
}
void PistonHeadBlock::registerClass()
{
    Block::PISTON_HEAD = (new PistonHeadBlock(34, 107))->ignoreMetaUpdates();
}

namespace {

static ::net::minecraft::registry::RegisterBlock<PistonHeadBlock> autoReg(34);
} // namespace
} // namespace net::minecraft::block

