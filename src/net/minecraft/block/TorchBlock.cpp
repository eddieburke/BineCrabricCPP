#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/TorchBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

TorchBlock::TorchBlock(int id, int textureId) : Block(id, textureId, material::Material::PISTON_BREAKABLE)
{
    setTickRandomly(true);
}

bool TorchBlock::canPlaceOn(World* world, int x, int y, int z) const
{
    return world != nullptr && (world->shouldSuffocate(x, y, z) || (Block::FENCE != nullptr && world->getBlockId(x, y, z) == Block::FENCE->id));
}

bool TorchBlock::canPlaceAt(World* world, int x, int y, int z) const
{
    if (world == nullptr) {
        return false;
    }
    if (world->shouldSuffocate(x - 1, y, z)) {
        return true;
    }
    if (world->shouldSuffocate(x + 1, y, z)) {
        return true;
    }
    if (world->shouldSuffocate(x, y, z - 1)) {
        return true;
    }
    if (world->shouldSuffocate(x, y, z + 1)) {
        return true;
    }
    return canPlaceOn(world, x, y - 1, z);
}

void TorchBlock::onPlaced(World* world, int x, int y, int z, int direction)
{
    if (world == nullptr) {
        return;
    }
    int meta = world->getBlockMeta(x, y, z);
    if (direction == 1 && canPlaceOn(world, x, y - 1, z)) {
        meta = 5;
    }
    if (direction == 2 && world->shouldSuffocate(x, y, z + 1)) {
        meta = 4;
    }
    if (direction == 3 && world->shouldSuffocate(x, y, z - 1)) {
        meta = 3;
    }
    if (direction == 4 && world->shouldSuffocate(x + 1, y, z)) {
        meta = 2;
    }
    if (direction == 5 && world->shouldSuffocate(x - 1, y, z)) {
        meta = 1;
    }
    world->setBlockMeta(x, y, z, meta);
}

void TorchBlock::onPlaced(World* world, int x, int y, int z)
{
    if (world == nullptr) {
        return;
    }
    if (world->shouldSuffocate(x - 1, y, z)) {
        world->setBlockMeta(x, y, z, 1);
    } else if (world->shouldSuffocate(x + 1, y, z)) {
        world->setBlockMeta(x, y, z, 2);
    } else if (world->shouldSuffocate(x, y, z - 1)) {
        world->setBlockMeta(x, y, z, 3);
    } else if (world->shouldSuffocate(x, y, z + 1)) {
        world->setBlockMeta(x, y, z, 4);
    } else if (canPlaceOn(world, x, y - 1, z)) {
        world->setBlockMeta(x, y, z, 5);
    }
    breakIfCannotPlaceAt(world, x, y, z);
}

void TorchBlock::onTick(World* world, int x, int y, int z, JavaRandom& random)
{
    Block::onTick(world, x, y, z, random);
    if (world != nullptr && world->getBlockMeta(x, y, z) == 0) {
        onPlaced(world, x, y, z);
    }
}

void TorchBlock::neighborUpdate(World* world, int x, int y, int z, int /*id*/)
{
    if (world == nullptr || !breakIfCannotPlaceAt(world, x, y, z)) {
        return;
    }

    const int meta = world->getBlockMeta(x, y, z);
    bool shouldBreak = false;
    if (!world->shouldSuffocate(x - 1, y, z) && meta == 1) {
        shouldBreak = true;
    }
    if (!world->shouldSuffocate(x + 1, y, z) && meta == 2) {
        shouldBreak = true;
    }
    if (!world->shouldSuffocate(x, y, z - 1) && meta == 3) {
        shouldBreak = true;
    }
    if (!world->shouldSuffocate(x, y, z + 1) && meta == 4) {
        shouldBreak = true;
    }
    if (!canPlaceOn(world, x, y - 1, z) && meta == 5) {
        shouldBreak = true;
    }
    if (shouldBreak) {
        dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
        world->setBlock(x, y, z, 0);
    }
}

bool TorchBlock::breakIfCannotPlaceAt(World* world, int x, int y, int z)
{
    if (world == nullptr || canPlaceAt(world, x, y, z)) {
        return world != nullptr;
    }
    dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
    world->setBlock(x, y, z, 0);
    return false;
}

std::optional<net::minecraft::HitResult> TorchBlock::raycast(
    World* world, int x, int y, int z, net::minecraft::Vec3d startPos, net::minecraft::Vec3d endPos) const
{
    if (world == nullptr) {
        return std::nullopt;
    }

    const int meta = world->getBlockMeta(x, y, z) & 7;
    float f = 0.15f;
    if (meta == 1) {
        const_cast<TorchBlock*>(this)->setBoundingBox(0.0f, 0.2f, 0.5f - f, f * 2.0f, 0.8f, 0.5f + f);
    } else if (meta == 2) {
        const_cast<TorchBlock*>(this)->setBoundingBox(1.0f - f * 2.0f, 0.2f, 0.5f - f, 1.0f, 0.8f, 0.5f + f);
    } else if (meta == 3) {
        const_cast<TorchBlock*>(this)->setBoundingBox(0.5f - f, 0.2f, 0.0f, 0.5f + f, 0.8f, f * 2.0f);
    } else if (meta == 4) {
        const_cast<TorchBlock*>(this)->setBoundingBox(0.5f - f, 0.2f, 1.0f - f * 2.0f, 0.5f + f, 0.8f, 1.0f);
    } else {
        f = 0.1f;
        const_cast<TorchBlock*>(this)->setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.6f, 0.5f + f);
    }
    return Block::raycast(world, x, y, z, startPos, endPos);
}

void TorchBlock::randomDisplayTick(World* world, int x, int y, int z, JavaRandom& /*random*/)
{
    if (world == nullptr) {
        return;
    }

    const int meta = world->getBlockMeta(x, y, z);
    const double centerX = static_cast<double>(x) + 0.5;
    const double centerY = static_cast<double>(y) + 0.7;
    const double centerZ = static_cast<double>(z) + 0.5;
    constexpr double smokeOffset = 0.22;
    constexpr double flameOffset = 0.27;
    if (meta == 1) {
        world->addParticle("smoke", centerX - flameOffset, centerY + smokeOffset, centerZ, 0.0, 0.0, 0.0);
        world->addParticle("flame", centerX - flameOffset, centerY + smokeOffset, centerZ, 0.0, 0.0, 0.0);
    } else if (meta == 2) {
        world->addParticle("smoke", centerX + flameOffset, centerY + smokeOffset, centerZ, 0.0, 0.0, 0.0);
        world->addParticle("flame", centerX + flameOffset, centerY + smokeOffset, centerZ, 0.0, 0.0, 0.0);
    } else if (meta == 3) {
        world->addParticle("smoke", centerX, centerY + smokeOffset, centerZ - flameOffset, 0.0, 0.0, 0.0);
        world->addParticle("flame", centerX, centerY + smokeOffset, centerZ - flameOffset, 0.0, 0.0, 0.0);
    } else if (meta == 4) {
        world->addParticle("smoke", centerX, centerY + smokeOffset, centerZ + flameOffset, 0.0, 0.0, 0.0);
        world->addParticle("flame", centerX, centerY + smokeOffset, centerZ + flameOffset, 0.0, 0.0, 0.0);
    } else {
        world->addParticle("smoke", centerX, static_cast<double>(y) + 0.7, centerZ, 0.0, 0.0, 0.0);
        world->addParticle("flame", centerX, static_cast<double>(y) + 0.7, centerZ, 0.0, 0.0, 0.0);
    }
}
namespace {

void TorchBlock::registerClass()
{
    Block::TORCH = (new TorchBlock(50, 80))->setHardness(0.0f)->setLuminance(0.9375f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("torch")->ignoreMetaUpdates();
}




static ::net::minecraft::registry::RegisterBlock<TorchBlock> autoReg(50);
} // namespace
} // namespace net::minecraft::block

