#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/FireBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/NetherPortalBlock.hpp"
#include "net/minecraft/block/TntBlock.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

FireBlock::FireBlock(int id, int textureId) : Block(id, textureId, material::Material::FIRE)
{
    setTickRandomly(true);
}

void FireBlock::init()
{
    if (flammablesInitialized_) {
        return;
    }
    flammablesInitialized_ = true;
    registerFlammableBlock(5, 5, 20);    // PLANKS
    registerFlammableBlock(85, 5, 20);   // FENCE
    registerFlammableBlock(53, 5, 20);   // WOODEN_STAIRS
    registerFlammableBlock(17, 5, 5);    // LOG
    registerFlammableBlock(18, 30, 60);  // LEAVES
    registerFlammableBlock(47, 30, 20);  // BOOKSHELF
    registerFlammableBlock(46, 15, 100); // TNT
    registerFlammableBlock(31, 60, 100); // GRASS (tall grass plant)
    registerFlammableBlock(35, 30, 60);  // WOOL
}

void FireBlock::registerFlammableBlock(int block, int burnChance, int spreadChance)
{
    burnChances_[static_cast<std::size_t>(block)] = burnChance;
    spreadChances_[static_cast<std::size_t>(block)] = spreadChance;
}

bool FireBlock::isFlammable(const BlockView* blockView, int x, int y, int z)
{
    return isFlammableBlock(blockView, x, y, z);
}

bool FireBlock::isFlammableBlock(const BlockView* blockView, int x, int y, int z)
{
    if (blockView == nullptr || Block::FIRE == nullptr) {
        return false;
    }
    return static_cast<const FireBlock*>(Block::FIRE)->burnChances_[static_cast<std::size_t>(blockView->getBlockId(x, y, z))] > 0;
}

int FireBlock::getBurnChance(World* world, int x, int y, int z, int currentChance) const
{
    if (world == nullptr) {
        return currentChance;
    }
    const int chance = burnChances_[static_cast<std::size_t>(world->getBlockId(x, y, z))];
    return chance > currentChance ? chance : currentChance;
}

bool FireBlock::areBlocksAroundFlammable(World* world, int x, int y, int z) const
{
    return isFlammableBlock(world, x + 1, y, z) || isFlammableBlock(world, x - 1, y, z)
        || isFlammableBlock(world, x, y - 1, z) || isFlammableBlock(world, x, y + 1, z)
        || isFlammableBlock(world, x, y, z - 1) || isFlammableBlock(world, x, y, z + 1);
}

int FireBlock::getBurnChanceAt(World* world, int x, int y, int z) const
{
    if (world == nullptr || !world->isAir(x, y, z)) {
        return 0;
    }
    int chance = 0;
    chance = getBurnChance(world, x + 1, y, z, chance);
    chance = getBurnChance(world, x - 1, y, z, chance);
    chance = getBurnChance(world, x, y - 1, z, chance);
    chance = getBurnChance(world, x, y + 1, z, chance);
    chance = getBurnChance(world, x, y, z - 1, chance);
    chance = getBurnChance(world, x, y, z + 1, chance);
    return chance;
}

void FireBlock::trySpreadingFire(World* world, int x, int y, int z, int spreadFactor,
    JavaRandom& random, int currentAge) const
{
    if (world == nullptr) {
        return;
    }
    const int blockId = world->getBlockId(x, y, z);
    const int spreadChance = spreadChances_[static_cast<std::size_t>(blockId)];
    if (random.nextInt(spreadFactor) >= spreadChance) {
        return;
    }
    const bool isTnt = Block::TNT != nullptr && blockId == Block::TNT->id;
    if (random.nextInt(currentAge + 10) < 5 && !world->isRaining(x, y, z)) {
        int nextAge = currentAge + random.nextInt(5) / 4;
        if (nextAge > 15) {
            nextAge = 15;
        }
        world->setBlock(x, y, z, id, static_cast<std::uint8_t>(nextAge));
    } else {
        world->setBlock(x, y, z, 0);
    }
    if (isTnt && Block::TNT != nullptr) {
        Block::TNT->onMetadataChange(world, x, y, z, 1);
    }
}

bool FireBlock::canPlaceAt(World* world, int x, int y, int z, int /*side*/) const
{
    if (world == nullptr) {
        return false;
    }
    return world->shouldSuffocate(x, y - 1, z) || areBlocksAroundFlammable(world, x, y, z);
}

void FireBlock::onPlaced(World* world, int x, int y, int z)
{
    if (world == nullptr) {
        return;
    }
    if (Block::OBSIDIAN != nullptr && Block::NETHER_PORTAL != nullptr
        && world->getBlockId(x, y - 1, z) == Block::OBSIDIAN->id) {
        if (auto* portal = dynamic_cast<NetherPortalBlock*>(Block::NETHER_PORTAL)) {
            if (portal->create(world, x, y, z)) {
                return;
            }
        }
    }
    if (!world->shouldSuffocate(x, y - 1, z) && !areBlocksAroundFlammable(world, x, y, z)) {
        world->setBlock(x, y, z, 0);
        return;
    }
    world->scheduleBlockUpdate(x, y, z, id, getTickRate());
}

void FireBlock::neighborUpdate(World* world, int x, int y, int z, int /*id*/)
{
    if (world == nullptr) {
        return;
    }
    if (!world->shouldSuffocate(x, y - 1, z) && !areBlocksAroundFlammable(world, x, y, z)) {
        world->setBlock(x, y, z, 0);
    }
}

void FireBlock::onTick(World* world, int x, int y, int z, JavaRandom& random)
{
    if (world == nullptr) {
        return;
    }
    const bool onNetherrack = Block::NETHERRACK != nullptr && world->getBlockId(x, y - 1, z) == Block::NETHERRACK->id;
    if (!canPlaceAt(world, x, y, z, 0)) {
        world->setBlock(x, y, z, 0);
    }
    if (!onNetherrack && world->isRaining()
        && (world->isRaining(x, y, z) || world->isRaining(x - 1, y, z) || world->isRaining(x + 1, y, z)
            || world->isRaining(x, y, z - 1) || world->isRaining(x, y, z + 1))) {
        world->setBlock(x, y, z, 0);
        return;
    }

    int age = world->getBlockMeta(x, y, z);
    if (age < 15) {
        world->setBlockMetaWithoutNotifyingNeighbors(x, y, z, age + random.nextInt(3) / 2);
    }
    world->scheduleBlockUpdate(x, y, z, id, getTickRate());

    if (!onNetherrack && !areBlocksAroundFlammable(world, x, y, z)) {
        if (!world->shouldSuffocate(x, y - 1, z) || age > 3) {
            world->setBlock(x, y, z, 0);
        }
        return;
    }
    if (!onNetherrack && !isFlammableBlock(world, x, y - 1, z) && age == 15 && random.nextInt(4) == 0) {
        world->setBlock(x, y, z, 0);
        return;
    }

    trySpreadingFire(world, x + 1, y, z, 300, random, age);
    trySpreadingFire(world, x - 1, y, z, 300, random, age);
    trySpreadingFire(world, x, y - 1, z, 250, random, age);
    trySpreadingFire(world, x, y + 1, z, 250, random, age);
    trySpreadingFire(world, x, y, z - 1, 300, random, age);
    trySpreadingFire(world, x, y, z + 1, 300, random, age);

    for (int ix = x - 1; ix <= x + 1; ++ix) {
        for (int iz = z - 1; iz <= z + 1; ++iz) {
            for (int iy = y - 1; iy <= y + 4; ++iy) {
                if (ix == x && iy == y && iz == z) {
                    continue;
                }
                const int burnChance = getBurnChanceAt(world, ix, iy, iz);
                if (burnChance <= 0) {
                    continue;
                }
                int spreadOdds = (burnChance + 40) / (age + 30);
                if (spreadOdds <= 0) {
                    continue;
                }
                int distanceFactor = 100;
                if (iy > y + 1) {
                    distanceFactor += (iy - (y + 1)) * 100;
                }
                if (random.nextInt(distanceFactor) > spreadOdds) {
                    continue;
                }
                if (world->isRaining() && world->isRaining(ix, iy, iz)) {
                    continue;
                }
                if (world->isRaining(ix - 1, iy, z) || world->isRaining(ix + 1, iy, iz)
                    || world->isRaining(ix, iy, iz - 1) || world->isRaining(ix, iy, iz + 1)) {
                    continue;
                }
                int nextAge = age + random.nextInt(5) / 4;
                if (nextAge > 15) {
                    nextAge = 15;
                }
                world->setBlock(ix, iy, iz, id, static_cast<std::uint8_t>(nextAge));
            }
        }
    }
}

void FireBlock::randomDisplayTick(
    World* world, int x, int y, int z, JavaRandom& random)
{
    if (world == nullptr) {
        return;
    }
    if (random.nextInt(24) == 0) {
        world->playSound(
            static_cast<float>(x) + 0.5f,
            static_cast<float>(y) + 0.5f,
            static_cast<float>(z) + 0.5f,
            "fire.fire",
            1.0f + random.nextFloat(),
            random.nextFloat() * 0.7f + 0.3f);
    }
    if (world->shouldSuffocate(x, y - 1, z) || isFlammable(world, x, y - 1, z)) {
        for (int i = 0; i < 3; ++i) {
            const float px = static_cast<float>(x) + random.nextFloat();
            const float py = static_cast<float>(y) + random.nextFloat() * 0.5f + 0.5f;
            const float pz = static_cast<float>(z) + random.nextFloat();
            world->addParticle("largesmoke", px, py, pz, 0.0, 0.0, 0.0);
        }
        return;
    }
    if (isFlammable(world, x - 1, y, z)) {
        for (int i = 0; i < 2; ++i) {
            world->addParticle(
                "largesmoke",
                static_cast<float>(x) + random.nextFloat() * 0.1f,
                static_cast<float>(y) + random.nextFloat(),
                static_cast<float>(z) + random.nextFloat(),
                0.0,
                0.0,
                0.0);
        }
    }
    if (isFlammable(world, x + 1, y, z)) {
        for (int i = 0; i < 2; ++i) {
            world->addParticle(
                "largesmoke",
                static_cast<float>(x + 1) - random.nextFloat() * 0.1f,
                static_cast<float>(y) + random.nextFloat(),
                static_cast<float>(z) + random.nextFloat(),
                0.0,
                0.0,
                0.0);
        }
    }
    if (isFlammable(world, x, y, z - 1)) {
        for (int i = 0; i < 2; ++i) {
            world->addParticle(
                "largesmoke",
                static_cast<float>(x) + random.nextFloat(),
                static_cast<float>(y) + random.nextFloat(),
                static_cast<float>(z) + random.nextFloat() * 0.1f,
                0.0,
                0.0,
                0.0);
        }
    }
    if (isFlammable(world, x, y, z + 1)) {
        for (int i = 0; i < 2; ++i) {
            world->addParticle(
                "largesmoke",
                static_cast<float>(x) + random.nextFloat(),
                static_cast<float>(y) + random.nextFloat(),
                static_cast<float>(z + 1) - random.nextFloat() * 0.1f,
                0.0,
                0.0,
                0.0);
        }
    }
    if (isFlammable(world, x, y + 1, z)) {
        for (int i = 0; i < 2; ++i) {
            world->addParticle(
                "largesmoke",
                static_cast<float>(x) + random.nextFloat(),
                static_cast<float>(y + 1) - random.nextFloat() * 0.1f,
                static_cast<float>(z) + random.nextFloat(),
                0.0,
                0.0,
                0.0);
        }
    }
}
namespace {

void FireBlock::registerClass()
{
    Block::FIRE = (new FireBlock(51, 31))->setHardness(0.0f)->setLuminance(1.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("fire")->disableTrackingStatistics()->ignoreMetaUpdates();
}




static ::net::minecraft::registry::RegisterBlock<FireBlock> autoReg(51);
} // namespace
} // namespace net::minecraft::block

