#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/VanillaBlockSounds.hpp"
#include "net/minecraft/block/LeavesBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/item/LeavesBlockItem.hpp"
#include "net/minecraft/item/ShearsItem.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/world/World.hpp"

#include <array>
#include <cstdint>

namespace net::minecraft::block {

namespace {

constexpr int kDecayRadius = 4;
constexpr int kRegionSize = kDecayRadius * 2 + 1;
constexpr int kRegionArea = kRegionSize * kRegionSize;
constexpr int kRegionVolume = kRegionArea * kRegionSize;
constexpr int8_t kBlocked = -1;
constexpr int8_t kPendingLeaf = -2;

inline int regionIndex(int dx, int dy, int dz)
{
    return (dx + kDecayRadius) * kRegionArea + (dy + kDecayRadius) * kRegionSize + (dz + kDecayRadius);
}

// BFS from logs through leaves within a (2*radius+1)^3 neighborhood. Returns distance to
// the nearest log (0-4), or -1 when the center leaf is not connected within radius.
int decayDistanceFromLog(World* world, int x, int y, int z)
{
    static thread_local std::array<int8_t, kRegionVolume> region {};
    static thread_local std::array<int, kRegionVolume> queue {};

    region.fill(kBlocked);

    int queueTail = 0;
    for (int dx = -kDecayRadius; dx <= kDecayRadius; ++dx) {
        for (int dy = -kDecayRadius; dy <= kDecayRadius; ++dy) {
            for (int dz = -kDecayRadius; dz <= kDecayRadius; ++dz) {
                const int blockId = world->getBlockId(x + dx, y + dy, z + dz);
                const int idx = regionIndex(dx, dy, dz);
                if (blockId == Block::LOG->id) {
                    region[static_cast<std::size_t>(idx)] = 0;
                    queue[static_cast<std::size_t>(queueTail++)] = idx;
                } else if (blockId == Block::LEAVES->id) {
                    region[static_cast<std::size_t>(idx)] = kPendingLeaf;
                }
            }
        }
    }

    for (int queueHead = 0; queueHead < queueTail; ++queueHead) {
        const int idx = queue[static_cast<std::size_t>(queueHead)];
        const int dist = region[static_cast<std::size_t>(idx)];
        if (dist >= kDecayRadius) {
            continue;
        }

        const int dz = (idx % kRegionSize) - kDecayRadius;
        const int tmp = idx / kRegionSize;
        const int dy = (tmp % kRegionSize) - kDecayRadius;
        const int dx = (tmp / kRegionSize) - kDecayRadius;

        static constexpr int offsets[6][3] = {
            {-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};
        for (const auto& offset : offsets) {
            const int nx = dx + offset[0];
            const int ny = dy + offset[1];
            const int nz = dz + offset[2];
            if (nx < -kDecayRadius || nx > kDecayRadius || ny < -kDecayRadius || ny > kDecayRadius
                || nz < -kDecayRadius || nz > kDecayRadius) {
                continue;
            }
            const int neighborIdx = regionIndex(nx, ny, nz);
            if (region[static_cast<std::size_t>(neighborIdx)] != kPendingLeaf) {
                continue;
            }
            region[static_cast<std::size_t>(neighborIdx)] = static_cast<int8_t>(dist + 1);
            queue[static_cast<std::size_t>(queueTail++)] = neighborIdx;
        }
    }

    const int center = region[static_cast<std::size_t>(regionIndex(0, 0, 0))];
    return center >= 0 ? center : -1;
}

} // namespace

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

    const int loadedRadius = kDecayRadius + 1;
    if (!world->isRegionLoaded(x - loadedRadius, y - loadedRadius, z - loadedRadius, x + loadedRadius,
            y + loadedRadius, z + loadedRadius)) {
        return;
    }

    const int decayDistance = decayDistanceFromLog(world, x, y, z);
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
    if (Item::byRawId(103) != nullptr && hand.itemId == Item::byRawId(103)->id) {
        player->increaseStat(stat::Stats::mineBlockStatId(id), 1);
        dropStack(world, x, y, z, ItemStack(id, 1, meta & 3));
        return;
    }
    Block::afterBreak(world, player, x, y, z, meta);
}
void LeavesBlock::registerClass()
{
    Block::LEAVES = (new LeavesBlock(kBlockId, 52))->setHardness(0.2f)->setOpacity(1)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("leaves")->disableTrackingStatistics()->ignoreMetaUpdates();
}




void LeavesBlock::registerBlockItems()
{
    (new item::LeavesBlockItem(18 - 256))->setTranslationKey("leaves");
}

MC_REGISTER_BLOCK(LeavesBlock)
} // namespace net::minecraft::block

