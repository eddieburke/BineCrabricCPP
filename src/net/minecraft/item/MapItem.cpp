#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/MapItem.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/block/MapColor.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/map/MapState.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/network/packet/InventoryPackets.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"

#include <memory>
#include <typeindex>

namespace net::minecraft::item {

map::MapState* MapItem::getSavedMapState(ItemStack& stack, World* world)
{
    if (world == nullptr) {
        return nullptr;
    }

    const std::string stateId = "map_" + std::to_string(stack.getDamage());
    map::MapState* mapState = dynamic_cast<map::MapState*>(
        world->persistentStateManager.getOrCreate(typeid(map::MapState), stateId));
    if (mapState == nullptr) {
        stack.setDamage(world->persistentStateManager.getIdCount("map"));
        const std::string newStateId = "map_" + std::to_string(stack.getDamage());
        auto newState = std::make_unique<map::MapState>(newStateId);
        newState->centerX = world->getProperties().getSpawnX();
        newState->centerZ = world->getProperties().getSpawnZ();
        newState->scale = 3;
        newState->dimension =
            world->dimension != nullptr ? static_cast<std::uint8_t>(world->dimension->id) : 0;
        newState->markDirty();
        mapState = newState.get();
        world->persistentStateManager.set(newStateId, std::move(newState));
    }
    return mapState;
}

map::MapState* MapItem::getMapState(std::int16_t mapId, World* world)
{
    if (world == nullptr) {
        return nullptr;
    }
    return world->getOrCreateState<map::MapState>("map_" + std::to_string(mapId));
}

void MapItem::update(World* world, Entity* entity, map::MapState* mapState)
{
    if (world == nullptr || entity == nullptr || mapState == nullptr) {
        return;
    }
    if (world->dimension == nullptr || world->dimension->id != static_cast<int>(mapState->dimension)) {
        return;
    }

    constexpr int mapSize = 128;
    const int scaleFactor = 1 << mapState->scale;
    const int centerX = mapState->centerX;
    const int centerZ = mapState->centerZ;
    const int entityMapX = MathHelper::floor(entity->x - static_cast<double>(centerX)) / scaleFactor + mapSize / 2;
    const int entityMapZ = MathHelper::floor(entity->z - static_cast<double>(centerZ)) / scaleFactor + mapSize / 2;
    int sampleRadius = mapSize / scaleFactor;
    if (world->dimension->hasCeiling) {
        sampleRadius /= 2;
    }

    ++mapState->inventoryTicks;
    for (int mapX = entityMapX - sampleRadius + 1; mapX < entityMapX + sampleRadius; ++mapX) {
        if ((mapX & 0xF) != (mapState->inventoryTicks & 0xF)) {
            continue;
        }
        int minDirtyZ = 255;
        int maxDirtyZ = 0;
        double previousHeightAverage = 0.0;
        for (int mapZ = entityMapZ - sampleRadius - 1; mapZ < entityMapZ + sampleRadius; ++mapZ) {
            if (mapX < 0 || mapZ < -1 || mapX >= mapSize || mapZ >= mapSize) {
                continue;
            }
            const int offsetX = mapX - entityMapX;
            const int offsetZ = mapZ - entityMapZ;
            const bool outsideCircle =
                offsetX * offsetX + offsetZ * offsetZ > (sampleRadius - 2) * (sampleRadius - 2);
            const int worldX = (centerX / scaleFactor + mapX - mapSize / 2) * scaleFactor;
            const int worldZ = (centerZ / scaleFactor + mapZ - mapSize / 2) * scaleFactor;

            double heightAverage = 0.0;
            int dominantBlockId = 0;
            int dominantCount = 0;
            std::array<int, 256> blockCounts {};
            Chunk& chunk = world->getChunkFromPos(worldX, worldZ);
            const int localX = worldX & 0xF;
            const int localZ = worldZ & 0xF;
            int fluidDepth = 0;

            if (world->dimension->hasCeiling) {
                const int hash = worldX + worldZ * 231871;
                const int mixed = hash * hash * 31287121 + hash * 11;
                if (((mixed >> 20) & 1) == 0) {
                    const int dirtId = Block::DIRT != nullptr ? Block::DIRT->id : 3;
                    blockCounts[static_cast<std::size_t>(dirtId)] += 10;
                } else {
                    const int stoneId = Block::STONE != nullptr ? Block::STONE->id : 1;
                    blockCounts[static_cast<std::size_t>(stoneId)] += 10;
                }
                heightAverage = 100.0;
            } else {
                for (int sampleX = 0; sampleX < scaleFactor; ++sampleX) {
                    for (int sampleZ = 0; sampleZ < scaleFactor; ++sampleZ) {
                        int height = chunk.getHeight(localX + sampleX, localZ + sampleZ) + 1;
                        int blockId = 0;
                        if (height > 1) {
                            bool foundSolid = false;
                            do {
                                foundSolid = true;
                                blockId = chunk.getBlockId(localX + sampleX, height - 1, localZ + sampleZ);
                                if (blockId == 0) {
                                    foundSolid = false;
                                } else if (height > 0 && blockId > 0 && blockId < Block::BLOCK_COUNT
                                    && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr
                                    && &Block::BLOCKS[static_cast<std::size_t>(blockId)]->material.mapColor
                                        == &block::MapColor::CLEAR) {
                                    foundSolid = false;
                                }
                                if (!foundSolid) {
                                    blockId = chunk.getBlockId(localX + sampleX, --height - 1, localZ + sampleZ);
                                }
                            } while (!foundSolid);
                            if (blockId != 0 && blockId < Block::BLOCK_COUNT
                                && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr
                                && Block::BLOCKS[static_cast<std::size_t>(blockId)]->material.isFluid()) {
                                int fluidY = height - 1;
                                int fluidBlockId = 0;
                                do {
                                    fluidBlockId = chunk.getBlockId(localX + sampleX, fluidY--, localZ + sampleZ);
                                    ++fluidDepth;
                                } while (fluidY > 0 && fluidBlockId != 0 && fluidBlockId < Block::BLOCK_COUNT
                                    && Block::BLOCKS[static_cast<std::size_t>(fluidBlockId)] != nullptr
                                    && Block::BLOCKS[static_cast<std::size_t>(fluidBlockId)]->material.isFluid());
                            }
                        }
                        heightAverage += static_cast<double>(height) / static_cast<double>(scaleFactor * scaleFactor);
                        ++blockCounts[static_cast<std::size_t>(blockId)];
                    }
                }
            }

            for (int blockId = 0; blockId < 256; ++blockId) {
                if (blockCounts[static_cast<std::size_t>(blockId)] > dominantCount) {
                    dominantBlockId = blockId;
                    dominantCount = blockCounts[static_cast<std::size_t>(blockId)];
                }
            }

            double shade = (heightAverage - previousHeightAverage) * 4.0 / static_cast<double>(scaleFactor + 4)
                + (static_cast<double>((mapX + mapZ) & 1) - 0.5) * 0.4;
            int shadeIndex = 1;
            if (shade > 0.6) {
                shadeIndex = 2;
            } else if (shade < -0.6) {
                shadeIndex = 0;
            }

            int mapColorId = 0;
            if (dominantBlockId > 0 && dominantBlockId < Block::BLOCK_COUNT
                && Block::BLOCKS[static_cast<std::size_t>(dominantBlockId)] != nullptr) {
                const block::MapColor& mapColor =
                    Block::BLOCKS[static_cast<std::size_t>(dominantBlockId)]->material.mapColor;
                if (&mapColor == &block::MapColor::BLUE) {
                    const double fluidShade =
                        static_cast<double>(fluidDepth) * 0.1 + static_cast<double>((mapX + mapZ) & 1) * 0.2;
                    shadeIndex = 1;
                    if (fluidShade < 0.5) {
                        shadeIndex = 2;
                    }
                    if (fluidShade > 0.9) {
                        shadeIndex = 0;
                    }
                }
                mapColorId = mapColor.id;
            }

            previousHeightAverage = heightAverage;
            if (mapZ < 0 || (outsideCircle && ((mapX + mapZ) & 1) == 0)) {
                continue;
            }
            const std::uint8_t nextColor =
                static_cast<std::uint8_t>(mapColorId * 4 + shadeIndex);
            const std::size_t colorIndex = static_cast<std::size_t>(mapX + mapZ * mapSize);
            if (mapState->colors[colorIndex] == nextColor) {
                continue;
            }
            if (minDirtyZ > mapZ) {
                minDirtyZ = mapZ;
            }
            if (maxDirtyZ < mapZ) {
                maxDirtyZ = mapZ;
            }
            mapState->colors[colorIndex] = nextColor;
        }
        if (minDirtyZ <= maxDirtyZ) {
            mapState->markDirtyColumn(mapX, minDirtyZ, maxDirtyZ);
        }
    }
}

void MapItem::inventoryTick(ItemStack* stack, World* world, Entity* entity, int /*slot*/, bool selected)
{
    if (world == nullptr || stack == nullptr || entity == nullptr || world->isRemote()) {
        return;
    }
    map::MapState* mapState = getSavedMapState(*stack, world);
    if (mapState == nullptr) {
        return;
    }
    if (auto* player = dynamic_cast<PlayerEntity*>(entity); player != nullptr) {
        mapState->update(player, *stack);
    }
    if (selected) {
        update(world, entity, mapState);
    }
}

Packet* MapItem::getUpdatePacket(ItemStack* stack, World* world, PlayerEntity* player)
{
    if (stack == nullptr || world == nullptr || player == nullptr || Item::byRawId(102) == nullptr) {
        return nullptr;
    }
    map::MapState* mapState = getSavedMapState(*stack, world);
    if (mapState == nullptr) {
        return nullptr;
    }
    const std::vector<std::uint8_t> updateData = mapState->getPlayerMarkerPacket(*stack, player);
    if (updateData.empty()) {
        return nullptr;
    }
    auto* packet = new MapUpdateS2CPacket();
    packet->itemRawId = Item::byRawId(102)->id;
    packet->id = stack->getDamage();
    packet->updateData = updateData;
    return packet;
}

void MapItem::onCraft(ItemStack* stack, World* world, PlayerEntity* player)
{
    if (stack == nullptr || world == nullptr || player == nullptr) {
        return;
    }
    stack->setDamage(world->persistentStateManager.getIdCount("map"));
    const std::string stateId = "map_" + std::to_string(stack->getDamage());
    auto mapState = std::make_unique<map::MapState>(stateId);
    mapState->centerX = MathHelper::floor(player->x);
    mapState->centerZ = MathHelper::floor(player->z);
    mapState->scale = 3;
    mapState->dimension =
        world->dimension != nullptr ? static_cast<std::uint8_t>(world->dimension->id) : 0;
    mapState->markDirty();
    world->persistentStateManager.set(stateId, std::move(mapState));
}

void MapItem::registerClass()
{
    static MapItem MAP(102);
    MAP.setTexturePosition(12, 3)->setTranslationKey("map");
}

void MapItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(102)),
        {std::string("###"), std::string("#X#"), std::string("###"), '#', Item::byRawId(83), 'X', Item::byRawId(89)});
}




namespace { static ::net::minecraft::registry::RegisterItem<MapItem> autoReg(102); } // namespace

} // namespace net::minecraft::item
