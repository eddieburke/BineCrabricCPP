#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/VanillaBlockSounds.hpp"
#include "net/minecraft/block/GrassBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

GrassBlock::GrassBlock(int blockId) : Block(blockId, material::Material::SOLID_ORGANIC)
{
    textureId = 3;
    setTickRandomly(true);
}

int GrassBlock::getDroppedItemId(int /*blockMeta*/, JavaRandom& random) const
{
    if (Block::DIRT != nullptr) {
        return Block::DIRT->getDroppedItemId(0, random);
    }
    return 3;
}

int GrassBlock::getTextureId(const BlockView* blockView, int x, int y, int z, int side) const
{
    if (side == 1) {
        return 0;
    }
    if (side == 0) {
        return 2;
    }
    if (blockView != nullptr) {
        const material::Material& topMaterial = blockView->getMaterial(x, y + 1, z);
        if (&topMaterial == &material::Material::SNOW_LAYER || &topMaterial == &material::Material::SNOW_BLOCK) {
            return 68;
        }
    }
    return 3;
}

int GrassBlock::getColorMultiplier(const BlockView* blockView, int x, int /*y*/, int z) const
{
    if (blockView == nullptr) {
        return 0xFFFFFF;
    }
    net::minecraft::BiomeSource* biomeSource = blockView->getBiomeSource();
    if (biomeSource == nullptr) {
        return 0xFFFFFF;
    }
    std::vector<net::minecraft::Biome*> scratch;
    biomeSource->getBiomesInArea(scratch, x, z, 1, 1);
    const auto& temperatureMap = biomeSource->temperatureMap();
    const auto& downfallMap = biomeSource->downfallMap();
    if (temperatureMap.empty() || downfallMap.empty()) {
        return 0xFFFFFF;
    }
    return net::minecraft::client::color::world::GrassColors::getColor(temperatureMap[0], downfallMap[0]);
}

void GrassBlock::onTick(World* world, int x, int y, int z, JavaRandom& random)
{
    if (world == nullptr || world->isRemote() || Block::DIRT == nullptr || Block::GRASS_BLOCK == nullptr) {
        return;
    }
    const int lightAbove = world->getLightLevelAbove(x, y, z);
    const int blockAboveId = world->getBlockId(x, y + 1, z);
    if (lightAbove < 4 && Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(blockAboveId)] > 2) {
        if (random.nextInt(4) != 0) {
            return;
        }
        world->setBlock(x, y, z, Block::DIRT->id);
    } else if (lightAbove >= 9) {
        const int spreadX = x + random.nextInt(3) - 1;
        const int spreadY = y + random.nextInt(5) - 3;
        const int spreadZ = z + random.nextInt(3) - 1;
        const int aboveId = world->getBlockId(spreadX, spreadY + 1, spreadZ);
        if (world->getBlockId(spreadX, spreadY, spreadZ) == Block::DIRT->id
            && world->getLightLevelAbove(spreadX, spreadY, spreadZ) >= 4
            && Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(aboveId)] <= 2) {
            world->setBlock(spreadX, spreadY, spreadZ, Block::GRASS_BLOCK->id);
        }
    }
}
void GrassBlock::registerClass()
{
    Block::GRASS_BLOCK = (new GrassBlock(kBlockId))->setHardness(0.6f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("grass");
}




MC_REGISTER_BLOCK(GrassBlock)
} // namespace net::minecraft::block

