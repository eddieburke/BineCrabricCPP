#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/LeavesBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

LeavesBlock::LeavesBlock(int blockId, int textureId)
    : TransparentBlock(blockId, textureId, material::Material::LEAVES, false)
{
    spriteIndex = textureId;
    setTickRandomly(true);
}

int LeavesBlock::getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const
{
    return Block::SAPLING != nullptr ? Block::SAPLING->id : 6;
}

int LeavesBlock::getDroppedItemCount(JavaRandom& random) const
{
    return random.nextInt(20) == 0 ? 1 : 0;
}

int LeavesBlock::getTexture(int /*side*/, int meta) const
{
    if ((meta & 3) == 1) {
        return textureId + 80;
    }
    return textureId;
}

int LeavesBlock::getColor(int meta) const
{
    if ((meta & 1) == 1) {
        return net::minecraft::client::color::world::FoliageColors::getSpruceColor();
    }
    if ((meta & 2) == 2) {
        return net::minecraft::client::color::world::FoliageColors::getBirchColor();
    }
    return net::minecraft::client::color::world::FoliageColors::getDefaultColor();
}

int LeavesBlock::getColorMultiplier(const BlockView* blockView, int x, int y, int z) const
{
    if (blockView == nullptr) {
        return net::minecraft::client::color::world::FoliageColors::getDefaultColor();
    }
    const int meta = blockView->getBlockMeta(x, y, z);
    if ((meta & 1) == 1) {
        return net::minecraft::client::color::world::FoliageColors::getSpruceColor();
    }
    if ((meta & 2) == 2) {
        return net::minecraft::client::color::world::FoliageColors::getBirchColor();
    }
    net::minecraft::BiomeSource* biomeSource = blockView->getBiomeSource();
    if (biomeSource == nullptr) {
        return net::minecraft::client::color::world::FoliageColors::getDefaultColor();
    }
    std::vector<net::minecraft::BiomeInfo> scratch;
    biomeSource->getBiomesInArea(scratch, x, z, 1, 1);
    const auto& temperatureMap = biomeSource->temperatureMap();
    const auto& downfallMap = biomeSource->downfallMap();
    if (temperatureMap.empty() || downfallMap.empty()) {
        return net::minecraft::client::color::world::FoliageColors::getDefaultColor();
    }
    return net::minecraft::client::color::world::FoliageColors::getColor(temperatureMap[0], downfallMap[0]);
}

void LeavesBlock::setFancyGraphics(bool fancy)
{
    renderSides = fancy;
    textureId = spriteIndex + (fancy ? 0 : 1);
}

void LeavesBlock::onBreak(World* world, int x, int y, int z)
{
    if (world == nullptr || Block::LEAVES == nullptr) {
        return;
    }
    constexpr int radius = 1;
    const int loadedRadius = radius + 1;
    if (!world->isRegionLoaded(x - loadedRadius, y - loadedRadius, z - loadedRadius, x + loadedRadius, y + loadedRadius,
            z + loadedRadius)) {
        return;
    }
    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dz = -radius; dz <= radius; ++dz) {
                const int leafX = x + dx;
                const int leafY = y + dy;
                const int leafZ = z + dz;
                if (world->getBlockId(leafX, leafY, leafZ) != Block::LEAVES->id) {
                    continue;
                }
                const int leafMeta = world->getBlockMeta(leafX, leafY, leafZ);
                world->setBlockMetaWithoutNotifyingNeighbors(leafX, leafY, leafZ, leafMeta | 8);
            }
        }
    }
}

void LeavesBlock::onTick(World* world, int x, int y, int z, JavaRandom& /*random*/)
{
    if (world == nullptr || world->isRemote() || Block::LEAVES == nullptr || Block::LOG == nullptr) {
        return;
    }
    const int meta = world->getBlockMeta(x, y, z);
    if ((meta & 8) == 0) {
        return;
    }

    constexpr int radius = 4;
    const int loadedRadius = radius + 1;
    constexpr int regionSize = 32;
    constexpr int regionVolume = regionSize * regionSize * regionSize;
    constexpr int regionHalf = regionSize / 2;
    const int regionArea = regionSize * regionSize;

    if (decayRegion_.size() != static_cast<std::size_t>(regionVolume)) {
        decayRegion_.assign(static_cast<std::size_t>(regionVolume), 0);
    }

    if (world->isRegionLoaded(x - loadedRadius, y - loadedRadius, z - loadedRadius, x + loadedRadius, y + loadedRadius,
            z + loadedRadius)) {
        for (int dx = -radius; dx <= radius; ++dx) {
            for (int dy = -radius; dy <= radius; ++dy) {
                for (int dz = -radius; dz <= radius; ++dz) {
                    const int blockId = world->getBlockId(x + dx, y + dy, z + dz);
                    const std::size_t index = static_cast<std::size_t>((dx + regionHalf) * regionArea
                        + (dy + regionHalf) * regionSize + (dz + regionHalf));
                    if (blockId == Block::LOG->id) {
                        decayRegion_[index] = 0;
                    } else if (blockId == Block::LEAVES->id) {
                        decayRegion_[index] = -2;
                    } else {
                        decayRegion_[index] = -1;
                    }
                }
            }
        }

        for (int distance = 1; distance <= 4; ++distance) {
            for (int dx = -radius; dx <= radius; ++dx) {
                for (int dy = -radius; dy <= radius; ++dy) {
                    for (int dz = -radius; dz <= radius; ++dz) {
                        const std::size_t center = static_cast<std::size_t>((dx + regionHalf) * regionArea
                            + (dy + regionHalf) * regionSize + (dz + regionHalf));
                        if (decayRegion_[center] != distance - 1) {
                            continue;
                        }
                        const int offsets[6][3] = {
                            {-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};
                        for (const auto& offset : offsets) {
                            const std::size_t neighbor = static_cast<std::size_t>(
                                (dx + offset[0] + regionHalf) * regionArea + (dy + offset[1] + regionHalf) * regionSize
                                + (dz + offset[2] + regionHalf));
                            if (decayRegion_[neighbor] == -2) {
                                decayRegion_[neighbor] = distance;
                            }
                        }
                    }
                }
            }
        }
    }

    const std::size_t centerIndex = static_cast<std::size_t>(regionHalf * regionArea + regionHalf * regionSize + regionHalf);
    const int decayDistance = decayRegion_[centerIndex];
    if (decayDistance >= 0) {
        world->setBlockMetaWithoutNotifyingNeighbors(x, y, z, meta & 0xFFFFFFF7);
    } else {
        breakLeaves(world, x, y, z);
    }
}

void LeavesBlock::breakLeaves(World* world, int x, int y, int z)
{
    dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
    world->setBlock(x, y, z, 0);
}

void LeavesBlock::afterBreak(
    World* world,
    net::minecraft::PlayerEntity* player,
    int x,
    int y,
    int z,
    int meta)
{
    if (world == nullptr || world->isRemote() || player == nullptr) {
        Block::afterBreak(world, player, x, y, z, meta);
        return;
    }
    const ItemStack hand = player->getHand();
    if (Item::SHEARS != nullptr && hand.itemId == Item::SHEARS->id) {
        player->increaseStat(stat::Stats::mineBlockStatId(id), 1);
        dropStack(world, x, y, z, ItemStack(id, 1, meta & 3));
        return;
    }
    Block::afterBreak(world, player, x, y, z, meta);
}
namespace {

void registerLeavesBlock()
{
    Block::LEAVES = (new LeavesBlock(18, 52))->setHardness(0.2f)->setOpacity(1)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("leaves")->disableTrackingStatistics()->ignoreMetaUpdates();
}

MINECRAFT_REGISTER_BLOCK(registerLeavesBlock, 18);

} // namespace
} // namespace net::minecraft::block

