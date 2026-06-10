#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/ChestBlockEntity.hpp"
#include "net/minecraft/block/entity/MobSpawnerBlockEntity.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"

namespace net::minecraft {

// Faithful 1:1 port of net.minecraft.world.gen.feature.DungeonFeature.
class DungeonFeature : public Feature {
public:
    bool generate(World* world, JavaRandom& random, int x, int y, int z) override
    {
        static constexpr int roomHeight = 3;
        const int roomWidth = random.nextInt(2) + 2;
        const int roomDepth = random.nextInt(2) + 2;
        int airCount = 0;
        for (int dx = x - roomWidth - 1; dx <= x + roomWidth + 1; ++dx) {
            for (int dy = y - 1; dy <= y + roomHeight + 1; ++dy) {
                for (int dz = z - roomDepth - 1; dz <= z + roomDepth + 1; ++dz) {
                    block::material::Material& material = world->getMaterial(dx, dy, dz);
                    if (dy == y - 1 && !material.isSolid()) {
                        return false;
                    }
                    if (dy == y + roomHeight + 1 && !material.isSolid()) {
                        return false;
                    }
                    if (dx != x - roomWidth - 1 && dx != x + roomWidth + 1 && dz != z - roomDepth - 1 && dz != z + roomDepth + 1) {
                        continue;
                    }
                    if (dy != y || !world->isAir(dx, dy, dz) || !world->isAir(dx, dy + 1, dz)) {
                        continue;
                    }
                    ++airCount;
                }
            }
        }
        if (airCount < 1 || airCount > 5) {
            return false;
        }

        for (int dx = x - roomWidth - 1; dx <= x + roomWidth + 1; ++dx) {
            for (int dy = y + roomHeight; dy >= y - 1; --dy) {
                for (int dz = z - roomDepth - 1; dz <= z + roomDepth + 1; ++dz) {
                    if (dx == x - roomWidth - 1 || dy == y - 1 || dz == z - roomDepth - 1 || dx == x + roomWidth + 1
                        || dy == y + roomHeight + 1 || dz == z + roomDepth + 1) {
                        if (dy >= 0 && !world->getMaterial(dx, dy - 1, dz).isSolid()) {
                            world->setBlock(dx, dy, dz, 0);
                            continue;
                        }
                        if (!world->getMaterial(dx, dy, dz).isSolid()) {
                            continue;
                        }
                        if (dy == y - 1 && random.nextInt(4) != 0) {
                            world->setBlock(dx, dy, dz, Block::MOSSY_COBBLESTONE->id);
                            continue;
                        }
                        world->setBlock(dx, dy, dz, Block::COBBLESTONE->id);
                        continue;
                    }
                    world->setBlock(dx, dy, dz, 0);
                }
            }
        }

        for (int attempt = 0; attempt < 2; ++attempt) {
            for (int tries = 0; tries < 3; ++tries) {
                const int chestX = x + random.nextInt(roomWidth * 2 + 1) - roomWidth;
                const int chestY = y;
                const int chestZ = z + random.nextInt(roomDepth * 2 + 1) - roomDepth;
                if (!world->isAir(chestX, chestY, chestZ)) {
                    continue;
                }
                int solidNeighbors = 0;
                if (world->getMaterial(chestX - 1, chestY, chestZ).isSolid()) {
                    ++solidNeighbors;
                }
                if (world->getMaterial(chestX + 1, chestY, chestZ).isSolid()) {
                    ++solidNeighbors;
                }
                if (world->getMaterial(chestX, chestY, chestZ - 1).isSolid()) {
                    ++solidNeighbors;
                }
                if (world->getMaterial(chestX, chestY, chestZ + 1).isSolid()) {
                    ++solidNeighbors;
                }
                if (solidNeighbors != 1) {
                    continue;
                }
                world->setBlock(chestX, chestY, chestZ, Block::CHEST->id);
                auto* chestBlockEntity = dynamic_cast<block::entity::ChestBlockEntity*>(world->getBlockEntity(chestX, chestY, chestZ));
                if (chestBlockEntity != nullptr) {
                    for (int slot = 0; slot < 8; ++slot) {
                        ItemStack itemStack = getRandomChestItem(random);
                        if (itemStack.empty()) {
                            continue;
                        }
                        chestBlockEntity->setStack(static_cast<std::size_t>(random.nextInt(static_cast<int>(chestBlockEntity->size()))), std::move(itemStack));
                    }
                }
                goto chestPlaced;
            }
        }
    chestPlaced:

        world->setBlock(x, y, z, Block::SPAWNER->id);
        if (auto* mobSpawnerBlockEntity = dynamic_cast<block::entity::MobSpawnerBlockEntity*>(world->getBlockEntity(x, y, z))) {
            mobSpawnerBlockEntity->setSpawnedEntityId(getRandomEntity(random));
        }
        return true;
    }

private:
    [[nodiscard]] static ItemStack getRandomChestItem(JavaRandom& random)
    {
        const int roll = random.nextInt(11);
        if (roll == 0) {
            return ItemStack(Item::byRawId(73)->id);
        }
        if (roll == 1) {
            return ItemStack(Item::byRawId(9)->id, random.nextInt(4) + 1);
        }
        if (roll == 2) {
            return ItemStack(Item::byRawId(41)->id);
        }
        if (roll == 3) {
            return ItemStack(Item::byRawId(40)->id, random.nextInt(4) + 1);
        }
        if (roll == 4) {
            return ItemStack(Item::byRawId(33)->id, random.nextInt(4) + 1);
        }
        if (roll == 5) {
            return ItemStack(Item::byRawId(31)->id, random.nextInt(4) + 1);
        }
        if (roll == 6) {
            return ItemStack(Item::byRawId(69)->id);
        }
        if (roll == 7 && random.nextInt(100) == 0) {
            return ItemStack(Item::byRawId(66)->id);
        }
        if (roll == 8 && random.nextInt(2) == 0) {
            return ItemStack(Item::byRawId(75)->id, random.nextInt(4) + 1);
        }
        if (roll == 9 && random.nextInt(10) == 0) {
            return ItemStack(Item::byRawId(2000)->id + random.nextInt(2));
        }
        if (roll == 10) {
            return ItemStack(Item::byRawId(95)->id, 1, 3);
        }
        return {};
    }

    [[nodiscard]] static std::string getRandomEntity(JavaRandom& random)
    {
        switch (random.nextInt(4)) {
        case 0:
            return "Skeleton";
        case 1:
            return "Zombie";
        case 2:
            return "Zombie";
        case 3:
            return "Spider";
        default:
            return "";
        }
    }
};

} // namespace net::minecraft
