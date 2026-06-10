#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/StairsBlock.hpp"
#include "net/minecraft/block/Block.hpp"

#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

StairsBlock::StairsBlock(int id, Block& base)
    : Block(id, base.textureId, base.material)
{
    baseBlock = &base;
    setHardness(base.getHardness());
    setResistance(base.getResistance() / 3.0f);
    setSoundGroup(base.soundGroup);
    setOpacity(255);
}

void StairsBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    setBoundingBox(getRenderBounds(blockView, x, y, z));
}

net::minecraft::Box StairsBlock::getRenderBounds(const BlockView* /*blockView*/, int /*x*/, int /*y*/, int /*z*/) const
{
    return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
}

std::optional<net::minecraft::Box> StairsBlock::getCollisionShape(World* world, int x, int y, int z) const
{
    return Block::getCollisionShape(world, x, y, z);
}

bool StairsBlock::isSideVisible(const BlockView* blockView, int x, int y, int z, int side) const
{
    return Block::isSideVisible(blockView, x, y, z, side);
}

void StairsBlock::addIntersectingBoundingBox(
    World* world, int x, int y, int z, const net::minecraft::Box& box, std::vector<Box>& boxes) const
{
    if (world == nullptr) {
        return;
    }

    const int meta = world->getBlockMeta(x, y, z);
    auto* self = const_cast<StairsBlock*>(this);
    if (meta == 0) {
        self->setBoundingBox(0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 1.0f);
        Block::addIntersectingBoundingBox(world, x, y, z, box, boxes);
        self->setBoundingBox(0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        Block::addIntersectingBoundingBox(world, x, y, z, box, boxes);
    } else if (meta == 1) {
        self->setBoundingBox(0.0f, 0.0f, 0.0f, 0.5f, 1.0f, 1.0f);
        Block::addIntersectingBoundingBox(world, x, y, z, box, boxes);
        self->setBoundingBox(0.5f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
        Block::addIntersectingBoundingBox(world, x, y, z, box, boxes);
    } else if (meta == 2) {
        self->setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f);
        Block::addIntersectingBoundingBox(world, x, y, z, box, boxes);
        self->setBoundingBox(0.0f, 0.0f, 0.5f, 1.0f, 1.0f, 1.0f);
        Block::addIntersectingBoundingBox(world, x, y, z, box, boxes);
    } else if (meta == 3) {
        self->setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f);
        Block::addIntersectingBoundingBox(world, x, y, z, box, boxes);
        self->setBoundingBox(0.0f, 0.0f, 0.5f, 1.0f, 0.5f, 1.0f);
        Block::addIntersectingBoundingBox(world, x, y, z, box, boxes);
    }
    self->setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

void StairsBlock::randomDisplayTick(World* world, int x, int y, int z, JavaRandom& random)
{
    if (baseBlock != nullptr) {
        baseBlock->randomDisplayTick(world, x, y, z, random);
    }
}

void StairsBlock::onBlockBreakStart(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player)
{
    if (baseBlock != nullptr) {
        baseBlock->onBlockBreakStart(world, x, y, z, player);
    }
}

void StairsBlock::onMetadataChange(World* world, int x, int y, int z, int meta)
{
    if (baseBlock != nullptr) {
        baseBlock->onMetadataChange(world, x, y, z, meta);
    }
}

float StairsBlock::getLuminance(const BlockView* blockView, int x, int y, int z) const
{
    return baseBlock != nullptr ? baseBlock->getLuminance(blockView, x, y, z) : 0.0f;
}

float StairsBlock::getBlastResistance(net::minecraft::Entity* entity) const
{
    return baseBlock != nullptr ? baseBlock->getBlastResistance(entity) : getResistance();
}

int StairsBlock::getRenderLayer() const
{
    return baseBlock != nullptr ? baseBlock->getRenderLayer() : Block::getRenderLayer();
}

int StairsBlock::getDroppedItemId(int blockMeta, JavaRandom& random) const
{
    return baseBlock != nullptr ? baseBlock->getDroppedItemId(blockMeta, random) : id;
}

int StairsBlock::getDroppedItemCount(JavaRandom& random) const
{
    return baseBlock != nullptr ? baseBlock->getDroppedItemCount(random) : Block::getDroppedItemCount(random);
}

int StairsBlock::getTexture(int side, int meta) const
{
    return baseBlock != nullptr ? baseBlock->getTexture(side, meta) : Block::getTexture(side, meta);
}

int StairsBlock::getTexture(int side) const
{
    return baseBlock != nullptr ? baseBlock->getTexture(side) : Block::getTexture(side);
}

int StairsBlock::getTextureId(const BlockView* blockView, int x, int y, int z, int side) const
{
    return baseBlock != nullptr ? baseBlock->getTextureId(blockView, x, y, z, side) : Block::getTextureId(blockView, x, y, z, side);
}

int StairsBlock::getTickRate() const
{
    return baseBlock != nullptr ? baseBlock->getTickRate() : Block::getTickRate();
}

net::minecraft::Box StairsBlock::getBoundingBox(World* world, int x, int y, int z) const
{
    return baseBlock != nullptr ? baseBlock->getBoundingBox(world, x, y, z) : Block::getBoundingBox(world, x, y, z);
}

void StairsBlock::applyVelocity(
    World* world, int x, int y, int z, net::minecraft::Entity* entity, net::minecraft::Vec3d& velocity)
{
    if (baseBlock != nullptr) {
        baseBlock->applyVelocity(world, x, y, z, entity, velocity);
    }
}

bool StairsBlock::hasCollision() const
{
    return baseBlock != nullptr ? baseBlock->hasCollision() : Block::hasCollision();
}

bool StairsBlock::hasCollision(int meta, bool allowLiquids) const
{
    return baseBlock != nullptr ? baseBlock->hasCollision(meta, allowLiquids) : Block::hasCollision(meta, allowLiquids);
}

bool StairsBlock::canPlaceAt(World* world, int x, int y, int z) const
{
    return baseBlock != nullptr ? baseBlock->canPlaceAt(world, x, y, z) : Block::canPlaceAt(world, x, y, z);
}

void StairsBlock::onPlaced(World* world, int x, int y, int z)
{
    neighborUpdate(world, x, y, z, 0);
    if (baseBlock != nullptr) {
        baseBlock->onPlaced(world, x, y, z);
    }
}

void StairsBlock::onPlaced(World* world, int x, int y, int z, net::minecraft::PlayerEntity* placer)
{
    if (world == nullptr || placer == nullptr) {
        return;
    }
    const int direction = MathHelper::floor(static_cast<double>(placer->yaw * 4.0f / 360.0f) + 0.5) & 3;
    if (direction == 0) {
        world->setBlockMeta(x, y, z, 2);
    }
    if (direction == 1) {
        world->setBlockMeta(x, y, z, 1);
    }
    if (direction == 2) {
        world->setBlockMeta(x, y, z, 3);
    }
    if (direction == 3) {
        world->setBlockMeta(x, y, z, 0);
    }
}

void StairsBlock::onBreak(World* world, int x, int y, int z)
{
    if (baseBlock != nullptr) {
        baseBlock->onBreak(world, x, y, z);
    }
}

void StairsBlock::dropStacks(World* world, int x, int y, int z, int meta, float luck)
{
    if (baseBlock != nullptr) {
        baseBlock->dropStacks(world, x, y, z, meta, luck);
    } else {
        Block::dropStacks(world, x, y, z, meta, luck);
    }
}

void StairsBlock::onSteppedOn(World* world, int x, int y, int z, net::minecraft::Entity* entity)
{
    if (baseBlock != nullptr) {
        baseBlock->onSteppedOn(world, x, y, z, entity);
    }
}

void StairsBlock::onTick(World* world, int x, int y, int z, JavaRandom& random)
{
    if (baseBlock != nullptr) {
        baseBlock->onTick(world, x, y, z, random);
    }
}

bool StairsBlock::onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player)
{
    return baseBlock != nullptr ? baseBlock->onUse(world, x, y, z, player) : Block::onUse(world, x, y, z, player);
}

void StairsBlock::onDestroyedByExplosion(World* world, int x, int y, int z)
{
    if (baseBlock != nullptr) {
        baseBlock->onDestroyedByExplosion(world, x, y, z);
    }
}
namespace {

void registerStairsBlocks()
{
    Block::WOODEN_STAIRS = (new StairsBlock(53, *Block::BLOCKS[5]))->setTranslationKey("stairsWood")->ignoreMetaUpdates();
    Block::COBBLESTONE_STAIRS = (new StairsBlock(67, *Block::BLOCKS[4]))->setTranslationKey("stairsStone")->ignoreMetaUpdates();
}

MINECRAFT_REGISTER_BLOCK(registerStairsBlocks, 67);

} // namespace
} // namespace net::minecraft::block

