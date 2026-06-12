#include "seedfinder/probe/DecorateProbe.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/LegacyChunkCache.hpp"
#include "net/minecraft/world/storage/EmptyWorldStorage.hpp"

#include <algorithm>
#include <array>
#include <memory>

namespace nm = net::minecraft;

namespace seedfinder {

namespace {

// Beta 1.7.3 block ids — fixed, so the decorate counter doesn't depend on Block:: static
// names that may vary in the port.
enum BlockId : int {
    Grass = 2,
    Dirt = 3,
    GoldOre = 14,
    IronOre = 15,
    CoalOre = 16,
    Log = 17,
    FlowingWater = 8,
    StillWater = 9,
    FlowingLava = 10,
    StillLava = 11,
    Sand = 12,
    LapisOre = 21,
    Dandelion = 37,
    Rose = 38,
    BrownMushroom = 39,
    RedMushroom = 40,
    MobSpawner = 52,
    DiamondOre = 56,
    RedstoneOre = 73,
    LitRedstoneOre = 74,
    SnowLayer = 78,
    Cactus = 81,
    Clay = 82,
    Reeds = 83,
    Pumpkin = 86,
};

int rawBlock(const nm::Chunk& chunk, int lx, int y, int lz)
{
    return static_cast<int>(chunk.blocks[static_cast<std::size_t>((lx << 11) | (lz << 7) | y)] & 0xFFU);
}

bool isGround(int id)
{
    return id == Dirt || id == Grass || id == Sand;
}

void tallyChunk(const nm::Chunk& chunk, ProbeResult& out)
{
    std::uint32_t maxCactus = out.max_cactus_height;
    for (int lx = 0; lx < 16; ++lx) {
        for (int lz = 0; lz < 16; ++lz) {
            int cactusRun = 0;
            bool sawTrunk = false;
            for (int y = 0; y < nm::Chunk::height; ++y) {
                const int id = rawBlock(chunk, lx, y, lz);
                switch (id) {
                case CoalOre: ++out.coal_ore; break;
                case IronOre: ++out.iron_ore; break;
                case GoldOre: ++out.gold_ore; break;
                case RedstoneOre:
                case LitRedstoneOre: ++out.redstone_ore; break;
                case LapisOre: ++out.lapis_ore; break;
                case Clay: ++out.clay; break;
                case MobSpawner: ++out.dungeons; break;
                case Reeds: ++out.sugar_cane; break;
                case Pumpkin: ++out.pumpkins; break;
                case BrownMushroom: ++out.brown_mushrooms; break;
                case RedMushroom: ++out.red_mushrooms; break;
                case Dandelion:
                case Rose: ++out.flowers; break;
                case SnowLayer: ++out.snow_blocks; break;
                case FlowingWater: ++out.water_springs; break;
                case FlowingLava: ++out.lava_springs; break;
                case DiamondOre: {
                    ++out.diamond_ore;
                    // Exposed if any 6-neighbour is air (chunk-local; borders treated as
                    // not exposed — a faithful-enough proxy for "diamonds near a cave").
                    const bool exposed =
                        (y + 1 < nm::Chunk::height && rawBlock(chunk, lx, y + 1, lz) == 0)
                        || (y > 0 && rawBlock(chunk, lx, y - 1, lz) == 0)
                        || (lx > 0 && rawBlock(chunk, lx - 1, y, lz) == 0)
                        || (lx < 15 && rawBlock(chunk, lx + 1, y, lz) == 0)
                        || (lz > 0 && rawBlock(chunk, lx, y, lz - 1) == 0)
                        || (lz < 15 && rawBlock(chunk, lx, y, lz + 1) == 0);
                    if (exposed) {
                        ++out.diamonds_exposed_to_cave;
                    }
                    break;
                }
                case Cactus:
                    ++cactusRun;
                    maxCactus = std::max(maxCactus, static_cast<std::uint32_t>(cactusRun));
                    continue;
                case Log:
                    // Trunk base: a log resting on ground counts as one tree.
                    if (!sawTrunk && y > 0 && isGround(rawBlock(chunk, lx, y - 1, lz))) {
                        ++out.trees;
                        sawTrunk = true;
                    }
                    break;
                default:
                    break;
                }
                if (id != Cactus) {
                    cactusRun = 0;
                }
            }
        }
    }
    out.max_cactus_height = maxCactus;
}

} // namespace

void DecorateProbe::probe(std::uint64_t seed, int originChunkX, int originChunkZ, const PassPlan& plan, ProbeResult& out)
{
    if (plan.lastDecorStep <= 0 && !plan.needSpawn) {
        return;
    }

    auto storage = std::make_unique<nm::EmptyWorldStorage>();
    nm::World world(storage.get(), "seedfinder", static_cast<std::int64_t>(seed), true);
    world.setEventProcessingEnabled(true);

    auto* cache = dynamic_cast<nm::LegacyChunkCache*>(world.getChunkSource());
    if (cache == nullptr) {
        return;
    }
    cache->setSpawnPoint(originChunkX, originChunkZ);
    cache->setDecorateStep(plan.lastDecorStep > 0 ? plan.lastDecorStep : 7);

    if (plan.needSpawn) {
        world.initializeSpawnPoint();
        const nm::Vec3i spawn = world.getSpawnPos();
        out.spawn_valid = true;
        out.spawn_x = spawn.x;
        out.spawn_y = spawn.y;
        out.spawn_z = spawn.z;
        out.spawn_surface_block_id = static_cast<std::uint8_t>(world.getSpawnBlockId(spawn.x, spawn.z));
        if (nm::BiomeSource* source = world.getBiomeSource()) {
            out.spawn_biome_id = static_cast<std::uint8_t>(source->getBiome(spawn.x, spawn.z).id);
        }
        out.spawn_on_sand = out.spawn_surface_block_id == static_cast<std::uint8_t>(Sand);
        // Re-pin the cache window on the scan origin for the region tally below.
        cache->setSpawnPoint(originChunkX, originChunkZ);
    }

    if (plan.lastDecorStep > 0) {
        const int radius = std::min(plan.radiusChunks, kMaxDecorateRadius);
        for (int cz = originChunkZ - radius; cz <= originChunkZ + radius; ++cz) {
            for (int cx = originChunkX - radius; cx <= originChunkX + radius; ++cx) {
                cache->getChunk(cx, cz);
                cache->getChunk(cx + 1, cz);
                cache->getChunk(cx, cz + 1);
                cache->getChunk(cx + 1, cz + 1);
                cache->decorate(cache, cx, cz);
            }
        }
        for (int cz = originChunkZ - radius; cz <= originChunkZ + radius; ++cz) {
            for (int cx = originChunkX - radius; cx <= originChunkX + radius; ++cx) {
                tallyChunk(cache->getChunk(cx, cz), out);
            }
        }
    }

    out.passesRun |= (plan.passes & pass::Populate);
}

} // namespace seedfinder
